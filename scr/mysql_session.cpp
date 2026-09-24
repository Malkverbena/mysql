/* mysql_session.cpp */

#include "mysql_session.h"

#include "godot_convert.h"
#include "mysql_async_operation.h"
#include "mysql_connection.h"
#include "mysql_error.h"
#include "mysql_params.h"
#include "mysql_pool.h"
#include "mysql_streaming_cursor.h"
#include "mysql_transaction.h"
#include "prepared_statement_cache.h"
#include "sql_script.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"

#include <boost/asio/cancel_after.hpp>
#include <boost/asio/post.hpp>
#include <boost/mysql/client_errc.hpp>
#include <boost/mysql/execution_state.hpp>
#include <boost/mysql/format_sql.hpp>
#include <boost/mysql/is_fatal_error.hpp>
#include <boost/mysql/rows_view.hpp>
#include <boost/mysql/statement.hpp>

#include <chrono>

namespace {

// Hands the result to the main thread, where `completed` is emitted. Through
// `callable_mp` rather than `call_deferred()` by method name, so `_complete` does not
// need to be bound, and a script cannot call it to fire `completed` spuriously. Like a
// deferred call by name, it is dropped if the operation was freed in the meantime.
void complete_deferred(const Ref<MySQLAsyncOperation> &p_operation, const Ref<MySQLResult> &p_result) {
	// From here on the connection is free: nothing below touches it again.
	p_operation->clear_running();
	callable_mp(p_operation.ptr(), &MySQLAsyncOperation::_complete).call_deferred(p_result);
}

// Hands a failed operation's result to the main thread, first dropping the connection if
// `p_connection_error` is fatal (see `MySQLConnection::drop()`), so the next call sees it
// disconnected instead of reading the leftovers of the failed operation as its own result.
// The drop is posted rather than run inline: it replaces the `any_connection` whose
// operation is completing right now, which must not be destroyed from inside its own
// completion handler. `running` stays set until the drop is done.
void fail_deferred(const Ref<MySQLAsyncOperation> &p_operation, boost::mysql::error_code p_connection_error, const Ref<MySQLResult> &p_result) {
	if (!boost::mysql::is_fatal_error(p_connection_error)) {
		complete_deferred(p_operation, p_result);
		return;
	}
	boost::asio::post(p_operation->connection->get_io_context(), [p_operation, p_connection_error, p_result]() {
		p_operation->connection->drop(p_connection_error);
		complete_deferred(p_operation, p_result);
	});
}

Dictionary make_not_connected_error() {
	return mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::not_connected), boost::mysql::diagnostics());
}

// The same error Boost.MySQL itself reports for a second operation on a busy connection.
Dictionary make_busy_error() {
	return mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::operation_in_progress), boost::mysql::diagnostics());
}

// The same error Boost.MySQL itself reports for a command sent while a streaming read
// (`start_execution` + `read_some_rows`) has not been read to the end.
Dictionary make_cursor_open_error() {
	return mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::engaged_in_multi_function), boost::mysql::diagnostics());
}

// Keeps the connection protocol in sync after a `max_result_bytes` overflow: the resultset
// (and any further ones) must be fully read before the next command, the same reason
// `MySQLStreamingCursor::close()` drains instead of abandoning the read midway. Errors while
// draining are not reported: the caller already has the real error (the limit overflow) to
// return.
void drain_execution_state(MySQLConnection &p_connection, boost::mysql::execution_state &p_state) {
	boost::mysql::error_code error;
	boost::mysql::diagnostics diagnostics;
	while (!p_state.complete()) {
		if (p_state.should_read_rows()) {
			p_connection.native().read_some_rows(p_state, error, diagnostics);
		} else { // should_read_head()
			p_connection.native().read_resultset_head(p_state, error, diagnostics);
		}
		if (error) {
			p_connection.drop_if_fatal(error);
			break;
		}
	}
}

// Blocking counterpart of the async chain in `start_async_execute()`/`AsyncHeadHandler`/
// `AsyncRowsHandler` below: reads the result incrementally instead of with a single
// `execute()` call, so a `max_result_bytes` overflow is caught as soon as it happens (bounding
// peak memory) instead of after Boost.MySQL has already materialized the whole result.
template <typename Request>
Ref<MySQLResult> execute_with_limit(MySQLConnection &p_connection, Request &&p_request, const Ref<MySQLConfig> &p_config) {
	boost::mysql::execution_state state;
	boost::mysql::error_code error;
	boost::mysql::diagnostics diagnostics;

	p_connection.native().start_execution(std::forward<Request>(p_request), state, error, diagnostics);
	if (error) {
		return MySQLResult::from_error(mysql_module::make_error_dict(p_connection.drop_if_fatal(error), diagnostics));
	}

	MySQLResult::Builder builder;
	builder.begin_resultset(state.meta());

	int64_t max_bytes = p_config->get_max_result_bytes();

	while (!state.complete()) {
		if (state.should_read_rows()) {
			boost::mysql::rows_view batch = p_connection.native().read_some_rows(state, error, diagnostics);
			if (error) {
				return MySQLResult::from_error(mysql_module::make_error_dict(p_connection.drop_if_fatal(error), diagnostics));
			}
			builder.add_rows(batch, state.meta(), p_config);
			if (max_bytes > 0 && (int64_t)builder.get_estimated_bytes() > max_bytes) {
				drain_execution_state(p_connection, state);
				return MySQLResult::from_error(mysql_module::make_client_error_dict(vformat("Result exceeds max_result_bytes (%d bytes).", (int64_t)max_bytes)));
			}
		} else { // should_read_head()
			builder.end_resultset(state.affected_rows(), state.last_insert_id(), state.info());
			p_connection.native().read_resultset_head(state, error, diagnostics);
			if (error) {
				return MySQLResult::from_error(mysql_module::make_error_dict(p_connection.drop_if_fatal(error), diagnostics));
			}
			builder.begin_resultset(state.meta());
		}
	}
	builder.end_resultset(state.affected_rows(), state.last_insert_id(), state.info());
	return builder.finish();
}

// Async counterpart of execute_with_limit() above: the same incremental read, but as a chain
// of completion handlers instead of a blocking loop, since Boost.MySQL's async operations
// only run on the I/O thread's `io_context::run()`. Every handler below re-enters the chain
// through async_advance() until execution_state::complete(), or finishes early on error or on
// a max_result_bytes overflow.
void async_advance(const Ref<MySQLAsyncOperation> &p_operation);
void async_drain_step(const Ref<MySQLAsyncOperation> &p_operation, const Dictionary &p_error, boost::mysql::error_code p_drain_error);

// Drains the rest of the result after a max_result_bytes overflow, the async counterpart of
// drain_execution_state(). Network errors while draining are not reported: the caller already
// has the real error (the limit overflow) to return.
struct AsyncDrainRowsHandler {
	Ref<MySQLAsyncOperation> operation;
	Dictionary error;
	void operator()(boost::mysql::error_code p_error, boost::mysql::rows_view p_rows) {
		async_drain_step(operation, error, p_error);
	}
};

struct AsyncDrainHeadHandler {
	Ref<MySQLAsyncOperation> operation;
	Dictionary error;
	void operator()(boost::mysql::error_code p_error) {
		async_drain_step(operation, error, p_error);
	}
};

void async_drain_step(const Ref<MySQLAsyncOperation> &p_operation, const Dictionary &p_error, boost::mysql::error_code p_drain_error) {
	if (p_drain_error || p_operation->exec_state.complete()) {
		fail_deferred(p_operation, p_drain_error, MySQLResult::from_error(p_error));
		return;
	}
	if (p_operation->exec_state.should_read_rows()) {
		AsyncDrainRowsHandler handler{ p_operation, p_error };
		p_operation->connection->native().async_read_some_rows(p_operation->exec_state, handler);
	} else { // should_read_head()
		AsyncDrainHeadHandler handler{ p_operation, p_error };
		p_operation->connection->native().async_read_resultset_head(p_operation->exec_state, handler);
	}
}

void issue_read_rows(const Ref<MySQLAsyncOperation> &p_operation);
void issue_read_head(const Ref<MySQLAsyncOperation> &p_operation);

// Completion of async_read_some_rows(): one batch of a resultset's rows.
struct AsyncRowsHandler {
	Ref<MySQLAsyncOperation> operation;
	void operator()(boost::mysql::error_code p_error, boost::mysql::rows_view p_rows) {
		if (p_error) {
			fail_deferred(operation, p_error, MySQLResult::from_error(mysql_module::make_error_dict(p_error, operation->diagnostics)));
			return;
		}
		operation->result_builder.add_rows(p_rows, operation->exec_state.meta(), operation->config);
		int64_t max_bytes = operation->config->get_max_result_bytes();
		if (max_bytes > 0 && (int64_t)operation->result_builder.get_estimated_bytes() > max_bytes) {
			Dictionary limit_error = mysql_module::make_client_error_dict(vformat("Result exceeds max_result_bytes (%d bytes).", (int64_t)max_bytes));
			async_drain_step(operation, limit_error, boost::mysql::error_code());
			return;
		}
		async_advance(operation);
	}
};

// Completion of async_start_execution() and async_read_resultset_head(): a resultset's head
// (column metadata), whether it is the first one or one further along a multi-resultset
// operation.
struct AsyncHeadHandler {
	Ref<MySQLAsyncOperation> operation;
	void operator()(boost::mysql::error_code p_error) {
		if (p_error) {
			fail_deferred(operation, p_error, MySQLResult::from_error(mysql_module::make_error_dict(p_error, operation->diagnostics)));
			return;
		}
		operation->result_builder.begin_resultset(operation->exec_state.meta());
		async_advance(operation);
	}
};

void issue_read_rows(const Ref<MySQLAsyncOperation> &p_operation) {
	AsyncRowsHandler handler{ p_operation };
	if (p_operation->timeout_ms > 0) {
		p_operation->connection->native().async_read_some_rows(p_operation->exec_state, p_operation->diagnostics, boost::asio::cancel_after(std::chrono::milliseconds(p_operation->timeout_ms), handler));
	} else {
		p_operation->connection->native().async_read_some_rows(p_operation->exec_state, p_operation->diagnostics, handler);
	}
}

void issue_read_head(const Ref<MySQLAsyncOperation> &p_operation) {
	AsyncHeadHandler handler{ p_operation };
	if (p_operation->timeout_ms > 0) {
		p_operation->connection->native().async_read_resultset_head(p_operation->exec_state, p_operation->diagnostics, boost::asio::cancel_after(std::chrono::milliseconds(p_operation->timeout_ms), handler));
	} else {
		p_operation->connection->native().async_read_resultset_head(p_operation->exec_state, p_operation->diagnostics, handler);
	}
}

void async_advance(const Ref<MySQLAsyncOperation> &p_operation) {
	if (p_operation->exec_state.complete()) {
		p_operation->result_builder.end_resultset(p_operation->exec_state.affected_rows(), p_operation->exec_state.last_insert_id(), p_operation->exec_state.info());
		complete_deferred(p_operation, p_operation->result_builder.finish());
		return;
	}
	if (p_operation->exec_state.should_read_rows()) {
		issue_read_rows(p_operation);
	} else { // should_read_head()
		p_operation->result_builder.end_resultset(p_operation->exec_state.affected_rows(), p_operation->exec_state.last_insert_id(), p_operation->exec_state.info());
		issue_read_head(p_operation);
	}
}

// `p_request` must stay valid until the operation ends: pass either the SQL string owned by
// the operation or a bound statement whose parameters are owned by the operation.
template <typename Request>
void start_async_execute(MySQLConnection &p_connection, Request &&p_request, const Ref<MySQLAsyncOperation> &p_operation, int p_timeout_ms) {
	p_operation->set_running();
	p_operation->connection = &p_connection;
	p_operation->timeout_ms = p_timeout_ms;
	AsyncHeadHandler handler{ p_operation };
	if (p_timeout_ms > 0) {
		p_connection.native().async_start_execution(std::forward<Request>(p_request), p_operation->exec_state, p_operation->diagnostics, boost::asio::cancel_after(std::chrono::milliseconds(p_timeout_ms), handler));
	} else {
		p_connection.native().async_start_execution(std::forward<Request>(p_request), p_operation->exec_state, p_operation->diagnostics, handler);
	}
}

} //namespace

MySQLSession::~MySQLSession() {
	if (io_thread.is_started()) {
		// The order matters: release the work guard and stop the `io_context` BEFORE
		// joining, so the thread leaves `run()` before the connection is destroyed.
		// Otherwise it would keep a dangling reference to the `io_context` and the
		// `any_connection` being destroyed.
		memdelete(io_work_guard);
		io_work_guard = nullptr;
		connection->get_io_context().stop();
		io_thread.wait_to_finish();
	}

	// The I/O thread is stopped at this point: if the operation was still running, it
	// never will finish, and the connection is left mid-operation.
	bool connection_healthy = !_is_async_busy();
	if (owner_pool.is_valid() && connection) {
		owner_pool->release(connection, connection_healthy);
	} else {
		memdelete(connection);
	}
	connection = nullptr;
}

void MySQLSession::_io_thread_main(void *p_session) {
	MySQLSession *session = static_cast<MySQLSession *>(p_session);
	session->connection->get_io_context().run();
}

void MySQLSession::_ensure_io_thread_started() {
	if (io_thread.is_started()) {
		return;
	}
	// A pooled connection may have had its `io_context` stopped by a previous session.
	connection->get_io_context().restart();
	io_work_guard = memnew(IOWorkGuard(boost::asio::make_work_guard(connection->get_io_context())));
	io_thread.start(&MySQLSession::_io_thread_main, this);
}

Ref<MySQLSession> MySQLSession::create_pooled(const Ref<MySQLConfig> &p_config, MySQLConnection *p_connection, const Ref<MySQLPool> &p_owner_pool) {
	Ref<MySQLSession> session;
	session.instantiate();
	session->config = p_config;
	session->connection = p_connection;
	session->owner_pool = p_owner_pool;
	return session;
}

void MySQLSession::set_config(const Ref<MySQLConfig> &p_config) {
	ERR_FAIL_COND_MSG(connection != nullptr, "MySQLSession: The config cannot be changed after the connection has been created.");
	ERR_FAIL_COND_MSG(p_config.is_null(), "MySQLSession: The config cannot be null.");
	config = p_config;
	connection = memnew(MySQLConnection(config));
}

bool MySQLSession::_is_async_busy() const {
	return pending_async_operation.is_valid() && pending_async_operation->is_running();
}

Dictionary MySQLSession::_busy_error() const {
	if (_is_async_busy()) {
		return make_busy_error();
	}
	if (active_cursor && active_cursor->is_engaged()) {
		return make_cursor_open_error();
	}
	return Dictionary();
}

void MySQLSession::_cursor_closed(const MySQLStreamingCursor *p_cursor) {
	if (active_cursor == p_cursor) {
		active_cursor = nullptr;
	}
}

Dictionary MySQLSession::connect_db() {
	if (!connection) {
		return mysql_module::make_client_error_dict("MySQLSession: Call set_config() before connect_db().");
	}
	if (_is_async_busy()) {
		return make_busy_error();
	}
	if (connection->connect()) {
		return Dictionary();
	}
	return mysql_module::make_error_dict(connection->get_last_error(), connection->get_last_diagnostics());
}

Dictionary MySQLSession::close_db() {
	if (!connection) {
		return Dictionary();
	}
	if (_is_async_busy()) {
		// Left connected, untouched: the operation still owns it.
		return make_busy_error();
	}
	// MySQLConnection::close() clears the prepared statement cache itself (the handles it
	// holds stop being valid the moment the connection closes).
	connection->close();
	if (connection->get_last_error()) {
		return mysql_module::make_error_dict(connection->get_last_error(), connection->get_last_diagnostics());
	}
	return Dictionary();
}

bool MySQLSession::is_db_connected() const {
	return connection && connection->is_connected();
}

Ref<MySQLResult> MySQLSession::_execute_text_std(const std::string &p_sql) {
	if (!connection || !connection->is_connected()) {
		return MySQLResult::from_error(make_not_connected_error());
	}
	Dictionary busy = _busy_error();
	if (!busy.is_empty()) {
		return MySQLResult::from_error(busy);
	}

	return execute_with_limit(*connection, p_sql, config);
}

Ref<MySQLResult> MySQLSession::execute_text(const String &p_sql) {
	return _execute_text_std(mysql_module::to_std_string(p_sql));
}

Ref<MySQLResult> MySQLSession::_execute_formatted_std(const std::string &p_sql, const Array &p_params) {
	if (!connection || !connection->is_connected()) {
		return MySQLResult::from_error(make_not_connected_error());
	}
	Dictionary busy = _busy_error();
	if (!busy.is_empty()) {
		return MySQLResult::from_error(busy);
	}

	mysql_module::FieldParams fields;
	String error_message;
	if (!mysql_module::array_to_field_params(p_params, fields, error_message)) {
		return MySQLResult::from_error(mysql_module::make_client_error_dict(error_message));
	}

	boost::system::result<boost::mysql::format_options> options = connection->native().format_opts();
	if (!options) {
		return MySQLResult::from_error(mysql_module::make_error_dict(options.error(), boost::mysql::diagnostics()));
	}

	// Incremental formatting (not `with_params`, which needs the number of arguments at
	// compile time): the SQL is split at every `?`, literal segments are appended as
	// `boost::mysql::runtime` (the template comes from the script, not from untrusted
	// data), and the values are escaped by Boost.MySQL itself. There is never any
	// home-made escaping. A `?` inside a quoted literal or a comment is text, not a
	// placeholder (see `skip_non_code()`, shared with `execute_script()`).
	boost::mysql::format_context context(*options);
	const int64_t sql_length = (int64_t)p_sql.size();
	size_t position = 0;
	size_t param_index = 0;
	while (true) {
		size_t placeholder = position;
		while (placeholder < p_sql.size() && p_sql[placeholder] != '?') {
			bool is_code = false;
			size_t skipped = (size_t)mysql_module::skip_non_code(p_sql, (int64_t)placeholder, sql_length, options->backslash_escapes, is_code);
			placeholder = (skipped != placeholder) ? skipped : placeholder + 1;
		}
		if (placeholder >= p_sql.size()) {
			placeholder = std::string::npos;
		}
		size_t segment_end = (placeholder == std::string::npos) ? p_sql.size() : placeholder;
		context.append_raw(boost::mysql::runtime(boost::mysql::string_view(p_sql.data() + position, segment_end - position)));
		if (placeholder == std::string::npos) {
			break;
		}
		if (param_index >= (size_t)fields.views.size()) {
			return MySQLResult::from_error(mysql_module::make_client_error_dict("execute_formatted: The SQL has more '?' placeholders than the parameters provided."));
		}
		context.append_value(fields.views[param_index]);
		param_index++;
		position = placeholder + 1;
	}
	if (param_index != (size_t)fields.views.size()) {
		return MySQLResult::from_error(mysql_module::make_client_error_dict("execute_formatted: More parameters were provided than '?' placeholders in the SQL."));
	}

	boost::system::result<std::string> formatted = std::move(context).get();
	if (!formatted) {
		return MySQLResult::from_error(mysql_module::make_error_dict(formatted.error(), boost::mysql::diagnostics()));
	}

	return _execute_text_std(*formatted);
}

Ref<MySQLResult> MySQLSession::execute_formatted(const String &p_sql, const Array &p_params) {
	return _execute_formatted_std(mysql_module::to_std_string(p_sql), p_params);
}

Ref<MySQLResult> MySQLSession::execute_prepared(const String &p_sql, const Array &p_params) {
	if (!connection || !connection->is_connected()) {
		return MySQLResult::from_error(make_not_connected_error());
	}
	Dictionary busy = _busy_error();
	if (!busy.is_empty()) {
		return MySQLResult::from_error(busy);
	}

	mysql_module::FieldParams fields;
	String error_message;
	if (!mysql_module::array_to_field_params(p_params, fields, error_message)) {
		return MySQLResult::from_error(mysql_module::make_client_error_dict(error_message));
	}

	boost::mysql::error_code error;
	boost::mysql::diagnostics diagnostics;
	boost::mysql::statement statement;
	if (!connection->get_statement_cache()->get_or_prepare(*connection, p_sql, statement, error, diagnostics)) {
		return MySQLResult::from_error(mysql_module::make_error_dict(connection->drop_if_fatal(error), diagnostics));
	}

	if ((int)statement.num_params() != (int)fields.views.size()) {
		return MySQLResult::from_error(mysql_module::make_client_error_dict(
				vformat("execute_prepared: The prepared statement expects %d parameter(s), got %d.", (int)statement.num_params(), (int)fields.views.size())));
	}

	return execute_with_limit(*connection, statement.bind(fields.views.ptr(), fields.views.ptr() + fields.views.size()), config);
}

bool MySQLSession::_backslash_escapes() const {
	// Kept up to date by Boost.MySQL from every OK packet the server sends. Without a
	// connection the statement will fail anyway; the server's default is as good as any.
	return (connection && connection->is_connected()) ? connection->native().backslash_escapes() : true;
}

Array MySQLSession::execute_script(const String &p_content) {
	Array out;

	if (!config.is_valid() || !config->get_allow_sql_script_execution()) {
		out.push_back(MySQLResult::from_error(mysql_module::make_client_error_dict("execute_script: allow_sql_script_execution is disabled in the config.")));
		return out;
	}

	// One statement at a time, re-reading the connection's backslash escaping before each:
	// a statement of the script may itself change the SQL mode (`NO_BACKSLASH_ESCAPES`).
	int position = 0;
	String statement_sql;
	while (mysql_module::next_sql_statement(p_content, position, _backslash_escapes(), statement_sql)) {
		Ref<MySQLResult> result = _execute_text_std(mysql_module::to_std_string(statement_sql));
		out.push_back(result);
		if (!result->is_ok()) {
			break; // Fail fast, like `mysql < script.sql` stopping at the first error.
		}
	}
	return out;
}

Dictionary MySQLSession::run_control_statement(const String &p_sql) {
	Ref<MySQLResult> result = _execute_text_std(mysql_module::to_std_string(p_sql));
	return result->is_ok() ? Dictionary() : result->get_error();
}

Ref<MySQLTransaction> MySQLSession::begin_transaction() {
	Dictionary error = run_control_statement("START TRANSACTION");
	if (!error.is_empty()) {
		return MySQLTransaction::create_failed(error);
	}
	return MySQLTransaction::create(Ref<MySQLSession>(this));
}

Ref<MySQLAsyncOperation> MySQLSession::async_execute_text(const String &p_sql) {
	Ref<MySQLAsyncOperation> operation;
	operation.instantiate();

	if (!connection || !connection->is_connected()) {
		complete_deferred(operation, MySQLResult::from_error(make_not_connected_error()));
		return operation;
	}
	Dictionary busy = _busy_error();
	if (!busy.is_empty()) {
		// Not tracked: the operation already running (or the open cursor) is still the
		// one that owns the connection.
		complete_deferred(operation, MySQLResult::from_error(busy));
		return operation;
	}
	pending_async_operation = operation;

	_ensure_io_thread_started();

	operation->config = config;
	operation->sql = mysql_module::to_std_string(p_sql);
	start_async_execute(*connection, operation->sql, operation, config->get_async_timeout_ms());
	return operation;
}

Ref<MySQLAsyncOperation> MySQLSession::async_execute_prepared(const String &p_sql, const Array &p_params) {
	Ref<MySQLAsyncOperation> operation;
	operation.instantiate();

	if (!connection || !connection->is_connected()) {
		complete_deferred(operation, MySQLResult::from_error(make_not_connected_error()));
		return operation;
	}
	Dictionary busy = _busy_error();
	if (!busy.is_empty()) {
		// Not tracked: the operation already running (or the open cursor) is still the
		// one that owns the connection.
		complete_deferred(operation, MySQLResult::from_error(busy));
		return operation;
	}
	pending_async_operation = operation;

	String error_message;
	if (!mysql_module::array_to_field_params(p_params, operation->params, error_message)) {
		complete_deferred(operation, MySQLResult::from_error(mysql_module::make_client_error_dict(error_message)));
		return operation;
	}

	boost::mysql::error_code error;
	boost::mysql::diagnostics diagnostics;
	boost::mysql::statement statement;
	if (!connection->get_statement_cache()->get_or_prepare(*connection, p_sql, statement, error, diagnostics)) {
		complete_deferred(operation, MySQLResult::from_error(mysql_module::make_error_dict(connection->drop_if_fatal(error), diagnostics)));
		return operation;
	}
	if ((int)statement.num_params() != (int)operation->params.views.size()) {
		complete_deferred(operation, MySQLResult::from_error(mysql_module::make_client_error_dict(vformat("async_execute_prepared: The prepared statement expects %d parameter(s), got %d.", (int)statement.num_params(), (int)operation->params.views.size()))));
		return operation;
	}

	_ensure_io_thread_started();

	operation->config = config;
	start_async_execute(*connection, statement.bind(operation->params.views.ptr(), operation->params.views.ptr() + operation->params.views.size()), operation, config->get_async_timeout_ms());
	return operation;
}

Ref<MySQLStreamingCursor> MySQLSession::execute_streaming(const String &p_sql) {
	if (!connection || !connection->is_connected()) {
		return MySQLStreamingCursor::from_error(make_not_connected_error());
	}
	Dictionary busy = _busy_error();
	if (!busy.is_empty()) {
		return MySQLStreamingCursor::from_error(busy);
	}
	Ref<MySQLStreamingCursor> cursor = MySQLStreamingCursor::start(Ref<MySQLSession>(this), *connection, config, mysql_module::to_std_string(p_sql));
	if (cursor->is_ok()) {
		active_cursor = cursor.ptr();
	}
	return cursor;
}

void MySQLSession::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_config", "config"), &MySQLSession::set_config);
	ClassDB::bind_method(D_METHOD("get_config"), &MySQLSession::get_config);

	ClassDB::bind_method(D_METHOD("connect_db"), &MySQLSession::connect_db);
	ClassDB::bind_method(D_METHOD("close_db"), &MySQLSession::close_db);
	ClassDB::bind_method(D_METHOD("is_db_connected"), &MySQLSession::is_db_connected);

	ClassDB::bind_method(D_METHOD("execute_text", "sql"), &MySQLSession::execute_text);
	ClassDB::bind_method(D_METHOD("execute_formatted", "sql", "params"), &MySQLSession::execute_formatted);
	ClassDB::bind_method(D_METHOD("execute_prepared", "sql", "params"), &MySQLSession::execute_prepared);
	ClassDB::bind_method(D_METHOD("execute_script", "content"), &MySQLSession::execute_script);

	ClassDB::bind_method(D_METHOD("begin_transaction"), &MySQLSession::begin_transaction);

	ClassDB::bind_method(D_METHOD("async_execute_text", "sql"), &MySQLSession::async_execute_text);
	ClassDB::bind_method(D_METHOD("async_execute_prepared", "sql", "params"), &MySQLSession::async_execute_prepared);
	ClassDB::bind_method(D_METHOD("execute_streaming", "sql"), &MySQLSession::execute_streaming);
}

/* mysql_streaming_cursor.cpp */

#include "mysql_streaming_cursor.h"

#include "godot_convert.h"
#include "mysql_async_operation.h"
#include "mysql_connection.h"
#include "mysql_error.h"
#include "mysql_session.h"
#include "mysql_type_convert.h"

#include "core/object/class_db.h"

#include <boost/mysql/client_errc.hpp>
#include <boost/mysql/metadata_collection_view.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/rows_view.hpp>

Ref<MySQLStreamingCursor> MySQLStreamingCursor::start(Ref<MySQLSession> p_session, MySQLConnection &p_connection, const Ref<MySQLConfig> &p_config, const std::string &p_sql) {
	Ref<MySQLStreamingCursor> cursor;
	cursor.instantiate();
	cursor->owner_session = p_session;
	cursor->connection = &p_connection;
	cursor->config = p_config;

	boost::mysql::error_code err;
	boost::mysql::diagnostics diag;
	p_connection.native().start_execution(p_sql, cursor->state, err, diag);
	if (err) {
		cursor->ok = false;
		cursor->error = mysql_module::make_error_dict(p_connection.drop_if_fatal(err), diag);
	}
	return cursor;
}

Ref<MySQLStreamingCursor> MySQLStreamingCursor::from_error(const Dictionary &p_error, bool p_async) {
	Ref<MySQLStreamingCursor> cursor;
	cursor.instantiate();
	cursor->ok = false;
	cursor->closed = true;
	cursor->error = p_error;
	cursor->async_mode = p_async;
	cursor->more = false;
	cursor->engaged = false;
	return cursor;
}

Ref<MySQLStreamingCursor> MySQLStreamingCursor::start_async(Ref<MySQLSession> p_session, MySQLConnection &p_connection, const Ref<MySQLConfig> &p_config, const String &p_sql, Ref<MySQLAsyncOperation> &r_first_step) {
	Ref<MySQLStreamingCursor> cursor;
	cursor.instantiate();
	cursor->owner_session = p_session;
	cursor->connection = &p_connection;
	cursor->config = p_config;
	cursor->async_mode = true;

	Ref<MySQLAsyncOperation> step;
	step.instantiate();
	step->config = p_config;
	step->sql = mysql_module::to_std_string(p_sql);
	step->cursor = cursor;
	cursor->first_step = step;
	cursor->last_step = step;
	p_session->_track_async_operation(step);
	mysql_module::start_cursor_open(step, *cursor.ptr());
	r_first_step = step;
	return cursor;
}

bool MySQLStreamingCursor::_step_running() const {
	return last_step.is_valid() && last_step->is_running();
}

void MySQLStreamingCursor::_apply_async_step(const MySQLAsyncOperation &p_step) {
	if (p_step.cursor_opened) {
		column_names = p_step.cursor_columns;
	}
	more = p_step.cursor_more;
	engaged = p_step.cursor_engaged;
	Ref<MySQLResult> result = p_step.get_result();
	if (result.is_valid() && !result->is_ok() && ok) {
		ok = false;
		error = result->get_error();
	}
	if (close_requested && !closed) {
		closed = true;
		if (ok && engaged) {
			_start_async_drain();
		} else if (owner_session.is_valid()) {
			owner_session->_cursor_closed(this);
		}
	}
}

bool MySQLStreamingCursor::has_more() const {
	if (async_mode) {
		return ok && !closed && (!first_step_taken || more);
	}
	return ok && !closed && state.should_read_rows();
}

bool MySQLStreamingCursor::is_engaged() const {
	if (async_mode) {
		return ok && !closed && engaged;
	}
	return ok && !closed && !state.complete();
}

PackedStringArray MySQLStreamingCursor::get_column_names() const {
	if (async_mode) {
		return column_names;
	}
	PackedStringArray names;
	boost::mysql::metadata_collection_view meta = state.meta();
	names.resize((int)meta.size());
	for (std::size_t i = 0; i < meta.size(); i++) {
		boost::mysql::string_view name = meta[i].column_name();
		names.set((int)i, mysql_module::to_godot_string(name.data(), name.size()));
	}
	return names;
}

Array MySQLStreamingCursor::next_batch() {
	Array out;
	ERR_FAIL_COND_V_MSG(async_mode, out, "MySQLStreamingCursor: This cursor comes from async_execute_streaming(); read it with async_next_batch().");
	// Same condition as `has_more()`: `state.complete()` alone is not enough, because
	// between two resultsets the execution is neither complete nor in a row-reading state.
	if (!ok || closed || !state.should_read_rows()) {
		return out;
	}

	boost::mysql::error_code err;
	boost::mysql::diagnostics diag;
	boost::mysql::rows_view batch = connection->native().read_some_rows(state, err, diag);
	if (err) {
		ok = false;
		error = mysql_module::make_error_dict(connection->drop_if_fatal(err), diag);
		return out;
	}

	boost::mysql::metadata_collection_view meta = state.meta();
	out.resize((int)batch.size());
	for (std::size_t r = 0; r < batch.size(); r++) {
		boost::mysql::row_view row = batch[r];
		Array row_array;
		row_array.resize((int)row.size());
		for (std::size_t c = 0; c < row.size(); c++) {
			row_array[(int)c] = mysql_module::field_to_variant(row[c], meta[c], config);
		}
		out[(int)r] = row_array;
	}
	return out;
}

void MySQLStreamingCursor::close() {
	if (closed || close_requested) {
		return;
	}
	if (async_mode) {
		if (_step_running()) {
			// The I/O thread owns the connection: drain once the step finishes (see
			// `_apply_async_step()`).
			close_requested = true;
			return;
		}
		// Idle: the connection is free, so the synchronous drain below is safe here too.
		engaged = false;
	}
	closed = true;
	if (ok && connection) {
		_drain();
	}
	if (owner_session.is_valid()) {
		owner_session->_cursor_closed(this);
	}
}

namespace {

// An operation that has already finished, delivered on the next frame like any other.
Ref<MySQLAsyncOperation> finished_operation(const Ref<MySQLResult> &p_result) {
	Ref<MySQLAsyncOperation> operation;
	operation.instantiate();
	mysql_module::complete_deferred(operation, p_result);
	return operation;
}

// An empty batch: one resultset with no columns and no rows, so that `get_rows()` on it
// returns an empty `Array` like any other batch.
Ref<MySQLResult> empty_result() {
	MySQLResult::Builder builder;
	builder.begin_resultset(boost::mysql::metadata_collection_view());
	builder.end_resultset(0, 0, boost::mysql::string_view());
	return builder.finish();
}

} // namespace

Ref<MySQLAsyncOperation> MySQLStreamingCursor::async_next_batch() {
	if (!async_mode) {
		return finished_operation(MySQLResult::from_error(mysql_module::make_client_error_dict("MySQLStreamingCursor: This cursor comes from execute_streaming(); read it with next_batch().")));
	}
	if (!first_step_taken && first_step.is_valid()) {
		first_step_taken = true;
		Ref<MySQLAsyncOperation> step = first_step;
		first_step = Ref<MySQLAsyncOperation>();
		// A finished operation never emits `completed` again: hand its result over in a
		// new one, so that `await cursor.async_next_batch().completed` always returns.
		return step->is_finished() ? finished_operation(step->get_result()) : step;
	}
	if (!ok) {
		return finished_operation(MySQLResult::from_error(error));
	}
	if (_step_running()) {
		return finished_operation(MySQLResult::from_error(mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::operation_in_progress), boost::mysql::diagnostics())));
	}
	if (closed || close_requested || !more) {
		return finished_operation(empty_result());
	}

	Ref<MySQLAsyncOperation> step;
	step.instantiate();
	step->config = config;
	step->cursor = Ref<MySQLStreamingCursor>(this);
	last_step = step;
	owner_session->_track_async_operation(step);
	mysql_module::start_cursor_read(step, *this, *connection);
	return step;
}

Ref<MySQLAsyncOperation> MySQLStreamingCursor::_start_async_drain() {
	closed = true;
	engaged = false;
	Ref<MySQLAsyncOperation> drain;
	drain.instantiate();
	drain->config = config;
	// The execution state moves to the drain, which no longer needs the cursor: it also
	// runs when the cursor is being destroyed.
	drain->exec_state = state;
	drain_step = drain;
	owner_session->_track_async_operation(drain);
	owner_session->_cursor_closed(this);
	mysql_module::start_cursor_drain(drain, *connection);
	return drain;
}

Ref<MySQLAsyncOperation> MySQLStreamingCursor::async_close() {
	if (!async_mode) {
		return finished_operation(MySQLResult::from_error(mysql_module::make_client_error_dict("MySQLStreamingCursor: This cursor comes from execute_streaming(); close it with close().")));
	}
	if (_step_running()) {
		return finished_operation(MySQLResult::from_error(mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::operation_in_progress), boost::mysql::diagnostics())));
	}
	if (closed || close_requested) {
		// Already draining (after a `close()` during a running step): the drain is what
		// the caller waits for.
		if (drain_step.is_valid() && !drain_step->is_finished()) {
			return drain_step;
		}
		return finished_operation(empty_result());
	}
	if (!ok || !engaged) {
		closed = true;
		engaged = false;
		if (owner_session.is_valid()) {
			owner_session->_cursor_closed(this);
		}
		return finished_operation(empty_result());
	}
	return _start_async_drain();
}

void MySQLStreamingCursor::_drain() {
	// Drain the rest of the execution silently. See the comment in the header.
	//
	// The branch on `should_read_rows()` is required, not cosmetic: with
	// `allow_multi_queries` the execution moves to `should_read_head()` between resultsets,
	// and `read_some_rows()` in that state returns an empty batch without an error and
	// without advancing `state`, so a loop that only ever calls it never terminates.
	boost::mysql::error_code err;
	boost::mysql::diagnostics diag;
	while (!state.complete()) {
		if (state.should_read_rows()) {
			connection->native().read_some_rows(state, err, diag);
		} else { // should_read_head()
			connection->native().read_resultset_head(state, err, diag);
		}
		if (err) {
			connection->drop_if_fatal(err);
			break;
		}
	}
}

MySQLStreamingCursor::~MySQLStreamingCursor() {
	if (async_mode && !closed) {
		// No step can be running here: each one holds a reference to the cursor. Drain in
		// the background instead of blocking the thread that released the last reference.
		if (ok && engaged && connection) {
			_start_async_drain();
		} else if (owner_session.is_valid()) {
			owner_session->_cursor_closed(this);
		}
		return;
	}
	close();
}

void MySQLStreamingCursor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_ok"), &MySQLStreamingCursor::is_ok);
	ClassDB::bind_method(D_METHOD("get_error"), &MySQLStreamingCursor::get_error);
	ClassDB::bind_method(D_METHOD("has_more"), &MySQLStreamingCursor::has_more);
	ClassDB::bind_method(D_METHOD("get_column_names"), &MySQLStreamingCursor::get_column_names);
	ClassDB::bind_method(D_METHOD("next_batch"), &MySQLStreamingCursor::next_batch);
	ClassDB::bind_method(D_METHOD("close"), &MySQLStreamingCursor::close);
	ClassDB::bind_method(D_METHOD("async_next_batch"), &MySQLStreamingCursor::async_next_batch);
	ClassDB::bind_method(D_METHOD("async_close"), &MySQLStreamingCursor::async_close);
}

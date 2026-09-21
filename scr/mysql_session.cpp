// SPDX-License-Identifier: MIT
/* mysql_session.cpp */

#include "mysql_session.h"

#include "godot_convert.h"
#include "mysql_async_operation.h"
#include "mysql_connection.h"
#include "mysql_error.h"
#include "mysql_params.h"
#include "mysql_streaming_cursor.h"
#include "mysql_transaction.h"
#include "prepared_statement_cache.h"

#include "core/object/class_db.h"

#include <boost/asio/cancel_after.hpp>
#include <boost/mysql/client_errc.hpp>
#include <boost/mysql/format_sql.hpp>
#include <boost/mysql/results.hpp>
#include <boost/mysql/statement.hpp>

#include <chrono>

namespace {

// Divide um script SQL em instruções, respeitando literais entre aspas simples, duplas
// e crase (com escape por barra invertida ou aspas dobradas) — não trata comentários SQL
// (-- ou /* */), documentado como limitação conhecida.
std::vector<std::string> split_sql_statements(const std::string &p_content) {
	std::vector<std::string> statements;
	std::string current;
	char quote = 0;

	auto push_trimmed = [&statements](const std::string &p_text) {
		size_t start = p_text.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) {
			return;
		}
		size_t stop = p_text.find_last_not_of(" \t\r\n");
		statements.push_back(p_text.substr(start, stop - start + 1));
	};

	for (size_t i = 0; i < p_content.size(); i++) {
		char c = p_content[i];

		if (quote != 0) {
			current += c;
			if (c == quote) {
				if (i + 1 < p_content.size() && p_content[i + 1] == quote) {
					current += p_content[++i];
				} else {
					quote = 0;
				}
			} else if (c == '\\' && quote != '`' && i + 1 < p_content.size()) {
				current += p_content[++i];
			}
			continue;
		}

		if (c == '\'' || c == '"' || c == '`') {
			quote = c;
			current += c;
		} else if (c == ';') {
			push_trimmed(current);
			current.clear();
		} else {
			current += c;
		}
	}
	push_trimmed(current);
	return statements;
}

} //namespace

MySQLSession::~MySQLSession() {
	if (io_thread_started) {
		// Ordem importa: soltar o work_guard e parar o io_context ANTES de dar join
		// garante que a thread saia de run() antes da Connection ser destruída — sem
		// isso, a thread ficaria com uma referência pendente pro io_context/any_
		// connection sendo destruídos (mesma classe de risco do S3 da auditoria, agora
		// entre threads em vez de entre expressões).
		io_work_guard.reset();
		if (connection) {
			connection->get_io_context().stop();
		}
		if (io_thread.joinable()) {
			io_thread.join();
		}
	}

	if (release_to_pool && connection) {
		release_to_pool(std::move(connection));
	}
}

void MySQLSession::_ensure_io_thread_started() {
	if (io_thread_started) {
		return;
	}
	io_work_guard = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
			boost::asio::make_work_guard(connection->get_io_context()));
	io_thread = std::thread([this]() {
		connection->get_io_context().run();
	});
	io_thread_started = true;
}

Ref<MySQLSession> MySQLSession::create_pooled(Ref<MySQLConfig> p_config, std::unique_ptr<MySQLConnection> p_connection, std::function<void(std::unique_ptr<MySQLConnection>)> p_release_to_pool) {
	Ref<MySQLSession> session;
	session.instantiate();
	session->config = p_config;
	session->connection = std::move(p_connection);
	session->statement_cache = std::make_unique<PreparedStatementCache>();
	session->release_to_pool = std::move(p_release_to_pool);
	return session;
}

void MySQLSession::set_config(const Ref<MySQLConfig> &p_config) {
	ERR_FAIL_COND_MSG(connection != nullptr, "MySQLSession: a config não pode ser trocada depois que a conexão já foi criada.");
	ERR_FAIL_COND_MSG(p_config.is_null(), "MySQLSession: a config não pode ser nula.");
	config = p_config;
	connection = std::make_unique<MySQLConnection>(config);
	statement_cache = std::make_unique<PreparedStatementCache>();
}

Dictionary MySQLSession::connect_db() {
	if (!connection) {
		return mysql_module::make_client_error_dict("MySQLSession: chame set_config() antes de connect_db().");
	}
	if (connection->connect()) {
		statement_cache->clear(); // handles de uma conexão anterior não valem mais.
		return Dictionary();
	}
	return mysql_module::make_error_dict(connection->get_last_error(), connection->get_last_diagnostics());
}

Dictionary MySQLSession::close_db() {
	if (!connection) {
		return Dictionary();
	}
	connection->close();
	statement_cache->clear();
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
		return MySQLResult::from_error(mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::not_connected), boost::mysql::diagnostics()));
	}

	boost::mysql::results results;
	boost::mysql::error_code err;
	boost::mysql::diagnostics diag;
	connection->native().execute(p_sql, results, err, diag);
	if (err) {
		return MySQLResult::from_error(mysql_module::make_error_dict(err, diag));
	}
	return MySQLResult::from_boost_results(results, config);
}

Ref<MySQLResult> MySQLSession::execute_text(const String &p_sql) {
	return _execute_text_std(mysql_module::to_std_string(p_sql));
}

Ref<MySQLResult> MySQLSession::_execute_formatted_std(const std::string &p_sql, const Array &p_params) {
	if (!connection || !connection->is_connected()) {
		return MySQLResult::from_error(mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::not_connected), boost::mysql::diagnostics()));
	}

	mysql_module::FieldParams fields;
	String error_message;
	if (!mysql_module::array_to_field_params(p_params, fields, error_message)) {
		return MySQLResult::from_error(mysql_module::make_client_error_dict(error_message));
	}

	boost::system::result<boost::mysql::format_options> opts = connection->native().format_opts();
	if (!opts) {
		return MySQLResult::from_error(mysql_module::make_error_dict(opts.error(), boost::mysql::diagnostics()));
	}

	// Formatação incremental (não with_params, que exige o número de argumentos fixo em
	// tempo de compilação): divide p_sql em '?' e intercala trechos literais
	// (boost::mysql::runtime — o molde vem do script, não de dado não confiável; ver
	// documentation/design-notes.md) com valores escapados de verdade pelo Boost.MySQL,
	// nunca por escaping próprio (resolve S7 da auditoria).
	boost::mysql::format_context ctx(*opts);
	size_t pos = 0;
	size_t param_index = 0;
	while (true) {
		size_t q = p_sql.find('?', pos);
		size_t seg_end = (q == std::string::npos) ? p_sql.size() : q;
		ctx.append_raw(boost::mysql::runtime(boost::mysql::string_view(p_sql.data() + pos, seg_end - pos)));
		if (q == std::string::npos) {
			break;
		}
		if (param_index >= fields.views.size()) {
			return MySQLResult::from_error(mysql_module::make_client_error_dict("execute_formatted: mais '?' no SQL do que parâmetros fornecidos."));
		}
		ctx.append_value(fields.views[param_index]);
		param_index++;
		pos = q + 1;
	}
	if (param_index != fields.views.size()) {
		return MySQLResult::from_error(mysql_module::make_client_error_dict("execute_formatted: mais parâmetros fornecidos do que '?' no SQL."));
	}

	boost::system::result<std::string> formatted = std::move(ctx).get();
	if (!formatted) {
		return MySQLResult::from_error(mysql_module::make_error_dict(formatted.error(), boost::mysql::diagnostics()));
	}

	return _execute_text_std(*formatted);
}

Ref<MySQLResult> MySQLSession::execute_formatted(const String &p_sql, const Array &p_params) {
	return _execute_formatted_std(mysql_module::to_std_string(p_sql), p_params);
}

Ref<MySQLResult> MySQLSession::_execute_prepared_std(const std::string &p_sql, const Array &p_params) {
	if (!connection || !connection->is_connected()) {
		return MySQLResult::from_error(mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::not_connected), boost::mysql::diagnostics()));
	}

	mysql_module::FieldParams fields;
	String error_message;
	if (!mysql_module::array_to_field_params(p_params, fields, error_message)) {
		return MySQLResult::from_error(mysql_module::make_client_error_dict(error_message));
	}

	boost::mysql::error_code err;
	boost::mysql::diagnostics diag;
	boost::mysql::statement stmt;
	if (!statement_cache->get_or_prepare(*connection, p_sql, stmt, err, diag)) {
		return MySQLResult::from_error(mysql_module::make_error_dict(err, diag));
	}

	if ((size_t)stmt.num_params() != fields.views.size()) {
		return MySQLResult::from_error(mysql_module::make_client_error_dict(
				vformat("execute_prepared: a prepared statement espera %d parâmetro(s), recebeu %d.", (int)stmt.num_params(), (int)fields.views.size())));
	}

	boost::mysql::results results;
	connection->native().execute(stmt.bind(fields.views.begin(), fields.views.end()), results, err, diag);
	if (err) {
		return MySQLResult::from_error(mysql_module::make_error_dict(err, diag));
	}
	return MySQLResult::from_boost_results(results, config);
}

Ref<MySQLResult> MySQLSession::execute_prepared(const String &p_sql, const Array &p_params) {
	return _execute_prepared_std(mysql_module::to_std_string(p_sql), p_params);
}

Array MySQLSession::execute_script(const String &p_content) {
	Array out;

	if (!config.is_valid() || !config->get_allow_sql_script_execution()) {
		out.push_back(MySQLResult::from_error(mysql_module::make_client_error_dict("execute_script: allow_sql_script_execution está desligado na config.")));
		return out;
	}

	std::vector<std::string> statements = split_sql_statements(mysql_module::to_std_string(p_content));
	for (const std::string &statement_sql : statements) {
		Ref<MySQLResult> result = _execute_text_std(statement_sql);
		out.push_back(result);
		if (!result->is_ok()) {
			break; // Fail-fast — como um "mysql < script.sql" pararia no primeiro erro.
		}
	}
	return out;
}

Dictionary MySQLSession::run_control_statement(const String &p_sql) {
	Ref<MySQLResult> result = _execute_text_std(mysql_module::to_std_string(p_sql));
	return result->is_ok() ? Dictionary() : result->get_error();
}

Ref<MySQLTransaction> MySQLSession::begin_transaction() {
	Dictionary err = run_control_statement("START TRANSACTION");
	if (!err.is_empty()) {
		ERR_FAIL_V_MSG(Ref<MySQLTransaction>(), vformat("MySQLSession: begin_transaction() falhou: %s", String(err.get("message", "erro desconhecido"))));
	}
	return MySQLTransaction::create(Ref<MySQLSession>(this));
}

Ref<MySQLAsyncOperation> MySQLSession::async_execute_text(const String &p_sql) {
	Ref<MySQLAsyncOperation> op;
	op.instantiate();

	if (!connection || !connection->is_connected()) {
		Ref<MySQLResult> error_result = MySQLResult::from_error(mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::not_connected), boost::mysql::diagnostics()));
		op->call_deferred("_complete", error_result);
		return op;
	}

	_ensure_io_thread_started();

	// sql_storage/results/diag precisam sobreviver até o callback rodar — capturados
	// por shared_ptr no lambda, não por valor local (que morreria ao sair desta
	// função). Mesma preocupação de lifetime do S3 da auditoria, agora atravessando
	// threads em vez de expressões.
	auto sql_storage = std::make_shared<std::string>(mysql_module::to_std_string(p_sql));
	auto op_results = std::make_shared<boost::mysql::results>();
	auto op_diag = std::make_shared<boost::mysql::diagnostics>();
	Ref<MySQLConfig> config_copy = config;

	auto on_done = [op, op_results, op_diag, config_copy](boost::mysql::error_code ec) {
		Ref<MySQLResult> r = ec ? MySQLResult::from_error(mysql_module::make_error_dict(ec, *op_diag)) : MySQLResult::from_boost_results(*op_results, config_copy);
		op->call_deferred("_complete", r);
	};

	int timeout_ms = config->get_async_timeout_ms();
	if (timeout_ms > 0) {
		connection->native().async_execute(*sql_storage, *op_results, *op_diag, boost::asio::cancel_after(std::chrono::milliseconds(timeout_ms), on_done));
	} else {
		connection->native().async_execute(*sql_storage, *op_results, *op_diag, on_done);
	}

	return op;
}

Ref<MySQLAsyncOperation> MySQLSession::async_execute_prepared(const String &p_sql, const Array &p_params) {
	Ref<MySQLAsyncOperation> op;
	op.instantiate();

	if (!connection || !connection->is_connected()) {
		Ref<MySQLResult> error_result = MySQLResult::from_error(mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::not_connected), boost::mysql::diagnostics()));
		op->call_deferred("_complete", error_result);
		return op;
	}

	auto fields = std::make_shared<mysql_module::FieldParams>();
	String error_message;
	if (!mysql_module::array_to_field_params(p_params, *fields, error_message)) {
		op->call_deferred("_complete", MySQLResult::from_error(mysql_module::make_client_error_dict(error_message)));
		return op;
	}

	boost::mysql::error_code prep_err;
	boost::mysql::diagnostics prep_diag;
	boost::mysql::statement stmt;
	if (!statement_cache->get_or_prepare(*connection, mysql_module::to_std_string(p_sql), stmt, prep_err, prep_diag)) {
		op->call_deferred("_complete", MySQLResult::from_error(mysql_module::make_error_dict(prep_err, prep_diag)));
		return op;
	}
	if ((size_t)stmt.num_params() != fields->views.size()) {
		op->call_deferred("_complete", MySQLResult::from_error(mysql_module::make_client_error_dict(vformat("async_execute_prepared: a prepared statement espera %d parâmetro(s), recebeu %d.", (int)stmt.num_params(), (int)fields->views.size()))));
		return op;
	}

	_ensure_io_thread_started();

	auto op_results = std::make_shared<boost::mysql::results>();
	auto op_diag = std::make_shared<boost::mysql::diagnostics>();
	Ref<MySQLConfig> config_copy = config;

	auto on_done = [op, op_results, op_diag, config_copy, fields](boost::mysql::error_code ec) {
		// fields é capturado só pra manter os field_views (e os buffers atrás deles)
		// vivos até aqui — não é usado depois disso.
		Ref<MySQLResult> r = ec ? MySQLResult::from_error(mysql_module::make_error_dict(ec, *op_diag)) : MySQLResult::from_boost_results(*op_results, config_copy);
		op->call_deferred("_complete", r);
	};

	int timeout_ms = config->get_async_timeout_ms();
	if (timeout_ms > 0) {
		connection->native().async_execute(stmt.bind(fields->views.begin(), fields->views.end()), *op_results, *op_diag, boost::asio::cancel_after(std::chrono::milliseconds(timeout_ms), on_done));
	} else {
		connection->native().async_execute(stmt.bind(fields->views.begin(), fields->views.end()), *op_results, *op_diag, on_done);
	}

	return op;
}

Ref<MySQLStreamingCursor> MySQLSession::execute_streaming(const String &p_sql) {
	if (!connection || !connection->is_connected()) {
		return MySQLStreamingCursor::from_error(mysql_module::make_error_dict(boost::mysql::make_error_code(boost::mysql::client_errc::not_connected), boost::mysql::diagnostics()));
	}
	return MySQLStreamingCursor::start(Ref<MySQLSession>(this), *connection, config, mysql_module::to_std_string(p_sql));
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

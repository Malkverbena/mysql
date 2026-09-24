/* mysql_connection.cpp */

#include "mysql_connection.h"

#include "godot_convert.h"
#include "prepared_statement_cache.h"

#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/mysql/any_address.hpp>
#include <boost/mysql/connect_params.hpp>
#include <boost/mysql/character_set.hpp>
#include <boost/mysql/format_sql.hpp>
#include <boost/mysql/is_fatal_error.hpp>
#include <boost/mysql/metadata_mode.hpp>
#include <boost/mysql/pipeline.hpp>
#include <boost/mysql/ssl_mode.hpp>

namespace {

bool wants_tls(MySQLConfig::TransportMode p_mode) {
	return p_mode == MySQLConfig::TCP_TLS_PREFERRED || p_mode == MySQLConfig::TCP_TLS_REQUIRED;
}

} //namespace

boost::asio::ssl::context MySQLConnection::_make_ssl_context(const Ref<MySQLConfig> &p_config) {
	boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_client);
	if (wants_tls(p_config->get_transport_mode())) {
		// Certificate validation is on by default, with the host name derived from the real
		// connection endpoint (never a fixed value).
		ctx.set_verify_mode(boost::asio::ssl::verify_peer);
		ctx.set_default_verify_paths();
		std::string host = mysql_module::to_std_string(p_config->get_host());
		ctx.set_verify_callback(boost::asio::ssl::host_name_verification(host));
	}
	return ctx;
}

boost::mysql::any_connection_params MySQLConnection::_make_any_connection_params(const Ref<MySQLConfig> &p_config, boost::asio::ssl::context &p_ssl_context) {
	boost::mysql::any_connection_params params;
	if (wants_tls(p_config->get_transport_mode())) {
		params.ssl_context = &p_ssl_context;
	}
	params.max_buffer_size = (std::size_t)p_config->get_max_buffer_size();
	return params;
}

MySQLConnection::MySQLConnection(Ref<MySQLConfig> p_config) :
		config(p_config),
		ssl_context(_make_ssl_context(p_config)),
		connection(io_context.get_executor(), _make_any_connection_params(p_config, ssl_context)) {
	statement_cache = memnew(PreparedStatementCache(p_config->get_statement_cache_size()));
	state = CONFIGURED;
}

MySQLConnection::~MySQLConnection() {
	memdelete(statement_cache);
}

boost::mysql::connect_params MySQLConnection::_make_connect_params() const {
	boost::mysql::connect_params params;

	if (config->get_transport_mode() == MySQLConfig::UNIX_SOCKET) {
		params.server_address = boost::mysql::unix_path{ mysql_module::to_std_string(config->get_unix_socket_path()) };
		// There is no UNIX+TLS: a UNIX socket is local and never uses TLS.
		params.ssl = boost::mysql::ssl_mode::disable;
	} else {
		boost::mysql::host_and_port address;
		address.host = mysql_module::to_std_string(config->get_host());
		address.port = (unsigned short)config->get_port();
		params.server_address = address;

		switch (config->get_transport_mode()) {
			case MySQLConfig::TCP_TLS_DISABLED:
				params.ssl = boost::mysql::ssl_mode::disable;
				break;
			case MySQLConfig::TCP_TLS_PREFERRED:
				params.ssl = boost::mysql::ssl_mode::enable;
				break;
			case MySQLConfig::TCP_TLS_REQUIRED:
			default:
				params.ssl = boost::mysql::ssl_mode::require;
				break;
		}
	}

	params.username = mysql_module::to_std_string(config->get_user());
	params.password = config->get_password_std();
	params.database = mysql_module::to_std_string(config->get_database());
	params.multi_queries = config->get_allow_multi_queries();

	return params;
}

bool MySQLConnection::connect() {
	state = CONNECTING;
	last_error.clear();
	last_diagnostics.clear();

	boost::mysql::connect_params params = _make_connect_params();
	connection.connect(params, last_error, last_diagnostics);

	if (last_error) {
		state = FAILED;
		return false;
	}

	// The Boost.MySQL default is `metadata_mode::minimal`, which leaves `column_name()`
	// empty. `MySQLResult::get_column_names()` is a documented capability, so `full` is
	// required here.
	connection.set_meta_mode(boost::mysql::metadata_mode::full);

	state = CONNECTED;
	return true;
}

bool MySQLConnection::reset_session() {
	ERR_FAIL_COND_V(state != CONNECTED, false);
	last_error.clear();
	last_diagnostics.clear();

	// Taken before the reset, which leaves the character set unknown to Boost.MySQL.
	boost::system::result<boost::mysql::format_options> options = connection.format_opts();

	boost::mysql::pipeline_request request;
	request.add_reset_connection();
	// The reset restores the server's default character set and collation, not the ones
	// `connect()` asked for (`utf8mb4_general_ci`, the `connect_params` default).
	// `add_set_character_set()` is what keeps Boost.MySQL's own tracking of the character
	// set right (`format_opts()` depends on it); the collation is then set back to the
	// exact one of a fresh connection, so string comparisons behave the same on every
	// lease. Changing only the collation keeps the character set `utf8mb4`.
	request.add_set_character_set(boost::mysql::utf8mb4_charset);
	request.add_execute("SET collation_connection = 'utf8mb4_general_ci'");
	// A `USE` run by a previous lease survives the reset; go back to the configured schema.
	std::string database = mysql_module::to_std_string(config->get_database());
	std::string use_database;
	if (!database.empty() && options) {
		use_database = boost::mysql::format_sql(*options, "USE {:i}", database);
		request.add_execute(use_database);
	}

	std::vector<boost::mysql::stage_response> responses;
	connection.run_pipeline(request, responses, last_error, last_diagnostics);

	// Every prepared statement was closed by the server (or the connection is about to be
	// closed below): the handles in the cache are no longer valid either way.
	statement_cache->clear();

	if (last_error || !options) {
		if (!last_error) {
			last_error = options.error();
		}
		boost::mysql::error_code reset_error = last_error;
		boost::mysql::diagnostics reset_diagnostics = last_diagnostics;
		close();
		last_error = reset_error; // Report why the reset failed, not how the close went.
		last_diagnostics = reset_diagnostics;
		return false;
	}
	return true;
}

void MySQLConnection::close() {
	if (state != CONNECTED) {
		return;
	}
	state = CLOSING;
	last_error.clear();
	last_diagnostics.clear();

	connection.close(last_error, last_diagnostics);

	// The handles a prepared statement cache holds are only valid on the connection that
	// prepared them: closing it invalidates every entry, whether or not the connection
	// (and this cache) go on to be reused for a later connect().
	statement_cache->clear();

	// An error while closing is not fatal for the caller: the connection is unusable anyway.
	// It is kept in `last_error` for whoever wants to inspect it.
	state = last_error ? FAILED : NONE;
}

void MySQLConnection::drop(const boost::mysql::error_code &p_error) {
	if (state != CONNECTED) {
		return;
	}
	// Destroying the old `any_connection` closes its socket without sending anything.
	connection = boost::mysql::any_connection(io_context.get_executor(), _make_any_connection_params(config, ssl_context));
	// Every prepared statement handle belonged to the dropped connection.
	statement_cache->clear();
	last_error = p_error;
	last_diagnostics.clear();
	state = FAILED;
}

const boost::mysql::error_code &MySQLConnection::drop_if_fatal(const boost::mysql::error_code &p_error) {
	if (boost::mysql::is_fatal_error(p_error)) {
		drop(p_error);
	}
	return p_error;
}

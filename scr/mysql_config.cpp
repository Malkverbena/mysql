/* mysql_config.cpp */

#include "mysql_config.h"

#include "godot_convert.h"

#include "core/object/class_db.h"

#include <boost/mysql/defaults.hpp>
#include <openssl/crypto.h>

void MySQLConfig::_wipe_password() {
	// `OPENSSL_cleanse()` instead of `memset()`, because the compiler cannot optimize the
	// write away as dead before the buffer is destroyed or replaced.
	if (!password.empty()) {
		OPENSSL_cleanse(password.data(), password.size());
	}
	password.clear();
}

MySQLConfig::~MySQLConfig() {
	_wipe_password();
}

void MySQLConfig::set_host(const String &p_host) {
	host = p_host;
}

String MySQLConfig::get_host() const {
	return host;
}

void MySQLConfig::set_port(int p_port) {
	ERR_FAIL_COND_MSG(p_port < 1 || p_port > 65535, vformat("MySQLConfig: port must be between 1 and 65535, got %d.", p_port));
	port = p_port;
}

int MySQLConfig::get_port() const {
	return port;
}

void MySQLConfig::set_unix_socket_path(const String &p_path) {
	unix_socket_path = p_path;
}

String MySQLConfig::get_unix_socket_path() const {
	return unix_socket_path;
}

void MySQLConfig::set_user(const String &p_user) {
	user = p_user;
}

String MySQLConfig::get_user() const {
	return user;
}

void MySQLConfig::set_password(const String &p_password) {
	_wipe_password();
	password = mysql_module::to_std_string(p_password);
}

const std::string &MySQLConfig::get_password_std() const {
	return password;
}

void MySQLConfig::set_database(const String &p_database) {
	database = p_database;
}

String MySQLConfig::get_database() const {
	return database;
}

void MySQLConfig::set_transport_mode(TransportMode p_mode) {
	// Every insecure setting emits a warning at the moment it is set.
	if (p_mode == TCP_TLS_DISABLED) {
		WARN_PRINT("MySQLConfig: transport_mode = TCP_TLS_DISABLED turns TLS off. The connection sends credentials and data unencrypted.");
	}
	transport_mode = p_mode;
}

MySQLConfig::TransportMode MySQLConfig::get_transport_mode() const {
	return transport_mode;
}

void MySQLConfig::set_tinyint1_mode(bool p_enabled) {
	tinyint1_mode = p_enabled;
}

bool MySQLConfig::get_tinyint1_mode() const {
	return tinyint1_mode;
}

void MySQLConfig::set_json_result_mode(JsonResultMode p_mode) {
	json_result_mode = p_mode;
}

MySQLConfig::JsonResultMode MySQLConfig::get_json_result_mode() const {
	return json_result_mode;
}

void MySQLConfig::set_allow_sql_script_execution(bool p_allowed) {
	if (p_allowed) {
		WARN_PRINT("MySQLConfig: allow_sql_script_execution = true enables manual execution of SQL scripts (several commands at once).");
	}
	allow_sql_script_execution = p_allowed;
}

bool MySQLConfig::get_allow_sql_script_execution() const {
	return allow_sql_script_execution;
}

void MySQLConfig::set_allow_multi_queries(bool p_allowed) {
	if (p_allowed) {
		WARN_PRINT("MySQLConfig: allow_multi_queries = true enables several statements per execution call (CLIENT_MULTI_STATEMENTS).");
	}
	allow_multi_queries = p_allowed;
}

bool MySQLConfig::get_allow_multi_queries() const {
	return allow_multi_queries;
}

void MySQLConfig::set_async_timeout_ms(int p_timeout_ms) {
	ERR_FAIL_COND_MSG(p_timeout_ms < 0, "MySQLConfig: async_timeout_ms cannot be negative (0 means no timeout).");
	async_timeout_ms = p_timeout_ms;
}

void MySQLConfig::set_max_buffer_size(int p_size) {
	// Boost.MySQL starts every connection with a buffer of `default_initial_read_buffer_size`
	// bytes and asserts that it fits within the maximum.
	ERR_FAIL_COND_MSG(p_size < (int)boost::mysql::default_initial_read_buffer_size, vformat("MySQLConfig: max_buffer_size must be at least %d bytes, got %d.", (int)boost::mysql::default_initial_read_buffer_size, p_size));
	max_buffer_size = p_size;
}

void MySQLConfig::set_max_result_bytes(int64_t p_size) {
	ERR_FAIL_COND_MSG(p_size < 0, "MySQLConfig: max_result_bytes cannot be negative (0 means no limit).");
	max_result_bytes = p_size;
}

void MySQLConfig::set_statement_cache_size(int p_size) {
	ERR_FAIL_COND_MSG(p_size < 1, "MySQLConfig: statement_cache_size must be at least 1.");
	statement_cache_size = p_size;
}

void MySQLConfig::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_host", "host"), &MySQLConfig::set_host);
	ClassDB::bind_method(D_METHOD("get_host"), &MySQLConfig::get_host);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "host"), "set_host", "get_host");

	ClassDB::bind_method(D_METHOD("set_port", "port"), &MySQLConfig::set_port);
	ClassDB::bind_method(D_METHOD("get_port"), &MySQLConfig::get_port);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "port"), "set_port", "get_port");

	ClassDB::bind_method(D_METHOD("set_unix_socket_path", "path"), &MySQLConfig::set_unix_socket_path);
	ClassDB::bind_method(D_METHOD("get_unix_socket_path"), &MySQLConfig::get_unix_socket_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "unix_socket_path"), "set_unix_socket_path", "get_unix_socket_path");

	ClassDB::bind_method(D_METHOD("set_user", "user"), &MySQLConfig::set_user);
	ClassDB::bind_method(D_METHOD("get_user"), &MySQLConfig::get_user);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "user"), "set_user", "get_user");

	ClassDB::bind_method(D_METHOD("set_password", "password"), &MySQLConfig::set_password);
	// There is no get_password() on purpose. See the comment in the header.

	ClassDB::bind_method(D_METHOD("set_database", "database"), &MySQLConfig::set_database);
	ClassDB::bind_method(D_METHOD("get_database"), &MySQLConfig::get_database);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "database"), "set_database", "get_database");

	ClassDB::bind_method(D_METHOD("set_transport_mode", "mode"), &MySQLConfig::set_transport_mode);
	ClassDB::bind_method(D_METHOD("get_transport_mode"), &MySQLConfig::get_transport_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "transport_mode", PROPERTY_HINT_ENUM, "TCP_TLS_DISABLED,TCP_TLS_PREFERRED,TCP_TLS_REQUIRED,UNIX_SOCKET"), "set_transport_mode", "get_transport_mode");

	ClassDB::bind_method(D_METHOD("set_tinyint1_mode", "enabled"), &MySQLConfig::set_tinyint1_mode);
	ClassDB::bind_method(D_METHOD("get_tinyint1_mode"), &MySQLConfig::get_tinyint1_mode);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tinyint1_mode"), "set_tinyint1_mode", "get_tinyint1_mode");

	ClassDB::bind_method(D_METHOD("set_json_result_mode", "mode"), &MySQLConfig::set_json_result_mode);
	ClassDB::bind_method(D_METHOD("get_json_result_mode"), &MySQLConfig::get_json_result_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "json_result_mode", PROPERTY_HINT_ENUM, "RAW_STRING,PARSED_VARIANT,LAZY_PARSED_VARIANT"), "set_json_result_mode", "get_json_result_mode");

	ClassDB::bind_method(D_METHOD("set_allow_sql_script_execution", "allowed"), &MySQLConfig::set_allow_sql_script_execution);
	ClassDB::bind_method(D_METHOD("get_allow_sql_script_execution"), &MySQLConfig::get_allow_sql_script_execution);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_sql_script_execution"), "set_allow_sql_script_execution", "get_allow_sql_script_execution");

	ClassDB::bind_method(D_METHOD("set_allow_multi_queries", "allowed"), &MySQLConfig::set_allow_multi_queries);
	ClassDB::bind_method(D_METHOD("get_allow_multi_queries"), &MySQLConfig::get_allow_multi_queries);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_multi_queries"), "set_allow_multi_queries", "get_allow_multi_queries");

	ClassDB::bind_method(D_METHOD("set_async_timeout_ms", "timeout_ms"), &MySQLConfig::set_async_timeout_ms);
	ClassDB::bind_method(D_METHOD("get_async_timeout_ms"), &MySQLConfig::get_async_timeout_ms);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "async_timeout_ms"), "set_async_timeout_ms", "get_async_timeout_ms");

	ClassDB::bind_method(D_METHOD("set_max_buffer_size", "size"), &MySQLConfig::set_max_buffer_size);
	ClassDB::bind_method(D_METHOD("get_max_buffer_size"), &MySQLConfig::get_max_buffer_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_buffer_size"), "set_max_buffer_size", "get_max_buffer_size");

	ClassDB::bind_method(D_METHOD("set_max_result_bytes", "size"), &MySQLConfig::set_max_result_bytes);
	ClassDB::bind_method(D_METHOD("get_max_result_bytes"), &MySQLConfig::get_max_result_bytes);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_result_bytes"), "set_max_result_bytes", "get_max_result_bytes");

	ClassDB::bind_method(D_METHOD("set_statement_cache_size", "size"), &MySQLConfig::set_statement_cache_size);
	ClassDB::bind_method(D_METHOD("get_statement_cache_size"), &MySQLConfig::get_statement_cache_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "statement_cache_size"), "set_statement_cache_size", "get_statement_cache_size");

	BIND_ENUM_CONSTANT(TCP_TLS_DISABLED);
	BIND_ENUM_CONSTANT(TCP_TLS_PREFERRED);
	BIND_ENUM_CONSTANT(TCP_TLS_REQUIRED);
	BIND_ENUM_CONSTANT(UNIX_SOCKET);

	BIND_ENUM_CONSTANT(RAW_STRING);
	BIND_ENUM_CONSTANT(PARSED_VARIANT);
	BIND_ENUM_CONSTANT(LAZY_PARSED_VARIANT);
}

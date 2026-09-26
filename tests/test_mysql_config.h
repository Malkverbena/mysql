/* test_mysql_config.h */
#pragma once

#include "../scr/mysql_config.h"

#include "tests/test_macros.h"

// Unit tests for the range checks of the numeric `MySQLConfig` setters. An out-of-range
// value is rejected with an error and the previous value is kept. No server is needed.
namespace TestMySQLConfig {

TEST_CASE("[Modules][MySQL] Config rejects a port outside 1-65535") {
	Ref<MySQLConfig> config;
	config.instantiate();
	config->set_port(3307);
	ERR_PRINT_OFF;
	config->set_port(65537); // Used to be narrowed to 1 when connecting.
	config->set_port(0);
	config->set_port(-1);
	ERR_PRINT_ON;
	CHECK(config->get_port() == 3307);
	config->set_port(65535);
	CHECK(config->get_port() == 65535);
	config->set_port(1);
	CHECK(config->get_port() == 1);
}

TEST_CASE("[Modules][MySQL] Config rejects a max_buffer_size below the initial buffer") {
	Ref<MySQLConfig> config;
	config.instantiate();
	int default_size = config->get_max_buffer_size();
	ERR_PRINT_OFF;
	config->set_max_buffer_size(-1); // Used to become effectively unlimited as a size_t.
	config->set_max_buffer_size(1023);
	ERR_PRINT_ON;
	CHECK(config->get_max_buffer_size() == default_size);
	config->set_max_buffer_size(1024);
	CHECK(config->get_max_buffer_size() == 1024);
}

TEST_CASE("[Modules][MySQL] Config rejects an asynchronous batch below one row") {
	Ref<MySQLConfig> config;
	config.instantiate();
	int default_rows = config->get_async_batch_rows();
	ERR_PRINT_OFF;
	config->set_async_batch_rows(0);
	config->set_async_batch_rows(-5);
	ERR_PRINT_ON;
	CHECK(config->get_async_batch_rows() == default_rows);
	config->set_async_batch_rows(1);
	CHECK(config->get_async_batch_rows() == 1);
}

TEST_CASE("[Modules][MySQL] Config rejects a negative timeout or result limit") {
	Ref<MySQLConfig> config;
	config.instantiate();
	config->set_async_timeout_ms(500);
	config->set_cancel_timeout_ms(700);
	config->set_max_result_bytes(4096);
	ERR_PRINT_OFF;
	config->set_async_timeout_ms(-1);
	config->set_cancel_timeout_ms(-1);
	config->set_max_result_bytes(-1);
	ERR_PRINT_ON;
	CHECK(config->get_async_timeout_ms() == 500);
	CHECK(config->get_cancel_timeout_ms() == 700);
	CHECK(config->get_max_result_bytes() == 4096);

	// 0 stays valid for all three: no timeout, no limit.
	config->set_async_timeout_ms(0);
	config->set_cancel_timeout_ms(0);
	config->set_max_result_bytes(0);
	CHECK(config->get_async_timeout_ms() == 0);
	CHECK(config->get_cancel_timeout_ms() == 0);
	CHECK(config->get_max_result_bytes() == 0);
}

TEST_CASE("[Modules][MySQL] Config rejects enum values it does not define") {
	Ref<MySQLConfig> config;
	config.instantiate();
	ERR_PRINT_OFF; // TCP_TLS_PREFERRED warns, and so do the invalid values below.
	config->set_transport_mode(MySQLConfig::TCP_TLS_PREFERRED);
	config->set_json_result_mode(MySQLConfig::RAW_STRING);
	config->set_transport_mode((MySQLConfig::TransportMode)7);
	config->set_transport_mode((MySQLConfig::TransportMode)-1);
	config->set_json_result_mode((MySQLConfig::JsonResultMode)3);
	ERR_PRINT_ON;
	CHECK(config->get_transport_mode() == MySQLConfig::TCP_TLS_PREFERRED);
	CHECK(config->get_json_result_mode() == MySQLConfig::RAW_STRING);
}

TEST_CASE("[Modules][MySQL] duplicate_config() copies every setting and stays independent") {
	Ref<MySQLConfig> config;
	config.instantiate();
	config->set_host("db.example");
	config->set_port(3307);
	config->set_unix_socket_path("/run/mysqld/mysqld.sock");
	config->set_user("user");
	config->set_password("secret");
	config->set_database("schema");
	config->set_transport_mode(MySQLConfig::UNIX_SOCKET);
	config->set_tinyint1_mode(true);
	config->set_json_result_mode(MySQLConfig::PARSED_VARIANT);
	ERR_PRINT_OFF; // The two allow_* setters warn.
	config->set_allow_sql_script_execution(true);
	config->set_allow_multi_queries(true);
	ERR_PRINT_ON;
	config->set_async_timeout_ms(1234);
	config->set_cancel_timeout_ms(567);
	config->set_async_batch_rows(89);
	config->set_max_buffer_size(4096);
	config->set_max_result_bytes(100000);
	config->set_statement_cache_size(7);

	Ref<MySQLConfig> copy = config->duplicate_config();
	CHECK(copy != config);
	CHECK(copy->get_host() == "db.example");
	CHECK(copy->get_port() == 3307);
	CHECK(copy->get_unix_socket_path() == "/run/mysqld/mysqld.sock");
	CHECK(copy->get_user() == "user");
	CHECK(copy->get_password_std() == "secret");
	CHECK(copy->get_database() == "schema");
	CHECK(copy->get_transport_mode() == MySQLConfig::UNIX_SOCKET);
	CHECK(copy->get_tinyint1_mode());
	CHECK(copy->get_json_result_mode() == MySQLConfig::PARSED_VARIANT);
	CHECK(copy->get_allow_sql_script_execution());
	CHECK(copy->get_allow_multi_queries());
	CHECK(copy->get_async_timeout_ms() == 1234);
	CHECK(copy->get_cancel_timeout_ms() == 567);
	CHECK(copy->get_async_batch_rows() == 89);
	CHECK(copy->get_max_buffer_size() == 4096);
	CHECK(copy->get_max_result_bytes() == 100000);
	CHECK(copy->get_statement_cache_size() == 7);

	config->set_host("other.example");
	config->set_password("changed");
	CHECK(copy->get_host() == "db.example");
	CHECK(copy->get_password_std() == "secret");
}

} // namespace TestMySQLConfig

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

TEST_CASE("[Modules][MySQL] Config rejects a negative timeout or result limit") {
	Ref<MySQLConfig> config;
	config.instantiate();
	config->set_async_timeout_ms(500);
	config->set_max_result_bytes(4096);
	ERR_PRINT_OFF;
	config->set_async_timeout_ms(-1);
	config->set_max_result_bytes(-1);
	ERR_PRINT_ON;
	CHECK(config->get_async_timeout_ms() == 500);
	CHECK(config->get_max_result_bytes() == 4096);

	// 0 stays valid for both: no timeout, no limit.
	config->set_async_timeout_ms(0);
	config->set_max_result_bytes(0);
	CHECK(config->get_async_timeout_ms() == 0);
	CHECK(config->get_max_result_bytes() == 0);
}

} // namespace TestMySQLConfig

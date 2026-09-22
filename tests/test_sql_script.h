// SPDX-License-Identifier: MIT
/* test_sql_script.h */
#pragma once

#include "../scr/sql_script.h"

#include "tests/test_macros.h"

// Unit tests for the SQL script splitter used by `MySQLSession::execute_script()`.
namespace TestSQLScript {

TEST_CASE("[Modules][MySQL] Script splitter separates statements on semicolons") {
	Vector<String> statements = mysql_module::split_sql_statements("SELECT 1; SELECT 2;\nSELECT 3");
	REQUIRE(statements.size() == 3);
	CHECK(statements[0] == "SELECT 1");
	CHECK(statements[1] == "SELECT 2");
	CHECK(statements[2] == "SELECT 3");
}

TEST_CASE("[Modules][MySQL] Script splitter drops empty statements and trims whitespace") {
	CHECK(mysql_module::split_sql_statements("").is_empty());
	CHECK(mysql_module::split_sql_statements("   \n\t ").is_empty());
	CHECK(mysql_module::split_sql_statements(";;  ;").is_empty());

	Vector<String> statements = mysql_module::split_sql_statements("\n  SELECT 1  ;\r\n;  SELECT 2 \n");
	REQUIRE(statements.size() == 2);
	CHECK(statements[0] == "SELECT 1");
	CHECK(statements[1] == "SELECT 2");
}

TEST_CASE("[Modules][MySQL] Script splitter ignores semicolons inside quoted literals") {
	Vector<String> single = mysql_module::split_sql_statements("INSERT INTO t VALUES ('a;b'); SELECT 1");
	REQUIRE(single.size() == 2);
	CHECK(single[0] == "INSERT INTO t VALUES ('a;b')");

	Vector<String> double_quoted = mysql_module::split_sql_statements("SELECT \"x;y\"; SELECT 2");
	REQUIRE(double_quoted.size() == 2);
	CHECK(double_quoted[0] == "SELECT \"x;y\"");

	Vector<String> backtick = mysql_module::split_sql_statements("SELECT `a;b` FROM t; SELECT 2");
	REQUIRE(backtick.size() == 2);
	CHECK(backtick[0] == "SELECT `a;b` FROM t");
}

TEST_CASE("[Modules][MySQL] Script splitter understands doubled quotes and backslash escapes") {
	// A doubled quote stays inside the literal.
	Vector<String> doubled = mysql_module::split_sql_statements("SELECT 'it''s; fine'; SELECT 2");
	REQUIRE(doubled.size() == 2);
	CHECK(doubled[0] == "SELECT 'it''s; fine'");

	// A backslash escapes the next character inside a string literal.
	Vector<String> escaped = mysql_module::split_sql_statements("SELECT 'a\\';b'; SELECT 2");
	REQUIRE(escaped.size() == 2);
	CHECK(escaped[0] == "SELECT 'a\\';b'");

	// A backslash is not an escape inside backticks.
	Vector<String> backtick = mysql_module::split_sql_statements("SELECT `a\\`; SELECT 2");
	REQUIRE(backtick.size() == 2);
	CHECK(backtick[0] == "SELECT `a\\`");
}

TEST_CASE("[Modules][MySQL] Script splitter keeps an unterminated literal in the last statement") {
	Vector<String> statements = mysql_module::split_sql_statements("SELECT 1; SELECT 'open; SELECT 2");
	REQUIRE(statements.size() == 2);
	CHECK(statements[0] == "SELECT 1");
	CHECK(statements[1] == "SELECT 'open; SELECT 2");
}

TEST_CASE("[Modules][MySQL] Script splitter handles multi-byte characters") {
	Vector<String> statements = mysql_module::split_sql_statements(String::utf8("INSERT INTO t VALUES ('caf\xc3\xa9;'); SELECT '\xe6\x97\xa5'"));
	REQUIRE(statements.size() == 2);
	CHECK(statements[0] == String::utf8("INSERT INTO t VALUES ('caf\xc3\xa9;')"));
	CHECK(statements[1] == String::utf8("SELECT '\xe6\x97\xa5'"));
}

} // namespace TestSQLScript

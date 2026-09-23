/* test_sql_script.h */
#pragma once

#include "../scr/sql_script.h"

#include "tests/test_macros.h"

#include "core/templates/vector.h"

#include <string>

// Unit tests for the SQL script splitter used by `MySQLSession::execute_script()`.
namespace TestSQLScript {

// Collects every statement `next_sql_statement()` finds, the way `execute_script()` walks them.
Vector<String> split_all(const String &p_content, bool p_backslash_escapes = true) {
	Vector<String> statements;
	int position = 0;
	String statement;
	while (mysql_module::next_sql_statement(p_content, position, p_backslash_escapes, statement)) {
		statements.push_back(statement);
	}
	return statements;
}

// Returns the byte offset of every `?` outside quoted literals and comments, the same scan
// `execute_formatted()` does to find its placeholders.
Vector<int> placeholder_offsets(const std::string &p_sql, bool p_backslash_escapes = true) {
	Vector<int> offsets;
	int64_t length = (int64_t)p_sql.size();
	int64_t i = 0;
	while (i < length) {
		bool is_code = false;
		int64_t skipped = mysql_module::skip_non_code(p_sql, i, length, p_backslash_escapes, is_code);
		if (skipped != i) {
			i = skipped;
			continue;
		}
		if (p_sql[i] == '?') {
			offsets.push_back((int)i);
		}
		i++;
	}
	return offsets;
}

TEST_CASE("[Modules][MySQL] Script splitter separates statements on semicolons") {
	Vector<String> statements = split_all("SELECT 1; SELECT 2;\nSELECT 3");
	REQUIRE(statements.size() == 3);
	CHECK(statements[0] == "SELECT 1");
	CHECK(statements[1] == "SELECT 2");
	CHECK(statements[2] == "SELECT 3");
}

TEST_CASE("[Modules][MySQL] Script splitter drops empty statements and trims whitespace") {
	CHECK(split_all("").is_empty());
	CHECK(split_all("   \n\t ").is_empty());
	CHECK(split_all(";;  ;").is_empty());

	Vector<String> statements = split_all("\n  SELECT 1  ;\r\n;  SELECT 2 \n");
	REQUIRE(statements.size() == 2);
	CHECK(statements[0] == "SELECT 1");
	CHECK(statements[1] == "SELECT 2");
}

TEST_CASE("[Modules][MySQL] Script splitter ignores semicolons inside quoted literals") {
	Vector<String> single = split_all("INSERT INTO t VALUES ('a;b'); SELECT 1");
	REQUIRE(single.size() == 2);
	CHECK(single[0] == "INSERT INTO t VALUES ('a;b')");

	Vector<String> double_quoted = split_all("SELECT \"x;y\"; SELECT 2");
	REQUIRE(double_quoted.size() == 2);
	CHECK(double_quoted[0] == "SELECT \"x;y\"");

	Vector<String> backtick = split_all("SELECT `a;b` FROM t; SELECT 2");
	REQUIRE(backtick.size() == 2);
	CHECK(backtick[0] == "SELECT `a;b` FROM t");
}

TEST_CASE("[Modules][MySQL] Script splitter understands doubled quotes and backslash escapes") {
	// A doubled quote stays inside the literal.
	Vector<String> doubled = split_all("SELECT 'it''s; fine'; SELECT 2");
	REQUIRE(doubled.size() == 2);
	CHECK(doubled[0] == "SELECT 'it''s; fine'");

	// A backslash escapes the next character inside a string literal.
	Vector<String> escaped = split_all("SELECT 'a\\';b'; SELECT 2");
	REQUIRE(escaped.size() == 2);
	CHECK(escaped[0] == "SELECT 'a\\';b'");

	// A backslash is not an escape inside backticks.
	Vector<String> backtick = split_all("SELECT `a\\`; SELECT 2");
	REQUIRE(backtick.size() == 2);
	CHECK(backtick[0] == "SELECT `a\\`");
}

TEST_CASE("[Modules][MySQL] Script splitter keeps an unterminated literal in the last statement") {
	Vector<String> statements = split_all("SELECT 1; SELECT 'open; SELECT 2");
	REQUIRE(statements.size() == 2);
	CHECK(statements[0] == "SELECT 1");
	CHECK(statements[1] == "SELECT 'open; SELECT 2");
}

TEST_CASE("[Modules][MySQL] Script splitter handles multi-byte characters") {
	Vector<String> statements = split_all(String::utf8("INSERT INTO t VALUES ('caf\xc3\xa9;'); SELECT '\xe6\x97\xa5'"));
	REQUIRE(statements.size() == 2);
	CHECK(statements[0] == String::utf8("INSERT INTO t VALUES ('caf\xc3\xa9;')"));
	CHECK(statements[1] == String::utf8("SELECT '\xe6\x97\xa5'"));
}

TEST_CASE("[Modules][MySQL] Script splitter ignores semicolons and quotes inside comments") {
	Vector<String> dash = split_all("-- first; second\nSELECT 1;\nSELECT 2;");
	REQUIRE(dash.size() == 2);
	CHECK(dash[0] == "-- first; second\nSELECT 1");
	CHECK(dash[1] == "SELECT 2");

	Vector<String> hash = split_all("SELECT 1; # don't; stop\nSELECT 2");
	REQUIRE(hash.size() == 2);
	CHECK(hash[0] == "SELECT 1");
	CHECK(hash[1] == "# don't; stop\nSELECT 2");

	Vector<String> block = split_all("/* a; 'b */ SELECT 3; SELECT 4");
	REQUIRE(block.size() == 2);
	CHECK(block[0] == "/* a; 'b */ SELECT 3");
	CHECK(block[1] == "SELECT 4");

	// Comment markers inside a quoted literal are text.
	Vector<String> quoted = split_all("SELECT '-- x; /* y'; SELECT 5");
	REQUIRE(quoted.size() == 2);
	CHECK(quoted[0] == "SELECT '-- x; /* y'");
}

TEST_CASE("[Modules][MySQL] Script splitter only reads a double dash followed by whitespace as a comment") {
	Vector<String> statements = split_all("SELECT 1--1; SELECT 2");
	REQUIRE(statements.size() == 2);
	CHECK(statements[0] == "SELECT 1--1");

	// A double dash at the very end of the input is still a comment.
	Vector<String> trailing = split_all("SELECT 1; --");
	REQUIRE(trailing.size() == 1);
	CHECK(trailing[0] == "SELECT 1");
}

TEST_CASE("[Modules][MySQL] Script splitter drops comment-only fragments but keeps versioned comments") {
	Vector<String> statements = split_all("-- header\n/* note */;\nSELECT 1; -- trailer");
	REQUIRE(statements.size() == 1);
	CHECK(statements[0] == "SELECT 1");

	Vector<String> versioned = split_all("/*!40101 SET NAMES utf8mb4 */;\nSELECT 1;");
	REQUIRE(versioned.size() == 2);
	CHECK(versioned[0] == "/*!40101 SET NAMES utf8mb4 */");
	CHECK(versioned[1] == "SELECT 1");

	// An unterminated block comment runs to the end of the input.
	CHECK(split_all("/* never closed; SELECT 1").is_empty());
}

TEST_CASE("[Modules][MySQL] Quoted literal skipping works on UTF-8 bytes") {
	// Same helper `execute_formatted()` uses to tell a literal `?` from a placeholder.
	std::string sql = "SELECT 'caf\xc3\xa9 ? it''s' , ?";
	int64_t end = mysql_module::skip_quoted_literal(sql, 7, (int64_t)sql.size(), true);
	CHECK(sql.substr(end) == " , ?");

	std::string unterminated = "'a\\'?";
	CHECK(mysql_module::skip_quoted_literal(unterminated, 0, (int64_t)unterminated.size(), true) == (int64_t)unterminated.size());
	// Without backslash escaping, the backslash is plain text and the second quote closes it.
	CHECK(mysql_module::skip_quoted_literal(unterminated, 0, (int64_t)unterminated.size(), false) == 4);
}

TEST_CASE("[Modules][MySQL] Script splitter follows NO_BACKSLASH_ESCAPES") {
	// With the server default, `\'` is an escaped quote, so the literal never closes.
	Vector<String> escaping = split_all("SELECT 'C:\\'; SELECT 2");
	REQUIRE(escaping.size() == 1);
	CHECK(escaping[0] == "SELECT 'C:\\'; SELECT 2");

	// Under NO_BACKSLASH_ESCAPES, the backslash is plain text and the literal closes.
	Vector<String> plain = split_all("SELECT 'C:\\'; SELECT 2", false);
	REQUIRE(plain.size() == 2);
	CHECK(plain[0] == "SELECT 'C:\\'");
	CHECK(plain[1] == "SELECT 2");

	// A backslash is never an escape inside backticks, whatever the mode.
	CHECK(split_all("SELECT `a\\`; SELECT 2", false).size() == 2);
}

TEST_CASE("[Modules][MySQL] Script splitter returns statements one at a time") {
	const String script = "SELECT 1; -- note\nSELECT 2;";
	int position = 0;
	String statement;
	REQUIRE(mysql_module::next_sql_statement(script, position, true, statement));
	CHECK(statement == "SELECT 1");
	CHECK(position == 9);
	REQUIRE(mysql_module::next_sql_statement(script, position, true, statement));
	CHECK(statement == "-- note\nSELECT 2");
	CHECK_FALSE(mysql_module::next_sql_statement(script, position, true, statement));
	CHECK(position == script.length());
}

TEST_CASE("[Modules][MySQL] Script splitter reads the content of a versioned comment as code") {
	Vector<String> statements = split_all("/*!50003 SELECT 'a;b' */; SELECT 2");
	REQUIRE(statements.size() == 2);
	CHECK(statements[0] == "/*!50003 SELECT 'a;b' */");
}

TEST_CASE("[Modules][MySQL] Placeholder scan skips quoted literals and comments") {
	CHECK(placeholder_offsets("SELECT ?, '?', \"?\", `?`") == Vector<int>{ 7 });
	CHECK(placeholder_offsets("SELECT ? -- is it ?\n, ?") == Vector<int>{ 7, 22 });
	CHECK(placeholder_offsets("SELECT ? # ?\n, ?") == Vector<int>{ 7, 15 });
	CHECK(placeholder_offsets("SELECT /* ? */ ?") == Vector<int>{ 15 });
	// An apostrophe inside a comment does not open a literal that would hide what follows.
	CHECK(placeholder_offsets("SELECT ? -- don't\n, ?") == Vector<int>{ 7, 20 });
	// A versioned comment is code: its placeholder counts.
	CHECK(placeholder_offsets("SELECT /*!50000 ? */ 1") == Vector<int>{ 16 });
	// `1--?` is arithmetic, not a comment.
	CHECK(placeholder_offsets("SELECT 1--?") == Vector<int>{ 10 });
}

TEST_CASE("[Modules][MySQL] Placeholder scan follows NO_BACKSLASH_ESCAPES") {
	const std::string sql = "SELECT 'C:\\', ?";
	CHECK(placeholder_offsets(sql, true).is_empty()); // The literal never closes.
	CHECK(placeholder_offsets(sql, false) == Vector<int>{ 14 });
}

} // namespace TestSQLScript

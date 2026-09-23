/* sql_script.h */
#pragma once

#include "core/string/ustring.h"

#include <type_traits>

namespace mysql_module {

// A small SQL lexer shared by `execute_script()` (to find the `;` between statements) and
// `execute_formatted()` (to find the `?` placeholders), so that both agree on what is code
// and what is a quoted literal or a comment.
//
// The templates work on both `String` (one code point per index) and a UTF-8
// `std::string` (one byte per index): every character they look at is ASCII, and UTF-8
// lead/continuation bytes never collide with ASCII.

template <typename C>
uint32_t sql_code_unit(C p_char) {
	// Through the unsigned type, so a UTF-8 byte of a `char` never compares as negative.
	return static_cast<uint32_t>(static_cast<std::make_unsigned_t<C>>(p_char));
}

// Returns the index just past the quoted literal that opens at `p_start` (which must hold
// a `'`, `"` or `` ` ``), or `p_length` if the literal is never closed. A doubled quote
// stays inside the literal. A backslash escapes the next character inside `'...'` and
// `"..."` only when `p_backslash_escapes` is true, which is the server's default; the
// `NO_BACKSLASH_ESCAPES` SQL mode turns it off. A backslash is never an escape inside
// backticks.
template <typename T>
int64_t skip_quoted_literal(const T &p_text, int64_t p_start, int64_t p_length, bool p_backslash_escapes) {
	const uint32_t quote = sql_code_unit(p_text[p_start]);
	for (int64_t i = p_start + 1; i < p_length; i++) {
		const uint32_t c = sql_code_unit(p_text[i]);
		if (c == quote) {
			if (i + 1 < p_length && sql_code_unit(p_text[i + 1]) == quote) {
				i++; // Doubled quote, still inside the literal.
			} else {
				return i + 1;
			}
		} else if (c == '\\' && p_backslash_escapes && quote != '`' && i + 1 < p_length) {
			i++; // Skip the escaped character.
		}
	}
	return p_length;
}

// If a quoted literal or a comment starts at `p_index`, returns the index just past it and
// sets `r_is_code` to whether the server treats it as code; otherwise returns `p_index`
// unchanged. Comments are `-- ` (MySQL only reads a double dash followed by whitespace or
// a control character as a comment: `1--1` is arithmetic) and `#`, both up to the end of
// the line, and `/* */` blocks. A versioned comment (`/*! ... */`) is code the server
// runs: only its `/*!` opener is skipped, so its content is scanned like any other code.
template <typename T>
int64_t skip_non_code(const T &p_text, int64_t p_index, int64_t p_length, bool p_backslash_escapes, bool &r_is_code) {
	const uint32_t c = sql_code_unit(p_text[p_index]);
	const uint32_t next = p_index + 1 < p_length ? sql_code_unit(p_text[p_index + 1]) : 0;

	if (c == '\'' || c == '"' || c == '`') {
		r_is_code = true;
		return skip_quoted_literal(p_text, p_index, p_length, p_backslash_escapes);
	}

	if (c == '#' || (c == '-' && next == '-' && (p_index + 2 >= p_length || sql_code_unit(p_text[p_index + 2]) <= ' '))) {
		r_is_code = false;
		int64_t i = p_index;
		while (i < p_length && sql_code_unit(p_text[i]) != '\n') {
			i++;
		}
		return i;
	}

	if (c == '/' && next == '*') {
		if (p_index + 2 < p_length && sql_code_unit(p_text[p_index + 2]) == '!') {
			r_is_code = true;
			return p_index + 3;
		}
		r_is_code = false;
		for (int64_t i = p_index + 2; i + 1 < p_length; i++) {
			if (sql_code_unit(p_text[i]) == '*' && sql_code_unit(p_text[i + 1]) == '/') {
				return i + 2;
			}
		}
		return p_length; // Never closed: runs to the end of the input.
	}

	return p_index;
}

// Finds the next statement of a SQL script, starting at `r_position`, splitting on `;`
// outside quoted literals and comments (see `skip_non_code()`). Comments are kept in the
// statement text sent to the server; a fragment made only of comments and whitespace is
// skipped (a versioned `/*! */` comment counts as code). The statement is trimmed.
// Returns false when there is no statement left. On success, `r_position` is left just
// past the statement, so the next call continues from there.
//
// Statements are found one at a time, not all at once, so the caller can re-read
// `p_backslash_escapes` from the connection between them: a statement of the script
// itself may change the SQL mode.
bool next_sql_statement(const String &p_content, int &r_position, bool p_backslash_escapes, String &r_statement);

} //namespace mysql_module

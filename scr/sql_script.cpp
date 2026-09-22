// SPDX-License-Identifier: MIT
/* sql_script.cpp */

#include "sql_script.h"

namespace mysql_module {

namespace {

void push_trimmed_statement(Vector<String> &r_statements, const String &p_text) {
	String trimmed = p_text.strip_edges();
	if (!trimmed.is_empty()) {
		r_statements.push_back(trimmed);
	}
}

} //namespace

Vector<String> split_sql_statements(const String &p_content) {
	Vector<String> statements;
	int length = p_content.length();
	int start = 0;
	char32_t quote = 0;

	for (int i = 0; i < length; i++) {
		char32_t c = p_content[i];

		if (quote != 0) {
			if (c == quote) {
				if (i + 1 < length && p_content[i + 1] == quote) {
					i++; // Doubled quote, still inside the literal.
				} else {
					quote = 0;
				}
			} else if (c == '\\' && quote != '`' && i + 1 < length) {
				i++; // Skip the escaped character.
			}
			continue;
		}

		if (c == '\'' || c == '"' || c == '`') {
			quote = c;
		} else if (c == ';') {
			push_trimmed_statement(statements, p_content.substr(start, i - start));
			start = i + 1;
		}
	}
	push_trimmed_statement(statements, p_content.substr(start));
	return statements;
}

} //namespace mysql_module

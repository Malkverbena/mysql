/* sql_script.cpp */

#include "sql_script.h"

namespace mysql_module {

bool next_sql_statement(const String &p_content, int &r_position, bool p_backslash_escapes, String &r_statement) {
	const int length = p_content.length();
	int start = r_position;
	// Whether the current fragment holds anything besides comments and whitespace.
	bool has_code = false;

	int i = r_position;
	while (i < length) {
		bool is_code = false;
		int skipped = (int)skip_non_code(p_content, i, length, p_backslash_escapes, is_code);
		if (skipped != i) {
			has_code = has_code || is_code;
			i = skipped;
			continue;
		}

		char32_t c = p_content[i];
		if (c == ';') {
			if (has_code) {
				r_statement = p_content.substr(start, i - start).strip_edges();
				r_position = i + 1;
				return true;
			}
			start = i + 1; // Empty or comment-only fragment.
		} else if (c > ' ') {
			has_code = true;
		}
		i++;
	}

	r_position = length;
	if (has_code) {
		r_statement = p_content.substr(start).strip_edges();
		return true;
	}
	return false;
}

} //namespace mysql_module

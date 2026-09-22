// SPDX-License-Identifier: MIT
/* godot_convert.cpp */

#include "godot_convert.h"

namespace mysql_module {

std::string to_std_string(const String &p_string) {
	// The `CharString` lives until the end of this function, and its content is copied
	// into the `std::string` before returning.
	CharString utf8 = p_string.utf8();
	return std::string(utf8.get_data(), (size_t)utf8.length());
}

String to_godot_string(const char *p_data, size_t p_len) {
	return String::utf8(p_data, (int)p_len);
}

String to_godot_string(const std::string &p_string) {
	return to_godot_string(p_string.data(), p_string.size());
}

} //namespace mysql_module

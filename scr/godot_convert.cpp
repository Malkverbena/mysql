// SPDX-License-Identifier: MIT
/* godot_convert.cpp */

#include "godot_convert.h"

namespace mysql_module {

std::string to_std_string(const String &p_string) {
	// CharString utf8 vive até o fim desta função — get_data() só é usado enquanto ela
	// está viva, e o conteúdo é copiado para o std::string antes de retornar.
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

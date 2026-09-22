// SPDX-License-Identifier: MIT
/* godot_convert.h */
#pragma once

#include "core/string/ustring.h"

#include <string>

// Boundary between Godot strings and `std::string`, used wherever the module talks to
// Boost.MySQL, whose API takes and returns `std::string` and `string_view`.
//
// Rule: never keep the result of `String::utf8().get_data()` in a variable. The temporary
// `CharString` is destroyed at the end of the expression that created it, which leaves the
// pointer dangling. These functions always copy the content before returning.
namespace mysql_module {

// Copies the UTF-8 content of a Godot `String` into an owned `std::string`.
std::string to_std_string(const String &p_string);

// Builds a Godot `String` from UTF-8 bytes sent by the server (the connection is always
// `utf8mb4`), with an explicit length. It never assumes a NUL terminator.
String to_godot_string(const char *p_data, size_t p_len);
String to_godot_string(const std::string &p_string);

} //namespace mysql_module

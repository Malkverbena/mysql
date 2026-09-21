// SPDX-License-Identifier: MIT
/* mysql_error.cpp */

#include "mysql_error.h"

#include "godot_convert.h"

#include <boost/mysql/is_fatal_error.hpp>

#include <cstring>

namespace mysql_module {

Dictionary make_error_dict(const boost::mysql::error_code &p_error, const boost::mysql::diagnostics &p_diagnostics) {
	Dictionary d;
	if (!p_error) {
		return d;
	}

	boost::mysql::string_view client_message = p_diagnostics.client_message();
	String message = client_message.empty() ? to_godot_string(p_error.message()) : to_godot_string(client_message.data(), client_message.size());

	boost::mysql::string_view server_message = p_diagnostics.server_message();

	const char *category_name = p_error.category().name();
	d["category"] = to_godot_string(category_name, strlen(category_name));
	d["message"] = message;
	d["server_message"] = to_godot_string(server_message.data(), server_message.size());
	d["is_fatal"] = boost::mysql::is_fatal_error(p_error);
	return d;
}

Dictionary make_client_error_dict(const String &p_message) {
	Dictionary d;
	d["category"] = "mysql_module.client";
	d["message"] = p_message;
	d["server_message"] = String();
	d["is_fatal"] = false;
	return d;
}

} //namespace mysql_module

/* sql_error.cpp */


#include "sql_error.h"




void set_le_dic(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec, const diagnostics diags){
	last_error.clear();
	last_error["FILE"] = String(p_file);
	last_error["LINE"] = p_line;
	last_error["FUNCTION"] = String(p_function);
	last_error["ERROR"] = ec.value();
	last_error["SERVER_MESSAGE"] = diags.server_message().data();
	last_error["CLIENT_MESSAGE"] = diags.client_message().data();
}


void print_sql_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec, const diagnostics diags){
	String exc = \
	vformat("# EXCEPTION Caught!\n")+\
	vformat("# ERR: SQLException in: %s", p_file) + vformat(" in function: %s", p_function) + vformat("() on line %s\n", p_line)+\
	vformat("# ERR: %s\n", ec.value())+\
	vformat("# Server error: (%s)\n", diags.server_message().data())+\
	vformat("\n# Client Error: (%s)", diags.client_message().data());
	ERR_PRINT(exc);
}



void print_boost_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec){
	String exc = \
	vformat("# BOOST Error Caught!\n")+\
	vformat("# ERR: SQLException in: %s", p_file) + vformat(" in function: %s", p_function) + vformat("() on line %s\n", p_line)+\
	vformat("# ERR: %s\n", ec.value());
	ERR_PRINT(exc);
}


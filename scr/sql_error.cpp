/* sql_error.cpp */


#include "sql_error.h"






void set_le_dic(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec, const diagnostics diags){
	Dictionary ret;
	ret["FILE"] = String(p_file);
	ret["LINE"] = p_line;
	ret["FUNCTION"] = String(p_function);
	ret["ERROR"] = ec.value();
	ret["SERVER_MESSAGE"] = diags.server_message().data();
	ret["CLIENT_MESSAGE"] = diags.client_message().data();
	_last_error = ret;
}




void print_sql_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec, const diagnostics diags){
	String exc = \
	vformat("# EXCEPTION Caught!\n")+\
	vformat("# ERR: SQLException in: %s", p_file) + vformat(" in function: %s", p_function) + vformat("() on line %s\n", p_line)+\
	vformat("# ERR: %s\n", ec.value())+\
	vformat("# Server error: (%s)\n", diags.server_message().data())+\
	vformat("\n# Client Error: (%s)", diags.client_message().data());
	//return exc.c_str();
	ERR_PRINT(exc);
}






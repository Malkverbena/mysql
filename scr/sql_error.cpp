/* sql_error.cpp */


#include "sql_error.h"



Dictionary SqlError::SQLException(const boost::mysql::error_with_diagnostics &err){

	Dictionary ret;
	ret["FILE"] = String(__FILE__);
	ret["LINE"] = (int)__LINE__;
	ret["FUNCTION"] = String(__FUNCTION__);
	ret["ERROR"] = err.what();
	ret["SERVER_MESSAGE"] = err.get_diagnostics().server_message().data();
	ret["CLIENT_MESSAGE"] = err.get_diagnostics().client_message().data();

#ifdef TOOLS_ENABLED
	print_line("# EXCEPTION Caught!");
	print_line("# ERR: SQLException in: " + String(ret["FILE"]) + " in function: "+ String(ret["FUNCTION"]) +"() on line "+ String(ret["LINE"]));
	print_line("# ERR: " + String(ret["ERROR"]));
	print_line("# Server error: (" + String(ret["SERVER_MESSAGE"]) + ")" + "\n# Client Error: (" + String(ret["CLIENT_MESSAGE"]) + ")");
#endif

	return ret;
}



void SqlError::runtime_error(const std::exception& err){
	WARN_PRINT(String("ERROR: runtime_error in ") + String(__FILE__));
	WARN_PRINT(vformat("( %s ) on line ", String(__func__)) + itos(__LINE__));
	WARN_PRINT(vformat("ERROR: %s", String(err.what())));
}



const char * SqlError::gdt2char(String p_str){
	String str = p_str;
	const char * p_char = str.utf8().get_data();
	return p_char;
}


boost::mysql::string_view SqlError::char2sql(String str){
	const char * p_char =SqlError::gdt2char(str);
	boost::mysql::string_view ret = p_char;
	return ret;
}



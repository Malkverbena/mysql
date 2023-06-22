/* sql_conn.cpp */

#include "sql_conn.h"

ConnTcpSsl::~ConnTcpSsl(){}


void ConnTcpSsl::connect(mysql::string_view hostname, mysql::string_view port) {
	eps = resolver.resolve(hostname, port);
	conn.connect(*eps.begin(), conn_params);
}


void ConnTcpSsl::disconnect(){
	conn.close();
}


Variant ConnTcpSsl::get_param(String param){
	ERR_FAIL_COND_V_MSG(param_names.find(param) == -1, ERR_INVALID_PARAMETER, "An String must be provided to username");
	if (param == String("username")){return String(conn_params.username().data());}
	if (param == String("password")){return String(conn_params.password().data());}
	if (param == String("database")){return String(conn_params.database().data());}
	if (param == String("connection_collation") ){return (uint16_t)conn_params.connection_collation();}
	if (param == String("ssl") ){return (int)conn_params.ssl();}
	if (param == String("multi_queries") ){return (bool)conn_params.multi_queries();}
	return Variant();
}


Error ConnTcpSsl::set_param(String param, Variant p_value){
	if (param == String("username") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::STRING, ERR_INVALID_PARAMETER, "An String must be provided to username");
		String value = p_value;
		conn_params.set_username(value.utf8().get_data());
	}
	if (param == String("password") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::STRING, ERR_INVALID_PARAMETER, "An String must be provided to password");
		String value = p_value;
		conn_params.set_password(value.utf8().get_data());
	}
	if (param == String("database") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::STRING, ERR_INVALID_PARAMETER, "A String must be provided to database");
		String value = p_value;
		conn_params.set_database(value.utf8().get_data());
	}
	if (param == String("connection_collation") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::INT, ERR_INVALID_PARAMETER, "An interger must be provided to connection_collation");
		uint16_t value = (uint16_t)p_value;
		conn_params.set_connection_collation(value);
	}
	if (param == String("ssl") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::INT, ERR_INVALID_PARAMETER, "An ssl_mode must be provided to ssl");   
		ssl_mode value = (ssl_mode)((int)p_value);
		conn_params.set_ssl(value);
	}
	if (param == String("multi_queries") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::BOOL, ERR_INVALID_PARAMETER, "An boolean must be provided to multi_queries");
		bool value = bool(p_value);
		conn_params.set_multi_queries(value);   
	}
	return OK;
}








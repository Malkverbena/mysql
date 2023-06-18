/* sql_conn.cpp */

#include "sql_conn.h"




// TODO: Parei aqui. Resolver os objetos de conexão.
Error ConnTcpSsl::run(){

	conn_params(username, password, schema);
	resolver(ctx.get_executor()),
	ssl_ctx(boost::asio::ssl::context::tls_client),
	conn(ctx, ssl_ctx),




}






Variant ConnTcpSsl::get_param(String param){

	if (param == String("connection_collation") ){
	return (uint16_t)conn_params->connection_collation();
	}

	if (param == String("database") ){
		return String(conn_params->database().data());  
	}

	if (param == String("multi_queries") ){
		return (bool)conn_params->multi_queries();
	}

	if (param == String("password") ){
		return String(conn_params->password().data());
	}

	if (param == String("tls") ){
		return (int)conn_params->ssl();
	}

	if (param == String("username") ){
			return String(conn_params->username().data());
	}

	if (param == String("port") ){
		return port;
	}

	if (param == String("hostname") ){
		//FIXME/TODO: Se conectado retorna o host. Senão retira da string do host na classe
		return String(host.data());
	}

	return Variant();
}




Error ConnTcpSsl::set_param(String param, Variant p_value){

	if (param == String("connection_collation") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::INT, ERR_INVALID_PARAMETER, "An interger must be provided to connection_collation");
		uint16_t value = (uint16_t)p_value;
		conn_params->set_connection_collation(value);
	}

	if (param == String("database") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::STRING, ERR_INVALID_PARAMETER, "A String must be provided to database");
		String value = p_value;
		conn_params->set_database(value.utf8().get_data());
	}

	if (param == String("multi_queries") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::BOOL, ERR_INVALID_PARAMETER, "An boolean must be provided to multi_queries");
		bool value = bool(p_value);
		conn_params->set_multi_queries(value);   
	}

	if (param == String("password") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::STRING, ERR_INVALID_PARAMETER, "An String must be provided to password");
		String value = p_value;
		conn_params->set_password(value.utf8().get_data());
	}

	if (param == String("tls") ){
		//FIXME Descobrir o tipo do ssl ENUM >> https://www.boost.org/doc/libs/master/libs/mysql/doc/html/mysql/ref/boost__mysql__ssl_mode.html
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::INT, ERR_INVALID_PARAMETER, "An ssl_mode must be provided to ssl");   
//		conn_params->set_ssl(value);
	}
	if (param == String("username") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::STRING, ERR_INVALID_PARAMETER, "An String must be provided to username");
		String value = p_value;
		conn_params->set_username(value.utf8().get_data());
	}

	if (param == String("port") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::INT, ERR_INVALID_PARAMETER, "An String must be provided to port");
		int value = (int)p_value;
		port = value;
	}

	if (param == String("hostname") ){
		ERR_FAIL_COND_V_MSG(p_value.get_type() == Variant::STRING, ERR_INVALID_PARAMETER, "An String must be provided to hostname");
		//FIXME
//		hostname = Str2char(value);
	}

	return OK;
}



ConnTcpSsl::ConnTcpSsl(bool _async){
	async = _async;
}

ConnTcpSsl::~ConnTcpSsl(){
}




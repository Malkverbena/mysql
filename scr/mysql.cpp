/* mysql.cpp */

#include "mysql.h"

//TODO: Testar se é TCP
Error MySQL::tcp_connect(const String conn_name, String hostname, String port, bool async){

	Error ret = OK;
	std::map<String, VariantConn>::iterator p_con;
	p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_CONNECTION_ERROR, "Connection not found.");

	std::visit([hostname, port, async, &ret](auto c){
		// FIXME: PONTEIRO DO RET
		ret = c->connect(hostname, port, async);
	}, p_con->second);
	return ret;
}


//TODO: Testar se é UNIX
Error MySQL::unix_connect(const String conn_name, String p_socket_path, bool async){

	Error ret = OK;
	std::map<String, VariantConn>::iterator p_con;
	p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_CONNECTION_ERROR, "Connection not found.");

	std::visit([p_socket_path, async, &ret](auto c){
		// FIXME: PONTEIRO DO RET
		ret = c->connect(p_socket_path, async);
	}, p_con->second);
	return ret;
}



Dictionary MySQL::get_credentials(const String conn_name){

	Dictionary ret;
	std::map<String, VariantConn>::iterator p_con;
	p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ret, "Connection not found.");

	std::visit([&ret](auto c){

		ret["username"] = String(c->conn_params.username().data());
		ret["password"] = String(c->conn_params.password().data());
		ret["database"] = String(c->conn_params.database().data());
		ret["connection_collation"] = (int)c->conn_params.connection_collation();
		ret["ssl"] = (MySQL::ssl_mode)c->conn_params.ssl();
		ret["multi_queries"] = c->conn_params.multi_queries();

	}, p_con->second);


//	ret["username"] = p_con->second.conn_ptr->conn_params.username().data();
//	ret["password"] = p_con->second.conn_ptr->conn_params.password().data();
//	ret["database"] = p_con->second.conn_ptr->conn_params.database().data();
//	ret["connection_collation"] = (int)p_con->second.conn_ptr->conn_params.connection_collation();
//	ret["ssl"] = (MySQL::ssl_mode)p_con->second.conn_ptr->conn_params.ssl();
//	ret["multi_queries"] = p_con->second.conn_ptr->conn_params.multi_queries();

	return ret;
}





Error MySQL::set_credentials( const String conn_name,
										String p_username,
										String p_password,
										String p_database,
										std::uint16_t collation,
										MySQL::ssl_mode p_ssl,
										bool multi_queries){

	std::map<String, VariantConn>::iterator p_con;
	p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_CONNECTION_ERROR, "Connection not found.");

	std::visit([p_username, p_password, p_database, collation, p_ssl, multi_queries](auto c){

		c->username = copy_string(const_cast<char*>(p_username.utf8().get_data()));
		c->password = copy_string(const_cast<char*>(p_password.utf8().get_data()));
		c->database = copy_string(const_cast<char*>(p_database.utf8().get_data()));

		c->conn_params.set_username(c->username);
		c->conn_params.set_password(c->password);
		c->conn_params.set_database(c->database);
		c->conn_params.set_connection_collation(collation);
		c->conn_params.set_ssl((mysql::ssl_mode)p_ssl);
		c->conn_params.set_multi_queries(multi_queries);
	
	}, p_con->second);


//	auto com = std::visit([](auto k){ return k;}, p_con->second);
//	con->username = copy_string(const_cast<char*>(p_username.utf8().get_data()));
/*
	p_con->second->username = copy_string(const_cast<char*>(p_username.utf8().get_data()));
	p_con->second->password = copy_string(const_cast<char*>(p_password.utf8().get_data()));
	p_con->second->database = copy_string(const_cast<char*>(p_database.utf8().get_data()));

	p_con->second->conn_params.set_username(p_con->second.conn_ptr->username);
	p_con->second->conn_params.set_password(p_con->second.conn_ptr->password);
	p_con->second->conn_params.set_database(p_con->second.conn_ptr->database);
	p_con->second->conn_params.set_connection_collation(collation);
	p_con->second->conn_params.set_ssl((mysql::ssl_mode)p_ssl);
	p_con->second->conn_params.set_multi_queries(multi_queries);
*/
	return OK;
}



PackedStringArray MySQL::get_connections() const {
	PackedStringArray ret;
	for(auto it:connections_holder){
		ret.append(it.first);
	}
	return ret;
}



Error MySQL::delete_connection(const String conn_name){
	Error err = MySQL::sql_disconnect(conn_name);
	ERR_FAIL_COND_V_MSG(err == ERR_CONNECTION_ERROR, ERR_CONNECTION_ERROR, "Connection not found.");
	connections_holder.erase(conn_name);
	ERR_FAIL_COND_V_MSG(err != OK, err, "Failed!.");
	return err;
}



Error MySQL::sql_disconnect(const String conn_name){
	std::map<String, VariantConn>::iterator p_con;
	p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_CONNECTION_ERROR, "Connection not found.");

	try{
	

		
		std::visit([](auto c){c->conn.close();}, p_con->second);
		
/*
		VariantConn	*v = &p_con->second;
		if (std::holds_alternative<std::shared_ptr<ConnTcp>>(*v)){
			std::get<1>(*v)->conn.close();
		}
		else if (std::holds_alternative<std::shared_ptr<ConnTcpSsl>>(*v)){
			std::get<2>(*v)->conn.close();
		}
		else if (std::holds_alternative<std::shared_ptr<ConnUnix>>(*v)){
			std::get<3>(*v)->conn.close();
		}
		else if (std::holds_alternative<std::shared_ptr<ConnUnixSsl>>(*v)){
			std::get<4>(*v)->conn.close();
		}
*/

	}
	catch (const error_with_diagnostics& err){
		SQLException(err);
		return FAILED;
	}
	catch (const std::exception& err){
		_runtime_error(err);
		return FAILED;
	}
	return OK;
}



Error MySQL::new_connection(const String conn_name, CONN_TYPE type){

	std::map<String, VariantConn>::iterator it_con;
	it_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(it_con != connections_holder.end(), ERR_ALREADY_EXISTS, "Connection already exists.");

	switch (type){
		case TCP:{
			connections_holder[conn_name] = std::make_shared<ConnTcp>();
		}
		case TCP_SSL:{
			connections_holder[conn_name] = std::make_shared<ConnTcpSsl>();
		}
		case UNIX:{
			connections_holder[conn_name] = std::make_shared<ConnUnix>();
		}
		case UNIX_SSL:{
			connections_holder[conn_name] = std::make_shared<ConnUnixSsl>();
		}
		default:
			// FIXME: Tem que imprimir o erro ERR_V_MSG
			return FAILED;
	}

	it_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(it_con==connections_holder.end(), FAILED, "Failed to create the connection!");
	return OK;

}

/*



Ref<SqlResult> MySQL::execute(const String conn_name, String stmt){return _execute(conn_name, stmt, false);}
//Ref<SqlResult> MySQL::execute_prepared(const String conn_name, String stmt, Array binds){return _execute(conn_name, stmt, true, binds);}

Ref<SqlResult> MySQL::_execute(const String conn_name, String stmt, bool prep, Array binds) {

	Ref<SqlResult> sql_result;
	std::map<String, ConnBox>::iterator p_con;
	p_con = connections_holder.find(conn_name);

	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), sql_result, "Connection not found.");

	sql_result = Ref<SqlResult>(memnew(SqlResult()));

	const char* sql = stmt.utf8().get_data();
	
	boost::mysql::error_code ec;
	diagnostics diag;
	results result;

	p_con->second.conn_ptr->conn.execute(sql, result, ec, diag);

	if (ec){
		std::cout << "Operation failed with error code: " << ec << '\n';
		std::cout << "Server diagnostics: " << diag.server_message() << std::endl;
		_last_error.clear();
		_last_error["FILE"] = String(__FILE__);
		_last_error["LINE"] = (int)__LINE__;
		_last_error["FUNCTION"] = String(__FUNCTION__);
		_last_error["ERROR"] = diag.what();
		_last_error["SERVER_MESSAGE"] = diag.diagnostics().server_message().data();
		_last_error["CLIENT_MESSAGE"] = diag.diagnostics().client_message().data();

		print_line("# EXCEPTION Caught!");
		print_line("# ERR: SQLException in: " + String(_last_error["FILE"]) + " in function: "+ String(_last_error["FUNCTION"]) +"() on line "+ String(_last_error["LINE"]));
		print_line("# ERR: " + String(_last_error["ERROR"]));
		print_line("# Server error: (" + String(_last_error["SERVER_MESSAGE"]) + ")" + "\n# Client Error: (" + String(_last_error["CLIENT_MESSAGE"]) + ")");

		WARN_PRINT(String("ERROR: runtime_error in ") + String(__FILE__));
		WARN_PRINT(vformat("( %s ) on line ", String(__func__)) + itos(__LINE__));
		WARN_PRINT(vformat("ERROR: %s", String(diag.what())));
	
	}


	rows_view all_rows = result.rows();



	for(size_t m = 0; m < all_rows.size(); m++){
		Dictionary column;
		String column_name = String(result.meta()[m].column_name().data());

		column["column_collation"]				= result.meta()[m].column_collation();
		column["column_length"]					= result.meta()[m].column_length();
		column["column_name"]					= column_name;
		column["database"]						= String(result.meta()[m].database().data());
		column["decimals"]						= result.meta()[m].decimals();
		column["has_no_default_value"]		= result.meta()[m].has_no_default_value();
		column["is_auto_increment"]			= result.meta()[m].is_auto_increment();
		column["is_multiple_key"]				= result.meta()[m].is_multiple_key();
		column["is_not_null"]					= result.meta()[m].is_not_null();
		column["is_primary_key"]				= result.meta()[m].is_primary_key();
		column["is_set_to_now_on_update"]	= result.meta()[m].is_set_to_now_on_update();
		column["is_unique_key"]					= result.meta()[m].is_unique_key();
		column["is_unsigned"]					= result.meta()[m].is_unsigned();
		column["is_zerofill"]					= result.meta()[m].is_zerofill();
		column["original_column_name"]		= String(result.meta()[m].original_column_name().data());
		column["original_table"]				= String(result.meta()[m].original_table().data());
		column["table"]							= String(result.meta()[m].table().data());
		column["type"]								= (int)result.meta()[m].type();

		sql_result->meta[column_name] = column;
	}

	for(size_t row = 0; row < all_rows.size(); row++){

		Dictionary line = Dictionary();
		size_t f = 0;
		for (auto fv : all_rows.at(row).as_vector()) {
			String column_name = String(result.meta()[f].column_name().data());
			line[column_name] = field2Var(fv);
			f++;
		}
		sql_result->result[row] = line;
	}


	return sql_result;
}

	
	







*/






MySQL::MySQL() {
}

MySQL::~MySQL() {
	// close connections before empty connections_holder
}




void MySQL::_bind_methods() {

	// ==== CONNECTION ==== /
	ClassDB::bind_method(D_METHOD("new_connection", "connection", "type"), &MySQL::new_connection, DEFVAL(TCP));
	ClassDB::bind_method(D_METHOD("get_connections"), &MySQL::get_connections);
	ClassDB::bind_method(D_METHOD("delete_connection", "connection"), &MySQL::delete_connection);
	
	ClassDB::bind_method(D_METHOD("tcp_connect", "connection", "hostname", "port", "async"), &MySQL::tcp_connect, DEFVAL("127.0.0.1"), DEFVAL("3306"), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("unix_connect", "connection", "socket path", "async"), &MySQL::unix_connect,  DEFVAL(false));
	ClassDB::bind_method(D_METHOD("sql_disconnect", "connection"), &MySQL::sql_disconnect);

	ClassDB::bind_method(D_METHOD("set_credentials", "connection", "username", "password", "schema", "collation", "ssl_mode", "multi_queries"),&MySQL::set_credentials,\
	DEFVAL(String()), DEFVAL(handshake_params::default_collation), DEFVAL((int)ssl_mode::require), DEFVAL(false));


/*
	ClassDB::bind_method(D_METHOD("get_credentials", "connection"), &MySQL::get_credentials);
	ClassDB::bind_method(D_METHOD("execute", "connection", "statement"), &MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "connection", "statement", "binds"), &MySQL::execute, DEFVAL(Array()));
*/

	BIND_ENUM_CONSTANT(TCP);
	BIND_ENUM_CONSTANT(TCP_SSL);
	BIND_ENUM_CONSTANT(UNIX);
	BIND_ENUM_CONSTANT(UNIX_SSL);

	BIND_ENUM_CONSTANT(disable);
	BIND_ENUM_CONSTANT(enable);
	BIND_ENUM_CONSTANT(require);

}




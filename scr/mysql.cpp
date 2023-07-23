/* mysql.cpp */

#include "mysql.h"


Ref<SqlResult> MySQL::execute_prepared(const String conn_name, const String p_stmt, Array binds){
	return _execute(conn_name, p_stmt, true, binds);
}



Ref<SqlResult> MySQL::execute(const String conn_name, const String p_stmt){
	return _execute(conn_name, p_stmt, false, Array());
}



Ref<SqlResult> MySQL::_execute(const String conn_name, const String p_stmt, bool prep, Array binds) {


	Ref<SqlResult> sql_result;
	auto p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), sql_result, "Connection not found.");

	sql_result = Ref<SqlResult>(memnew(SqlResult()));

	const char* stmt = p_stmt.utf8().get_data();
	results result;

	Error err = std::visit([&](auto c) -> Error {
		mysql::error_code ec;
		diagnostics diag;

		if (not prep){
			c->conn.execute(stmt, result, ec, diag);
			SQL_EXCEPTION_ERR(ec, diag);
		}
		else{
			const statement prep_stmt = c->conn.prepare_statement(stmt);
			const std::vector<field> args = binds_to_field(binds);
			c->conn.execute(prep_stmt.bind(args.begin(), args.end()), result);
			SQL_EXCEPTION_ERR(ec, diag);
		}
		
		return OK;
	}, p_con->second);


	if (err){
		return sql_result;
	}


	rows_view all_rows = result.rows();

	for(auto m:result.meta()){
		Dictionary column;
		String column_name = String(m.column_name().data());

		column["column_collation"]			= m.column_collation();
		column["column_length"]				= m.column_length();
		column["column_name"]				= column_name;
		column["database"]					= String(m.database().data());
		column["decimals"]					= m.decimals();
		column["has_no_default_value"]		= m.has_no_default_value();
		column["is_auto_increment"]			= m.is_auto_increment();
		column["is_multiple_key"]			= m.is_multiple_key();
		column["is_not_null"]				= m.is_not_null();
		column["is_primary_key"]			= m.is_primary_key();
		column["is_set_to_now_on_update"]	= m.is_set_to_now_on_update();
		column["is_unique_key"]				= m.is_unique_key();
		column["is_unsigned"]				= m.is_unsigned();
		column["is_zerofill"]				= m.is_zerofill();
		column["original_column_name"]		= String(m.original_column_name().data());
		column["original_table"]			= String(m.original_table().data());
		column["table"]						= String(m.table().data());
		column["type"]						= (int)m.type();

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

	

Error MySQL::tcp_connect(const String conn_name, String hostname, String port, bool async){
	auto p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	return std::visit([&](auto c) -> Error {
		return c->connect(hostname, port, async);
	}, p_con->second);
}


Error MySQL::unix_connect(const String conn_name, String p_socket_path, bool async){
	auto p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	return std::visit([&](auto c) -> Error {
		return c->connect(p_socket_path, async);
	}, p_con->second);
}



Dictionary MySQL::get_credentials(const String conn_name){
	auto p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), Dictionary(), "Connection does not exist!");
	return std::visit([](auto c) -> Dictionary {
		Dictionary ret;
		ret["username"] = String(c->conn_params.username().data());
		ret["password"] = String(c->conn_params.password().data());
		ret["database"] = String(c->conn_params.database().data());
		ret["connection_collation"] = (int)c->conn_params.connection_collation();
		ret["ssl"] = (MySQL::ssl_mode)c->conn_params.ssl();
		ret["multi_queries"] = c->conn_params.multi_queries();
		return ret;
	}, p_con->second);
}



Error MySQL::set_credentials(	const String conn_name,
								String p_username,
								String p_password,
								String p_database,
								std::uint16_t collation,
								MySQL::ssl_mode p_ssl,
								bool multi_queries){

	auto p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	std::visit([&](auto c) {
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
	ERR_FAIL_COND_V_MSG(err == ERR_CONNECTION_ERROR, ERR_DOES_NOT_EXIST, "Connection does not exist!");
	connections_holder.erase(conn_name);
	ERR_FAIL_COND_V_MSG(err != OK, err, "Failed!.");
	return err;
}



Error MySQL::sql_disconnect(const String conn_name){
	auto p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	return std::visit([](auto& c) -> Error {
		mysql::error_code ec;
		diagnostics diag;
		c->conn.close(ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}, p_con->second);
}



Error MySQL::new_connection(const String conn_name, CONN_TYPE type){

	auto it_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(it_con != connections_holder.end(), ERR_ALREADY_EXISTS, "Connection already exists!");

	if (type == TCP){
		connections_holder[conn_name] = std::make_shared<ConnTcp>();
	}
	else if (type == TCP_SSL){
		connections_holder[conn_name] = std::make_shared<ConnTcpSsl>();
	}
	else if (type == UNIX){
		connections_holder[conn_name] = std::make_shared<ConnUnix>();
	}
	else if (type == UNIX_SSL){
		connections_holder[conn_name] = std::make_shared<ConnUnixSsl>();
	}
	else{
		ERR_FAIL_V_EDMSG(FAILED, "Invalid connection type.");
	}

	it_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(it_con==connections_holder.end(), ERR_CANT_CREATE, "Failed to create the connection!");
	return OK;

}



MySQL::MySQL() {
}

MySQL::~MySQL() {
	// TODO: terminate/close connections before empty connections_holder
}




void MySQL::_bind_methods() {

	// ==== CONNECTION ==== /
	ClassDB::bind_method(D_METHOD("new_connection", "connection", "type"), &MySQL::new_connection, DEFVAL(TCP));
	ClassDB::bind_method(D_METHOD("get_connections"), &MySQL::get_connections);
	ClassDB::bind_method(D_METHOD("delete_connection", "connection"), &MySQL::delete_connection);
	
	ClassDB::bind_method(D_METHOD("tcp_connect", "connection", "hostname", "port", "async"), &MySQL::tcp_connect, DEFVAL("127.0.0.1"), DEFVAL("3306"), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("unix_connect", "connection", "socket path", "async"), &MySQL::unix_connect,  DEFVAL(false));
	ClassDB::bind_method(D_METHOD("sql_disconnect", "connection"), &MySQL::sql_disconnect);

	ClassDB::bind_method(D_METHOD("get_credentials", "connection"), &MySQL::get_credentials);
	ClassDB::bind_method(D_METHOD("set_credentials", "connection", "username", "password", "schema", "collation", "ssl_mode", "multi_queries"),&MySQL::set_credentials,\
	DEFVAL(String()), DEFVAL(handshake_params::default_collation), DEFVAL((int)ssl_mode::require), DEFVAL(false));

	ClassDB::bind_method(D_METHOD("execute", "connection", "statement"), &MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "connection", "statement", "binds"), &MySQL::execute, DEFVAL(Array()));


	BIND_ENUM_CONSTANT(TCP);
	BIND_ENUM_CONSTANT(TCP_SSL);
	BIND_ENUM_CONSTANT(UNIX);
	BIND_ENUM_CONSTANT(UNIX_SSL);

	BIND_ENUM_CONSTANT(disable);
	BIND_ENUM_CONSTANT(enable);
	BIND_ENUM_CONSTANT(require);

}




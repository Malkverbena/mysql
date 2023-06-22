/* mysql.cpp */

#include "mysql.h"


void MySQL::new_connection(String conn_name, CONN_TYPE type, bool async, bool tls){
	// TODO: template : ConnTcp - ConnTcpSsl - ConnSocket - ConnSocketSsl
	//	std::shared_ptr<ConnTcpSsl>();
	connections_holder[conn_name] = ConnBox(type, async, tls);
}



Error MySQL::delete_connection(String conn_name){
	std::map<String, ConnBox>::iterator p_con;
	p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_CONNECTION_ERROR, "Connection not find.");
	p_con->second.conn->disconnect();
	connections_holder.erase(conn_name);
	connections.erase(conn_name);
	return OK;
}



Array MySQL::get_connections(){
	return connections.keys();
}



Error MySQL::set_credentials( String conn_name,
										String p_username,
										String p_password,
										String p_schema,
										std::uint16_t collation,
										ssl_mode p_ssl,
										bool multi_queries) {

	std::map<String, ConnBox>::iterator p_con;
	p_con = connections_holder.find(conn_name);
	ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_CONNECTION_ERROR, "Connection not find.");

	ConnTcpSsl * _conn = new ConnTcpSsl(
		p_username.utf8().get_data(),
		p_password.utf8().get_data(),
		p_schema.utf8().get_data(),
		collation,
		p_ssl,
		multi_queries,
		p_con->second.async
	);

	p_con->second.conn.reset(_conn);
	
	return OK;
}



Dictionary MySQL::get_params(String conn_name, PackedStringArray p_params){

	Dictionary ret;

	std::map<String, ConnBox>::iterator p_con;
	p_con = connections_holder.find(conn_name);

	for (int n = 0; n <= p_params.size(); n++){
		ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ret, vformat("%s is an invalid parameter ", p_params[n]));
		ret[p_params[n]] = p_con->second.conn->get_param(p_params[n]);
	}
	
	return ret;
}



Error MySQL::set_params(String conn_name, Dictionary p_params){

	Array keys = p_params.keys();
	Error err = OK;

	std::map<String, ConnBox>::iterator p_con;
	p_con = connections_holder.find(conn_name);

	for(int n = 0; n <= keys.size(); n++){
		ERR_FAIL_COND_V_MSG(p_con==connections_holder.end(), ERR_INVALID_PARAMETER, vformat("%s is an invalid parameter: ", keys[n]));
		err = p_con->second.conn->set_param(keys[n], p_params[keys[n]]);
	}

	return err;
}







void MySQL::_bind_methods() {

	/* ==== CONNECTION ==== */
	ClassDB::bind_method(D_METHOD("new_connection", "connection", "type", "async", "tls"), &MySQL::new_connection, DEFVAL(TCP), DEFVAL(false), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("delete_connection", "connection"), &MySQL::delete_connection);
	ClassDB::bind_method(D_METHOD("get_connections"), &MySQL::get_connections);
	ClassDB::bind_method(D_METHOD("get_params", "connection", "parameters"), &MySQL::get_params);
	ClassDB::bind_method(D_METHOD("set_params", "connection", "parameters"), &MySQL::set_params);
	ClassDB::bind_method(D_METHOD("set_credentials", "connection", "username", "password", "schema", "collation", "ssl_mode", "multi_queries"),&MySQL::set_credentials,\
	DEFVAL(String()), DEFVAL(handshake_params::default_collation), DEFVAL((int)ssl_mode::require), DEFVAL(false));







	BIND_ENUM_CONSTANT(TCP);
	BIND_ENUM_CONSTANT(SOCKET);

	BIND_ENUM_CONSTANT((int)ssl_mode::disable);
	BIND_ENUM_CONSTANT((int)ssl_mode::enable);
	BIND_ENUM_CONSTANT((int)ssl_mode::require);

}

MySQL::MySQL() {

}

MySQL::~MySQL() {

	// close connections before empty connections_holder
//	connections_holder.clear();
//	delete connections_holder;

}

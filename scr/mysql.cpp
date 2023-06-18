/* mysql.cpp */

#include "mysql.h"




Error MySQL::set_credentials(String conn_name, String p_hostname, String p_user, String p_password, String p_schema, int p_port){

	std::map<String, std::shared_ptr<ConnTcpSsl>>::iterator it;
	it = connections_holder.find(conn_name);

	it->second->host		= mysql::string_view(p_hostname.utf8().get_data());
	it->second->username	= mysql::string_view(p_user.utf8().get_data());
	it->second->password	=  mysql::string_view(p_password.utf8().get_data());
	it->second->schema	= mysql::string_view(p_schema.utf8().get_data());
	it->second->port		= p_port;
	//	it->second->collation		//FIXME

}



void MySQL::new_connection(String conn_name, CONN_TYPE type, bool async, bool tls){
	// TODO: template : ConnTcp - ConnTcpSsl - ConnSocket - ConnSocketSsl

	std::shared_ptr<ConnTcpSsl> a(new ConnTcpSsl(async));
	connections_holder.emplace(conn_name, a);
	
}




void MySQL::delete_connection(String conn_name){

	std::map<String, std::shared_ptr<ConnTcpSsl>>::iterator it;
	it = connections_holder.find(conn_name);
//FIXME	it->second->disconnect();

	connections_holder.erase(conn_name);
	
}




Dictionary MySQL::get_params(String conn_name, PackedStringArray p_params){
	Dictionary ret;

	for (int n = 0; n <= p_params.size(); n++){
		ERR_FAIL_COND_V_MSG(not (param_names.has(p_params[n])), ret, vformat("%s is an invalid parameter ", p_params[n]));

		std::map<String, std::shared_ptr<ConnTcpSsl>>::iterator it;
		it = connections_holder.find(conn_name);
		ret[p_params[n]] = it->second->get_param(p_params[n]);
	}
}


Error MySQL::set_params(String conn_name, Dictionary p_params){
	Array keys = p_params.keys();
	Error err = OK;

	for(int n = 0; n <= keys.size(); n++){
		ERR_FAIL_COND_V_MSG((param_names.find(keys[n]) == -1), ERR_INVALID_PARAMETER, vformat("%s is an invalid parameter: ", keys[n]));

		std::map<String, std::shared_ptr<ConnTcpSsl>>::iterator it;
		it = connections_holder.find(conn_name);
		err = it->second->set_param(keys[n], p_params[keys[n]]);

		if(err != OK){
			return err;
		}
	}

	return err;
}



void MySQL::_bind_methods() {

	ClassDB::bind_method(D_METHOD("new_connection", "connection", "type", "async", "tls"), &MySQL::new_connection, DEFVAL(TCP), DEFVAL(false), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("delete_connection", "connection"), &MySQL::delete_connection);
	ClassDB::bind_method(D_METHOD("get_params", "connection", "parameters"), &MySQL::get_params);
	ClassDB::bind_method(D_METHOD("set_params", "connection", "parameters"), &MySQL::set_params);

	ClassDB::bind_method(D_METHOD("set_credentials", "connection", "hostname", "user", "password", "schema","port"), &MySQL::set_credentials, DEFVAL(""), DEFVAL(3306));


	BIND_ENUM_CONSTANT(TCP);
	BIND_ENUM_CONSTANT(SOCKET);

	BIND_ENUM_CONSTANT((int64_t)boost::mysql::ssl_mode::disable);
	BIND_ENUM_CONSTANT((int64_t)boost::mysql::ssl_mode::enable);
	BIND_ENUM_CONSTANT((int64_t)boost::mysql::ssl_mode::require);

}

MySQL::MySQL() {

}

MySQL::~MySQL() {

	// close connections before empty connections_holder
//	connections_holder.clear();
//	delete connections_holder;

}

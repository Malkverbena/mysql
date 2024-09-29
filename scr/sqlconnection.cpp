/* sqlconnection.cpp */

#include "sqlconnection.h"












/*

void SqlConnection::start_connection(){

	ctx = std::make_shared<asio::io_context>();
	conn = std::make_shared<mysql::any_connection>(ctx);
	conn->set_meta_mode(mysql::metadata_mode::full);

}


	MySQL::MysqlCollations get_connection_collation(){ return MySQL::MysqlCollations(credential_params.connection_collation; ) }


	void set_connection_collation(MySQL::MysqlCollations p_collation)
*/



void SqlConnection::_bind_methods() {


	ClassDB::bind_method(D_METHOD("get_username"), &SqlConnection::get_username);
	ClassDB::bind_method(D_METHOD("set_username", "value"), &SqlConnection::set_username);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "username"), "set_username", "get_username");
	ADD_PROPERTY_DEFAULT("username", "");

	ClassDB::bind_method(D_METHOD("get_password"), &SqlConnection::get_password);
	ClassDB::bind_method(D_METHOD("set_password", "value"), &SqlConnection::set_password);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "password"), "set_password", "get_password");
	ADD_PROPERTY_DEFAULT("password", "");

	ClassDB::bind_method(D_METHOD("get_database"), &SqlConnection::get_database);
	ClassDB::bind_method(D_METHOD("set_database", "value"), &SqlConnection::set_database);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "database"), "set_database", "get_database");
	ADD_PROPERTY_DEFAULT("database", "");

	ClassDB::bind_method(D_METHOD("get_multi_queries"), &SqlConnection::get_multi_queries);
	ClassDB::bind_method(D_METHOD("set_multi_queries", "value"), &SqlConnection::set_multi_queries);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "multi_queries"), "set_multi_queries", "get_multi_queries");
	ADD_PROPERTY_DEFAULT("multi_queries", false);

	ClassDB::bind_method(D_METHOD("get_ssl_mode"), &SqlConnection::get_ssl_mode);
	ClassDB::bind_method(D_METHOD("set_ssl_mode", "value"), &SqlConnection::set_ssl_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "SslMode", PROPERTY_HINT_ENUM, "ssl_disable, ssl_enable, ssl_require"), "set_ssl_mode", "get_ssl_mode");
	ADD_PROPERTY_DEFAULT("SslMode", SqlCertificate::SslMode::ssl_enable);

	ClassDB::bind_method(D_METHOD("get_connection_collation"), &SqlConnection::get_connection_collation);
	ClassDB::bind_method(D_METHOD("set_connection_collation", "value"), &SqlConnection::set_ssl_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_collation", PROPERTY_HINT_ENUM), "set_connection_collation", "set_connection_collation");
	ADD_PROPERTY_DEFAULT("connection_collation", SqlConnection::MysqlCollations::default_collation);


}






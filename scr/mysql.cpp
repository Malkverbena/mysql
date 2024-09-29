/* mysql.cpp */


#include "mysql.h"




// Default constructor
MySQL::MySQL() 
//	: conn_type(SqlConnection::NONE), status(SqlConnection::DISCONNECTED), ssl_ctx(ssl_mode::disable) 
{}


// Overloaded constructor to set connection type
	MySQL::MySQL(const SqlConnection::ConnType p_type, const String p_host_name){}
//	: conn_type(SqlConnection::NONE), status(SqlConnection::DISCONNECTED), ssl_ctx(ssl_mode::disable){
//		configure(p_type, p_host_name);



MySQL::~MySQL() {
	// close_connection
}

void MySQL::_bind_methods() {
}
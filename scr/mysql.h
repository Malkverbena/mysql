/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H


#include "sql_conn.h"


#include "core/object/ref_counted.h"  
#include "core/core_bind.h"

#include <boost/mysql.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/system/system_error.hpp>
#include <boost/mysql/ssl_mode.hpp>
#include <boost/mysql/connection.hpp>

#include <boost/mysql/string_view.hpp>
#include <boost/mysql/buffer_params.hpp>
#include <boost/mysql/handshake_params.hpp>

#include <iostream>
#include <string>
#include <map>
#include <memory>

class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);

private:




//==========================================

	// Helpers

	const PackedStringArray param_names = {
		String("connection_collation"), String("database"), String("multi_queries"), String("password"), String("username"), String("hostname"), String("port")
	};


	std::map<String, std::shared_ptr<ConnTcpSsl>> connections_holder;   //TODO: Make connections a template

protected:
	static void _bind_methods();



public:

	enum CONN_TYPE{
		TCP,
		SOCKET,
	};

	// Add a new connection to the connection stack.
	//	CONN_NAME: The name of the connection.
	// ASYNC: Determines that the new connection will be asynchronous.
	// TLS: Determines that the new connection will use TLS only.
	void new_connection(String conn_name, CONN_TYPE type = TCP, bool async = false, bool tls = true);

	// Delete a connection from connection stack.
	//	CONN_NAME: The name of the connection to be deleted.
	void delete_connection(String conn_name);

	// Quikly setup for a connection
	Error set_credentials(String conn_name, String p_hostname, String p_user, String p_password, String p_schema = "", int p_port = 3306);
	// FIXME: Port must be a variable in connection

	// Retrieves parameter values from the specified connection. NOTE: Values must be passed within an PackedStringArray.
	//	CONN_NAME: The name of the connection from which you want to retrieve information.
	// PARAMS: The list of parameters to retrieve. NOTE: Valid params: ["username", "ssl", "password", "multi_queries", "database", "connection_collation"]
	Dictionary get_params(String conn_name, PackedStringArray p_params);

	// Sets the connection's parameters.
	//	CONN_NAME: The name of the connection.
	// PARAMS: The parameters to be passed to the function. Must be a valid parameter followed by a corresponding value.
	Error set_params(String conn_name, Dictionary p_params);







	MySQL();
	~MySQL();
};

	VARIANT_ENUM_CAST(MySQL::CONN_TYPE);
	VARIANT_ENUM_CAST(boost::mysql::ssl_mode);

#endif // MYSQL_H


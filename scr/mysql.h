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


friend class ConnTcpSsl;

public:
	enum CONN_TYPE{
		TCP,
		SOCKET,
	};

private:

	class ConnBox{
	public:
		bool async; CONN_TYPE type; bool tls;
		std::shared_ptr<ConnTcpSsl> conn;
		ConnBox( CONN_TYPE _type = TCP, bool _async = false, bool _tls = false ){
			type = _type; async = _async; tls = _tls;
		}
	};

	std::map<String, ConnBox> connections_holder;   //TODO: Make connections a template
	Dictionary connections;



protected:

	static void _bind_methods();



public:
	
/* ==== CONNECTION ==== */

	// Add a new connection to the connection stack.
	//	CONN_NAME: The name of the connection.
	// TYPE: Use TCP or UNIX SOCKET.
	// ASYNC: Determines that the new connection will be asynchronous.
	// SSL: Determines how the new connection will use SSL.
	void new_connection(String conn_name, CONN_TYPE type = TCP, bool async = false, bool tls = false);

	// Delete a connection from connection stack.
	//	CONN_NAME: The name of the connection to be deleted.
	Error delete_connection(String conn_name);

	// List all connections in the stack
	Array get_connections();

	// Retrieves parameter values from the specified connection. NOTE: Values must be passed within an PackedStringArray.
	//	CONN_NAME: The name of the connection from which you want to retrieve information.
	// PARAMS: The list of parameters to retrieve. NOTE: Valid params: ["username", "ssl", "password", "multi_queries", "database", "connection_collation"]
	Dictionary get_params(String conn_name, PackedStringArray p_params);

	// Sets the connection's parameters.
	//	CONN_NAME: The name of the connection.
	// PARAMS: The parameters to be passed to the function. Must be a valid parameter followed by a corresponding value.
	Error set_params(String conn_name, Dictionary p_params);

	// Setup new connections.
	Error set_credentials(	String conn_name,
									String p_username,
									String p_password,
									String p_schema 			= String(),
									std::uint16_t collation	= handshake_params::default_collation,
									ssl_mode p_ssl				= ssl_mode::require,
									bool multi_queries		= false);










	MySQL();
	~MySQL();
};

VARIANT_ENUM_CAST(MySQL::CONN_TYPE);
VARIANT_ENUM_CAST(boost::mysql::ssl_mode);

#endif // MYSQL_H


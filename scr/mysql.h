/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H



#include "sql_conn.h"
#include "sql_result.h"



//=====================================
class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);


public:
//https://www.boost.org/doc/libs/develop/libs/mysql/doc/html/mysql/ssl.html
	enum ssl_mode{
		disable	= (int)mysql::ssl_mode::disable,
		enable	= (int)mysql::ssl_mode::enable,
		require	= (int)mysql::ssl_mode::require,
	};

	// TODO: Passar para classe conn
	enum CONN_TYPE{
		TCP		= 0,
		TCP_TLS	= 1,
		UNIX	= 2,
		UNIX_TLS= 3
	};



private:

	std::map<String, VariantConn> connections_holder;

	Ref<SqlResult> _execute(const String conn_name, const String p_stmt, bool prep, Array binds);


protected:

	static void _bind_methods();


public:


// ==== CONNECTION ==== /

	// Add a new connection to the connection stack.
	// CONN_NAME: The name of the connection.
	// TYPE: Use TCP or UNIX SOCKET.
	// ASYNC: Determines that the new connection will be asynchronous.
	// SSL: Determines how the new connection will use SSL.
	Error new_connection(const String conn_name, CONN_TYPE type = TCP);   //****


	// Setup new connections.  >> https://www.boost.org/doc/libs/develop/libs/mysql/doc/html/mysql/ref/boost__mysql__handshake_params.html
	Error set_credentials(const String conn_name,
									String p_username,
									String p_password,
									String p_database			= String(),
									std::uint16_t collation	= handshake_params::default_collation,
									MySQL::ssl_mode p_ssl	= MySQL::ssl_mode::enable,
									bool multi_queries		= false);


	/*
	If it is an SSL connection, it sets the SSL context's verify mode to verify the peer, 
	adds the certificate authority using the provided certificate, sets the verify callback 
	to verify the host name using the provided "common name".
	*/
	// CONN_NAME: The name of the connection.
	// CERT PATH: The path to the certificate.
	// COMMON NAME: The common name of the certificate.
	Error set_certificate(const String conn_name, const String p_certificate_path, const String p_common_name = String("mysql"));


	// Delete a connection from connection stack.
	//	CONN_NAME: The name of the connection to be deleted.
	Error delete_connection(const String conn_name);

	// List all connections in the stack
	PackedStringArray get_connections() const;



	// Get handshake parameters
	Dictionary get_credentials(const String conn_name);

	// Establishes a connection to a MySQL server.
	Error tcp_connect(const String conn_name, const String p_hostname = "127.0.0.1", const String p_port = "3306", const bool p_async = true);
	Error unix_connect(const String conn_name, const String p_socket_path, const bool p_async = false);

	// Closes the connection to the server.
	Error sql_disconnect(const String conn_name);


	Ref<SqlResult> execute(const String conn_name, const String p_stmt);
	Ref<SqlResult> execute_prepared(const String conn_name, const String p_stmt, Array binds = Array());
	

	//Sobrecarregar asgunções execute com void ? ou execute_async?

	MySQL();
	~MySQL();

};


VARIANT_ENUM_CAST(MySQL::CONN_TYPE);
VARIANT_ENUM_CAST(MySQL::ssl_mode);



#endif // MYSQL_H


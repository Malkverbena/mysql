/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H



#include "sql_conn.h"
#include "sql_result.h"
#include "sql_collations.h"



//=====================================
class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);


private:

	std::map<String, VariantConn> connections_holder;

	Ref<SqlResult> _execute(const String conn_name, const String p_stmt, const bool prep, const Array binds);


protected:

	static void _bind_methods();


public:

	// Add a new connection to the connection stack.
	// CONN_NAME: The name of the connection.
	// TYPE: Use TCP or UNIX SOCKET.
	// ASYNC: Determines that the new connection will be asynchronous.
	// SSL: Determines how the new connection will use SSL.
	Error new_connection(const String conn_name, SqlCollations::CONN_TYPE type);   //****


	// Setup new connections.  >> https://www.boost.org/doc/libs/develop/libs/mysql/doc/html/mysql/ref/boost__mysql__handshake_params.html
	Error set_credentials(const String conn_name,
									String p_username,
									String p_password,
									String p_database		= String(),
									std::uint16_t collation	= SqlCollations::default_collation,
									SqlCollations::ssl_mode p_ssl	= SqlCollations::ssl_mode::enable,
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

	SqlCollations::CONN_TYPE get_connection_type(const String conn_name);

	// Establishes a connection to a MySQL server.
	Error tcp_connect(const String conn_name, const String p_hostname = "127.0.0.1", const String p_port = "3306", const bool p_async = true);
	Error unix_connect(const String conn_name, const String p_socket_path, const bool p_async = false);

	// Closes the connection to the server.
	Error sql_disconnect(const String conn_name);

	Ref<SqlResult> execute(const String conn_name, const String p_stmt);
	Ref<SqlResult> execute_prepared(const String conn_name, const String p_stmt, const Array binds = Array());


	MySQL();
	~MySQL();

};



#endif // MYSQL_H


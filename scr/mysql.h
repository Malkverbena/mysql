/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H


#include "sql_conn.h"
#include <variant>

//=====================================
class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);


public:

	enum ssl_mode{
		disable	= (int)boost::mysql::ssl_mode::disable,
		enable	= (int)boost::mysql::ssl_mode::enable,
		require	= (int)boost::mysql::ssl_mode::require,
	};

	// TODO: Passar para classe conn
	enum CONN_TYPE{
		NONE		= 0,
		TCP		= 1,
		TCP_SSL	= 2,
		UNIX		= 3,
		UNIX_SSL	= 4
	};

	using VariantConn = std::variant<std::monostate,
								std::shared_ptr<ConnTcp>,
								std::shared_ptr<ConnTcpSsl>, 
								std::shared_ptr<ConnUnix>,
								std::shared_ptr<ConnUnixSsl>>;

	CONN_TYPE get_conn_type(VariantConn v);
	
private:

	Dictionary _last_error;
	std::map<String, VariantConn> connections_holder;


	Ref<SqlResult> _execute(const String conn_name, String stmt, bool prep, Array binds = Array());




protected:

	static void _bind_methods();
	void SQLException(const boost::mysql::error_with_diagnostics &err);
	void runtime_error(const std::exception& err);


public:


// ==== CONNECTION ==== /

	// Add a new connection to the connection stack.
	//	CONN_NAME: The name of the connection.
	// TYPE: Use TCP or UNIX SOCKET.
	// ASYNC: Determines that the new connection will be asynchronous.
	// SSL: Determines how the new connection will use SSL.
	Error new_connection(const String conn_name, CONN_TYPE type = TCP);


	// Setup new connections.  >> https://www.boost.org/doc/libs/develop/libs/mysql/doc/html/mysql/ref/boost__mysql__handshake_params.html
	Error set_credentials(const String conn_name,
									String p_username,
									String p_password,
									String p_database			= String(),
									std::uint16_t collation	= handshake_params::default_collation,
									MySQL::ssl_mode p_ssl	= MySQL::ssl_mode::disable,
									bool multi_queries		= false);

	// Delete a connection from connection stack.
	//	CONN_NAME: The name of the connection to be deleted.
	Error delete_connection(const String conn_name);

	// List all connections in the stack
	PackedStringArray get_connections() const;



	// Get handshake parameters
	Dictionary get_credentials(const String conn_name);

	// Establishes a connection to a MySQL server.
	Error sql_connect(const String conn_name, String hostname = "127.0.0.1", String port = "3306", bool sync = true, String socket_path = "");

	// Closes the connection to the server.
	Error sql_disconnect(const String conn_name);



// ==== 	QUERY ==== /

	Ref<SqlResult> execute(const String conn_name, String stmt);
//	Ref<SqlResult> execute_prepared(const String conn_name, String stmt, Array binds = Array());
	




	MySQL();
	~MySQL();

};


VARIANT_ENUM_CAST(MySQL::CONN_TYPE);
VARIANT_ENUM_CAST(MySQL::ssl_mode);





#endif // MYSQL_H


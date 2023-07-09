/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H


#include "sql_conn.h"

using namespace std;




//=====================================
class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);

friend class ConnTcpSsl;

public:

	enum ssl_mode{
		disable	= (int)boost::mysql::ssl_mode::disable,
		enable	= (int)boost::mysql::ssl_mode::enable,
		require	= (int)boost::mysql::ssl_mode::require,
	};

	enum CONN_TYPE{
		TCP,
		SOCKET,
	};



private:

	Dictionary _last_error;

	class ConnBox{
	public:
		CONN_TYPE type;
		bool async;
		bool tls;
		std::shared_ptr<ConnTcpSsl> conn_ptr;
		ConnBox( CONN_TYPE _type = TCP, bool _async = false, bool _tls = false ){
			async = _async;
			type = _type;
			tls = _tls;
			conn_ptr.reset(new ConnTcpSsl());
		}
	};

	std::map<String, ConnBox> connections_holder;	//TODO: Make connections a template

	Ref<SqlResult> _execute(const String conn_name, String stmt, bool prep, Array binds = Array());

protected:

	static void _bind_methods();
	void SQLException(const boost::mysql::error_with_diagnostics &err);
	void runtime_error(const std::exception& err);


public:


/* ==== CONNECTION ==== */

	// Add a new connection to the connection stack.
	//	CONN_NAME: The name of the connection.
	// TYPE: Use TCP or UNIX SOCKET.
	// ASYNC: Determines that the new connection will be asynchronous.
	// SSL: Determines how the new connection will use SSL.
	Error new_connection(const String conn_name, CONN_TYPE type = TCP, bool async = false, bool tls = false);

	// Delete a connection from connection stack.
	//	CONN_NAME: The name of the connection to be deleted.
	Error delete_connection(const String conn_name);

	// List all connections in the stack
	PackedStringArray get_connections() const;

	// Setup new connections.  >> https://www.boost.org/doc/libs/develop/libs/mysql/doc/html/mysql/ref/boost__mysql__handshake_params.html
	Error set_credentials(const String conn_name,
									String p_username,
									String p_password,
									String p_database			= String(),
									std::uint16_t collation	= handshake_params::default_collation,
									MySQL::ssl_mode p_ssl	= MySQL::ssl_mode::disable,
									bool multi_queries		= false);

	// Get handshake parameters
	Dictionary get_credentials(const String conn_name);

	// Establishes a connection to a MySQL server.
	Error sql_connect(const String conn_name, String hostname = "127.0.0.1", String port = "3306");

	// Closes the connection to the server.
	Error sql_disconnect(const String conn_name);



/* ==== 	QUERY ==== */

	Ref<SqlResult> execute(const String conn_name, String stmt);
//	Ref<SqlResult> execute_prepared(const String conn_name, String stmt, Array binds = Array());
	




	MySQL();
	~MySQL();
};
















VARIANT_ENUM_CAST(MySQL::CONN_TYPE);
VARIANT_ENUM_CAST(MySQL::ssl_mode);

#endif // MYSQL_H


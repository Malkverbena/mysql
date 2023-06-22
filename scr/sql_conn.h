/* sql_conn.h */

#ifndef 	SQL_CONN_H
#define SQL_CONN_H


//#include "helpers.h"


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


using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::mysql;

//	template <class T>
// return class name: typeid(this).name()

class Conn{
public:
};


/* ===== TCP  TYPE: tcp_connection ===== */
/* ===== TCP_SSL  TYPE: tcp_ssl_connection ===== */





class ConnTcpSsl : public Conn {
friend class MySQL;

private:
	// Common
	io_context ctx;								// boost::asio context
	ip::tcp::resolver::results_type eps;	// Physical endpoint(s) to connect to
	handshake_params conn_params;				// MySQL credentials and other connection config
	ip::tcp::resolver resolver;				// To perform hostname resolution

	// TCP Connection
	ssl::context ssl_ctx;						// MySQL 8+ default settings require SSL
	tcp_ssl_connection conn;					// Represents the connection to the MySQL server



public:

	const PackedStringArray param_names = {
		String("connection_collation"), String("database"), String("multi_queries"), String("password"), String("username"), String("hostname"), String("port")
	};

	// Set connection parameters.
	Error set_param(String param, Variant p_value);

	// Retrives connection parameters.
	Variant get_param(String param);						

	// Establish the connection  NOTE: must be inside try{}
	void connect(mysql::string_view hostanme, mysql::string_view port = "3306");

	// Closes the connection.
	void disconnect();

	// The configuration of the parameters is done inside the Constructor. 
	ConnTcpSsl(	mysql::string_view	username, 
					mysql::string_view	password,
					mysql::string_view	schema,
					std::uint16_t			collation,
					mysql::ssl_mode		ssl,
					bool 						multi_queries,
					bool						async)
	:	conn_params(username, password, schema, collation, ssl, multi_queries),
		resolver(ctx.get_executor()),
		ssl_ctx(boost::asio::ssl::context::tls_client),
		conn(ctx, ssl_ctx)
	{
	}
	~ConnTcpSsl();
};




/* ===== UNIX  TYPE: unix_connection ===== */
/* ===== UNIX_SSL  TYPE: unix_ssl_connection ===== */










#endif  // SQL_CONN_H

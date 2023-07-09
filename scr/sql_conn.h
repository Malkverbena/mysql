/* sql_conn.h */

#ifndef 	SQL_CONN_H
#define SQL_CONN_H


#include "sql_result.h"




/*
set_querty()
	use_literal -> User stringLiteral "R" for querties
	set_meta -> set the return of metadata - metadata_mode
	



*/



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
	io_context ctx;												// boost::asio context
	ip::tcp::resolver::results_type eps;					// Physical endpoint(s) to connect to
//	boost::asio::local::stream_protocol::endpoint eps;
//	boost::asio::ssl::stream eps;
//	auto endpoint;
	
	boost::mysql::handshake_params conn_params;			// MySQL credentials and other connection config
	ip::tcp::resolver resolver;								// To perform hostname resolution

	// TCP Connection
	ssl::context ssl_ctx;										// MySQL 8+ default settings require SSL
	tcp_ssl_connection conn;									// Represents the connection to the MySQL server

	char *username;
	char *password;
	char *database;


public:
	// The configuration of the parameters is done inside the Constructor. 
	ConnTcpSsl()
	:	conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		resolver(ctx.get_executor()),
		ssl_ctx(boost::asio::ssl::context::tls_client),
		conn(ctx.get_executor(), ssl_ctx)
	{
	}
	~ConnTcpSsl(){}

};





/* ===== UNIX  TYPE: unix_connection ===== */
/* ===== UNIX_SSL  TYPE: unix_ssl_connection ===== */










#endif  // SQL_CONN_H

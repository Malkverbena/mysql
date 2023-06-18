/* sql_conn.h */

#ifndef 	SQL_CONN_H
#define SQL_CONN_H


#include "helpers.h"


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
//using namespace boost::mysql;

// === SQL aceita const char *
// === Retirnar result como um objeto(classe)

//	template <class T>
// return class name: typeid(this).name()


/* ===== TCP  TYPE: tcp_connection ===== */

/* ===== TCP_SSL  TYPE: tcp_ssl_connection ===== */



class Conn{

public:


};


//--------------------------------------

class ConnTcpSsl : public Conn {

friend class MySQL;

private:
	bool async = false;
//	bool tls = true;

	mysql::string_view username = "";
	mysql::string_view password = "";
	mysql::string_view schema = "";
	std::uint16_t collation = 45;  //FIXME: dESCOBRIR O TIPO DE DADO DDE COLLATION

	mysql::string_view host = "localhost";
	int port = 3306;
	
	// Common
	unique_ptr<asio::ip::tcp::resolver::results_type> eps;	// Physical endpoint(s) to connect to
	unique_ptr<asio::io_context> ctx;								// boost::asio context
	unique_ptr<mysql::handshake_params> conn_params;			// MySQL credentials and other connection config
	unique_ptr<asio::ip::tcp::resolver> resolver;				// To perform hostname resolution

	// TCP Connection
	unique_ptr<mysql::tcp_ssl_connection> conn;					// Represents the connection to the MySQL server
	unique_ptr<asio::ssl::context> ssl_ctx;						// MySQL 8+ default settings require SSL

protected:

	Error set_param(String param, Variant p_value);	// Set connection parameters.
	Variant get_param(String param);						// Retrives connection parameters.


	Error run();	// setup the connection's objects


	void start();					// Setup connection (run)
	Error connect();				// Establish the connection - must be inside try{}
	void disconnect();			// Disconnect & cleanning.
	int status();					// Return the connection status

public:
	ConnTcpSsl(bool _async = false);
	~ConnTcpSsl();

};




/* ===== UNIX  TYPE: unix_connection ===== */

/* ===== UNIX_SSL  TYPE: unix_ssl_connection ===== */










#endif  // SQL_CONN_H

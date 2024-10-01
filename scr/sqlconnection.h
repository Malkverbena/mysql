/* sqlconnection.h */

#ifndef SQLCONNECTION_H
#define SQLCONNECTION_H


#include "sqlcertificate.h"

#include <boost/asio/io_context.hpp>
#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/any_address.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>



using namespace sqlhelpers;
using namespace boost::asio;


class SqlConnection : public RefCounted {
	GDCLASS(SqlConnection, RefCounted);
	friend class MySQL;


public:

	using MysqlCollations = MYSQLCOLLATIONS;
	enum ConnType {NONE, TCP, TCP_TLS, UNIX};
	enum ConnectionStatus {DISCONNECTED, CONNECTED};
	enum AddressType {
		host_and_port = static_cast<int>(boost::mysql::address_type::host_and_port),
		unix_path = static_cast<int>(boost::mysql::address_type::unix_path)
	};


private:

	Ref<SqlCertificate> certificate;
	Dictionary last_sql_error;

	ConnType conn_type;
	ConnectionStatus status;

	bool use_certificate;
	bool enable_async = false;

	boost::mysql::connect_params credentials_params;
	boost::mysql::any_connection_params ctor_params;

	std::shared_ptr<asio::io_context> ctx = nullptr;
	std::shared_ptr<mysql::any_connection> conn = nullptr;




public:

	SqlConnection(const Ref<SqlCertificate> &p_certificate = nullptr) : certificate(p_certificate)  {
		credentials_params.connection_collation = mysql_collations::utf8mb4_general_ci;
		credentials_params.connection_collation = false;
		credentials_params.ssl = ssl_mode::enable;
	}

	~SqlConnection();
	


protected:

	static void _bind_methods();




public:


	Dictionary get_sql_error() const {return last_sql_error.duplicate(true);};

	// Establishes a connection to a MySQL server.
	// "HostName" can be a  UNIX socket path or an IP/HostName. In case of UNIX connections the port will be ignorated.
	Error db_connect(const String p_hostname = "/var/run/mysqld/mysqld.sock", const int p_port = 3307);

	// Cleanly closes the connection to the server.
	// Sends a quit request. This is required by the MySQL protocol, to inform the server that we're closing the connection gracefully.
	Error close_connection();

	//Resets server-side session state, like variables and prepared statements.
	Error reset_connection();

	// Checks whether the server is alive.
	bool is_server_alive();

	// Configure the type of connection and if the connection will use a ssl context class to handle certificates.
	// The context must me configurated before configure the connection. Otherwise it it will return an error.
	Error configure_connection(ConnType connectiontype = TCP, bool p_use_certificate = false);


	void set_async(bool p_enable_async) { enable_async = p_enable_async; }
	bool get_async() const { return enable_async; }
	ConnType get_connection_type() const { return conn_type; }

	// Returns if the connection is configurated to use certificates.
	bool is_using_certificate() const {return use_certificate; }

	// Returns whether the connection negotiated the use of SSL or not.
	// This function can be used to determine whether you are using a SSL connection or not when using SSL negotiation.
	// This function always returns false for connections that haven't been established yet. If the connection establishment fails, the return value is undefined.
	bool get_uses_ssl() const { return conn->uses_ssl(); }

	// Returns whether backslashes are being treated as escape sequences.
	// By default, the server treats backslashes in string values as escape characters.
	// This behavior can be disabled by activating the NO_BACKSLASH_ESCAPES SQL mode.
	// Every time an operation involving server communication completes, the server reports whether this mode was activated or not as part of the response.
	// Connections store this information and make it available through this function.
	// 		- If backslash are treated like escape characters, returns true.
	//		- If NO_BACKSLASH_ESCAPES has been activated, returns false.
	//		- If connection establishment hasn't happened yet, returns true.
	//		- Calling this function while an async operation that changes backslash behavior is outstanding may return true or false.
	// This function does not involve server communication.
	bool backslash_escapes() const { return conn->backslash_escapes(); }

	// Get credentials methods
	String get_username() const { return SQLstring_to_GDstring(credentials_params.username); }
	String get_password() const { return SQLstring_to_GDstring(credentials_params.password); }
	String get_database() const { return SQLstring_to_GDstring(credentials_params.database); }
	bool get_multi_queries() const { return credentials_params.multi_queries; }
	SqlCertificate::SSLMode get_ssl_mode() const { return SqlCertificate::SSLMode(credentials_params.ssl); }
	SqlConnection::MysqlCollations get_connection_collation() const { return static_cast<SqlConnection::MysqlCollations>(credentials_params.connection_collation); }

	// Set credentials methods
	void set_username(String p_username) { credentials_params.username = GDstring_to_SQLstring(p_username); }
	void set_password(String p_password) { credentials_params.password = GDstring_to_SQLstring(p_password); }
	void set_database(String p_database) { credentials_params.database = GDstring_to_SQLstring(p_database); }
	void set_ssl_mode(SqlCertificate::SSLMode p_mode){ credentials_params.ssl = static_cast<boost::mysql::ssl_mode>(p_mode); }
	void set_multi_queries(bool p_multi_queries) { credentials_params.multi_queries = p_multi_queries; }
	void set_connection_collation(SqlConnection::MysqlCollations p_collation) { credentials_params.connection_collation = p_collation; }





};


VARIANT_ENUM_CAST(SqlConnection::ConnectionStatus);
VARIANT_ENUM_CAST(SqlConnection::MysqlCollations);
VARIANT_ENUM_CAST(SqlConnection::ConnType);
VARIANT_ENUM_CAST(SqlConnection::AddressType);



#endif // SQLCONNECTION_H

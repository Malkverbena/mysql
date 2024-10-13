/* sqlconnection.h */

#ifndef SQLCONNECTION_H
#define SQLCONNECTION_H


#include "sqlcertificate.h"

#include <boost/asio/io_context.hpp>
#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/any_address.hpp>




using namespace sqlhelpers;
using namespace boost::asio;


class SqlConnection : public RefCounted {
	GDCLASS(SqlConnection, RefCounted);
	friend class MySQL;


protected:

	static void _bind_methods();



public:

	using MysqlCollations = MYSQLCOLLATIONS;
	enum ConnType {NONE, TCP, TCP_TLS, UNIX};
	enum ConnectionStatus {DISCONNECTED, CONNECTED};
	enum AddressType {
		host_and_port = static_cast<int>(boost::mysql::address_type::host_and_port),
		unix_path = static_cast<int>(boost::mysql::address_type::unix_path)
	};


private:
	mutable std::mutex connection_mutex;

	Ref<SqlCertificate> certificate;

	ConnType conn_type;
	ConnectionStatus status;

	bool use_certificate;

	boost::mysql::connect_params credentials_params;
	boost::mysql::any_connection_params ctor_params;

	std::shared_ptr<asio::io_context> ctx = nullptr;
	std::shared_ptr<mysql::any_connection> conn = nullptr;

	awaitable<Dictionary> coro_async_db_connect(boost::mysql::error_code& ec, boost::mysql::diagnostics& diag);



public:

	SqlConnection(const Ref<SqlCertificate> &p_certificate = nullptr) : certificate(p_certificate)  {
		credentials_params.connection_collation = mysql_collations::utf8mb4_general_ci;
		credentials_params.connection_collation = false;
		credentials_params.ssl = ssl_mode::enable;
	}

	~SqlConnection();



	Dictionary async_db_connect(const String p_hostname = "/var/run/mysqld/mysqld.sock", const int p_port = 3307);

//	Error async_close_connection();

//	Error async_reset_connection();







	// Establishes a connection to a MySQL server.
	// "HostName" can be a  UNIX socket path or an IP/HostName. In case of UNIX connections the port will be ignorated.
	Dictionary db_connect(const String p_hostname = "/var/run/mysqld/mysqld.sock", const int p_port = 3307);


	// Cleanly closes the connection to the server.
	// Sends a quit request. This is required by the MySQL protocol, to inform the server that we're closing the connection gracefully.
	Dictionary close_connection();


	//Resets server-side session state, like variables and prepared statements.
	Dictionary reset_connection();


	// Checks whether the server is alive.
	bool is_server_alive();

	// Configure the type of connection and if the connection will use a ssl context class to handle certificates.
	// The context must me configurated before configure the connection. Otherwise it it will return an error.
	Error configure_connection(ConnType connectiontype = TCP, bool p_use_certificate = false);


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
	String get_username() const;
	String get_password() const;
	String get_database() const;
	bool get_multi_queries() const ;
	SqlCertificate::SSLMode get_ssl_mode() const ;
	int get_connection_collation() const ;


	// Set credentials methods
	void set_username(String p_username);
	void set_password(String p_password);
	void set_database(String p_database);
	void set_ssl_mode(SqlCertificate::SSLMode p_mode);
	void set_multi_queries(bool p_multi_queries);
	void set_connection_collation(const int p_collation);



};


VARIANT_ENUM_CAST(SqlConnection::ConnectionStatus);
VARIANT_ENUM_CAST(SqlConnection::MysqlCollations);
VARIANT_ENUM_CAST(SqlConnection::ConnType);
VARIANT_ENUM_CAST(SqlConnection::AddressType);




#endif // SQLCONNECTION_H

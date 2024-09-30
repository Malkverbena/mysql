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


public:

	enum ConnType {NONE, TCP, TCP_TLS, UNIX};
	enum ConnectionStatus {DISCONNECTED, CONNECTED};
	using MysqlCollations = MYSQLCOLLATIONS;

protected:

	static void _bind_methods();



private:

	ConnType conn_type;
	ConnectionStatus status;

	bool use_certificate;
	bool enable_async;

//   char *username;
//	char *password;
//	char *database;


	boost::mysql::connect_params credential_params;
	boost::mysql::any_connection_params ctor_params;

	std::shared_ptr<asio::io_context> ctx;
	std::shared_ptr<mysql::any_connection> conn = nullptr;




public:

	SqlConnection(){
		credential_params.connection_collation = mysql_collations::utf8mb4_general_ci;
		credential_params.connection_collation = false;
		credential_params.ssl = ssl_mode::enable;
	}


	~SqlConnection(){
		// close_connection
	}







	bool get_uses_ssl() { return conn->uses_ssl(); }

	String get_username() { return SqlStr2GdtStr(credential_params.username); }
	String get_password() { return SqlStr2GdtStr(credential_params.password); }
	String get_database() { return SqlStr2GdtStr(credential_params.database); }
	bool get_multi_queries() { return credential_params.multi_queries; }
	SqlCertificate::SSLMode get_ssl_mode() { return SqlCertificate::SSLMode(credential_params.ssl); }
	SqlConnection::MysqlCollations get_connection_collation() { return static_cast<SqlConnection::MysqlCollations>(credential_params.connection_collation); }

	void set_username(String p_username) { credential_params.username = GdtStr2SqlStr(p_username); }
	void set_password(String p_password) { credential_params.password = GdtStr2SqlStr(p_password); }
	void set_database(String p_database) { credential_params.database = GdtStr2SqlStr(p_database); }
	void set_ssl_mode(SqlCertificate::SSLMode p_mode){ credential_params.ssl = static_cast<boost::mysql::ssl_mode>(p_mode); }
	void set_multi_queries(bool p_multi_queries) { credential_params.multi_queries = p_multi_queries; }
	void set_connection_collation(SqlConnection::MysqlCollations p_collation) { credential_params.connection_collation = p_collation; }























};


VARIANT_ENUM_CAST(SqlConnection::ConnectionStatus);
VARIANT_ENUM_CAST(SqlConnection::MysqlCollations);
VARIANT_ENUM_CAST(SqlConnection::ConnType);


#endif // SQLCONNECTION_H

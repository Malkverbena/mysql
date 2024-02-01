/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H


#include "helpers.h"
#include "sql_result.h"

#include <memory>
#include <fstream>
#include <stdexcept>

#include <core/config/project_settings.h>

#include <boost/mysql/handshake_params.hpp>
#include <boost/mysql/unix.hpp>
#include <boost/mysql/unix_ssl.hpp>
#include <boost/mysql/tcp.hpp>
#include <boost/mysql/tcp_ssl.hpp>

#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/detached.hpp>


using namespace boost::asio;

constexpr auto tuple_awaitable = as_tuple(use_awaitable);


class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);


protected:

	static void _bind_methods();


public:

	using ConnType = CONN_TYPE;
	using MysqlCollations = MYSQLCOLLATIONS;

	enum SslMode {
		ssl_disable	= (int)mysql::ssl_mode::disable,
		ssl_enable	= (int)mysql::ssl_mode::enable,
		ssl_require	= (int)mysql::ssl_mode::require,
	};


private:

	//Error
	Dictionary last_error;

	// Status
	ConnType type = TCPTLS;

	// Params
	char *username;
	char *password;
	char *database;
	mysql::handshake_params conn_params;

	// Aux
	std::shared_ptr<asio::io_context> ctx = nullptr;
	std::shared_ptr<asio::ssl::context> ssl_ctx = nullptr;
	std::shared_ptr<asio::ip::tcp::resolver> resolver = nullptr;

	// Connection
	std::shared_ptr<mysql::tcp_connection> tcp_conn = nullptr;
	std::shared_ptr<mysql::tcp_ssl_connection> tcp_ssl_conn = nullptr;
	std::shared_ptr<mysql::unix_connection> unix_conn = nullptr;
	std::shared_ptr<mysql::unix_ssl_connection> unix_ssl_conn = nullptr;

	asio::awaitable<void> coro_execute(const char* query, std::shared_ptr<mysql::results> result);
	asio::awaitable<void> coro_execute_prepared(const char* query, std::vector<mysql::field> args, std::shared_ptr<mysql::results> result);


private:

	Error set_certificate(const String p_cert_file, const String p_host_name);

	void build_result(mysql::results raw_result, Ref<SqlResult> *gdt_result);

	Dictionary get_metadata(mysql::results result);

	Dictionary get_raw(mysql::results result);

	Array multiqueries(std::string queries);

	Ref<SqlResult> build_godot_result(mysql::results result);

	Ref<SqlResult> build_godot_result(
		mysql::rows_view batch, mysql::metadata_collection_view meta_collection,
		std::uint64_t affected_rows, std::uint64_t last_insert_id, unsigned warning_count
	);


public:

	// The define method will configure a connection.
	// If there is already an active connection, it will be disconnected and reseted.
	// The path to the certificate and hostname are intended for use with TLS connections only.
	// In non-TLS connections the certificate  and the hostname will be ignored.
	// If the user wishes to modify the certificate, the connection must be reseted.
	// Resetting the connection does not reset the credentials.
	Error define(const ConnType _type = TCPTLS, const String p_cert_file = "", const String p_host_name = "mysql");

	// Retrieves the connection type.
	ConnType get_connection_type() const { return type;};

	// Method used to initiate a TCP connection.
	// It will not work with UNIX-type connections.
	Error tcp_connect(const String p_hostname = "127.0.0.1", const String p_port = "3306");

	// Method used to connect to the database via Socket.
	// It will not work with TCP-type connections.
	Error unix_connect(const String p_socket_path = "/var/run/mysqld/mysqld.sock");

	// Close the connection.
	Error close_connection();

	// Returns a dictionary with the last exception that occurred.
	Dictionary get_last_error() const {return last_error.duplicate(true);};

	// Checks whether there is an active connection to the server.
	bool is_server_alive();

	// Execute queries.
	Ref<SqlResult> execute(const String p_stmt);

	// Execute prepared queries.
	Ref<SqlResult> execute_prepared(const String p_stmt, const Array binds = Array());

	// Execute sql scripts.
	// This function perform multi-queries, because this the "multi_queries" option must be true in the connection credentials,
	// otherwise this function won't be executed.
	// Be extremely careful with this function.
	Array execute_sql(const String p_path_to_file);

	// Execute Multi-function operations.
	// You can use multi-function operations to execute several function text queries.
	// This function return an Array with a SqlResult for each query executed.
	Array execute_multi(const String p_queries);

	// Returns a dictionary with the connection credentials.
	Dictionary get_credentials() const;

	// Configure connection credentials.
	Error set_credentials(
			String p_username,
			String p_password,
			String p_database		= String(),
			std::uint16_t collation	= default_collation,
			SslMode p_ssl			= ssl_enable,
			bool multi_queries		= false
	);

	// Execute queries on asynchronous connections.
	Ref<SqlResult> async_execute(const String p_stmt);

	// Execute prepared queries on asynchronous connections.
	Ref<SqlResult> async_execute_prepared(const String p_stmt, const Array binds = Array());

	MySQL():
		conn_params((const char *)&username, (const char *)&password, (const char *)&database)
	{}
	~MySQL(){
		close_connection();
	}

};

VARIANT_ENUM_CAST(MySQL::MysqlCollations);
VARIANT_ENUM_CAST(MySQL::ConnType);
VARIANT_ENUM_CAST(MySQL::SslMode);


#endif // MYSQL_H
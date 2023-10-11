/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H

#include "helpers.h"



class SqlConnection : public RefCounted {
	GDCLASS(SqlConnection, RefCounted);

friend class MySQL;


public:

	using MysqlCollations = COLLATIONS;
	using ConnType = CONN_TYPE;
	
	enum ssl_mode {
		disable	= (int)mysql::ssl_mode::disable,
		enable	= (int)mysql::ssl_mode::enable,
		require	= (int)mysql::ssl_mode::require,
	};

protected:

	static void _bind_methods();


private:
	static constexpr auto tuple_awaitable = asio::as_tuple(asio::use_awaitable);

	std::shared_ptr<asio::io_context> ctx;
	String ID;
	bool async = false;
	bool busy = false;
	ConnType conn_type;
	char *username;
	char *password;
	char *database;
	mysql::handshake_params conn_params;

	std::shared_ptr<asio::ip::tcp::resolver> resolver = nullptr;
	std::shared_ptr<asio::ssl::context> ssl_ctx = nullptr;

	std::shared_ptr<mysql::tcp_connection> tcp_conn = nullptr;
	std::shared_ptr<mysql::tcp_ssl_connection> tcp_ssl_conn = nullptr;
	std::shared_ptr<mysql::unix_connection> unix_conn = nullptr;
	std::shared_ptr<mysql::unix_ssl_connection> unix_ssl_conn = nullptr;


public:

	SqlConnection():
		conn_params((const char *)&username, (const char *)&password, (const char *)&database)
	{}

	void initialize() {
		if (conn_type == TCP) {
			resolver = std::make_shared<asio::ip::tcp::resolver>(ctx->get_executor());
			tcp_conn = std::make_shared<mysql::tcp_connection>(ctx->get_executor());
			tcp_conn->set_meta_mode(mysql::metadata_mode::full);
		}else if (conn_type == TCPSSL) {
			ssl_ctx = std::make_shared<asio::ssl::context>(ssl::context::tls_client);
			resolver = std::make_shared<asio::ip::tcp::resolver>(ctx->get_executor());
			tcp_ssl_conn = std::make_shared<mysql::tcp_ssl_connection>(ctx->get_executor(), *ssl_ctx);
			tcp_ssl_conn->set_meta_mode(mysql::metadata_mode::full);
		}
		else if (conn_type == UNIX) {
			unix_conn = std::make_shared<mysql::unix_connection>(*ctx);
			tcp_ssl_conn->set_meta_mode(mysql::metadata_mode::full);
		}
		else if (conn_type == UNIXSSL) {
			ssl_ctx = std::make_shared<asio::ssl::context>(ssl::context::tls_client);
			unix_ssl_conn = std::make_shared<mysql::unix_ssl_connection>(*ctx, *ssl_ctx);
			tcp_ssl_conn->set_meta_mode(mysql::metadata_mode::full);
		}
	}


};



class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);


private:

	Dictionary last_sql_exception;
	Dictionary last_boost_exception;
	Dictionary last_std_exception;


	std::map<String, std::shared_ptr<SqlConnection>> connectionMap;

	boost::asio::awaitable<void> coro_connect_tcp(std::shared_ptr<SqlConnection>& conn, const char* hostname, const char* port);

	boost::asio::awaitable<void> coro_connect_unix(std::shared_ptr<SqlConnection>& conn, const char* socket);

	Ref<SqlResult> _execute(const String conn_name, const String p_stmt, const bool prep, const Array binds);

	void _async_execute(const String conn_name, const String p_stmt, const bool prep, const Array binds);

	Ref<SqlResult> make_godot_result(mysql::results result);

	boost::asio::awaitable<void> coro_execute(
		std::shared_ptr<SqlConnection> conn, const char* s_stmt, std::vector<mysql::field> *args, const bool prep
	);


protected:

	static void _bind_methods();


public:
	Error new_connection(const String conn_name, SqlConnection::ConnType type = TCPSSL);

	Error delete_connection(const String conn_name);

	PackedStringArray get_connections() const;

	SqlConnection::ConnType get_connection_type(const String conn_name) const;

	bool is_async(const String conn_name) const;

	Error set_certificate(const String conn_name, const String p_cert_file, const String p_host_name = "mysql");

	Dictionary get_credentials(const String conn_name) const;

	Error set_credentials(
		const String conn_name,
		const String p_username,
		const String p_password,
		const String p_database							= "",
		const SqlConnection::MysqlCollations collation	= SqlConnection::MysqlCollations::default_collation,
		const SqlConnection::ssl_mode p_ssl				= SqlConnection::ssl_mode::enable,
		const bool multi_queries						= true
	);

	
	Error initialize_connection(const String conn_name);

	// "/var/run/mysqld/mysqld.sock"
	Error connect_unix(const String conn_name, const String p_socket_path = "/tmp/mysql.sock", bool p_async = false);

	Error connect_tcp(const String conn_name, const String p_hostname = "127.0.0.1", const String p_port = "3306", bool p_async = false);

	Error close_connection(const String conn_name);

	bool is_connectioin_alive(const String conn_name) const;

	Ref<SqlResult> execute(const String conn_name, const String p_stmt);

	Ref<SqlResult> execute_prepared(const String conn_name, const String p_stmt, const Array binds = Array());

	void async_execute(const String conn_name, const String p_stmt);

	void async_execute_prepared(const String conn_name, const String p_stmt, const Array binds = Array());

	MySQL();
	~MySQL();

};


VARIANT_ENUM_CAST(SqlConnection::MysqlCollations);
VARIANT_ENUM_CAST(SqlConnection::ConnType);
VARIANT_ENUM_CAST(SqlConnection::ssl_mode);


#endif // MYSQL_H


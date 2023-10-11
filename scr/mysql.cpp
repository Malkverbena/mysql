
/* mysql.cpp */

#include "mysql.h"
#include <boost/mysql/statement.hpp>


boost::asio::awaitable<void> MySQL::coro_execute(
	std::shared_ptr<SqlConnection> conn, const char* s_stmt, std::vector<mysql::field> *args, const bool prep) {

	error_code ec;
	mysql::diagnostics diag;
	mysql::results result;
	SqlConnection::ConnType type = conn->conn_type;

	conn->busy = true;
	if (not prep) {
		if (type == SqlConnection::ConnType::TCP) {
			std::tie(ec) = co_await conn->tcp_conn->async_execute(s_stmt, result, diag, conn->tuple_awaitable);
		}
		else if (type == SqlConnection::ConnType::TCPSSL) {
			std::tie(ec) = co_await conn->tcp_ssl_conn->async_execute(s_stmt, result, diag, conn->tuple_awaitable);
		}
		else if (type == SqlConnection::ConnType::UNIX) {
			std::tie(ec) = co_await conn->unix_conn->async_execute(s_stmt, result, diag, conn->tuple_awaitable);
		}
		else if (type == SqlConnection::ConnType::UNIXSSL) {
			std::tie(ec) = co_await conn->unix_ssl_conn->async_execute(s_stmt, result, diag, conn->tuple_awaitable);
		}
		boost::mysql::throw_on_error(ec, diag);
	}

	else{
		boost::mysql::statement prep_stmt;
		if (type == SqlConnection::ConnType::TCP) {
			auto c = conn->tcp_conn;
			std::tie(ec, prep_stmt) = co_await c->async_prepare_statement(s_stmt, diag, conn->tuple_awaitable);
			boost::mysql::throw_on_error(ec, diag);
			std::tie(ec) = co_await c->async_execute(prep_stmt.bind(args->begin(), args->end()), result, diag, conn->tuple_awaitable);
			boost::mysql::throw_on_error(ec, diag);
		}
		else if (type == SqlConnection::ConnType::TCPSSL) {
			auto c = conn->tcp_ssl_conn;
			std::tie(ec, prep_stmt) = co_await c->async_prepare_statement(s_stmt, diag, conn->tuple_awaitable);
			boost::mysql::throw_on_error(ec, diag);
			std::tie(ec) = co_await c->async_execute(prep_stmt.bind(args->begin(), args->end()), result, diag, conn->tuple_awaitable);
			boost::mysql::throw_on_error(ec, diag);
		}
		else if (type == SqlConnection::ConnType::UNIX) {
			auto c = conn->unix_conn;
			std::tie(ec, prep_stmt) = co_await c->async_prepare_statement(s_stmt, diag, conn->tuple_awaitable);
			boost::mysql::throw_on_error(ec, diag);
			std::tie(ec) = co_await c->async_execute(prep_stmt.bind(args->begin(), args->end()), result, diag, conn->tuple_awaitable);
			boost::mysql::throw_on_error(ec, diag);
		}
		else if (type == SqlConnection::ConnType::UNIXSSL) {
			auto c = conn->unix_ssl_conn;
			std::tie(ec, prep_stmt) = co_await c->async_prepare_statement(s_stmt, diag, conn->tuple_awaitable);
			boost::mysql::throw_on_error(ec, diag);
			std::tie(ec) = co_await c->async_execute(prep_stmt.bind(args->begin(), args->end()), result, diag, conn->tuple_awaitable);
		}
	}

	conn->busy = false;
	this->emit_signal("querty_complete", make_godot_result(result), conn->ID);
}


void MySQL::async_execute(const String conn_name, const String p_stmt) {
	return _async_execute(conn_name, p_stmt, false, Array());
}


void MySQL::async_execute_prepared(const String conn_name, const String p_stmt, const Array binds) {
	return _async_execute(conn_name, p_stmt, true, binds);
}


void MySQL::_async_execute(const String conn_name, const String p_stmt, const bool prep, const Array binds) {

	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_EDMSG(p_con == connectionMap.end(), "Connection not found.");
	ERR_FAIL_COND_EDMSG(not p_con->second->async, "This method only works with asynchronous connections. For synchronous connections use synchronous functions.");

    auto conn = p_con->second;
    const char* s_stmt = p_stmt.utf8().get_data();
    std::vector<mysql::field> args = binds_to_field(binds);

    try {
		conn->busy = true;
		boost::asio::co_spawn(
			conn->ctx->get_executor(),
			[conn, s_stmt, &args, prep, this]  {
				return coro_execute(conn, s_stmt, &args, prep);
			},
			[conn](std::exception_ptr ptr) {
				if (ptr)
				{	
					conn->busy = false;
					std::rethrow_exception(ptr);
				}
			}
		);
	
    } catch (const mysql::error_with_diagnostics& diags) {
		conn->busy = false;
        HANDLE_SQL_EXCEPTION_IN_COROTINE(diags, &last_sql_exception);
    } catch (const mysql::error_code& ec) {
		conn->busy = false;
        HANDLE_BOOST_EXCEPTION_IN_COROTINE(ec, &last_boost_exception);
    } catch (const std::exception& err) {
		conn->busy = false;
        std::cerr << "Error: " << err.what() << std::endl;
    }

}


Ref<SqlResult> MySQL::execute(const String conn_name, const String p_stmt) {
	return _execute(conn_name, p_stmt, false, Array());
}


Ref<SqlResult> MySQL::execute_prepared(const String conn_name, const String p_stmt, const Array binds) {
	return _execute(conn_name, p_stmt, true, binds);
}


Ref<SqlResult> MySQL::_execute(const String conn_name, const String p_stmt, const bool prep, const Array binds) {

	Ref<SqlResult> sql_result;
	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), sql_result, "Connection not found.");

	ERR_FAIL_COND_V_EDMSG(p_con->second->async, sql_result,
	"This method only works with synchronous connections. For asynchronous connections use asynchronous functions.");

	const char* s_stmt = p_stmt.utf8().get_data();
	auto conn = p_con->second;
	SqlConnection::ConnType type = conn->conn_type;
	mysql::results result;	

	try{
		conn->busy = true;
		if (not prep) {
			if (type == SqlConnection::ConnType::TCP) {
				conn->tcp_conn->execute(s_stmt, result);
			}
			else if (type == SqlConnection::ConnType::TCPSSL) {
				conn->tcp_ssl_conn->execute(s_stmt, result);
			}
			else if (type == SqlConnection::ConnType::UNIX) {
				conn->unix_conn->execute(s_stmt, result);
			}
			else if (type == SqlConnection::ConnType::UNIXSSL) {
				conn->unix_ssl_conn->execute(s_stmt, result);
			}
		}
		else{
			std::vector<mysql::field> args = binds_to_field(binds);

			if (type == SqlConnection::ConnType::TCP) {
				mysql::statement prep_stmt = conn->tcp_conn->prepare_statement(s_stmt);
				conn->tcp_conn->execute(prep_stmt.bind(args.begin(), args.end()), result);
			}
			else if (type == SqlConnection::ConnType::TCPSSL) {
				mysql::statement prep_stmt = conn->tcp_ssl_conn->prepare_statement(s_stmt);
				conn->tcp_ssl_conn->execute(prep_stmt.bind(args.begin(), args.end()), result);
			}
			else if (type == SqlConnection::ConnType::UNIX) {
				mysql::statement prep_stmt = conn->unix_conn->prepare_statement(s_stmt);
				conn->unix_conn->execute(prep_stmt.bind(args.begin(), args.end()), result);
			}
			else if (type == SqlConnection::ConnType::UNIXSSL) {
				mysql::statement prep_stmt = conn->unix_ssl_conn->prepare_statement(s_stmt);
				conn->unix_ssl_conn->execute(prep_stmt.bind(args.begin(), args.end()), result);
			}
		}
		
	
	}
	catch (const mysql::error_with_diagnostics& diags) {
		conn->busy = false;
		HANDLE_SQL_EXCEPTION(diags, &last_sql_exception, sql_result);
	}
	catch (const mysql::error_code& ec) {
		conn->busy = false;
		HANDLE_BOOST_EXCEPTION(ec, &last_boost_exception, sql_result);
	}
	catch (const std::exception& err) {
		conn->busy = false;
		HANDLE_STD_EXCEPTION(err, &last_std_exception, sql_result);
	}

	conn->busy = false;
	Ref<SqlResult> ret = MySQL::make_godot_result(result);
	return ret;

}



boost::asio::awaitable<void> MySQL::coro_connect_tcp(std::shared_ptr<SqlConnection>& conn, const char* hostname, const char* port) {
	error_code ec;
	mysql::diagnostics diag;
	auto endpoints = co_await conn->resolver->async_resolve(hostname, port, asio::use_awaitable);

	if (conn->conn_type == SqlConnection::ConnType::TCP) {
		std::tie(ec) = co_await conn->tcp_conn->async_connect(*endpoints.begin(), conn->conn_params, diag, conn->tuple_awaitable);
	}else{
		std::tie(ec) = co_await conn->tcp_ssl_conn->async_connect(*endpoints.begin(), conn->conn_params, diag, conn->tuple_awaitable);
	}
	mysql::throw_on_error(ec, diag);
}


boost::asio::awaitable<void> MySQL::coro_connect_unix(std::shared_ptr<SqlConnection>& conn, const char* socket) {
	error_code ec;
	mysql::diagnostics diag;
	local::stream_protocol::endpoint endpoints(socket);

	if (conn->conn_type == SqlConnection::ConnType::UNIX) {
		std::tie(ec) = co_await conn->unix_conn->async_connect(endpoints, conn->conn_params, diag, conn->tuple_awaitable);
	}else{
		std::tie(ec) = co_await conn->unix_ssl_conn->async_connect(endpoints, conn->conn_params, diag, conn->tuple_awaitable);
	}
	mysql::throw_on_error(ec, diag);
}


Error MySQL::initialize_connection(const String conn_name) {
	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	p_con->second->initialize();
	return OK;
}


Error MySQL::connect_unix(const String conn_name, const String p_socket_path, bool p_async) {

	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	bool is_unix = (p_con->second->conn_type == SqlConnection::ConnType::UNIX or p_con->second->conn_type == SqlConnection::ConnType::UNIXSSL); 
	ERR_FAIL_COND_V_EDMSG(is_unix, ERR_INVALID_PARAMETER, "It's not a UNIX Connection!");

	auto conn = p_con->second;
	bool is_ssl = (conn->conn_type == SqlConnection::ConnType::UNIXSSL);
	conn->async = p_async;
	const char * socket = p_socket_path.utf8().get_data();

	try{
		if (not p_async) {
			local::stream_protocol::endpoint endpoints(socket);
			if (is_ssl) {
			    conn->unix_ssl_conn->connect(endpoints, conn->conn_params);
			}
			else {
			    conn->unix_conn->connect(endpoints, conn->conn_params);
			}
			return OK;
		}
		else {  //async	
			conn->conn_params.set_multi_queries(false);
			boost::asio::co_spawn(
				conn->ctx->get_executor(),
				[&] {
					return coro_connect_unix(conn, socket);
				},
				[](std::exception_ptr ptr) {
					if (ptr) {
						std::rethrow_exception(ptr);
					}
				}
			);
			conn->ctx->run();
			return OK;
		} // else (async)
	
	}
	catch (const mysql::error_with_diagnostics& diags) {
		HANDLE_SQL_EXCEPTION(diags, &last_sql_exception, FAILED);
	}
	catch (const mysql::error_code& ec) {
		HANDLE_BOOST_EXCEPTION(ec, &last_boost_exception, FAILED);
	}
	catch (const std::exception& err) {
		HANDLE_STD_EXCEPTION(err, &last_std_exception, FAILED);
	}

	return OK;
}


Error MySQL::connect_tcp(const String conn_name, const String p_hostname, const String p_port, bool p_async) {

	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	bool is_tcp = (p_con->second->conn_type == SqlConnection::ConnType::TCP or p_con->second->conn_type == SqlConnection::ConnType::TCPSSL); 
	ERR_FAIL_COND_V_EDMSG(not is_tcp, ERR_INVALID_PARAMETER, "It's not a TCP Connection!");

	auto conn = p_con->second;
	bool is_ssl = (conn->conn_type == SqlConnection::ConnType::TCPSSL);
	conn->async = p_async;
	const char * hostname = p_hostname.utf8().get_data();
	const char * port = p_port.utf8().get_data();

	try{
		
		if (not p_async) {
			auto endpoints = ip::tcp::resolver(conn->ctx->get_executor()).resolve(hostname, port);
			if (is_ssl) {
				conn->tcp_ssl_conn->connect(*endpoints.begin(), conn->conn_params);
			}else {
				conn->tcp_conn->connect(*endpoints.begin(), conn->conn_params);
			}
			return OK;
		}
		else {  //async	
			conn->conn_params.set_multi_queries(false);
			boost::asio::co_spawn(
				conn->ctx->get_executor(),
				[&] {
					return coro_connect_tcp(conn, hostname, port);
				},
				[](std::exception_ptr ptr) {
					if (ptr) {
						std::rethrow_exception(ptr);
					}
				}
			);
			conn->ctx->run();
			return OK;
		} // else (async)

	}
	catch (const mysql::error_with_diagnostics& diags) {
		HANDLE_SQL_EXCEPTION(diags, &last_sql_exception, FAILED);
	}
	catch (const mysql::error_code& ec) {
		HANDLE_BOOST_EXCEPTION(ec, &last_boost_exception, FAILED);
	}
	catch (const std::exception& err) {
		HANDLE_STD_EXCEPTION(err, &last_std_exception, FAILED);
	}

	return OK;
}


Error MySQL::close_connection(const String conn_name) {
	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
		
	auto conn = p_con->second;
	SqlConnection::ConnType type = conn->conn_type;

	try{
	
		if (type == SqlConnection::ConnType::TCP) {
			conn->tcp_conn->close();
		}
		else if (type == SqlConnection::ConnType::TCPSSL) {
			conn->tcp_ssl_conn->close();
		}
		else if (type == SqlConnection::ConnType::UNIX) {
			conn->unix_conn->close();
		}

		else if (type == SqlConnection::ConnType::UNIXSSL) {
			conn->unix_ssl_conn->close();
		}

	}
	catch (const mysql::error_with_diagnostics& diags) {
		HANDLE_SQL_EXCEPTION(diags, &last_sql_exception, FAILED);
	}
	catch (const mysql::error_code& ec) {
		HANDLE_BOOST_EXCEPTION(ec, &last_boost_exception, FAILED);
	}
	catch (const std::exception& err) {
		HANDLE_STD_EXCEPTION(err, &last_std_exception, FAILED);
	}

	connectionMap.erase(conn_name);
	p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con != connectionMap.end(), FAILED, "Could not delete the connections!");
	return OK;
}


Error MySQL::new_connection(const String conn_name, SqlConnection::ConnType type) {
	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con != connectionMap.end(), ERR_ALREADY_EXISTS, "Connection already exists!");
	connectionMap[conn_name] = std::make_shared<SqlConnection>();
	p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), ERR_CANT_CREATE, "Failed to create the connection!");
	p_con->second->conn_type = type;
	p_con->second->ID = conn_name;
	return OK;
}


Error MySQL::delete_connection(const String conn_name) {
	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	close_connection(conn_name);
	connectionMap.erase(conn_name);
	return OK;
}


Error MySQL::set_credentials(
		const String conn_name,
		const String p_username,
		const String p_password,
		const String p_database,
		const SqlConnection::MysqlCollations collation,
		const SqlConnection::ssl_mode p_ssl,
		const bool multi_queries
) {

	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con==connectionMap.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	auto c = p_con->second;

	c->username = copy_string(const_cast<char*>(p_username.utf8().get_data()));
	c->password = copy_string(const_cast<char*>(p_password.utf8().get_data()));
	c->database = copy_string(const_cast<char*>(p_database.utf8().get_data()));

	// It can be removed
	c->conn_params.set_username(c->username);
	c->conn_params.set_password(c->password);
	c->conn_params.set_database(c->database);

	c->conn_params.set_connection_collation(collation);
	c->conn_params.set_ssl((mysql::ssl_mode)p_ssl);
	c->conn_params.set_multi_queries(multi_queries);

	return OK;
}


Error MySQL::set_certificate(const String conn_name, const String p_cert_file, const String p_host_name) {

	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con==connectionMap.end(), ERR_DOES_NOT_EXIST, "Connection does not exist!");
	bool is_ssl = (p_con->second->conn_type == SqlConnection::ConnType::TCPSSL or p_con->second->conn_type == SqlConnection::ConnType::UNIXSSL);
	ERR_FAIL_COND_V_EDMSG(is_ssl, ERR_INVALID_PARAMETER, "Connection type is not TCPSSL or UNIXSSL!");

	const std::string& cert_path = p_cert_file.utf8().get_data();
	const std::string& host = p_host_name.utf8().get_data();
	auto c = p_con->second->ssl_ctx;

	try{
		c->set_verify_mode(asio::ssl::verify_peer);
		c->set_verify_callback(ssl::host_name_verification(host));
		c->load_verify_file(cert_path);
	}
	catch (const mysql::error_with_diagnostics& diags) {
		HANDLE_SQL_EXCEPTION(diags, &last_sql_exception, FAILED);
	}
	catch (const mysql::error_code& ec) {
		HANDLE_BOOST_EXCEPTION(ec, &last_boost_exception, FAILED);
	}
	catch (const std::exception& err) {
		HANDLE_STD_EXCEPTION(err, &last_std_exception, FAILED);
	}

	return OK;
}


Dictionary MySQL::get_credentials(const String conn_name) const {
	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), Dictionary(), "Connection does not exist!");
	auto c = p_con->second->conn_params;
	Dictionary ret;
	ret["username"] = String(c.username().data());
	ret["password"] = String(c.password().data());
	ret["database"] = String(c.database().data());
	ret["connection_collation"] = (int)c.connection_collation();
	ret["ssl"] = (SqlConnection::ssl_mode)c.ssl();
	ret["multi_queries"] = c.multi_queries();
	return ret;
}


SqlConnection::ConnType MySQL::get_connection_type(const String conn_name) const {
	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), SqlConnection::ConnType::NONE, "Connection does not exist!");
	return p_con->second->conn_type;
}


bool MySQL::is_async(const String conn_name) const {
	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), false, "Connection does not exist!");
	return p_con->second->async;
}


PackedStringArray MySQL::get_connections() const{
	PackedStringArray ret;
	for (auto p_con : connectionMap) {
		ret.push_back(p_con.first);
	}
	return ret;
}


Ref<SqlResult> MySQL::make_godot_result(mysql::results result) {


	Dictionary meta;

	for(auto m:result.meta()) {
		Dictionary column;
		String column_name = String(m.column_name().data());

		column["column_collation"]			= m.column_collation();
		column["column_length"]				= m.column_length();
		column["column_name"]				= column_name;
		column["database"]					= String(m.database().data());
		column["decimals"]					= m.decimals();
		column["has_no_default_value"]		= m.has_no_default_value();
		column["is_auto_increment"]			= m.is_auto_increment();
		column["is_multiple_key"]			= m.is_multiple_key();
		column["is_not_null"]				= m.is_not_null();
		column["is_primary_key"]			= m.is_primary_key();
		column["is_set_to_now_on_update"]	= m.is_set_to_now_on_update();
		column["is_unique_key"]				= m.is_unique_key();
		column["is_unsigned"]				= m.is_unsigned();
		column["is_zerofill"]				= m.is_zerofill();
		column["original_column_name"]		= String(m.original_column_name().data());
		column["original_table"]			= String(m.original_table().data());
		column["table"]						= String(m.table().data());
		column["type"]						= (int)m.type();

		meta[column_name] = column;
	}


	Dictionary querty_result;
	mysql::rows_view all_rows = result.rows();

	for(size_t row = 0; row < all_rows.size(); row++) {
		Dictionary line = Dictionary();
		size_t f = 0;
		for (auto fv : all_rows.at(row).as_vector()) {
			String column_name = String(result.meta()[f].column_name().data());
			line[column_name] = field2Var(fv);
			f++;
		}
		querty_result[row] = line;
	}

	//Ref<SqlResult> sql_result = Ref<SqlResult>(memnew(Ref<SqlResult()>()));

	Ref<SqlResult> sql_result;
	sql_result->result = querty_result;
	sql_result->meta = meta;
	sql_result->affected_rows = result.affected_rows();
	sql_result->last_insert_id = result.last_insert_id();
	sql_result->warning_count = result.warning_count();

	return sql_result;

}



bool MySQL::is_connectioin_alive(const String conn_name) const {

	auto p_con = connectionMap.find(conn_name);
	ERR_FAIL_COND_V_EDMSG(p_con == connectionMap.end(), false, "Connection not found.");
	SqlConnection::ConnType type = p_con->second->conn_type;
	auto conn = p_con->second;
	mysql::error_code err;
    mysql::diagnostics diag;

	if (type == SqlConnection::ConnType::TCP) {
		conn->tcp_conn->ping(err, diag);
	}
	else if (type == SqlConnection::ConnType::TCPSSL) {
		conn->tcp_ssl_conn->ping(err, diag);
	}
	else if (type == SqlConnection::ConnType::UNIX) {
		conn->unix_conn->ping(err, diag);
	}
	else if (type == SqlConnection::ConnType::UNIXSSL) {
		conn->unix_ssl_conn->ping(err, diag);
	}

	if (err){
		std::cout << "Operation failed with error code: " << err << '\n';
		std::cout << "Server diagnostics: " << diag.server_message() << std::endl;
		return false;
	}

	return true;
}



void MySQL::_bind_methods() {

	ADD_SIGNAL(MethodInfo("querty_complete", PropertyInfo(Variant::OBJECT, "Ref<SqlResult>"), PropertyInfo(Variant::STRING, "ID")));

	// Manage connections
	ClassDB::bind_method(D_METHOD("get_connection_type", "connection"), &MySQL::get_connection_type);
	ClassDB::bind_method(D_METHOD("is_async", "connection"), &MySQL::is_async);
	ClassDB::bind_method(D_METHOD("new_connection", "connection", "type"), &MySQL::new_connection, DEFVAL(SqlConnection::ConnType::TCPSSL));
	ClassDB::bind_method(D_METHOD("delete_connection", "connection"), &MySQL::delete_connection);
	ClassDB::bind_method(D_METHOD("get_connections"), &MySQL::get_connections);

	// Manage settings
	ClassDB::bind_method(D_METHOD("get_credentials", "connection"), &MySQL::get_credentials);
	ClassDB::bind_method(D_METHOD("set_credentials", "connection", "username", "password", "schema", "collation", "ssl_mode", "multi_queries"),\
	&MySQL::set_credentials, DEFVAL(""), DEFVAL(SqlConnection::MysqlCollations::default_collation), DEFVAL(SqlConnection::ssl_mode::enable), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_certificate", "connection", "cert_file", "host_name"), &MySQL::set_certificate, DEFVAL("mysql"));

	// Handle connections
	ClassDB::bind_method(D_METHOD("initialize_connection", "connection"), &MySQL::initialize_connection);
	ClassDB::bind_method(D_METHOD("connect_tcp", "connection", "hostname", "port", "async"), &MySQL::connect_tcp, DEFVAL("127.0.0.1"), DEFVAL("3306"), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("connect_unix", "connection", "socket_path", "async"), &MySQL::connect_unix, DEFVAL("/tmp/mysql.sock"), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("close_connection", "connection"), &MySQL::close_connection);
	ClassDB::bind_method(D_METHOD("is_connectioin_alive", "connection"), &MySQL::is_connectioin_alive);


	// Handle database
	ClassDB::bind_method(D_METHOD("execute", "connection", "statement"), &MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "connection", "statement", "binds"), &MySQL::execute_prepared, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("async_execute", "connection", "statement"), &MySQL::execute);
	ClassDB::bind_method(D_METHOD("async_execute_prepared", "connection", "statement", "binds"), &MySQL::execute_prepared, DEFVAL(Array()));


}


MySQL::MySQL() {

}


MySQL::~MySQL() {

}


void SqlConnection::_bind_methods() {

	BIND_ENUM_CONSTANT(TCP);
	BIND_ENUM_CONSTANT(TCPSSL);
	BIND_ENUM_CONSTANT(UNIX);
	BIND_ENUM_CONSTANT(UNIXSSL);

	BIND_ENUM_CONSTANT(disable);
	BIND_ENUM_CONSTANT(enable);
	BIND_ENUM_CONSTANT(require);

	BIND_ENUM_CONSTANT(default_collation);
	BIND_ENUM_CONSTANT(big5_chinese_ci);
	BIND_ENUM_CONSTANT(latin2_czech_cs);
	BIND_ENUM_CONSTANT(dec8_swedish_ci);
	BIND_ENUM_CONSTANT(cp850_general_ci);
	BIND_ENUM_CONSTANT(latin1_german1_ci);
	BIND_ENUM_CONSTANT(hp8_english_ci);
	BIND_ENUM_CONSTANT(koi8r_general_ci);
	BIND_ENUM_CONSTANT(latin1_swedish_ci);
	BIND_ENUM_CONSTANT(latin2_general_ci);
	BIND_ENUM_CONSTANT(swe7_swedish_ci);
	BIND_ENUM_CONSTANT(ascii_general_ci);
	BIND_ENUM_CONSTANT(ujis_japanese_ci);
	BIND_ENUM_CONSTANT(sjis_japanese_ci);
	BIND_ENUM_CONSTANT(cp1251_bulgarian_ci);
	BIND_ENUM_CONSTANT(latin1_danish_ci);
	BIND_ENUM_CONSTANT(hebrew_general_ci);
	BIND_ENUM_CONSTANT(tis620_thai_ci);
	BIND_ENUM_CONSTANT(euckr_korean_ci);
	BIND_ENUM_CONSTANT(latin7_estonian_cs);
	BIND_ENUM_CONSTANT(latin2_hungarian_ci);
	BIND_ENUM_CONSTANT(koi8u_general_ci);
	BIND_ENUM_CONSTANT(cp1251_ukrainian_ci);
	BIND_ENUM_CONSTANT(gb2312_chinese_ci);
	BIND_ENUM_CONSTANT(greek_general_ci);
	BIND_ENUM_CONSTANT(cp1250_general_ci);
	BIND_ENUM_CONSTANT(latin2_croatian_ci);
	BIND_ENUM_CONSTANT(gbk_chinese_ci);
	BIND_ENUM_CONSTANT(cp1257_lithuanian_ci);
	BIND_ENUM_CONSTANT(latin5_turkish_ci);
	BIND_ENUM_CONSTANT(latin1_german2_ci);
	BIND_ENUM_CONSTANT(armscii8_general_ci);
	BIND_ENUM_CONSTANT(utf8_general_ci);
	BIND_ENUM_CONSTANT(cp1250_czech_cs);
	BIND_ENUM_CONSTANT(ucs2_general_ci);
	BIND_ENUM_CONSTANT(cp866_general_ci);
	BIND_ENUM_CONSTANT(keybcs2_general_ci);
	BIND_ENUM_CONSTANT(macce_general_ci);
	BIND_ENUM_CONSTANT(macroman_general_ci);
	BIND_ENUM_CONSTANT(cp852_general_ci);
	BIND_ENUM_CONSTANT(latin7_general_ci);
	BIND_ENUM_CONSTANT(latin7_general_cs);
	BIND_ENUM_CONSTANT(macce_bin);
	BIND_ENUM_CONSTANT(cp1250_croatian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_general_ci);
	BIND_ENUM_CONSTANT(utf8mb4_bin);
	BIND_ENUM_CONSTANT(latin1_bin);
	BIND_ENUM_CONSTANT(latin1_general_ci);
	BIND_ENUM_CONSTANT(latin1_general_cs);
	BIND_ENUM_CONSTANT(cp1251_bin);
	BIND_ENUM_CONSTANT(cp1251_general_ci);
	BIND_ENUM_CONSTANT(cp1251_general_cs);
	BIND_ENUM_CONSTANT(macroman_bin);
	BIND_ENUM_CONSTANT(utf16_general_ci);
	BIND_ENUM_CONSTANT(utf16_bin);
	BIND_ENUM_CONSTANT(utf16le_general_ci);
	BIND_ENUM_CONSTANT(cp1256_general_ci);
	BIND_ENUM_CONSTANT(cp1257_bin);
	BIND_ENUM_CONSTANT(cp1257_general_ci);
	BIND_ENUM_CONSTANT(utf32_general_ci);
	BIND_ENUM_CONSTANT(utf32_bin);
	BIND_ENUM_CONSTANT(utf16le_bin);
	BIND_ENUM_CONSTANT(binary);
	BIND_ENUM_CONSTANT(armscii8_bin);
	BIND_ENUM_CONSTANT(ascii_bin);
	BIND_ENUM_CONSTANT(cp1250_bin);
	BIND_ENUM_CONSTANT(cp1256_bin);
	BIND_ENUM_CONSTANT(cp866_bin);
	BIND_ENUM_CONSTANT(dec8_bin);
	BIND_ENUM_CONSTANT(greek_bin);
	BIND_ENUM_CONSTANT(hebrew_bin);
	BIND_ENUM_CONSTANT(hp8_bin);
	BIND_ENUM_CONSTANT(keybcs2_bin);
	BIND_ENUM_CONSTANT(koi8r_bin);
	BIND_ENUM_CONSTANT(koi8u_bin);
	BIND_ENUM_CONSTANT(utf8_tolower_ci);
	BIND_ENUM_CONSTANT(latin2_bin);
	BIND_ENUM_CONSTANT(latin5_bin);
	BIND_ENUM_CONSTANT(latin7_bin);
	BIND_ENUM_CONSTANT(cp850_bin);
	BIND_ENUM_CONSTANT(cp852_bin);
	BIND_ENUM_CONSTANT(swe7_bin);
	BIND_ENUM_CONSTANT(utf8_bin);
	BIND_ENUM_CONSTANT(big5_bin);
	BIND_ENUM_CONSTANT(euckr_bin);
	BIND_ENUM_CONSTANT(gb2312_bin);
	BIND_ENUM_CONSTANT(gbk_bin);
	BIND_ENUM_CONSTANT(sjis_bin);
	BIND_ENUM_CONSTANT(tis620_bin);
	BIND_ENUM_CONSTANT(ucs2_bin);
	BIND_ENUM_CONSTANT(ujis_bin);
	BIND_ENUM_CONSTANT(geostd8_general_ci);
	BIND_ENUM_CONSTANT(geostd8_bin);
	BIND_ENUM_CONSTANT(latin1_spanish_ci);
	BIND_ENUM_CONSTANT(cp932_japanese_ci);
	BIND_ENUM_CONSTANT(cp932_bin);
	BIND_ENUM_CONSTANT(eucjpms_japanese_ci);
	BIND_ENUM_CONSTANT(eucjpms_bin);
	BIND_ENUM_CONSTANT(cp1250_polish_ci);
	BIND_ENUM_CONSTANT(utf16_unicode_ci);
	BIND_ENUM_CONSTANT(utf16_icelandic_ci);
	BIND_ENUM_CONSTANT(utf16_latvian_ci);
	BIND_ENUM_CONSTANT(utf16_romanian_ci);
	BIND_ENUM_CONSTANT(utf16_slovenian_ci);
	BIND_ENUM_CONSTANT(utf16_polish_ci);
	BIND_ENUM_CONSTANT(utf16_estonian_ci);
	BIND_ENUM_CONSTANT(utf16_spanish_ci);
	BIND_ENUM_CONSTANT(utf16_swedish_ci);
	BIND_ENUM_CONSTANT(utf16_turkish_ci);
	BIND_ENUM_CONSTANT(utf16_czech_ci);
	BIND_ENUM_CONSTANT(utf16_danish_ci);
	BIND_ENUM_CONSTANT(utf16_lithuanian_ci);
	BIND_ENUM_CONSTANT(utf16_slovak_ci);
	BIND_ENUM_CONSTANT(utf16_spanish2_ci);
	BIND_ENUM_CONSTANT(utf16_roman_ci);
	BIND_ENUM_CONSTANT(utf16_persian_ci);
	BIND_ENUM_CONSTANT(utf16_esperanto_ci);
	BIND_ENUM_CONSTANT(utf16_hungarian_ci);
	BIND_ENUM_CONSTANT(utf16_sinhala_ci);
	BIND_ENUM_CONSTANT(utf16_german2_ci);
	BIND_ENUM_CONSTANT(utf16_croatian_ci);
	BIND_ENUM_CONSTANT(utf16_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf16_vietnamese_ci);
	BIND_ENUM_CONSTANT(ucs2_unicode_ci);
	BIND_ENUM_CONSTANT(ucs2_icelandic_ci);
	BIND_ENUM_CONSTANT(ucs2_latvian_ci);
	BIND_ENUM_CONSTANT(ucs2_romanian_ci);
	BIND_ENUM_CONSTANT(ucs2_slovenian_ci);
	BIND_ENUM_CONSTANT(ucs2_polish_ci);
	BIND_ENUM_CONSTANT(ucs2_estonian_ci);
	BIND_ENUM_CONSTANT(ucs2_spanish_ci);
	BIND_ENUM_CONSTANT(ucs2_swedish_ci);
	BIND_ENUM_CONSTANT(ucs2_turkish_ci);
	BIND_ENUM_CONSTANT(ucs2_czech_ci);
	BIND_ENUM_CONSTANT(ucs2_danish_ci);
	BIND_ENUM_CONSTANT(ucs2_lithuanian_ci);
	BIND_ENUM_CONSTANT(ucs2_slovak_ci);
	BIND_ENUM_CONSTANT(ucs2_spanish2_ci);
	BIND_ENUM_CONSTANT(ucs2_roman_ci);
	BIND_ENUM_CONSTANT(ucs2_persian_ci);
	BIND_ENUM_CONSTANT(ucs2_esperanto_ci);
	BIND_ENUM_CONSTANT(ucs2_hungarian_ci);
	BIND_ENUM_CONSTANT(ucs2_sinhala_ci);
	BIND_ENUM_CONSTANT(ucs2_german2_ci);
	BIND_ENUM_CONSTANT(ucs2_croatian_ci);
	BIND_ENUM_CONSTANT(ucs2_unicode_520_ci);
	BIND_ENUM_CONSTANT(ucs2_vietnamese_ci);
	BIND_ENUM_CONSTANT(ucs2_general_mysql500_ci);
	BIND_ENUM_CONSTANT(utf32_unicode_ci);
	BIND_ENUM_CONSTANT(utf32_icelandic_ci);
	BIND_ENUM_CONSTANT(utf32_latvian_ci);
	BIND_ENUM_CONSTANT(utf32_romanian_ci);
	BIND_ENUM_CONSTANT(utf32_slovenian_ci);
	BIND_ENUM_CONSTANT(utf32_polish_ci);
	BIND_ENUM_CONSTANT(utf32_estonian_ci);
	BIND_ENUM_CONSTANT(utf32_spanish_ci);
	BIND_ENUM_CONSTANT(utf32_swedish_ci);
	BIND_ENUM_CONSTANT(utf32_turkish_ci);
	BIND_ENUM_CONSTANT(utf32_czech_ci);
	BIND_ENUM_CONSTANT(utf32_danish_ci);
	BIND_ENUM_CONSTANT(utf32_lithuanian_ci);
	BIND_ENUM_CONSTANT(utf32_slovak_ci);
	BIND_ENUM_CONSTANT(utf32_spanish2_ci);
	BIND_ENUM_CONSTANT(utf32_roman_ci);
	BIND_ENUM_CONSTANT(utf32_persian_ci);
	BIND_ENUM_CONSTANT(utf32_esperanto_ci);
	BIND_ENUM_CONSTANT(utf32_hungarian_ci);
	BIND_ENUM_CONSTANT(utf32_sinhala_ci);
	BIND_ENUM_CONSTANT(utf32_german2_ci);
	BIND_ENUM_CONSTANT(utf32_croatian_ci);
	BIND_ENUM_CONSTANT(utf32_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf32_vietnamese_ci);
	BIND_ENUM_CONSTANT(utf8_unicode_ci);
	BIND_ENUM_CONSTANT(utf8_icelandic_ci);
	BIND_ENUM_CONSTANT(utf8_latvian_ci);
	BIND_ENUM_CONSTANT(utf8_romanian_ci);
	BIND_ENUM_CONSTANT(utf8_slovenian_ci);
	BIND_ENUM_CONSTANT(utf8_polish_ci);
	BIND_ENUM_CONSTANT(utf8_estonian_ci);
	BIND_ENUM_CONSTANT(utf8_spanish_ci);
	BIND_ENUM_CONSTANT(utf8_swedish_ci);
	BIND_ENUM_CONSTANT(utf8_turkish_ci);
	BIND_ENUM_CONSTANT(utf8_czech_ci);
	BIND_ENUM_CONSTANT(utf8_danish_ci);
	BIND_ENUM_CONSTANT(utf8_lithuanian_ci);
	BIND_ENUM_CONSTANT(utf8_slovak_ci);
	BIND_ENUM_CONSTANT(utf8_spanish2_ci);
	BIND_ENUM_CONSTANT(utf8_roman_ci);
	BIND_ENUM_CONSTANT(utf8_persian_ci);
	BIND_ENUM_CONSTANT(utf8_esperanto_ci);
	BIND_ENUM_CONSTANT(utf8_hungarian_ci);
	BIND_ENUM_CONSTANT(utf8_sinhala_ci);
	BIND_ENUM_CONSTANT(utf8_german2_ci);
	BIND_ENUM_CONSTANT(utf8_croatian_ci);
	BIND_ENUM_CONSTANT(utf8_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf8_vietnamese_ci);
	BIND_ENUM_CONSTANT(utf8_general_mysql500_ci);
	BIND_ENUM_CONSTANT(utf8mb4_unicode_ci);
	BIND_ENUM_CONSTANT(utf8mb4_icelandic_ci);
	BIND_ENUM_CONSTANT(utf8mb4_latvian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_romanian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_slovenian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_polish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_estonian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_spanish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_swedish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_turkish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_czech_ci);
	BIND_ENUM_CONSTANT(utf8mb4_danish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_lithuanian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_slovak_ci);
	BIND_ENUM_CONSTANT(utf8mb4_spanish2_ci);
	BIND_ENUM_CONSTANT(utf8mb4_roman_ci);
	BIND_ENUM_CONSTANT(utf8mb4_persian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_esperanto_ci);
	BIND_ENUM_CONSTANT(utf8mb4_hungarian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_sinhala_ci);
	BIND_ENUM_CONSTANT(utf8mb4_german2_ci);
	BIND_ENUM_CONSTANT(utf8mb4_croatian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf8mb4_vietnamese_ci);
	BIND_ENUM_CONSTANT(gb18030_chinese_ci);
	BIND_ENUM_CONSTANT(gb18030_bin);
	BIND_ENUM_CONSTANT(gb18030_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf8mb4_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_de_pb_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_is_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_lv_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_ro_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_sl_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_pl_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_et_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_es_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_sv_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_tr_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_cs_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_da_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_lt_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_sk_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_es_trad_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_la_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_eo_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_hu_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_hr_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_vi_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_de_pb_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_is_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_lv_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_ro_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_sl_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_pl_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_et_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_es_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_sv_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_tr_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_cs_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_da_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_lt_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_sk_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_es_trad_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_la_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_eo_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_hu_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_hr_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_vi_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_ja_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_ja_0900_as_cs_ks);
	BIND_ENUM_CONSTANT(utf8mb4_0900_as_ci);
	BIND_ENUM_CONSTANT(utf8mb4_ru_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_ru_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_zh_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_0900_bin);

}








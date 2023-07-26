/* sql_conn.cpp */

#include "sql_conn.h"
#include "core/io/resource_loader.h"




Error ConnTcp::connect(const String p_socket_path, const bool p_async){
	ERR_FAIL_V_EDMSG(FAILED, "The tcp_connect function was designed to work with TCP connections only. For UNIX connection use unix_connect.");
}


Error ConnTcpTls::connect(const String p_socket_path, const bool p_async){
	ERR_FAIL_V_EDMSG(FAILED, "The tcp_connect function was designed to work with TCP connections only. For UNIX connection use unix_connect.");
}


Error ConnUnix::connect(const String p_hostname,const String p_port, const bool p_async){
	ERR_FAIL_V_EDMSG(FAILED, "The unix_connect function was designed to work with UNIX connections only. For TCP connection use tcp_connect.");
}


Error ConnUnixTls::connect(const String p_hostname,const String p_port, const bool p_async){
	ERR_FAIL_V_EDMSG(FAILED, "The unix_connect function was designed to work with UNIX connections only. For TCP connection use tcp_connect.");
}


Error ConnTcp::connect(const String p_hostname,const String p_port, const bool p_async){

	// Async cannot use multi_queries.
	if (p_async){
		conn_params.set_multi_queries(false);
	}

	async = p_async;
	diagnostics diag;
	mysql::error_code ec;
	const char * hostname = p_hostname.utf8().get_data();
	const char * port = p_port.utf8().get_data();
	conn.set_meta_mode(metadata_mode::full);

	if (p_async){
		// Resolve endpoint
        resolver.async_resolve(hostname, port, 
			[&](mysql::error_code err, tcp::resolver::results_type results) {
				mysql::throw_on_error(ec);
				if (ec){
					return;
				}
				endpoints = std::move(results);
				// Connect
				conn.async_connect(*endpoints.begin(), conn_params, diag, 
				[&](mysql::error_code err2) {
					mysql::throw_on_error(ec, diag);
				});
            }
		);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}

	else{
  		endpoints = resolver.resolve(hostname, port, ec);
		conn.connect(*endpoints.begin(), conn_params, ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
}



Error ConnTcpTls::connect(const String p_hostname,const String p_port, const bool p_async){

	// Async cannot use multi_queries.
	if (p_async){
		conn_params.set_multi_queries(false);
	}

	async = p_async;
	diagnostics diag;
	mysql::error_code ec;
	const char * hostname = p_hostname.utf8().get_data();
	const char * port = p_port.utf8().get_data();
	conn.set_meta_mode(metadata_mode::full);

	if (async){
		// Resolve endpoint
        resolver.async_resolve(hostname, port, 
			[&](mysql::error_code err, tcp::resolver::results_type results) {
				mysql::throw_on_error(ec);
				if (ec){
					return;
				}
				endpoints = std::move(results);
				// Connect
				conn.async_connect(*endpoints.begin(), conn_params, diag, 
				[&](mysql::error_code err2) {
					mysql::throw_on_error(ec, diag);
				});
            }
		);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}

	else{
  		endpoints = resolver.resolve(hostname, port, ec);
		conn.connect(*endpoints.begin(), conn_params, ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
}



Error ConnUnix::connect(String p_socket_path, bool p_async){

	// Async cannot use multi_queries.
	if (p_async){
		conn_params.set_multi_queries(false);
	}

	async = p_async;
	diagnostics diag;
	mysql::error_code ec;
	const char * socket_path = p_socket_path.utf8().get_data();
	conn.set_meta_mode(metadata_mode::full);

	if (async){
		local::stream_protocol::endpoint endpoints(socket_path);
		conn.async_connect(endpoints, conn_params, diag, 
			[&](mysql::error_code err) {
				mysql::throw_on_error(ec, diag);
			});
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}

	else{
  		local::stream_protocol::endpoint endpoints(socket_path);
		conn.connect(endpoints, conn_params, ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}

}



Error ConnUnixTls::connect(String p_socket_path, bool p_async){

	// Async cannot use multi_queries.
	if (p_async){
		conn_params.set_multi_queries(false);
	}

	async = p_async;
	diagnostics diag;
	mysql::error_code ec;
	const char * socket_path = p_socket_path.utf8().get_data();
	conn.set_meta_mode(metadata_mode::full);

	if (async){
		local::stream_protocol::endpoint endpoints(socket_path);
		conn.async_connect(endpoints, conn_params, diag, 
			[&](mysql::error_code err) {
				mysql::throw_on_error(ec, diag);
			});
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}

	else{
  		local::stream_protocol::endpoint endpoints(socket_path);
		conn.connect(endpoints, conn_params, ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}

}



Error ConnTcp::close_connection(){
	mysql::error_code ec;
	diagnostics diag;
	if (async){
		conn.async_close(diag, [&](mysql::error_code err) { mysql::throw_on_error(ec, diag); });
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
	else{
		conn.close(ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
}


Error ConnTcpTls::close_connection(){
	mysql::error_code ec;
	diagnostics diag;
	if (async){
		conn.async_close(diag, [&](mysql::error_code err) { mysql::throw_on_error(ec, diag); });
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
	else{
		conn.close(ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
}

Error ConnUnix::close_connection(){
	mysql::error_code ec;
	diagnostics diag;
	if (async){
		conn.async_close(diag, [&](mysql::error_code err) { mysql::throw_on_error(ec, diag); });
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
	else{
		conn.close(ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
}


Error ConnUnixTls::close_connection(){
	mysql::error_code ec;
	diagnostics diag;
	if (async){
		conn.async_close(diag, [&](mysql::error_code err) { mysql::throw_on_error(ec, diag); });
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
	else{
		conn.close(ec, diag);
		SQL_EXCEPTION_ERR(ec, diag);
		return OK;
	}
}


Error ConnTcp::set_cert(const String cert_path, const String p_common_name){
	ERR_FAIL_V_EDMSG(ERR_INVALID_PARAMETER, "You can only configure a SSL certificate on SSL connections..");
}

Error ConnUnix::set_cert(const String cert_path, const String p_common_name){
	ERR_FAIL_V_EDMSG(ERR_INVALID_PARAMETER, "You can only configure a SSL certificate on SSL connections..");
}

Error ConnTcpTls::set_cert(const String cert_path, const String p_common_name){

	Ref<FileAccess> f = FileAccess::open(cert_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_INVALID_PARAMETER, "Cannot open certificate file on path: '" + cert_path + "'.");

	String m_cert = f->get_as_text(false);
	const char * c_data = m_cert.utf8().get_data();
	std::vector<char> cert(c_data, c_data + m_cert.length());
	const std::string& host = p_common_name.utf8().get_data();

	ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
	ssl_ctx.add_certificate_authority(boost::asio::buffer(cert));
	ssl_ctx.set_verify_callback(ssl::host_name_verification(host));
	conn = boost::mysql::tcp_ssl_connection(ctx.get_executor(), ssl_ctx);

	return OK;
}

Error ConnUnixTls::set_cert(const String cert_path, const String p_common_name){

	Ref<FileAccess> f = FileAccess::open(cert_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_INVALID_PARAMETER, "Cannot open certificate file on path: '" + cert_path + "'.");

	String m_cert = f->get_as_text(false);
	const char * c_data = m_cert.utf8().get_data();
	std::vector<char> cert(c_data, c_data + m_cert.length());
	const std::string& host = p_common_name.utf8().get_data();

	ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
	ssl_ctx.add_certificate_authority(boost::asio::buffer(cert));
	ssl_ctx.set_verify_callback(ssl::host_name_verification(host));
	conn = boost::mysql::unix_ssl_connection(ctx.get_executor(), ssl_ctx);

	return OK;
}
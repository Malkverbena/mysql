/* sql_conn.cpp */

#include "sql_conn.h"



Error ConnTcp::connect(const String p_socket_path, const bool p_async){
	ERR_FAIL_V_EDMSG(FAILED, "The tcp_connect function was designed to work with TCP connections only. For UNIX connection use unix_connect.");
}

Error ConnTcp::connect(const String p_hostname,const String p_port, const bool p_async){

	// Async cannot use multi_queries.
	if (p_async){
		conn_params.set_multi_queries(false);
	}

	mysql::error_code ec;
	diagnostics diag;
	conn.set_meta_mode(metadata_mode::full);
	endpoints = resolver.resolve(p_hostname.utf8().get_data(), p_port.utf8().get_data());
	conn.connect(*endpoints.begin(), conn_params, ec, diag);
	SQL_EXCEPTION_ERR(ec, diag);
	return OK;
}


Error ConnTcpSsl::connect(const String p_socket_path, const bool p_async){
	ERR_FAIL_V_EDMSG(FAILED, "The tcp_connect function was designed to work with TCP connections only. For UNIX connection use unix_connect.");
}

Error ConnTcpSsl::connect(const String p_hostname,const String p_port, const bool p_async){

	// Async cannot use multi_queries.
	if (p_async){
		conn_params.set_multi_queries(false);
	}

	mysql::error_code ec;
	diagnostics diag;
	conn.set_meta_mode(metadata_mode::full);
	endpoints = resolver.resolve(p_hostname.utf8().get_data(), p_port.utf8().get_data());
	conn.connect(*endpoints.begin(), conn_params, ec, diag);	
	SQL_EXCEPTION_ERR(ec, diag);
	return OK;
}

Error ConnUnix::connect(const String p_hostname,const String p_port, const bool p_async){
	ERR_FAIL_V_EDMSG(FAILED, "The unix_connect function was designed to work with UNIX connections only. For TCP connection use tcp_connect.");
}

Error ConnUnix::connect(String p_socket_path, bool p_async){

	// Async cannot use multi_queries.
	if (p_async){
		conn_params.set_multi_queries(false);
	}

	mysql::error_code ec;
	diagnostics diag;
	socket_path = copy_string(const_cast<char*>(p_socket_path.utf8().get_data()));
	endpoints = local::stream_protocol::endpoint(socket_path);
	conn.connect(endpoints, conn_params, ec, diag);
	SQL_EXCEPTION_ERR(ec, diag);
	return OK;
}


Error ConnUnixSsl::connect(const String p_hostname,const String p_port, const bool p_async){
	ERR_FAIL_V_EDMSG(FAILED, "The unix_connect function was designed to work with UNIX connections only. For TCP connection use tcp_connect.");
}
Error ConnUnixSsl::connect(String p_socket_path, bool p_async){

	// Async cannot use multi_queries.
	if (p_async){
		conn_params.set_multi_queries(false);
	}

	mysql::error_code ec;
	diagnostics diag;
	socket_path = copy_string(const_cast<char*>(p_socket_path.utf8().get_data()));
	endpoints = local::stream_protocol::endpoint(socket_path);
	conn.connect(endpoints, conn_params, ec, diag);
	SQL_EXCEPTION_ERR(ec, diag);
	return OK;
}
















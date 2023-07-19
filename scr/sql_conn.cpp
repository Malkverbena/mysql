/* sql_conn.cpp */

#include "sql_conn.h"




Error ConnTcp::connect(String hostname, String port, bool async){

	// Async cannot use multi_queries.
	if (async){conn_params.set_multi_queries(false);}

	try{
		conn.set_meta_mode(metadata_mode::full);
		endpoints = resolver.resolve(hostname.utf8().get_data(), port.utf8().get_data());
		conn.connect(*endpoints.begin(), conn_params);	
	}
	catch (const error_with_diagnostics& err){
		SQLException(err);
		return FAILED;
	}
	catch (const std::exception& err){
		_runtime_error(err);
		return FAILED;
	}
	return OK;
}


Error ConnTcpSsl::connect(String hostname, String port, bool async){

	// Async cannot use multi_queries.
	if (async){conn_params.set_multi_queries(false);}

	try{
		conn.set_meta_mode(metadata_mode::full);
		endpoints = resolver.resolve(hostname.utf8().get_data(), port.utf8().get_data());
		conn.connect(*endpoints.begin(), conn_params);	
	}
	catch (const error_with_diagnostics& err){
		SQLException(err);
		return FAILED;
	}
	catch (const std::exception& err){
		_runtime_error(err);
		return FAILED;
	}
	return OK;
}


Error ConnUnix::connect(String p_socket_path, bool async){

	// Async cannot use multi_queries.
	if (async){conn_params.set_multi_queries(false);}

	try{
		socket_path = copy_string(const_cast<char*>(p_socket_path.utf8().get_data()));
		endpoints = local::stream_protocol::endpoint(socket_path);
		conn.connect(endpoints, conn_params); 
	}
	catch (const error_with_diagnostics& err){
		SQLException(err);
		return FAILED;
	}
	catch (const std::exception& err){
		_runtime_error(err);
		return FAILED;
	}
	return OK;
	}


Error ConnUnixSsl::connect(String p_socket_path, bool async){

	// Async cannot use multi_queries.
	if (async){conn_params.set_multi_queries(false);}

	try{
		socket_path = copy_string(const_cast<char*>(p_socket_path.utf8().get_data()));
		endpoints = local::stream_protocol::endpoint(socket_path);
		conn.connect(endpoints, conn_params); 
	}
	catch (const error_with_diagnostics& err){
		SQLException(err);
		return FAILED;
	}
	catch (const std::exception& err){
		_runtime_error(err);
		return FAILED;
	}
	return OK;}
















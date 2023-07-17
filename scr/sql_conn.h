/* sql_conn.h */

#ifndef 	SQL_CONN_H
#define SQL_CONN_H


#include "sql_result.h"




class Conn{
friend class MySQL;
//friend class ConnTcp;
//friend class ConnTcpSsl;
//friend class ConnUnix;
//friend class ConnUnixSsl;

protected:
	io_context ctx;
	int type;
	bool async = false;
	char *username;
	char *password;
	char *database;
};



class ConnTcp : public Conn{
friend class MySQL;

private:
//	io_service ctx;
	handshake_params conn_params;
	ip::tcp::resolver resolver;
	ip::tcp::resolver::results_type endpoints; // = resolver.resolve(host, port);
	tcp_connection conn;

public:
	ConnTcp():
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		resolver(ctx.get_executor()),
		conn(ctx)
	 {
	 	type = 0;  //FIXME
	 }
	~ConnTcp(){
		conn.close();
	}
};




class ConnTcpSsl : public Conn{
friend class MySQL;

private:
//	io_context ctx;
	ip::tcp::resolver::results_type eps;
	handshake_params conn_params;
	ip::tcp::resolver resolver;
	ssl::context ssl_ctx;
	tcp_ssl_connection conn;

public:
	ConnTcpSsl():	
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		resolver(ctx.get_executor()),
		ssl_ctx(ssl::context::tls_client),
		conn(ctx.get_executor(), ssl_ctx)
	{
	 	type = 1;  //FIXME
	}
	~ConnTcpSsl(){
		conn.close();
	}
};



/* ===== UNIX  TYPE: unix_connection ===== */
class ConnUnix : public Conn{
friend class MySQL;

private:
//	io_context ctx;
	local::stream_protocol::endpoint ep;
	handshake_params conn_params;
	unix_connection conn;

protected:
	const char* socket_path;

public:
	ConnUnix():
		ep((const char *)&socket_path),
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		conn(ctx)
	{
		type = 2;  //FIXME
	}
	~ConnUnix(){
		conn.close();
	}
};



class ConnUnixSsl : public Conn{
friend class MySQL;

private:
//	io_context ctx;
	local::stream_protocol::endpoint ep;
	handshake_params conn_params;
	ssl::context ssl_ctx;
	unix_ssl_connection conn;

protected:
	const char* socket_path;


public:
	ConnUnixSsl():
		ep(socket_path),
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		ssl_ctx(ssl::context::tls_client),
		conn(ctx, ssl_ctx)
	{
		type = 3;  //FIXME
	}
	~ConnUnixSsl(){
		conn.close();
	}
};








#endif  // SQL_CONN_H

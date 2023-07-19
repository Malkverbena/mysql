/* sql_conn.h */

#ifndef 	SQL_CONN_H
#define SQL_CONN_H


#include "sql_result.h"

#include <variant>
#include <typeinfo>



//========================================

class Conn{
friend class MySQL;
protected:
	io_context ctx;
	int type;
	bool async = false;
	char *username;
	char *password;
	char *database;
};


//========================================

class ConnTcp : public Conn{
friend class MySQL;

private:
//	io_service ctx;
	handshake_params conn_params;
	ip::tcp::resolver resolver;
	ip::tcp::resolver::results_type endpoints; // = resolver.resolve(host, port);
	tcp_connection conn;

public:
	Error connect(String p_socket_path, bool p_async){return OK;}
	Error connect(String hostname, String port, bool p_async);
	int conn_type(){return 1;}
	ConnTcp():
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		resolver(ctx.get_executor()),
		conn(ctx)
	 {
	 }
	~ConnTcp(){
		conn.close();
	}
};


//========================================

class ConnTcpSsl : public Conn{
friend class MySQL;

private:
//	io_context ctx;
	ip::tcp::resolver::results_type endpoints;
	handshake_params conn_params;
	ip::tcp::resolver resolver;
	ssl::context ssl_ctx;
	tcp_ssl_connection conn;

public:
	Error connect(String p_socket_path, bool p_async){return OK;}
	Error connect(String hostname, String port, bool p_async);
	int conn_type(){return 2;}
	ConnTcpSsl():	
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		resolver(ctx.get_executor()),
		ssl_ctx(ssl::context::tls_client),
		conn(ctx.get_executor(), ssl_ctx)
	{
	}
	~ConnTcpSsl(){
		conn.close();
	}
};


//========================================

class ConnUnix : public Conn{
friend class MySQL;

private:
//	io_context ctx;
	local::stream_protocol::endpoint endpoints;
	handshake_params conn_params;
	unix_connection conn;

protected:
	const char* socket_path;

public:
	Error connect(String p_socket_path, bool p_async);
	Error connect(String hostname, String port, bool p_async){return OK;}
	int conn_type(){return 3;}
	ConnUnix():
		endpoints((const char *)&socket_path),
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		conn(ctx)
	{
	}
	~ConnUnix(){
		conn.close();
	}
};

//========================================

class ConnUnixSsl : public Conn{
friend class MySQL;

private:
//	io_context ctx;
	local::stream_protocol::endpoint endpoints;
	handshake_params conn_params;
	ssl::context ssl_ctx;
	unix_ssl_connection conn;

protected:
	const char* socket_path;


public:
	Error connect(String p_socket_path, bool p_async);
	Error connect(String hostname, String port, bool p_async){return OK;}
	int conn_type(){return 4;}
	ConnUnixSsl():
		endpoints(socket_path),
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		ssl_ctx(ssl::context::tls_client),
		conn(ctx, ssl_ctx)
	{
	}
	~ConnUnixSsl(){
		conn.close();
	}
};


//========================================


typedef std::shared_ptr<ConnTcp>			conntcp;
typedef std::shared_ptr<ConnTcpSsl>		conntcpssl;
typedef std::shared_ptr<ConnUnix>		connunix;
typedef std::shared_ptr<ConnUnixSsl>	connunixssl;
using VariantConn = std::variant<conntcp, conntcpssl, connunix, connunixssl>;




#endif  // SQL_CONN_H

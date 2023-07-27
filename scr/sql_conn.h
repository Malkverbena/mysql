/* sql_conn.h */

#ifndef SQL_CONN_H
#define SQL_CONN_H


#include "helpers.h"
#include "sql_collations.h"


#include <variant>
#include <typeinfo>



using namespace boost::asio::ip;



//========================================

class Conn{
friend class MySQL;

protected:
	io_context ctx;
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
	int conn_type(){return SqlCollations::CONN_TYPE::TCP;}
	bool is_async(){return async;}

	Error set_cert(const String cert_path, const String p_common_name);
	Error connect(const String p_socket_path, const bool p_async);
	Error connect(const String p_hostname,const String p_port, const bool p_async);
	Error close_connection();

	ConnTcp():
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		resolver(ctx.get_executor()),
		conn(ctx)
	 {
	 }
	~ConnTcp(){
		close_connection();
	}
};


//========================================

class ConnTcpTls : public Conn{
friend class MySQL;

private:
//	io_context ctx;
	ip::tcp::resolver::results_type endpoints;
	handshake_params conn_params;
	ip::tcp::resolver resolver;
	ssl::context ssl_ctx;
	tcp_ssl_connection conn;

public:
	int conn_type(){return SqlCollations::CONN_TYPE::TCP_TLS;}
	bool is_async(){return async;}

	Error set_cert(const String cert_path, const String p_common_name);
	Error connect(const String p_socket_path, const bool p_async);
	Error connect(const String p_hostname,const String p_port, const bool p_async);
	Error close_connection();

	ConnTcpTls():	
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		resolver(ctx.get_executor()),
		ssl_ctx(ssl::context::tls_client),
		conn(ctx.get_executor(), ssl_ctx)
	{
	}
	~ConnTcpTls(){
		close_connection();
	}
};


//========================================

class ConnUnix : public Conn{
friend class MySQL;

private:
//	io_context ctx;
//	local::stream_protocol::endpoint endpoints;
	handshake_params conn_params;
	unix_connection conn;

//protected:
public:
	int conn_type(){return SqlCollations::CONN_TYPE::UNIX;}
	bool is_async(){return async;}

	Error set_cert(const String cert_path, const String p_common_name);
	Error connect(const String p_socket_path, const bool p_async);
	Error connect(const String p_hostname,const String p_port, const bool p_async);
	Error close_connection();

	ConnUnix():
		//endpoints(""),
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		conn(ctx)
	{
	}
	~ConnUnix(){
		close_connection();
	}
};

//========================================
/*
Note that there is no need to use TLS when using UNIX sockets. 
As the traffic doesn't leave the machine, MySQL considers 
them secure, and will allow using authentication plugins like 
caching_sha2_password even if TLS is not used.
*/
class ConnUnixTls : public Conn{
friend class MySQL;

private:
//	io_context ctx;
//	local::stream_protocol::endpoint endpoints;
	handshake_params conn_params;
	ssl::context ssl_ctx;
	unix_ssl_connection conn;

//protected:
public:
	int conn_type(){return SqlCollations::CONN_TYPE::UNIX_TLS;}
	bool is_async(){return async;}

	Error set_cert(const String cert_path, const String p_common_name);
	Error connect(const String p_socket_path, const bool p_async);
	Error connect(const String p_hostname,const String p_port, const bool p_async);
	Error close_connection();

	ConnUnixTls():
		//endpoints(""),
		conn_params((const char *)&username, (const char *)&password, (const char *)&database),
		ssl_ctx(ssl::context::tls_client),
		conn(ctx, ssl_ctx)
	{
	}
	~ConnUnixTls(){
		close_connection();
	}
};




//========================================


typedef std::shared_ptr<ConnTcp>		conntcp;
typedef std::shared_ptr<ConnTcpTls>		conntcptls;
typedef std::shared_ptr<ConnUnix>		connunix;
typedef std::shared_ptr<ConnUnixTls>	connunixtls;

using VariantConn = std::variant<conntcp, conntcptls, connunix, connunixtls>;



#endif  // SQL_CONN_H




/*
	set_ssl(String ca_pem)

	// Este contexto será usado pelo objeto de fluxo SSL subjacente. Nós podemos
	// configure aqui todas as opções relacionadas ao SSL, como verificação por pares ou CA
	// certificados. Faremos isso nas próximas linhas.
	boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tls_client);

	// Verifique se o certificado do servidor é válido e assinado por uma CA confiável.
	// Caso contrário, nossa operação de aperto de mão ou conexão falhará.
	ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);

	// Carregue uma CA confiável, usada para assinar o certificado do servidor.
	// Isso permitirá que a verificação da assinatura seja bem-sucedida em nosso exemplo.
	// Você precisará executar o servidor MySQL com os certificados de teste
	// localizado em $ BOOST_MYSQL_ROOT / tools / ssl/
	ssl_ctx.add_certificate_authority(boost::asio::buffer(CA_PEM));

	// Esperamos que o nome comum do certificado do servidor seja "mysql".
	// Caso contrário, o certificado será rejeitado e o aperto de mão ou a conexão falhará.
	ssl_ctx.set_verify_callback(boost::asio::ssl::host_name_verification("mysql"));

	// Passe em nosso contexto SSL para a conexão. Observe que nós
	// pode criar muitas conexões a partir de um único contexto. Precisamos manter o
	// contexto vivo até terminarmos de usar a conexão.
	boost::mysql::tpc_tls_connection conn(ctx, ssl_ctx);

*/
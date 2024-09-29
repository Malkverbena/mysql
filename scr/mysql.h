/* mysql.h */



#ifndef MYSQL_H
#define MYSQL_H


#include "sqlconnection.h"
#include "sqlresult.h"
#include "sqlcertificate.h"



class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);

public:

	



protected:

	/* Crie o método ”_bind_methods” de acordo com as instruções da documentação do godot.*/
	static void _bind_methods();


private:

	Dictionary last_error;

public:

	/* CLASS */

	// Regular constructor
	MySQL();

	// Overloaded constructor that defines connection type
    MySQL(const SqlConnection::ConnType p_type, const String p_host_name = "mysql");

	// Destructor
    ~MySQL();



	/* SSL-TLS & CERTIFICATE */

	/* O método "define" vai definir o tipo de conexão.Se já houver uma conexão ativa, ela será desconectada e redefinida.
	Esse método aceita três parâmetros:
	const ConnType p_type = TCP, // O tipo da conexão.
	const Godot::String p_host_name = "mysql" // O host name da conexão.
	const Godot::String p_certificate,   // Um caminho para um certificado ou um certificado no formato de Godot::String. 
	Teste se p_certificate é um caminho para um certificado ou um certificado em si. 
	Escreva rotinas diferentes para as duas situações, se "p_certificate" for um caminho para o arquivo do certificado ou se for o certificado no formato de String.
	O "p_certificate" e o "p_host_name" são destinados somente ao uso das conexões TLS. Em conexões não TLS e serão ignorados se a conexão não for TLS.
	Se o usuário desejar modificar o certificado, a conexão deverá ser reiniciada e redefinida. */
//	Godot::Error define(const ConnType _type = TCPTLS, const String p_cert_file = "", const String p_host_name = "mysql");


	/* CONNECTION */

	/*O método “set_credentials” configura os parâmetros da conexão e deve receber as seguintes  parâmetros:
	Godot::String p_username,
	Godot::String p_password,
	Godot::String p_database = Godot::String(),
	std::uint16_t collation = default_collation,
	SslMode p_ssl  = ssl_enable,
	bool multi_queries = false
//	O método set_credentials retorna um Godot::Error baseado nos erros do Godot caso falhe. Defina os erros apropriados de acordo com a falha.*/
/*	Godot::Error set_credentials(
			String p_username,
			String p_password,
			String p_database		= String(),
			std::uint16_t collation	= default_collation,
			SslMode p_ssl			= ssl_enable,
			bool multi_queries		= false
	);
*/

	/* O segundo método “set_credentials” é uma sobrecarga do primeiro. 
	Essa sobrecarga do método “set_credentials” configura os parâmetros da conexão mas recebe apenas um Godot::Dictionary contendo todos os parâmetros que a primeira
	versão do  “set_credentials” contém. Teste se as chaves e os valores do Dicionário são válidos e caso não sejam retorne um  Godot::Error.*/
//	Godot::Error set_credentials(Godot::Dictionary params);

	/*O método “connect” Inicia uma conexão TCP de forma síncrona com o servidor mysql.
	O método “connect” recebe os seguintes parâmetro:
	const String p_hostname = "127.0.0.1"
	const String p_port = "3306"*/
//	Godot::Error connect(const String p_hostname = "127.0.0.1", const String p_port = "3306");

	/* O método “connect” Inicia uma conexão UNIX de forma síncrona com o servidor mysql.
	O método “connect” é uma sobrecarga do método “connect”e recebe os seguintes parâmetro:
	const String p_socket_path = "/var/run/mysqld/mysqld.sock")*/
//    asio::awaitable<void> async_connect(const String p_socket_path =  = "/var/run/mysqld/mysqld.sock");

	/*	O método “async_connect” Inicia uma conexão TCP de forma assíncrona com o servidor mysql.
	O método “async_connect” recebe os seguintes parâmetro:
	const String p_hostname = "127.0.0.1"
	const String p_port = "3306"*/
//	asio::awaitable<void> async_connect(const String p_hostname = "127.0.0.1", const String p_port = "3306");

	/* O método “async_connect” Inicia uma conexão UNIX de forma assíncrona com o servidor mysql.
	O método “async_connect” é uma sobrecarga do método “async_connect”e recebe os seguintes parâmetro:
	const String p_socket_path = "/var/run/mysqld/mysqld.sock") */
//	Godot::Error async_connect(const String p_socket_path = "/var/run/mysqld/mysqld.sock");

	/* O método “close_connection”  encerra e fecha a conexão de maneira apropriada. */
//	void close_connection();

	/* Retorna ao status da conexão. Use um ping para checar o status da conexão*/
//	const ConnectionStatus get_status();

	/* O método “get_last_error” retorna um dicionário com o último erro ou exceção gerados pelo mysql.*/
	Dictionary get_last_error() const {return last_error.duplicate(true);};

	/* O método “execute” executa queries de texto simples. Esse método recebe um argumento do tipo Godot::String com o statement.
	Caso ocorra uma exceção ou erro, as informações referentes a esse evento devem ser colocadas na variável “last_error“ com os macros BOOST_EXCEPTION, 
	SQL_EXCEPTION, CORO_SQL_EXCEPTION, CORO_SQL_EXCEPTION_VOID conforme o necessário.
	O método “execute” retorna um classe do tipo SqlResult*/
//	Ref<SqlResult> execute(const String p_stmt);




};




#endif // MYSQL_H

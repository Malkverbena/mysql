/* mysql.h */



#ifndef MYSQL_H
#define MYSQL_H


#include "sqlconnection.h"
#include "sqlresult.h"
#include "sqlcertificate.h"




class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);

public:

	Ref<SqlCertificate> certificate;
	Ref<SqlConnection> connection; // = Ref<SqlConnection>(memnew(SqlConnection(certificate)));

	MySQL();
	~MySQL();


protected:

	static void _bind_methods();


private:

	Array multiqueries(std::string queries);

	awaitable<boost::mysql::results> coro_async_execute(cconst char* query, error_code& ec, diagnostics& diag);

	Ref<SqlResult> build_godot_result(mysql::results result);

	Ref<SqlResult> build_godot_result(
		mysql::rows_view batch,
		mysql::metadata_collection_view meta_collection,
		std::uint64_t affected_rows,
		std::uint64_t last_insert_id,
		unsigned warning_count
	);



public:



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




	Ref<SqlResult> async_execute(const String p_stmt);

//	Ref<SqlResult> async_execute_prepared(const String p_stmt, const Array binds = Array());

//	Array async_execute_sql(const String p_path_to_file);

//	Array async_execute_multi(const String p_queries);



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
	Dictionary get_last_error() const {return connection->last_sql_error.duplicate(true);};

	/* O método “execute” executa queries de texto simples. Esse método recebe um argumento do tipo Godot::String com o statement.
	Caso ocorra uma exceção ou erro, as informações referentes a esse evento devem ser colocadas na variável “last_error“ com os macros BOOST_EXCEPTION,
	SQL_EXCEPTION, CORO_SQL_EXCEPTION, CORO_SQL_EXCEPTION_VOID conforme o necessário.
	O método “execute” retorna um classe do tipo SqlResult*/
//	Ref<SqlResult> execute(const String p_stmt);




};




#endif // MYSQL_H

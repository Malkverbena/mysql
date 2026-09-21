// SPDX-License-Identifier: MIT
/* mysql_connection.h */
#pragma once

#include "mysql_config.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>

// MySQLConnection — um único socket físico (any_connection do Boost.MySQL) e sua
// máquina de estados (NONE -> CONFIGURED -> CONNECTING -> CONNECTED -> CLOSING ->
// FAILED). Não é thread-safe (nem any_connection é — ver "Thread safety" em
// any_connection.hpp): uso exclusivo de uma MySQLSession por vez. NÃO é exposta ao
// GDScript — é o recurso que MySQLSession possui sozinha (caminho simples) ou que
// MySQLPool gerencia em conjunto (caminho com pool, Fase 5).
//
// Cada MySQLConnection tem seu próprio boost::asio::io_context nesta fase: ainda não
// existe a thread de I/O dedicada da Fase 5, então connect()/close() são chamadas
// síncronas e bloqueantes (as sobrecargas de connect()/close() com error_code do
// Boost.MySQL não usam io_context::run() — são operações bloqueantes de verdade, ao
// contrário do "assíncrono" antigo que a auditoria apontou como falso, C1).
//
// Ver documentation/roadmap.md (Fase 2).
class MySQLConnection {
public:
	enum State {
		NONE,
		CONFIGURED,
		CONNECTING,
		CONNECTED,
		CLOSING,
		FAILED,
	};

private:
	Ref<MySQLConfig> config;
	State state = NONE;

	// Ordem de declaração importa: ssl_context precisa ser construído antes e
	// destruído depois de connection, que guarda um ponteiro pra ele (contrato de
	// any_connection_params::ssl_context em any_connection.hpp).
	boost::asio::io_context io_context;
	boost::asio::ssl::context ssl_context;
	boost::mysql::any_connection connection;

	boost::mysql::error_code last_error;
	boost::mysql::diagnostics last_diagnostics;

	static boost::asio::ssl::context _make_ssl_context(const Ref<MySQLConfig> &p_config);
	static boost::mysql::any_connection_params _make_any_connection_params(const Ref<MySQLConfig> &p_config, boost::asio::ssl::context &p_ssl_context);
	boost::mysql::connect_params _make_connect_params() const;

public:
	explicit MySQLConnection(Ref<MySQLConfig> p_config);

	// Move-only, como any_connection (ver Thread safety / Single outstanding operation
	// em any_connection.hpp).
	MySQLConnection(const MySQLConnection &) = delete;
	MySQLConnection &operator=(const MySQLConnection &) = delete;

	// connect()/close() são síncronas e bloqueantes nesta fase (ver comentário acima).
	// Devolvem false em erro; get_last_error()/get_last_diagnostics() têm o motivo.
	bool connect();
	void close();

	bool is_connected() const { return state == CONNECTED; }
	State get_state() const { return state; }

	const boost::mysql::error_code &get_last_error() const { return last_error; }
	const boost::mysql::diagnostics &get_last_diagnostics() const { return last_diagnostics; }

	// Uso interno de MySQLSession (Fase 4) pra executar queries na conexão real.
	boost::mysql::any_connection &native() { return connection; }

	// Uso interno de MySQLSession (Fase 5): a thread de I/O dedicada roda este
	// io_context pra processar as operações async_* desta conexão.
	boost::asio::io_context &get_io_context() { return io_context; }
};

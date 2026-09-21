// SPDX-License-Identifier: MIT
/* mysql_session.h */
#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#include "mysql_config.h"
#include "mysql_result.h"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <functional>
#include <memory>
#include <string>
#include <thread>

class MySQLConnection;
class PreparedStatementCache;
class MySQLTransaction;
class MySQLAsyncOperation;
class MySQLStreamingCursor;

// MySQLSession — API principal exposta ao GDScript: execute_text/execute_formatted/
// execute_prepared/execute_script, begin_transaction(). Dona de uma MySQLConnection
// (própria, no caminho simples, ou emprestada por um MySQLPool — Fase 5) e do seu
// PreparedStatementCache. Não é thread-safe: uma Session por thread/fluxo de cada vez —
// ver documentation/design-notes.md.
//
// Precisa de MySQLConfig antes de ser usada: `MySQLSession.new()` (exigido pelo
// ClassDB pra instanciar via GDScript) cria a Session vazia, e set_config() é quem
// constrói a MySQLConnection interna de fato. Todo método que precisa de conexão
// devolve um erro explícito (categoria "mysql_module.client") se chamado antes de
// set_config() — nunca derrefencia um ponteiro nulo (resolve S4 da auditoria).
//
// Ver documentation/roadmap.md (Fases 2-5).
class MySQLSession : public RefCounted {
	GDCLASS(MySQLSession, RefCounted);

	Ref<MySQLConfig> config;
	std::unique_ptr<MySQLConnection> connection;
	std::unique_ptr<PreparedStatementCache> statement_cache;

	// Só definido quando a Session veio de um MySQLPool (Fase 5): devolve a conexão pro
	// pool ao ser destruída, em vez de destruí-la.
	std::function<void(std::unique_ptr<MySQLConnection>)> release_to_pool;

	// Thread de I/O dedicada (Fase 5), criada sob demanda no primeiro async_*. O
	// work_guard impede io_context::run() de retornar quando não há operação pendente
	// no momento — a thread fica viva entre chamadas assíncronas.
	std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> io_work_guard;
	std::thread io_thread;
	bool io_thread_started = false;

	void _ensure_io_thread_started();

	Ref<MySQLResult> _execute_text_std(const std::string &p_sql);
	Ref<MySQLResult> _execute_formatted_std(const std::string &p_sql, const Array &p_params);
	Ref<MySQLResult> _execute_prepared_std(const std::string &p_sql, const Array &p_params);

protected:
	static void _bind_methods();

public:
	MySQLSession() = default;
	~MySQLSession();

	// Uso interno de MySQLPool (Fase 5) — não é bind_method.
	static Ref<MySQLSession> create_pooled(Ref<MySQLConfig> p_config, std::unique_ptr<MySQLConnection> p_connection, std::function<void(std::unique_ptr<MySQLConnection>)> p_release_to_pool);

	void set_config(const Ref<MySQLConfig> &p_config);
	Ref<MySQLConfig> get_config() const { return config; }

	// Nomeadas *_db de propósito: Object já reserva connect()/close()/is_connected()
	// pra sinais — usar os mesmos nomes pra "conectar ao banco" ia esconder os métodos
	// de sinal da classe (viraria impossível fazer session.connect("sinal", callable)
	// do jeito normal do Godot).
	Dictionary connect_db();
	Dictionary close_db();
	bool is_db_connected() const;

	Ref<MySQLResult> execute_text(const String &p_sql);
	Ref<MySQLResult> execute_formatted(const String &p_sql, const Array &p_params);
	Ref<MySQLResult> execute_prepared(const String &p_sql, const Array &p_params);
	// Recebe o conteúdo do script (não um caminho de arquivo — corrige um defeito da
	// versão anterior). Divide em instruções, respeitando literais entre aspas (não
	// trata comentários SQL), e executa uma a uma, parando na primeira que falhar. Só
	// funciona com allow_sql_script_execution habilitado na config.
	Array execute_script(const String &p_content);

	Ref<MySQLTransaction> begin_transaction();
	// Uso interno de MySQLTransaction — não é bind_method.
	Dictionary run_control_statement(const String &p_sql);

	// Fase 5: assíncrono real (thread de I/O dedicada) e streaming.
	Ref<MySQLAsyncOperation> async_execute_text(const String &p_sql);
	Ref<MySQLAsyncOperation> async_execute_prepared(const String &p_sql, const Array &p_params);
	Ref<MySQLStreamingCursor> execute_streaming(const String &p_sql);

	// Uso interno de MySQLPool (Fase 5).
	MySQLConnection *get_connection() const { return connection.get(); }
};

// SPDX-License-Identifier: MIT
/* mysql_streaming_cursor.h */
#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#include "mysql_config.h"

#include <boost/mysql/execution_state.hpp>

#include <string>

class MySQLConnection;
class MySQLSession;

// MySQLStreamingCursor — leitura incremental (next_batch()/has_more()/close()) sobre
// execution_state/read_some_rows do Boost.MySQL; is_ok()/get_error() (Dictionary). Se
// fechada no meio, drena o resto do resultset: o protocolo exige o resultset drenado
// por completo antes do próximo comando na mesma conexão, senão a conexão desalinha
// (mesmo efeito do S8 da auditoria, só que causado pelo consumidor em vez de um bug de
// leitura). Só uma por conexão por vez — quem quiser rodar outra query espera esta
// fechar. Síncrona nesta fase (bloqueia em next_batch()); uma variante assíncrona é uma
// extensão natural sobre a mesma engine da Fase 5, mas não faz parte deste lote.
//
// Guarda um Ref<MySQLSession> (não a MySQLConnection direto) pra manter a Session — e a
// conexão por trás dela — viva enquanto o cursor existir, pela mesma razão de
// MySQLTransaction (evita o padrão de ponteiro pendente do S3 da auditoria).
//
// Ver documentation/roadmap.md (Fase 5).
class MySQLStreamingCursor : public RefCounted {
	GDCLASS(MySQLStreamingCursor, RefCounted);

	Ref<MySQLSession> owner_session;
	MySQLConnection *connection = nullptr;
	Ref<MySQLConfig> config;
	boost::mysql::execution_state state;
	bool ok = true;
	bool closed = false;
	Dictionary error;

protected:
	static void _bind_methods();

public:
	// Uso interno de MySQLSession::execute_streaming() — não são bind_method.
	static Ref<MySQLStreamingCursor> start(Ref<MySQLSession> p_session, MySQLConnection &p_connection, const Ref<MySQLConfig> &p_config, const std::string &p_sql);
	static Ref<MySQLStreamingCursor> from_error(const Dictionary &p_error);

	bool is_ok() const { return ok; }
	Dictionary get_error() const { return error; }
	bool has_more() const { return ok && !closed && !state.complete(); }
	PackedStringArray get_column_names() const;
	Array next_batch();
	void close();

	~MySQLStreamingCursor();
};

// SPDX-License-Identifier: MIT
/* mysql_transaction.h */
#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"

class MySQLSession;

// MySQLTransaction — commit()/rollback(); se destruída sem nenhum dos dois, faz
// rollback automático (rede de segurança, sem exceção). Guarda um Ref<MySQLSession> (não
// um ponteiro cru): assim a Session — e a conexão por trás dela — não pode ser destruída
// enquanto uma Transaction ainda existir, o que eliminaria o risco de ponteiro pendente
// (o mesmo tipo de bug do S3 da auditoria, só que na fronteira Session/Transaction em
// vez de String/CharString).
// Ver documentation/roadmap.md (Fase 4).
class MySQLTransaction : public RefCounted {
	GDCLASS(MySQLTransaction, RefCounted);

	Ref<MySQLSession> session;
	bool finished = false;

protected:
	static void _bind_methods();

public:
	// Uso interno de MySQLSession::begin_transaction() — não é bind_method.
	static Ref<MySQLTransaction> create(Ref<MySQLSession> p_session);

	Dictionary commit();
	Dictionary rollback();

	~MySQLTransaction();
};

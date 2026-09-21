// SPDX-License-Identifier: MIT
/* mysql_async_operation.h */
#pragma once

#include "mysql_result.h"

#include "core/object/ref_counted.h"

// MySQLAsyncOperation — RefCounted com sinal `completed`, devolvido por toda chamada
// async_*; carrega o MySQLResult (ou o erro, dentro dele) quando termina. _complete() só
// é chamada via call_deferred a partir da thread de I/O da Fase 5 — nunca diretamente —
// porque emit_signal() e o resto da API do Godot não são seguros fora da thread
// principal.
// Ver documentation/roadmap.md (Fase 5).
class MySQLAsyncOperation : public RefCounted {
	GDCLASS(MySQLAsyncOperation, RefCounted);

	Ref<MySQLResult> result;
	bool finished_flag = false;

protected:
	static void _bind_methods();

public:
	// Precisa ser bind_method (não só uso interno) porque call_deferred() despacha por
	// nome de método, não por ponteiro de função. Não chame diretamente fora da thread
	// principal.
	void _complete(Ref<MySQLResult> p_result);

	bool is_finished() const { return finished_flag; }
	Ref<MySQLResult> get_result() const { return result; }
};

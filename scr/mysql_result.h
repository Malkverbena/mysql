// SPDX-License-Identifier: MIT
/* mysql_result.h */
#pragma once

#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#include "mysql_config.h"

#include <boost/mysql/column_type.hpp>
#include <boost/mysql/results.hpp>

// MySQLResult — dado puro devolvido por uma execução: metadata ordenada, linhas (Array
// de Array, preserva colunas duplicadas de JOINs — nunca Dictionary por nome, ver D1 da
// auditoria), affected_rows/last_insert_id em 64 bits (String se BIGINT UNSIGNED
// estourar INT64_MAX), múltiplos resultsets preservados. is_ok()/get_error() (Dictionary
// com category/message/server_message/is_fatal) em vez de estado de erro ambiente — ver
// design-notes.md.
//
// get_parsed_json() é a única operação não totalmente "pura": json_result_mode ==
// LAZY_PARSED_VARIANT guarda o texto bruto nas linhas e só converte (com cache) quando
// pedido explicitamente — decisão registrada em design-notes.md.
//
// Ver documentation/roadmap.md (Fase 3).
class MySQLResult : public RefCounted {
	GDCLASS(MySQLResult, RefCounted);

	struct ResultsetData {
		Vector<String> column_names;
		Vector<boost::mysql::column_type> column_types; // Só pra saber quais colunas são JSON em get_parsed_json().
		Array rows; // Array de Array — cada linha é um Array de valores na ordem das colunas.
		Variant affected_rows;
		Variant last_insert_id;
		String info;
	};

	bool ok = true;
	Dictionary error;
	Vector<ResultsetData> resultsets;
	HashMap<uint64_t, Variant> json_cache;

	const ResultsetData *_get_resultset(int p_index) const;
	static uint64_t _json_cache_key(int p_resultset, int p_row, int p_column);

protected:
	static void _bind_methods();

public:
	// Uso interno (MySQLSession, Fase 4) — não são bind_method.
	static Ref<MySQLResult> from_boost_results(const boost::mysql::results &p_results, const Ref<MySQLConfig> &p_config);
	static Ref<MySQLResult> from_error(const Dictionary &p_error);

	bool is_ok() const { return ok; }
	Dictionary get_error() const { return error; }

	int get_resultset_count() const { return resultsets.size(); }
	PackedStringArray get_column_names(int p_resultset = 0) const;
	Array get_rows(int p_resultset = 0) const;
	Variant get_affected_rows(int p_resultset = 0) const;
	Variant get_last_insert_id(int p_resultset = 0) const;
	String get_info(int p_resultset = 0) const;

	// Converte (com cache) uma célula JSON guardada como texto bruto — só faz sentido
	// com json_result_mode == LAZY_PARSED_VARIANT; nos outros modos a célula já vem
	// pronta em get_rows().
	Variant get_parsed_json(int p_resultset, int p_row, int p_column);
};

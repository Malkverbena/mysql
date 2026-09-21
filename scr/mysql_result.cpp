// SPDX-License-Identifier: MIT
/* mysql_result.cpp */

#include "mysql_result.h"

#include "core/io/json.h"
#include "core/object/class_db.h"

#include "godot_convert.h"
#include "mysql_type_convert.h"

#include <boost/mysql/metadata_collection_view.hpp>
#include <boost/mysql/resultset_view.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/rows_view.hpp>

const MySQLResult::ResultsetData *MySQLResult::_get_resultset(int p_index) const {
	if (p_index < 0 || p_index >= resultsets.size()) {
		ERR_FAIL_V_MSG(nullptr, vformat("MySQLResult: índice de resultset inválido (%d), há %d", p_index, resultsets.size()));
	}
	return &resultsets[p_index];
}

uint64_t MySQLResult::_json_cache_key(int p_resultset, int p_row, int p_column) {
	return ((uint64_t)(uint32_t)p_resultset << 48) | ((uint64_t)(uint32_t)p_column << 32) | (uint64_t)(uint32_t)p_row;
}

Ref<MySQLResult> MySQLResult::from_boost_results(const boost::mysql::results &p_results, const Ref<MySQLConfig> &p_config) {
	Ref<MySQLResult> result;
	result.instantiate();
	result->ok = true;

	std::size_t resultset_count = p_results.size();
	result->resultsets.resize((int)resultset_count);

	for (std::size_t i = 0; i < resultset_count; i++) {
		boost::mysql::resultset_view rv = p_results[i];
		ResultsetData &data = result->resultsets.write[(int)i];

		boost::mysql::metadata_collection_view meta = rv.meta();
		for (std::size_t c = 0; c < meta.size(); c++) {
			const boost::mysql::metadata &col_meta = meta[c];
			boost::mysql::string_view name = col_meta.column_name();
			data.column_names.push_back(mysql_module::to_godot_string(name.data(), name.size()));
			data.column_types.push_back(col_meta.type());
		}

		boost::mysql::rows_view rows = rv.rows();
		data.rows.resize((int)rows.size());
		for (std::size_t r = 0; r < rows.size(); r++) {
			boost::mysql::row_view row = rows[r];
			Array row_array;
			row_array.resize((int)row.size());
			for (std::size_t c = 0; c < row.size(); c++) {
				row_array[(int)c] = mysql_module::field_to_variant(row[c], meta[c], p_config);
			}
			data.rows[(int)r] = row_array;
		}

		data.affected_rows = mysql_module::uint64_to_variant(rv.affected_rows());
		data.last_insert_id = mysql_module::uint64_to_variant(rv.last_insert_id());

		boost::mysql::string_view info = rv.info();
		data.info = mysql_module::to_godot_string(info.data(), info.size());
	}

	return result;
}

Ref<MySQLResult> MySQLResult::from_error(const Dictionary &p_error) {
	Ref<MySQLResult> result;
	result.instantiate();
	result->ok = false;
	result->error = p_error;
	return result;
}

PackedStringArray MySQLResult::get_column_names(int p_resultset) const {
	const ResultsetData *data = _get_resultset(p_resultset);
	PackedStringArray names;
	if (!data) {
		return names;
	}
	names.resize(data->column_names.size());
	for (int i = 0; i < data->column_names.size(); i++) {
		names.set(i, data->column_names[i]);
	}
	return names;
}

Array MySQLResult::get_rows(int p_resultset) const {
	const ResultsetData *data = _get_resultset(p_resultset);
	return data ? data->rows : Array();
}

Variant MySQLResult::get_affected_rows(int p_resultset) const {
	const ResultsetData *data = _get_resultset(p_resultset);
	return data ? data->affected_rows : Variant();
}

Variant MySQLResult::get_last_insert_id(int p_resultset) const {
	const ResultsetData *data = _get_resultset(p_resultset);
	return data ? data->last_insert_id : Variant();
}

String MySQLResult::get_info(int p_resultset) const {
	const ResultsetData *data = _get_resultset(p_resultset);
	return data ? data->info : String();
}

Variant MySQLResult::get_parsed_json(int p_resultset, int p_row, int p_column) {
	const ResultsetData *data = _get_resultset(p_resultset);
	if (!data) {
		return Variant();
	}
	ERR_FAIL_INDEX_V(p_column, data->column_types.size(), Variant());
	ERR_FAIL_COND_V_MSG(data->column_types[p_column] != boost::mysql::column_type::json, Variant(), "MySQLResult: get_parsed_json() chamado numa coluna que não é JSON.");

	uint64_t key = _json_cache_key(p_resultset, p_row, p_column);
	if (Variant *cached = json_cache.getptr(key)) {
		return *cached;
	}

	ERR_FAIL_INDEX_V(p_row, data->rows.size(), Variant());
	Array row = data->rows[p_row];
	ERR_FAIL_INDEX_V(p_column, row.size(), Variant());

	Variant parsed = JSON::parse_string(row[p_column]);
	json_cache.insert(key, parsed);
	return parsed;
}

void MySQLResult::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_ok"), &MySQLResult::is_ok);
	ClassDB::bind_method(D_METHOD("get_error"), &MySQLResult::get_error);

	ClassDB::bind_method(D_METHOD("get_resultset_count"), &MySQLResult::get_resultset_count);
	ClassDB::bind_method(D_METHOD("get_column_names", "resultset"), &MySQLResult::get_column_names, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_rows", "resultset"), &MySQLResult::get_rows, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_affected_rows", "resultset"), &MySQLResult::get_affected_rows, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_last_insert_id", "resultset"), &MySQLResult::get_last_insert_id, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_info", "resultset"), &MySQLResult::get_info, DEFVAL(0));

	ClassDB::bind_method(D_METHOD("get_parsed_json", "resultset", "row", "column"), &MySQLResult::get_parsed_json);
}

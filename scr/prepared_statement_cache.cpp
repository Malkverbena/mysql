// SPDX-License-Identifier: MIT
/* prepared_statement_cache.cpp */

#include "prepared_statement_cache.h"

#include "mysql_connection.h"

bool PreparedStatementCache::get_or_prepare(MySQLConnection &p_connection, const std::string &p_sql, boost::mysql::statement &r_statement, boost::mysql::error_code &r_error, boost::mysql::diagnostics &r_diagnostics) {
	auto found = lookup.find(p_sql);
	if (found != lookup.end()) {
		lru.splice(lru.begin(), lru, found->second);
		r_statement = found->second->second;
		r_error.clear();
		r_diagnostics.clear();
		return true;
	}

	r_error.clear();
	r_diagnostics.clear();
	boost::mysql::statement stmt = p_connection.native().prepare_statement(p_sql, r_error, r_diagnostics);
	if (r_error) {
		return false;
	}

	if (lru.size() >= max_size) {
		Entry &oldest = lru.back();
		boost::mysql::error_code close_err;
		boost::mysql::diagnostics close_diag;
		// Erro ao fechar o handle mais antigo não é fatal aqui: só significa que o
		// servidor mantém o recurso alocado até a conexão terminar.
		p_connection.native().close_statement(oldest.second, close_err, close_diag);
		lookup.erase(oldest.first);
		lru.pop_back();
	}

	lru.push_front(Entry(p_sql, stmt));
	lookup[p_sql] = lru.begin();
	r_statement = stmt;
	return true;
}

void PreparedStatementCache::clear() {
	lru.clear();
	lookup.clear();
}

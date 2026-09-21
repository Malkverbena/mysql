// SPDX-License-Identifier: MIT
/* prepared_statement_cache.h */
#pragma once

#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>
#include <boost/mysql/statement.hpp>

#include <list>
#include <string>
#include <unordered_map>

class MySQLConnection;

// PreparedStatementCache — cache LRU de SQL -> handle de prepared statement, dentro de
// uma única MySQLSession (handles são por conexão, resolve C7 da auditoria: hoje
// prepara/executa/fecha a cada chamada, 3 round-trips). Ao estourar max_size, fecha
// (close_statement) o menos usado antes de descartá-lo, porque o handle continua
// ocupando recursos no servidor até ser fechado ou a conexão terminar. clear() (chamada
// ao reconectar) só esvazia o cache, sem fechar nada: os handles antigos já não valem
// pra uma conexão nova. NÃO é exposta ao GDScript.
// Ver documentation/roadmap.md (Fase 4).
class PreparedStatementCache {
public:
	explicit PreparedStatementCache(size_t p_max_size = 64) :
			max_size(p_max_size) {}

	bool get_or_prepare(MySQLConnection &p_connection, const std::string &p_sql, boost::mysql::statement &r_statement, boost::mysql::error_code &r_error, boost::mysql::diagnostics &r_diagnostics);
	void clear();

private:
	using Entry = std::pair<std::string, boost::mysql::statement>;

	size_t max_size;
	std::list<Entry> lru; // frente = mais usado recentemente.
	std::unordered_map<std::string, std::list<Entry>::iterator> lookup;
};

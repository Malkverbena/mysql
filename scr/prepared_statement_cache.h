/* prepared_statement_cache.h */
#pragma once

#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"

#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>
#include <boost/mysql/statement.hpp>

class MySQLConnection;

// LRU cache from SQL text to prepared statement handle, owned by a single
// `MySQLConnection` (handles are only valid on the connection that prepared them). Without it every execution would prepare, execute and
// close the statement, costing three round trips.
//
// When the cache is full, the least recently used statement is closed on the server
// before being dropped, because the handle keeps using server resources until it is
// closed or the connection ends. `clear()` (called when the connection closes, or when a
// pooled connection is reset for a new lease) only empties the cache without closing
// anything: the server has already closed those statements.
//
// Internal class, never exposed to GDScript.
class PreparedStatementCache {
	struct Entry {
		String sql;
		boost::mysql::statement statement;
	};

	int max_size;
	List<Entry> lru; // Front is the most recently used.
	HashMap<String, List<Entry>::Element *> lookup;

public:
	explicit PreparedStatementCache(int p_max_size = 64) :
			max_size(p_max_size) {}

	bool get_or_prepare(MySQLConnection &p_connection, const String &p_sql, boost::mysql::statement &r_statement, boost::mysql::error_code &r_error, boost::mysql::diagnostics &r_diagnostics);
	void clear();

	int get_size() const { return lru.size(); }
	bool contains(const String &p_sql) const { return lookup.has(p_sql); }
};

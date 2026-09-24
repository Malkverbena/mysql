/* prepared_statement_cache.cpp */

#include "prepared_statement_cache.h"

#include "godot_convert.h"
#include "mysql_connection.h"

bool PreparedStatementCache::get_or_prepare(MySQLConnection &p_connection, const String &p_sql, boost::mysql::statement &r_statement, boost::mysql::error_code &r_error, boost::mysql::diagnostics &r_diagnostics) {
	r_error.clear();
	r_diagnostics.clear();

	List<Entry>::Element **found = lookup.getptr(p_sql);
	if (found) {
		lru.move_to_front(*found);
		r_statement = (*found)->get().statement;
		return true;
	}

	boost::mysql::statement statement = p_connection.native().prepare_statement(mysql_module::to_std_string(p_sql), r_error, r_diagnostics);
	if (r_error) {
		return false;
	}

	if (lru.size() >= max_size) {
		List<Entry>::Element *oldest = lru.back();
		boost::mysql::error_code close_error;
		boost::mysql::diagnostics close_diagnostics;
		// A failure to close the oldest handle is not fatal: the server just keeps the
		// resource allocated until the connection ends.
		p_connection.native().close_statement(oldest->get().statement, close_error, close_diagnostics);
		lookup.erase(oldest->get().sql);
		lru.erase(oldest);
	}

	Entry entry;
	entry.sql = p_sql;
	entry.statement = statement;
	lookup[p_sql] = lru.push_front(entry);
	r_statement = statement;
	return true;
}

void PreparedStatementCache::clear() {
	lru.clear();
	lookup.clear();
}

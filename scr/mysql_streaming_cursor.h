/* mysql_streaming_cursor.h */
#pragma once

#include "mysql_config.h"

#include "core/object/ref_counted.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#include <boost/mysql/execution_state.hpp>

#include <string>

class MySQLConnection;
class MySQLSession;

// Incremental reading (`next_batch()`, `has_more()`, `close()`) on top of `execution_state`
// and `read_some_rows()` from Boost.MySQL, with `is_ok()` and `get_error()` (a
// `Dictionary`). If it is closed midway, it drains the rest of the resultset: the protocol
// requires the resultset to be fully read before the next command on the same connection,
// otherwise the connection gets out of sync. Only one cursor can be open per connection at
// a time; anyone who wants to run another query waits for it to close. It is synchronous
// (it blocks in `next_batch()`); an asynchronous variant would be a natural extension on
// the same I/O thread engine.
//
// It holds a `Ref<MySQLSession>` (not the `MySQLConnection` directly) to keep the session,
// and the connection behind it, alive while the cursor exists, for the same reason as
// `MySQLTransaction`.
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
	// Internal use by `MySQLSession::execute_streaming()`, not bound.
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

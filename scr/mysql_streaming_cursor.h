/* mysql_streaming_cursor.h */
#pragma once

#include "mysql_config.h"

#include "core/object/ref_counted.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#include <boost/mysql/execution_state.hpp>

#include <string>

class MySQLAsyncOperation;
class MySQLConnection;
class MySQLSession;

// Incremental reading (`next_batch()`, `has_more()`, `close()`) on top of `execution_state`
// and `read_some_rows()` from Boost.MySQL, with `is_ok()` and `get_error()` (a
// `Dictionary`). If it is closed midway, it drains the rest of the resultset: the protocol
// requires the resultset to be fully read before the next command on the same connection,
// otherwise the connection gets out of sync. Until then, nothing else runs on the
// connection: any other call on the session fails with `engaged_in_multi_function` (nothing
// waits or queues), so only one cursor can be open per connection at a time. A cursor from
// `execute_streaming()` is synchronous (it blocks in `next_batch()`); see below for the
// asynchronous kind.
//
// It holds a `Ref<MySQLSession>` (not the `MySQLConnection` directly) to keep the session,
// and the connection behind it, alive while the cursor exists, for the same reason as
// `MySQLTransaction`.
//
// A cursor from `MySQLSession::async_execute_streaming()` is asynchronous instead: it
// opens on the I/O thread of the session right away, and reads with `async_next_batch()`,
// each batch a step on the I/O thread like any other `async_*` operation (see
// `mysql_session.cpp`). Its `state` then belongs to whichever step is in flight, so the
// main thread never reads it while one runs: `has_more()`, `is_ok()`, `get_error()` and
// `get_column_names()` answer from copies each step leaves on its operation, applied on
// the main thread right before the step's `completed` is emitted (`_apply_async_step()`).
// The two kinds do not mix: `next_batch()` is for synchronous cursors only, and
// `async_next_batch()`/`async_close()` for asynchronous ones.
class MySQLStreamingCursor : public RefCounted {
	GDCLASS(MySQLStreamingCursor, RefCounted);

	Ref<MySQLSession> owner_session;
	MySQLConnection *connection = nullptr;
	Ref<MySQLConfig> config;
	bool ok = true;
	bool closed = false;
	Dictionary error;

	// Asynchronous cursors only, all on the main thread.
	bool async_mode = false;
	// The step that opens the cursor also reads the first batch, and the first
	// `async_next_batch()` returns it (see `start_async()`).
	Ref<MySQLAsyncOperation> first_step;
	bool first_step_taken = false;
	// The step last started. A new one is rejected while it runs.
	Ref<MySQLAsyncOperation> last_step;
	bool more = true;
	bool engaged = true;
	PackedStringArray column_names;
	// `close()` was called while a step was running: drain once it finishes.
	bool close_requested = false;
	// The drain started by `_start_async_drain()`, returned by a later `async_close()`.
	Ref<MySQLAsyncOperation> drain_step;

	void _drain();
	bool _step_running() const;
	// Hands the rest of the execution to a drain on the I/O thread (`async_close()`,
	// `close()` after a running step, destruction), and releases the connection's cursor
	// slot in the session, which stays busy with the drain instead.
	Ref<MySQLAsyncOperation> _start_async_drain();

protected:
	static void _bind_methods();

public:
	// Internal use by `MySQLSession::execute_streaming()`, not bound.
	static Ref<MySQLStreamingCursor> start(Ref<MySQLSession> p_session, MySQLConnection &p_connection, const Ref<MySQLConfig> &p_config, const std::string &p_sql);
	static Ref<MySQLStreamingCursor> from_error(const Dictionary &p_error, bool p_async = false);
	// Internal use by `MySQLSession::async_execute_streaming()`, not bound. Starts the
	// first step (open, then read the first batch) and returns it in `r_first_step`.
	static Ref<MySQLStreamingCursor> start_async(Ref<MySQLSession> p_session, MySQLConnection &p_connection, const Ref<MySQLConfig> &p_config, const String &p_sql, Ref<MySQLAsyncOperation> &r_first_step);

	// The execution being read. Internal use by the asynchronous steps in
	// `mysql_session.cpp`, only while the step that uses it runs.
	boost::mysql::execution_state state;
	// Internal use by `MySQLAsyncOperation::_complete()`, on the main thread.
	void _apply_async_step(const MySQLAsyncOperation &p_step);
	MySQLConnection *get_connection() const { return connection; }

	bool is_ok() const { return ok; }
	Dictionary get_error() const { return error; }
	// `should_read_rows()`, not `!complete()`: a cursor only reads a single resultset (see
	// `MySQLSession::execute_streaming()`), and with `allow_multi_queries` the execution is
	// neither complete nor readable as rows between two resultsets. Reporting "more" there
	// would make the documented `while (has_more()) next_batch()` loop spin forever on empty
	// batches, because `read_some_rows()` in that state returns nothing and advances nothing.
	// The resultsets left over are drained by `close()`.
	//
	// An asynchronous cursor answers from the copy its last step left instead, and says
	// "more" until the first batch has been handed out, so that the documented loop never
	// skips it.
	bool has_more() const;
	// Whether the cursor still holds the connection: until the execution is complete (read
	// to the end, or drained by `close()`), no other command can run on it.
	bool is_engaged() const;
	PackedStringArray get_column_names() const;
	Array next_batch();
	void close();
	Ref<MySQLAsyncOperation> async_next_batch();
	Ref<MySQLAsyncOperation> async_close();

	~MySQLStreamingCursor();
};

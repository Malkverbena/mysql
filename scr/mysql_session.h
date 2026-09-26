/* mysql_session.h */
#pragma once

#include "mysql_config.h"
#include "mysql_result.h"

#include "core/object/ref_counted.h"
#include "core/os/thread.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <string>

class MySQLConnection;
class MySQLPool;
class MySQLTransaction;
class MySQLAsyncOperation;
class MySQLStreamingCursor;

// Main class exposed to GDScript: `execute_text()`, `execute_formatted()`,
// `execute_prepared()`, `execute_script()`, `begin_transaction()` and the asynchronous and
// streaming variants. It owns a `MySQLConnection` (its own, or one leased from a
// `MySQLPool`) — the prepared statement cache belongs to the `MySQLConnection` itself, not
// to the session, so it survives across pool leases (see the comment on
// `MySQLConnection::statement_cache`).
//
// A session is not thread safe: use one session per thread.
//
// A session needs a `MySQLConfig` before it can be used. `MySQLSession.new()` (required by
// `ClassDB` to instantiate it from GDScript) creates an empty session, and `set_config()`
// builds the internal connection. Every method that needs the connection returns an
// explicit error (category `mysql_module.client`) when called before `set_config()`, and
// never dereferences a null pointer.
class MySQLSession : public RefCounted {
	GDCLASS(MySQLSession, RefCounted);

	typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> IOWorkGuard;

	Ref<MySQLConfig> config;
	MySQLConnection *connection = nullptr;

	// Only set when the session was leased from a pool: the connection goes back to the
	// pool on destruction instead of being destroyed. Holding a reference also keeps the
	// pool alive for as long as the session exists.
	Ref<MySQLPool> owner_pool;

	// The asynchronous operation last started on this connection (a call rejected because
	// one was already running does not replace it). While it runs, the I/O thread owns the
	// connection: every other method checks `_is_async_busy()` first and fails with
	// `operation_in_progress` instead of touching the connection from the main thread
	// (which would be a data race with the I/O thread, even just to let Boost.MySQL
	// reject the call). Also checked in the destructor: if the session is destroyed while
	// the operation is still running, the connection cannot be reused (Boost.MySQL allows
	// one outstanding operation per `any_connection`, and the module cannot cancel one in
	// flight), so a pooled connection is discarded instead of recycled (see
	// `MySQLPool::release()`).
	Ref<MySQLAsyncOperation> pending_async_operation;

	// The streaming cursor last opened on this connection, cleared by the cursor when it
	// closes. A raw pointer, not a `Ref`: the cursor holds a `Ref` to this session, so it
	// never outlives it, and a `Ref` here would be a reference cycle. While it has not
	// been read to the end, Boost.MySQL rejects any other command on the connection; the
	// session rejects them first (`_busy_error()`), so that an `async_*` call cannot hand
	// that rejection to the I/O thread while the main thread is still reading the cursor.
	MySQLStreamingCursor *active_cursor = nullptr;

	// Dedicated I/O thread, started on demand by the first `async_*` call. The work guard
	// keeps `io_context::run()` from returning while no operation is pending, so the thread
	// stays alive between asynchronous calls.
	IOWorkGuard *io_work_guard = nullptr;
	Thread io_thread;

	static void _io_thread_main(void *p_session);
	void _ensure_io_thread_started();

	Ref<MySQLResult> _execute_text_std(const std::string &p_sql);
	Ref<MySQLResult> _execute_formatted_std(const std::string &p_sql, const Array &p_params);
	bool _is_async_busy() const;
	// Why the connection cannot take a new command right now (an asynchronous operation
	// is running, or a streaming cursor has not been read to the end), as the same error
	// Boost.MySQL would give; empty if it is free.
	Dictionary _busy_error() const;
	// Whether the server currently treats `\` as an escape inside string literals.
	bool _backslash_escapes() const;

protected:
	static void _bind_methods();

public:
	MySQLSession() = default;
	~MySQLSession();

	// Internal use by `MySQLPool`, not bound.
	static Ref<MySQLSession> create_pooled(const Ref<MySQLConfig> &p_config, MySQLConnection *p_connection, const Ref<MySQLPool> &p_owner_pool);

	// Keeps a copy of `p_config` (see the comment on `MySQLConfig`).
	void set_config(const Ref<MySQLConfig> &p_config);
	// A copy: changing it has no effect on this session.
	Ref<MySQLConfig> get_config() const { return config.is_valid() ? config->duplicate_config() : Ref<MySQLConfig>(); }

	// Named `*_db` on purpose: `Object` already reserves `connect()`, `close()` and
	// `is_connected()` for signals. Reusing those names would hide the signal methods and
	// make `session.connect("signal", callable)` impossible.
	Dictionary connect_db();
	Dictionary close_db();
	bool is_db_connected() const;

	Ref<MySQLResult> execute_text(const String &p_sql);
	Ref<MySQLResult> execute_formatted(const String &p_sql, const Array &p_params);
	Ref<MySQLResult> execute_prepared(const String &p_sql, const Array &p_params);
	// Takes the script content, not a file path. It splits the content into statements
	// (see `next_sql_statement()`) and runs them one by one, stopping at the first one
	// that fails. Only works with `allow_sql_script_execution` enabled in the config.
	Array execute_script(const String &p_content);

	Ref<MySQLTransaction> begin_transaction();
	// Internal use by `MySQLTransaction`, not bound.
	Dictionary run_control_statement(const String &p_sql);
	// Internal use by `MySQLTransaction`, not bound: the error a statement would get right
	// now because the session is busy (see `_busy_error()`), or an empty `Dictionary`.
	Dictionary get_busy_error() const { return _busy_error(); }

	Ref<MySQLAsyncOperation> async_execute_text(const String &p_sql);
	Ref<MySQLAsyncOperation> async_execute_prepared(const String &p_sql, const Array &p_params);
	Ref<MySQLStreamingCursor> execute_streaming(const String &p_sql);
	Ref<MySQLStreamingCursor> async_execute_streaming(const String &p_sql);
	// Internal use by `MySQLStreamingCursor`, not bound: makes `p_operation` the one that
	// owns the connection (see `pending_async_operation`) and starts the I/O thread.
	void _track_async_operation(const Ref<MySQLAsyncOperation> &p_operation);
	// Internal use by `MySQLStreamingCursor::close()`, not bound.
	void _cursor_closed(const MySQLStreamingCursor *p_cursor);

	// Internal use by `MySQLPool`.
	MySQLConnection *get_connection() const { return connection; }
};

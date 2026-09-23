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

	// Set by async_execute_text()/async_execute_prepared(), overwritten on every call:
	// tracks only the most recently started operation, which is correct for the common
	// case (a session runs one operation at a time; starting a second one while the first
	// is still in flight fails immediately, see MySQLSession::async_execute_text()).
	// Checked in the destructor: if the session is destroyed before this operation has
	// finished, the connection cannot be reused — Boost.MySQL allows only one outstanding
	// operation on a given `any_connection`, and the module cannot cancel one already in
	// flight (see "MySQLStreamingCursor assíncrono"/cancellation in the roadmap). A pooled
	// connection in that state is discarded instead of recycled (see
	// MySQLPool::release()); this does not cover the narrower case of two operations
	// started back to back without awaiting either before the session is dropped, since
	// only the later one (already rejected, and so already finished) would be tracked.
	Ref<MySQLAsyncOperation> pending_async_operation;

	// Dedicated I/O thread, started on demand by the first `async_*` call. The work guard
	// keeps `io_context::run()` from returning while no operation is pending, so the thread
	// stays alive between asynchronous calls.
	IOWorkGuard *io_work_guard = nullptr;
	Thread io_thread;

	static void _io_thread_main(void *p_session);
	void _ensure_io_thread_started();

	Ref<MySQLResult> _execute_text_std(const std::string &p_sql);
	Ref<MySQLResult> _execute_formatted_std(const std::string &p_sql, const Array &p_params);
	// Whether the server currently treats `\` as an escape inside string literals.
	bool _backslash_escapes() const;

protected:
	static void _bind_methods();

public:
	MySQLSession() = default;
	~MySQLSession();

	// Internal use by `MySQLPool`, not bound.
	static Ref<MySQLSession> create_pooled(const Ref<MySQLConfig> &p_config, MySQLConnection *p_connection, const Ref<MySQLPool> &p_owner_pool);

	void set_config(const Ref<MySQLConfig> &p_config);
	Ref<MySQLConfig> get_config() const { return config; }

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

	Ref<MySQLAsyncOperation> async_execute_text(const String &p_sql);
	Ref<MySQLAsyncOperation> async_execute_prepared(const String &p_sql, const Array &p_params);
	Ref<MySQLStreamingCursor> execute_streaming(const String &p_sql);

	// Internal use by `MySQLPool`.
	MySQLConnection *get_connection() const { return connection; }
};

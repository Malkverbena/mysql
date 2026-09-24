/* mysql_async_operation.h */
#pragma once

#include "mysql_config.h"
#include "mysql_params.h"
#include "mysql_result.h"

#include "core/object/ref_counted.h"
#include "core/templates/safe_refcount.h"

#include <boost/asio/cancellation_signal.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>
#include <boost/mysql/execution_state.hpp>

#include <cstdint>
#include <string>

class MySQLConnection;

// `RefCounted` with a `completed` signal, returned by every `async_*` call. It carries the
// `MySQLResult` (or the error, inside it) once the operation finishes.
//
// It also owns everything the running operation needs: the SQL text, the bound parameters,
// the incremental execution state, the in-progress `MySQLResult::Builder` and the
// diagnostics. The chain of Asio completion handlers (see `mysql_session.cpp`) holds a
// reference to the operation at every step, so this state lives exactly as long as the
// operation can still write to it, and no separate shared state is needed.
class MySQLAsyncOperation : public RefCounted {
	GDCLASS(MySQLAsyncOperation, RefCounted);

	Ref<MySQLResult> result;
	bool finished_flag = false;
	// Set on the main thread right before the operation is handed to Boost.MySQL, cleared
	// on the I/O thread once Boost.MySQL is done with the connection (just before the
	// result is sent to the main thread). Unlike `finished_flag`, which only changes when
	// the main thread processes the deferred `_complete`, it says exactly when the
	// connection is free again, and it is safe to read from the main thread while the I/O
	// thread writes it.
	SafeFlag running;
	// Set by the first `cancel()`, so that a second call does not start a second `KILL`.
	SafeFlag cancel_requested;

protected:
	static void _bind_methods();

public:
	// State used by `MySQLSession` while the operation is in flight. Not for scripts.
	Ref<MySQLConfig> config;
	std::string sql;
	mysql_module::FieldParams params;
	MySQLConnection *connection = nullptr;
	// Applied per network round trip of the chain (`start_execution`, each `read_some_rows`,
	// each `read_resultset_head`), not once for the whole operation. See the comment on
	// `MySQLConfig::async_timeout_ms`.
	int timeout_ms = 0;
	boost::mysql::execution_state exec_state;
	MySQLResult::Builder result_builder;
	boost::mysql::diagnostics diagnostics;
	// The server's id of the connection running the operation, taken when it starts:
	// `cancel()` runs `KILL QUERY <id>` from a side connection.
	std::uint32_t server_connection_id = 0;
	bool has_server_connection_id = false;

	// Only touched on the I/O thread. Every step of the operation is bound to this signal,
	// so emitting it cancels the step in flight (the local fallback of `cancel()`).
	boost::asio::cancellation_signal cancel_signal;
	// While the `KILL QUERY` of `cancel()` is in flight, the operation does not hand its
	// result to the main thread (and the session stays busy) even if it finishes: the KILL
	// could otherwise reach the server after the next query started, and stop that one
	// instead. The result waits here until the KILL is done.
	bool kill_in_flight = false;
	Ref<MySQLResult> held_result;

	// Internal use by `MySQLSession`, not bound: runs on the main thread through a deferred
	// `callable_mp` (see `complete_deferred()` in `mysql_session.cpp`).
	void _complete(Ref<MySQLResult> p_result);

	bool is_finished() const { return finished_flag; }
	bool is_running() const { return running.is_set(); }
	void set_running() { running.set(); }
	void clear_running() { running.clear(); }
	Ref<MySQLResult> get_result() const { return result; }

	// Asks the server to stop the query (`KILL QUERY` from a short-lived side connection,
	// with the same config), and falls back to cancelling the operation locally if that
	// fails. Either way the operation still finishes through `completed`. Returns `false`
	// if there is nothing to cancel (already finished, or never started).
	bool cancel();
	// Internal use by `MySQLSession`: its destructor stopped the I/O thread with the
	// operation still running, and the connection is about to be discarded.
	void abandon() { connection = nullptr; }
};

namespace mysql_module {

// Defined in `mysql_session.cpp`, next to the rest of the asynchronous machinery. Called by
// `MySQLAsyncOperation::cancel()` on the main thread; the work runs on the I/O thread.
void start_async_cancel(const Ref<MySQLAsyncOperation> &p_operation);

} // namespace mysql_module

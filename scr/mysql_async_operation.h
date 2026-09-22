/* mysql_async_operation.h */
#pragma once

#include "mysql_config.h"
#include "mysql_params.h"
#include "mysql_result.h"

#include "core/object/ref_counted.h"

#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>
#include <boost/mysql/execution_state.hpp>

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

	// Must be bound (not only internal) because `call_deferred()` dispatches by method
	// name. Do not call it directly from outside the main thread.
	void _complete(Ref<MySQLResult> p_result);

	bool is_finished() const { return finished_flag; }
	Ref<MySQLResult> get_result() const { return result; }
};

/* mysql_config.h */
#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/type_info.h"

#include <string>

// Connection configuration: credentials, `transport_mode`, `tinyint1_mode`,
// `json_result_mode`, `allow_sql_script_execution` and `allow_multi_queries`. It is meant
// to be immutable after the first `connect_db()` (a usage convention, not enforced in C++:
// whoever builds the connection must not reconfigure a `MySQLConfig` already in use). It is
// shared by reference between all the `MySQLConnection`s created from it, including the
// ones inside a `MySQLPool`.
//
// There is no `get_password()`: the password is never returned to GDScript. It is only
// available through `get_password_std()`, for internal use, which is never bound.
class MySQLConfig : public RefCounted {
	GDCLASS(MySQLConfig, RefCounted);

public:
	enum TransportMode {
		TCP_TLS_DISABLED,
		TCP_TLS_PREFERRED,
		TCP_TLS_REQUIRED,
		UNIX_SOCKET,
	};

	enum JsonResultMode {
		RAW_STRING,
		PARSED_VARIANT,
		LAZY_PARSED_VARIANT,
	};

private:
	String host = "127.0.0.1";
	int port = 3306;
	String unix_socket_path;
	String user;
	// A `std::string` on purpose: the buffer is wiped explicitly with `OPENSSL_cleanse()`
	// and Boost.MySQL takes the password as a `string_view`. Never exposed to GDScript.
	std::string password;
	String database;

	TransportMode transport_mode = TCP_TLS_REQUIRED;
	bool tinyint1_mode = false;
	JsonResultMode json_result_mode = LAZY_PARSED_VARIANT;
	bool allow_sql_script_execution = false;
	bool allow_multi_queries = false;

	// Timeout of asynchronous operations. 0 means no timeout. Applied per network round trip
	// (per Boost.MySQL async call chained while reading a result), not once for the whole
	// logical execute_*() call: a result with many batches/resultsets gets a fresh budget on
	// every hop instead of one shared deadline for all of them. A timeout is a fatal error:
	// the connection is dropped (see `MySQLConnection::drop()`).
	int async_timeout_ms = 30000;
	// Timeout of each step (connect, then `KILL QUERY`) of the short-lived side connection
	// `MySQLAsyncOperation::cancel()` opens to stop the query on the server. 0 means no
	// timeout. When a step fails or times out, the cancellation falls back to cancelling
	// the operation locally, which drops the connection.
	int cancel_timeout_ms = 5000;
	// Minimum number of rows each `MySQLStreamingCursor::async_next_batch()` gathers before
	// handing the batch over (fewer only at the end of the resultset). One read from the
	// server returns only what fits in the read buffer, often a handful of rows, and each
	// asynchronous batch costs the caller at least a frame, so reading row by row would be
	// far too slow. A batch can exceed it by the rows of one read. The synchronous
	// `next_batch()` is not affected.
	int async_batch_rows = 500;
	// Limit on the size of a packet or row. Boost.MySQL already defaults to 64 MB; this only
	// exposes it as an option instead of leaving it hardcoded. This is NOT a limit on the
	// total size of a result (see `max_result_bytes`): Boost.MySQL enforces it per protocol
	// packet, so it bounds a single row, not the sum of all of them.
	int max_buffer_size = 0x4000000;
	// Limit on the total estimated size (all resultsets, all rows) of a single execute_*()
	// result. 0 means no limit. Unlike `max_buffer_size`, this is enforced by reading the
	// result incrementally (`start_execution`/`read_some_rows`) and aborting as soon as the
	// running total goes over the limit, instead of letting Boost.MySQL materialize the whole
	// result first and checking afterwards - so it actually bounds peak memory instead of just
	// failing after the fact. The size counted per cell is an estimate (exact byte length for
	// strings/blobs, a fixed small cost for every other type), not the wire size.
	int64_t max_result_bytes = 0;
	// Max number of prepared statements kept per connection by `PreparedStatementCache`
	// (LRU: the least recently used one is closed server-side to make room). Kept small
	// enough that `MySQLPool::max_size` connections, each filling this cache with distinct
	// SQL text, stay comfortably under a MySQL/MariaDB server's default global
	// `max_prepared_stmt_count` (16382) with headroom for other sessions on the same server.
	int statement_cache_size = 512;

	void _wipe_password();

protected:
	static void _bind_methods();

public:
	void set_host(const String &p_host);
	String get_host() const;

	void set_port(int p_port);
	int get_port() const;

	void set_unix_socket_path(const String &p_path);
	String get_unix_socket_path() const;

	void set_user(const String &p_user);
	String get_user() const;

	void set_password(const String &p_password);
	// Internal use by `MySQLConnection`. Not bound, so it never reaches GDScript.
	const std::string &get_password_std() const;

	void set_database(const String &p_database);
	String get_database() const;

	void set_transport_mode(TransportMode p_mode);
	TransportMode get_transport_mode() const;

	void set_tinyint1_mode(bool p_enabled);
	bool get_tinyint1_mode() const;

	void set_json_result_mode(JsonResultMode p_mode);
	JsonResultMode get_json_result_mode() const;

	void set_allow_sql_script_execution(bool p_allowed);
	bool get_allow_sql_script_execution() const;

	void set_allow_multi_queries(bool p_allowed);
	bool get_allow_multi_queries() const;

	void set_async_timeout_ms(int p_timeout_ms);
	int get_async_timeout_ms() const { return async_timeout_ms; }
	void set_cancel_timeout_ms(int p_timeout_ms);
	int get_cancel_timeout_ms() const { return cancel_timeout_ms; }
	void set_async_batch_rows(int p_rows);
	int get_async_batch_rows() const { return async_batch_rows; }

	void set_max_buffer_size(int p_size);
	int get_max_buffer_size() const { return max_buffer_size; }

	void set_max_result_bytes(int64_t p_size);
	int64_t get_max_result_bytes() const { return max_result_bytes; }

	void set_statement_cache_size(int p_size);
	int get_statement_cache_size() const { return statement_cache_size; }

	MySQLConfig() = default;
	~MySQLConfig();
};

VARIANT_ENUM_CAST(MySQLConfig::TransportMode);
VARIANT_ENUM_CAST(MySQLConfig::JsonResultMode);

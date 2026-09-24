/* mysql_connection.h */
#pragma once

#include "mysql_config.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>

#include <atomic>

class PreparedStatementCache;

// A single physical socket (`any_connection` from Boost.MySQL) and its state machine
// (NONE -> CONFIGURED -> CONNECTING -> CONNECTED -> CLOSING -> FAILED). It is not thread
// safe (neither is `any_connection`, see "Thread safety" in `any_connection.hpp`): only one
// `MySQLSession` uses it at a time. It is NOT exposed to GDScript. It is the resource a
// `MySQLSession` owns by itself, or that a `MySQLPool` manages.
//
// Each `MySQLConnection` has its own `boost::asio::io_context`. `connect()` and `close()`
// are synchronous and blocking (the `error_code` overloads of Boost.MySQL do not need
// `io_context::run()`). The `async_*` operations of `MySQLSession` run this `io_context` on
// a dedicated I/O thread.
class MySQLConnection {
public:
	enum State {
		NONE,
		CONFIGURED,
		CONNECTING,
		CONNECTED,
		CLOSING,
		FAILED,
	};

private:
	Ref<MySQLConfig> config;
	// Atomic because `drop()` can change it on the I/O thread (a fatal error in an
	// asynchronous operation) while the main thread reads it through `is_connected()`.
	std::atomic<State> state{ NONE };

	// The declaration order matters: `ssl_context` must be constructed before, and destroyed
	// after, `connection`, which keeps a pointer to it (see the contract of
	// `any_connection_params::ssl_context` in `any_connection.hpp`).
	boost::asio::io_context io_context;
	boost::asio::ssl::context ssl_context;
	boost::mysql::any_connection connection;

	boost::mysql::error_code last_error;
	boost::mysql::diagnostics last_diagnostics;

	// Owned here, not by `MySQLSession`: a connection leased from a `MySQLPool` outlives
	// any single session that borrows it, and the prepared statement handles it holds are
	// only valid on this specific connection. When the cache belonged to the session, it
	// was discarded with it while its handles stayed open on the server forever, one set
	// per lease. Now the handles live exactly as long as the server keeps them: until the
	// connection closes or `reset_session()` resets it, and both clear the cache.
	PreparedStatementCache *statement_cache = nullptr;

	static boost::asio::ssl::context _make_ssl_context(const Ref<MySQLConfig> &p_config);
	static boost::mysql::any_connection_params _make_any_connection_params(const Ref<MySQLConfig> &p_config, boost::asio::ssl::context &p_ssl_context);
	boost::mysql::connect_params _make_connect_params() const;

public:
	explicit MySQLConnection(Ref<MySQLConfig> p_config);
	~MySQLConnection();

	// Not copyable, like `any_connection` (see "Thread safety" and "Single outstanding
	// operation" in `any_connection.hpp`).
	MySQLConnection(const MySQLConnection &) = delete;
	MySQLConnection &operator=(const MySQLConnection &) = delete;

	// `connect()` and `close()` are synchronous and blocking (see the comment above). They
	// return `false` on error, and `get_last_error()` and `get_last_diagnostics()` have the
	// reason.
	bool connect();
	void close();

	// Drops the connection without talking to the server, after an operation failed with a
	// fatal error (`boost::mysql::is_fatal_error()`, which includes a cancellation such as
	// an `async_timeout_ms` expiring). Boost.MySQL leaves the connection in an unspecified
	// state after such an error, and `close()` must not be used on it (it sends a quit
	// request, and the protocol may be out of sync, with the reply of the failed operation
	// still on its way). Replacing `any_connection` closes the socket at the transport
	// level instead. The state becomes `FAILED` and `get_last_error()` has `p_error`;
	// `connect()` opens a new connection. Does nothing if not connected.
	void drop(const boost::mysql::error_code &p_error);
	// Calls `drop()` if `p_error` is fatal. Returns `p_error`, to chain it into the error
	// the caller reports.
	const boost::mysql::error_code &drop_if_fatal(const boost::mysql::error_code &p_error);

	// Internal use by `MySQLPool::acquire()` before handing a recycled connection to a new
	// lease: runs `RESET CONNECTION` (rolls back an open transaction, drops temporary
	// tables, clears user variables, restores session variables and closes every prepared
	// statement), then restores what `connect()` had set up, in a single round trip. On
	// failure the connection is closed, and `get_last_error()` has the reason.
	bool reset_session();

	bool is_connected() const { return state == CONNECTED; }
	State get_state() const { return state; }

	const boost::mysql::error_code &get_last_error() const { return last_error; }
	const boost::mysql::diagnostics &get_last_diagnostics() const { return last_diagnostics; }

	// Internal use by `MySQLSession` to run queries on the real connection.
	boost::mysql::any_connection &native() { return connection; }

	// Internal use by `MySQLSession`: the cache survives across pool leases (see the
	// comment on `statement_cache` above).
	PreparedStatementCache *get_statement_cache() { return statement_cache; }

	// Internal use by `MySQLSession`: the dedicated I/O thread runs this `io_context` to
	// process the `async_*` operations of this connection.
	boost::asio::io_context &get_io_context() { return io_context; }
};

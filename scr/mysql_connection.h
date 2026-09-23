/* mysql_connection.h */
#pragma once

#include "mysql_config.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>

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
	State state = NONE;

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
	// only valid on this specific connection. Keeping the cache with the connection lets
	// it survive across leases, instead of every new session starting a fresh cache and
	// leaving the previous lease's handles open on the server forever (they were never
	// closed, since the old cache was simply discarded with the session that owned it).
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

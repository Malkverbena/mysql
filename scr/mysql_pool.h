/* mysql_pool.h */
#pragma once

#include "mysql_config.h"
#include "mysql_session.h"

#include "core/object/ref_counted.h"
#include "core/os/condition_variable.h"
#include "core/os/mutex.h"
#include "core/templates/local_vector.h"

class MySQLConnection;

// Owns up to `max_size` `MySQLConnection`s and leases them one at a time as a
// `MySQLSession` (`acquire()`), never the same connection to two leases at once. It is the
// only class of the module meant to be called from several threads (`any_connection` is
// not thread safe by itself). `acquire()` blocks if the pool is at its maximum size and
// no connection is free.
//
// The session returned by `acquire()` is not connected yet: the caller decides when (and
// whether) to call `connect_db()`, the same way as without a pool.
class MySQLPool : public RefCounted {
	GDCLASS(MySQLPool, RefCounted);

	Ref<MySQLConfig> config;
	int max_size = 8;

	// The threading classes Godot itself is built on (`std`, or `mingw_stdthread` on MinGW),
	// not Godot's `BinaryMutex`/`ConditionVariable` wrappers: those have no timed wait,
	// which `acquire(timeout_ms)` needs.
	THREADING_NAMESPACE::mutex mutex;
	THREADING_NAMESPACE::condition_variable condition;
	LocalVector<MySQLConnection *> idle;
	int total_count = 0;

protected:
	static void _bind_methods();

public:
	~MySQLPool();

	// Keeps a copy of `p_config` (see the comment on `MySQLConfig`); every connection of the
	// pool uses that copy.
	void set_config(const Ref<MySQLConfig> &p_config);
	// A copy: changing it has no effect on this pool.
	Ref<MySQLConfig> get_config() const { return config.is_valid() ? config->duplicate_config() : Ref<MySQLConfig>(); }

	void set_max_size(int p_max_size);
	int get_max_size() const { return max_size; }

	// Blocks until a connection is free if the pool is already at its maximum size: without
	// a limit when `p_timeout_ms` is 0, otherwise for at most `p_timeout_ms` milliseconds,
	// returning null if no connection became free in time.
	Ref<MySQLSession> acquire(int p_timeout_ms = 0);

	// Internal use by `MySQLSession`, which hands its connection back on destruction.
	// Takes ownership of `p_connection`. `p_healthy` is false when the session still had
	// an asynchronous operation in flight when it was destroyed: the connection is
	// discarded instead of recycled in that case (see the comment on
	// MySQLSession::pending_async_operation), and the pool creates a fresh replacement on
	// the next acquire() instead of permanently losing that slot. Not bound.
	void release(MySQLConnection *p_connection, bool p_healthy);
};

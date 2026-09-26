/* mysql_transaction.h */
#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"

class MySQLSession;

// `commit()` and `rollback()`. If the object is destroyed without either having been
// called, it rolls back automatically as a safety net (no exception involved).
//
// It holds a `Ref<MySQLSession>` instead of a raw pointer, so the session (and the
// connection behind it) cannot be destroyed while a transaction still exists.
//
// `MySQLSession::begin_transaction()` always returns one, even when `START TRANSACTION`
// fails: the transaction is then already finished, `is_ok()` is false and `get_error()`
// holds the reason, the same pattern as `MySQLResult` (never a null reference that a
// chained `begin_transaction().commit()` would crash on).
class MySQLTransaction : public RefCounted {
	GDCLASS(MySQLTransaction, RefCounted);

	Ref<MySQLSession> session;
	bool finished = false;
	// The connection generation the transaction started in (see
	// `MySQLConnection::generation`). If the connection was lost and reopened since, the
	// server already rolled the transaction back, and COMMIT on the new connection would
	// report success for data that is gone.
	uint64_t generation = 0;
	// Error from `START TRANSACTION`; empty when the transaction started.
	Dictionary start_error;

	// Whether the connection is still the one the transaction started on.
	bool _still_exists() const;
	// COMMIT or ROLLBACK, unless the session is busy (see the `.cpp`).
	Dictionary _finish(const char *p_statement, const char *p_method);

protected:
	static void _bind_methods();

public:
	// Internal use by `MySQLSession::begin_transaction()`, not bound.
	static Ref<MySQLTransaction> create(Ref<MySQLSession> p_session);
	static Ref<MySQLTransaction> create_failed(const Dictionary &p_error);

	bool is_ok() const { return start_error.is_empty(); }
	Dictionary get_error() const { return start_error; }

	Dictionary commit();
	Dictionary rollback();

	~MySQLTransaction();
};

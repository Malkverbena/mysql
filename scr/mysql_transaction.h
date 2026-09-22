// SPDX-License-Identifier: MIT
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
class MySQLTransaction : public RefCounted {
	GDCLASS(MySQLTransaction, RefCounted);

	Ref<MySQLSession> session;
	bool finished = false;

protected:
	static void _bind_methods();

public:
	// Internal use by `MySQLSession::begin_transaction()`, not bound.
	static Ref<MySQLTransaction> create(Ref<MySQLSession> p_session);

	Dictionary commit();
	Dictionary rollback();

	~MySQLTransaction();
};

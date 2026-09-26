/* mysql_transaction.cpp */

#include "mysql_transaction.h"

#include "mysql_error.h"
#include "mysql_session.h"

#include "core/object/class_db.h"

Ref<MySQLTransaction> MySQLTransaction::create(Ref<MySQLSession> p_session) {
	Ref<MySQLTransaction> tx;
	tx.instantiate();
	tx->session = p_session;
	return tx;
}

Ref<MySQLTransaction> MySQLTransaction::create_failed(const Dictionary &p_error) {
	Ref<MySQLTransaction> tx;
	tx.instantiate();
	tx->start_error = p_error;
	tx->finished = true; // Nothing to commit or roll back, not even automatically.
	return tx;
}

Dictionary MySQLTransaction::_finish(const char *p_statement, const char *p_method) {
	if (!start_error.is_empty()) {
		return start_error;
	}
	if (finished) {
		return mysql_module::make_client_error_dict(vformat("MySQLTransaction: %s() called on a transaction that is already finished.", p_method));
	}
	// A busy session rejects the statement before sending it: the transaction is still
	// open on the server, so it stays open here too and the call can be repeated once the
	// session is free. Marking it finished used to leave it open on the server for good,
	// holding its locks, with neither commit(), rollback() nor the destructor able to end it.
	Dictionary busy = session->get_busy_error();
	if (!busy.is_empty()) {
		return busy;
	}
	finished = true;
	return session->run_control_statement(p_statement);
}

Dictionary MySQLTransaction::commit() {
	return _finish("COMMIT", "commit");
}

Dictionary MySQLTransaction::rollback() {
	return _finish("ROLLBACK", "rollback");
}

MySQLTransaction::~MySQLTransaction() {
	// Safety net: without an explicit commit() or rollback(), the transaction would stay
	// open on the connection indefinitely.
	if (!finished && session.is_valid()) {
		if (!session->get_busy_error().is_empty()) {
			WARN_PRINT("MySQLTransaction: Destroyed without an explicit commit() or rollback() while the session is busy (an asynchronous operation or a streaming cursor). It cannot be rolled back now: it stays open, holding its locks, until the session runs COMMIT or ROLLBACK or the connection closes.");
			return;
		}
		WARN_PRINT("MySQLTransaction: Destroyed without an explicit commit() or rollback(). Rolling back automatically.");
		session->run_control_statement("ROLLBACK");
	}
}

void MySQLTransaction::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_ok"), &MySQLTransaction::is_ok);
	ClassDB::bind_method(D_METHOD("get_error"), &MySQLTransaction::get_error);
	ClassDB::bind_method(D_METHOD("commit"), &MySQLTransaction::commit);
	ClassDB::bind_method(D_METHOD("rollback"), &MySQLTransaction::rollback);
}

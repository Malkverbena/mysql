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

Dictionary MySQLTransaction::commit() {
	if (!start_error.is_empty()) {
		return start_error;
	}
	if (finished) {
		return mysql_module::make_client_error_dict("MySQLTransaction: commit() called on a transaction that is already finished.");
	}
	finished = true;
	return session->run_control_statement("COMMIT");
}

Dictionary MySQLTransaction::rollback() {
	if (!start_error.is_empty()) {
		return start_error;
	}
	if (finished) {
		return mysql_module::make_client_error_dict("MySQLTransaction: rollback() called on a transaction that is already finished.");
	}
	finished = true;
	return session->run_control_statement("ROLLBACK");
}

MySQLTransaction::~MySQLTransaction() {
	// Safety net: without an explicit commit() or rollback(), the transaction would stay
	// open on the connection indefinitely.
	if (!finished && session.is_valid()) {
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

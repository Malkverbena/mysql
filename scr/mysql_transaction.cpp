// SPDX-License-Identifier: MIT
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

Dictionary MySQLTransaction::commit() {
	if (finished) {
		return mysql_module::make_client_error_dict("MySQLTransaction: commit() chamado numa transação já finalizada.");
	}
	finished = true;
	return session->run_control_statement("COMMIT");
}

Dictionary MySQLTransaction::rollback() {
	if (finished) {
		return mysql_module::make_client_error_dict("MySQLTransaction: rollback() chamado numa transação já finalizada.");
	}
	finished = true;
	return session->run_control_statement("ROLLBACK");
}

MySQLTransaction::~MySQLTransaction() {
	// Rede de segurança: sem commit()/rollback() explícito, a transação não fica
	// pendurada aberta na conexão indefinidamente.
	if (!finished && session.is_valid()) {
		WARN_PRINT("MySQLTransaction: destruída sem commit()/rollback() explícito — fazendo ROLLBACK automático.");
		session->run_control_statement("ROLLBACK");
	}
}

void MySQLTransaction::_bind_methods() {
	ClassDB::bind_method(D_METHOD("commit"), &MySQLTransaction::commit);
	ClassDB::bind_method(D_METHOD("rollback"), &MySQLTransaction::rollback);
}

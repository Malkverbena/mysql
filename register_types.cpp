// SPDX-License-Identifier: MIT
/* register_types.cpp */

#include "register_types.h"

#include "scr/mysql_async_operation.h"
#include "scr/mysql_config.h"
#include "scr/mysql_connection.h"
#include "scr/mysql_pool.h"
#include "scr/mysql_result.h"
#include "scr/mysql_session.h"
#include "scr/mysql_streaming_cursor.h"
#include "scr/mysql_transaction.h"
#include "scr/prepared_statement_cache.h"

#include "core/object/class_db.h"

// Reescrita em andamento (ver documentation/roadmap.md). As classes são registradas
// aqui à medida que forem implementadas, seguindo a ordem das fases do roadmap:
// MySQLConfig, MySQLSession, MySQLPool, MySQLTransaction, MySQLResult,
// MySQLStreamingCursor, MySQLAsyncOperation. MySQLConnection e PreparedStatementCache
// são internas e nunca são registradas aqui.

void initialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(MySQLConfig);
	GDREGISTER_CLASS(MySQLResult);
	GDREGISTER_CLASS(MySQLSession);
	GDREGISTER_CLASS(MySQLTransaction);
	GDREGISTER_CLASS(MySQLPool);
	GDREGISTER_CLASS(MySQLAsyncOperation);
	GDREGISTER_CLASS(MySQLStreamingCursor);
}

void uninitialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

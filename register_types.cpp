// SPDX-License-Identifier: MIT
/* register_types.cpp */

#include "register_types.h"

// Reescrita em andamento (ver documentation/roadmap.md). As classes são registradas
// aqui à medida que forem implementadas, seguindo a ordem das fases do roadmap:
// MySQLConfig, MySQLSession, MySQLPool, MySQLTransaction, MySQLResult,
// MySQLStreamingCursor, MySQLAsyncOperation. MySQLConnection e PreparedStatementCache
// são internas e nunca são registradas aqui.

void initialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

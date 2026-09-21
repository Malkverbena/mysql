// SPDX-License-Identifier: MIT
/* prepared_statement_cache.h */
#pragma once

// PreparedStatementCache — cache LRU de SQL -> handle de prepared statement, dentro de
// uma única MySQLSession (handles são por conexão). Invalidado ao reconectar. Sem
// lógica de execução própria. NÃO é exposta ao GDScript.
// Ver documentation/roadmap.md (Fase 4).

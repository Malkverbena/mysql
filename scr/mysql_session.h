// SPDX-License-Identifier: MIT
/* mysql_session.h */
#pragma once

// MySQLSession — API principal exposta ao GDScript: execute_text / execute_formatted /
// execute_prepared / execute_streaming / execute_script (se allow_sql_script_execution),
// begin_transaction(). Dona de uma MySQLConnection (própria, no caminho simples, ou
// emprestada por um MySQLPool) e do seu PreparedStatementCache. Não é thread-safe: uma
// Session por thread/fluxo de cada vez.
// Ver documentation/roadmap.md (Fases 2-5) e documentation/design-notes.md.

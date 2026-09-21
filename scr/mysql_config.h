// SPDX-License-Identifier: MIT
/* mysql_config.h */
#pragma once

// MySQLConfig — configuração centralizada e imutável depois de criada: transport_mode,
// TLS, tinyint1_mode, json_result_mode, allow_sql_script_execution, allow_multi_queries,
// timeouts/limites de resultado. Compartilhada por referência entre todas as
// MySQLConnection nascidas dela (inclusive dentro de um MySQLPool).
// Ver documentation/design-notes.md e documentation/roadmap.md (Fases 2-5).

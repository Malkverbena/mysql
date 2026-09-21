// SPDX-License-Identifier: MIT
/* mysql_result.h */
#pragma once

// MySQLResult — dado puro: metadata ordenada, linhas (Array, preserva colunas
// duplicadas), affected_rows/last_insert_id em 64 bits, navegação entre múltiplos
// resultsets. is_ok()/get_error() (Dictionary) em vez de estado de erro ambiente. Sem
// I/O próprio.
// Ver documentation/roadmap.md (Fase 3) e documentation/design-notes.md (BIGINT
// UNSIGNED acima de INT64_MAX -> String).

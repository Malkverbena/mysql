// SPDX-License-Identifier: MIT
/* mysql_streaming_cursor.h */
#pragma once

// MySQLStreamingCursor — leitura incremental (next_batch()/has_more()/close()) sobre
// execution_state/read_some_rows do Boost.MySQL; is_ok()/get_error() (Dictionary). Se
// fechada no meio, drena ou reseta a conexão (o protocolo exige o resultset drenado por
// completo). Só uma por conexão por vez.
// Ver documentation/roadmap.md (Fase 5).

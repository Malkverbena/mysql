// SPDX-License-Identifier: MIT
/* mysql_async_operation.h */
#pragma once

// MySQLAsyncOperation — RefCounted com sinal `completed`, devolvido por toda chamada
// async_*; carrega o MySQLResult/MySQLStreamingCursor ou o erro quando termina. Suporta
// cancelamento. Pensada pra uso com `await` no GDScript.
// Ver documentation/roadmap.md (Fase 5).

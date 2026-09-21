// SPDX-License-Identifier: MIT
/* mysql_connection.h */
#pragma once

// MySQLConnection — um único socket físico (any_connection do Boost.MySQL) e sua
// máquina de estados (NONE -> CONFIGURED -> CONNECTING -> CONNECTED -> CLOSING ->
// FAILED). Não é thread-safe: uso exclusivo de uma MySQLSession por vez. NÃO é exposta
// ao GDScript — é o recurso que MySQLSession possui sozinha (caminho simples) ou que
// MySQLPool gerencia em conjunto (caminho com pool).
// Ver documentation/roadmap.md (Fase 2).

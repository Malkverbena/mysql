// SPDX-License-Identifier: MIT
/* mysql_pool.h */
#pragma once

// MySQLPool — dona de N MySQLConnection; empresta uma por vez como MySQLSession
// (acquire()), nunca a mesma Connection pra duas leases ao mesmo tempo. É a única peça
// deste design pensada pra ser chamada de várias threads. Centraliza reconexão/retry.
// Ver documentation/roadmap.md (Fase 5).

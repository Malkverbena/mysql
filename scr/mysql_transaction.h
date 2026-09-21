// SPDX-License-Identifier: MIT
/* mysql_transaction.h */
#pragma once

// MySQLTransaction — commit()/rollback(); se destruída sem nenhum dos dois, faz
// rollback automático (rede de segurança, sem exceção). Criada por e não sobrevive à
// MySQLSession que a originou.
// Ver documentation/roadmap.md (Fase 4).

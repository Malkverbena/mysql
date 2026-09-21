// SPDX-License-Identifier: MIT
/* mysql_pool.h */
#pragma once

#include "mysql_config.h"
#include "mysql_session.h"

#include "core/object/ref_counted.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

class MySQLConnection;

// MySQLPool — dona de N MySQLConnection; empresta uma por vez como MySQLSession
// (acquire()), nunca a mesma Connection pra duas leases ao mesmo tempo. É a única peça
// deste design pensada pra ser chamada de várias threads (any_connection não é
// thread-safe por si só — ver comentário em mysql_connection.h). acquire() bloqueia se
// o pool já está no tamanho máximo e não há conexão livre.
//
// A Session devolvida por acquire() ainda não está conectada — quem chama decide quando
// (e se) chamar session.connect(), do mesmo jeito que no caminho sem pool.
//
// Ver documentation/roadmap.md (Fase 5).
class MySQLPool : public RefCounted {
	GDCLASS(MySQLPool, RefCounted);

	Ref<MySQLConfig> config;
	int max_size = 8;

	std::mutex mutex;
	std::condition_variable cv;
	std::vector<std::unique_ptr<MySQLConnection>> idle;
	int total_count = 0;

	void _release(std::unique_ptr<MySQLConnection> p_connection);

protected:
	static void _bind_methods();

public:
	MySQLPool() = default;

	void set_config(const Ref<MySQLConfig> &p_config);
	Ref<MySQLConfig> get_config() const { return config; }

	void set_max_size(int p_max_size);
	int get_max_size() const { return max_size; }

	// Bloqueia até haver uma conexão livre, se o pool já estiver no tamanho máximo.
	Ref<MySQLSession> acquire();
};

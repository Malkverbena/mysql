// SPDX-License-Identifier: MIT
/* mysql_pool.cpp */

#include "mysql_pool.h"

#include "core/object/class_db.h"

#include "mysql_connection.h"

void MySQLPool::set_config(const Ref<MySQLConfig> &p_config) {
	ERR_FAIL_COND_MSG(total_count > 0, "MySQLPool: a config não pode ser trocada depois que alguma conexão já foi criada.");
	ERR_FAIL_COND_MSG(p_config.is_null(), "MySQLPool: a config não pode ser nula.");
	config = p_config;
}

void MySQLPool::set_max_size(int p_max_size) {
	ERR_FAIL_COND_MSG(p_max_size < 1, "MySQLPool: max_size precisa ser pelo menos 1.");
	max_size = p_max_size;
}

void MySQLPool::_release(std::unique_ptr<MySQLConnection> p_connection) {
	{
		std::lock_guard<std::mutex> lock(mutex);
		idle.push_back(std::move(p_connection));
	}
	cv.notify_one();
}

Ref<MySQLSession> MySQLPool::acquire() {
	ERR_FAIL_COND_V_MSG(config.is_null(), Ref<MySQLSession>(), "MySQLPool: chame set_config() antes de acquire().");

	std::unique_ptr<MySQLConnection> conn;
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (!idle.empty()) {
			conn = std::move(idle.back());
			idle.pop_back();
		} else if (total_count < max_size) {
			total_count++;
			lock.unlock();
			conn = std::make_unique<MySQLConnection>(config);
		} else {
			cv.wait(lock, [this] { return !idle.empty(); });
			conn = std::move(idle.back());
			idle.pop_back();
		}
	}

	// self_ref mantém o Pool vivo enquanto a Session emprestada existir — a Session
	// devolve a conexão pra ele no destrutor (release_to_pool), então o Pool precisa
	// sobreviver até lá mesmo que o script solte a referência dele antes.
	Ref<MySQLPool> self_ref(this);
	return MySQLSession::create_pooled(config, std::move(conn), [self_ref](std::unique_ptr<MySQLConnection> p_conn) {
		self_ref->_release(std::move(p_conn));
	});
}

void MySQLPool::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_config", "config"), &MySQLPool::set_config);
	ClassDB::bind_method(D_METHOD("get_config"), &MySQLPool::get_config);

	ClassDB::bind_method(D_METHOD("set_max_size", "max_size"), &MySQLPool::set_max_size);
	ClassDB::bind_method(D_METHOD("get_max_size"), &MySQLPool::get_max_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_size"), "set_max_size", "get_max_size");

	ClassDB::bind_method(D_METHOD("acquire"), &MySQLPool::acquire);
}

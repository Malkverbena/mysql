/* mysql_pool.cpp */

#include "mysql_pool.h"

#include "mysql_connection.h"

#include "core/object/class_db.h"

MySQLPool::~MySQLPool() {
	// Every leased session holds a reference to the pool, so by now all connections
	// are idle.
	for (MySQLConnection *connection : idle) {
		memdelete(connection);
	}
}

void MySQLPool::set_config(const Ref<MySQLConfig> &p_config) {
	ERR_FAIL_COND_MSG(total_count > 0, "MySQLPool: The config cannot be changed after a connection has been created.");
	ERR_FAIL_COND_MSG(p_config.is_null(), "MySQLPool: The config cannot be null.");
	config = p_config;
}

void MySQLPool::set_max_size(int p_max_size) {
	ERR_FAIL_COND_MSG(p_max_size < 1, "MySQLPool: max_size must be at least 1.");
	max_size = p_max_size;
}

void MySQLPool::release(MySQLConnection *p_connection) {
	{
		MutexLock lock(mutex);
		idle.push_back(p_connection);
	}
	condition.notify_one();
}

Ref<MySQLSession> MySQLPool::acquire() {
	ERR_FAIL_COND_V_MSG(config.is_null(), Ref<MySQLSession>(), "MySQLPool: Call set_config() before acquire().");

	MySQLConnection *connection = nullptr;
	bool create_new = false;
	{
		MutexLock lock(mutex);
		if (idle.is_empty() && total_count < max_size) {
			total_count++;
			create_new = true;
		} else {
			while (idle.is_empty()) {
				condition.wait(lock);
			}
			connection = idle[idle.size() - 1];
			idle.remove_at(idle.size() - 1);
		}
	}

	if (create_new) {
		// Built outside the lock: creating the connection (TLS context included) is slow.
		connection = memnew(MySQLConnection(config));
	}

	// The session keeps a reference to the pool, so the pool outlives every leased
	// session even if the script drops its own reference first.
	return MySQLSession::create_pooled(config, connection, Ref<MySQLPool>(this));
}

void MySQLPool::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_config", "config"), &MySQLPool::set_config);
	ClassDB::bind_method(D_METHOD("get_config"), &MySQLPool::get_config);

	ClassDB::bind_method(D_METHOD("set_max_size", "max_size"), &MySQLPool::set_max_size);
	ClassDB::bind_method(D_METHOD("get_max_size"), &MySQLPool::get_max_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_size"), "set_max_size", "get_max_size");

	ClassDB::bind_method(D_METHOD("acquire"), &MySQLPool::acquire);
}

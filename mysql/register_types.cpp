/* register_types.cpp */

#include "register_types.h"
#include "mysql.h"

#ifdef GODOT4
	#include "core/object/class_db.h"
#else
	#include "core/class_db.h"
#endif


void register_mysql_types() {
	ClassDB::register_class<MySQL>();
}

void unregister_mysql_types() {
}


/* register_types.cpp */

#include "register_types.h"


#ifdef GODOT4
	#include "core/object/class_db.h"
#else
	#include "core/class_db.h"
#endif



#include "MySQL.h"

void register_MySQL_types() {
	ClassDB::register_class<MySQL>();
}

void unregister_MySQL_types() {
}


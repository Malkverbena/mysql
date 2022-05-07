/* register_types.cpp */

#include "register_types.h"
#include "mysql.h"


#ifdef GODOT4
#include "core/object/class_db.h"

void initialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(MySQL);
}

void uninitialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}


#else
#include "core/class_db.h"

void register_mysql_types() {
	ClassDB::register_class<MySQL>();
}

void unregister_mysql_types() {
}

#endif


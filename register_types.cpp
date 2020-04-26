/* register_types.cpp */

#include "register_types.h"
#include "core/class_db.h"
#include "MySQL.h"

void register_MySQL_types() { ClassDB::register_class<MySQL>(); }

void unregister_MySQL_types() {}


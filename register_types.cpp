/* register_types.cpp */

#include "register_types.h"

#include "core/object/class_db.h"
#include "mysql.h"

void initialize_mysql_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
            return;
    }
    ClassDB::register_class<MySQL>();
}

void uninitialize_mysql_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
            return;
    }

}

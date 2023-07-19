/* register_types.cpp MySQL */


#include "register_types.h"

#include "core/object/class_db.h"
//#include "scr/sql_error.h"
#include "scr/helpers.h"
#include "scr/sql_conn.h"
#include "scr/mysql.h"
#include "scr/sql_result.h"


void initialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<MySQL>();
	ClassDB::register_class<SqlResult>();
}

void uninitialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

}

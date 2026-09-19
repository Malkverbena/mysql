
/* register_types.cpp */


#include "register_types.h"

#include "scr/constants.h"
#include "scr/helpers.h"
#include "scr/sql_result.h"
#include "scr/mysql.h"


void initialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(MySQL);
	GDREGISTER_CLASS(SqlResult);
}


void uninitialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

}

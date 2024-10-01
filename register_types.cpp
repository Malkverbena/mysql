/* register_types.cpp */


#include "register_types.h"



#include "scr/mysql.h"
//#include "scr/sqlconnection.h"
//#include "scr/sqlcertificate.h"
//#include "scr/helpers.h"



void initialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

//	GDREGISTER_CLASS(MySQL);
//	GDREGISTER_CLASS(SqlResult);
	GDREGISTER_CLASS(SqlConnection);
	GDREGISTER_CLASS(SqlCertificate);

}


void uninitialize_mysql_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

}

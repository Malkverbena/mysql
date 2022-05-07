/* register_types.h */

void register_mysql_types();
void unregister_mysql_types();



#ifndef MYSQL_REGISTER_TYPES_H
#define MYSQL_REGISTER_TYPES_H

#include "modules/register_module_types.h"

#ifndef GODOT4

void initialize_mysql_module(ModuleInitializationLevel p_level);
void uninitialize_mysql_module(ModuleInitializationLevel p_level);

#else

void register_mysql_types();
void unregister_mysql_types();

#endif



#endif // MYSQL_REGISTER_TYPES_H





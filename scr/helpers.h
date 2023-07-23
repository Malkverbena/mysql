/* helpers.h */

#ifndef 	HELPERS_H
#define HELPERS_H


#include "sql_error.h"


static const PackedStringArray param_names = {String("connection_collation"), String("ssl"), String("database"), String("multi_queries"), String("password"), String("username")};


const char * gdt2char(String str);


boost::mysql::string_view char2sql(String str);


char* copy_string(char s[]);




Variant get_array_kind(column_type type, Array p_array);


bool is_json(String p_arg);

Variant field2Var(const field_view fv);


String char2gdt(const char * s);


void SQLException(const boost::mysql::error_with_diagnostics &err);
void _runtime_error(const std::exception& err);



#endif  // HELPERS_H








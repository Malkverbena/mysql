/* helpers.h */

#ifndef 	HELPERS_H
#define HELPERS_H


#include "core/object/ref_counted.h"  
#include "core/core_bind.h"
#include "core/io/json.h"

#include <boost/mysql.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/system/system_error.hpp>
#include <boost/mysql/ssl_mode.hpp>
#include <boost/mysql/connection.hpp>
#include <boost/mysql/string_view.hpp>
#include <boost/mysql/buffer_params.hpp>
#include <boost/mysql/handshake_params.hpp>

#include <iostream>
#include <memory>
#include <map>
#include <string.h>


using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::mysql;


static const PackedStringArray param_names = {String("connection_collation"), String("ssl"), String("database"), String("multi_queries"), String("password"), String("username")};


const char * gdt2char(String str);


boost::mysql::string_view char2sql(String str);


char* copy_string(char s[]);




Variant get_array_kind(column_type type, Array p_array);


bool is_json(String p_arg);

Variant field2Var(const field_view fv);


String char2gdt(const char * s);

const char * sqlstr2gdt(boost::mysql::string_view s);


#endif  // HELPERS_H








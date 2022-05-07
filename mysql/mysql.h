/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H

#ifdef GODOT4
	#include "core/object/ref_counted.h"
	#include "core/core_bind.h"
#else
	#include "core/reference.h"
	#include "core/bind/core_bind.h"
#endif

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <mysql_error.h>

#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/metadata.h>
#include <cppconn/parameter_metadata.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/statement.h>

//#include <cppconn/datatype.h>

//#include <cppconn/sqlstring.h>
//#include <cppconn/warning.h>
//#include <cppconn/variant.h>


//#include <cppconn/build_config.h>
//#include <cppconn/version_info.h>
//#include <cppconn/config.h>


#pragma once

#ifdef GODOT4
class MySQL : public RefCounted{
	GDCLASS(MySQL, RefCounted);

#else
class MySQL : public Reference{
	GDCLASS(MySQL, Reference);
#endif

typedef Dictionary MySQLException;


protected:
	static void _bind_methods();


private:


public:


	MySQL();
	~MySQL();

};


#endif
// MYSQL_H





//---------------------------------------------------------------------------------------------
/*
https://dev.mysql.com/doc/refman/8.0/en/blob.html  NOTE: Afeta o desempenho
https://mariadb.com/kb/en/json-data-type/
*/

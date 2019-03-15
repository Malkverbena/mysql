/* register_types.cpp */

#include "register_types.h"
#include "core/class_db.h"
#include "MySQL.h"

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

void register_MySQL_types() {

        ClassDB::register_class<MySQL>();
}

void unregister_MySQL_types() {
   //nothing to do here
}


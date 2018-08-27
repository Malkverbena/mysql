/* mysql.h */
#ifndef MYSQL_H
#define MYSQL_H

#include <string>
#include "reference.h"

#include <mysql_error.h>
#include <mysql_driver.h>
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>


class MySQL : public Reference {
	GDCLASS(MySQL,Reference);


private:

	String sql2String(sql::SQLString r);

protected:

	static void _bind_methods();

public:

	void set_credentials(String shost, String suser, String spass);
	void select_database(String db);
	void query(String s);

	Array fetch_dictinary(String q);
	Array fetch_dictinary_string(String q);
	Array fetch_array_string(String m);
	Array fetch_array(String m);

	Array get_columns_name(String p);
	Array get_column_types(String l);

	MySQL();
	
};

#endif	// MYSQL_H

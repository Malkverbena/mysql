/* mysql.h */
#ifndef MYSQL_H
#define MYSQL_H

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

	String sql2String(sql::SQLString p_str);
	Array sql_tasks(String p_sql_query, const int task);
	
protected:

	static void _bind_methods();

public:

	void set_credentials(String p_host, String p_user, String p_pass);
	void select_database(String p_database);
	void query(String p_sql_query);

	Array fetch_dictinary(String p_sql_query);
	Array fetch_dictinary_string(String p_sql_query);
	Array fetch_array_string(String p_sql_query);
	Array fetch_array(String p_sql_query);

	Array get_columns_name(String p_sql_query);
	Array get_column_types(String p_sql_query);

	MySQL();
	
};

#endif	// MYSQL_H

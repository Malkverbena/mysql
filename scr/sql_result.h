/* sql_result.h */

#ifndef SQLRESULT_H
#define SQLRESULT_H

#include "sql_error.h"  
#pragma once

class SqlResult : public RefCounted {
	GDCLASS(SqlResult, RefCounted);

friend class MySQL;

private:

	int affected_rows;
	int last_insert_id;
	int warning_count;

	String info;
	
//	boost::mysql::results raw;

	Dictionary result;
	Dictionary meta;

	
protected:
	static void _bind_methods();


public:

	
	Array get_array();													// Retrieves the query content as an array.
	Dictionary get_dictionary(){return result;}					// Retrieves the query content as an dictionary.

	Variant get_row(int row, bool as_array = true);				// Retrieves a specific row.
	Array get_column(String column, bool as_array = true);	// Retrieves a specific column by searching for the column name.
// *OVERLOAD
//	Array get_column(int column, bool as_string = false);		// Retrieves a specific column by searching for the column number.

	Dictionary get_metadata();											// Returns metadata from a specific column.
	int get_affected_rows() const {return affected_rows;}
	int get_last_insert_id() const {return last_insert_id;}
	int get_warning_count() const {return warning_count;}


	SqlResult(){}
	~SqlResult(){}
};


#endif // SQLRESULT_H



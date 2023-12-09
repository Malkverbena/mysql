/* sql_result.h */

#ifndef SQLRESULT_H
#define SQLRESULT_H

#include "core/object/ref_counted.h"
#include "core/core_bind.h"



class SqlResult : public RefCounted {
	GDCLASS(SqlResult, RefCounted);

friend class MySQL;


private:

	Dictionary result;
	Dictionary meta;
	int affected_rows;
	int last_insert_id;
	int warning_count;


protected:

	static void _bind_methods();


public:

	// Retrieve the query content as an array.
	Array get_array() const;

	// Retrieves a specific row.
	Variant get_row(int row, bool as_array = true) const;

	// Retrieves a specific column by searching for the column name.
	Array get_column(String column, bool as_array = true) const;

	// Retrieves a specific column by searching for the column number.
	Array get_column_by_id(int column, bool as_string = false) const;

	// Returns metadata from a specific column.
	Dictionary get_metadata() const {return meta;};

	// Retrieves the query content as an dictionary.
	Dictionary get_dictionary() const {return result;};

	// Retrieves the number of affected rows by the query.
	int get_affected_rows() const {return affected_rows;};

	// Retrieves the last insert id by the query.
	int get_last_insert_id() const {return last_insert_id;};

	// Retrieves the number of warnings.
	int get_warning_count() const {return warning_count;};

	SqlResult();
	~SqlResult();

};


#endif // SQLRESULT_H
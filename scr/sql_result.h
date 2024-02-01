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

	Array get_array() const;

	Variant get_row(int row, bool as_array = true) const;

	Array get_column(String column, bool as_array = true) const;

	Array get_column_by_id(int column, bool as_array = true) const;

	Dictionary get_metadata() const {return meta;};

	Dictionary get_dictionary() const {return result;};

	int get_affected_rows() const {return affected_rows;};

	int get_last_insert_id() const {return last_insert_id;};

	int get_warning_count() const {return warning_count;};

	SqlResult();
	~SqlResult();

};


#endif // SQLRESULT_H
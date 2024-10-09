/* sqlresult.h */

#ifndef SQLRESULT_H
#define SQLRESULT_H


#include "core/object/ref_counted.h"
#include "core/core_bind.h"


class SqlResult : public RefCounted {
	GDCLASS(SqlResult, RefCounted);

friend class MySQL;


private:

	Dictionary sql_exception;
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
	Dictionary get_metadata() const ;
	Dictionary get_dictionary() const ;
	int get_affected_rows() const ;
	int get_last_insert_id() const ;
	int get_warning_count() const ;

	SqlResult();
	~SqlResult();

};



#endif // SQLRESULT_H

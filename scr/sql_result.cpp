/* sql_result.cpp */



#include "sql_result.h"


Array SqlResult::get_array() const {
	Array ret;
	for (int col = 0; col < result.size(); col++) {
		Dictionary dic_row = result[col];
		Array row = dic_row.values();
		ret.push_back(row);
	}
	return ret;
}


Variant SqlResult::get_row(int row, bool as_array) const {
	if(as_array) {
		Array ret = Array();
		Dictionary line = result[row];
		for (int col = 0; col < line.size(); col++) {
			ret.push_back(line[col]);
		}
		return ret;
	}
	return result[row];
}


Array SqlResult::get_column(String column, bool as_array) const {
	Array ret;
	for (int col = 0; col < result.size(); col++) {
		Dictionary dic_row = result[col];
		if (as_array) {
			ret.push_back(dic_row[column]);
		}
		else {
			Dictionary b = Dictionary();
			b[dic_row.keys()[col]]=dic_row[column];
			ret.push_back(b);
		}
	}
	return ret;
}


Array SqlResult::get_column_by_id(int column, bool as_array) const {
	Array ret;
	for (int col = 0; col < result.size(); col++) {
		Dictionary dic_row = result[col];
		if (as_array) {
			ret.push_back(dic_row[column]);
		}
		else {
			Dictionary b = Dictionary();
			b[dic_row.keys()[col]]=dic_row[column];
			ret.push_back(b);
		}
	}
	return ret;
}


void SqlResult::_bind_methods() {

	/* ===== QUERY ===== */
	ClassDB::bind_method(D_METHOD("get_array"), &SqlResult::get_array);
	ClassDB::bind_method(D_METHOD("get_dictionary"), &SqlResult::get_dictionary);
	ClassDB::bind_method(D_METHOD("get_row", "row", "as_array"), &SqlResult::get_row, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_column", "column", "as_array"), &SqlResult::get_column, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_column_by_id", "column", "as_array"), &SqlResult::get_column);

	/* ===== META ===== */
	ClassDB::bind_method(D_METHOD("get_metadata"), &SqlResult::get_metadata);
	ClassDB::bind_method(D_METHOD("get_affected_rows"), &SqlResult::get_affected_rows);
	ClassDB::bind_method(D_METHOD("get_last_insert_id"), &SqlResult::get_last_insert_id);
	ClassDB::bind_method(D_METHOD("get_warning_count"), &SqlResult::get_warning_count);

}

SqlResult::SqlResult(){
}


SqlResult::~SqlResult(){
}


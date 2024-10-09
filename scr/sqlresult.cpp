/* sqlresult.cpp */


#include "sqlresult.h"

std::mutex connection_mutex;



Dictionary  SqlResult::get_metadata() const {
	std::lock_guard<std::mutex> lock(connection_mutex);
	return meta;
}
Dictionary  SqlResult::get_dictionary() const {
	std::lock_guard<std::mutex> lock(connection_mutex);
	return result;
}
int  SqlResult::get_affected_rows() const {
	std::lock_guard<std::mutex> lock(connection_mutex);
	return affected_rows;
}
int  SqlResult::get_last_insert_id() const {
	std::lock_guard<std::mutex> lock(connection_mutex);
	return last_insert_id;
}
int  SqlResult::get_warning_count() const {
	std::lock_guard<std::mutex> lock(connection_mutex);
	return warning_count;
}


Array SqlResult::get_array() const {
	std::lock_guard<std::mutex> lock(connection_mutex);
	Array ret;
	for (int col = 0; col < result.size(); col++) {
		Dictionary dic_row = result[col];
		Array row = dic_row.values();
		ret.push_back(row);
	}
	return ret;
}


Variant SqlResult::get_row(int row, bool as_array) const {
	std::lock_guard<std::mutex> lock(connection_mutex);
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
	std::lock_guard<std::mutex> lock(connection_mutex);
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
	std::lock_guard<std::mutex> lock(connection_mutex);
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
	// ===== QUERY =====
	ClassDB::bind_method(D_METHOD("get_array"), &SqlResult::get_array);
	ClassDB::bind_method(D_METHOD("get_dictionary"), &SqlResult::get_dictionary);
	ClassDB::bind_method(D_METHOD("get_row", "row", "as_array"), &SqlResult::get_row, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_column", "column", "as_array"), &SqlResult::get_column, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_column_by_id", "column", "as_array"), &SqlResult::get_column, DEFVAL(true));
	// ===== META =====
	ClassDB::bind_method(D_METHOD("get_metadata"), &SqlResult::get_metadata);
	ClassDB::bind_method(D_METHOD("get_affected_rows"), &SqlResult::get_affected_rows);
	ClassDB::bind_method(D_METHOD("get_last_insert_id"), &SqlResult::get_last_insert_id);
	ClassDB::bind_method(D_METHOD("get_warning_count"), &SqlResult::get_warning_count);
}


SqlResult::SqlResult(){

}


SqlResult::~SqlResult(){

}


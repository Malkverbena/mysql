/* sql_result.cpp */


#include "sql_result.h"


Array SqlResult::get_array(){
	return Array();
}



Dictionary SqlResult::get_dictionary(){
/*
	Dictionary ret;
	for (int col = 0; col < result.size(); col++){

		Array row = result[col];

		String column_name = String("column_name");
		String col_name = meta[col].get(column_name);
		Dictionary line;
		

		for (int row = 0; row < c.size(); row++ ){
			Array s = c;
			line[col_name] = s[row];
		}
		ret[col_name] = line;
	}
*/

	



	return result;
}



Array SqlResult::get_column(String column){
	Array ret;
	
	
//	for (int column = 0; column < result.size(); column++){
	
	
	
	
	
	return ret;
}



Variant SqlResult::get_row(bool as_dictionary){
	Variant ret;
	return ret;
}






/*
Array SqlResult::get_column(int column, bool as_string){
	Array ret;
	return ret;
}
*/


Dictionary SqlResult::get_metadata(){
	return meta;
}



void SqlResult::_bind_methods() {

	/* ===== QUERY ===== */
	ClassDB::bind_method(D_METHOD("get_array"), &SqlResult::get_array);
	ClassDB::bind_method(D_METHOD("get_dictionary"), &SqlResult::get_dictionary);
	ClassDB::bind_method(D_METHOD("get_row", "as_array"), &SqlResult::get_row, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_column", "column"), &SqlResult::get_column);
//	ClassDB::bind_method(D_METHOD("get_column", "column"), &SqlResult::get_column);

	/* ===== META ===== */
	ClassDB::bind_method(D_METHOD("get_metadata"), &SqlResult::get_metadata);
	ClassDB::bind_method(D_METHOD("get_affected_rows"), &SqlResult::get_affected_rows);
	ClassDB::bind_method(D_METHOD("get_last_insert_id"), &SqlResult::get_last_insert_id);
	ClassDB::bind_method(D_METHOD("get_warning_count"), &SqlResult::get_warning_count);
	
}


/*
SqlResult::SqlResult(){
}



SqlResult::~SqlResult(){
}

*/

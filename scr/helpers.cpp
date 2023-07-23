/* helpers.cpp */


#include "sql_conn.h"


String char2gdt(const char * s){
    String str = String::utf8((char *)s);
    return str;
}


const char * gdt2char(String p_str){
	String str = p_str;
	const char * p_char = str.utf8().get_data();
	return p_char;
}


boost::mysql::string_view char2sql(String str){
	const char * p_char = gdt2char(str);
	boost::mysql::string_view ret = p_char;
	return ret;
}


char* copy_string(char s[]){
	 char* s2;
	 s2 = (char*)malloc(20);
	 strcpy(s2, s);
	 return (char*)s2;
}


/*
bool is_json(String p_arg ){
	JSON json;
	return (json.parse(p_arg) == OK);
}
*/


Variant field2Var(const field_view fv){

	if (fv.is_null()){
		Variant n = Variant();
		return n;
	}

	else if ((column_type)fv.kind() == column_type::tinyint){
		bool b = (bool)fv.get_int64();
		return b;
	}

	else if (fv.is_int64()){
		int64_t i = fv.as_int64();
		return i;
	}

	else if (fv.is_uint64()){
		uint64_t ui = fv.get_uint64();
		return ui;
	}

	else if (fv.is_float()){
		float f = fv.get_float();
		return f;
	}

	else if (fv.is_double()){
		double d = fv.get_double();
		return d;
	}

	else if (fv.is_string()){
		String s;
		s = String(fv.as_string().data());
		//return String(fv.as_string().data());
		return s;
	}

	else if (fv.is_blob()){
		blob_view bw = fv.get_blob();
		const unsigned char* b = bw.data();
		Vector<uint8_t> gdt_blob;
		gdt_blob.resize(bw.size());
		memcpy(gdt_blob.ptrw(), b, bw.size());
		return gdt_blob;
	}

	else if (fv.is_date()){
		Dictionary gdt_date;
		gdt_date["day"] = fv.get_date().day();
		gdt_date["month"] = fv.get_date().month();
		gdt_date["year"] = fv.get_date().year();
		return gdt_date;
	}

	else if (fv.is_datetime()){
		Dictionary gdt_datetime;
		gdt_datetime["day"] = fv.get_datetime().day();
		gdt_datetime["month"] = fv.get_datetime().month();
		gdt_datetime["year"] = fv.get_datetime().year();
		gdt_datetime["hour"] = fv.get_datetime().hour();
		gdt_datetime["minute"] = fv.get_datetime().minute();
		gdt_datetime["second"] = fv.get_datetime().second();
		gdt_datetime["microsecond"] = fv.get_datetime().microsecond();
		return gdt_datetime;
	}

	else if (fv.is_time()){
		// TODO: Conversão para milisegundos checar se godot usa mesmo milisegundos.
		int64_t t = fv.get_time().count();
		return t;
	}

	return Variant();
}


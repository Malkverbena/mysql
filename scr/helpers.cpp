/* helpers.cpp */


#include "helpers.h"



char* copy_string(char s[]) {
	 char* s2;
	 s2 = (char*)malloc(20);
	 strcpy(s2, s);
	 return (char*)s2;
}


String SqlStr2GdtStr(mysql::string_view s) {
	String str(s.data(), s.size());
	return str;
}


mysql::string_view GdtStr2SqlStr(String s) {
	boost::mysql::string_view str(s.utf8().get_data());
	return str;
}


void boost_dictionary(Dictionary *dic, const char *p_function, const char *p_file, int p_line, const mysql::error_code ec) {
	dic->clear();
	(*dic)["FILE"] = String(p_file);
	(*dic)["LINE"] = p_line;
	(*dic)["FUNCTION"] = String(p_function);
	(*dic)["ERROR"] = ec.value();
	(*dic)["DESCRIPTION"] = ec.what().data();
}


void sql_dictionary(Dictionary *dic, const char *p_function, const char *p_file, int p_line, const mysql::error_with_diagnostics diags) {
	dic->clear();
	(*dic)["FILE"] = String(p_file);
	(*dic)["LINE"] = p_line;
	(*dic)["FUNCTION"] = String(p_function);
	(*dic)["ERROR"] = diags.code().value();
	(*dic)["DESCRIPTION"] = String(diags.what()); 
	(*dic)["SERVER_MESSAGE"] = diags.get_diagnostics().server_message().data();
	(*dic)["CLIENT_MESSAGE"] = diags.get_diagnostics().client_message().data();
}


void print_std_exception(const char *p_function, const char *p_file, int p_line, std::exception err) {
	String exc = \
	String("# STD Exception Caught!\n") + \
	vformat("# ERR: Exception in: %s", p_file) + \
	vformat(" in function: %s", p_function) + \
	vformat("() on line %s\n", p_line)+\
	vformat("# ERR: %s\n", err.what());
	ERR_PRINT(exc);
}



void print_boost_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec) {
	String exc = \
	String("# BOOST Error Caught!\n") + \
	vformat("# ERR: Exception in: %s", p_file) + \
	vformat(" in function: %s", p_function) + \
	vformat("() on line %s\n", p_line)+\
	vformat("# ERR: %s\n", ec.value());
	ERR_PRINT(exc);
}


void print_sql_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_with_diagnostics diags) {
	String exc = \
	String("# SQL EXCEPTION Caught!\n") + \
	vformat("# ERR: SQLException in: %s", p_file) + vformat(" in function: %s", p_function) + vformat("() on line %s\n", p_line) +\
	vformat("# ERR: Code: %s\n", diags.code().value()) +\
	vformat("# ERR: Description: %s\n", String(diags.what())) +\
	vformat("# Server error: ", diags.get_diagnostics().server_message().data()) +\
	vformat("# Client Error: ", diags.get_diagnostics().client_message().data());
	ERR_PRINT(exc);
}


bool is_date(Dictionary d) {
	if (d.has("day") and d["day"].get_type() != Variant::INT) {return true;}
	if (d.has("month") and d["month"].get_type() != Variant::INT) {return true;}
	if (d.has("year") and d["year"].get_type() != Variant::INT) {return true;}
	return false;
}


bool is_time(Dictionary t) {
	if (t.has("hour") and t["hour"].get_type() != Variant::INT) {return true;}
	if (t.has("minute") and t["minute"].get_type() != Variant::INT) {return true;}
	if (t.has("second") and t["second"].get_type() != Variant::INT) {return true;}
	if (t.has("microsecond") and t["microsecond"].get_type() != Variant::INT) {return true;}
	return false;
}


bool is_datetime(Dictionary dt) {
	if (not is_date(dt)) {return false;}
	if (not is_time(dt)) {return false;}
	return true;
}


std::vector<mysql::field> binds_to_field(const Array arguments) {

	std::vector<mysql::field> ret;

	for (int arg = 0; arg < arguments.size(); arg++) {
		mysql::field a_field;
		int type = arguments[arg].get_type();

		if (type == Variant::NIL) {
			a_field = nullptr;
		}
		else if (type == Variant::BOOL) {
			bool val = arguments[arg];
			a_field = val;
		}
		else if (type == Variant::INT) {
			long int val = arguments[arg];
			a_field = val;
		}
		else if (type == Variant::FLOAT) {
			double val = arguments[arg];
			a_field = val;
		}
		else if (type == Variant::STRING) {
			String str = arguments[arg];
			const char * val =str.utf8().get_data();
			a_field = val;
		}
		else if (type == Variant::PACKED_BYTE_ARRAY) {
			Vector<uint8_t> input = arguments[arg];
			mysql::blob val;
			for (int p = 0; p < input.size(); p++) {
				val[p] = (unsigned char)input[p];
			}
			a_field = val;
		}

		else if (type == Variant::DICTIONARY) {
			Dictionary ts = arguments[arg];
			if (is_datetime(arguments[arg])) {
				uint16_t year			= ts.has("year")		? (uint16_t)ts["year"] : 0;
				uint8_t month			= ts.has("month")		? (uint8_t)ts["month"] : 0;
				uint8_t day				= ts.has("day")			? (uint8_t)ts["day"] : 0;
				uint16_t hour			= ts.has("hour")		? (uint16_t)ts["hour"] : 0;
				uint16_t minute			= ts.has("minute")		? (uint16_t)ts["minute"] : 0;
				uint16_t second			= ts.has("second")		? (uint16_t)ts["second"] : 0;
				uint32_t microsecond	= ts.has("microsecond")	? (uint32_t)ts["microsecond"] : 0;
				mysql::datetime val(year, month, day, hour, minute, second, microsecond);
				a_field = val;
			}
			else if(is_date(arguments[arg])) {
				uint16_t year	= ts.has("year")	? (uint16_t)ts["year"] : 0;
				uint8_t month	= ts.has("month")	? (uint8_t)ts["month"] : 0;
				uint8_t day		= ts.has("day")		? (uint8_t)ts["day"] : 0;
				mysql::date val(year, month, day);
				a_field = val;
			}
			else if (is_time(arguments[arg])) {
				int hour			= ts.has("hour")		? (int)ts["hour"] : 0;
				int minute			= ts.has("minute")		? (int)ts["minute"] : 0;
				int second			= ts.has("second")		? (int)ts["second"] : 0;
				int microsecond		= ts.has("microsecond")	? (int)ts["microsecond"] : 0;
				std::chrono::microseconds val = std::chrono::hours(hour) +
												std::chrono::minutes(minute) +
												std::chrono::seconds(second) +
												std::chrono::microseconds(microsecond);
				a_field = val;
			}
			//else{//Variant Geral}
		}	
		
		ret.push_back(a_field);

	}

	return ret;

}



Variant field2Var(const mysql::field_view fv) {

	if (fv.is_null()) {
		Variant n = Variant();
		return n;
	}

	else if ((mysql::column_type)fv.kind() == mysql::column_type::tinyint) {
		bool b = (bool)fv.get_int64();
		return b;
	}

	else if (fv.is_int64()) {
		int64_t i = fv.as_int64();
		return i;
	}

	else if (fv.is_uint64()) {
		uint64_t ui = fv.get_uint64();
		return ui;
	}

	else if (fv.is_float()) {
		float f = fv.get_float();
		return f;
	}

	else if (fv.is_double()) {
		double d = fv.get_double();
		return d;
	}

	else if (fv.is_string()) {
		String s;
		s = String(fv.as_string().data());
		return s;
	}

	else if (fv.is_blob()) {
		mysql::blob_view bw = fv.get_blob();
		const unsigned char* b = bw.data();
		Vector<uint8_t> gdt_blob;
		gdt_blob.resize(bw.size());
		memcpy(gdt_blob.ptrw(), b, bw.size());
		return gdt_blob;
	}

	else if (fv.is_date()) {
		Dictionary gdt_date;
		gdt_date["day"]		= fv.get_date().day();
		gdt_date["month"]	= fv.get_date().month();
		gdt_date["year"]	= fv.get_date().year();
		return gdt_date;
	}

	else if (fv.is_datetime()) {
		Dictionary gdt_datetime;
		gdt_datetime["day"] 		= fv.get_datetime().day();
		gdt_datetime["month"]		= fv.get_datetime().month();
		gdt_datetime["year"]		= fv.get_datetime().year();
		gdt_datetime["hour"]		= fv.get_datetime().hour();
		gdt_datetime["minute"]		= fv.get_datetime().minute();
		gdt_datetime["second"]		= fv.get_datetime().second();
		gdt_datetime["microsecond"]	= fv.get_datetime().microsecond();
		return gdt_datetime;
	}

	else if (fv.is_time()) {
		// TODO: Conversão para milisegundos checar se godot usa mesmo milisegundos.
		int64_t t = fv.get_time().count();
		return t;
	}

	return Variant();

}
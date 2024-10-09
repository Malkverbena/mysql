/* helpers.h */

#ifndef HELPERS_H
#define HELPERS_H


#include "core/object/ref_counted.h"
#include "core/core_bind.h"

#include <iostream>
#include <string_view>
#include <memory>
#include <mutex>
#include <thread>


#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio.hpp>


#include <core/config/project_settings.h>

#include <boost/mysql/ssl_mode.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/ssl/context_base.hpp>

#include <boost/mysql/mariadb_collations.hpp>
#include <boost/mysql/mysql_collations.hpp>

#include <boost/mysql/error_code.hpp>
#include <boost/mysql/error_with_diagnostics.hpp>

#include <boost/mysql/results.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/rows_view.hpp>
#include <boost/mysql/field.hpp>
#include <boost/mysql/time.hpp>
#include <boost/mysql/column_type.hpp>
#include <boost/mysql/metadata_collection_view.hpp>
#include <boost/mysql/metadata_mode.hpp>



#ifdef TOOLS_ENABLED
#define FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define FUNCTION_NAME __FUNCTION__
#endif


using namespace boost;
using namespace boost::mysql;


//using boost::mysql::with_diagnostics;



namespace sqlhelpers {


/*
Dictionary error_template = []() {
	Dictionary temp_dic;
	temp_dic["ERROR"] = OK;
	return temp_dic;
}();
*/




inline Dictionary ok_dictionary(){
	Dictionary ret;
	ret["ERROR"] = OK;
	return ret;
}


Dictionary boost_dictionary(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec);
Dictionary sql_dictionary(const char *p_function, const char *p_file, int p_line, const mysql::diagnostics diag, const mysql::error_code ec);
void print_boost_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec);
void print_sql_exception(const char *p_function, const char *p_file, int p_line, const mysql::diagnostics diag, const mysql::error_code ec);
void print_std_exception(const char *p_function, const char *p_file, int p_line, std::exception err);



boost::asio::const_buffer GDstring_to_SQLbuffer(const String &godot_string);


String ensure_global_path(String p_path);
char* copy_string(char s[]);
String SQLstring_to_GDstring(mysql::string_view s);
mysql::string_view GDstring_to_SQLstring(String s);


bool is_date(Dictionary d);
bool is_datetime(Dictionary dt);
bool is_time(Dictionary t);
std::vector<mysql::field> binds_to_field(const Array args);
Variant field2Var(const mysql::field_view fv, mysql::column_type column_type);
Dictionary make_metadata_result(mysql::metadata_collection_view meta_collection);
Dictionary make_raw_result(mysql::rows_view batch, mysql::metadata_collection_view meta_coll);



#define BOOST_EXCEPTION(m_errcode)															\
	if (unlikely(m_errcode)) {																\
		print_boost_exception(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);				\
		return boost_dictionary(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);				\
	} else	{																				\
		return ok_dictionary();																\
	}

#define CORO_SQL_EXCEPTION(m_errcode, m_diag)															\
	do {																								\
		if (unlikely(m_errcode)) {																		\
			print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);					\
			Dictionary m_dic = sql_dictionary(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
			co_return m_dic;																			\
		} else {																						\
			co_return ok_dictionary();																	\
		}																								\
	} while(0)


#define SQL_EXCEPTION(m_errcode, m_diag)															\
	if (unlikely(m_errcode)) {																		\
		print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);					\
		return sql_dictionary(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);				\
	} else																							\
		return ok_dictionary();



/*
#define RETURN_NO_ERROR return error_template.duplicate(true);


#define BOOST_EXCEPTION(m_errcode)												\
	Dictionary m_dic;															\
	m_dic["ERROR"] = OK;														\
	if (unlikely(m_errcode)) {													\
		boost_dictionary(&m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		print_boost_exception(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		return m_dic;															\
	} else	{																	\
		return m_dic;															\
	}


#define CORO_SQL_EXCEPTION(m_errcode, m_diag)												\
	do {																					\
		Dictionary m_dic;																	\
		m_dic["ERROR"] = OK;																\
		if (unlikely(m_errcode)) {															\
			sql_dictionary(&m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
			print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);		\
			co_return m_dic;																\
		} else {																			\
			co_return error_template.duplicate(true);										\
		}																					\
	} while(0)




#define CORO_SQL_EXCEPTION_WITH_RESULT(m_errcode, m_diag, m_result)							\
	do {																					\
		Dictionary m_dic;																	\
		m_dic["ERROR"] = OK;																\
		if (unlikely(m_errcode)) {															\
			sql_dictionary(&m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
			print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);		\
			co_return m_dic;																\
		} else {																			\
			co_return m_dic;																\
		}																					\
	} while(0)


#define SQL_EXCEPTION(m_errcode, m_diag)												\
	Dictionary m_dic;																	\
	m_dic["ERROR"] = OK;																\
	if (unlikely(m_errcode)) {															\
		sql_dictionary(&m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
		print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);		\
		return m_dic;																	\
	} else																				\
		return m_dic;


#define SQL_EXCEPTION_ON_RESULT(m_errcode, m_diag, m_result)							\
	Dictionary m_dic;																	\
	m_dic["ERROR"] = OK;																\
	if (unlikely(m_errcode)) {															\
		sql_dictionary(&m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
		print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);		\
		m_result->sql_exception = m_dic;												\
		return m_result;																\
	} else																				\
		((void)0)















#define BOOST_EXCEPTION(m_errcode, m_dic, m_ret)								\
	if (unlikely(m_errcode)) {													\
		boost_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		print_boost_exception(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		return (Error)-m_errcode.value();										\
	} else																		\
		((void)0)



#define BOOST_EXCEPTION_RETURN(m_errcode)										\
	Dictionary m_dic = error_template.duplicate(true);							\
	if (unlikely(m_errcode)) {													\
		boost_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		print_boost_exception(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		return m_dic;															\
	} else	{																	\
		return m_dic;															\
	}




#define CORO_BOOST_EXCEPTION_RETURN(m_errcode, m_ret)								\
	Dictionary m_dic = error_template.duplicate(true);							\
	if (unlikely(m_errcode)) {													\
		boost_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		print_boost_exception(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		co_return m_dic;															\
	} else	{																	\
		co_return m_dic;															\
	}


#define SQL_EXCEPTION(m_errcode, m_diag, m_dic, m_ret)								\
	if (unlikely(m_errcode)) {														\
		sql_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);\
		print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
		return m_ret;																\
	} else																			\
		((void)0)



#define CORO_SQL_EXCEPTION(m_errcode, m_diag, m_dic, m_ret)							\
	if (unlikely(m_errcode)) {														\
		sql_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);\
		print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
		co_return m_ret;
	} else	{																		\
		((void)0)
	}





#define CORO_SQL_EXCEPTION(m_errcode, m_diag, m_dic, m_ret)									\
	do {																					\
		if (unlikely(m_errcode)) {															\
			sql_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
			print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);		\
			co_return m_ret;																\
		} else {																			\
			((void)0);																		\
		}																					\
	} while(0)







#define CORO_SQL_EXCEPTION_VOID(m_errcode, m_diag, m_dic)							\
	if (unlikely(m_errcode)) {														\
		sql_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);\
		print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
		co_return;																	\
	} else																			\
		((void)0)


*/

}


enum MYSQLCOLLATIONS {
	default_collation			= mysql::mysql_collations::utf8mb4_general_ci, // 45
	big5_chinese_ci				= mysql::mysql_collations::big5_chinese_ci,
	latin2_czech_cs				= mysql::mysql_collations::latin2_czech_cs,
	dec8_swedish_ci				= mysql::mysql_collations::dec8_swedish_ci,
	cp850_general_ci			= mysql::mysql_collations::cp850_general_ci,
	latin1_german1_ci			= mysql::mysql_collations::latin1_german1_ci,
	hp8_english_ci				= mysql::mysql_collations::hp8_english_ci,
	koi8r_general_ci			= mysql::mysql_collations::koi8r_general_ci,
	latin1_swedish_ci			= mysql::mysql_collations::latin1_swedish_ci,
	latin2_general_ci			= mysql::mysql_collations::latin2_general_ci,
	swe7_swedish_ci				= mysql::mysql_collations::swe7_swedish_ci,
	ascii_general_ci			= mysql::mysql_collations::ascii_general_ci,
	ujis_japanese_ci			= mysql::mysql_collations::ujis_japanese_ci,
	sjis_japanese_ci			= mysql::mysql_collations::sjis_japanese_ci,
	cp1251_bulgarian_ci			= mysql::mysql_collations::cp1251_bulgarian_ci,
	latin1_danish_ci			= mysql::mysql_collations::latin1_danish_ci,
	hebrew_general_ci			= mysql::mysql_collations::hebrew_general_ci,
	tis620_thai_ci				= mysql::mysql_collations::tis620_thai_ci,
	euckr_korean_ci				= mysql::mysql_collations::euckr_korean_ci,
	latin7_estonian_cs			= mysql::mysql_collations::latin7_estonian_cs,
	latin2_hungarian_ci			= mysql::mysql_collations::latin2_hungarian_ci,
	koi8u_general_ci			= mysql::mysql_collations::koi8u_general_ci,
	cp1251_ukrainian_ci			= mysql::mysql_collations::cp1251_ukrainian_ci,
	gb2312_chinese_ci			= mysql::mysql_collations::gb2312_chinese_ci,
	greek_general_ci			= mysql::mysql_collations::greek_general_ci,
	cp1250_general_ci			= mysql::mysql_collations::cp1250_general_ci,
	latin2_croatian_ci			= mysql::mysql_collations::latin2_croatian_ci,
	gbk_chinese_ci				= mysql::mysql_collations::gbk_chinese_ci,
	cp1257_lithuanian_ci		= mysql::mysql_collations::cp1257_lithuanian_ci,
	latin5_turkish_ci			= mysql::mysql_collations::latin5_turkish_ci,
	latin1_german2_ci			= mysql::mysql_collations::latin1_german2_ci,
	armscii8_general_ci			= mysql::mysql_collations::armscii8_general_ci,
	utf8_general_ci				= mysql::mysql_collations::utf8_general_ci,
	cp1250_czech_cs				= mysql::mysql_collations::cp1250_czech_cs,
	ucs2_general_ci				= mysql::mysql_collations::ucs2_general_ci,
	cp866_general_ci			= mysql::mysql_collations::cp866_general_ci,
	keybcs2_general_ci			= mysql::mysql_collations::keybcs2_general_ci,
	macce_general_ci			= mysql::mysql_collations::macce_general_ci,
	macroman_general_ci			= mysql::mysql_collations::macroman_general_ci,
	cp852_general_ci			= mysql::mysql_collations::cp852_general_ci,
	latin7_general_ci			= mysql::mysql_collations::latin7_general_ci,
	latin7_general_cs			= mysql::mysql_collations::latin7_general_cs,
	macce_bin					= mysql::mysql_collations::macce_bin,
	cp1250_croatian_ci			= mysql::mysql_collations::cp1250_croatian_ci,
	utf8mb4_general_ci			= mysql::mysql_collations::utf8mb4_general_ci,
	utf8mb4_bin					= mysql::mysql_collations::utf8mb4_bin,
	latin1_bin					= mysql::mysql_collations::latin1_bin,
	latin1_general_ci			= mysql::mysql_collations::latin1_general_ci,
	latin1_general_cs			= mysql::mysql_collations::latin1_general_cs,
	cp1251_bin					= mysql::mysql_collations::cp1251_bin,
	cp1251_general_ci			= mysql::mysql_collations::cp1251_general_ci,
	cp1251_general_cs			= mysql::mysql_collations::cp1251_general_cs,
	macroman_bin				= mysql::mysql_collations::macroman_bin,
	utf16_general_ci			= mysql::mysql_collations::utf16_general_ci,
	utf16_bin					= mysql::mysql_collations::utf16_bin,
	utf16le_general_ci			= mysql::mysql_collations::utf16le_general_ci,
	cp1256_general_ci			= mysql::mysql_collations::cp1256_general_ci,
	cp1257_bin					= mysql::mysql_collations::cp1257_bin,
	cp1257_general_ci			= mysql::mysql_collations::cp1257_general_ci,
	utf32_general_ci			= mysql::mysql_collations::utf32_general_ci,
	utf32_bin					= mysql::mysql_collations::utf32_bin,
	utf16le_bin					= mysql::mysql_collations::utf16le_bin,
	binary						= mysql::mysql_collations::binary,
	armscii8_bin				= mysql::mysql_collations::armscii8_bin,
	ascii_bin					= mysql::mysql_collations::ascii_bin,
	cp1250_bin					= mysql::mysql_collations::cp1250_bin,
	cp1256_bin					= mysql::mysql_collations::cp1256_bin,
	cp866_bin					= mysql::mysql_collations::cp866_bin,
	dec8_bin					= mysql::mysql_collations::dec8_bin,
	greek_bin					= mysql::mysql_collations::greek_bin,
	hebrew_bin					= mysql::mysql_collations::hebrew_bin,
	hp8_bin						= mysql::mysql_collations::hp8_bin,
	keybcs2_bin					= mysql::mysql_collations::keybcs2_bin,
	koi8r_bin					= mysql::mysql_collations::koi8r_bin,
	koi8u_bin					= mysql::mysql_collations::koi8u_bin,
	utf8_tolower_ci				= mysql::mysql_collations::utf8_tolower_ci,
	latin2_bin					= mysql::mysql_collations::latin2_bin,
	latin5_bin					= mysql::mysql_collations::latin5_bin,
	latin7_bin					= mysql::mysql_collations::latin7_bin,
	cp850_bin					= mysql::mysql_collations::cp850_bin,
	cp852_bin					= mysql::mysql_collations::cp852_bin,
	swe7_bin					= mysql::mysql_collations::swe7_bin,
	utf8_bin					= mysql::mysql_collations::utf8_bin,
	big5_bin					= mysql::mysql_collations::big5_bin,
	euckr_bin					= mysql::mysql_collations::euckr_bin,
	gb2312_bin					= mysql::mysql_collations::gb2312_bin,
	gbk_bin						= mysql::mysql_collations::gbk_bin,
	sjis_bin					= mysql::mysql_collations::sjis_bin,
	tis620_bin					= mysql::mysql_collations::tis620_bin,
	ucs2_bin					= mysql::mysql_collations::ucs2_bin,
	ujis_bin					= mysql::mysql_collations::ujis_bin,
	geostd8_general_ci			= mysql::mysql_collations::geostd8_general_ci,
	geostd8_bin					= mysql::mysql_collations::geostd8_bin,
	latin1_spanish_ci			= mysql::mysql_collations::latin1_spanish_ci,
	cp932_japanese_ci			= mysql::mysql_collations::cp932_japanese_ci,
	cp932_bin					= mysql::mysql_collations::cp932_bin,
	eucjpms_japanese_ci			= mysql::mysql_collations::eucjpms_japanese_ci,
	eucjpms_bin					= mysql::mysql_collations::eucjpms_bin,
	cp1250_polish_ci			= mysql::mysql_collations::cp1250_polish_ci,
	utf16_unicode_ci			= mysql::mysql_collations::utf16_unicode_ci,
	utf16_icelandic_ci			= mysql::mysql_collations::utf16_icelandic_ci,
	utf16_latvian_ci			= mysql::mysql_collations::utf16_latvian_ci,
	utf16_romanian_ci			= mysql::mysql_collations::utf16_romanian_ci,
	utf16_slovenian_ci			= mysql::mysql_collations::utf16_slovenian_ci,
	utf16_polish_ci				= mysql::mysql_collations::utf16_polish_ci,
	utf16_estonian_ci			= mysql::mysql_collations::utf16_estonian_ci,
	utf16_spanish_ci			= mysql::mysql_collations::utf16_spanish_ci,
	utf16_swedish_ci			= mysql::mysql_collations::utf16_swedish_ci,
	utf16_turkish_ci			= mysql::mysql_collations::utf16_turkish_ci,
	utf16_czech_ci				= mysql::mysql_collations::utf16_czech_ci,
	utf16_danish_ci				= mysql::mysql_collations::utf16_danish_ci,
	utf16_lithuanian_ci			= mysql::mysql_collations::utf16_lithuanian_ci,
	utf16_slovak_ci				= mysql::mysql_collations::utf16_slovak_ci,
	utf16_spanish2_ci			= mysql::mysql_collations::utf16_spanish2_ci,
	utf16_roman_ci				= mysql::mysql_collations::utf16_roman_ci,
	utf16_persian_ci			= mysql::mysql_collations::utf16_persian_ci,
	utf16_esperanto_ci			= mysql::mysql_collations::utf16_esperanto_ci,
	utf16_hungarian_ci			= mysql::mysql_collations::utf16_hungarian_ci,
	utf16_sinhala_ci			= mysql::mysql_collations::utf16_sinhala_ci,
	utf16_german2_ci			= mysql::mysql_collations::utf16_german2_ci,
	utf16_croatian_ci			= mysql::mysql_collations::utf16_croatian_ci,
	utf16_unicode_520_ci		= mysql::mysql_collations::utf16_unicode_520_ci,
	utf16_vietnamese_ci			= mysql::mysql_collations::utf16_vietnamese_ci,
	ucs2_unicode_ci				= mysql::mysql_collations::ucs2_unicode_ci,
	ucs2_icelandic_ci			= mysql::mysql_collations::ucs2_icelandic_ci,
	ucs2_latvian_ci				= mysql::mysql_collations::ucs2_latvian_ci,
	ucs2_romanian_ci			= mysql::mysql_collations::ucs2_romanian_ci,
	ucs2_slovenian_ci			= mysql::mysql_collations::ucs2_slovenian_ci,
	ucs2_polish_ci				= mysql::mysql_collations::ucs2_polish_ci,
	ucs2_estonian_ci			= mysql::mysql_collations::ucs2_estonian_ci,
	ucs2_spanish_ci				= mysql::mysql_collations::ucs2_spanish_ci,
	ucs2_swedish_ci				= mysql::mysql_collations::ucs2_swedish_ci,
	ucs2_turkish_ci				= mysql::mysql_collations::ucs2_turkish_ci,
	ucs2_czech_ci				= mysql::mysql_collations::ucs2_czech_ci,
	ucs2_danish_ci				= mysql::mysql_collations::ucs2_danish_ci,
	ucs2_lithuanian_ci			= mysql::mysql_collations::ucs2_lithuanian_ci,
	ucs2_slovak_ci				= mysql::mysql_collations::ucs2_slovak_ci,
	ucs2_spanish2_ci			= mysql::mysql_collations::ucs2_spanish2_ci,
	ucs2_roman_ci				= mysql::mysql_collations::ucs2_roman_ci,
	ucs2_persian_ci				= mysql::mysql_collations::ucs2_persian_ci,
	ucs2_esperanto_ci			= mysql::mysql_collations::ucs2_esperanto_ci,
	ucs2_hungarian_ci			= mysql::mysql_collations::ucs2_hungarian_ci,
	ucs2_sinhala_ci				= mysql::mysql_collations::ucs2_sinhala_ci,
	ucs2_german2_ci				= mysql::mysql_collations::ucs2_german2_ci,
	ucs2_croatian_ci			= mysql::mysql_collations::ucs2_croatian_ci,
	ucs2_unicode_520_ci			= mysql::mysql_collations::ucs2_unicode_520_ci,
	ucs2_vietnamese_ci			= mysql::mysql_collations::ucs2_vietnamese_ci,
	ucs2_general_mysql500_ci	= mysql::mysql_collations::ucs2_general_mysql500_ci,
	utf32_unicode_ci			= mysql::mysql_collations::utf32_unicode_ci,
	utf32_icelandic_ci			= mysql::mysql_collations::utf32_icelandic_ci,
	utf32_latvian_ci			= mysql::mysql_collations::utf32_latvian_ci,
	utf32_romanian_ci			= mysql::mysql_collations::utf32_romanian_ci,
	utf32_slovenian_ci			= mysql::mysql_collations::utf32_slovenian_ci,
	utf32_polish_ci				= mysql::mysql_collations::utf32_polish_ci,
	utf32_estonian_ci			= mysql::mysql_collations::utf32_estonian_ci,
	utf32_spanish_ci			= mysql::mysql_collations::utf32_spanish_ci,
	utf32_swedish_ci			= mysql::mysql_collations::utf32_swedish_ci,
	utf32_turkish_ci			= mysql::mysql_collations::utf32_turkish_ci,
	utf32_czech_ci				= mysql::mysql_collations::utf32_czech_ci,
	utf32_danish_ci				= mysql::mysql_collations::utf32_danish_ci,
	utf32_lithuanian_ci			= mysql::mysql_collations::utf32_lithuanian_ci,
	utf32_slovak_ci				= mysql::mysql_collations::utf32_slovak_ci,
	utf32_spanish2_ci			= mysql::mysql_collations::utf32_spanish2_ci,
	utf32_roman_ci				= mysql::mysql_collations::utf32_roman_ci,
	utf32_persian_ci			= mysql::mysql_collations::utf32_persian_ci,
	utf32_esperanto_ci			= mysql::mysql_collations::utf32_esperanto_ci,
	utf32_hungarian_ci			= mysql::mysql_collations::utf32_hungarian_ci,
	utf32_sinhala_ci			= mysql::mysql_collations::utf32_sinhala_ci,
	utf32_german2_ci			= mysql::mysql_collations::utf32_german2_ci,
	utf32_croatian_ci			= mysql::mysql_collations::utf32_croatian_ci,
	utf32_unicode_520_ci		= mysql::mysql_collations::utf32_unicode_520_ci,
	utf32_vietnamese_ci			= mysql::mysql_collations::utf32_vietnamese_ci,
	utf8_unicode_ci				= mysql::mysql_collations::utf8_unicode_ci,
	utf8_icelandic_ci			= mysql::mysql_collations::utf8_icelandic_ci,
	utf8_latvian_ci				= mysql::mysql_collations::utf8_latvian_ci,
	utf8_romanian_ci			= mysql::mysql_collations::utf8_romanian_ci,
	utf8_slovenian_ci			= mysql::mysql_collations::utf8_slovenian_ci,
	utf8_polish_ci				= mysql::mysql_collations::utf8_polish_ci,
	utf8_estonian_ci			= mysql::mysql_collations::utf8_estonian_ci,
	utf8_spanish_ci				= mysql::mysql_collations::utf8_spanish_ci,
	utf8_swedish_ci				= mysql::mysql_collations::utf8_swedish_ci,
	utf8_turkish_ci				= mysql::mysql_collations::utf8_turkish_ci,
	utf8_czech_ci				= mysql::mysql_collations::utf8_czech_ci,
	utf8_danish_ci				= mysql::mysql_collations::utf8_danish_ci,
	utf8_lithuanian_ci			= mysql::mysql_collations::utf8_lithuanian_ci,
	utf8_slovak_ci				= mysql::mysql_collations::utf8_slovak_ci,
	utf8_spanish2_ci			= mysql::mysql_collations::utf8_spanish2_ci,
	utf8_roman_ci				= mysql::mysql_collations::utf8_roman_ci,
	utf8_persian_ci				= mysql::mysql_collations::utf8_persian_ci,
	utf8_esperanto_ci			= mysql::mysql_collations::utf8_esperanto_ci,
	utf8_hungarian_ci			= mysql::mysql_collations::utf8_hungarian_ci,
	utf8_sinhala_ci				= mysql::mysql_collations::utf8_sinhala_ci,
	utf8_german2_ci				= mysql::mysql_collations::utf8_german2_ci,
	utf8_croatian_ci			= mysql::mysql_collations::utf8_croatian_ci,
	utf8_unicode_520_ci			= mysql::mysql_collations::utf8_unicode_520_ci,
	utf8_vietnamese_ci			= mysql::mysql_collations::utf8_vietnamese_ci,
	utf8_general_mysql500_ci	= mysql::mysql_collations::utf8_general_mysql500_ci,
	utf8mb4_unicode_ci			= mysql::mysql_collations::utf8mb4_unicode_ci,
	utf8mb4_icelandic_ci		= mysql::mysql_collations::utf8mb4_icelandic_ci,
	utf8mb4_latvian_ci			= mysql::mysql_collations::utf8mb4_latvian_ci,
	utf8mb4_romanian_ci			= mysql::mysql_collations::utf8mb4_romanian_ci,
	utf8mb4_slovenian_ci		= mysql::mysql_collations::utf8mb4_slovenian_ci,
	utf8mb4_polish_ci			= mysql::mysql_collations::utf8mb4_polish_ci,
	utf8mb4_estonian_ci			= mysql::mysql_collations::utf8mb4_estonian_ci,
	utf8mb4_spanish_ci			= mysql::mysql_collations::utf8mb4_spanish_ci,
	utf8mb4_swedish_ci			= mysql::mysql_collations::utf8mb4_swedish_ci,
	utf8mb4_turkish_ci			= mysql::mysql_collations::utf8mb4_turkish_ci,
	utf8mb4_czech_ci			= mysql::mysql_collations::utf8mb4_czech_ci,
	utf8mb4_danish_ci			= mysql::mysql_collations::utf8mb4_danish_ci,
	utf8mb4_lithuanian_ci		= mysql::mysql_collations::utf8mb4_lithuanian_ci,
	utf8mb4_slovak_ci			= mysql::mysql_collations::utf8mb4_slovak_ci,
	utf8mb4_spanish2_ci			= mysql::mysql_collations::utf8mb4_spanish2_ci,
	utf8mb4_roman_ci			= mysql::mysql_collations::utf8mb4_roman_ci,
	utf8mb4_persian_ci			= mysql::mysql_collations::utf8mb4_persian_ci,
	utf8mb4_esperanto_ci		= mysql::mysql_collations::utf8mb4_esperanto_ci,
	utf8mb4_hungarian_ci		= mysql::mysql_collations::utf8mb4_hungarian_ci,
	utf8mb4_sinhala_ci			= mysql::mysql_collations::utf8mb4_sinhala_ci,
	utf8mb4_german2_ci			= mysql::mysql_collations::utf8mb4_german2_ci,
	utf8mb4_croatian_ci			= mysql::mysql_collations::utf8mb4_croatian_ci,
	utf8mb4_unicode_520_ci		= mysql::mysql_collations::utf8mb4_unicode_520_ci,
	utf8mb4_vietnamese_ci		= mysql::mysql_collations::utf8mb4_vietnamese_ci,
	gb18030_chinese_ci			= mysql::mysql_collations::gb18030_chinese_ci,
	gb18030_bin					= mysql::mysql_collations::gb18030_bin,
	gb18030_unicode_520_ci		= mysql::mysql_collations::gb18030_unicode_520_ci,
	utf8mb4_0900_ai_ci			= mysql::mysql_collations::utf8mb4_0900_ai_ci,
	utf8mb4_de_pb_0900_ai_ci	= mysql::mysql_collations::utf8mb4_de_pb_0900_ai_ci,
	utf8mb4_is_0900_ai_ci		= mysql::mysql_collations::utf8mb4_is_0900_ai_ci,
	utf8mb4_lv_0900_ai_ci		= mysql::mysql_collations::utf8mb4_lv_0900_ai_ci,
	utf8mb4_ro_0900_ai_ci		= mysql::mysql_collations::utf8mb4_ro_0900_ai_ci,
	utf8mb4_sl_0900_ai_ci		= mysql::mysql_collations::utf8mb4_sl_0900_ai_ci,
	utf8mb4_pl_0900_ai_ci		= mysql::mysql_collations::utf8mb4_pl_0900_ai_ci,
	utf8mb4_et_0900_ai_ci		= mysql::mysql_collations::utf8mb4_et_0900_ai_ci,
	utf8mb4_es_0900_ai_ci		= mysql::mysql_collations::utf8mb4_es_0900_ai_ci,
	utf8mb4_sv_0900_ai_ci		= mysql::mysql_collations::utf8mb4_sv_0900_ai_ci,
	utf8mb4_tr_0900_ai_ci		= mysql::mysql_collations::utf8mb4_tr_0900_ai_ci,
	utf8mb4_cs_0900_ai_ci		= mysql::mysql_collations::utf8mb4_cs_0900_ai_ci,
	utf8mb4_da_0900_ai_ci		= mysql::mysql_collations::utf8mb4_da_0900_ai_ci,
	utf8mb4_lt_0900_ai_ci		= mysql::mysql_collations::utf8mb4_lt_0900_ai_ci,
	utf8mb4_sk_0900_ai_ci		= mysql::mysql_collations::utf8mb4_sk_0900_ai_ci,
	utf8mb4_es_trad_0900_ai_ci	= mysql::mysql_collations::utf8mb4_es_trad_0900_ai_ci,
	utf8mb4_la_0900_ai_ci		= mysql::mysql_collations::utf8mb4_la_0900_ai_ci,
	utf8mb4_eo_0900_ai_ci		= mysql::mysql_collations::utf8mb4_eo_0900_ai_ci,
	utf8mb4_hu_0900_ai_ci		= mysql::mysql_collations::utf8mb4_hu_0900_ai_ci,
	utf8mb4_hr_0900_ai_ci		= mysql::mysql_collations::utf8mb4_hr_0900_ai_ci,
	utf8mb4_vi_0900_ai_ci		= mysql::mysql_collations::utf8mb4_vi_0900_ai_ci,
	utf8mb4_0900_as_cs			= mysql::mysql_collations::utf8mb4_0900_as_cs,
	utf8mb4_de_pb_0900_as_cs	= mysql::mysql_collations::utf8mb4_de_pb_0900_as_cs,
	utf8mb4_is_0900_as_cs		= mysql::mysql_collations::utf8mb4_is_0900_as_cs,
	utf8mb4_lv_0900_as_cs		= mysql::mysql_collations::utf8mb4_lv_0900_as_cs,
	utf8mb4_ro_0900_as_cs		= mysql::mysql_collations::utf8mb4_ro_0900_as_cs,
	utf8mb4_sl_0900_as_cs		= mysql::mysql_collations::utf8mb4_sl_0900_as_cs,
	utf8mb4_pl_0900_as_cs		= mysql::mysql_collations::utf8mb4_pl_0900_as_cs,
	utf8mb4_et_0900_as_cs		= mysql::mysql_collations::utf8mb4_et_0900_as_cs,
	utf8mb4_es_0900_as_cs		= mysql::mysql_collations::utf8mb4_es_0900_as_cs,
	utf8mb4_sv_0900_as_cs		= mysql::mysql_collations::utf8mb4_sv_0900_as_cs,
	utf8mb4_tr_0900_as_cs		= mysql::mysql_collations::utf8mb4_tr_0900_as_cs,
	utf8mb4_cs_0900_as_cs		= mysql::mysql_collations::utf8mb4_cs_0900_as_cs,
	utf8mb4_da_0900_as_cs		= mysql::mysql_collations::utf8mb4_da_0900_as_cs,
	utf8mb4_lt_0900_as_cs		= mysql::mysql_collations::utf8mb4_lt_0900_as_cs,
	utf8mb4_sk_0900_as_cs		= mysql::mysql_collations::utf8mb4_sk_0900_as_cs,
	utf8mb4_es_trad_0900_as_cs	= mysql::mysql_collations::utf8mb4_es_trad_0900_as_cs,
	utf8mb4_la_0900_as_cs		= mysql::mysql_collations::utf8mb4_la_0900_as_cs,
	utf8mb4_eo_0900_as_cs		= mysql::mysql_collations::utf8mb4_eo_0900_as_cs,
	utf8mb4_hu_0900_as_cs		= mysql::mysql_collations::utf8mb4_hu_0900_as_cs,
	utf8mb4_hr_0900_as_cs		= mysql::mysql_collations::utf8mb4_hr_0900_as_cs,
	utf8mb4_vi_0900_as_cs		= mysql::mysql_collations::utf8mb4_vi_0900_as_cs,
	utf8mb4_ja_0900_as_cs		= mysql::mysql_collations::utf8mb4_ja_0900_as_cs,
	utf8mb4_ja_0900_as_cs_ks	= mysql::mysql_collations::utf8mb4_ja_0900_as_cs_ks,
	utf8mb4_0900_as_ci			= mysql::mysql_collations::utf8mb4_0900_as_ci,
	utf8mb4_ru_0900_ai_ci		= mysql::mysql_collations::utf8mb4_ru_0900_ai_ci,
	utf8mb4_ru_0900_as_cs		= mysql::mysql_collations::utf8mb4_ru_0900_as_cs,
	utf8mb4_zh_0900_as_cs		= mysql::mysql_collations::utf8mb4_zh_0900_as_cs,
	utf8mb4_0900_bin			= mysql::mysql_collations::utf8mb4_0900_bin
};




#endif  // HELPERS_H
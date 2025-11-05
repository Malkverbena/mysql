/* helpers.h */


#ifndef HELPERS_H
#define HELPERS_H


#include "constants.h"

#include "core/object/ref_counted.h"
#include "core/core_bind.h"

#include <boost/mysql/error_with_diagnostics.hpp>
#include <boost/mysql/field.hpp>
#include <boost/mysql/time.hpp>
#include <boost/mysql/column_type.hpp>
#include <boost/mysql/metadata_collection_view.hpp>
#include <boost/mysql/metadata_mode.hpp>
#include <boost/mysql/rows_view.hpp>
#include <boost/mysql/row_view.hpp>


void boost_dictionary(Dictionary *dic, const char *p_function, const char *p_file, int p_line, const mysql::error_code ec);
void sql_dictionary(Dictionary *dic, const char *p_function, const char *p_file, int p_line, const mysql::diagnostics diag, const mysql::error_code ec);
void print_boost_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec);
void print_sql_exception(const char *p_function, const char *p_file, int p_line, const mysql::diagnostics diag, const mysql::error_code ec);
void print_std_exception(const char *p_function, const char *p_file, int p_line, std::exception err);


char* copy_string(char s[]);

mysql::string_view GdtStr2SqlStr(String s);

//String SqlStr2GdtStr(mysql::string_view s);
String SqlStr2GdtStr(boost::mysql::string_view s) {
    if (s.empty()) {
        return String();
    }
    return String::utf8(s.data(), static_cast<int>(s.size()));
}

bool is_date(Dictionary d);
bool is_datetime(Dictionary dt);
bool is_time(Dictionary t);
std::vector<mysql::field> binds_to_field(const Array args);
Variant field2Var(const mysql::field_view fv, mysql::column_type column_type);
Dictionary make_metadata_result(mysql::metadata_collection_view meta_collection);
Dictionary make_raw_result(mysql::rows_view batch, mysql::metadata_collection_view meta_coll);


#define BOOST_EXCEPTION(m_errcode, m_dic, m_ret)								\
	if (unlikely(m_errcode)) {													\
		boost_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		print_boost_exception(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
		return (Error)-m_errcode.value();										\
	} else																		\
		((void)0)

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
	} else																			\
		((void)0)

#define CORO_SQL_EXCEPTION_VOID(m_errcode, m_diag, m_dic)							\
	if (unlikely(m_errcode)) {														\
		sql_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);\
		print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);	\
		co_return;																	\
	} else																			\
		((void)0)


#endif  // HELPERS_H


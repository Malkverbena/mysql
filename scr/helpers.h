/* helpers.h */

#ifndef HELPERS_H
#define HELPERS_H


#include "headers_and_constants.h"
#include "sql_result.h"



void boost_dictionary(Dictionary *dic, const char *p_function, const char *p_file, int p_line, const mysql::error_code ec);
void sql_dictionary(Dictionary *dic, const char *p_function, const char *p_file, int p_line, const mysql::error_with_diagnostics diags);

void print_boost_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec);
void print_sql_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_with_diagnostics diags);
void print_std_exception(const char *p_function, const char *p_file, int p_line, std::exception err);



#define HANDLE_SQL_EXCEPTION(m_diags, m_dic, m_ret)							\
	sql_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diags);		\
	print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diags);		\
	return m_ret;															\


#define HANDLE_BOOST_EXCEPTION(m_errcode, m_dic, m_ret)						\
	boost_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
	print_boost_exception(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
	return m_ret;															\


#define HANDLE_STD_EXCEPTION(m_err, m_dic, m_ret)							\
	print_std_exception(FUNCTION_NAME, __FILE__, __LINE__, m_err);			\
	return m_ret;


#define HANDLE_SQL_EXCEPTION_IN_COROTINE(m_diags, m_dic)					\
	sql_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_diags);		\
	print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diags);		\


#define HANDLE_BOOST_EXCEPTION_IN_COROTINE(m_errcode, m_dic)				\
	boost_dictionary(m_dic, FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\
	print_boost_exception(FUNCTION_NAME, __FILE__, __LINE__, m_errcode);	\



char* copy_string(char s[]);

String SqlStr2GdtStr(mysql::string_view s);

mysql::string_view GdtStr2SqlStr(String s);

bool is_date(Dictionary d);

bool is_datetime(Dictionary dt);

bool is_time(Dictionary t);

std::vector<mysql::field> binds_to_field(const Array args);

Variant field2Var(const mysql::field_view fv);



#endif  // HELPERS_H








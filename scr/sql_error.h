/* sql_error.h */

#ifndef SQL_ERROR_H
#define SQL_ERROR_H


#include "core/object/ref_counted.h"  
#include "core/core_bind.h"
#include "core/io/json.h"


#include <boost/mysql.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/system/system_error.hpp>
#include <boost/mysql/ssl_mode.hpp>
#include <boost/mysql/connection.hpp>
#include <boost/mysql/string_view.hpp>
#include <boost/mysql/buffer_params.hpp>
#include <boost/mysql/handshake_params.hpp>

#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/buffer.hpp>


#include <iostream>
#include <memory>
#include <map>
#include <string.h>
#include <vector>
#include <iomanip>



#include <boost/asio/local/stream_protocol.hpp>
#include <boost/mysql/error_with_diagnostics.hpp>
#include <boost/mysql/results.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/statement.hpp>


using namespace std;
using namespace boost;
using namespace boost::mysql;
using namespace boost::asio;


#define DETAIL_FULL

#ifdef DETAIL_FULL
#define FUNC_STR __PRETTY_FUNCTION__
#else
#define FUNC_STR __FUNCTION__
#endif


static Dictionary last_error;



void print_sql_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec, const diagnostics diags);

void set_le_dic(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec, const diagnostics diags);

void print_boost_exception(const char *p_function, const char *p_file, int p_line, const mysql::error_code ec);


#define SQL_EXCEPTION_ERR(m_errcode, m_diag)                                            \
    if (unlikely(m_errcode)) {                                                          \
        print_sql_exception(FUNC_STR, __FILE__, __LINE__, m_errcode, m_diag);           \
        set_le_dic(FUNC_STR, __FILE__, __LINE__, m_errcode, m_diag);                    \
        return (Error)-m_errcode.value();                                               \
    }  else                                                                             \
        ((void)0)          
    

#define SQL_EXCEPTION_VAL(m_errcode, m_diag, m_val)                             \
    if (unlikely(m_errcode)) {                                                  \
        print_sql_exception(FUNC_STR, __FILE__, __LINE__, m_errcode, m_diag);   \
        set_le_dic(FUNC_STR, __FILE__, __LINE__, m_errcode, m_diag);            \
        return m_val;                                                           \
    } else                                                                      \
        ((void)0)  


#define BOOST_EXCEPTION(m_errcode)                                      \
    if (unlikely(m_errcode)) {                                          \
        print_boost_exception(FUNC_STR, __FILE__, __LINE__, m_errcode); \
        return (Error)-m_errcode.value();                               \
    } else                                                              \
        ((void)0) 

#endif  // SQL_ERROR_H




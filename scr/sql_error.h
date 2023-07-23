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

#include <iostream>
#include <memory>
#include <map>
#include <string.h>


#include <boost/asio/local/stream_protocol.hpp>
#include <boost/mysql/error_with_diagnostics.hpp>
#include <boost/mysql/results.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/statement.hpp>


using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::mysql;





#endif  // SQL_ERROR_H




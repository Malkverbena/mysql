/* boost_mysql_src.cpp */

// The only translation unit that instantiates Boost.MySQL in "separate" mode
// (`config.cfg`: `boost_mysql_mode = separate`).
//
// It is equivalent to `<boost/mysql/src.hpp>`, except for `impl/connection_pool.ipp`: the
// Boost connection pool uses `try`/`catch`, which does not compile with `-fno-exceptions`,
// and this module has its own pool. When updating Boost, compare this list with the one in
// `boost/mysql/src.hpp`.

#ifdef BOOST_MYSQL_SEPARATE_COMPILATION

#include <boost/mysql/detail/config.hpp>

#include <boost/mysql/impl/any_connection.ipp>
#include <boost/mysql/impl/character_set.ipp>
#include <boost/mysql/impl/column_type.ipp>
#include <boost/mysql/impl/connection_impl.ipp>
#include <boost/mysql/impl/date.ipp>
#include <boost/mysql/impl/datetime.ipp>
#include <boost/mysql/impl/engine_impl_instantiations.ipp>
#include <boost/mysql/impl/error_categories.ipp>
#include <boost/mysql/impl/escape_string.ipp>
#include <boost/mysql/impl/execution_state_impl.ipp>
#include <boost/mysql/impl/field.ipp>
#include <boost/mysql/impl/field_kind.ipp>
#include <boost/mysql/impl/field_view.ipp>
#include <boost/mysql/impl/format_sql.ipp>
#include <boost/mysql/impl/internal/error/server_error_to_string.ipp>
#include <boost/mysql/impl/is_fatal_error.ipp>
#include <boost/mysql/impl/meta_check_context.ipp>
#include <boost/mysql/impl/pipeline.ipp>
#include <boost/mysql/impl/results_impl.ipp>
#include <boost/mysql/impl/resultset.ipp>
#include <boost/mysql/impl/row_impl.ipp>
#include <boost/mysql/impl/static_execution_state_impl.ipp>
#include <boost/mysql/impl/static_results_impl.ipp>

#endif // BOOST_MYSQL_SEPARATE_COMPILATION

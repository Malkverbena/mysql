/* boost_mysql_src.cpp */


// Única unidade de compilação que instancia o Boost.MySQL no modo "separate"
// (config.cfg: boost_mysql_mode = separate).
//
// Equivale a <boost/mysql/src.hpp>, exceto por impl/connection_pool.ipp: o pool de
// conexões usa try/catch, que não compila com -fno-exceptions, e este módulo não o usa.
// Ao atualizar o Boost, compare esta lista com a de boost/mysql/src.hpp.

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

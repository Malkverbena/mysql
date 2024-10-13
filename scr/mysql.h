/* mysql.h */



#ifndef MYSQL_H
#define MYSQL_H


#include "sqlconnection.h"
#include "sqlresult.h"
#include "sqlcertificate.h"


using namespace sqlhelpers;


class MySQL : public RefCounted {
	GDCLASS(MySQL, RefCounted);

public:

	Ref<SqlCertificate> certificate;
	Ref<SqlConnection> connection; // = Ref<SqlConnection>(memnew(SqlConnection(certificate)));

	MySQL();
	~MySQL();


protected:

	static void _bind_methods();


	inline Dictionary get_sql_exception(mysql::error_code m_errcode, mysql::diagnostics m_diag){
		if (m_errcode){
			print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);
			return sql_dictionary(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);
		} else {
			return ok_dictionary();
		}
	}


	inline Ref<SqlResult> check_exception_on_loop(mysql::error_code m_errcode, mysql::diagnostics m_diag){
		Ref<SqlResult> _err = Ref<SqlResult>(memnew(SqlResult()));
		print_sql_exception(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);
		_err->sql_exception = sql_dictionary(FUNCTION_NAME, __FILE__, __LINE__, m_diag, m_errcode);
		return _err;
	}



private:


	asio::awaitable<mysql::results> coro_async_execute(const String p_stmt, mysql::error_code& ec, mysql::diagnostics& diag);

	asio::awaitable<mysql::results> coro_async_execute_prepared(const String p_stmt, const Array binds, mysql::error_code& ec, mysql::diagnostics& diag);

	asio::awaitable<Variant> coro_async_multiqueries(std::string script_contents);

	std::string GDstring_to_STDpath(const String p_path_to_file);

	Variant multiqueries(std::string queries);

	Ref<SqlResult> build_godot_result(mysql::results result, Dictionary sql_exception);

	Ref<SqlResult> build_godot_result(
		mysql::rows_view batch,
		mysql::metadata_collection_view meta_collection,
		std::uint64_t affected_rows,
		std::uint64_t last_insert_id,
		unsigned warning_count,
		Dictionary sql_exception
	);



public:

	Ref<SqlConnection> get_connection();
	Ref<SqlCertificate> get_certificate();



	// Execute queries.
	Ref<SqlResult> execute(const String p_stmt);

	// Execute prepared queries.
	Ref<SqlResult> execute_prepared(const String p_stmt, const Array binds = Array());

	// Execute sql scripts.
	// This function perform multi-queries, because this the "multi_queries" option must be true in the connection credentials,
	// otherwise this function won't be executed.
	// Be extremely careful with this function.
	Variant execute_sql(const String p_path_to_file);

	// Execute Multi-function operations.
	// You can use multi-function operations to execute several function text queries.
	// This function return an Array with a SqlResult for each query executed.
	Variant execute_multi(const String p_queries);




	Ref<SqlResult> async_execute(const String p_stmt);

	Ref<SqlResult> async_execute_prepared(const String p_stmt, const Array binds = Array());

	Variant async_execute_sql(const String p_path_to_file);

	Variant async_execute_multi(const String p_queries);









};




#endif // MYSQL_H

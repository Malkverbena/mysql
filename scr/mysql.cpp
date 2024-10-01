/* mysql.cpp */


#include "mysql.h"

#include <fstream>



Ref<SqlResult> MySQL::async_execute(const String p_stmt){
	const char* query = p_stmt.utf8().get_data();
	error_code ec;
	diagnostics diag;
	connection->last_sql_error.clear();

	auto result_future = asio::co_spawn(connection->ctx,
		coro_async_execute(query, ec, diag),
		asio::use_future
	);
	io_context.run();

	SQL_EXCEPTION(ec, diag, &connection->last_sql_error, Ref<SqlResult>());
	boost::mysql::results result = result_future.get();
	return build_godot_result(result);

}


awaitable<boost::mysql::results> MySQL::coro_async_execute(const char* query, error_code& ec, diagnostics& diag){
	auto executor = co_await asio::this_coro::executor;
	boost::mysql::results result;
	co_await conn.async_execute(query, result, diag, asio::redirect_error(use_awaitable, ec));
	CORO_SQL_EXCEPTION(ec, diag, &connection->last_sql_error, Ref<SqlResult>());
	co_return result;
}










Ref<SqlResult> MySQL::execute(const String p_stmt){
	const char* query = p_stmt.utf8().get_data();
	boost::mysql::results result;
	mysql::error_code ec;
	mysql::diagnostics diag;
	connection->last_sql_error.clear();
	connection->conn->execute(query, result, ec, diag);
	SQL_EXCEPTION(ec, diag, &connection->last_sql_error, Ref<SqlResult>());
	return build_godot_result(result);
}


Ref<SqlResult> MySQL::execute_prepared(const String p_stmt, const Array binds){
	std::vector<mysql::field> args = binds_to_field(binds);
	const char* query = p_stmt.utf8().get_data();
	mysql::error_code ec;
	mysql::diagnostics diag;
	mysql::results result;
	connection->last_sql_error.clear();
	mysql::statement prep_stmt = connection->conn->prepare_statement(query, ec, diag);
	SQL_EXCEPTION(ec, diag, &connection->last_sql_error, Ref<SqlResult>());
	connection->conn->execute(prep_stmt.bind(args.begin(), args.end()), result, ec, diag);
	SQL_EXCEPTION(ec, diag, &connection->last_sql_error, Ref<SqlResult>());
	connection->conn->close_statement(prep_stmt, ec, diag);
	SQL_EXCEPTION(ec, diag, &connection->last_sql_error, Ref<SqlResult>());
	return build_godot_result(result);
}


Array MySQL::execute_sql(const String p_path_to_file){
	bool multi_err = not connection->credentials_params.multi_queries;
	ERR_FAIL_COND_V_EDMSG(multi_err, Array(), "This function requires that credentials.multi_queries be enable, once it's uses using multi-queries");
	ProjectSettings &ps = *ProjectSettings::get_singleton();
	const char *path_to_file = ps.globalize_path(p_path_to_file).utf8().get_data();
	std::ifstream ifs(path_to_file);
	ERR_FAIL_COND_V_EDMSG(not ifs, Array(), "Cannot open file: " + p_path_to_file);
	std::string script_contents = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	return multiqueries(script_contents);
}


Array MySQL::execute_multi(const String p_queries){
	bool multi_err = not connection->credentials_params.multi_queries;
	ERR_FAIL_COND_V_EDMSG(multi_err, Array(), "This function requires that credentials.multi_queries be enable, once it's uses using multi-queries");
	std::string q(p_queries.utf8().get_data());
	return multiqueries(q);
}


Array MySQL::multiqueries(std::string queries){

	mysql::execution_state st;
	mysql::diagnostics diag;
	mysql::error_code ec;
	connection->last_sql_error.clear();
	Array ret;

	connection->conn->start_execution(queries, st, ec, diag);
	SQL_EXCEPTION(ec, diag, &connection->last_sql_error, ret);

	for (std::size_t resultset_number = 0; !st.complete(); ++resultset_number) {
		if (st.should_read_head()) {
			connection->conn->read_resultset_head(st, ec, diag);
			SQL_EXCEPTION(ec, diag, &connection->last_sql_error, ret);
			mysql::rows_view batch = connection->conn->read_some_rows(st, ec, diag);
			SQL_EXCEPTION(ec, diag, &connection->last_sql_error, ret);
			ret.append(build_godot_result(batch, st.meta(), st.affected_rows(), st.last_insert_id(), st.warning_count()));
		}
	}
	return ret;
}


Ref<SqlResult> MySQL::build_godot_result(
	mysql::rows_view batch,
	mysql::metadata_collection_view meta_collection,
	std::uint64_t affected_rows,
	std::uint64_t last_insert_id,
	unsigned warning_count
)
{
	Ref<SqlResult> gdt_result = Ref<SqlResult>(memnew(SqlResult()));
	gdt_result->result = make_raw_result(batch, meta_collection);
	gdt_result->meta = make_metadata_result(meta_collection);
	gdt_result->affected_rows = affected_rows;
	gdt_result->last_insert_id = last_insert_id;
	gdt_result->warning_count = warning_count;
	return gdt_result;
}



Ref<SqlResult> MySQL::build_godot_result(mysql::results result){

	Ref<SqlResult> gdt_result = Ref<SqlResult>(memnew(SqlResult()));
	gdt_result->result = make_raw_result(result.rows(), result.meta());
	gdt_result->meta = make_metadata_result(result.meta());
	gdt_result->affected_rows = result.affected_rows();
	gdt_result->last_insert_id = result.last_insert_id();
	gdt_result->warning_count = result.warning_count();
	return gdt_result;

}


// Default constructor
MySQL::MySQL() {
	certificate.instantiate();
	connection.instantiate();
	if (!connection.is_valid()) {
		connection = Ref<SqlConnection>(memnew(SqlConnection(certificate)));
	}
}

MySQL::~MySQL() {
}

void MySQL::_bind_methods() {


	ClassDB::bind_method(D_METHOD("execute", "query"), &MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "statement", "values"), &MySQL::execute_prepared, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("execute_sql", "sql path"), &MySQL::execute_sql);
	ClassDB::bind_method(D_METHOD("execute_multi", "queries"), &MySQL::execute_multi);



	ClassDB::bind_method(D_METHOD("async_execute", "query"), &MySQL::async_execute);





//	ClassDB::bind_method(D_METHOD("get_last_error"), &MySQL::get_last_error);


}
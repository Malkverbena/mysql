/* mysql.cpp */


#include "mysql.h"

#include <fstream>



Ref<SqlResult> MySQL::async_execute(const String p_stmt){
	std::lock_guard<std::mutex> lock(connection_mutex);
	mysql::error_code ec;
	mysql::diagnostics diag;

	auto result_future = asio::co_spawn(
		this->connection->ctx->get_executor(),
		coro_async_execute(p_stmt, ec, diag),
		asio::use_future
	);
	this->connection->ctx->run();

	mysql::results result = result_future.get();
	return build_godot_result(result, get_sql_exception(ec, diag));
}


asio::awaitable<mysql::results> MySQL::coro_async_execute(const String p_stmt, mysql::error_code& ec, mysql::diagnostics& diag){
	auto executor = co_await asio::this_coro::executor;
	const char* query = p_stmt.utf8().get_data();
	mysql::results result;
	co_await this->connection->conn->async_execute(query, result, diag, asio::redirect_error(asio::use_awaitable, ec));
	co_return result;
}



Ref<SqlResult> MySQL::async_execute_prepared(const String p_stmt, const Array binds){
	std::lock_guard<std::mutex> lock(connection_mutex);
	mysql::error_code ec;
	mysql::diagnostics diag;

	auto result_future = asio::co_spawn(
		connection->ctx->get_executor(),
		coro_async_execute_prepared(p_stmt, binds, ec, diag),
		asio::use_future
	);
	this->connection->ctx->run();

	mysql::results result = result_future.get();
	return build_godot_result(result, get_sql_exception(ec, diag));
}


asio::awaitable<mysql::results> MySQL::coro_async_execute_prepared(const String p_stmt, const Array binds, mysql::error_code& ec, mysql::diagnostics& diag){
	auto executor = co_await asio::this_coro::executor;

	const char* query = p_stmt.utf8().get_data();
	std::vector<mysql::field> args = binds_to_field(binds);
	mysql::results result;

	mysql::statement prep_stmt = co_await connection->conn->async_prepare_statement(query, diag, asio::redirect_error(asio::use_awaitable, ec));
	if (ec){co_return result;}

	co_await connection->conn->async_execute(prep_stmt.bind(args.begin(), args.end()), result, diag, asio::redirect_error(asio::use_awaitable, ec));
	if (ec){co_return result;}

	co_await connection->conn->async_close_statement(prep_stmt, diag, asio::redirect_error(use_awaitable, ec));
	co_return result;

}




Variant MySQL::async_execute_sql(const String p_path_to_file){
	std::lock_guard<std::mutex> lock(connection_mutex);
	std::string script_contents = GDstring_to_STDpath(p_path_to_file);
	if (script_contents == ""){
		return Variant(String());
	}

	auto result_future = asio::co_spawn(
		connection->ctx->get_executor(),
		coro_async_multiqueries(script_contents),
		asio::use_future
	);
	this->connection->ctx->run();

	Array result;
	result = Variant(result_future.get());
	return result;
}



Variant MySQL::async_execute_multi(const String p_queries){
	std::lock_guard<std::mutex> lock(connection_mutex);
	bool multi_err = not connection->credentials_params.multi_queries;
	ERR_FAIL_COND_V_EDMSG(multi_err, FAILED, "This function requires that credentials.multi_queries be enable, once it's uses using multi-queries");
	std::string queries(p_queries.utf8().get_data());


	auto result_future = asio::co_spawn(
		connection->ctx->get_executor(),
		coro_async_multiqueries(queries),
		asio::use_future
	);
	this->connection->ctx->run();

	Variant result;
	result = result_future.get();
	return result;
}


asio::awaitable<Variant> MySQL::coro_async_multiqueries(std::string script_contents){

	auto executor = co_await asio::this_coro::executor;

	mysql::execution_state st;
	mysql::diagnostics diag;
	mysql::error_code ec;
	Array ret;

	co_await connection->conn->async_start_execution(script_contents, st, diag, asio::redirect_error(asio::use_awaitable, ec));
	CORO_SQL_EXCEPTION(ec, diag);

	for (std::size_t resultset_number = 0; !st.complete(); ++resultset_number) {
		if (st.should_read_head()) {
			co_await connection->conn->async_read_resultset_head(st, diag, asio::redirect_error(asio::use_awaitable, ec));
			if (ec){
				ret.append(check_exception_on_loop(ec, diag));
				continue;
			}
			mysql::rows_view batch = co_await connection->conn->async_read_some_rows(st, diag, asio::redirect_error(asio::use_awaitable, ec));
			if (ec){
				ret.append(check_exception_on_loop(ec, diag));
				continue;
			}
			ret.append(build_godot_result(batch, st.meta(), st.affected_rows(), st.last_insert_id(), st.warning_count(), get_sql_exception(ec, diag)));
		}
	}
	co_return ret;
}


Ref<SqlResult> MySQL::execute(const String p_stmt){
	const char* query = p_stmt.utf8().get_data();
	mysql::results result;
	mysql::error_code ec;
	mysql::diagnostics diag;
	connection->conn->execute(query, result, ec, diag);
	get_sql_exception(ec, diag);
	return build_godot_result(result, get_sql_exception(ec, diag));
}


Ref<SqlResult> MySQL::execute_prepared(const String p_stmt, const Array binds){
	Ref<SqlResult> err = Ref<SqlResult>(memnew(SqlResult()));
	std::vector<mysql::field> args = binds_to_field(binds);
	const char* query = p_stmt.utf8().get_data();
	mysql::error_code ec;
	mysql::diagnostics diag;
	mysql::results result;

	mysql::statement prep_stmt = connection->conn->prepare_statement(query, ec, diag);
	if (ec) {
		return build_godot_result(result, get_sql_exception(ec, diag));
	}

	connection->conn->execute(prep_stmt.bind(args.begin(), args.end()), result, ec, diag);
	if (ec) {
		return build_godot_result(result, get_sql_exception(ec, diag));
	}

	connection->conn->close_statement(prep_stmt, ec, diag);
	return build_godot_result(result, get_sql_exception(ec, diag));
}












std::string MySQL::GDstring_to_STDpath(const String p_path_to_file){
	bool multi_err = not connection->credentials_params.multi_queries;
	ERR_FAIL_COND_V_EDMSG(multi_err, "", "This function requires that credentials.multi_queries be enable, once it's uses using multi-queries");
	ProjectSettings &ps = *ProjectSettings::get_singleton();
	const char *path_to_file = ps.globalize_path(p_path_to_file).utf8().get_data();
	std::ifstream ifs(path_to_file);
	ERR_FAIL_COND_V_EDMSG(not ifs, "", "Cannot open file: " + p_path_to_file);
	std::string script_contents = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	return script_contents;
}



Variant MySQL::execute_sql(const String p_path_to_file){
	std::string script_contents = GDstring_to_STDpath(p_path_to_file);
	if (GDstring_to_STDpath(p_path_to_file) == ""){
		return String();
	}
	return multiqueries(script_contents);
}


Variant MySQL::execute_multi(const String p_queries){
	bool multi_err = not connection->credentials_params.multi_queries;
	ERR_FAIL_COND_V_EDMSG(multi_err, FAILED, "This function requires that credentials.multi_queries be enable, once it's uses using multi-queries");
	std::string q(p_queries.utf8().get_data());
	return multiqueries(q);
}


Variant MySQL::multiqueries(std::string queries){

	mysql::execution_state st;
	mysql::diagnostics diag;
	mysql::error_code ec;
	Array ret;

	connection->conn->start_execution(queries, st, ec, diag);
	SQL_EXCEPTION(ec, diag);

	for (std::size_t resultset_number = 0; !st.complete(); ++resultset_number) {
		if (st.should_read_head()) {
			connection->conn->read_resultset_head(st, ec, diag);
			if (ec){
				ret.append(check_exception_on_loop(ec, diag));
				continue;
			}
			mysql::rows_view batch = connection->conn->read_some_rows(st, ec, diag);
			if (ec){
				ret.append(check_exception_on_loop(ec, diag));
				continue;
			}
			ret.append(build_godot_result(batch, st.meta(), st.affected_rows(), st.last_insert_id(), st.warning_count(), get_sql_exception(ec, diag)));
		}
	}
	return ret;
}



Ref<SqlResult> MySQL::build_godot_result(
	mysql::rows_view batch,
	mysql::metadata_collection_view meta_collection,
	std::uint64_t affected_rows,
	std::uint64_t last_insert_id,
	unsigned warning_count,
	Dictionary _sql_exception
	){
		Ref<SqlResult> gdt_result = Ref<SqlResult>(memnew(SqlResult()));
		gdt_result->result = make_raw_result(batch, meta_collection);
		gdt_result->meta = make_metadata_result(meta_collection);
		gdt_result->affected_rows = affected_rows;
		gdt_result->last_insert_id = last_insert_id;
		gdt_result->warning_count = warning_count;
		gdt_result->sql_exception = _sql_exception.duplicate(true);
		return gdt_result;
}



Ref<SqlResult> MySQL::build_godot_result(mysql::results result, Dictionary _sql_exception){
	Ref<SqlResult> gdt_result = Ref<SqlResult>(memnew(SqlResult()));
	gdt_result->result = make_raw_result(result.rows(), result.meta());
	gdt_result->meta = make_metadata_result(result.meta());
	gdt_result->affected_rows = result.affected_rows();
	gdt_result->last_insert_id = result.last_insert_id();
	gdt_result->warning_count = result.warning_count();
	gdt_result->sql_exception = _sql_exception.duplicate(true);
	return gdt_result;
}



Ref<SqlConnection> MySQL::get_connection() {
	return connection;
}

Ref<SqlCertificate> MySQL::get_certificate() {
	return certificate;
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
	ClassDB::bind_method(D_METHOD("execute_sql", "sql file path"), &MySQL::execute_sql);
	ClassDB::bind_method(D_METHOD("execute_multi", "queries"), &MySQL::execute_multi);

	ClassDB::bind_method(D_METHOD("async_execute", "query"), &MySQL::async_execute);
	ClassDB::bind_method(D_METHOD("async_execute_prepared", "statement", "values"), &MySQL::async_execute_prepared, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("async_execute_sql", "sql file path"), &MySQL::async_execute_sql);
	ClassDB::bind_method(D_METHOD("async_execute_multi", "queries"), &MySQL::async_execute_multi);

	ClassDB::bind_method(D_METHOD("get_connection"), &MySQL::get_connection);
	ClassDB::bind_method(D_METHOD("get_certificate"), &MySQL::get_certificate);

/*
	
	ADD_PROPERTY(
		PropertyInfo(
			Variant::OBJECT, "connection", PROPERTY_HINT_RESOURCE_TYPE, "SqlConnection", PROPERTY_USAGE_NONE), "", "get_connection"
	);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::OBJECT, "certificate", PROPERTY_HINT_RESOURCE_TYPE, "SqlCertificate", PROPERTY_USAGE_NONE), "", "get_certificate"
	);


*/
	
}
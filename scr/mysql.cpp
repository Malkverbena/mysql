/* mysql.cpp */

#include "mysql.h"




Error MySQL::define(const ConnType _type, const String p_cert_file, const String p_host_name) {

	last_error.clear();
	type = _type;
	async = false;

	tcp_conn.reset();
	tcp_ssl_conn.reset();
	unix_conn.reset();
	unix_ssl_conn.reset();
	resolver.reset();
	ssl_ctx.reset();
	ctx.reset();

	bool is_ssl = (type == TCPSSL or type == UNIXSSL);
	if (is_ssl and not p_cert_file.is_empty()){
		WARN_PRINT("Connection type is not TCPSSL or UNIXSSL! The certificate ill be ignorated.");
	}

	if (type == TCP){
		ctx = std::make_shared<asio::io_context>();
		tcp_conn = std::make_shared<mysql::tcp_connection>(ctx->get_executor());
		resolver = std::make_shared<asio::ip::tcp::resolver>(ctx->get_executor());
		tcp_conn->set_meta_mode(mysql::metadata_mode::full);
	}

	else if (type == TCPSSL){
		ctx = std::make_shared<asio::io_context>();
		ssl_ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::tls_client);
		if (is_ssl and not p_cert_file.is_empty()){
			Error err = set_certificate(ssl_ctx, p_cert_file, p_host_name);
			if (err){
				return err;
			}
		}
		tcp_ssl_conn = std::make_shared<mysql::tcp_ssl_connection>(ctx->get_executor(), *ssl_ctx);
		resolver = std::make_shared<asio::ip::tcp::resolver>(ctx->get_executor());
		tcp_ssl_conn->set_meta_mode(mysql::metadata_mode::full);
	}

	else if (type == UNIX){
		ctx = std::make_shared<asio::io_context>();
		unix_conn = std::make_shared<mysql::unix_connection>(*ctx);
		unix_conn->set_meta_mode(mysql::metadata_mode::full);
	}

	else if (type == UNIXSSL){
		ctx = std::make_shared<asio::io_context>();
		ssl_ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::tls_client);
		if (is_ssl and not p_cert_file.is_empty()){
			Error err = set_certificate(ssl_ctx, p_cert_file, p_host_name);
			if (err){
				return err;
			}
		}
		unix_ssl_conn = std::make_shared<mysql::unix_ssl_connection>(*ctx, *ssl_ctx);
		unix_ssl_conn->set_meta_mode(mysql::metadata_mode::full);
	}

	return OK;

}


Error MySQL::set_certificate(std::shared_ptr<asio::ssl::context> ssl_ctx, const String p_cert_file, const String p_host_name){

	const std::string& cert_path = p_cert_file.utf8().get_data();
	const std::string& host = p_host_name.utf8().get_data();
	last_error.clear();
	mysql::error_code ec;

	ssl_ctx->set_verify_mode(boost::asio::ssl::verify_peer, ec);
	BOOST_EXCEPTION(ec, &last_error, FAILED);
	ssl_ctx->set_verify_callback(ssl::host_name_verification(host), ec);
	BOOST_EXCEPTION(ec, &last_error, FAILED);
	ssl_ctx->load_verify_file(cert_path, ec);
	BOOST_EXCEPTION(ec, &last_error, FAILED);
	return OK;
}


Dictionary MySQL::get_credentials() const {

	Dictionary ret;
	ret["username"] = String(conn_params.username().data());
	ret["password"] = String(conn_params.password().data());
	ret["database"] = String(conn_params.database().data());
	ret["connection_collation"] = (int)conn_params.connection_collation();
	ret["ssl"] = (SslMode)conn_params.ssl();
	ret["multi_queries"] = conn_params.multi_queries();
	return ret;
}


Error MySQL::set_credentials(String p_username, String p_password, String p_database, std::uint16_t collation, SslMode p_ssl, bool multi_queries) {

	username = copy_string(const_cast<char*>(p_username.utf8().get_data()));
	password = copy_string(const_cast<char*>(p_password.utf8().get_data()));
	database = copy_string(const_cast<char*>(p_database.utf8().get_data()));

	// TODO: Testar se pode ser removido.
	conn_params.set_username(username);
	conn_params.set_password(password);
	conn_params.set_database(database);
	//TODO/

	conn_params.set_connection_collation(collation);
	conn_params.set_ssl((mysql::ssl_mode)p_ssl);
	conn_params.set_multi_queries(multi_queries);

	return OK;

}


Error MySQL::tcp_connect(const String p_hostname, const String p_port, const bool p_async) {

	last_error.clear();
	bool is_tcp = (type == TCP or type == TCPSSL);
	ERR_FAIL_COND_V_EDMSG(not is_tcp, ERR_INVALID_PARAMETER,\
	"The tcp_connect function was designed to work with TCP connections only. For UNIX connections use unix_connect method.");

	const char * hostname = p_hostname.utf8().get_data();
	const char * port = p_port.utf8().get_data();
	async = p_async;

	mysql::error_code ec;
	mysql::diagnostics diag;

	auto endpoints = resolver->resolve(hostname, port, ec);
	if (type == TCP){
		tcp_conn->connect(*endpoints.begin(), conn_params, ec, diag);
	}
	else {
		tcp_ssl_conn->connect(*endpoints.begin(), conn_params, ec, diag);
	}
	SQL_EXCEPTION(ec, diag, &last_error, FAILED);
	return OK;

	
}


Error MySQL::unix_connect(const String p_socket_path, const bool p_async) {

	last_error.clear();
	bool is_unix = (type == UNIX or type == UNIXSSL);
	ERR_FAIL_COND_V_EDMSG(not is_unix, ERR_INVALID_PARAMETER, "The unix_connect function was designed to work with UNIX connections only. For TCP connections use tcp_connect method.");

	const char * socket = p_socket_path.utf8().get_data();
	async = p_async;

	mysql::error_code ec;
	mysql::diagnostics diag;

	local::stream_protocol::endpoint endpoints(socket);
	if (type == UNIX){
		unix_conn->connect(endpoints, conn_params, ec, diag);
	}
	else {
		unix_ssl_conn->connect(endpoints, conn_params, ec, diag);
	}
	SQL_EXCEPTION(ec, diag, &last_error, FAILED);
	
	return OK;
}


Error MySQL::close_connection() {

	last_error.clear();
	mysql::error_code ec;
	mysql::diagnostics diag;

	if (type == TCP){
		tcp_conn->close(ec, diag);
	}
	else if (type == TCPSSL){
		tcp_ssl_conn->close(ec, diag);
	}
	else if (type == UNIX){
		unix_conn->close(ec, diag);
	}
	else if (type == UNIXSSL){
		unix_ssl_conn->close(ec, diag);
	}
	
	SQL_EXCEPTION(ec, diag, &last_error, FAILED);
	return OK;
}



bool MySQL::is_server_alive() {

	mysql::error_code ec;
	mysql::diagnostics diag;

	if (type == TCP){
		tcp_conn->ping(ec, diag);
	}
	else if (type == TCPSSL){
		tcp_ssl_conn->ping(ec, diag);
	}
	else if (type == UNIX){
		unix_conn->ping(ec, diag);
	}
	else if (type == UNIXSSL){
		unix_ssl_conn->ping(ec, diag);
	}
	
	print_line("IS SERVER ALIVE RETURN NORMAL");

	if (ec){
		return false;
	}

	return true;
}




/*
Ref<SqlResult> MySQL::async_execute(const String p_stmt) {
	return _async_execute(p_stmt, Array(), false);
}

Ref<SqlResult> MySQL::async_execute_prepared(const String p_stmt, const Array binds) {
	return _async_execute(p_stmt, binds, true);
}

Ref<SqlResult> MySQL::_async_execute(const String p_stmt, const Array binds, bool is_prepareted){}
*/




Ref<SqlResult> MySQL::build_godot_result(mysql::results result){

	Dictionary meta;

	for(auto m:result.meta()) {
		Dictionary column;
		String column_name = String(m.column_name().data());

		column["column_collation"]			= m.column_collation();
		column["column_length"]				= m.column_length();
		column["column_name"]				= column_name;
		column["database"]					= String(m.database().data());
		column["decimals"]					= m.decimals();
		column["has_no_default_value"]		= m.has_no_default_value();
		column["is_auto_increment"]			= m.is_auto_increment();
		column["is_multiple_key"]			= m.is_multiple_key();
		column["is_not_null"]				= m.is_not_null();
		column["is_primary_key"]			= m.is_primary_key();
		column["is_set_to_now_on_update"]	= m.is_set_to_now_on_update();
		column["is_unique_key"]				= m.is_unique_key();
		column["is_unsigned"]				= m.is_unsigned();
		column["is_zerofill"]				= m.is_zerofill();
		column["original_column_name"]		= String(m.original_column_name().data());
		column["original_table"]			= String(m.original_table().data());
		column["table"]						= String(m.table().data());
		column["type"]						= (int)m.type();

		meta[column_name] = column;
	}

	Dictionary querty_result;
	mysql::rows_view all_rows = result.rows();

	for(size_t row = 0; row < all_rows.size(); row++) {
		Dictionary line = Dictionary();
		size_t f = 0;
		for (auto fv : all_rows.at(row).as_vector()) {
			String column_name = String(result.meta()[f].column_name().data());
			mysql::column_type column_type = result.meta()[f].type();
			line[column_name] = field2Var(fv, column_type);
			f++;
		}
		querty_result[row] = line;
	}

	Ref<SqlResult> gdt_result = Ref<SqlResult>(memnew(SqlResult()));

	gdt_result->result = querty_result;
	gdt_result->meta = meta;
	gdt_result->affected_rows = result.affected_rows();
	gdt_result->last_insert_id = result.last_insert_id();
	gdt_result->warning_count = result.warning_count();

	return gdt_result;

}


Ref<SqlResult> MySQL::execute(const String p_stmt) {

	last_error.clear();
	Ref<SqlResult> sql_result;
	ERR_FAIL_COND_V_EDMSG(async, sql_result,
	"This method only works with synchronous connections. For asynchronous connections use async_execute function.");

	const char* query = p_stmt.utf8().get_data();
	boost::mysql::results result;
	mysql::error_code ec;
	mysql::diagnostics diag;

	if (type == TCP){
		tcp_conn->execute(query, result, ec, diag);
	}
	else if (type == TCPSSL){
		tcp_ssl_conn->execute(query, result, ec, diag);
	}
	else if (type == UNIX){
		unix_conn->execute(query, result, ec, diag);
	}
	else if (type == UNIXSSL){
		unix_ssl_conn->execute(query, result, ec, diag);
	}
	SQL_EXCEPTION(ec, diag, &last_error, sql_result);

	return build_godot_result(result);

}



Ref<SqlResult> MySQL::execute_prepared(const String p_stmt, const Array binds) {

	last_error.clear();
	Ref<SqlResult> sql_result;
	ERR_FAIL_COND_V_EDMSG(async, sql_result, "This method only works with synchronous connections. For asynchronous connections use async_execute function.");

	std::vector<mysql::field> args = binds_to_field(binds);
	const char* query = p_stmt.utf8().get_data();

	mysql::error_code ec;
	mysql::diagnostics diag;
	boost::mysql::results result;

	if (type == TCP){
		mysql::statement prep_stmt = tcp_conn->prepare_statement(query, ec, diag);
		SQL_EXCEPTION(ec, diag, &last_error, sql_result);
		tcp_conn->execute(prep_stmt.bind(args.begin(), args.end()), result, ec, diag);
	}
	else if (type == TCPSSL){
		mysql::statement prep_stmt = tcp_conn->prepare_statement(query, ec, diag);
		SQL_EXCEPTION(ec, diag, &last_error, sql_result);
		tcp_conn->execute(prep_stmt.bind(args.begin(), args.end()), result, ec, diag);
	}
	else if (type == UNIX){
		mysql::statement prep_stmt = tcp_conn->prepare_statement(query, ec, diag);
		SQL_EXCEPTION(ec, diag, &last_error, sql_result);
		tcp_conn->execute(prep_stmt.bind(args.begin(), args.end()), result, ec, diag);
		}
	else if (type == UNIXSSL){
		mysql::statement prep_stmt = tcp_conn->prepare_statement(query, ec, diag);
		SQL_EXCEPTION(ec, diag, &last_error, sql_result);
		tcp_conn->execute(prep_stmt.bind(args.begin(), args.end()), result, ec, diag);
	}

	SQL_EXCEPTION(ec, diag, &last_error, sql_result);

	return build_godot_result(result);

}








void MySQL::_bind_methods() {

	ClassDB::bind_method(D_METHOD("define", "connection type", "certificate file", "hostname verification"), &MySQL::define, DEFVAL(ConnType::TCPSSL), DEFVAL(""), DEFVAL("mysql"));
	ClassDB::bind_method(D_METHOD("get_credentials"), &MySQL::get_credentials);
	ClassDB::bind_method(D_METHOD("set_credentials", "username", "password", "database", "collation", "ssl_mode", "multi_queries"),\
	&MySQL::set_credentials, DEFVAL(""), DEFVAL(default_collation), DEFVAL(ssl_enable), DEFVAL(false));

	ClassDB::bind_method(D_METHOD("tcp_connect", "hostname", "port", "async"), &MySQL::tcp_connect, DEFVAL("127.0.0.1"), DEFVAL("3306"), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("unix_connect", "socket_path", "async"), &MySQL::unix_connect, DEFVAL("/var/run/mysqld/mysqld.sock"), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("close_connection"), &MySQL::close_connection);
	ClassDB::bind_method(D_METHOD("is_server_alive"), &MySQL::is_server_alive);

	ClassDB::bind_method(D_METHOD("get_connection_type"), &MySQL::get_connection_type);
	ClassDB::bind_method(D_METHOD("get_last_error"), &MySQL::get_last_error);
	ClassDB::bind_method(D_METHOD("is_async"), &MySQL::is_async);

	ClassDB::bind_method(D_METHOD("execute", "query"), &MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "statement", "values"), &MySQL::execute_prepared, DEFVAL(Array()));
//	ClassDB::bind_method(D_METHOD("execute_sql", "sql path"), &MySQL::execute_sql);
//	ClassDB::bind_method(D_METHOD("async_execute", "query"), &MySQL::async_execute);
//	ClassDB::bind_method(D_METHOD("async_execute_prepared", "query", "values"), &MySQL::async_execute_prepared, DEFVAL(Array()));




	BIND_ENUM_CONSTANT(TCP);
	BIND_ENUM_CONSTANT(TCPSSL);
	BIND_ENUM_CONSTANT(UNIX);
	BIND_ENUM_CONSTANT(UNIXSSL);

	BIND_ENUM_CONSTANT(ssl_disable);
	BIND_ENUM_CONSTANT(ssl_enable);
	BIND_ENUM_CONSTANT(ssl_require);

	BIND_ENUM_CONSTANT(default_collation);
	BIND_ENUM_CONSTANT(big5_chinese_ci);
	BIND_ENUM_CONSTANT(latin2_czech_cs);
	BIND_ENUM_CONSTANT(dec8_swedish_ci);
	BIND_ENUM_CONSTANT(cp850_general_ci);
	BIND_ENUM_CONSTANT(latin1_german1_ci);
	BIND_ENUM_CONSTANT(hp8_english_ci);
	BIND_ENUM_CONSTANT(koi8r_general_ci);
	BIND_ENUM_CONSTANT(latin1_swedish_ci);
	BIND_ENUM_CONSTANT(latin2_general_ci);
	BIND_ENUM_CONSTANT(swe7_swedish_ci);
	BIND_ENUM_CONSTANT(ascii_general_ci);
	BIND_ENUM_CONSTANT(ujis_japanese_ci);
	BIND_ENUM_CONSTANT(sjis_japanese_ci);
	BIND_ENUM_CONSTANT(cp1251_bulgarian_ci);
	BIND_ENUM_CONSTANT(latin1_danish_ci);
	BIND_ENUM_CONSTANT(hebrew_general_ci);
	BIND_ENUM_CONSTANT(tis620_thai_ci);
	BIND_ENUM_CONSTANT(euckr_korean_ci);
	BIND_ENUM_CONSTANT(latin7_estonian_cs);
	BIND_ENUM_CONSTANT(latin2_hungarian_ci);
	BIND_ENUM_CONSTANT(koi8u_general_ci);
	BIND_ENUM_CONSTANT(cp1251_ukrainian_ci);
	BIND_ENUM_CONSTANT(gb2312_chinese_ci);
	BIND_ENUM_CONSTANT(greek_general_ci);
	BIND_ENUM_CONSTANT(cp1250_general_ci);
	BIND_ENUM_CONSTANT(latin2_croatian_ci);
	BIND_ENUM_CONSTANT(gbk_chinese_ci);
	BIND_ENUM_CONSTANT(cp1257_lithuanian_ci);
	BIND_ENUM_CONSTANT(latin5_turkish_ci);
	BIND_ENUM_CONSTANT(latin1_german2_ci);
	BIND_ENUM_CONSTANT(armscii8_general_ci);
	BIND_ENUM_CONSTANT(utf8_general_ci);
	BIND_ENUM_CONSTANT(cp1250_czech_cs);
	BIND_ENUM_CONSTANT(ucs2_general_ci);
	BIND_ENUM_CONSTANT(cp866_general_ci);
	BIND_ENUM_CONSTANT(keybcs2_general_ci);
	BIND_ENUM_CONSTANT(macce_general_ci);
	BIND_ENUM_CONSTANT(macroman_general_ci);
	BIND_ENUM_CONSTANT(cp852_general_ci);
	BIND_ENUM_CONSTANT(latin7_general_ci);
	BIND_ENUM_CONSTANT(latin7_general_cs);
	BIND_ENUM_CONSTANT(macce_bin);
	BIND_ENUM_CONSTANT(cp1250_croatian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_general_ci);
	BIND_ENUM_CONSTANT(utf8mb4_bin);
	BIND_ENUM_CONSTANT(latin1_bin);
	BIND_ENUM_CONSTANT(latin1_general_ci);
	BIND_ENUM_CONSTANT(latin1_general_cs);
	BIND_ENUM_CONSTANT(cp1251_bin);
	BIND_ENUM_CONSTANT(cp1251_general_ci);
	BIND_ENUM_CONSTANT(cp1251_general_cs);
	BIND_ENUM_CONSTANT(macroman_bin);
	BIND_ENUM_CONSTANT(utf16_general_ci);
	BIND_ENUM_CONSTANT(utf16_bin);
	BIND_ENUM_CONSTANT(utf16le_general_ci);
	BIND_ENUM_CONSTANT(cp1256_general_ci);
	BIND_ENUM_CONSTANT(cp1257_bin);
	BIND_ENUM_CONSTANT(cp1257_general_ci);
	BIND_ENUM_CONSTANT(utf32_general_ci);
	BIND_ENUM_CONSTANT(utf32_bin);
	BIND_ENUM_CONSTANT(utf16le_bin);
	BIND_ENUM_CONSTANT(binary);
	BIND_ENUM_CONSTANT(armscii8_bin);
	BIND_ENUM_CONSTANT(ascii_bin);
	BIND_ENUM_CONSTANT(cp1250_bin);
	BIND_ENUM_CONSTANT(cp1256_bin);
	BIND_ENUM_CONSTANT(cp866_bin);
	BIND_ENUM_CONSTANT(dec8_bin);
	BIND_ENUM_CONSTANT(greek_bin);
	BIND_ENUM_CONSTANT(hebrew_bin);
	BIND_ENUM_CONSTANT(hp8_bin);
	BIND_ENUM_CONSTANT(keybcs2_bin);
	BIND_ENUM_CONSTANT(koi8r_bin);
	BIND_ENUM_CONSTANT(koi8u_bin);
	BIND_ENUM_CONSTANT(utf8_tolower_ci);
	BIND_ENUM_CONSTANT(latin2_bin);
	BIND_ENUM_CONSTANT(latin5_bin);
	BIND_ENUM_CONSTANT(latin7_bin);
	BIND_ENUM_CONSTANT(cp850_bin);
	BIND_ENUM_CONSTANT(cp852_bin);
	BIND_ENUM_CONSTANT(swe7_bin);
	BIND_ENUM_CONSTANT(utf8_bin);
	BIND_ENUM_CONSTANT(big5_bin);
	BIND_ENUM_CONSTANT(euckr_bin);
	BIND_ENUM_CONSTANT(gb2312_bin);
	BIND_ENUM_CONSTANT(gbk_bin);
	BIND_ENUM_CONSTANT(sjis_bin);
	BIND_ENUM_CONSTANT(tis620_bin);
	BIND_ENUM_CONSTANT(ucs2_bin);
	BIND_ENUM_CONSTANT(ujis_bin);
	BIND_ENUM_CONSTANT(geostd8_general_ci);
	BIND_ENUM_CONSTANT(geostd8_bin);
	BIND_ENUM_CONSTANT(latin1_spanish_ci);
	BIND_ENUM_CONSTANT(cp932_japanese_ci);
	BIND_ENUM_CONSTANT(cp932_bin);
	BIND_ENUM_CONSTANT(eucjpms_japanese_ci);
	BIND_ENUM_CONSTANT(eucjpms_bin);
	BIND_ENUM_CONSTANT(cp1250_polish_ci);
	BIND_ENUM_CONSTANT(utf16_unicode_ci);
	BIND_ENUM_CONSTANT(utf16_icelandic_ci);
	BIND_ENUM_CONSTANT(utf16_latvian_ci);
	BIND_ENUM_CONSTANT(utf16_romanian_ci);
	BIND_ENUM_CONSTANT(utf16_slovenian_ci);
	BIND_ENUM_CONSTANT(utf16_polish_ci);
	BIND_ENUM_CONSTANT(utf16_estonian_ci);
	BIND_ENUM_CONSTANT(utf16_spanish_ci);
	BIND_ENUM_CONSTANT(utf16_swedish_ci);
	BIND_ENUM_CONSTANT(utf16_turkish_ci);
	BIND_ENUM_CONSTANT(utf16_czech_ci);
	BIND_ENUM_CONSTANT(utf16_danish_ci);
	BIND_ENUM_CONSTANT(utf16_lithuanian_ci);
	BIND_ENUM_CONSTANT(utf16_slovak_ci);
	BIND_ENUM_CONSTANT(utf16_spanish2_ci);
	BIND_ENUM_CONSTANT(utf16_roman_ci);
	BIND_ENUM_CONSTANT(utf16_persian_ci);
	BIND_ENUM_CONSTANT(utf16_esperanto_ci);
	BIND_ENUM_CONSTANT(utf16_hungarian_ci);
	BIND_ENUM_CONSTANT(utf16_sinhala_ci);
	BIND_ENUM_CONSTANT(utf16_german2_ci);
	BIND_ENUM_CONSTANT(utf16_croatian_ci);
	BIND_ENUM_CONSTANT(utf16_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf16_vietnamese_ci);
	BIND_ENUM_CONSTANT(ucs2_unicode_ci);
	BIND_ENUM_CONSTANT(ucs2_icelandic_ci);
	BIND_ENUM_CONSTANT(ucs2_latvian_ci);
	BIND_ENUM_CONSTANT(ucs2_romanian_ci);
	BIND_ENUM_CONSTANT(ucs2_slovenian_ci);
	BIND_ENUM_CONSTANT(ucs2_polish_ci);
	BIND_ENUM_CONSTANT(ucs2_estonian_ci);
	BIND_ENUM_CONSTANT(ucs2_spanish_ci);
	BIND_ENUM_CONSTANT(ucs2_swedish_ci);
	BIND_ENUM_CONSTANT(ucs2_turkish_ci);
	BIND_ENUM_CONSTANT(ucs2_czech_ci);
	BIND_ENUM_CONSTANT(ucs2_danish_ci);
	BIND_ENUM_CONSTANT(ucs2_lithuanian_ci);
	BIND_ENUM_CONSTANT(ucs2_slovak_ci);
	BIND_ENUM_CONSTANT(ucs2_spanish2_ci);
	BIND_ENUM_CONSTANT(ucs2_roman_ci);
	BIND_ENUM_CONSTANT(ucs2_persian_ci);
	BIND_ENUM_CONSTANT(ucs2_esperanto_ci);
	BIND_ENUM_CONSTANT(ucs2_hungarian_ci);
	BIND_ENUM_CONSTANT(ucs2_sinhala_ci);
	BIND_ENUM_CONSTANT(ucs2_german2_ci);
	BIND_ENUM_CONSTANT(ucs2_croatian_ci);
	BIND_ENUM_CONSTANT(ucs2_unicode_520_ci);
	BIND_ENUM_CONSTANT(ucs2_vietnamese_ci);
	BIND_ENUM_CONSTANT(ucs2_general_mysql500_ci);
	BIND_ENUM_CONSTANT(utf32_unicode_ci);
	BIND_ENUM_CONSTANT(utf32_icelandic_ci);
	BIND_ENUM_CONSTANT(utf32_latvian_ci);
	BIND_ENUM_CONSTANT(utf32_romanian_ci);
	BIND_ENUM_CONSTANT(utf32_slovenian_ci);
	BIND_ENUM_CONSTANT(utf32_polish_ci);
	BIND_ENUM_CONSTANT(utf32_estonian_ci);
	BIND_ENUM_CONSTANT(utf32_spanish_ci);
	BIND_ENUM_CONSTANT(utf32_swedish_ci);
	BIND_ENUM_CONSTANT(utf32_turkish_ci);
	BIND_ENUM_CONSTANT(utf32_czech_ci);
	BIND_ENUM_CONSTANT(utf32_danish_ci);
	BIND_ENUM_CONSTANT(utf32_lithuanian_ci);
	BIND_ENUM_CONSTANT(utf32_slovak_ci);
	BIND_ENUM_CONSTANT(utf32_spanish2_ci);
	BIND_ENUM_CONSTANT(utf32_roman_ci);
	BIND_ENUM_CONSTANT(utf32_persian_ci);
	BIND_ENUM_CONSTANT(utf32_esperanto_ci);
	BIND_ENUM_CONSTANT(utf32_hungarian_ci);
	BIND_ENUM_CONSTANT(utf32_sinhala_ci);
	BIND_ENUM_CONSTANT(utf32_german2_ci);
	BIND_ENUM_CONSTANT(utf32_croatian_ci);
	BIND_ENUM_CONSTANT(utf32_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf32_vietnamese_ci);
	BIND_ENUM_CONSTANT(utf8_unicode_ci);
	BIND_ENUM_CONSTANT(utf8_icelandic_ci);
	BIND_ENUM_CONSTANT(utf8_latvian_ci);
	BIND_ENUM_CONSTANT(utf8_romanian_ci);
	BIND_ENUM_CONSTANT(utf8_slovenian_ci);
	BIND_ENUM_CONSTANT(utf8_polish_ci);
	BIND_ENUM_CONSTANT(utf8_estonian_ci);
	BIND_ENUM_CONSTANT(utf8_spanish_ci);
	BIND_ENUM_CONSTANT(utf8_swedish_ci);
	BIND_ENUM_CONSTANT(utf8_turkish_ci);
	BIND_ENUM_CONSTANT(utf8_czech_ci);
	BIND_ENUM_CONSTANT(utf8_danish_ci);
	BIND_ENUM_CONSTANT(utf8_lithuanian_ci);
	BIND_ENUM_CONSTANT(utf8_slovak_ci);
	BIND_ENUM_CONSTANT(utf8_spanish2_ci);
	BIND_ENUM_CONSTANT(utf8_roman_ci);
	BIND_ENUM_CONSTANT(utf8_persian_ci);
	BIND_ENUM_CONSTANT(utf8_esperanto_ci);
	BIND_ENUM_CONSTANT(utf8_hungarian_ci);
	BIND_ENUM_CONSTANT(utf8_sinhala_ci);
	BIND_ENUM_CONSTANT(utf8_german2_ci);
	BIND_ENUM_CONSTANT(utf8_croatian_ci);
	BIND_ENUM_CONSTANT(utf8_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf8_vietnamese_ci);
	BIND_ENUM_CONSTANT(utf8_general_mysql500_ci);
	BIND_ENUM_CONSTANT(utf8mb4_unicode_ci);
	BIND_ENUM_CONSTANT(utf8mb4_icelandic_ci);
	BIND_ENUM_CONSTANT(utf8mb4_latvian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_romanian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_slovenian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_polish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_estonian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_spanish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_swedish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_turkish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_czech_ci);
	BIND_ENUM_CONSTANT(utf8mb4_danish_ci);
	BIND_ENUM_CONSTANT(utf8mb4_lithuanian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_slovak_ci);
	BIND_ENUM_CONSTANT(utf8mb4_spanish2_ci);
	BIND_ENUM_CONSTANT(utf8mb4_roman_ci);
	BIND_ENUM_CONSTANT(utf8mb4_persian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_esperanto_ci);
	BIND_ENUM_CONSTANT(utf8mb4_hungarian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_sinhala_ci);
	BIND_ENUM_CONSTANT(utf8mb4_german2_ci);
	BIND_ENUM_CONSTANT(utf8mb4_croatian_ci);
	BIND_ENUM_CONSTANT(utf8mb4_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf8mb4_vietnamese_ci);
	BIND_ENUM_CONSTANT(gb18030_chinese_ci);
	BIND_ENUM_CONSTANT(gb18030_bin);
	BIND_ENUM_CONSTANT(gb18030_unicode_520_ci);
	BIND_ENUM_CONSTANT(utf8mb4_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_de_pb_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_is_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_lv_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_ro_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_sl_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_pl_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_et_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_es_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_sv_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_tr_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_cs_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_da_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_lt_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_sk_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_es_trad_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_la_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_eo_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_hu_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_hr_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_vi_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_de_pb_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_is_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_lv_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_ro_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_sl_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_pl_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_et_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_es_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_sv_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_tr_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_cs_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_da_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_lt_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_sk_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_es_trad_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_la_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_eo_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_hu_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_hr_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_vi_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_ja_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_ja_0900_as_cs_ks);
	BIND_ENUM_CONSTANT(utf8mb4_0900_as_ci);
	BIND_ENUM_CONSTANT(utf8mb4_ru_0900_ai_ci);
	BIND_ENUM_CONSTANT(utf8mb4_ru_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_zh_0900_as_cs);
	BIND_ENUM_CONSTANT(utf8mb4_0900_bin);

}


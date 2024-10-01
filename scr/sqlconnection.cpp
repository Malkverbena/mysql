/* sqlconnection.cpp */

#include "sqlconnection.h"

using namespace sqlhelpers;


Error SqlConnection::configure_connection(ConnType connectiontype, bool p_use_certificate) {

	//#if defined(DEBUG_ENABLED) && defined(TOOLS_ENABLED)
	bool unconf = (p_use_certificate and not certificate.is_valid());
	ERR_FAIL_COND_V_EDMSG(unconf, ERR_UNCONFIGURED, "Certificate unconfigured!");
	
	use_certificate = p_use_certificate;
	conn_type = connectiontype;
	last_sql_error.clear();
	ctx.reset();
	conn.reset();
	ctx = std::make_shared<asio::io_context>();

	if (use_certificate) {
		ctor_params.ssl_context = certificate->ssl_ctx.get();
		conn = std::make_shared<mysql::any_connection>(*ctx, ctor_params);
	}
	else{
		conn = std::make_shared<mysql::any_connection>(*ctx);
	}
	conn->set_meta_mode(mysql::metadata_mode::full);
	return OK;
}


Error SqlConnection::db_connect(const String p_hostname, const int port) {

	if (conn_type == TCP or conn_type == TCP_TLS){
		credentials_params.server_address.emplace_host_and_port(GDstring_to_SQLstring(p_hostname),  static_cast<std::uint16_t>(port));
	}	
	else if (conn_type == UNIX){
		credentials_params.server_address.emplace_unix_path(GDstring_to_SQLstring(p_hostname));
	}
	else{
		return ERR_INVALID_PARAMETER;
	}

	mysql::error_code ec;
	mysql::diagnostics diag;
	conn->connect(credentials_params, ec, diag);
	SQL_EXCEPTION(ec, diag, &last_sql_error, FAILED);
	return OK;
}


Error SqlConnection::close_connection() {
	mysql::error_code ec;
	mysql::diagnostics diag;
	conn->close();
	SQL_EXCEPTION(ec, diag, &last_sql_error, FAILED);
	return OK;
}


Error SqlConnection::reset_connection(){
	mysql::error_code ec;
	mysql::diagnostics diag;
	conn->reset_connection();
	SQL_EXCEPTION(ec, diag, &last_sql_error, FAILED);
	return OK;
}


bool SqlConnection::is_server_alive() {
	mysql::error_code ec;
	mysql::diagnostics diag;
	conn->reset_connection();
	if (ec){
		return false;
	}
	return true;
}


SqlConnection::~SqlConnection() {
	close_connection();
}



void SqlConnection::_bind_methods() {


	// SQL
	ClassDB::bind_method(D_METHOD("db_connect", "hostname", "port"), &SqlConnection::db_connect, DEFVAL("/var/run/mysqld/mysqld.sock"), DEFVAL(3307));
	ClassDB::bind_method(D_METHOD("get_connection_type"), &SqlConnection::get_connection_type);
	ClassDB::bind_method(D_METHOD("backslash_escapes"), &SqlConnection::backslash_escapes);
	ClassDB::bind_method(D_METHOD("close_connection"), &SqlConnection::close_connection);
	ClassDB::bind_method(D_METHOD("reset_connection"), &SqlConnection::reset_connection);
	ClassDB::bind_method(D_METHOD("is_server_alive"), &SqlConnection::is_server_alive);

	


	// Connection
	ClassDB::bind_method(D_METHOD("configure_connection", "connection type", "use_certificate"), &SqlConnection::configure_connection, DEFVAL(TCP), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("is_using_certificate"), &SqlConnection::is_using_certificate);
	ClassDB::bind_method(D_METHOD("get_sql_error"), &SqlConnection::get_sql_error);
	ClassDB::bind_method(D_METHOD("get_async"), &SqlConnection::get_async);
	ClassDB::bind_method(D_METHOD("set_async", "value"), &SqlConnection::set_async);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "enable_async"), "set_async", "get_async");
	ADD_PROPERTY_DEFAULT("enable_async", "false");


	// Credentials
	ClassDB::bind_method(D_METHOD("get_username"), &SqlConnection::get_username);
	ClassDB::bind_method(D_METHOD("set_username", "value"), &SqlConnection::set_username);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "username"), "set_username", "get_username");
	ADD_PROPERTY_DEFAULT("username", "");

	ClassDB::bind_method(D_METHOD("get_password"), &SqlConnection::get_password);
	ClassDB::bind_method(D_METHOD("set_password", "value"), &SqlConnection::set_password);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "password"), "set_password", "get_password");
	ADD_PROPERTY_DEFAULT("password", "");

	ClassDB::bind_method(D_METHOD("get_database"), &SqlConnection::get_database);
	ClassDB::bind_method(D_METHOD("set_database", "value"), &SqlConnection::set_database);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "database"), "set_database", "get_database");
	ADD_PROPERTY_DEFAULT("database", "");

	ClassDB::bind_method(D_METHOD("get_multi_queries"), &SqlConnection::get_multi_queries);
	ClassDB::bind_method(D_METHOD("set_multi_queries", "value"), &SqlConnection::set_multi_queries);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "multi_queries"), "set_multi_queries", "get_multi_queries");
	ADD_PROPERTY_DEFAULT("multi_queries", false);

	ClassDB::bind_method(D_METHOD("get_ssl_mode"), &SqlConnection::get_ssl_mode);
	ClassDB::bind_method(D_METHOD("set_ssl_mode", "value"), &SqlConnection::set_ssl_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "SSLMode", PROPERTY_HINT_ENUM, "ssl_disable, ssl_enable, ssl_require"), "set_ssl_mode", "get_ssl_mode");
	ADD_PROPERTY_DEFAULT("SSLMode", SqlCertificate::SSLMode::ssl_enable);

	ClassDB::bind_method(D_METHOD("get_connection_collation"), &SqlConnection::get_connection_collation);
	ClassDB::bind_method(D_METHOD("set_connection_collation", "value"), &SqlConnection::set_ssl_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_collation", PROPERTY_HINT_ENUM), "set_connection_collation", "set_connection_collation");
	ADD_PROPERTY_DEFAULT("connection_collation", SqlConnection::MysqlCollations::default_collation);



	// AddressType
	BIND_ENUM_CONSTANT(host_and_port);
	BIND_ENUM_CONSTANT(unix_path);


	// ConnectionStatus
	BIND_ENUM_CONSTANT(DISCONNECTED);
	BIND_ENUM_CONSTANT(CONNECTED);


	// ConnType
	BIND_ENUM_CONSTANT(NONE);
	BIND_ENUM_CONSTANT(TCP);
	BIND_ENUM_CONSTANT(TCP_TLS);
	BIND_ENUM_CONSTANT(UNIX);
	
	
	// MysqlCollations
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






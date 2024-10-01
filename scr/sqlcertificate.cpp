

/* sqlcertificate.cpp */

#include "sqlcertificate.h"




Error SqlCertificate::add_certificate_authority(String p_CA){
	boost::system::error_code ec;
	const const_buffer & certificate = GDstring_to_SQLbuffer(p_CA);
	ssl_ctx->add_certificate_authority(certificate, ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::add_verify_path(String p_path){
	String path = ensure_global_path(p_path);
	boost::system::error_code ec;
	ssl_ctx->add_verify_path(GDstring_to_SQLstring(path), ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::load_verify_file(const String p_path){
	boost::system::error_code ec;
	String path = ensure_global_path(p_path);
	ssl_ctx->load_verify_file(GDstring_to_SQLstring(path), ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::set_default_verify_paths(){
	boost::system::error_code ec;
	ssl_ctx->set_default_verify_paths(ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::clear_options(const int p_ssl_options){
	boost::system::error_code ec;
	ssl_ctx->clear_options(

		static_cast<ssl::context_base::options>(p_ssl_options),

		ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::set_options(const int p_ssl_options){
	boost::system::error_code ec;
	ssl_ctx->set_options(static_cast<ssl::context_base::options>(p_ssl_options), ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


void SqlCertificate::configure_callback_password(const String p_write_password, const String p_read_password){
	read_password= GDstring_to_SQLstring(p_read_password);
	write_password = GDstring_to_SQLstring(p_write_password);
}


Error SqlCertificate::set_password_callback() {
	boost::system::error_code ec;
	if (read_password.empty() || write_password.empty()) {
		return ERR_INVALID_PARAMETER;
	}
	ssl_ctx->set_password_callback(
		[this](std::size_t max_length, ssl::context_base::password_purpose purpose) -> std::string {

			std::string password;
			if (purpose == ssl::context_base::password_purpose::for_reading){
				password = read_password;
			}
			else if (purpose == ssl::context_base::password_purpose::for_writing){
				password = write_password;
			}
			else{
				return "";
			}
			if (password.size() > max_length) {
				return password.substr(0, max_length);
			}
			return password;
		},
		ec
	);
	BOOST_EXCEPTION(ec, &last_ssl_error, ERR_INVALID_PARAMETER);
	return OK;
}


Error SqlCertificate::set_verify_depth(const int depth){
	boost::system::error_code ec;
	ssl_ctx->set_verify_depth(depth, ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::set_verify_mode(const int p_mode){
	boost::system::error_code ec;
	ssl_ctx->set_verify_mode(static_cast<ssl::verify_mode>(p_mode), ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::use_certificate(const String p_certificate, const FileFormat file_format){
	boost::system::error_code ec;
	const const_buffer & certificate = GDstring_to_SQLbuffer(p_certificate);
	ssl_ctx->use_certificate(
		certificate,
		static_cast<ssl::context::file_format>(file_format),
		ec
	);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}

Error SqlCertificate::use_certificate_chain(const String p_chain){
	boost::system::error_code ec;
	const const_buffer & chain = GDstring_to_SQLbuffer(p_chain);
	ssl_ctx->use_certificate_chain(chain, ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}

Error SqlCertificate::use_certificate_chain_file(const String p_filename){
	boost::system::error_code ec;
	String filename = ensure_global_path(p_filename);
	ssl_ctx->use_certificate_chain_file(GDstring_to_SQLstring(filename), ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}

Error SqlCertificate::use_certificate_file(const String p_filename, const FileFormat p_format){
	boost::system::error_code ec;
	String filename = ensure_global_path(p_filename);
	ssl_ctx->use_certificate_file(
		GDstring_to_SQLstring(filename),
		static_cast<ssl::context::file_format>(p_format),
		ec
	);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}

Error SqlCertificate::use_private_key(const String p_private_key, const FileFormat p_format){
	boost::system::error_code ec;
	const const_buffer & private_key = GDstring_to_SQLbuffer(p_private_key);

	ssl_ctx->use_private_key(
		private_key,
		static_cast<ssl::context::file_format>(p_format),
		ec
	);

	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::use_private_key_file(const String p_filename, const FileFormat p_format){
	boost::system::error_code ec;
	String filename = ensure_global_path(p_filename);
	ssl_ctx->use_private_key_file(
		GDstring_to_SQLstring(filename),
		static_cast<ssl::context::file_format>(p_format),
		ec
	);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::use_rsa_private_key(const String p_private_key, const FileFormat p_format){
	boost::system::error_code ec;
	const const_buffer & private_key = GDstring_to_SQLbuffer(p_private_key);
	ssl_ctx->use_rsa_private_key(private_key, static_cast<ssl::context::file_format>(p_format),	ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::use_rsa_private_key_file(const String p_filename, const FileFormat p_format){
	boost::system::error_code ec;
	String filename = ensure_global_path(p_filename);
	ssl_ctx->use_rsa_private_key_file(
		GDstring_to_SQLstring(filename),
		static_cast<ssl::context::file_format>(p_format),
		ec
	);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::use_tmp_dh(const String p_dh_buffer){
	boost::system::error_code ec;
	const const_buffer & dh_buffer = GDstring_to_SQLbuffer(p_dh_buffer);
	ssl_ctx->use_tmp_dh(dh_buffer, ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}


Error SqlCertificate::use_tmp_dh_file(const String p_filename){
	boost::system::error_code ec;
	String filename = ensure_global_path(p_filename);
	ssl_ctx->use_tmp_dh_file(GDstring_to_SQLstring(filename), ec);
	BOOST_EXCEPTION(ec, &last_ssl_error, FAILED);
	return OK;
}



void SqlCertificate::start_certificate(SSLMethods p_method){
	ssl_ctx.reset();
	ssl_ctx = std::make_shared<asio::ssl::context>(static_cast<boost::asio::ssl::context>(p_method));
}


void SqlCertificate::_bind_methods() {

	ClassDB::bind_method(D_METHOD("start_certificate", "ssl method"), &SqlCertificate::add_certificate_authority);

	ClassDB::bind_method(D_METHOD("add_certificate_authority", "CA"), &SqlCertificate::add_certificate_authority);
	ClassDB::bind_method(D_METHOD("add_verify_path", "path"), &SqlCertificate::add_verify_path);
	ClassDB::bind_method(D_METHOD("clear_options", "ssl options"), &SqlCertificate::clear_options);
	ClassDB::bind_method(D_METHOD("set_options", "ssl options"), &SqlCertificate::set_options);
	ClassDB::bind_method(D_METHOD("load_verify_file", "path"), &SqlCertificate::load_verify_file);
	ClassDB::bind_method(D_METHOD("set_default_verify_paths"), &SqlCertificate::set_default_verify_paths);
	ClassDB::bind_method(D_METHOD("set_password_callback"), &SqlCertificate::set_password_callback);
	ClassDB::bind_method(D_METHOD("configure_callback_password", "write password", "read password"), &SqlCertificate::configure_callback_password);
	ClassDB::bind_method(D_METHOD("set_verify_depth", "depth"), &SqlCertificate::set_verify_depth);
	ClassDB::bind_method(D_METHOD("set_verify_mode", "mode"), &SqlCertificate::set_verify_mode);
	ClassDB::bind_method(D_METHOD("use_certificate", "certificate", "file format"), &SqlCertificate::use_certificate);
	ClassDB::bind_method(D_METHOD("use_certificate_chain", "chain"), &SqlCertificate::use_certificate_chain);
	ClassDB::bind_method(D_METHOD("use_certificate_chain_file", "filename"), &SqlCertificate::use_certificate_chain_file);
	ClassDB::bind_method(D_METHOD("use_certificate_file", "filename", "format"), &SqlCertificate::use_certificate_file);
	ClassDB::bind_method(D_METHOD("use_private_key", "private_key", "format"), &SqlCertificate::use_private_key);
	ClassDB::bind_method(D_METHOD("use_private_key_file", "filename", "format"), &SqlCertificate::use_private_key_file);
	ClassDB::bind_method(D_METHOD("use_rsa_private_key", "private_key", "format"), &SqlCertificate::use_rsa_private_key);
	ClassDB::bind_method(D_METHOD("use_rsa_private_key_file", "filename", "format"), &SqlCertificate::use_rsa_private_key_file);
	ClassDB::bind_method(D_METHOD("use_tmp_dh", "dh_buffer"), &SqlCertificate::use_tmp_dh);
	ClassDB::bind_method(D_METHOD("use_tmp_dh_file", "filename"), &SqlCertificate::use_tmp_dh_file);


	// FileFormat
	BIND_ENUM_CONSTANT(FileFormat::asn1);
	BIND_ENUM_CONSTANT(FileFormat::pem);

	// VerifyMode
	BIND_ENUM_CONSTANT(verify_none);
	BIND_ENUM_CONSTANT(verify_peer);
	BIND_ENUM_CONSTANT(verify_fail_if_no_peer_cert);
	BIND_ENUM_CONSTANT(verify_client_once);


	// PasswordPurpose
	BIND_ENUM_CONSTANT(PasswordPurpose::for_reading);
	BIND_ENUM_CONSTANT(PasswordPurpose::for_writing);

	// SSLMode
	BIND_ENUM_CONSTANT(ssl_disable);
	BIND_ENUM_CONSTANT(ssl_enable);
	BIND_ENUM_CONSTANT(ssl_require);

	// SSLOptions
	BIND_ENUM_CONSTANT(VERIFY_PEER);
	BIND_ENUM_CONSTANT(DEFAULT_WORKAROUNDS);
	BIND_ENUM_CONSTANT(NO_SSLV2);
	BIND_ENUM_CONSTANT(NO_SSLV3);
	BIND_ENUM_CONSTANT(NO_TLSV1);
	BIND_ENUM_CONSTANT(NO_TLSV1_1);
	BIND_ENUM_CONSTANT(NO_TLSV1_2);
	BIND_ENUM_CONSTANT(NO_TLSV1_3);
	BIND_ENUM_CONSTANT(SINGLE_DH_USE);

	// SslMethods
	BIND_ENUM_CONSTANT(SSLMethods::sslv2);			/// Generic SSL version 2.
	BIND_ENUM_CONSTANT(SSLMethods::sslv2_client);	/// SSL version 2 client.
	BIND_ENUM_CONSTANT(SSLMethods::sslv2_server);	/// SSL version 2 server.
	BIND_ENUM_CONSTANT(SSLMethods::sslv3);			/// Generic SSL version 3.
	BIND_ENUM_CONSTANT(SSLMethods::sslv3_client);	/// SSL version 3 client.
	BIND_ENUM_CONSTANT(SSLMethods::sslv3_server);	/// SSL version 3 server.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv1);			/// Generic TLS version 1.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv1_client);	/// TLS version 1 client.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv1_server);	/// TLS version 1 server.
	BIND_ENUM_CONSTANT(SSLMethods::sslv23);			/// Generic SSL/TLS.
	BIND_ENUM_CONSTANT(SSLMethods::sslv23_client);	/// SSL/TLS client.
	BIND_ENUM_CONSTANT(SSLMethods::sslv23_server);	/// SSL/TLS server.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv11);			/// Generic TLS version 1.1.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv11_client);	/// TLS version 1.1 client.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv11_server);	/// TLS version 1.1 server.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv12);			/// Generic TLS version 1.2.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv12_client);	/// TLS version 1.2 client.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv12_server);	/// TLS version 1.2 server.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv13);			/// Generic TLS version 1.3.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv13_client);	/// TLS version 1.3 client.
	BIND_ENUM_CONSTANT(SSLMethods::tlsv13_server);	/// TLS version 1.3 server.
	BIND_ENUM_CONSTANT(SSLMethods::tls);			/// Generic TLS.
	BIND_ENUM_CONSTANT(SSLMethods::tls_client);		/// TLS client.
	BIND_ENUM_CONSTANT(SSLMethods::tls_server);		/// TLS server.






}



SqlCertificate::SqlCertificate(){}
SqlCertificate::~SqlCertificate(){}
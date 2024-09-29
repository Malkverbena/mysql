

/* sqlcertificate.cpp */

#include "sqlcertificate.h"




Error SqlCertificate::add_certificate_authority(String CA){
	boost::system::error_code & ec;
	ssl_ctx->add_certificate_authority(GdtStr2SqlStr(CA), ec);
	BOOST_EXCEPTION(ec, &ssl_error, FAILED);
	return OK;
	// TODO Criar rotina de retorno do erro
}


Error SqlCertificate::add_verify_path(String p_path){
	boost::system::error_code & ec;
	ssl_ctx->add_verify_path(GdtStr2SqlStr(p_path), ec);
	BOOST_EXCEPTION(ec, &ssl_error, FAILED);
	return OK
}


Error SqlCertificate::load_verify_file(const String p_path){
	boost::system::error_code & ec;
	ssl_ctx->load_verify_file(GdtStr2SqlStr(p_path), ec);
	BOOST_EXCEPTION(ec, &ssl_error, FAILED);
	return OK
}


Error SqlCertificate::set_default_verify_paths(){
	boost::system::error_code & ec;
	ssl_ctx->set_default_verify_paths(ec);
	BOOST_EXCEPTION(ec, &ssl_error, FAILED);
	return OK
}


















void SqlCertificate::start_certificate(Ssl_Methods p_method){

	ssl_ctx = std::make_shared<asio::ssl::context>(asio::ssl::context(p_method));
	
}


void SqlCertificate::_bind_methods() {
}
SqlCertificate::SqlCertificate(){}
SqlCertificate::~SqlCertificate(){}
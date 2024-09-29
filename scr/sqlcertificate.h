
/* sqlcertificate.h */

//https://live.boost.org/doc/libs/1_86_0/doc/html/boost_asio/reference/ssl__context.html



#ifndef SQLCERTIFICATE_H
#define SQLCERTIFICATE_H


#include "helpers.h"

#include <boost/mysql/ssl_mode.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/ssl/context_base.hpp>


using namespace sqlhelpers;
using namespace boost::asio;


class SqlCertificate : public RefCounted {
	GDCLASS(SqlCertificate, RefCounted);
	friend class MySQL;
	friend class SqlConnection;


public:

	using Ssl_Methods = ssl::context_base::method;



enum SslMode {
	ssl_disable	= int(boost::mysql::ssl_mode::disable),
	ssl_enable	= int(boost::mysql::ssl_mode::enable),
	ssl_require	= int(boost::mysql::ssl_mode::require)
};

	SqlCertificate();
	~SqlCertificate();


private:

	Dictionary ssl_error;
	std::shared_ptr<asio::ssl::context> ssl_ctx = nullptr;


	void start_certificate(Ssl_Methods p_method = Ssl_Methods(asio::ssl::context::tls_client)); // Inicia o contexto SSL
//	void stop_certificate();                     // deleta  todo o contexto


	Error add_certificate_authority(const String CA);	//Add certification authority for performing verification.
	Error add_verify_path(const String p_path);			// Add a directory containing certificate authority files to be used for performing verification.
//	Error clear_options();								//Clear options on the context.
	Error load_verify_file(const String p_path);		// Load a certification authority file for performing verification.
//	Error native_handle();				// Get the underlying implementation in the native type.
//	Error set_default_verify_paths();	// Configures the context to use the default directories for finding certification authority certificates.
//	Error set_options();					// Set options on the context.
//	Error set_password_callback();		// Set the password callback.
//	Error set_verify_callback();			// Set the callback used to verify peer certificates.
//	Error set_verify_depth();			// Set the peer verification depth.
//	Error set_verify_mode();				// Set the peer verification mode.
//	Error use_certificate();				// Use a certificate from a memory buffer.
//	Error use_certificate_chain();		// Use a certificate chain from a memory buffer.
//	Error use_certificate_chain_file();	// Use a certificate chain from a file.
//	Error use_certificate_file();		// Use a certificate from a file.
//	Error use_private_key();				// Use a private key from a memory buffer.
//	Error use_private_key_file();		// Use a private key from a file.
//	Error use_rsa_private_key();			// Use an RSA private key from a memory buffer.
//	Error use_rsa_private_key_file();	// Use an RSA private key from a file.
//	Error use_tmp_dh();					// Use the specified memory buffer to obtain the temporary Diffie-Hellman parameters.
//	Error use_tmp_dh_file();				// Use the specified file to obtain the temporary Diffie-Hellman parameters.





protected:

	static void _bind_methods();


};


VARIANT_ENUM_CAST(SqlCertificate::SslMode);
VARIANT_ENUM_CAST(SqlCertificate::Ssl_Methods);



#endif // SQLCERTIFICATE_H


/*
SslMethods: 

 /// Generic SSL version 2.
	sslv2,

	/// SSL version 2 client.
	sslv2_client,

	/// SSL version 2 server.
	sslv2_server,

	/// Generic SSL version 3.
	sslv3,

	/// SSL version 3 client.
	sslv3_client,

	/// SSL version 3 server.
	sslv3_server,

	/// Generic TLS version 1.
	tlsv1,

	/// TLS version 1 client.
	tlsv1_client,

	/// TLS version 1 server.
	tlsv1_server,

	/// Generic SSL/TLS.
	sslv23,

	/// SSL/TLS client.
	sslv23_client,

	/// SSL/TLS server.
	sslv23_server,

	/// Generic TLS version 1.1.
	tlsv11,

	/// TLS version 1.1 client.
	tlsv11_client,

	/// TLS version 1.1 server.
	tlsv11_server,

	/// Generic TLS version 1.2.
	tlsv12,

	/// TLS version 1.2 client.
	tlsv12_client,

	/// TLS version 1.2 server.
	tlsv12_server,

	/// Generic TLS version 1.3.
	tlsv13,

	/// TLS version 1.3 client.
	tlsv13_client,

	/// TLS version 1.3 server.
	tlsv13_server,

	/// Generic TLS.
	tls,

	/// TLS client.
	tls_client,

	/// TLS server.
	tls_server
*/
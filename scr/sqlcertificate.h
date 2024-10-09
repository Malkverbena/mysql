
/* sqlcertificate.h */

//https://live.boost.org/doc/libs/1_86_0/doc/html/boost_asio/reference/ssl__context.html



#ifndef SQLCERTIFICATE_H
#define SQLCERTIFICATE_H


#include "helpers.h"


using namespace sqlhelpers;
using namespace boost::asio;


extern std::mutex connection_mutex;


class SqlCertificate : public RefCounted {
	GDCLASS(SqlCertificate, RefCounted);
	friend class MySQL;
	friend class SqlConnection;


public:

	using SSLMethods = boost::asio::ssl::context_base::method;
	using FileFormat = boost::asio::ssl::context_base::file_format;
	using PasswordPurpose = boost::asio::ssl::context_base::password_purpose;

enum SSLMode {
	ssl_disable	= int(boost::mysql::ssl_mode::disable),
	ssl_enable	= int(boost::mysql::ssl_mode::enable),
	ssl_require	= int(boost::mysql::ssl_mode::require)
};

enum VerifyMode{
	verify_none = int(ssl::verify_none),
	verify_peer = int(ssl::verify_peer),
	verify_fail_if_no_peer_cert = int(ssl::verify_fail_if_no_peer_cert),
	verify_client_once = int(ssl::verify_client_once),
};

	enum SSLOptions {
		VERIFY_PEER = boost::asio::ssl::context::verify_peer,
		SINGLE_DH_USE = boost::asio::ssl::context::single_dh_use,
		DEFAULT_WORKAROUNDS = boost::asio::ssl::context::default_workarounds,
		NO_SSLV2 = boost::asio::ssl::context::no_sslv2,
		NO_SSLV3 = boost::asio::ssl::context::no_sslv3,
		NO_TLSV1 = boost::asio::ssl::context::no_tlsv1,
		NO_TLSV1_1 = boost::asio::ssl::context::no_tlsv1_1,
		NO_TLSV1_2 = boost::asio::ssl::context::no_tlsv1_2,
		NO_TLSV1_3 = boost::asio::ssl::context::no_tlsv1_3,
	};


	SqlCertificate();
	~SqlCertificate();


private:

	std::string read_password;
	std::string write_password;
	std::shared_ptr<asio::ssl::context> ssl_ctx = nullptr;

//	std::shared_ptr<asio::ssl::context> get_context() { return ssl_ctx; }
	bool is_valid() { return (ssl_ctx != nullptr); }


public:

	// Godot specific.   ===========================================================

	// Star5ts the ssl context.
	void start_certificate(SSLMethods p_method = SSLMethods(asio::ssl::context::tls_client)); // Inicia o contexto SSL


	// SSL context methods.  ===========================================================

	// Add certification authority for performing verification.
	// This function is used to add one trusted certification authority from a memory buffer.
	// Parameters: "ca": The buffer containing the certification authority certificate. The certificate must use the PEM format.
	Dictionary add_certificate_authority(const String p_CA);

	// Add a directory containing certificate authority files to be used for performing verification.
	// This function is used to specify the name of a directory containing certification authority certificates.
	// Each file in the directory must contain a single certificate. The files must be named using the subject name's hash and an extension of ".0".
	// Parameters: "path": The name of a directory containing the certificates.
	Dictionary add_verify_path(const String p_path);

	// Clear options on the context.
	// This function may be used to configure the SSL options used by the context.
	// Parameters: "ssl_options": A bitmask of options. The available option values are defined in the ssl::context_base class.
	// Use the enum SSLOptions to get the values, This enum corresponds to "boost::asio::ssl::context".
	Dictionary clear_options(const int ssl_options);

	// Set options on the context.
	// A bitmask of options. The available option values are defined in the ssl::context_base class.
	// The options are bitwise-ored with any existing value for the options.
	// Parameters: "ssl_options": A bitmask of options. The available option values are defined in the ssl::context_base class.
	// Use the enum SSLOptions to get the values, This enum corresponds to "boost::asio::ssl::context".
	Dictionary set_options(const int ssl_options);


	// Load a certification authority file for performing verification.
	// This function is used to load the certificates for one or more trusted certification authorities from a file.
	// Parameters: "filename" The name of a file containing certification authority certificates in PEM format.
	Dictionary load_verify_file(const String p_path);

	//Configures the context to use the default directories for finding certification authority certificates.
	// This function specifies that the context should use the default, system-dependent directories for locating certification authority certificates.
	Dictionary set_default_verify_paths();

	// Set the password callback.
	// This function is used to specify a callback function to obtain password information about an encrypted key in PEM format.
	// Parameters:
	//			"max_length": The maximum size for a password.
	//			"purpose": Whether password is for reading or writing.
	// The return value of the callback is a string containing the password.
	// The function "configure_callback_password" must be called first to configure the passwords.
	Dictionary set_password_callback();

	// Configure the passwords callbacks.
	void configure_callback_password(const String p_write_password, const String p_read_password);

	// Set the peer verification depth.
	// This function may be used to configure the maximum verification depth allowed by the context.
	// Parameters: "depth": Maximum depth for the certificate chain verification that shall be allowed.
	Dictionary set_verify_depth(const int depth);

	// Set the peer verification mode.
	// This function may be used to configure the peer verification mode used by the context.
	// Parameters: "mode" A bitmask of peer verification modes. See ssl::verify_mode for available values.
	// Use the enum VerifyMode in the flags.
	Dictionary set_verify_mode(const int p_mode);

	// Use a certificate from a memory buffer.
	// This function is used to load a certificate into the context from a buffer.
	// Parameters:
	//		"certificate": The buffer containing the certificate.
	//		"format": The certificate format (ASN.1 or PEM).
	Dictionary use_certificate(const String p_certificate, const FileFormat file_format);

	// Use a certificate chain from a memory buffer.
	// This function is used to load a certificate chain into the context from a buffer.
	// Parameters: "chain": The buffer containing the certificate chain. The certificate chain must use the PEM format.
	Dictionary use_certificate_chain(const String p_chain);

	// Use a certificate chain from a file.
	// This function is used to load a certificate chain into the context from a file.
	// Parameters: "filename": The path to file containing the certificate. The file must use the PEM format.
	Dictionary use_certificate_chain_file(const String p_filename);

	//Use a certificate from a file.
	// This function is used to load a certificate into the context from a file.
	// Parameters:
	//		"filename": The name of the file containing the certificate.
	//		"format": The file format (ASN.1 or PEM).
	Dictionary use_certificate_file(const String p_filename, const FileFormat p_format);


	// Use a private key from a memory buffer.
	// This function is used to load a private key into the context from a buffer.
	// Parameters:
	//		"private_key": The buffer containing the private key.
	//		"format": The private key format (ASN.1 or PEM).
	Dictionary use_private_key(const String p_private_key, const FileFormat p_format);

	// Use a private key from a file.
	// This function is used to load a private key into the context from a file.
	// Parameters
	//		"filename": The name of the file containing the private key.
	//		"format": The file format (ASN.1 or PEM).
	Dictionary use_private_key_file(const String p_filename, const FileFormat p_format);

	// Use an RSA private key from a memory buffer.
	// This function is used to load an RSA private key into the context from a buffer.
	// Parameters:
	//		"private_key": The buffer containing the RSA private key.
	//		"format": The private key format (ASN.1 or PEM).
	Dictionary use_rsa_private_key(const String p_private_key, const FileFormat p_format);

	// Use an RSA private key from a file.
	// This function is used to load an RSA private key into the context from a file.
	// Parameters
	//		"filename": The name of the file containing the RSA private key.
	//		"format": The file format (ASN.1 or PEM).
	Dictionary use_rsa_private_key_file(const String p_filename, const FileFormat p_format);

	// Use the specified memory buffer to obtain the temporary Diffie-Hellman parameters.
	// This function is used to load Diffie-Hellman parameters into the context from a buffer.
	// Parameters: "dh_buffer": The memory buffer containing the Diffie-Hellman parameters. The buffer must use the PEM format.
	Dictionary use_tmp_dh(const String p_dh_buffer);

	// Use the specified file to obtain the temporary Diffie-Hellman parameters.
	// This function is used to load Diffie-Hellman parameters into the context from a file.
	// Parameters: "filename": The name of the file containing the Diffie-Hellman parameters. The file must use the PEM format.
	Dictionary use_tmp_dh_file(const String p_filename);





protected:

	static void _bind_methods();



};



VARIANT_ENUM_CAST(SqlCertificate::SSLMode);
VARIANT_ENUM_CAST(SqlCertificate::FileFormat);
VARIANT_ENUM_CAST(SqlCertificate::SSLMethods);
VARIANT_ENUM_CAST(SqlCertificate::SSLOptions);
VARIANT_ENUM_CAST(SqlCertificate::VerifyMode);
VARIANT_ENUM_CAST(SqlCertificate::PasswordPurpose);





#endif // SQLCERTIFICATE_H


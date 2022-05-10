/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H

#ifdef GODOT4
	#include "core/object/ref_counted.h"
	#include "core/core_bind.h"
#else
	#include "core/reference.h"
	#include "core/bind/core_bind.h"
#endif

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <mysql_error.h>

#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/metadata.h>
#include <cppconn/parameter_metadata.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/statement.h>

//#include <cppconn/datatype.h>

//#include <cppconn/sqlstring.h>
//#include <cppconn/warning.h>
//#include <cppconn/variant.h>


//#include <cppconn/build_config.h>
//#include <cppconn/version_info.h>
//#include <cppconn/config.h>


#pragma once

#ifdef GODOT4
class MySQL : public RefCounted{
	GDCLASS(MySQL, RefCounted);

#else
class MySQL : public Reference{
	GDCLASS(MySQL, Reference);
#endif


private:

	/*       ERROS        */
	typedef Dictionary MySQLException;
	MySQLException sqlError;
	void print_SQLException( sql::SQLException &e );
	void print_runtime_error( std::runtime_error &e );

	/*      CONNECTIOn    */
	sql::ConnectOptionsMap connection_properties;
	sql::mysql::MySQL_Driver *driver;
	std::shared_ptr<sql::Connection> conn;

	/*      HEELPERS      */
	String string_SQL_2_GDT ( sql::SQLString &p_string );

	enum TYPE {
		INVALID,
		BOOL,
		INT,
		LISTA,
		MAP,
		STRING,
		VOID,
	};




#ifdef GODOT4
	typedef PackedStringArray PoolStringArray;
#endif

	PoolStringArray prop_string = String("hostName, userName, password, schema, pipe, pluginDir, postInit, preInit, readDefaultFile, readDefaultGroup, rsaKey, socket, sslCA, sslCAPath, sslCert, sslCipher, sslCRL, sslCRLPath, sslKey, characterSetResults, charsetDir, defaultAuth, defaultPreparedStatementResultType, defaultStatementResultType, OPT_TLS_VERSION, OPT_CHARSET_NAME").rsplit(", ");
	PoolStringArray prop_bool = String("OPT_RECONNECT, CLIENT_COMPRESS, CLIENT_FOUND_ROWS, CLIENT_IGNORE_SIGPIPE, CLIENT_IGNORE_SPACE, CLIENT_INTERACTIVE, CLIENT_LOCAL_FILES, CLIENT_MULTI_STATEMENTS, CLIENT_NO_SCHEMA, OPT_REPORT_DATA_TRUNCATION, OPT_CAN_HANDLE_EXPIRED_PASSWORDS, OPT_ENABLE_CLEARTEXT_PLUGIN, OPT_GET_SERVER_PUBLIC_KEY, sslEnforce, sslVerify, useLegacyAuth").rsplit(", ");
	PoolStringArray prop_int = String("port, OPT_CONNECT_TIMEOUT, OPT_LOCAL_INFILE, OPT_WRITE_TIMEOUT, OPT_READ_TIMEOUT").rsplit(", ");
	PoolStringArray prop_map = String("OPT_CONNECT_ATTR_ADD").rsplit(", ");
	PoolStringArray prop_list = String("OPT_CONNECT_ATTR_DELETE").rsplit(", ");
	PoolStringArray prop_void = String("OPT_CONNECT_ATTR_RESET, OPT_NAMED_PIPE").rsplit(", ");
	int	get_prop_type( String p_prop );


protected:

	static void _bind_methods();



public:




	enum ConnectionStatus {
		NO_CONNECTION,
		CLOSED,
		CONNECTED,
		DISCONNECTED
	};


	/*     CONNECTIONS    */
	void set_credentials( String p_host, String p_user, String p_pass );
	ConnectionStatus connection_start();
	ConnectionStatus connection_stop();
	ConnectionStatus connection_status();
	Dictionary get_metadata();


	/*     PROPERTIES     */

	Error set_property(String p_property, Variant p_value);
	Variant get_property(String p_property);

	Error set_properties_array(Dictionary p_properties);
	Dictionary get_properties_array(Array p_properties);

	Error set_database( String p_database );
	String get_database();

//	Error set_client_option(String "p_option", Variant "p_value");
//	Variant get_client_option(String "p_option" );
//	void set_session_variable( String p_varname, String p_value );
//	String get_session_variable( String p_varname );


	// QUERY
	Array query(String p_sqlquery, DataFormat data_model = DICTIONARY, bool return_string = false, PoolIntArray meta_col = PoolIntArray());
	Array query_prepared(String p_sqlquery, Array prep_values = Array(), DataFormat data_model = DICTIONARY, bool return_string = false, PoolIntArray meta_col = PoolIntArray());



	Variant teste(String p_property);

	MySQL();
	~MySQL();

};


VARIANT_ENUM_CAST(MySQL::ConnectionStatus);



#endif
// MYSQL_H





//---------------------------------------------------------------------------------------------
/*
https://dev.mysql.com/doc/refman/8.0/en/blob.html  NOTE: Afeta o desempenho
https://mariadb.com/kb/en/json-data-type/
*/

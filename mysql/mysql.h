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


public:

#ifdef GODOT4
	typedef PackedStringArray PoolStringArray;
	typedef PackedInt32Array PoolIntArray;
#endif

//	typedef sql::enum_transaction_isolation Isolation;


	enum Isolation { // aka sql::enum_transaction_isolation
		TRANSACTION_ERROR = -1,
		TRANSACTION_NONE = sql::enum_transaction_isolation::TRANSACTION_NONE,
		TRANSACTION_READ_COMMITTED = sql::enum_transaction_isolation::TRANSACTION_READ_COMMITTED,
		TRANSACTION_READ_UNCOMMITTED = sql::enum_transaction_isolation::TRANSACTION_READ_UNCOMMITTED,
		TRANSACTION_REPEATABLE_READ = sql::enum_transaction_isolation::TRANSACTION_REPEATABLE_READ,
		TRANSACTION_SERIALIZABLE = sql::enum_transaction_isolation::TRANSACTION_SERIALIZABLE
	};



	enum MetaCollection {
		COLUMNS_NAMES,	// returns an array with only one element (names of columns)
		COLUMNS_TYPES,	// returns an array with only one element (types of the columns)
		ATTRIBUTES,		// returns a dictionary with column atributes ( )
		TABLE_INFO,		// return a dictionary with table info
		NO_QUERY,		// DO NOT return the result of querty. Useful only if you wish the metadata only.
	};

	enum ConnectionStatus {
		NO_CONNECTION,	// No connection set
		CLOSED,			// Connection was started but closed
		CONNECTED,		// Connected and operational
		DISCONNECTED	// Disconnected by server or client
	};

	enum DataFormat {
		ARRAY,			// returns data as an array
		DICTIONARY		// returns data as an dictionary
	};


private:

	enum OP {
		AUTOCOMMIT,
		CATALOG,
		CLIENT_INFO,
		DATABASE,
		ISOLATION,
		READONLY,
	};

	enum TYPE {
		INVALID,
		BOOL,
		INT,
		LIST,
		MAP,
		STRING,
		VOID,
	};

	/*       ERROS        */
	typedef Dictionary MySQLException;
	MySQLException sqlError;
	void print_SQLException( sql::SQLException &e );
	void print_runtime_error( std::runtime_error &e );

	/*      CONNECTION    */
	sql::ConnectOptionsMap connection_properties;
	sql::mysql::MySQL_Driver *driver;
	std::shared_ptr<sql::Connection> conn;

	/*       HELPERS      */
	Array format_time( String str, bool return_string );
	String string_SQL_2_GDT ( sql::SQLString &p_string );
	bool is_mat_empty( Variant p_matrix );
	bool is_json( Variant p_arg );
	bool is_mysql_time( String time );
	int	get_prop_type( String p_prop );
	int get_time_format( String time );
	void set_datatype( std::shared_ptr<sql::PreparedStatement> prep_stmt, std::stringstream *blob, Variant arg, int index );
	PoolStringArray prop_string = String("hostName, userName, password, schema, pipe, pluginDir, postInit, preInit, readDefaultFile, readDefaultGroup, rsaKey, socket, sslCA, sslCAPath, sslCert, sslCipher, sslCRL, sslCRLPath, sslKey, characterSetResults, charsetDir, defaultAuth, defaultPreparedStatementResultType, defaultStatementResultType, OPT_TLS_VERSION, OPT_CHARSET_NAME").rsplit(", ");
	PoolStringArray prop_bool = String("OPT_RECONNECT, CLIENT_COMPRESS, CLIENT_FOUND_ROWS, CLIENT_IGNORE_SIGPIPE, CLIENT_IGNORE_SPACE, CLIENT_INTERACTIVE, CLIENT_LOCAL_FILES, CLIENT_MULTI_STATEMENTS, CLIENT_NO_SCHEMA, OPT_REPORT_DATA_TRUNCATION, OPT_CAN_HANDLE_EXPIRED_PASSWORDS, OPT_ENABLE_CLEARTEXT_PLUGIN, OPT_GET_SERVER_PUBLIC_KEY, sslEnforce, sslVerify, useLegacyAuth").rsplit(", ");
	PoolStringArray prop_int = String("port, OPT_CONNECT_TIMEOUT, OPT_LOCAL_INFILE, OPT_WRITE_TIMEOUT, OPT_READ_TIMEOUT").rsplit(", ");
	PoolStringArray prop_map = String("OPT_CONNECT_ATTR_ADD").rsplit(", ");
	PoolStringArray prop_list = String("OPT_CONNECT_ATTR_DELETE").rsplit(", ");
	PoolStringArray prop_void = String("OPT_CONNECT_ATTR_RESET, OPT_NAMED_PIPE").rsplit(", ");

	/*        CORE        */
	Array _query(String p_sqlquery, Array p_values, DataFormat data_model, bool return_as_string, PoolIntArray meta_col, bool _prep);
	int _execute( String p_sqlquery, Array p_values, bool prep_st, bool update);
	Error _set_conn_op( Variant p_value, OP op);
	Variant _get_conn_op(OP op);


protected:

	static void _bind_methods();


public:

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
	Dictionary get_properties_array(PoolStringArray p_properties);



	/*     CONNECTOR      */
//	Error set_client_option(String "p_option", Variant "p_value");
//	Variant get_client_option(String "p_option" );
//	void set_session_variable( String p_varname, String p_value );
//	String get_session_variable( String p_varname );


	/*      DATABASE      */
	Array query(String p_sqlquery, DataFormat data_model = DICTIONARY, bool return_as_string = false, PoolIntArray meta_col = PoolIntArray());
	Array query_prepared(String p_sqlquery, Array prep_values = Array(), DataFormat data_model = DICTIONARY, bool return_as_string = false, PoolIntArray meta_col = PoolIntArray());
	bool execute(String p_sqlquery);
	bool execute_prepared(String p_sqlquery, Array p_values);
	int update(String p_sqlquery);
	int update_prepared(String p_sqlquery, Array p_values);


	/*    TRANSACTION     */
//	void rollback_savepoint(String savepoint){conn->rollback( savepoint_map[savepoint] );}
	Error create_savepoint(String p_savept);
	Error delete_savepoint(String p_savept);


	/*     CONNECTION     */

	Error set_autocommit( bool autocommit ) { return _set_conn_op( autocommit, MySQL::OP::AUTOCOMMIT );}
	bool get_autocommit() { return _get_conn_op(MySQL::OP::AUTOCOMMIT);	}

	Error set_transaction_isolation( Isolation level ){ return _set_conn_op( level, MySQL::OP::ISOLATION ); }
	Isolation get_transaction_isolation() { int iso = (int)_get_conn_op(MySQL::OP::ISOLATION);	return (Isolation) iso;	}

	Error set_database( String database ) { return _set_conn_op( database, MySQL::OP::DATABASE ); }
	String get_database() { return _get_conn_op(MySQL::OP::DATABASE); }

	Error set_readyonly( bool readyonly ) { return _set_conn_op( readyonly, MySQL::OP::READONLY ); }
	String get_readyonly() { return _get_conn_op(MySQL::OP::READONLY); }

	Error set_catalog( String catalog) { return _set_conn_op( catalog, MySQL::OP::CATALOG );}
	String get_catalog() { return _get_conn_op(MySQL::OP::CATALOG);	}

	String get_client_info() { return _get_conn_op(MySQL::OP::CLIENT_INFO);	}


//	void commit(){conn->commit();}
//	void set_transaction_isolation( Isolation level) {conn->setTransactionIsolation( (sql::enum_transaction_isolation)(level));}
//	Isolation get_transaction_isolation(){return (Isolation)(conn->getTransactionIsolation()); }
//	Error create_savepoint(String p_savept);
//	Error delete_savepoint(String p_savept);
//	PoolStringArray get_savepoints();


	Variant teste(String p_property);

	MySQL();
	~MySQL();

};


VARIANT_ENUM_CAST(MySQL::ConnectionStatus);
VARIANT_ENUM_CAST(MySQL::MetaCollection);
VARIANT_ENUM_CAST(MySQL::DataFormat);
VARIANT_ENUM_CAST(MySQL::Isolation);


#endif  // MYSQL_H





//---------------------------------------------------------------------------------------------
/*
https://dev.mysql.com/doc/refman/8.0/en/blob.html  NOTE: Afeta o desempenho
https://mariadb.com/kb/en/json-data-type/
*/

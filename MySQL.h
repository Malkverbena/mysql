/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H

#ifdef GODOT4
	#include "core/object/ref_counted.h"  
	#include "core/core_bind.h"
#else
	#include "core/reference.h"
	#include "core/bind/core_bind.h"  
	//#include "core/crypto/crypto_core.h"
	//#include "core/error_macros.h"
#endif

#include <vector>
#include <sstream>


#include <mysql_connection.h>
#include <mysql_driver.h>
#include <mysql_error.h> 

#include <cppconn/build_config.h>
#include <cppconn/config.h>
#include <cppconn/connection.h>
#include <cppconn/datatype.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/metadata.h>
#include <cppconn/parameter_metadata.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/statement.h>
#include <cppconn/sqlstring.h>
#include <cppconn/warning.h>
#include <cppconn/version_info.h>
#include <cppconn/variant.h>


#pragma once

#ifdef GODOT4
class MySQL : public RefCounted{
	GDCLASS(MySQL,RefCounted);
#else
class MySQL : public Reference{
	GDCLASS(MySQL,Reference);
#endif


protected:
	static void _bind_methods();


public:

#ifdef GODOT4
	typedef PackedInt64Array PoolIntArray;
	typedef PackedByteArray PoolByteArray;
#endif

	typedef Dictionary MySQLException;
	MySQLException sqlError;
	std::string sqlstate ;

	enum Isolation {		// aka sql::enum_transaction_isolation
		TRANSACTION_NONE= 0,
		TRANSACTION_READ_COMMITTED,
		TRANSACTION_READ_UNCOMMITTED,
		TRANSACTION_REPEATABLE_READ,
		TRANSACTION_SERIALIZABLE
	};

	enum ConnectionStatus {
		NO_CONNECTION, 
		CLOSED, 
		CONNECTED, 
		DISCONNECTED
	};
	
	enum DataFormat {
		ARRAY,		// returns data as an array
		DICTIONARY	// returns data as an dictionary
	};
	
	enum MetaCollection {	// TODO: Use bitwise?
		COLUMNS_NAMES = 1,	// returns an array with only one element (names of columns)
		COLUMNS_TYPES = 2,	// returns an array with only one element (types of the columns)
		INFO		  = 4	// return the fields info (is...)
	};


private:
	sql::ConnectOptionsMap connection_properties;
	sql::mysql::MySQL_Driver *driver;
	std::shared_ptr<sql::Connection> conn;

#ifdef GODOT4
	core_bind::Marshalls *_marshalls = memnew(core_bind::Marshalls);
#else	
	_Marshalls *_marshalls = memnew(_Marshalls);
	//static _Marshalls *get_singleton();
#endif
	
	//FIXME  Use smart points here - conn->setSavepoint(avepoint) return a sql::Savepoint*
	//Map<String, std::unique_ptr<sql::Savepoint>> savepoint_map;
	std::map<String, sql::Savepoint*> savepoint_map;

	const std::string void_properties [2] = {  //void
		"OPT_CONNECT_ATTR_RESET", 
		"OPT_NAMED_PIPE"
	};
	
	const std::string sqlmap_properties [2] = { //map
		"OPT_CONNECT_ATTR_ADD",
		"OPT_CONNECT_ATTR_DELETE",
	};
	
	const std::string int_properties [5] = { 	//int
		"port",
		"OPT_CONNECT_TIMEOUT",
		"OPT_LOCAL_INFILE",
		"OPT_WRITE_TIMEOUT",
		"OPT_READ_TIMEOUT",
	};
	
	const std::string bool_properties [17] = { //bool
		"OPT_RECONNECT",
		"CLIENT_COMPRESS",
		"CLIENT_FOUND_ROWS",
		"CLIENT_IGNORE_SIGPIPE",
		"CLIENT_IGNORE_SPACE",
		"CLIENT_INTERACTIVE",
		"CLIENT_LOCAL_FILES",
		"CLIENT_MULTI_STATEMENTS",
		"CLIENT_MULTI_STATEMENTS",
		"CLIENT_NO_SCHEMA",
		"OPT_REPORT_DATA_TRUNCATION",
		"OPT_CAN_HANDLE_EXPIRED_PASSWORDS",
		"OPT_ENABLE_CLEARTEXT_PLUGIN",
		"OPT_GET_SERVER_PUBLIC_KEY",
		"sslEnforce",
		"sslVerify",
		"useLegacyAuth"
	};
	
	const std::string string_properties [26] = { //string
		"hostName",
		"userName",
		"password",
		"schema",
		"pipe",
		"pluginDir",
		"postInit",
		"preInit",
		"readDefaultFile",
		"readDefaultGroup",
		"rsaKey",
		"socket",
		"sslCA",
		"sslCAPath",
		"sslCert",
		"sslCipher",
		"sslCRL",
		"sslCRLPath",
		"sslKey",
		"characterSetResults",
		"charsetDir",
		"defaultAuth",
		"defaultPreparedStatementResultType",
		"defaultStatementResultType",
		"OPT_TLS_VERSION",
		"OPT_CHARSET_NAME"
	};

	//QUERTY
	int _execute( String p_sqlquery, Array p_values, bool prep_st, bool update);
	void set_datatype(std::shared_ptr<sql::PreparedStatement> prep_stmt, std::stringstream * blob, Variant arg, int index);
	Array _query(String p_sqlquery, Array p_values = Array(), DataFormat data_model = DICTIONARY, bool return_string = false, PoolIntArray meta_col = PoolIntArray(), bool _prep = false);


	//ERRORS
	void print_SQLException(sql::SQLException &e);
	void print_runtime_error(std::runtime_error &e);


	//CHECKERS
	std::string	get_prop_type( std::string p_prop );
	bool is_json( Variant p_arg );
	bool is_mysql_time(String time);
	
	
	//HELPERS
	Array format_time(String str, bool return_string);
	int get_time_format(String time);
	String SQLstr2GDstr( sql::SQLString &p_string );


public:
	bool encode_objects = true;
	void set_allow_objects(bool _encode_objects) {encode_objects = _encode_objects;}
	bool get_allow_objects() {return encode_objects;}
	
	void set_multi_statement(bool _multi_statement);
	bool get_multi_statement() {return (bool)get_property("CLIENT_MULTI_STATEMENTS");}

	void set_reconnection(bool _can_reconnect);
	bool get_reconnection() {return (bool)get_property("OPT_RECONNECT");}


 
	// CONNECTION
	Dictionary get_metadata();
	ConnectionStatus get_connection_status();
	ConnectionStatus stop_connection();
	MySQLException start_connection();
	void set_credentials( String p_host, String p_user, String p_pass ); // For quickly connections

	// PROPERTIES
	Variant get_property(String p_property);
	void set_property(String p_property, Variant p_value);
	Dictionary get_properties_array(Array p_properties);
	void set_properties_array(Dictionary p_properties);
	void set_database( String p_database );
	String get_database();


	// EXECUTE
	bool execute(String p_sqlquery);
	bool execute_prepared(String p_sqlquery, Array p_values);

	// EXECUTE UPDATE
	int update(String p_sqlquery);
	int update_prepared(String p_sqlquery, Array p_values);

	// QUERY
	Array query(String p_sqlquery, DataFormat data_model = DICTIONARY, bool return_string = false, PoolIntArray meta_col = PoolIntArray());  
	Array query_prepared(String p_sqlquery, Array prep_values = Array(), DataFormat data_model = DICTIONARY, bool return_string = false, PoolIntArray meta_col = PoolIntArray()); 


	// TRANSACTION
	void rollback_savepoint(String savepoint){conn->rollback( savepoint_map[savepoint] );}
	void set_auto_commit(bool autoCommit){conn->setAutoCommit(autoCommit);}
	bool get_auto_commit(){return conn->getAutoCommit();}
	void commit(){conn->commit();}
	void set_transaction_isolation( Isolation level) {conn->setTransactionIsolation( (sql::enum_transaction_isolation)(level));}	
	Isolation get_transaction_isolation(){return (Isolation)(conn->getTransactionIsolation()); }

	Error create_savepoint(String p_savept);
	Error delete_savepoint(String p_savept);


#ifdef GODOT4
	PackedStringArray get_savepoints();
#else	
	PoolStringArray get_savepoints();
#endif



	MySQL();
	~MySQL();

};


VARIANT_ENUM_CAST(MySQL::DataFormat);
VARIANT_ENUM_CAST(MySQL::MetaCollection);
VARIANT_ENUM_CAST(MySQL::ConnectionStatus);
VARIANT_ENUM_CAST(MySQL::Isolation);

#endif	// MYSQL_H


/*
#ifdef GODOT4

#else	

#endif
*/

	//TODO

	// TRANSACTION
	//Variant transaction( [] );
	//Variant transaction_prepared( {} );





	




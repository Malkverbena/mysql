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

	enum Isolation { // aka sql::enum_transaction_isolation plus extras from this modules 
		TRANSACTION_ERROR					= -1,
		TRANSACTION_NONE					= sql::enum_transaction_isolation::TRANSACTION_NONE,
		TRANSACTION_READ_COMMITTED		= sql::enum_transaction_isolation::TRANSACTION_READ_COMMITTED,
		TRANSACTION_READ_UNCOMMITTED	= sql::enum_transaction_isolation::TRANSACTION_READ_UNCOMMITTED,
		TRANSACTION_REPEATABLE_READ	= sql::enum_transaction_isolation::TRANSACTION_REPEATABLE_READ,
		TRANSACTION_SERIALIZABLE		= sql::enum_transaction_isolation::TRANSACTION_SERIALIZABLE
	};

	enum MetaCollection {
		COLUMNS_NAMES,	// returns an array with only one element (names of columns)
		COLUMNS_TYPES,	// returns an array with only one element (types of the columns)
		ATTRIBUTES,		// returns a dictionary with column atributes ( )
		TABLE_INFO,		// return a dictionary with table info
		NO_QUERY,		// DO NOT return the result of querty. This function is a waste of processing. Useful only if you want the metadata and nothing else.
	};

	enum ConnectionStatus {
		NO_CONNECTION,	// No connection set
		CLOSED,			// Connection was started but closed
		CONNECTED,		// Connected and operational
		DISCONNECTED	// Disconnected by server or client
	};

	enum DataFormat {
		ARRAY,		// returns data as an array
		DICTIONARY	// returns data as an dictionary
	};


private:

	enum OP {
		AUTOCOMMIT			= 1,
		CATALOG				= 2,
		CLIENT_INFO			= 3,
		DATABASE				= 4,
		ISOLATION			= 5,
		READONLY				= 6,
		CREATE_SAVEPOINT	= 7,
		DELETE_SAVEPOINT	= 8,
		COMMIT				= 9,
		ROLLBACK				= 10,
		DRIVER_INFO			= 11,
	};

	enum PROPERTY_TYPES {
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
	MySQLException _last_sql_error;
	void print_SQLException( sql::SQLException &e );
	void print_runtime_error( std::runtime_error &e );

	/*      CONNECTION    */
	sql::ConnectOptionsMap connection_properties;
	sql::mysql::MySQL_Driver *driver;
	std::shared_ptr<sql::Connection> conn;
	std::map <String, sql::Savepoint* > savepoint_map;

	/*        CORE        */
	Array _query(String p_sqlquery, Array p_values, DataFormat data_model, bool return_as_string, PoolIntArray meta_col, bool _prep);
	int _execute( String p_sqlquery, Array p_values, bool prep_st, bool update);
	Error _set_conn( Variant p_value, OP op);
	Variant _get_conn(OP op);

	/*       HELPERS      */
	Array format_time( String str, bool return_string );
	String string_SQL_2_GDT ( sql::SQLString &p_string );
//	bool is_mat_empty( Variant p_matrix );
	bool is_json( Variant p_arg );
	bool is_mysql_time( String time );
	int	get_prop_type( String p_prop );
	int get_time_format( String time );
	void _fit_statement( std::shared_ptr<sql::PreparedStatement> prep_stmt, std::stringstream *blob, Variant arg, int index );

	PoolStringArray prop_int = String("port, OPT_CONNECT_TIMEOUT, OPT_LOCAL_INFILE, OPT_WRITE_TIMEOUT, OPT_READ_TIMEOUT").rsplit(", ");
	PoolStringArray prop_map = String("OPT_CONNECT_ATTR_ADD").rsplit(", ");
	PoolStringArray prop_list = String("OPT_CONNECT_ATTR_DELETE").rsplit(", ");
	PoolStringArray prop_void = String("OPT_CONNECT_ATTR_RESET, OPT_NAMED_PIPE").rsplit(", ");
	PoolStringArray prop_string = String("hostName, userName, password, schema, pipe, pluginDir, postInit, preInit, readDefaultFile, readDefaultGroup, rsaKey, socket, sslCA, sslCAPath, sslCert, sslCipher, sslCRL, sslCRLPath, sslKey, characterSetResults, charsetDir, defaultAuth, defaultPreparedStatementResultType, defaultStatementResultType, OPT_TLS_VERSION, OPT_CHARSET_NAME").rsplit(", ");
	PoolStringArray prop_bool = String("OPT_RECONNECT, CLIENT_COMPRESS, CLIENT_FOUND_ROWS, CLIENT_IGNORE_SIGPIPE, CLIENT_IGNORE_SPACE, CLIENT_INTERACTIVE, CLIENT_LOCAL_FILES, CLIENT_MULTI_STATEMENTS, CLIENT_NO_SCHEMA, OPT_REPORT_DATA_TRUNCATION, OPT_CAN_HANDLE_EXPIRED_PASSWORDS, OPT_ENABLE_CLEARTEXT_PLUGIN, OPT_GET_SERVER_PUBLIC_KEY, sslEnforce, sslVerify, useLegacyAuth").rsplit(", ");
	

protected:

	static void _bind_methods();



public:

	/*     CONNECTION     */
	void set_credentials( String p_host, String p_user, String p_pass, String p_schema = String() );
	Dictionary get_last_error() { return _last_sql_error; }
	Dictionary get_connection_metadata();
	ConnectionStatus connection_start();
	ConnectionStatus connection_stop();
	ConnectionStatus connection_status();

	/*     PROPERTIES     */
	Error set_property(String p_property, Variant p_value);
	Variant get_property(String p_property);
	Error set_properties_array(Dictionary p_properties);
	Dictionary get_properties_array(PoolStringArray p_properties);

	/*    CONN OPTIONS    */
	Dictionary get_driver_info() { return _get_conn(DRIVER_INFO); }	// PASS
	String get_client_info() { return _get_conn( CLIENT_INFO);	}	// PASS
	Error set_client_option(String p_option, Variant p_value);
	Variant get_client_option( String p_option);					// PASS

	/*      DATABASE      */
	Array query(String p_sqlquery, DataFormat data_model = DICTIONARY, bool return_as_string = false, PoolIntArray meta_col = PoolIntArray());
	Array query_prepared(String p_sqlquery, Array prep_values = Array(), DataFormat data_model = DICTIONARY, bool return_as_string = false, PoolIntArray meta_col = PoolIntArray());
	bool execute(String p_sqlquery);
	bool execute_prepared(String p_sqlquery, Array p_values);
	int update(String p_sqlquery);
	int update_prepared(String p_sqlquery, Array p_values);

	/*     CONNECTOR      */
	Error set_readyonly( bool readyonly ) { return _set_conn( readyonly, READONLY ); }
	bool get_readyonly() { return _get_conn( READONLY ); }
	Error set_catalog( String catalog) { return _set_conn( catalog, CATALOG );}
	String get_catalog() { return _get_conn( CATALOG);	}

	Error set_database( String database ) {
		if ( connection_status() == CONNECTED ) {
			return _set_conn( database, DATABASE );
		}
		else {
			return set_property( String("schema"), database );
		}
	}

	String get_database() {
		if ( connection_status() == CONNECTED ) {
			return _get_conn( DATABASE );
		}
		else {
			return get_property( String("schema") );
		}
	}


	/*    TRANSACTION     */
	Error set_autocommit( bool autocommit ) { return _set_conn( autocommit, AUTOCOMMIT ); }
	bool get_autocommit() { return _get_conn( AUTOCOMMIT);}
	//Ativa ou desativa o modo de confirmação automática padrão para a sessão atual.


//	https://dev.mysql.com/doc/refman/8.0/en/innodb-transaction-isolation-levels.html
	Error set_transaction_isolation( Isolation level ){
		ERR_FAIL_COND_V_EDMSG( level == TRANSACTION_ERROR, ERR_PARAMETER_RANGE_ERROR, "TRANSACTION_ERROR is not a valid parameter.");
		ERR_FAIL_COND_V_EDMSG( level == TRANSACTION_NONE, ERR_PARAMETER_RANGE_ERROR, "TRANSACTION_NONE is not a valid parameter.");
		ERR_FAIL_COND_V_EDMSG( level < TRANSACTION_ERROR, ERR_PARAMETER_RANGE_ERROR, "Invalid parameter.");
		ERR_FAIL_COND_V_EDMSG( level > TRANSACTION_SERIALIZABLE, ERR_PARAMETER_RANGE_ERROR, "Invalid parameter.");
		return _set_conn( level, ISOLATION );
	}

	Isolation get_transaction_isolation() {
		int iso = (int)_get_conn(ISOLATION);
		return (Isolation)iso;
	}

	// All savepoints of the current transaction are deleted if you execute a COMMIT, or a ROLLBACK that does not name a savepoint.
	Error rollback(String savepoint = ""){ return _set_conn(savepoint, ROLLBACK); }


	// confirma a transação atual, tornando suas alterações permanentes.
	// All savepoints of the current transaction are deleted if you execute a COMMIT, or a ROLLBACK that does not name a savepoint.
	Error commit(){ return _set_conn( String(), COMMIT); }



	Error create_savepoint(String p_savept) {
	//	https://dev.mysql.com/doc/refman/5.6/en/savepoint.html
	//	Cria um ponto de salvamento nomeado com um identificador.
		ERR_FAIL_COND_V_EDMSG( p_savept == String(""), ERR_INVALID_PARAMETER, "Savepoints must have a name.");
		return _set_conn(p_savept, CREATE_SAVEPOINT);
	}


//	Remove o ponto de salvamento.
//	Não ocorre commit ou rollback para savepoints deletados.
//	Todos os pontos de salvamento da transação atual são excluídos se você executar um COMMIT, ou um ROLLBACK que não nomeie um ponto de salvamento.
//	All savepoints of the current transaction are deleted if you execute a COMMIT, or a ROLLBACK that does not name a savepoint.
	Error delete_savepoint(String p_savept) { return _set_conn(p_savept, DELETE_SAVEPOINT); }

	PoolStringArray get_savepoints(){
		PoolStringArray ret;
		for ( auto it:savepoint_map ){
			ret.append( it.first );
		}
		return ret;
	}

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


IF DEBUG
https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-debug-tracing.html
con->setClientOption("libmysql_debug", "d:t:O,client.trace");

*/

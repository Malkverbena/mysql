/* mysql.h */
#ifndef MYSQL_H
#define MYSQL_H


#ifdef GODOT4
	#include "core/object/reference.h"
#else
	#include "core/reference.h"
#endif

#include <memory>
#include <vector>


//#include "MySQL.hpp"

#include <mysql_error.h>
#include <mysql_driver.h>
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <cppconn/metadata.h>
#include <cppconn/connection.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/prepared_statement.h>

#pragma once
#define CPPCONN_PUBLIC_FUNC


class MySQL : public Reference{
	GDCLASS(MySQL,Reference);


protected:
	static void _bind_methods();

	enum SQL_TIME { 
		NON_TIME,	// - Non sqltime
		DATETIME,	// - 0000-00-00 00:00:00
		DATE,		// - 0000-00-00
		TIME,		// - 00:00:00
		YEAR,		// - 0000
	};



private:
	sql::ConnectOptionsMap connection_properties;
	sql::mysql::MySQL_Driver *driver;
	std::unique_ptr<sql::Connection> conn;


	const std::string void_properties [2] = {  //void
		"OPT_CONNECT_ATTR_RESET", 
		"OPT_NAMED_PIPE"
	};
	
	const std::string sqlmap_properties [2] = { //map
		"OPT_CONNECT_ATTR_ADD",
		"OPT_CONNECT_ATTR_DELETE",
	};
	
	const std::string int_properties [5] = { //int
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
	int _execute( String p_sqlquery, Array p_values, bool _prep);
	
	void set_datatype(std::shared_ptr <sql::PreparedStatement> prep_stmt, Array prep_val );



	//ERRORS
	void print_SQLException(sql::SQLException &e);
	void print_runtime_error(std::runtime_error &e);


	//CHECKERS
	std::string	get_prop_type( std::string p_prop );
	bool is_json( Variant p_arg );
	

	bool is_mysql_time(String time);
	
	
	//HELPERS
	Array format_time(String str, bool return_string);
	MySQL::SQL_TIME get_time_format(String time);
	String SQLstr2GDstr( const sql::SQLString *p_str );
	String SQLstr2GDstr( sql::SQLString &p_string );
//	String SQLstr2GDstr( sql::SQLString p_string );
	
	sql::SQLString GDstr2SQLstr(String &p_str); 


public:

	typedef Dictionary MySQLException;
	MySQLException sqlError;
	std::string sqlstate ;

	enum ConnectionStatus {
		NO_CONNECTION, 
		CLOSED, 
		CONNECTED, 
		DISCONNECTED
	};
	
	enum SQLDataTypes {
		NIL,
		// Atomic types	
		BOOLEAN,
		INT,
		FLOAT,
		STRING,
		// Meta types
		JSON,
		BLOB,
		DATATIME,
		TYPE_MAX	
	};
	
	enum recover_data_like {	
		COLUMNS_NAMES,	// returns an array with only one element (names of columns)
		COLUMNS_TYPES,	// returns an array with only one element (types of the columns)
		SIMPLE_ARRAY,	// only values
		NAMED_ARRAY,	// array[0] has columns names
		TYPED_ARRAY,	// array[0] has columns types
		FULL_ARRAY,		// array[0] has columns names & array[1] has columns types
		DICTIONARY,		// {column_name:value}
	};


	Variant test(PoolByteArray p_args);


	//CONNECTION
	ConnectionStatus get_connection_status();
	ConnectionStatus stop_connection();
	MySQLException start_connection();
	
	
	//PROPERTIES
	Variant get_property(String p_property);
	void set_property(String p_property, Variant p_value);


	//EXECUTE
	int execute(String p_sqlquery);
	int execute_prepared(String p_sqlquery, Array p_values);

/*
	
	int execute_prepared_query(String p_SQLquery, Array p_values);

	Array fetch_query(String p_sqlquery, int return_like = DICTIONARY,  bool return_string = false);	
	Array fetch_prepared_query(String p_sqlquery, Array p_values, int return_what = DICTIONARY,  bool return_string = false);	

	Variant transaction( [] );
	Variant transaction_prepared( {} );
*/



	MySQL();
	~MySQL();

};


VARIANT_ENUM_CAST(MySQL::recover_data_like);
VARIANT_ENUM_CAST(MySQL::ConnectionStatus);





#endif	// MYSQL_H

//TODO:

/*
std::string MySQL_Connection::getSessionVariable(const std::string & varname)
void MySQL_Connection::setSessionVariable(const std::string & varname, const std::string & value)
*/



	//Savepoint *savept;
	//int tipo = p_value.get_type();
	//tipo == Variant::INT
	//if (pstmt->getMoreResults())

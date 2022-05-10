/*  mysql.cpp */
#include "core/io/json.h"
#include "mysql.h"
/*
#include <iostream>
#include <utility>
#include <map>
*/

using namespace std;


Variant MySQL::teste(String p_property){
	return String();
}

/*        QUERY       */

Array MySQL::query(String p_sqlquery, DataFormat data_model, bool return_string, PoolIntArray meta_col){
	return _query(p_sqlquery, Array(), data_model, return_string, meta_col, false);
}

Array MySQL::query_prepared(String p_sqlquery, Array p_values, DataFormat data_model, bool return_string, PoolIntArray meta_col){
	return _query(p_sqlquery, p_values, data_model, return_string, meta_col, true);
}

Array MySQL::_query(String p_sqlquery, Array p_values, DataFormat data_model, bool return_string, PoolIntArray meta_col, bool _prep){
	sql::SQLString query = p_sqlquery.utf8().get_data();
	Array ret;
	std::shared_ptr <sql::PreparedStatement> prep_stmt;
	std::unique_ptr <sql::Statement> stmt;
	std::unique_ptr <sql::ResultSet> res;
	sql::ResultSetMetaData *res_meta;
	int data_size = p_values.size();








/*     PROPERTIES     */

Error MySQL::set_property(String p_property, Variant p_value){
	int prop_type = get_prop_type(p_property);
	sql::SQLString property = String(p_value).utf8().get_data();
	int val_type = p_value.get_type();

	switch(prop_type) {
		case MySQL::TYPE::INVALID:
			return ERR_DOES_NOT_EXIST;

		case MySQL::TYPE::STRING:
			ERR_FAIL_COND_V_EDMSG( val_type != Variant::STRING, ERR_INVALID_PARAMETER, "The parameter of this property must be of type String");
			connection_properties[property] = (sql::SQLString)String(p_value).utf8().get_data();
			break;

		case MySQL::TYPE::BOOL:
			ERR_FAIL_COND_V_EDMSG( val_type != Variant::BOOL, ERR_INVALID_PARAMETER, "The parameter of this property must be of type Boolean");
			connection_properties[property] = (bool)p_value;
			break;

		case MySQL::TYPE::INT:
			ERR_FAIL_COND_V_EDMSG( val_type != Variant::INT, ERR_INVALID_PARAMETER, "The parameter of this property must be of type Integer");
			connection_properties[property] = (std::string)String(p_value).utf8().get_data();
			break;

		case MySQL::TYPE::VOID:
			ERR_FAIL_COND_V_EDMSG( val_type != Variant::NIL, ERR_INVALID_PARAMETER, "The parameter of this property must be of type Null");
			connection_properties[property] = sql::SQLString("NULL");   // nullptr?
			break;

		case MySQL::TYPE::LISTA:{
#ifdef GODOT4
			ERR_FAIL_COND_V_EDMSG( val_type != Variant::PACKED_STRING_ARRAY, ERR_INVALID_PARAMETER, "The parameter of this property must be of type PackedStringArray");
#else
			ERR_FAIL_COND_V_EDMSG( val_type != Variant::STRING_ARRAY, ERR_INVALID_PARAMETER, "The parameter of this property must be of type PoolStringArray");
#endif
			PoolStringArray data_list = PoolStringArray(p_value);
			list< sql::SQLString > list_prop ;
			for (int a = 0; a < data_list.size(); a++ ){
				list_prop.push_front( data_list[a].utf8().get_data() );
			}
			connection_properties[property] = list_prop;
			break;
		}

		case MySQL::TYPE::MAP:{
			ERR_FAIL_COND_V_EDMSG( val_type != Variant::DICTIONARY, ERR_INVALID_PARAMETER, "The parameter of this property must be of type Dictionary");
			map< sql::SQLString, sql::SQLString > map_prop;
			Dictionary map_data = Dictionary(p_value);
			Array map_keys = map_data.keys();
			Array map_values = map_data.values();
			for (int j = 0; j < map_data.size(); j++ ){
				map_prop[ String(map_keys[j]).utf8().get_data() ] = String(map_values[j]).utf8().get_data();
			}
			connection_properties[property] = map_prop;
			break;
		}

	}
	return OK;
}


Variant MySQL::get_property(String p_property){
	int prop_type = get_prop_type(p_property);
	sql::SQLString property = p_property.utf8().get_data();

	if (property == "schema"){
		sql::SQLString _p_s = conn->getSchema();
		return string_SQL_2_GDT( _p_s );
	}

	switch(prop_type) {
		case MySQL::TYPE::INVALID:
			// Colocar no doc
			// "Non-existent property. \nFor more information visit: https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-connect-options.html"
			return Variant();

		case MySQL::TYPE::STRING:
 			return string_SQL_2_GDT( *connection_properties[ property ].get<sql::SQLString>() );

		case MySQL::TYPE::BOOL:
			return *connection_properties[ property ].get< bool >();

		case MySQL::TYPE::INT:
			return *connection_properties[ property ].get< int >();

		case MySQL::TYPE::VOID:
			return string_SQL_2_GDT( *connection_properties[ property ].get<sql::SQLString>() );

		case MySQL::TYPE::MAP: {
			auto ret_map = *connection_properties[ property ].get< map< sql::SQLString, sql::SQLString > >();
			Dictionary ret_dic;
			for (auto it = ret_map.begin(); it != ret_map.end(); it++ ) {
				sql::SQLString _key = it->first;
				sql::SQLString _value = it->second;
				ret_dic[ string_SQL_2_GDT( _key ) ] = string_SQL_2_GDT( _value );
			}
			return ret_dic;
		}

		case MySQL::TYPE::LISTA:
			auto ret_list = *connection_properties[ property ].get< std::list < sql::SQLString > >();
			PoolStringArray data;
			for (sql::SQLString val : ret_list) {
				data.append( string_SQL_2_GDT(val) );
			}
			return data;
	}
	return Variant();
}


Error MySQL::set_properties_array(Dictionary p_properties){
	ERR_FAIL_COND_V_EDMSG( connection_status() != CONNECTED, ERR_CONNECTION_ERROR, "DatabaseMetaData FAILURE - database not connected! - METHOD: set_properties_set");
	Error erro;
	for ( int i = 0; i < p_properties.size(); i++){
		erro = set_property( p_properties.keys()[i], p_properties.values()[i] );
		ERR_FAIL_COND_V( erro != OK, erro );
	}
	return OK;
}


Dictionary MySQL::get_properties_array(Array p_properties){
	ERR_FAIL_COND_V_EDMSG( connection_status() != CONNECTED, Dictionary(), "DatabaseMetaData FAILURE - database not connected! - METHOD: get_properties_set");
	Dictionary ret;
	for ( int i = 0; i < p_properties.size(); i++){
		ret[ p_properties[i] ] = get_property( p_properties[i] );
		ERR_FAIL_COND_V( ret[ p_properties[i] ] == Variant(), Variant() );
	}
	return ret;
}





/*     CONNECTIONS    */

void MySQL::set_credentials( String p_host, String p_user, String p_pass ) {
	connection_properties["hostName"] = p_host.utf8().get_data();
	connection_properties["userName"] = p_user.utf8().get_data();
	connection_properties["password"] = p_pass.utf8().get_data();
}


MySQL::ConnectionStatus MySQL::connection_start() {
	try {
		driver = sql::mysql::get_mysql_driver_instance();
		conn.reset( driver->connect( connection_properties ) );
	}
	catch (sql::SQLException &e) {
		print_SQLException( e );
	}
	return connection_status();
}


MySQL::ConnectionStatus MySQL::connection_stop() {
	if (conn.get()) { // != NULL
		if (!conn->isClosed()){
			conn->close();
		}
	}
	return connection_status();
}


MySQL::ConnectionStatus MySQL::connection_status() {
	MySQL::ConnectionStatus ret;
	if (conn.get()) {
		if (!conn-> isClosed()) {
			if (conn-> isValid()) {
				ret = CONNECTED;
			} else {
				ret = DISCONNECTED;
			}
		}else{
			ret = CLOSED;
		}
	}else{
		ret = NO_CONNECTION;
	}
	return ret;
}


Dictionary MySQL::get_metadata(){
	ERR_FAIL_COND_V_MSG(connection_status() != CONNECTED, Dictionary(), "DatabaseMetaData FAILURE - database is not connected! - METHOD: get_metadata");
	Dictionary ret;
	sql::DatabaseMetaData *dbcon_meta = conn->getMetaData();
	std::unique_ptr < sql::ResultSet > res(dbcon_meta->getSchemas());
	ret["Total number of schemas"] = res->rowsCount();
	ret["Database Product Name"] = string_SQL_2_GDT( (sql::SQLString&)(dbcon_meta->getDatabaseProductName()));
	sql::SQLString _version = dbcon_meta->getDatabaseProductVersion();
	ret["Database Product Version"] = string_SQL_2_GDT(_version);
	sql::SQLString _name = dbcon_meta->getUserName();
	ret["Database User Name"] = string_SQL_2_GDT(_name);
	ret["Driver name"] = string_SQL_2_GDT( (sql::SQLString&)(dbcon_meta->getDriverName()));
	ret["Driver version"] = string_SQL_2_GDT( (sql::SQLString&)(dbcon_meta->getDriverVersion()));
	ret["Database in Read-Only Mode"] = dbcon_meta->isReadOnly();
	ret["Supports Transactions"] = dbcon_meta->supportsTransactions();
	ret["Supports DML Transactions only"] = dbcon_meta->supportsDataManipulationTransactionsOnly();
	ret["Supports Batch Updates"] = dbcon_meta->supportsBatchUpdates();
	ret["Supports Outer Joins"] = dbcon_meta->supportsOuterJoins();
	ret["Supports Multiple Transactions"] = dbcon_meta->supportsMultipleTransactions();
	ret["Supports Named Parameters"] = dbcon_meta->supportsNamedParameters();
	ret["Supports Statement Pooling"] = dbcon_meta->supportsStatementPooling();
	ret["Supports Stored Procedures"] = dbcon_meta->supportsStoredProcedures();
	ret["Supports Union"] = dbcon_meta->supportsUnion();
	ret["Maximum Connections"] = dbcon_meta->getMaxConnections();
	ret["Maximum Columns per Table"] = dbcon_meta->getMaxColumnsInTable();
	ret["Maximum Columns per Index"] = dbcon_meta->getMaxColumnsInIndex();
	ret["Maximum Row Size per Table"] = dbcon_meta->getMaxRowSize();
	return ret;
}


void MySQL::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_credentials", "HostName", "UserName", "Password"),&MySQL::set_credentials);
	ClassDB::bind_method(D_METHOD("connection_start"),&MySQL::connection_start);
	ClassDB::bind_method(D_METHOD("connection_stop"),&MySQL::connection_stop);
	ClassDB::bind_method(D_METHOD("connection_status"),&MySQL::connection_status);
	ClassDB::bind_method(D_METHOD("get_metadata"),&MySQL::get_metadata);

	ClassDB::bind_method(D_METHOD("get_property", "property"),&MySQL::get_property);
	ClassDB::bind_method(D_METHOD("set_property", "property", "value"),&MySQL::set_property);
	ClassDB::bind_method(D_METHOD("set_properties_array", "properties"),&MySQL::set_properties_array);
	ClassDB::bind_method(D_METHOD("get_properties_array", "properties"),&MySQL::get_properties_array);
	ClassDB::bind_method(D_METHOD("set_database", "database"),&MySQL::set_database);
	ClassDB::bind_method(D_METHOD("get_database"),&MySQL::get_database);

//	ClassDB::bind_method(D_METHOD("set_client_option", "option", "value"),&MySQL::set_client_option);
//	ClassDB::bind_method(D_METHOD("get_client_option", "option"),&MySQL::get_client_option);
//	ClassDB::bind_method(D_METHOD("set_session_variable", "varname", "value"),&MySQL::set_session_variable);
//	ClassDB::bind_method(D_METHOD("get_session_variable", "varname"),&MySQL::get_session_variable);

	ClassDB::bind_method(D_METHOD("query", "sql_statement", "DataFormat", "return_string", "meta"),&MySQL::query, DEFVAL(DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()) );
	ClassDB::bind_method(D_METHOD("query_prepared", "sql_statement", "Values", "DataFormat", "return_string", "meta"), &MySQL::query_prepared, DEFVAL(Array()),


	ClassDB::bind_method(D_METHOD("teste", "teste"),&MySQL::teste);



	BIND_ENUM_CONSTANT(NO_CONNECTION);
	BIND_ENUM_CONSTANT(CLOSED);
	BIND_ENUM_CONSTANT(CONNECTED);
	BIND_ENUM_CONSTANT(DISCONNECTED);

}


/*       ERROS        */

void MySQL::print_SQLException(sql::SQLException &e) {
	//If (e.getErrorCode() == 1047) = Your server does not seem to support Prepared Statements at all. Perhaps MYSQL < 4.1?.

	Variant file = __FILE__;
	Variant line = __LINE__;
	Variant func = __FUNCTION__;
	Variant errCode = e.getErrorCode();
	sql::SQLString _err = e.getSQLState();
	String _error = string_SQL_2_GDT(_err);

	sqlError.clear();
	sqlError["FILE"] = String( file );
	sqlError["FUNCTION"] = String( func );
	sqlError["LINE"] = String( line );
	sqlError["ERROR"] = String( e.what() );
	sqlError["MySQL error code"] = String( errCode );
	sqlError["SQLState"] = _error;

	//FIXME: 	ERR_FAIL_	just print
	//	ERR_PRINT(sqlError)

#ifdef TOOLS_ENABLED
	print_line("# EXCEPTION Caught ˇ");
	print_line("# ERR: SQLException in: "+String(file)+" in function: "+String(func)+"() on line "+String(line));
	print_line("# ERR: " + String(e.what()));
	print_line(" (MySQL error code: " + String( errCode)+ ")" );
	print_line("SQLState: " + _error);
#endif

}


void MySQL::print_runtime_error(std::runtime_error &e) {
	std::cout << "ERROR: runtime_error in " << __FILE__;
	std::cout << " (" << __func__ << ") on line " << __LINE__ << std::endl;
	std::cout << "ERROR: " << e.what() << std::endl;
}



/*      HEELPERS      */
String MySQL::string_SQL_2_GDT( sql::SQLString &p_string ){
	const char * _str_ = p_string.c_str();
	String str = String::utf8( (char *) _str_ );
	return str;
}


int	MySQL::get_prop_type( String prop ){
	if ( prop_string.has( prop ) ) {
		return MySQL::TYPE::STRING;
	}
	else if ( prop_bool.has( prop ) ) {
		return MySQL::TYPE::BOOL;
	}
	else if ( prop_int.has( prop ) ) {
		return MySQL::TYPE::INT;
	}
	else if ( prop_void.has( prop ) ) {
		return MySQL::TYPE::VOID;
	}
	else if ( prop_map.has( prop ) ) {
		return MySQL::TYPE::MAP;
	}
	else if ( prop_list.has( prop ) ) {
		return MySQL::TYPE::LISTA;
	}
	else{
		return MySQL::TYPE::INVALID;
	}
}


MySQL::MySQL(){
	connection_properties["port"] = 3306;
	connection_properties["OPT_RECONNECT"] = true;
	connection_properties["CLIENT_MULTI_STATEMENTS"] = false;


/*
IF DEBUG
https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-debug-tracing.html
con->setClientOption("libmysql_debug", "d:t:O,client.trace");
*/

}


MySQL::~MySQL(){
}




/*  mysql.cpp */

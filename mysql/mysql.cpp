/*  mysql.cpp */
#include "mysql.h"
#include <sstream>
#include "core/io/json.h"
using namespace std;
Variant MySQL::teste(String p_property){
	return String();
}


/*     CONNECTION     */

Error MySQL::_set_conn_op( Variant p_value, MySQL::OP op){
	int conn_status = connection_status();
	ERR_FAIL_COND_V_EDMSG( (conn_status != CONNECTED and op != MySQL::OP::DATABASE), ERR_CONNECTION_ERROR, "There is no active connection!" );
	try{
		switch(op) {
			case DATABASE: {
				String data = p_value;
				sql::SQLString database = data.utf8().get_data();
				if (conn_status == CONNECTED){
					conn->setSchema( database );
				} else {
					connection_properties["schema"] = database;
				}
				break;
			}
			case CATALOG: {
				String data = p_value;
				sql::SQLString catalog = data.utf8().get_data();
				conn->setCatalog( catalog );
				break;
			}
			case AUTOCOMMIT: {
				bool autocommit = p_value;
				conn->setAutoCommit( autocommit );
				break;
			}
			case ISOLATION: {
				int level = p_value;
				ERR_FAIL_COND_V_EDMSG( level == TRANSACTION_ERROR, ERR_PARAMETER_RANGE_ERROR, "TRANSACTION_ERROR is not a valid parameter.");
				ERR_FAIL_COND_V_EDMSG( level == TRANSACTION_NONE, ERR_PARAMETER_RANGE_ERROR, "TRANSACTION_NONE is not a valid parameter.");
				ERR_FAIL_COND_V_EDMSG( level < TRANSACTION_ERROR, ERR_PARAMETER_RANGE_ERROR, "Invalid parameter.");
				ERR_FAIL_COND_V_EDMSG( level > TRANSACTION_SERIALIZABLE, ERR_PARAMETER_RANGE_ERROR, "Invalid parameter.");
				conn->setTransactionIsolation( (sql::enum_transaction_isolation)level );
				break;
			}
			case READONLY: {
				bool readyonly = p_value;
				conn->setReadOnly( readyonly );
				break;
			}
			case CLIENT_INFO: {
				break;
			}
		}
	}

	catch (sql::SQLException &e) {
		print_SQLException(e);
	}
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
	return OK;
}

/*
Error MySQL::set_catalog(String p_catalog){
	ERR_FAIL_COND_V_EDMSG( connection_status() != CONNECTED, ERR_CONNECTION_ERROR, "There is no active connection!" );
	try{
		sql::SQLString catalog = p_catalog.utf8().get_data();
		conn->setCatalog( catalog );
	}
	catch (sql::SQLException &e) {
		print_SQLException(e);
	}
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
	return OK;
}
Error MySQL::set_database( String p_database ){
	sql::SQLString database = p_database.utf8().get_data();
	int conn_status = connection_status();
	if ( conn_status == CONNECTED ) {
		conn->setSchema(database);
		return OK;
	}
	if ( conn_status == NO_CONNECTION ) {
		return ERR_CONNECTION_ERROR;
	}
	else {
		connection_properties["schema"] = database;
		return OK;
	}
}
Error MySQL::set_autocommit(bool autoCommit){
	ERR_FAIL_COND_V_EDMSG( connection_status() != CONNECTED, ERR_CONNECTION_ERROR, "There is no active connection!" );
	try{
		conn->setAutoCommit(autoCommit);
	}
	catch (sql::SQLException &e) {
		print_SQLException(e);
	}
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
	return OK;
}
Error MySQL::set_transaction_isolation( Isolation level) {
	ERR_FAIL_COND_V_EDMSG( level == TRANSACTION_ERROR, ERR_PARAMETER_RANGE_ERROR, "TRANSACTION_ERROR is not a valid parameter.");
	ERR_FAIL_COND_V_EDMSG( level == TRANSACTION_NONE, ERR_PARAMETER_RANGE_ERROR, "TRANSACTION_NONE is not a valid parameter.");
	ERR_FAIL_COND_V_EDMSG( connection_status() != CONNECTED, ERR_CONNECTION_ERROR, "There is no active connection!" );
	try{
		conn->setTransactionIsolation( (sql::enum_transaction_isolation)level  );
	}
	catch (sql::SQLException &e) {
		print_SQLException(e);
	}
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
	return OK;
}

*/



Variant MySQL::_get_conn_op(MySQL::OP op){
	int conn_status = connection_status();
	ERR_FAIL_COND_V_EDMSG( (conn_status != CONNECTED and op != DATABASE), false, "There is no active connection!" );
	try{
		switch(op) {
			case DATABASE: {
				if (conn_status == CONNECTED){
					sql::SQLString database = conn->getSchema();
					return string_SQL_2_GDT( database );
				} else {
					return string_SQL_2_GDT( *connection_properties["schema"].get<sql::SQLString>() );
				}
			}
			case CATALOG:{
				sql::SQLString catalog = conn->getCatalog();
				return string_SQL_2_GDT( catalog );
			}
			case AUTOCOMMIT:{
				return (bool)conn->getAutoCommit();
			}
			case ISOLATION:{
				return (int)conn->getTransactionIsolation();
			}
			case READONLY: {
				return (bool)conn->isReadOnly();
			}
			case CLIENT_INFO: {
				sql::SQLString client_info = conn->getClientInfo();
				return string_SQL_2_GDT( client_info );
			}
		}
	}
	catch (sql::SQLException &e) {
		print_SQLException(e);
	}
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
	return Variant();
}





/*

String MySQL::get_database(){
	int conn_status = connection_status();
	if ( conn_status == CONNECTED ) {
		sql::SQLString database = conn->getSchema();
		return string_SQL_2_GDT( database );
	}
	if ( conn_status == NO_CONNECTION ) {
		ERR_FAIL_V_MSG("", "There is no active connection!");
	}
	else {
		return string_SQL_2_GDT( *connection_properties["schema"].get<sql::SQLString>() );
	}
}

String catalog MySQL::get_catalog(){
	ERR_FAIL_COND_V_EDMSG( connection_status() != CONNECTED, false, "There is no active connection!" );


	try{
		_autocommit = conn->getAutoCommit();


	}
	catch (sql::SQLException &e) {
		print_SQLException(e);
	}
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
	return _autocommit;
}
bool MySQL::get_autocommit(){
	ERR_FAIL_COND_V_EDMSG( connection_status() != CONNECTED, false, "There is no active connection!" );
	bool _autocommit = false;
	try{
		_autocommit = conn->getAutoCommit();
	}
	catch (sql::SQLException &e) {
		print_SQLException(e);
	}
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
	return _autocommit;
}
MySQL::Isolation MySQL::get_transaction_isolation() {
	ERR_FAIL_COND_V_EDMSG( connection_status() != CONNECTED, TRANSACTION_ERROR, "There is no active connection!" );
	int ret;
	try{
		ret = (int)conn->getTransactionIsolation();
	}
	catch (sql::SQLException &e) {
		print_SQLException(e);
	}
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
	return (MySQL::Isolation)ret;
}

*/















/*    TRANSACTION     */

















/*      DATABASE      */
Array MySQL::query(String p_sqlquery, DataFormat data_model, bool return_as_string, PoolIntArray meta_col){
	return _query(p_sqlquery, Array(), data_model, return_as_string, meta_col, false);
}

Array MySQL::query_prepared(String p_sqlquery, Array p_values, DataFormat data_model, bool return_as_string, PoolIntArray meta_col){
	return _query(p_sqlquery, p_values, data_model, return_as_string, meta_col, true);
}

bool MySQL::execute(String p_sqlquery){
	return bool(_execute( p_sqlquery, Array(), false, false ));
}

bool MySQL::execute_prepared(String p_sqlquery, Array p_values){
	return bool(_execute( p_sqlquery, p_values, true, false ));
}

int MySQL::update( String p_sqlquery ){
	return _execute( p_sqlquery, Array(), false, true );
}

int MySQL::update_prepared( String p_sqlquery, Array p_values){
	return _execute( p_sqlquery, p_values, true, true);
}




/*        CORE        */
int MySQL::_execute( String p_sqlquery, Array p_values, bool prep_st, bool update){
	sql::SQLString query = p_sqlquery.utf8().get_data();
	int afectedrows;
	int data_size =  p_values.size();

	try {
		if (prep_st){
			std::shared_ptr<sql::PreparedStatement> prep_stmt;
			prep_stmt.reset(conn->prepareStatement(query));
			std::vector<std::stringstream> multiBlob (data_size);
			for (int h =0; h < data_size; h++){
				set_datatype(prep_stmt, &multiBlob[h], p_values[h], h);
			}
			if ( update ){
				afectedrows = prep_stmt->executeUpdate();
			}
			else{
				afectedrows = int( prep_stmt->execute() );
			}

		}else{
			std::unique_ptr <sql::Statement> stmt;
			stmt.reset(conn->createStatement());
			if ( update ){
				afectedrows = stmt->executeUpdate(query);
			}
			else {
				afectedrows = int( stmt->execute(query) );
			}
		}
	}

	catch (sql::SQLException &e) {
		print_SQLException(e);
	}

	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}

	return afectedrows;
}


Array MySQL::_query(String p_sqlquery, Array p_values, DataFormat data_model, bool return_as_string, PoolIntArray meta_col, bool _prep){
	sql::SQLString query = p_sqlquery.utf8().get_data();
	std::shared_ptr <sql::PreparedStatement> prep_stmt;
	std::unique_ptr <sql::Statement> stmt;
	std::unique_ptr <sql::ResultSet> res;
	sql::ResultSetMetaData *res_meta;
	int data_size = p_values.size();
	Array ret;

	try{
		if ( not _prep ) {
			stmt.reset(conn->createStatement());
			res.reset( stmt->executeQuery(query));
			res_meta = res->getMetaData();
		}
		else{
			prep_stmt.reset(conn->prepareStatement(query));
			std::vector<std::stringstream> multiBlob (data_size);
			for (int h =0; h < data_size; h++){
				set_datatype(prep_stmt, &multiBlob[h], p_values[h], h);
			}
			res.reset( prep_stmt->executeQuery());
			res_meta = res->getMetaData();
		}


		// GET META INFO  --------  BEGIN
		Array columnname; Array columntypes; Dictionary table_info; Array attributes;

		// TODO:
		if ( meta_col.has( MySQL::MetaCollection::TABLE_INFO ) ){
//			sql::DatabaseMetaData *conn_meta = conn->getMetaData();
	//		table_info[ "TABLE NAME" ] = res_meta->getTableName();
//			getExportedKeys(const sql::SQLString& catalog, const sql::SQLString& schema, const sql::SQLString& table)
//			table_info[ "EXPORTED KEYS" ] = conn_meta->getExportedKeys( nome da tabela );
//			table_info[ "PRIMARY KEYS" ] = conn_meta->getPrimaryKeys( nome da tabela );
//			table_info[ "IMPORTED KEYS" ] = conn_meta->getImportedKeys( nome da tabela );
//			table_info[ "INDEX INFO" ] = conn_meta->getIndexInfo
	//		table_info[ "TABLE NAME" ] = res_meta->getTableName(0);
//			table_info[ "SCHEMA" ] = res_meta->getSchemaName(0);
			table_info[ "DEFINITELY WRITABLE" ] = res_meta->isDefinitelyWritable(0);
			table_info[ "WRITABLE" ] = res_meta->isWritable(0);
			if ( is_mat_empty( table_info ) ){
				ret.push_back( table_info );
			}
		}
		if ( not is_mat_empty( meta_col ) ){
			for (uint8_t i = 1; i <= res_meta->getColumnCount(); i++) {
				if ( meta_col.has( MySQL::MetaCollection::COLUMNS_NAMES ) ){
					sql::SQLString str_name = res_meta->getColumnName(i);
					columnname.push_back( string_SQL_2_GDT( str_name ) );
				}
				if ( meta_col.has( MySQL::MetaCollection::COLUMNS_TYPES ) ){
					sql::SQLString str_type = res_meta->getColumnTypeName(i);
					columntypes.push_back( string_SQL_2_GDT( str_type ) );
				}
				if ( meta_col.has( MySQL::MetaCollection::ATTRIBUTES ) ){
					Dictionary col_att;
					col_att[ String( "NOT NULL" ) ] = res_meta->isNullable(i);
					col_att[ String( "AUTO INCREMENT" ) ] = res_meta->isAutoIncrement(i);
				//	col_att[ String( "CHARSET" ) ] = res_meta->getColumnCharset(i);
				//	col_att[ String( "COLLATION" ) ] = res_meta->getColumnCollation(i);
				//	col_att[ String( "NUMERIC" ) ] = res_meta->res_meta->isNumeric(i);
					col_att[ String( "PRECISION" ) ] = res_meta->getPrecision(i);
					col_att[ String( "CURRENCY" ) ] = res_meta->isCurrency(i);
				//	col_att[ String( "DISPLAY SIZE" ) ] = res_meta->getColumnDisplaySize(i);
				//	col_att[ String( "LABEL" ) ] = res_meta->getColumnLabel(i);
				//	col_att[ String( "NAME" ) ] = res_meta->getColumnName(i);
				//	col_att[ String( "CATALOG" ) ] = res_meta->getCatalogName(i);
					col_att[ String( "UNSIGNED" ) ] = not res_meta->isSigned(i);
					col_att[ String( "ZERO FILL" ) ] = res_meta->isZerofill(i);
					attributes.push_back( col_att );
				}
			}
			if ( is_mat_empty(columnname) ) {
				ret.push_back(columnname);
			}
			if ( is_mat_empty(columntypes) ) {
				ret.push_back(columntypes);
			}
			if ( is_mat_empty(table_info) ) {
				ret.push_back(table_info);
			}
		}
		if ( meta_col.has( MySQL::MetaCollection::NO_QUERY ) ){
			return ret;
		}
		// GET META INFO  --------  END


		// FITTING DATA  --------  BEGIN
		while (res->next()) {
			Array line;
			Dictionary row;

			for ( unsigned int i = 1; i <= res_meta->getColumnCount(); i++) {
				sql::SQLString col_name = res_meta->getColumnName(i);
				String column_name = string_SQL_2_GDT( col_name );
				int d_type = res_meta->getColumnType(i);

				// RETURN DATA AS STRING
				if ( return_as_string ){
					sql::SQLString str_val = res->getString(i);
					String str_value = string_SQL_2_GDT( str_val );

					if ( data_model == DICTIONARY ) {
						row[ column_name ] = str_value;
					} else {
						line.push_back( str_value );
					}
				}

				else{ // RETURN DATA WITH VALID TYPES

					// NULL
					if ( res->isNull(i) ){
						if  ( data_model == DICTIONARY ) {
							row[ column_name ] = Variant();
						}else{
							line.push_back( Variant() );
						}
					}

					// BOOL
					else if ( d_type == sql::DataType::BIT ){
						if ( data_model == DICTIONARY ) {
							row[ column_name ] = res->getBoolean(i);
						} else {
							line.push_back( res->getBoolean(i) );
						}
					}

					// INT32
					else if ( d_type == sql::DataType::TINYINT || d_type == sql::DataType::SMALLINT || d_type == sql::DataType::MEDIUMINT) {
						if ( data_model == DICTIONARY ) {
							row[ column_name ] = res->getInt(i);
						} else {
							line.push_back( res->getInt(i) );
						}
					}

					// INT64
					else if ( d_type == sql::DataType::INTEGER || d_type == sql::DataType::BIGINT ) {
						if ( data_model == DICTIONARY ) {
							row[ column_name ] = res->getInt64(i);
						} else {
							line.push_back( res->getInt64(i) );
						}
					}

					// FLOAT
					else if ( d_type == sql::DataType::REAL || d_type == sql::DataType::DOUBLE || d_type == sql::DataType::DECIMAL || d_type == sql::DataType::NUMERIC ) {
						double my_float = res->getDouble(i);
						if ( data_model == DICTIONARY ) {
							row[ column_name ] = my_float;
						} else {
							line.push_back( my_float );
						}
					}

					// TIME
					// It should return time information as a dictionary but if the sequence of the data be modified (using TIME_FORMAT or DATE_FORMAT for exemple),
					// it will return the dictionary fields with wrong names. So I prefer return the data as an array.
					else if ( d_type == sql::DataType::DATE || d_type == sql::DataType::TIME || d_type == sql::DataType::TIMESTAMP || d_type == sql::DataType::YEAR ) {
						sql::SQLString time_string = res->getString(i);
						Array time = format_time( string_SQL_2_GDT( time_string ) , false );
						if ( data_model == DICTIONARY ) {
							row[ column_name ] = time;
						} else {
							line.push_back( time );
						}
					}

					// ENUM
					else if ( d_type == sql::DataType::ENUM ){
						sql::SQLString enum_str = res->getString(i);
						if ( data_model == DICTIONARY ) {
							row[ column_name ] = string_SQL_2_GDT( enum_str );
						} else {
							line.push_back( string_SQL_2_GDT( enum_str ) );
						}
					}

					// CHAR / JSON
					// Why not getString instead getBlob? Becouse it has size limit and can't be used properly with JSON statements!
					else if ( d_type == sql::DataType::CHAR || d_type == sql::DataType::VARCHAR || d_type == sql::DataType::LONGVARCHAR || d_type == sql::DataType::JSON){
						std::unique_ptr< std::istream > raw(res->getBlob(i));
						raw->seekg (0, raw->end);
						int length = raw->tellg();
						raw->seekg (0, raw->beg);
						char buffer[length];
						raw->get(buffer, length +1);
						sql::SQLString p_str(buffer);
						String str = string_SQL_2_GDT( p_str );
						if ( data_model == DICTIONARY ) {
							row[ column_name ] = str;
						} else {
							line.push_back( str );
						}
					}

					// BINARY
					else if (d_type == sql::DataType::BINARY || d_type == sql::DataType::VARBINARY || d_type == sql::DataType::LONGVARBINARY ){
						std::unique_ptr< std::istream > raw(res->getBlob(i));
						raw->seekg (0, raw->end);
						int length = raw->tellg();
						raw->seekg (0, raw->beg);
						char buffer[length];
						raw->get(buffer, length +1);
						Vector<uint8_t> ret_data;
						ret_data.resize(length);
						memcpy(ret_data.ptrw(), buffer, length);
						if ( data_model == DICTIONARY ) {
							row[ column_name ] = ret_data;
						} else {
							line.push_back( ret_data );
						}
					}

					else{
						// This module can't recognize this format.
						ERR_FAIL_COND_V_EDMSG( true, Array(), "Format not supoeted!!");
					}
					// FITTING DATA  --------  END

				} // RETURN DATA WITH VALID TYPES

			} // FOR
			if ( data_model == DICTIONARY )
				{ ret.push_back( row );
			}else{
				ret.push_back( line );
			}
		}  // WHILE
	} // try

	catch (sql::SQLException &e) {
		print_SQLException(e);
	}

	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}

	return ret;
}


/*     PROPERTIES     */



Error MySQL::set_property(String p_property, Variant p_value){
	int val_type = p_value.get_type();
	int prop_type = get_prop_type(p_property);
	String prop = p_property;
	sql::SQLString property = prop.utf8().get_data();

	if ( property == "schema") {
		String database = p_value;
		return set_database( database );
	}

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

		case MySQL::TYPE::LIST:{
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
	} // SWITCH
	return OK;
}


Variant MySQL::get_property(String p_property){
	int prop_type = get_prop_type(p_property);
	sql::SQLString property = p_property.utf8().get_data();

	if (property == "schema"){
		return get_database();
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

		case MySQL::TYPE::LIST:
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
	Error erro;
	for ( int i = 0; i < p_properties.size(); i++){
		erro = set_property( p_properties.keys()[i], p_properties.values()[i] );
		ERR_FAIL_COND_V( erro != OK, erro );
	}
	return OK;
}


Dictionary MySQL::get_properties_array(PoolStringArray p_properties){
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




/*      HELPERS       */

bool MySQL::is_json( Variant p_arg ){
#ifdef GODOT4
	JSON json;
	return (json.parse(p_arg) == OK);
#else
	String errs;
	int errl;
	Variant r_ret;
	Error err = JSON::parse(p_arg, r_ret, errs, errl);
	return (err == OK);
#endif
}


bool MySQL::is_mat_empty( Variant p_matrix ){
	int tp = p_matrix.get_type();
	if ( tp == Variant::DICTIONARY ){
		Dictionary matrix = p_matrix;
#ifdef GODOT4
		return matrix.is_empty();
#else
		return matrix.empty();
#endif
	}

#ifdef GODOT4
	if ( tp >= Variant::PACKED_BYTE_ARRAY and tp <= Variant::PACKED_BYTE_ARRAY ) {
		Array ret = p_matrix;
		return ret.is_empty();
	}
#else
	if ( tp >= Variant::POOL_BYTE_ARRAY and tp <= Variant::POOL_COLOR_ARRAY ) {
		Array ret = p_matrix;
		return ret.empty();
	}
#endif
	CRASH_NOW_MSG("The given type is not an matrix.");
}


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
		return MySQL::TYPE::LIST;
	}
	else{
		return MySQL::TYPE::INVALID;
	}
}


bool MySQL::is_mysql_time(String time){
	if (get_time_format( time ) == sql::DataType::UNKNOWN){
		return false;
	}else{
		return true;
	}
}


Array MySQL::format_time(String str, bool return_string) {
	Array datando;
	std::string strss =	str.utf8().get_data();
	char seps[] = ": -";
	char *token;
	token = strtok( &strss[0], seps );

	while( token != NULL ) {
		if (return_string) {
			datando.push_back( String(token) );   //--As String
		}
		else{
			datando.push_back( atoi(token) ); //--As Data (int)
		}

		token = strtok( NULL, seps );
	}
	return datando;
}


int MySQL::get_time_format(String time){
	std::string s_time = time.utf8().get_data();
	int len = time.length();
	if (s_time.find_first_not_of( "0123456789:- " ) == std::string::npos){
		// - 0000
		if ( len == 4 ) {
			if (s_time.find_first_not_of( "0123456789" ) == std::string::npos){
				return sql::DataType::YEAR;
			}
		}
		// - 00:00:00
		else if ( len == 8 ) {
			if ( time[2] ==  ':' && time[5] == ':' ) {
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 3 && String(arr_time[2]).length() == 2){
					return sql::DataType::TIME ;
				}
			}
		}
		// - 0000-00-00
		else if ( len == 10 ) {
			if ( time[4] ==  '-' && time[7] == '-') {
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 3 && String(arr_time[2]).length() == 2){
					return sql::DataType::DATE;
				}
			}
		}
		// - 0000-00-00 00:00:00
		else if ( len == 19 ) {
			if ( time[4] ==  '-' && time[7] == '-' && time[13] ==  ':' && time[16] == ':' && time[10] == ' ' ){
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 6 && String(arr_time[2]).length() == 2){
					return sql::DataType::TIMESTAMP;
				}
			}
		}
	}
	return sql::DataType::UNKNOWN;
}


void MySQL::set_datatype(std::shared_ptr<sql::PreparedStatement> prep_stmt, std::stringstream *multiBlob, Variant arg, int index){
	int value_type = arg.get_type();

	if (value_type == Variant::NIL){
		prep_stmt->setNull(index+1, sql::DataType::SQLNULL);
	}
	else if (value_type == Variant::BOOL){
		prep_stmt->setBoolean(index+1, bool(arg));
	}
	else if (value_type == Variant::INT){
		prep_stmt->setInt64(index+1, int32_t(arg));
	}

#ifdef GODOT4
	else if (value_type == Variant::FLOAT){
#else
	else if (value_type == Variant::REAL){
#endif
		prep_stmt->setDouble(index+1, double(arg));
	}

	else if (value_type == Variant::STRING){
		String gdt_data = arg;
		std::string std_string = gdt_data.utf8().get_data();
		sql::SQLString sql_string = gdt_data.utf8().get_data();

		if ( is_mysql_time( gdt_data )) {
			prep_stmt->setDateTime(index+1, sql_string );
		}
		else if ( is_json( gdt_data ) ) {
			*multiBlob << std_string;
			prep_stmt->setBlob(index+1, multiBlob );
		}
		else{
			prep_stmt->setString(index+1, sql_string );
		}
	}

#ifdef GODOT4
	else if (value_type == Variant::Variant::PACKED_BYTE_ARRAY){
		Vector<uint8_t> data = arg;
		for ( int y = 0; y < data.size(); y++){
			*multiBlob << (char)data[y];
		}
#else
	else if (value_type == Variant::POOL_BYTE_ARRAY){
		PoolByteArray _data = arg;
		PoolVector<uint8_t>::Read in_buffer = _data.read();
		for ( int y = 0; y < _data.size(); y++){
			*multiBlob << (char)in_buffer[y];
		}
#endif
		prep_stmt->setBlob(index+1, multiBlob);
	}
}


/*       GODOT        */

void MySQL::_bind_methods() {

	/*     CONNECTIONS    */
	ClassDB::bind_method(D_METHOD("set_credentials", "HostName", "UserName", "Password"),&MySQL::set_credentials);
	ClassDB::bind_method(D_METHOD("connection_start"),&MySQL::connection_start);
	ClassDB::bind_method(D_METHOD("connection_stop"),&MySQL::connection_stop);
	ClassDB::bind_method(D_METHOD("connection_status"),&MySQL::connection_status);
	ClassDB::bind_method(D_METHOD("get_metadata"),&MySQL::get_metadata);

	/*     PROPERTIES     */
	ClassDB::bind_method(D_METHOD("get_property", "property"),&MySQL::get_property);
	ClassDB::bind_method(D_METHOD("set_property", "property", "value"),&MySQL::set_property);
	ClassDB::bind_method(D_METHOD("set_properties_array", "properties"),&MySQL::set_properties_array);
	ClassDB::bind_method(D_METHOD("get_properties_array", "properties"),&MySQL::get_properties_array);


	/*     CONNECTOR      */
//	ClassDB::bind_method(D_METHOD("set_client_option", "option", "value"),&MySQL::set_client_option);
//	ClassDB::bind_method(D_METHOD("get_client_option", "option"),&MySQL::get_client_option);
//	ClassDB::bind_method(D_METHOD("set_session_variable", "varname", "value"),&MySQL::set_session_variable);
//	ClassDB::bind_method(D_METHOD("get_session_variable", "varname"),&MySQL::get_session_variable);

	/*      DATABASE      */
	ClassDB::bind_method(D_METHOD("query", "Sql Statement", "DataFormat", "Return data as String", "Metadata"),&MySQL::query, DEFVAL(DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()) );
	ClassDB::bind_method(D_METHOD("query_prepared", "sql_statement", "Values", "DataFormat", "return_string", "meta"), &MySQL::query_prepared, DEFVAL(Array()), DEFVAL(MySQL::DataFormat::DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()));
	ClassDB::bind_method(D_METHOD("execute", "sql_statement"),&MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "sql_statement", "Values"),&MySQL::execute_prepared);
	ClassDB::bind_method(D_METHOD("update", "sql_statement"),&MySQL::update);
	ClassDB::bind_method(D_METHOD("update_prepared", "sql_statement", "Values"),&MySQL::update_prepared);

	/*    TRANSACTION     */
	ClassDB::bind_method(D_METHOD("set_autocommit", "bool"),&MySQL::set_autocommit);
	ClassDB::bind_method(D_METHOD("get_autocommit"),&MySQL::get_autocommit);
	ClassDB::bind_method(D_METHOD("set_database", "database"),&MySQL::set_database);
	ClassDB::bind_method(D_METHOD("get_database"),&MySQL::get_database);
	ClassDB::bind_method(D_METHOD("set_transaction_isolation", "level"),&MySQL::set_transaction_isolation);
	ClassDB::bind_method(D_METHOD("get_transaction_isolation"),&MySQL::get_transaction_isolation);
	ClassDB::bind_method(D_METHOD("set_readyonly", "readyonly"),&MySQL::set_readyonly);
	ClassDB::bind_method(D_METHOD("get_readyonly"),&MySQL::get_readyonly);
	ClassDB::bind_method(D_METHOD("set_catalog", "catalog"),&MySQL::set_catalog);
	ClassDB::bind_method(D_METHOD("get_catalog"),&MySQL::get_catalog);
	ClassDB::bind_method(D_METHOD("get_client_info"),&MySQL::get_client_info);





//	ClassDB::bind_method(D_METHOD("commit"),&MySQL::commit);

//	ClassDB::bind_method(D_METHOD("set_transaction_isolation", "level"),&MySQL::set_transaction_isolation);
//	ClassDB::bind_method(D_METHOD("get_transaction_isolation"),&MySQL::get_transaction_isolation);

//	ClassDB::bind_method(D_METHOD("rollback_savepoint", "savepoint"),&MySQL::rollback_savepoint);
//	ClassDB::bind_method(D_METHOD("create_savepoint", "savepoint"),&MySQL::create_savepoint);
//	ClassDB::bind_method(D_METHOD("delete_savepoint", "savepoint"),&MySQL::delete_savepoint);
//	ClassDB::bind_method(D_METHOD("get_savepoints"),&MySQL::get_savepoints);

	ClassDB::bind_method(D_METHOD("teste", "teste"),&MySQL::teste);

//	BIND_ENUM_CONSTANT(TRANSACTION_ERROR);
/*
	BIND_ENUM_CONSTANT(sql::enum_transaction_isolation::TRANSACTION_NONE);
	BIND_ENUM_CONSTANT(sql::enum_transaction_isolation::TRANSACTION_READ_COMMITTED);
	BIND_ENUM_CONSTANT(sql::enum_transaction_isolation::TRANSACTION_READ_UNCOMMITTED);
	BIND_ENUM_CONSTANT(sql::enum_transaction_isolation::TRANSACTION_REPEATABLE_READ);
	BIND_ENUM_CONSTANT(sql::enum_transaction_isolation::TRANSACTION_SERIALIZABLE);
*/
	BIND_ENUM_CONSTANT(TRANSACTION_ERROR);
	BIND_ENUM_CONSTANT(TRANSACTION_NONE);
	BIND_ENUM_CONSTANT(TRANSACTION_READ_COMMITTED);
	BIND_ENUM_CONSTANT(TRANSACTION_READ_UNCOMMITTED);
	BIND_ENUM_CONSTANT(TRANSACTION_REPEATABLE_READ);
	BIND_ENUM_CONSTANT(TRANSACTION_SERIALIZABLE);


	BIND_ENUM_CONSTANT(COLUMNS_NAMES);
	BIND_ENUM_CONSTANT(COLUMNS_TYPES);
	BIND_ENUM_CONSTANT(ATTRIBUTES);
	BIND_ENUM_CONSTANT(TABLE_INFO);
	BIND_ENUM_CONSTANT(NO_QUERY);

	BIND_ENUM_CONSTANT(NO_CONNECTION);
	BIND_ENUM_CONSTANT(CLOSED);
	BIND_ENUM_CONSTANT(CONNECTED);
	BIND_ENUM_CONSTANT(DISCONNECTED);

	BIND_ENUM_CONSTANT(ARRAY);
	BIND_ENUM_CONSTANT(DICTIONARY);

}


MySQL::MySQL(){
	connection_properties["port"] = 3306;
	connection_properties["OPT_RECONNECT"] = true;
	connection_properties["CLIENT_MULTI_STATEMENTS"] = false;
	connection_properties["OPT_REPORT_DATA_TRUNCATION"] = false;
/*
IF DEBUG
https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-debug-tracing.html
con->setClientOption("libmysql_debug", "d:t:O,client.trace");
*/
}


MySQL::~MySQL(){
}

/*  mysql.cpp */

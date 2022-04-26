//*  MySQL.cpp */
#include "core/io/json.h"
#include "mysql.h"


//--------FETCH QUERY / EXECUTE / UPDADE-------------------

Array MySQL::query(String p_sqlquery, DataFormat data_model, bool return_string, PoolIntArray meta_col){
	return _query(p_sqlquery, Array(), data_model, return_string, meta_col, false); 
}

Array MySQL::query_prepared(String p_sqlquery, Array p_values, DataFormat data_model, bool return_string, PoolIntArray meta_col){ 
	return _query(p_sqlquery, p_values, data_model, return_string, meta_col, true); 
}

//-----
bool MySQL::execute(String p_sqlquery){
	return bool(_execute( p_sqlquery, Array(), false, false ));
}

bool MySQL::execute_prepared(String p_sqlquery, Array p_values){
	return bool(_execute( p_sqlquery, p_values, true, false ));
}

//-----
int MySQL::update( String p_sqlquery ){ 
	return _execute( p_sqlquery, Array(), false, true ); 
}

int MySQL::update_prepared( String p_sqlquery, Array p_values){ 
	return _execute( p_sqlquery, p_values, true, true); 
}



//--------HEAVY DUTY-------------------

Array MySQL::_query(String p_sqlquery, Array p_values, DataFormat data_model, bool return_string, PoolIntArray meta_col, bool _prep){
	sql::SQLString query = p_sqlquery.utf8().get_data();
	Array ret;
	std::shared_ptr <sql::PreparedStatement> prep_stmt;
	std::unique_ptr <sql::Statement> stmt;
	std::unique_ptr <sql::ResultSet> res;
	sql::ResultSetMetaData *res_meta;
	int data_size = p_values.size();

	try {		

		// Prepared statement
		if ( _prep ) {	
			prep_stmt.reset(conn->prepareStatement(query));
			std::vector<std::stringstream> multiBlob (data_size);

			for (int h =0; h < data_size; h++){
				set_datatype(prep_stmt, &multiBlob[h], p_values[h], h);	
			}

			res.reset( prep_stmt->executeQuery());	
			res_meta = res->getMetaData();

		}

		// Non Prepared statement
		else{ 
			stmt.reset(conn->createStatement());
			res.reset( stmt->executeQuery(query));
			res_meta = res->getMetaData();
		}

		Array columnname;
		Array columntypes;
		Array columninfo;

		for (int m = 0; m < meta_col.size(); m++){

#ifdef GODOT4
			if ( meta_col[m] == COLUMNS_NAMES && columnname.is_empty()){
#else	
			if ( meta_col[m] == COLUMNS_NAMES && columnname.empty()){
#endif		
				for (uint8_t i = 1; i <= res_meta->getColumnCount(); i++) {
					sql::SQLString _stri = res_meta->getColumnName(i);
					columnname.push_back( SQLstr2GDstr( _stri ));
				}
				ret.push_back(columnname);
			}

#ifdef GODOT4
			if (meta_col[m] == COLUMNS_TYPES && columntypes.is_empty()){
				for (uint8_t i = 1; i <= res_meta->getColumnCount(); i++) {
					sql::SQLString _stri = res_meta->getColumnTypeName(i);
					columntypes.push_back( SQLstr2GDstr( _stri ));
				}
				ret.push_back(columntypes);
			}
			if (meta_col[m] == INFO && columninfo.is_empty()){

#else	
			if (meta_col[m] == COLUMNS_TYPES && columntypes.empty()){
				for (uint8_t i = 1; i <= res_meta->getColumnCount(); i++) {
					sql::SQLString _stri = res_meta->getColumnTypeName(i);
					columntypes.push_back( SQLstr2GDstr( _stri ));
				}
				ret.push_back(columntypes);
			}
			if (meta_col[m] == INFO && columninfo.empty()){
#endif
			//TODO
				ret.push_back(columninfo);
			}
		}

		//--------FETCH DATA
		while (res->next()) {
			Array line;
			Dictionary row;

			for ( unsigned int i = 1; i <= res_meta->getColumnCount(); i++) { 
				sql::SQLString _col_name = res_meta->getColumnName(i);
				String column_name = SQLstr2GDstr( _col_name );
				int d_type = res_meta->getColumnType(i);

				//--------RETURN DATA AS STRING       
				if ( return_string ){
					sql::SQLString _val = res->getString(i);
					String _value = SQLstr2GDstr( _val );

					if ( data_model == DICTIONARY ){
						row[ column_name ] = _value; 			
					}else{
						line.push_back( _value );					
					}
				}
				
				//--------RETURN DATA WITH IT'S SELF TYPES
				else{  

					//	NULL
					if ( res->isNull(i) ){
						if ( data_model == DICTIONARY ){
							row[ column_name ] = Variant();
						}else{
							line.push_back( Variant() ); 
						}
					}

					//	BOOL
					else if ( d_type == sql::DataType::BIT ){
						if ( data_model == DICTIONARY ){
							row[ column_name ] = res->getBoolean(i); 
						}else{
							line.push_back( res->getBoolean(i) ); 
						}
					}

					//	INT
					else if ( d_type == sql::DataType::TINYINT || d_type == sql::DataType::SMALLINT || d_type == sql::DataType::MEDIUMINT) {
						if ( data_model == DICTIONARY ){ 
							row[ column_name ] = res->getInt(i); 
						}else{ 
							line.push_back( res->getInt(i) ); 
						}
					}
					
					//	BIG INT
					else if ( d_type == sql::DataType::INTEGER || d_type == sql::DataType::BIGINT ) {
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = res->getInt64(i); 
						}else{ 
							line.push_back( res->getInt64(i) ); 
						}
					}

					//	FLOAT 
					else if ( d_type == sql::DataType::REAL || d_type == sql::DataType::DOUBLE || d_type == sql::DataType::DECIMAL || d_type == sql::DataType::NUMERIC ) {
						double my_float = res->getDouble(i);
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = my_float; 
						}else{ 
						line.push_back( my_float ); 
						}							
					}

					//	TIME
					// It should return time information as a dictionary but if the sequence of the data be modified (using TIME_FORMAT or DATE_FORMAT for exemple), 
					// it will return the dictionary fields with wrong names. So I prefer return the data as an array.	
					else if ( d_type == sql::DataType::DATE || d_type == sql::DataType::TIME || d_type == sql::DataType::TIMESTAMP || d_type == sql::DataType::YEAR ) {
						sql::SQLString _stri = res->getString(i);
						Array time = format_time( SQLstr2GDstr( _stri ) , false );

						if ( data_model == DICTIONARY ){ 
							row[ column_name ] = time;
						}else{
							line.push_back( time );	
						}
					}

					// STRING - JSON_STRING - VARIANT
					// Why not getString instead getBlob? Becouse it has size limit and can't be used properly with JSON statements!
					else if (d_type == sql::DataType::ENUM || d_type == sql::DataType::CHAR || d_type == sql::DataType::VARCHAR || d_type == sql::DataType::LONGVARCHAR || d_type == sql::DataType::JSON){
						std::unique_ptr< std::istream > raw(res->getBlob(i));
						raw->seekg (0, raw->end);
				   		int length = raw->tellg();
				   		raw->seekg (0, raw->beg);
				   		char buffer[length];
				   		raw->get(buffer, length +1);
				   		sql::SQLString p_str(buffer);
					 	String str = SQLstr2GDstr( p_str );
						Variant variant_data = _marshalls->base64_to_variant(str, encode_objects);
						
						//STRING - JSON_STRING
						if (variant_data.get_type() == Variant::NIL){
							variant_data = str;
						}
						
						//Fetch data
						if ( data_model == DICTIONARY ){
							row[ column_name ] = variant_data;
						}else{
							line.push_back( variant_data );
						}
					}				


					//  BINARY 
					else if (d_type == sql::DataType::BINARY || d_type == sql::DataType::VARBINARY || d_type == sql::DataType::LONGVARBINARY ){
						std::unique_ptr< std::istream > out_stream(res->getBlob(1));
						out_stream->seekg(0, out_stream->end);
						int out_length = out_stream->tellg();
						out_stream->seekg(std::ios::beg);
						char bytes[out_length];
						out_stream->get(bytes, out_length +1);
						PoolByteArray ret_data;

#ifdef GODOT4
						for (int w = 0; w < out_length; w++){
							ret_data.push_back( ((uint8_t)bytes[w]) );
						}

#else
						ret_data.resize(out_length);
						PoolVector<uint8_t>::Write _data = ret_data.write();
						for (int w = 0; w < out_length; w++){
							_data[w] = ((uint8_t)bytes[w]);
						}
						_data.release();
#endif

						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = ret_data; 
						}else{ 
							line.push_back( ret_data ); 
						}	
					}else{
						// This module can't recognize this format.
						ERR_FAIL_COND_V_MSG( get_connection_status() != CONNECTED, Array(), "Format not supoeted!!");
					}

				} // ELSE -> RETURN DATA
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
			
			if (update){
				afectedrows = prep_stmt->executeUpdate();
			}else{
				afectedrows = int (prep_stmt->execute());
			}
			
		}else{
			std::unique_ptr <sql::Statement> stmt;
			stmt.reset(conn->createStatement());
			afectedrows = stmt->executeUpdate(query);
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



void MySQL::set_datatype(std::shared_ptr<sql::PreparedStatement> prep_stmt, std::stringstream *blob, Variant arg, int index){
	int value_type = arg.get_type();
	
	//NULL  
		if (value_type == Variant::NIL){ 
			prep_stmt->setNull(index+1, sql::DataType::SQLNULL); 
			std::cout << "NILL: " << std::endl; 
		}  

	//BOOL
		else if (value_type == Variant::BOOL){ 
			prep_stmt->setBoolean(index+1, bool(arg)); 
			std::cout << "BOOL: " << std::boolalpha << bool(arg)  << std::endl; 
		}
				
	//INT
		else if (value_type == Variant::INT){ 
			prep_stmt->setInt64(index+1, int64_t(arg)); 
			std::cout << "INT: " << int64_t(arg) << std::endl; 
		}
		
	// FLOAT
#ifdef GODOT4		
		else if (value_type == Variant::FLOAT){ 
#else
		else if (value_type == Variant::REAL){ 
#endif
			prep_stmt->setDouble(index+1, double(arg)); 
		}

	// STRING - DATATIME - JSON_STRING
		else if (value_type == Variant::STRING){
			String gdt_data = arg;
			sql::SQLString sql_data = gdt_data.utf8().get_data();
			std::string std_string = gdt_data.utf8().get_data();

			if ( is_mysql_time( gdt_data )) {
				prep_stmt->setDateTime(index+1, sql_data );
			}else if ( is_json( gdt_data ) ) {
				*blob << std_string;
				prep_stmt->setBlob(index+1, blob );		
			}else{
				prep_stmt->setString(index+1, sql_data );
			}
		}

	// BINARY

#ifdef GODOT4
		else if (value_type == Variant::Variant::PACKED_BYTE_ARRAY){
			PoolByteArray _data = arg;
			for ( int y = 0; y < _data.size(); y++){
				*blob << (char)_data[y];
			}

#else	
		else if (value_type == Variant::POOL_BYTE_ARRAY){	
			PoolByteArray _data = arg;
			PoolVector<uint8_t>::Read in_buffer = _data.read();
			for ( int y = 0; y < _data.size(); y++){
				*blob << (char)in_buffer[y];
			}
#endif
			prep_stmt->setBlob(index+1, blob);
		}

	// VARIANT
		else{
			String buff = _marshalls->variant_to_base64(arg, encode_objects);
			*blob << buff.utf8().get_data();
			prep_stmt->setBlob(index+1, blob);
		}
}
	

//---------------------------------------------------------------------------------------------

/*
https://dev.mysql.com/doc/refman/8.0/en/blob.html  NOTE: Afeta o desempenho
https://mariadb.com/kb/en/json-data-type/
*/


//--------CONNECTION-------------------

MySQL::ConnectionStatus MySQL::get_connection_status(){
	MySQL::ConnectionStatus ret;	
	if (conn.get()) {
		if (!conn-> isClosed()) {
			if (conn-> isValid()){
				ret = CONNECTED;
			}else{
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


MySQL::MySQLException MySQL::start_connection(){
	try {
		driver = sql::mysql::get_mysql_driver_instance();
		conn.reset(driver->connect(connection_properties));
	}

	catch (sql::SQLException &e) {
		print_SQLException(e);
	} 


#ifdef DEBUG_ENABLED
	catch (std::runtime_error &e) {
		print_runtime_error(e);       //FIXME: 	ERR_FAIL_		
	}
#endif
	MySQLException Err_Except = sqlError.duplicate();
	sqlError.clear();
	return Err_Except;
}



MySQL::ConnectionStatus MySQL::stop_connection(){
	ConnectionStatus ret;
	if (conn.get()) {// != NULL
		ret = ConnectionStatus::CLOSED; 
		if (!conn->isClosed()) {
			conn->close();
		}
	}else{
		ret = ConnectionStatus::NO_CONNECTION; 
	}
	return ret;  	
}


//--------PROPERTIES-------------------


void MySQL::set_multi_statement(bool _multi_statement) {
	connection_properties["CLIENT_MULTI_STATEMENTS"] = _multi_statement;
	set_property("CLIENT_MULTI_STATEMENTS", _multi_statement); 
}


void MySQL::set_reconnection(bool _can_reconnect) {
	connection_properties["OPT_RECONNECT"] =_can_reconnect;
	set_property("OPT_RECONNECT", _can_reconnect); 
}


void MySQL::set_properties_array(Dictionary p_properties){
	ERR_FAIL_COND_MSG( get_connection_status() != CONNECTED, "DatabaseMetaData FAILURE - database not connected! - METHOD: set_properties_set");
	for ( int i = 0; i < p_properties.size(); i++){
		set_property( p_properties.keys()[i], p_properties.values()[i] );
	}
}


Dictionary MySQL::get_properties_array(Array p_properties){
	ERR_FAIL_COND_V_MSG( get_connection_status() != CONNECTED, Dictionary(), "DatabaseMetaData FAILURE - database not connected! - METHOD: get_properties_set");
	Dictionary ret;
	for ( int i = 0; i < p_properties.size(); i++){
		ret[ p_properties[i] ] = get_property( p_properties[i] );
	}
	return ret;
}



void MySQL::set_credentials( String p_host, String p_user, String p_pass ) {
	connection_properties["hostName"] = p_host.utf8().get_data();
	connection_properties["userName"] = p_user.utf8().get_data();
	connection_properties["password"] = p_pass.utf8().get_data();
}	


void MySQL::set_property(String p_property, Variant p_value){
	sql::SQLString property = p_property.utf8().get_data();
	std::string value_type = get_prop_type( property );
	String _val = p_value;
	sql::SQLString value = _val.utf8().get_data();
	
	if (value_type == "string" ){ 
		connection_properties[property] = (sql::SQLString)value; 
	}

	else if (value_type == "void"){ 
		connection_properties[property] = (std::string)value; 
	}

	else if (value_type == "bool"){ 
		connection_properties[property] = (bool)p_value; 
	}

	else if (value_type == "int" ){ 
		connection_properties[property] = (int)p_value; 
	}

/*	TODO suporte para MAP(dictionary) 
	else if (value_type == "map" ){
	}
*/
	else{
		ERR_FAIL_MSG("Invalid data type. For more information visit: https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-connect-options.html");
	}

	if (conn != NULL){
		if (property == "schema"){
			conn->setSchema( value );
		}else{
			conn->setClientOption(property, value );
		}
	}
}


Variant MySQL::get_property(String p_property){
	sql::SQLString property = p_property.utf8().get_data();
	std::string value_type = get_prop_type( property );

	if (property == "schema"){
		sql::SQLString _p_s = conn->getSchema();
		return SQLstr2GDstr( _p_s ); 
	}

	if (value_type == "string" || value_type == "void"){ 
		return SQLstr2GDstr( *connection_properties[ property ].get<sql::SQLString>() ); 
	}

	else if (value_type == "int" ){
		return *connection_properties[ property ].get< int >();
	}

	else if (value_type == "bool"){
		return *connection_properties[ property ].get< bool >();
	}

	//	TODO suporte para MAP(dictionary) 
	//else if (value_type == "map" ) {FIXME}

	else{
		ERR_FAIL_V_MSG(Variant(), "Invalid data type. For more information visit: https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-connect-options.html");
	}

}


String MySQL::get_database(){
	if (conn != NULL){
		sql::SQLString database = conn->getSchema();
		return SQLstr2GDstr(database);
	}else{
		return (String)"Invalid Connection!";
	}
}


void MySQL::set_database( String p_database ){
	sql::SQLString database = p_database.utf8().get_data();
	if(database != ""){
		if (conn != NULL){
			conn->setSchema(database);
		}else{
			connection_properties["schema"] = database;
		}
	}
}


Dictionary MySQL::get_metadata(){
	ERR_FAIL_COND_V_MSG(get_connection_status() != CONNECTED, Dictionary(), "DatabaseMetaData FAILURE - database is not connected! - METHOD: get_metadata");
	Dictionary ret;
	sql::DatabaseMetaData *dbcon_meta = conn->getMetaData();
	std::unique_ptr < sql::ResultSet > res(dbcon_meta->getSchemas());
	ret["Total number of schemas"] = res->rowsCount();
	ret["Database Product Name"] = SQLstr2GDstr( (sql::SQLString&)(dbcon_meta->getDatabaseProductName()));
	sql::SQLString _version = dbcon_meta->getDatabaseProductVersion();
	ret["Database Product Version"] = SQLstr2GDstr(_version);
	sql::SQLString _name = dbcon_meta->getUserName();
	ret["Database User Name"] = SQLstr2GDstr(_name);
	ret["Driver name"] = SQLstr2GDstr( (sql::SQLString&)(dbcon_meta->getDriverName()));
	ret["Driver version"] = SQLstr2GDstr( (sql::SQLString&)(dbcon_meta->getDriverVersion()));
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



//--------TRANSACTION-------------------

Error MySQL::create_savepoint(String p_savept){
	ERR_FAIL_COND_V_MSG(get_connection_status() != CONNECTED, ERR_DATABASE_CANT_WRITE, "DatabaseMetaData FAILURE - database is not connected! - METHOD: create_savepoint");
	if (savepoint_map.count(p_savept) != 0){
		return ERR_ALREADY_EXISTS;
	}
	sql::Savepoint *savept;
	savept = conn->setSavepoint( p_savept.utf8().get_data() );
	savepoint_map.insert( std::pair<String, sql::Savepoint*> (p_savept, savept) );
	return OK;
}



Error MySQL::delete_savepoint(String p_savept){
	ERR_FAIL_COND_V_MSG(get_connection_status() != CONNECTED, ERR_DATABASE_CANT_WRITE, "DatabaseMetaData FAILURE - database is not connected! - METHOD: delete_savepoint");
	if (savepoint_map.count(p_savept) == 0){
		return ERR_DOES_NOT_EXIST;
	}
	conn->releaseSavepoint( savepoint_map[p_savept] ); 
	delete savepoint_map[p_savept];
	savepoint_map.erase(p_savept);
	return OK;
}



#ifdef GODOT4
	PackedStringArray MySQL::get_savepoints(){
#else	
	PoolStringArray MySQL::get_savepoints(){
#endif
	ERR_FAIL_COND_V_MSG(get_connection_status() != CONNECTED, PoolStringArray(), "DatabaseMetaData FAILURE - database is not connected! - METHOD: get_savepoints");
	PoolStringArray ret;
	std::map<String, sql::Savepoint*>::iterator it;
	for(auto x:savepoint_map){
		ret.append(x.first);
	}
	return ret;
}



//--------HELPERS-------------------

String MySQL::SQLstr2GDstr( sql::SQLString &p_string ){
	const char * _str_ = p_string.c_str();
	String str = String::utf8( (char *) _str_ );
	return str;
}


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


std::string	MySQL::get_prop_type( std::string p_prop ){ 
	for (int i = 0; i < (int)sizeof(string_properties)/32; i++){ 
		if (string_properties[i] == p_prop) {
			return "string"; 
		} 
	}

	for (int i = 0; i < (int)sizeof(sqlmap_properties)/32; i++){ 
		if (sqlmap_properties[i] == p_prop) {
			return "map"; 
		} 
	}

	for (int i = 0; i < (int)sizeof(bool_properties)/32; i++){ 
		if (bool_properties[i] == p_prop) {
			return "bool"; 
		} 
	}

	for (int i = 0; i < (int)sizeof(void_properties)/32; i++){ 
		if (void_properties[i] == p_prop) {
			return "void"; 
		} 
	}

	for (int i = 0; i < (int)sizeof(int_properties)/32; i++){ 
		if (int_properties[i] == p_prop) {
			return "int"; 
		} 
	}

	return "invalid";
}


bool MySQL::is_mysql_time(String time){
	if (get_time_format( time ) == sql::DataType::UNKNOWN){
		return false;
	}else{
		return true;
	}
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



//--------ERRORS-------------------

void MySQL::print_SQLException(sql::SQLException &e) {
	//If (e.getErrorCode() == 1047) = Your server does not seem to support Prepared Statements at all. Perhaps MYSQL < 4.1?.
		
	Variant file = __FILE__;
	Variant line = __LINE__;
	Variant func = __FUNCTION__;
	Variant errCode = e.getErrorCode();
	sql::SQLString _err = e.getSQLState();
	String _error = SQLstr2GDstr(_err);

	sqlError.clear();
	sqlError["FILE"] = String( file );
	sqlError["FUNCTION"] = String( func );
	sqlError["LINE"] = String( line );
	sqlError["ERROR"] = String( e.what() );
	sqlError["MySQL error code"] = String( errCode );
	sqlError["SQLState"] = _error;
	
	// Cast error signal && return (typedef Dictionary MySQLException)

#ifdef TOOLS_ENABLED
	//FIXME: 	ERR_FAIL_	just print	 
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



//--------GODOT STUFF-------------------

void MySQL::_bind_methods() {

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_multi_statement"), "set_multi_statement", "get_multi_statement");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_objects"), "set_allow_objects", "get_allow_objects");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "can_reconnect"), "set_reconnection", "get_reconnection");


	ClassDB::bind_method(D_METHOD("set_multi_statement", "allow_multi_statement"),&MySQL::set_multi_statement);
	ClassDB::bind_method(D_METHOD("get_multi_statement"),&MySQL::get_multi_statement);
	ClassDB::bind_method(D_METHOD("set_allow_objects", "allow_objects"),&MySQL::set_allow_objects);
	ClassDB::bind_method(D_METHOD("get_allow_objects"),&MySQL::get_allow_objects);
	ClassDB::bind_method(D_METHOD("set_reconnection", "can_reconnect"),&MySQL::set_reconnection);
	ClassDB::bind_method(D_METHOD("get_reconnection"),&MySQL::get_reconnection);


	//--- Connection Managers
	ClassDB::bind_method(D_METHOD("set_credentials", "HostName", "UserName", "Password"),&MySQL::set_credentials);
	ClassDB::bind_method(D_METHOD("get_connection_status"),&MySQL::get_connection_status);
	ClassDB::bind_method(D_METHOD("start_connection"),&MySQL::start_connection);
	ClassDB::bind_method(D_METHOD("stop_connection"),&MySQL::stop_connection);
	ClassDB::bind_method(D_METHOD("get_metadata"),&MySQL::get_metadata);
	

	//--- Properties Managers
	ClassDB::bind_method(D_METHOD("set_property", "property", "p_value"),&MySQL::set_property);
	ClassDB::bind_method(D_METHOD("get_property", "property"),&MySQL::get_property);
	ClassDB::bind_method(D_METHOD("set_properties_array", "properties"),&MySQL::set_properties_array);
	ClassDB::bind_method(D_METHOD("get_properties_array", "properties"),&MySQL::get_properties_array);
	ClassDB::bind_method(D_METHOD("set_database", "database"),&MySQL::set_database);
	ClassDB::bind_method(D_METHOD("get_database"),&MySQL::get_database);


	//--- Execute/Update/Query
	ClassDB::bind_method(D_METHOD("execute", "sql_statement"),&MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "sql_statement", "Values"),&MySQL::execute_prepared);	
	ClassDB::bind_method(D_METHOD("update", "sql_statement"),&MySQL::update);
	ClassDB::bind_method(D_METHOD("update_prepared", "sql_statement", "Values"),&MySQL::update_prepared);
	ClassDB::bind_method(D_METHOD("query", "sql_statement", "DataFormat", "return_string", "meta"),&MySQL::query, DEFVAL(DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()) );
	ClassDB::bind_method(D_METHOD("query_prepared", "sql_statement", "Values", "DataFormat", "return_string", "meta"), &MySQL::query_prepared, DEFVAL(Array()), 
	DEFVAL(DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()));

	
	//--- Transaction
	ClassDB::bind_method(D_METHOD("set_auto_commit", "bool"),&MySQL::set_auto_commit);
	ClassDB::bind_method(D_METHOD("get_auto_commit"),&MySQL::get_auto_commit);
	ClassDB::bind_method(D_METHOD("commit"),&MySQL::commit);
	ClassDB::bind_method(D_METHOD("rollback_savepoint", "savepoint"),&MySQL::rollback_savepoint);
	ClassDB::bind_method(D_METHOD("create_savepoint", "savepoint"),&MySQL::create_savepoint);
	ClassDB::bind_method(D_METHOD("delete_savepoint", "savepoint"),&MySQL::delete_savepoint);
	ClassDB::bind_method(D_METHOD("get_savepoints"),&MySQL::get_savepoints);
	ClassDB::bind_method(D_METHOD("set_transaction_isolation", "level"),&MySQL::set_transaction_isolation);
	ClassDB::bind_method(D_METHOD("get_transaction_isolation"),&MySQL::get_transaction_isolation);


	BIND_ENUM_CONSTANT(TRANSACTION_NONE);
	BIND_ENUM_CONSTANT(TRANSACTION_READ_COMMITTED);
	BIND_ENUM_CONSTANT(TRANSACTION_READ_UNCOMMITTED);
	BIND_ENUM_CONSTANT(TRANSACTION_REPEATABLE_READ);
	BIND_ENUM_CONSTANT(TRANSACTION_SERIALIZABLE);

	BIND_ENUM_CONSTANT(NO_CONNECTION);
	BIND_ENUM_CONSTANT(CLOSED);
	BIND_ENUM_CONSTANT(CONNECTED);
	BIND_ENUM_CONSTANT(DISCONNECTED);

	BIND_ENUM_CONSTANT(ARRAY);
	BIND_ENUM_CONSTANT(DICTIONARY);

	BIND_ENUM_CONSTANT(COLUMNS_NAMES);
	BIND_ENUM_CONSTANT(COLUMNS_TYPES);
	BIND_ENUM_CONSTANT(INFO);

}


MySQL::MySQL(){
	connection_properties["port"] = 3306;
	connection_properties["OPT_RECONNECT"] = true;
	connection_properties["CLIENT_MULTI_STATEMENTS"] = false;
}


MySQL::~MySQL(){
	stop_connection();
}


/*
Variant MySQL::transaction( [] ){
}
Variant MySQL::transaction_prepared( {} ){
}
*/

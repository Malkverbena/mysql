//*  MySQL.cpp */


#include "MySQL.h"
#include "core/io/json.h"
#include <vector>


bool p_full_objects = true; //TODO: colocar o p_full_objects como propriedade ao lado do reconnect
bool use_json = false;



Variant MySQL::test(Array arg){

	sql::SQLString WHITE = "INSERT INTO TRANSACTION (data) VALUES (?)";
	sql::SQLString READ = "SELECT data FROM TRANSACTION ";
	
	setAutoCommit(false);


	try {
		print_line("");
	}

	catch (sql::SQLException &e) { 
		print_SQLException(e); 
		conn->rollback();
	}
	
	catch (std::runtime_error &e) { 
		print_runtime_error(e); 
	}



	Variant ret;
	return ret;
	

}	



//--------FETCH QUERY-------------------
Array MySQL::query(String p_sqlquery, DataFormat data_model, bool return_string, PoolIntArray meta_col){
	return _query(p_sqlquery, Array(), data_model, return_string, meta_col, false); 
}


Array MySQL::query_prepared(String p_sqlquery, Array p_values, DataFormat data_model, bool return_string, PoolIntArray meta_col){ 
	return _query(p_sqlquery, p_values, data_model, return_string, meta_col, true); 
}


Array MySQL::_query(String p_sqlquery, Array p_values, DataFormat data_model, bool return_string, PoolIntArray meta_col, bool _prep){

	// TODO: Testar conexão antes de prosseguir -> if conn == null: ERR_FAIL -> Start your connection 
	
	
//	if (get_connection_status() != CONNECTED) {
//			throw std::runtime_error("DatabaseMetaData FAILURE - database connection closed");
//	}
//	

	sql::SQLString query = p_sqlquery.utf8().get_data();
	Array ret;
	
	std::shared_ptr <sql::PreparedStatement> prep_stmt;
	std::unique_ptr <sql::Statement> stmt;
	std::unique_ptr <sql::ResultSet> res;
	sql::ResultSetMetaData *res_meta;
	int data_size = p_values.size();

	try {		

		//DatabaseMetaData *dbcon_meta = con->getMetaData();

		if (  _prep ) {	
			// Prepared statement
			prep_stmt.reset(conn->prepareStatement(query));
			std::vector<std::stringstream> multiBlob (data_size);

			for (int h =0; h < data_size; h++){
				set_datatype(prep_stmt, &multiBlob[h], p_values[h], h);	
			}

			res.reset( prep_stmt->executeQuery());	
			res_meta = res->getMetaData();
		} else { 
			// Non Prepared statement
			stmt.reset(conn->createStatement());
			res.reset( stmt->executeQuery(query));
			res_meta = res->getMetaData();
		}


		Array columnname;
		Array columntypes;
		Array columninfo;

		for (int m = 0; m < meta_col.size(); m++){
			if ( meta_col[m] == COLUMNS_NAMES && columnname.empty()){
				for (uint8_t i = 1; i <= res_meta->getColumnCount(); i++) {
					sql::SQLString _stri = res_meta->getColumnName(i);
					columnname.push_back( SQLstr2GDstr( _stri ));
				}
				ret.push_back(columnname);
			}

			if (meta_col[m] == COLUMNS_TYPES && columntypes.empty()){
				for (uint8_t i = 1; i <= res_meta->getColumnCount(); i++) {
					sql::SQLString _stri = res_meta->getColumnTypeName(i);
					columntypes.push_back( SQLstr2GDstr( _stri ));
				}
				ret.push_back(columntypes);
			}

			if (meta_col[m] == INFO && columninfo.empty()){
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

				//--------RETURN STRING       
				if ( return_string ) {
					sql::SQLString _val = res->getString(i);
					String _value = SQLstr2GDstr( _val );

					if ( data_model == DICTIONARY ){
						row[ column_name ] = _value; 			
					} else {
						line.push_back( _value );					
					}
				}
				
				//--------RETURN DATA WITH IT'S SELF TYPES
				else{  
					//	NULL
					if ( res->isNull(i) ){
						if ( data_model == DICTIONARY ){
							row[ column_name ] = Variant();
						} else {
							line.push_back( Variant() ); 
						}
					}

					//	BOOL
					else if ( d_type == sql::DataType::BIT ){
						if  ( data_model == DICTIONARY ){
							row[ column_name ] = res->getBoolean(i); 
						} else {
							line.push_back( res->getBoolean(i) ); 
						}
					}

					//	INT
					else if ( d_type == sql::DataType::ENUM || d_type == sql::DataType::TINYINT || d_type == sql::DataType::SMALLINT || d_type == sql::DataType::MEDIUMINT) {
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = res->getInt(i); 
						} else { 
							line.push_back( res->getInt(i) ); 
						}
					}

					//	BIG INT
					else if ( d_type == sql::DataType::INTEGER || d_type == sql::DataType::BIGINT ) {
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = res->getInt64(i); 
						} else { 
							line.push_back( res->getInt64(i) ); 
						}
					}

					//	FLOAT 
					else if ( d_type == sql::DataType::REAL || d_type == sql::DataType::DOUBLE || d_type == sql::DataType::DECIMAL || d_type == sql::DataType::NUMERIC ) {
						double my_float = res->getDouble(i);
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = my_float; 
						} else { 
						line.push_back( my_float ); 
						}							
					}

					//	TIME
					// It should return time information as a dictionary but if the sequence of the data be modified (using TIME_FORMAT or DATE_FORMAT for exemple), 
					// it will return the dictionary fields with wrong names. So I prefer return the data as an array.	
					else if ( d_type == sql::DataType::DATE || d_type == sql::DataType::TIME || d_type == sql::DataType::TIMESTAMP || d_type == sql::DataType::YEAR ) {
						sql::SQLString _stri = res->getString(i);
						Array time = format_time( SQLstr2GDstr( _stri ) , false );
					
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = time;
						} else {
							line.push_back( time );	
						}
					}

					//	STRING - VARIANT - JSON
					// Why not getString instead getBlob? Becouse it has size limit and can't be used properly with JSON statements!
					else if (d_type == sql::DataType::CHAR || d_type == sql::DataType::VARCHAR || d_type == sql::DataType::LONGVARCHAR || d_type == sql::DataType::JSON ){

						std::unique_ptr< std::istream > raw(res->getBlob(i));

						raw->seekg (0, raw->end);
				   		int length = raw->tellg();
				   		raw->seekg (0, raw->beg);

				   		char buffer[length];
				   		raw->get(buffer, length +1);

				   		sql::SQLString p_str(buffer);
				   		String str = SQLstr2GDstr( p_str );

				   		_Marshalls *marshalls = memnew(_Marshalls);
				   		Variant _data = marshalls->base64_to_variant(str, p_full_objects);

						if  ( data_model == DICTIONARY ){ 
							if (_data.get_type() == Variant::NIL){
								row[ column_name ] = str;
							} else{ 
								row[ column_name ] = _data;
							}
						} else { 
							if (_data.get_type() == Variant::NIL){ 
								line.push_back( str ); 
							} else {
								line.push_back( _data );
							}							
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
						ret_data.resize(out_length);
						PoolVector<uint8_t>::Write _data = ret_data.write();

						for (int w = 0; w < out_length; w++){
							_data[w] = ((uint8_t)bytes[w]);
						}

						_data.release();

						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = ret_data; 
						} else { 
							line.push_back( ret_data ); 
						}			
					} else {
						//TODO
						print_line("Format not supoeted!!");
					}

				} // ELSE -> RETURN DATA
			} // FOR

			if ( data_model == DICTIONARY ) 
				{ ret.push_back( row ); 
			} else { 
				ret.push_back( line ); 
			}		
		}  // WHILE
	} // try
	
	catch (sql::SQLException &e) { print_SQLException(e); } 
	catch (std::runtime_error &e) { print_runtime_error(e); }

	return ret;

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


int MySQL::_execute( String p_sqlquery, Array p_values, bool prep_st, bool update){

	if (get_connection_status() != CONNECTED) {
			throw std::runtime_error("DatabaseMetaData FAILURE - database connection closed");
	}
	
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
		
	catch (sql::SQLException &e) { print_SQLException(e); } 
	catch (std::runtime_error &e) { print_runtime_error(e); }

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
		else if (value_type) == Variant::FLOAT){ 
			prep_stmt->setDouble(index+1, double(arg)); 
		}
#else
		else if (value_type == Variant::Variant::REAL){ 
			prep_stmt->setDouble(index+1, double(arg)); 
			}
#endif

	// STRING - DATATIME - JSON
		else if (value_type == Variant::STRING){

			String gdt_data = arg;
			sql::SQLString sql_data = gdt_data.utf8().get_data();
			std::string std_string = gdt_data.utf8().get_data();

			if ( is_mysql_time( gdt_data )) {
				prep_stmt->setDateTime(index+1, sql_data );
			} else if ( is_json( gdt_data ) ) {
				*blob << std_string;
				prep_stmt->setBlob(index+1, blob );		
			} else {
				prep_stmt->setString(index+1, sql_data );
			}
		}

	// BINARY
		else if (value_type == Variant::POOL_BYTE_ARRAY){	
			PoolByteArray _data = arg;
			PoolVector<uint8_t>::Read in_buffer = _data.read();
			for ( int y = 0; y < _data.size(); y++){
				*blob << (char)in_buffer[y];
			}
			prep_stmt->setBlob(index+1, blob);
		}
		
	// VARIANT
		else{
			_Marshalls *marshalls = memnew(_Marshalls);
			String buff = marshalls->variant_to_base64(arg, p_full_objects);
			*blob << buff.utf8().get_data();
			prep_stmt->setBlob(index+1, blob);
		}

}
	


//---------------------------------------------------------------------------------------------


/*
https://dev.mysql.com/doc/refman/8.0/en/blob.html  NOTE: Afeta o desempenho
https://mariadb.com/kb/en/json-data-type/
https://zh.wikibooks.org/zh-hk/MySQL_Connector/C%2B%2B
*/



//--------CONNECTION-------------------


Dictionary MySQL::get_metadata(){
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


MySQL::ConnectionStatus MySQL::get_connection_status(){
	MySQL::ConnectionStatus ret;	
	if (conn.get()) {// != NULL
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

//TODO : set reconnection

MySQL::MySQLException MySQL::start_connection(){
	sqlError.clear();
	try {
		driver = sql::mysql::get_mysql_driver_instance();
		conn.reset(driver->connect(connection_properties));
	}

	catch (sql::SQLException &e) {
		print_SQLException(e);
	} 
						
#ifdef DEBUG_ENABLED
	catch (std::runtime_error &e) {
		print_runtime_error(e);
	}
#endif

	sqlError.clear();
	return sqlError;
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


/*
//--------TRANSACTION-------------------
bool MySQL::getAutoCommit(){return conn->getAutoCommit();}
void MySQL::setAutoCommit(bool autoCommit){}
void MySQL::rollback(){}
void MySQL::rollback(Savepoint * savepoint){}
void MySQL::commit(){}
void MySQL::setTransactionIsolation(enum_transaction_isolation level)
void MySQL::releaseSavepoint(Savepoint * savepoint){}
Savepoint * setSavepoint(const sql::SQLString& name){}
Savepoint * setSavepoint(){}
*/


//--------PROPERTIES-------------------

void MySQL::set_properties_set(Dictionary p_properties){

	for ( int i = 0; i < p_properties.size(); i++){
		set_property( p_properties.keys()[i], p_properties.values()[i] );
	}
}


Dictionary MySQL::get_properties_set(Array p_properties){

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
	
	
	if (value_type == "string" ) { connection_properties[property] = (sql::SQLString)value; }
	else if (value_type == "void") { connection_properties[property] = (std::string)value; }
	else if (value_type == "bool") { connection_properties[property] = (bool)p_value; }
	else if (value_type == "int" ) { connection_properties[property] = (int)p_value; }

	
//	TODO suporte para MAP(dictionary) 
//	else if (value_type == "map" ) {}


	else{
		//Precisa apenas imprimir a mesagem de erro (no debug?)
		ERR_FAIL_MSG("Invalid data type. For more information visit: https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-connect-options.html");
	}

	if (conn != NULL) {
		if (property == "schema"){
			conn->setSchema( value );
		} else {
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

	if (value_type == "string" || value_type == "void") { return SQLstr2GDstr( *connection_properties[ property ].get<sql::SQLString>() ); }

	else if (value_type == "int" ) {return *connection_properties[ property ].get< int >();}

	else if (value_type == "bool") {return *connection_properties[ property ].get< bool >();}


	//	TODO suporte para MAP(dictionary) 
	//	else if (value_type == "map" ) {FIXME}

	/*
	else{
		// Erro precisa retornar uma variante vazia e imprimir a mensagem de erro no debug
		ERR_FAIL_MSG("Invalid data type. For more information visit: https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-connect-options.html");
	}
	*/

	return Variant();
	
}



//--------HELPERS-------------------
String MySQL::SQLstr2GDstr( sql::SQLString &p_string ) {
	const char * _str_ = p_string.c_str();
	String str = String::utf8( (char *) _str_ );
	return str;
}


bool MySQL::is_json( Variant p_arg ) {
	String errs; int errl; Variant r_ret;
	Error err = JSON::parse(p_arg, r_ret, errs, errl);
	return (err == OK); 
}


std::string	MySQL::get_prop_type( std::string p_prop ) { 
	for (int i = 0; i < (int)sizeof(string_properties)/32; i++){ if (string_properties[i] == p_prop) {return "string"; } }
	for (int i = 0; i < (int)sizeof(sqlmap_properties)/32; i++){ if (sqlmap_properties[i] == p_prop) {return "map"; } }
	for (int i = 0; i < (int)sizeof(bool_properties)/32; i++){ if (bool_properties[i] == p_prop) {return "bool"; } }
	for (int i = 0; i < (int)sizeof(void_properties)/32; i++){ if (void_properties[i] == p_prop) {return "void"; } }
	for (int i = 0; i < (int)sizeof(int_properties)/32; i++){ if (int_properties[i] == p_prop) {return "int"; } }
	return "invalid";
}


bool MySQL::is_mysql_time(String time) {
	if (get_time_format( time ) == sql::DataType::UNKNOWN){
		return false;
	} else {
		return true;
	}
}


int MySQL::get_time_format(String time) {

	std::string s_time = time.utf8().get_data();
	int len = time.length();

	if (s_time.find_first_not_of( "0123456789:- " ) == std::string::npos) {
		// - 0000
		if ( len == 4 ) {
			if (s_time.find_first_not_of( "0123456789" ) == std::string::npos) {
				return sql::DataType::YEAR;
			}
		}
		// - 00:00:00
		else if ( len == 8 ) {
			if ( time[2] ==  ':' && time[5] == ':' ) {
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 3 && String(arr_time[2]).length() == 2) {
					return sql::DataType::TIME ;
				}
			}
		}
		// - 0000-00-00
		else if ( len == 10 ) {
			if ( time[4] ==  '-' && time[7] == '-') {
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 3 && String(arr_time[2]).length() == 2) {
					return sql::DataType::DATE;
				}
			}
		}
		// - 0000-00-00 00:00:00
		else if ( len == 19 ) {
			if ( time[4] ==  '-' && time[7] == '-' && time[13] ==  ':' && time[16] == ':' && time[10] == ' ' ) {
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 6 && String(arr_time[2]).length() == 2) {
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
		} else {
			datando.push_back( atoi(token) ); //--As Data (int)
		}
		token = strtok( NULL, seps );
	}
	return datando;
}



//--------ERRORS-------------------

void MySQL::print_SQLException(sql::SQLException &e) {  //FIXME 
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
	//sqlError["SQLState"] = _error;
	
	// Cast error signal && return (typedef Dictionary MySQLException)

#ifdef TOOLS_ENABLED
	print_line("# EXCEPTION Caught ˇ");
	print_line("# ERR: SQLException in: "+String(file)+" in function: "+String(func)+"() on line "+String(line));
	print_line("# ERR: " + String(e.what()));
	print_line(" (MySQL error code: " + String( errCode)+ ")" );
	//print_line("SQLState: " + _error);
#endif

}

//--------
void MySQL::print_runtime_error(std::runtime_error &e) {
	std::cout << "ERROR: runtime_error in " << __FILE__;
	std::cout << " (" << __func__ << ") on line " << __LINE__ << std::endl;
	std::cout << "ERROR: " << e.what() << std::endl;
}



//--------GODOT STUFF-------------------

void MySQL::_bind_methods() {
/*
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "can_reconnect"), "set_reconnect", "can_reconnect");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "encode objects"), "set_reconnect", "can_reconnect");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "accept multi statement"), "set_reconnect", "can_reconnect");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "encode object"), "set_reconnect", "can_reconnect");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use json"), "set_reconnect", "can_reconnect");
*/

	ClassDB::bind_method(D_METHOD("test", "argument"),&MySQL::test);



	//--- Connection Managers
	ClassDB::bind_method(D_METHOD("set_credentials", "HostName", "UserName", "Password"),&MySQL::set_credentials);
	ClassDB::bind_method(D_METHOD("get_connection_status"),&MySQL::get_connection_status);
	ClassDB::bind_method(D_METHOD("start_connection"),&MySQL::start_connection);
	ClassDB::bind_method(D_METHOD("stop_connection"),&MySQL::stop_connection);
	ClassDB::bind_method(D_METHOD("get_metadata"),&MySQL::get_metadata);
	

	//--- Properties Managers
	ClassDB::bind_method(D_METHOD("set_property", "property", "p_value"),&MySQL::set_property);
	ClassDB::bind_method(D_METHOD("get_property", "property"),&MySQL::get_property);
	
	
	ClassDB::bind_method(D_METHOD("set_properties_set", "properties"),&MySQL::set_properties_set);
	ClassDB::bind_method(D_METHOD("get_properties_set", "properties"),&MySQL::get_properties_set);


	//--- Execute/Update/Query
	ClassDB::bind_method(D_METHOD("execute", "sql_statement"),&MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "sql_statement", "Values"),&MySQL::execute_prepared);	

	ClassDB::bind_method(D_METHOD("update", "sql_statement"),&MySQL::update);
	ClassDB::bind_method(D_METHOD("update_prepared", "sql_statement", "Values"),&MySQL::update_prepared);
	
	ClassDB::bind_method(D_METHOD("query", "sql_statement", "DataFormat", "return_string", "meta"),&MySQL::query, DEFVAL(DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()) );
	ClassDB::bind_method(D_METHOD("query_prepared", "sql_statement", "Values", "DataFormat", "return_string", "meta"), &MySQL::query_prepared, 
	DEFVAL(Array()), DEFVAL(DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()));

	
	//--- Transaction

	ClassDB::bind_method(D_METHOD("setAutoCommit", "bool"),&MySQL::setAutoCommit);
	ClassDB::bind_method(D_METHOD("getAutoCommit"),&MySQL::getAutoCommit);
	ClassDB::bind_method(D_METHOD("commit"),&MySQL::commit);
	ClassDB::bind_method(D_METHOD("rollback", "sql_statement"),&MySQL::rollback);


	//---

	ClassDB::bind_method(D_METHOD("getTransactionIsolation"),&MySQL::getTransactionIsolation);
	ClassDB::bind_method(D_METHOD("setTransactionIsolation", "level"),&MySQL::setTransactionIsolation);

	
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

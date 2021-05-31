//*  MySQL.cpp */


#include "MySQL.h"
#include "core/io/json.h"


#include <iostream>
#include <istream>
#include <streambuf>
#include <string>
#include <sstream>

/*
std::istream MySQL::stream_in(PoolByteArray p_args){

  	PoolVector<uint8_t>::Read r = p_args.read();
	int r_size = p_args.size();
	
	char buffer[r_size];
			
	for (int i = 0; i < r_size; i++){
		buffer[i] = (char)r[i];
	} 
 
	membuf sbuf(buffer, buffer + sizeof(buffer));
	std::istream in(&sbuf);

	//std::istream * blob;
	//&in
	return in;
}
*/


Variant MySQL::test(PoolByteArray p_args){





	return Variant();
}



//--------FETCH QUERY-------------------
Array MySQL::fetch_query(String p_sqlquery, Template data_model,  bool return_string){
	return query(p_sqlquery, Array(), data_model, return_string, false);
}


Array MySQL::fetch_prepared_query(String p_sqlquery, Array prep_values, Template data_model,  bool return_string){
	return query(p_sqlquery, prep_values, data_model, return_string, true);
}


Array MySQL::query(String p_sqlquery, Array prep_values, Template data_model,  bool return_string, bool _prep){

	// if conn == null: ERR_FAIL -> Start your connection 


	std::unique_ptr <sql::Statement> stmt;
	std::shared_ptr <sql::PreparedStatement> prep_stmt;
	std::unique_ptr <sql::ResultSet> res;
	//std::unique_ptr <sql::ResultSetMetaData> res_meta;
	sql::ResultSetMetaData *res_meta;
	
	
	sql::SQLString query = GDstr2SQLstr( p_sqlquery );
	
	Array ret;

	try {
	
		// Prep statement
		if (  _prep ) {	
			prep_stmt.reset(conn->prepareStatement(query));
			set_datatype( prep_stmt,  prep_values);
			res.reset( prep_stmt->executeQuery());	
			res_meta = res->getMetaData();
		}

		// Non prep statement
		else {
			stmt.reset(conn->createStatement());
			res.reset( stmt->executeQuery(query));
			res_meta = res->getMetaData();
		}



		//--------
		Array columnname;
		for (uint8_t i = 0; i < res_meta->getColumnCount(); i++) {
			sql::SQLString _stri = res_meta->getColumnName(i);
			columnname.push_back( SQLstr2GDstrS( _stri ));
		}
		if ( data_model == COLUMNS_NAMES){
			return columnname;
		}


		//--------
		Array typenames;
		for (uint8_t i = 0; i < res_meta->getColumnCount(); i++) {
			sql::SQLString _stri = res_meta->getColumnTypeName(i);
			typenames.push_back( SQLstr2GDstrS( _stri ));
		}
		if ( data_model == COLUMNS_TYPES){
			return typenames;
		}



		//-------- NAMED && TYPED ARRAYS
		if ( data_model == NAMED_ARRAY or data_model == FULL_ARRAY){
			ret.push_back(columnname);
		}
		if ( data_model == TYPED_ARRAY or data_model == FULL_ARRAY){
			ret.push_back(typenames);
		}
			

		//-------- METADATA
		/*if ( data_model == METADATA ){
			return
		}
		*/

		//-------- INFO
		/*if ( data_model == INFO ){
			return
		}*/

			
/*
setBlob std::istream * blob
setBoolean bool value
setDateTime const sql::SQLString& value
setDouble double value
setInt64 int64_t value
setNull int sqlType
setString const sql::SQLString& value
setJson ??????	
*/

		
		//--Start arrays or dictionary confection

			
		Array rows;


		//--------FETCH MAIN ARRAY		
		while (res->next()) {
			
			Array line;
			Dictionary row;
				
			for (unsigned int i = 0; i < res_meta->getColumnCount(); i++) {
				sql::SQLString _col_name = res_meta->getColumnName(i);
				String column_name = SQLstr2GDstrS( _col_name );
				int d_type = res_meta->getColumnType(i);


				//--------RETURN STRING
				if ( return_string ) {
					sql::SQLString _val = res->getString(i);
					String _value = SQLstr2GDstrS( _val );
					
					if ( data_model == DICTIONARY ){
						row[ column_name ] = _value; 			
					}
					else{
						line.push_back( _value );					
					}
				}



				//--------RETURN DATA WITH IT'S SELF TYPES
				else{
					
					//	NULL
					if ( res->isNull(i) ){
						if ( data_model == DICTIONARY ){
							row[ column_name ] = NIL; 
						}
						else{
							line.push_back( NIL ); 
						}
					}


					//	BOOL
					else if ( d_type == sql::DataType::BIT ){
						if  ( data_model == DICTIONARY ){
							row[ column_name ] = res->getBoolean(i); 
						}
						else {
							line.push_back( res->getBoolean(i) ); 
						}
					}
											
					//	INT
					else if ( d_type == sql::DataType::ENUM or d_type == sql::DataType::TINYINT or d_type == sql::DataType::SMALLINT or d_type == sql::DataType::MEDIUMINT) {
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = res->getInt(i); 
						}
						else { 
							line.push_back( res->getInt(i) ); 
						}
					}


					//	BIG INT
					else if ( d_type == sql::DataType::INTEGER or d_type == sql::DataType::BIGINT ) {
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = res->getInt64(i); 
						}
						else { 
							line.push_back( res->getInt64(i) ); 
						}
					}


					//	FLOAT 
					//FIXME use double on GODOT 4 and float on GODOT 3?
					else if ( d_type == sql::DataType::REAL or d_type == sql::DataType::DOUBLE or d_type == sql::DataType::DECIMAL or d_type == sql::DataType::NUMERIC ) {
						double my_float = res->getDouble(i);
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = my_float; 
						}
						else { 
						line.push_back( my_float ); 
						}							
					}
			
						
					//	TIME
					// It should return time information as a dictionary but if the sequence of the data be modified (using TIME_FORMAT or DATE_FORMAT for exemple), 
					// it will return the dictionary fields with wrong names. So I prefer return the data as an array.	
					else if ( d_type == sql::DataType::DATE or d_type == sql::DataType::TIME or d_type == sql::DataType::TIMESTAMP or d_type == sql::DataType::YEAR ) {
						sql::SQLString _stri = res->getString(i);
						Array time = format_time( SQLstr2GDstrS( _stri ) , false );
					
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = time;
						}
						else{
							line.push_back( time );	
						}
					}
					
					
					
					//	STRING
					else if (d_type == sql::DataType::CHAR or d_type == sql::DataType::VARCHAR or d_type == sql::DataType::LONGVARCHAR ){
						sql::SQLString _stri = res->getString(i);
						String s = SQLstr2GDstrS( _stri );
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = s; 
						}
						else { 
							line.push_back( s ); 
						}	
					}				
					
					
					//  BINARY - PoolByteArray
					else if (d_type == sql::DataType::BINARY or d_type == sql::DataType::VARBINARY or d_type == sql::DataType::LONGVARBINARY ){

						boost::scoped_ptr<std::istream> inStr(res->getBlob(i));
						unsigned int blob_size = inStr->tellg();					
						char _buffer[ blob_size ];
						
						while (!inStr->eof()){
							inStr->read(_buffer, blob_size);
						}
						
						PoolByteArray bin_obj;
						for (unsigned int t = 0; t < sizeof(_buffer); i++) {
							bin_obj.push_back((uint8_t)_buffer[t]);
						}


						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = bin_obj; 
						}
						else { 
							line.push_back( bin_obj ); 
						}			
						
						
						
					}
					
					/*
					else if ( d_type == sql::DataType::JSON ){
					
						std::istream *blobData = res->getBlob(i);
						unsigned int BlobSize = blobData->tellg();
						std::istreambuf_iterator<char> isb = std::istreambuf_iterator<char>(*blobData);
						sql::SQLString blobString = std::string(isb, std::istreambuf_iterator<char>());
						blobData->seekg(0, std::ios::end);
						line.push_back( SQLstr2GDstrS( blobString ) );	
					}
					*/
						
						
						
						// BLOB FIXME TODO FIXME TODO FIXME TODO FIXME TODO FIXME TODO FIXME TODO
						/*
						else if ( d_type == sql::DataType::BINARY or d_type == sql::DataType::VARBINARY or d_type == sql::DataType::LONGVARBINARY ){
						
							std::unique_ptr<std::istream> blob_data(rs->getBlob(i));
							
							for(int i = 0; !inStr->eof(); i++){
							
							
							}
							
							
							//https://stackoverflow.com/questions/21157244/how-to-get-blob-data-from-mysql-in-c
							
							
							
						}
						*/
										
					}
				
				}
				ret.push_back(line);
				
				
				
			
			}
		
	
	
	}
	
	catch (sql::SQLException &e) { print_SQLException(e); } 
	catch (std::runtime_error &e) { print_runtime_error(e); }

	return Array();

}






/*
    UNKNOWN = 0,


blob
    BINARY,
    VARBINARY,
    LONGVARBINARY,
    
    
    
    GEOMETRY,



    SET,
    SQLNULL,
    JSON
    
*/













int MySQL::execute( String p_sqlquery ){
	return _execute( p_sqlquery, Array(), false );
}


int MySQL::execute_prepared( String p_sqlquery, Array p_values){
	return _execute( p_sqlquery, p_values, true );
}


int MySQL::_execute( String p_sqlquery, Array p_values, bool _prep){

	sql::SQLString query = GDstr2SQLstr( p_sqlquery );
	int afectedrows;

	try {
		if (_prep){
			std::shared_ptr <sql::PreparedStatement> prep_stmt;
			prep_stmt.reset(conn->prepareStatement(query));	
			set_datatype( prep_stmt,  p_values );
			afectedrows = prep_stmt->executeUpdate();
		}
		
		else{
			std::unique_ptr <sql::Statement> stmt;
			stmt.reset(conn->createStatement());
			afectedrows = stmt->executeUpdate(query);
		}

	}
		
	catch (sql::SQLException &e) { print_SQLException(e); } 
	catch (std::runtime_error &e) { print_runtime_error(e); }

	return afectedrows;
}





/*
setBlob std::istream * blob
setBoolean bool value
setDateTime const sql::SQLString& value
setDouble double value
setInt64 int64_t value
setNull int sqlType
setString const sql::SQLString& value
setJson ??????	
*/


/*
NULL -> setNull
BOOL -> setBoolean
INTEGER -> setInt64
FLOAT -> setDouble

STRING -> setString
	JSON?
	DATATIME -> setDateTime
	
	
POOL_BYTE_ARRAY -> setBlob

VARIANT GERAL/OBJECT -> setBlob
	To binary


	

*/

void MySQL::set_datatype(std::shared_ptr <sql::PreparedStatement> prep_stmt, Array prep_values ){

	//for (int i = 1; i <= 5; ++i) 
	for (int i = 0; i < prep_values.size() ; i++){
	
		//NULL
		if (prep_values[i].get_type() == Variant::Variant::NIL){
			prep_stmt->setNull(i+1, sql::DataType::SQLNULL);
		}

		//BOOL
		else if (prep_values[i].get_type() == Variant::Variant::BOOL){
			prep_stmt->setBoolean(i+1, bool(prep_values[i]));
		}
		
		//INT
		else if (prep_values[i].get_type() == Variant::Variant::INT){
			prep_stmt->setInt64(i+1, int64_t(prep_values[i]));
		}

		// FLOAT
#ifdef GODOT4		
		else if (prep_values[i].get_type() == Variant::Variant::FLOAT){
			prep_stmt->setDouble(i+1, double(prep_values[i]));
		}
#else
		else if (prep_values[i].get_type() == Variant::Variant::REAL){
			prep_stmt->setDouble(i+1, double(prep_values[i]));
		}
#endif




//	https://gist.github.com/FlorianWolters/be839c84991a789df1c6
//	https://stackoverflow.com/questions/45722747/how-can-i-create-a-istream-from-a-uint8-t-vector
//	https://stackoverflow.com/questions/1121142/set-binary-data-using-setblob-in-mysql-connector-c-causes-crash
//	https://dev.mysql.com/doc/refman/8.0/en/blob.html  NOTE: Afeta o desempenho
//	https://mariadb.com/kb/en/json-data-type/

	//blob = PoolArray
	//TEXT = Variant or JSON or String



		//BINARY
		else if (prep_values[i].get_type() == Variant::Variant::POOL_BYTE_ARRAY){	
		
			print_line("POOL_BYTE_ARRAY");
			
			PoolByteArray data = prep_values[i];


			int len = data.size();
			char _buffer[len];
			
			
			for (int t = 0; t < len; t++) {
				_buffer[t] = (char)data[t];
				//_buffer[t] = static_cast<char>(data[t]);
				//std::cout << "buffer: " << buffer[t] << std::endl;
			}	


			membuf sbuf(buffer, buffer + sizeof(buffer));
			std::istream in(&sbuf);
			prep_stmt->setBlob(i+1, &in);


		//	std::stringstream blob_input_stream;
		//	blob_input_stream << _buffer;
		//	prep_stmt->setBlob(i+1, &blob_input_stream);


			
		}







		//STRING
		else if (prep_values[i].get_type() == Variant::Variant::STRING){
			
		
			String stri = String(prep_values[i]);
			sql::SQLString caracteres = stri.utf8().get_data();
			prep_stmt->setString(i+1, caracteres );
			
			
			
			
			// Date and Time types
			//if ( is_mysql_time( str_val )) { 
			//	prep_stmt->setDateTime(i, sql_str );
		//	}
			
		// JSON
			/*
			else if ( is_json( str_val ) ) {
				std::string s = str_val.utf8().get_data();
				std::istringstream iss(s);
				std::istream& stream = iss;
				iss << id;
				prep_stmt->setBlob(i, &stream );
			}
			*/
			//TEXT & CHAR
			//else{
		//		prep_stmt->setString(i, sql_str );	
		//	}
			
			
		}




	}


	
}











//--------CONNECTION-------------------

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










//--------PROPERTIES-------------------
/*
void MySQL::set_properties_kit(Dictionary p_properties){

	int a;
	int b;
}
*/

Dictionary MySQL::get_properties_kit(Array p_properties){

	return Dictionary();
}



	
	
void MySQL::set_credentials( String p_host, String p_user, String p_pass ) {
	connection_properties["hostName"] = GDstr2SQLstr(p_host);
	connection_properties["userName"] = GDstr2SQLstr(p_user);
	connection_properties["password"] = GDstr2SQLstr(p_pass);
}	


void MySQL::set_property(String p_property, Variant p_value){

	sql::SQLString property = GDstr2SQLstr( p_property );
	std::string value_type = get_prop_type( property );
	String _val = p_value;
	sql::SQLString value = GDstr2SQLstr( _val );
	
	
	if (value_type == "string" ) { connection_properties[property] = (sql::SQLString)value; }
	
	else if (value_type == "void") { connection_properties[property] =  (std::string)value; }
	
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
		}else{
			conn->setClientOption(property, value );
		}
	}

}



Variant MySQL::get_property(String p_property){

	sql::SQLString property = GDstr2SQLstr( p_property );
	std::string value_type = get_prop_type( property );


	if (property == "schema"){
		 sql::SQLString _p_s = conn->getSchema();
		return SQLstr2GDstrS( _p_s ); 
	}


	if (value_type == "string" or value_type == "void") { 
		const sql::SQLString * p_s;
		p_s = connection_properties[ property ].get<sql::SQLString>();
		return SQLstr2GDstrP( p_s ); 
	}

	else if (value_type == "int" ) { 
		const int * p_i;
		p_i = connection_properties[ property ].get< int >(); 	
		return *p_i;
	}

	else if (value_type == "bool") {
		const bool * p_b;
		p_b = connection_properties[ property ].get< bool >(); 
		return *p_b;
	}


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


String MySQL::SQLstr2GDstrP( const sql::SQLString *p_str ) {
	const char * _str_ = p_str->c_str();
	String str = String::utf8( (char *) _str_ );
	return str;
}



String MySQL::SQLstr2GDstrS( sql::SQLString &p_string ) {
	const char * _str_ = p_string.c_str();
	String str = String::utf8( (char *) _str_ );
	return str;
}



sql::SQLString MySQL::GDstr2SQLstr(String &p_str){
	return p_str.utf8().get_data();
}



bool MySQL::is_json( Variant p_arg ) {
	String errs;
	int errl;
	Variant r_ret;
	Error err = JSON::parse(p_arg, r_ret, errs, errl);
	return (err == OK); 
}


std::string	MySQL::get_prop_type( std::string p_prop ) { 

	for (int i = 0; i < (int)sizeof(string_properties)/32; i++){ 
		if (string_properties[i] == p_prop) {
			return "string"; 
		} 
	}
	for (int i = 0; i < (int)sizeof(bool_properties)/32; i++){ 
		if (bool_properties[i] == p_prop) {
			return "bool"; 
		} 
	}
	for (int i = 0; i < (int)sizeof(int_properties)/32; i++){ 
		if (int_properties[i] == p_prop) {
			return "int"; 
		} 
	}
	for (int i = 0; i < (int)sizeof(sqlmap_properties)/32; i++){ 
		if (sqlmap_properties[i] == p_prop) {
			return "map"; 
		} 
	}
	for (int i = 0; i < (int)sizeof(void_properties)/32; i++){ 
		if (void_properties[i] == p_prop) {
			return "void"; 
		} 
	}	

	return "invalid";
}







bool MySQL::is_mysql_time(String time) {
	if (get_time_format( time ) == sql::DataType::UNKNOWN){
		return false;
	} 
	else {
		return true;
	}
}



int MySQL::get_time_format(String time) {

	std::string s_time = time.utf8().get_data();    ///Impo
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
	std::string strss =	GDstr2SQLstr(str);
	char seps[] = ": -";
	char *token;
	token = strtok( &strss[0], seps );
	
	while( token != NULL ) {
		if (return_string) {
			datando.push_back( String(token) );   //--As String
		} 
		else {
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

	sqlError.clear();
	sqlError["FILE"] = String( file );
	sqlError["FUNCTION"] = String( func );
	sqlError["LINE"] = String( line );
	sqlError["ERROR"] = String( e.what() );
	sqlError["MySQL error code"] = String( errCode );
	//sqlError["SQLState"] = SQLstr2GDstr( e.getSQLState() );

#ifdef TOOLS_ENABLED
	print_line("# EXCEPTION Caught ˇ");
	print_line("# ERR: SQLException in: "+String(file)+" in function: "+String(func)+"() on line "+String(line));
	print_line("# ERR: " + String(e.what()));
	print_line(" (MySQL error code: " + String( errCode)+ ")" );
//	print_line("SQLState: "+sql2String(*e.getSQLState()));
#endif

}

//--------
void MySQL::print_runtime_error(std::runtime_error &e) {
	std::cout << "ERROR: runtime_error in " << __FILE__;
	std::cout << " (" << __func__ << ") on line " << __LINE__ << std::endl;
	std::cout << "ERROR: " << e.what() << std::endl;
}


//TODO: REMOVE p
//--------GODOT STUFF-------------------

void MySQL::_bind_methods() {

	//ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reconnect"), "set_reconnect", "can_reconnect");
	
/*		ADD_PROPERTY ( metadata template)
		{ 
		}
*/	
	
	ClassDB::bind_method(D_METHOD("test", "argumentos"),&MySQL::test);
	//ClassDB::bind_method(D_METHOD("stream_in", "PoolArray"),&MySQL::test);


	//--- Connection Managers
	ClassDB::bind_method(D_METHOD("set_credentials", "HostName", "UserName", "Password"),&MySQL::set_credentials);
	ClassDB::bind_method(D_METHOD("get_connection_status"),&MySQL::get_connection_status);
	ClassDB::bind_method(D_METHOD("start_connection"),&MySQL::start_connection);
	ClassDB::bind_method(D_METHOD("stop_connection"),&MySQL::stop_connection);
	

	//--- Properties Managers
	ClassDB::bind_method(D_METHOD("set_property", "property", "p_value"),&MySQL::set_property);
	ClassDB::bind_method(D_METHOD("get_property", "property"),&MySQL::get_property);
	//ClassDB::bind_method(D_METHOD("set_properties_kit", "properties"),&MySQL::set_properties_kit);
	ClassDB::bind_method(D_METHOD("get_properties_kit", "properties"),&MySQL::get_properties_kit);


	//--- Execute/Query
	ClassDB::bind_method(D_METHOD("execute", "sql_statement"),&MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "sql_statement", "Values"),&MySQL::execute_prepared);
	ClassDB::bind_method(D_METHOD("fetch_query", "sql_statement", "template", "return_string"),&MySQL::fetch_query);
	ClassDB::bind_method(D_METHOD("fetch_prepared_query", "sql_statement", "Values", "template", "return_string"),&MySQL::fetch_prepared_query);
		


	BIND_ENUM_CONSTANT(NO_CONNECTION);
	BIND_ENUM_CONSTANT(CLOSED);
	BIND_ENUM_CONSTANT(CONNECTED);
	BIND_ENUM_CONSTANT(DISCONNECTED);

	BIND_ENUM_CONSTANT(COLUMNS_NAMES);
	BIND_ENUM_CONSTANT(COLUMNS_TYPES);
	BIND_ENUM_CONSTANT(SIMPLE_ARRAY);
	BIND_ENUM_CONSTANT(NAMED_ARRAY);
	BIND_ENUM_CONSTANT(TYPED_ARRAY);
	BIND_ENUM_CONSTANT(FULL_ARRAY);	
	BIND_ENUM_CONSTANT(DICTIONARY);
	BIND_ENUM_CONSTANT(METADATA);	
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


Variant MySQL::set_properties(Dictionary p_options){
}
Dictionary MySQL::get_properties(Array p_options = []){
}



Array MySQL::fetch_query(String p_sqlquery, int return_like = DICTIONARY,  bool return_string = false){
}


Array MySQL::fetch_prepared_query(String p_sqlquery, Array p_values, int return_what = DICTIONARY,  bool return_string = false){
}


Variant MySQL::transaction( [] ){
}


Variant MySQL::transaction_prepared( {} ){
}

	Ref(const Variant &p_variant) {
		reference = nullptr;
		Object *refb = T::___get_from_variant(p_variant);
		if (refb == nullptr) {
			unref();
			return;
		}
		Ref r;
		r.reference = Object::cast_to<T>(refb);
		ref(r);
		r.reference = nullptr;
	}


	inline static Ref<T> __internal_constructor(Object *obj) {
		Ref<T> r;
		r.reference = (T *)obj;
		return r;
	}	
	
	*/

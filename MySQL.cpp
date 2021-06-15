//*  MySQL.cpp */


#include "MySQL.h"
#include "core/io/json.h"
#include <vector>


bool p_full_objects = true; //TODO: colocar o p_full_objects como propriedade ao lado do reconnect
bool use_json = false;

String MySQL::test(String arg){

	sql::SQLString WHITE = "INSERT INTO json (jcol) VALUES (?)";
	sql::SQLString READ = "SELECT jcol FROM json";
	

	std::shared_ptr <sql::PreparedStatement> prep_stmt;
	std::unique_ptr <sql::Statement> stmt;
	std::unique_ptr <sql::ResultSet> res;
	
	std::istringstream blob_input_stream;	
	
	blob_input_stream.str( arg.utf8().get_data() );
	
	try {
		// WHITE CHAR - READY FOR USE
		prep_stmt.reset(conn->prepareStatement( WHITE ));	
		prep_stmt->setBlob(1, &blob_input_stream);  ///----------

		int afectedrows = prep_stmt->execute();

		std::cout << "afectedrows: " << afectedrows << std::endl << std::endl;
	}

	catch (sql::SQLException &e) { print_SQLException(e); } 
	catch (std::runtime_error &e) { print_runtime_error(e); }
	
	String ret;
	
	try {
			
		// READ CHAR
		stmt.reset(conn->createStatement());
		res.reset( stmt->executeQuery( READ ));
			
		while (res->next()) {
		
	//	sql::SQLString _val = res->getString(1);
	//	ret = SQLstr2GDstrS(_val);
		
		
		
/*
			std::unique_ptr< std::istream > buffer(res->getBlob(1));

		    buffer->seekg (0, buffer->end);
   			int length = buffer->tellg();
   			buffer->seekg (0, buffer->beg);

			char str[length];
			buffer->get( str , length+1);	
			ret = SQLstr2GDstrS((sql::SQLString &)str);
*/		
		}
	}
	
	catch (sql::SQLException &e) { print_SQLException(e); } 
	catch (std::runtime_error &e) { print_runtime_error(e); }

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

	sql::SQLString query = GDstr2SQLstr( p_sqlquery );
	Array ret;
	

	std::unique_ptr <sql::Statement> stmt;
	std::shared_ptr <sql::PreparedStatement> p_stmt;
	std::unique_ptr <sql::ResultSet> res;
	sql::ResultSetMetaData *res_meta;
	std::stringstream blob;
	

	try {		

		//DatabaseMetaData *dbcon_meta = con->getMetaData();

		if (  _prep ) {	
			// Prep statement
			p_stmt.reset(conn->prepareStatement(query));
	//		set_datatype( p_stmt,  p_values, &blob);
			res.reset( p_stmt->executeQuery());	
			res_meta = res->getMetaData();
		}
		
		else { 
			// Non prep statement
			stmt.reset(conn->createStatement());
			res.reset( stmt->executeQuery(query));
			res_meta = res->getMetaData();
		}

		//--------------------------------------
		Array columnname;
		Array columntypes;
		Array columninfo;
		Array columnmeta;


		for (int m = 0; m < meta_col.size(); m++){

		//----------------------------------------------------------------------
			if ( meta_col[m] == COLUMNS_NAMES && columnname.empty()){
				for (uint8_t i = 1; i <= res_meta->getColumnCount(); i++) {
					sql::SQLString _stri = res_meta->getColumnName(i);
					columnname.push_back( SQLstr2GDstrS( _stri ));
				}
				ret.push_back(columnname);
			}

		//----------------------------------------------------------------------
			if (meta_col[m] == COLUMNS_TYPES && columntypes.empty()){
				for (uint8_t i = 1; i <= res_meta->getColumnCount(); i++) {
					sql::SQLString _stri = res_meta->getColumnTypeName(i);
					columntypes.push_back( SQLstr2GDstrS( _stri ));
				}
				ret.push_back(columntypes);
			}

		//----------------------------------------------------------------------			
			if (meta_col[m] == METADATA && columninfo.empty()){
			//TODO
				ret.push_back(columninfo);
			}


		//----------------------------------------------------------------------			
			if (meta_col[m] == INFO && columnmeta.empty()){
			//TODO
				ret.push_back(columnmeta);
			}


		}
					

		//--------FETCH DATA
		while (res->next()) {
			
			Array line;
			Dictionary row;
				
			for ( unsigned int i = 1; i <= res_meta->getColumnCount(); i++) { 
				sql::SQLString _col_name = res_meta->getColumnName(i);
				String column_name = SQLstr2GDstrS( _col_name );
				int d_type = res_meta->getColumnType(i);
				
				std::cout << "D_TYPE: " << d_type << std::endl;


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
					std::cout << "NULL" << std::endl;
						if ( data_model == DICTIONARY ){
							row[ column_name ] = Variant();
						}
						else{
							line.push_back( Variant() ); 
						}
					}

					//	BOOL
					else if ( d_type == sql::DataType::BIT ){
					std::cout << "BOOL" << std::endl;
						if  ( data_model == DICTIONARY ){
							row[ column_name ] = res->getBoolean(i); 
						}
						else {
							line.push_back( res->getBoolean(i) ); 
						}
					}
											
					//	INT
					else if ( d_type == sql::DataType::ENUM || d_type == sql::DataType::TINYINT || d_type == sql::DataType::SMALLINT || d_type == sql::DataType::MEDIUMINT) {
					std::cout << "INT" << std::endl;
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = res->getInt(i); 
						}
						else { 
							line.push_back( res->getInt(i) ); 
						}
					}


					//	BIG INT
					else if ( d_type == sql::DataType::INTEGER || d_type == sql::DataType::BIGINT ) {
					std::cout << "BIG INT" << std::endl;
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = res->getInt64(i); 
						}
						else { 
							line.push_back( res->getInt64(i) ); 
						}
					}


					//	FLOAT 
					else if ( d_type == sql::DataType::REAL || d_type == sql::DataType::DOUBLE || d_type == sql::DataType::DECIMAL || d_type == sql::DataType::NUMERIC ) {
					std::cout << "FLOAT" << std::endl;
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
					else if ( d_type == sql::DataType::DATE || d_type == sql::DataType::TIME || d_type == sql::DataType::TIMESTAMP || d_type == sql::DataType::YEAR ) {
					std::cout << "TIME" << std::endl;
						sql::SQLString _stri = res->getString(i);
						Array time = format_time( SQLstr2GDstrS( _stri ) , false );
					
						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = time;
						}
						else{
							line.push_back( time );	
						}
					}


					//	STRING - VARIANT - JSON
					else if (d_type == sql::DataType::CHAR || d_type == sql::DataType::VARCHAR || d_type == sql::DataType::LONGVARCHAR ){
					
					
						// Why not getString? Becouse it has size limit and can't be used properly with JSON statements!

						std::cout << "STRING: " << std::endl;
						
						std::unique_ptr< std::istream > buffer(res->getBlob(i));
						buffer->seekg (0, buffer->end);
						int length = buffer->tellg();
						buffer->seekg (0, buffer->beg);	
						char str[length];
						buffer->get(str, length+1);	
						
						//-----------------Gera strings
						
						sql::SQLString trans_str = str;
						String p_str = SQLstr2GDstrS(trans_str);

						//-----------------Gera variant
	
						int strlen = p_str.length();
						CharString cstr = p_str.ascii();
						PoolVector<uint8_t> buf;
						buf.resize(strlen / 4 * 3 + 1);
						PoolVector<uint8_t>::Write w = buf.write();
						size_t len = 0;
						
						// Return Variant
						bool is_variant = false;
						if (CryptoCore::b64_decode(&w[0], buf.size(), &len, (unsigned char *)cstr.get_data(), strlen) == OK){
							Variant ret_var;
							std::cout << "PRIMEIRO TESTE : " << std::endl;
							
							
							Error err = decode_variant(ret_var, &w[0], len, NULL, p_full_objects); //FIXME throw error when Error != ERR_INVALID_DATA
							
							if (err == OK){
								is_variant = true;
								std::cout << "RETURN VARIANT: " << std::endl;
								
								if  ( data_model == DICTIONARY ){ 
									row[ column_name ] = ret_var; 
								}
								else { 
									line.push_back( ret_var ); 
								}						
							}
						}
						
						
						// Return String or Json
						if (!is_variant) {
							// return json
							if (is_json( p_str )){
								//TODO: Habilitar automatic json parser???
								//Decidiro que fazer
								std::cout << "RETURN JSON: " << trans_str << std::endl;

							
							}
						
						
						
							if  ( data_model == DICTIONARY ){ 
								row[ column_name ] = p_str; 
							}
							else { 
								line.push_back( p_str ); 
							}
							
						}

					}				


					//  BINARY - PoolByteArray
					else if (d_type == sql::DataType::BINARY || d_type == sql::DataType::VARBINARY || d_type == sql::DataType::LONGVARBINARY || d_type == sql::DataType::JSON ){
					std::cout << "BINARY" << std::endl;  
					//TODO: Refinar isso
						PoolByteArray _result;
						std::unique_ptr< std::istream > blob_output_stream(res->getBlob(i));
						char _buff;
						blob_output_stream->seekg(0, blob_output_stream->end);
						int out_stream_length = blob_output_stream->tellg();
						blob_output_stream->seekg(std::ios::beg);
						_result.resize(out_stream_length);
						PoolVector<uint8_t>::Write _data = _result.write();

						for (int w = 0; w < out_stream_length; w++){
							blob_output_stream->get(_buff);
							_data[w] = ((uint8_t)_buff);
						}

						_data.release();

						if  ( data_model == DICTIONARY ){ 
							row[ column_name ] = _result; 
						}
						else { 
							line.push_back( _result ); 
						}			
					}
					
					else{
						//TODO
						print_line("Format not supoeted!!");
					}
					
									
					
				} // ELSE -> RETURN DATA
			} // FOR



			if ( data_model == DICTIONARY ) 
				{ ret.push_back( row ); 
			}
			else { 
				ret.push_back( line ); 
			}		

		}  // WHILE
	} // try
	
	catch (sql::SQLException &e) { print_SQLException(e); } 
	catch (std::runtime_error &e) { print_runtime_error(e); }

	return ret;

} 



int MySQL::execute( String p_sqlquery ){ return _execute( p_sqlquery, Array(), false ); }

int MySQL::execute_prepared( String p_sqlquery, Array p_values){ return _execute( p_sqlquery, p_values, true ); }

int MySQL::_execute( String p_sqlquery, Array p_values, bool prep_st){

	sql::SQLString query = GDstr2SQLstr( p_sqlquery );
	int afectedrows;
	
	Vector<std::stringstream*> multiBlob;
	//std::stringstream blob;
	
	try {
		if (prep_st){	
		//	std::shared_ptr <sql::PreparedStatement> prep_stmt;
		//	prep_stmt.reset(conn->prepareStatement(query));
			
			//set_datatype( prep_stmt,  p_values, &multiBlob);  //FIXME: BUG o BLOB  que vai ser utilizado mais de uma vez. Vai dar merda na hora do execute.  
			// OPCÕES: Usar um Vector<std::stringstream> 
			// OPCÕES: 
			
			
			//afectedrows = prep_stmt->executeUpdate();
			
			afectedrows = set_datatype(query , p_values);
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



//void MySQL::set_datatype(std::shared_ptr <sql::PreparedStatement> prep_stmt, Array p_values, Vector<std::__cxx11::basic_stringstream<char>*> *multiBlob ){
int MySQL::set_datatype( sql::SQLString query, Array p_values){


	
	std::unique_ptr<sql::PreparedStatement> prep_stmt;
	prep_stmt.reset(conn->prepareStatement(query));
	
	//Vector<std::stringstream>::Write multiBlob;
	
	std::vector<std::stringstream*> multiBlob (p_values.size());
	size_t cont = 0;



	for (int i = 0; i < p_values.size() ; i++){

		int value_type = p_values[i].get_type();
			
	//NULL  
		if (value_type == Variant::NIL){ 
			prep_stmt->setNull(i+1, sql::DataType::SQLNULL); 
		}  

	//BOOL
		else if (value_type == Variant::BOOL){ 
			prep_stmt->setBoolean(i+1, bool(p_values[i])); 
		}
				
	//INT
		else if (value_type == Variant::INT){ 
			prep_stmt->setInt64(i+1, int64_t(p_values[i])); 
		}
		
	// FLOAT
#ifdef GODOT4		
		else if (value_type) == Variant::FLOAT){ 
			prep_stmt->setDouble(i+1, double(p_values[i])); 
		}
#else
		else if (value_type == Variant::Variant::REAL){ 
			prep_stmt->setDouble(i+1, double(p_values[i])); 
		}
#endif

	// STRING - DATATIME - JSON
		else if (value_type == Variant::STRING){

			String str_data = p_values[i];
			sql::SQLString sql_data = GDstr2SQLstr(str_data);
			std::string std_string = str_data.utf8().get_data();
			
			
			std::cout << "IS JSON: " << std::boolalpha << ( is_json( str_data ) ) << std::endl;

			if ( is_mysql_time( str_data )) {
				prep_stmt->setDateTime(i+1, sql_data );
			}
			
			
			
			else if ( is_json( str_data ) ) {
				std::stringstream blob;
				blob.str(std_string);
				//blob.seekg(std::ios::beg);	// TODO Checar se essa linha é relevante ou pode ser removida 
				multiBlob[ cont ] = &blob;
				prep_stmt->setBlob(i+1, multiBlob[ cont ] );		
				cont++; // Deve ficar no final do loop

			}
			else{
				prep_stmt->setString(i+1, sql_data );
			}
		}


	// BINARY
		else if (value_type == Variant::POOL_BYTE_ARRAY){	
			std::stringstream blob;
			PoolByteArray data = PoolByteArray(p_values[i]);
			PoolVector<uint8_t>::Read buffer = data.read();
			int length = data.size();

			for ( int y = 0; y < length; y++){
				blob << (char)buffer[y];
			}

			blob.seekg(std::ios::beg);
			prep_stmt->setBlob(i+1, &blob);
		}
	
		
	// VARIANT
		else{
			std::stringstream blob;
			_Marshalls *marshalls = memnew(_Marshalls);
			String buff = marshalls->variant_to_base64(p_values[i], p_full_objects);
			blob << buff.utf8().get_data();
			blob.seekg(std::ios::beg);	
			std::cout << "VARIANT: " << blob.str()  <<std::endl; 
			prep_stmt->setBlob(i+1, &blob);
		}

			

	



	} // FOR 
	

	
	return prep_stmt->executeUpdate();
}
	



/*
setBlob std::istream * blob
setBoolean bool value
setDateTime const sql::SQLString& value
setDouble double value
setInt64 int64_t value
setNull int sqlType
setString const sql::SQLString& value
setJson stream	
*/


/*
NULL -> setNull
BOOL -> setBoolean
INTEGER -> setInt64
FLOAT -> setDouble

STRING -> setString
	JSON -> setBlob (stringstream)
	DATATIME -> setDateTime
	
	
POOL_BYTE_ARRAY -> setBlob(istream)

VARIANT GERAL/OBJECT (encode to binary)-> setBlob setBlob(istream)
	
*/





//---------------------------------------------------------------------------------------------

//	https://gist.github.com/FlorianWolters/be839c84991a789df1c6
//	https://stackoverflow.com/questions/45722747/how-can-i-create-a-istream-from-a-uint8-t-vector
//	https://stackoverflow.com/questions/1121142/set-binary-data-using-setblob-in-mysql-connector-c-causes-crash
//	https://dev.mysql.com/doc/refman/8.0/en/blob.html  NOTE: Afeta o desempenho
//	https://mariadb.com/kb/en/json-data-type/
//  https://zh.wikibooks.org/zh-hk/MySQL_Connector/C%2B%2B

/*

			
		// JSON
			
			else if ( is_json( str_val ) ) {
				std::string s = str_val.utf8().get_data();
				std::istringstream iss(s);
				std::istream& stream = iss;
				iss << id;
				prep_stmt->setBlob(i, &stream );
			}
			
			//TEXT & CHAR
			//else{
		//		prep_stmt->setString(i, sql_str );	
		//	}
			
			
*/


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
/*
void MySQL::set_properties_kit(Dictionary p_properties){


}


Dictionary MySQL::get_properties_kit(Array p_properties){

	return Dictionary();
}

*/

	
	
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


	if (value_type == "string" || value_type == "void") { 
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

void MySQL::print_SQLException(sql::SQLException &e) {  //FIXME 
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



//--------GODOT STUFF-------------------

void MySQL::_bind_methods() {

	/*
	Propriedades:
		can reconnect
		encode objects
		accept multi statement
		encode_object
		use_json	
	*/

	//ADD_PROPERTY(PropertyInfo(Variant::BOOL, "can_reconnect"), "set_reconnect", "can_reconnect");
	//ADD_PROPERTY(PropertyInfo(Variant::BOOL, "encode objects"), "set_reconnect", "can_reconnect");
	//ADD_PROPERTY(PropertyInfo(Variant::BOOL, "accept multi statement"), "set_reconnect", "can_reconnect");
	//ADD_PROPERTY(PropertyInfo(Variant::BOOL, "encode_object"), "set_reconnect", "can_reconnect");
	//ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_json"), "set_reconnect", "can_reconnect");


	ClassDB::bind_method(D_METHOD("test", "argument"),&MySQL::test);



	//--- Connection Managers
	ClassDB::bind_method(D_METHOD("set_credentials", "HostName", "UserName", "Password"),&MySQL::set_credentials);
	ClassDB::bind_method(D_METHOD("get_connection_status"),&MySQL::get_connection_status);
	ClassDB::bind_method(D_METHOD("start_connection"),&MySQL::start_connection);
	ClassDB::bind_method(D_METHOD("stop_connection"),&MySQL::stop_connection);
	

	//--- Properties Managers
	ClassDB::bind_method(D_METHOD("set_property", "property", "p_value"),&MySQL::set_property);
	ClassDB::bind_method(D_METHOD("get_property", "property"),&MySQL::get_property);
	
	
	//ClassDB::bind_method(D_METHOD("set_properties_kit", "properties"),&MySQL::set_properties_kit);
	//ClassDB::bind_method(D_METHOD("get_properties_kit", "properties"),&MySQL::get_properties_kit);


	//--- Execute/Query
	ClassDB::bind_method(D_METHOD("execute", "sql_statement"),&MySQL::execute);
	ClassDB::bind_method(D_METHOD("execute_prepared", "sql_statement", "Values"),&MySQL::execute_prepared);
	
	ClassDB::bind_method(D_METHOD("query", "sql_statement", "DataFormat", "return_string", "meta"),&MySQL::query, DEFVAL(DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()) );
	ClassDB::bind_method(D_METHOD("query_prepared", "sql_statement", "Values", "DataFormat", "return_string", "meta"),&MySQL::query_prepared, DEFVAL(Array()), DEFVAL(DICTIONARY), DEFVAL(false), DEFVAL(PoolIntArray()));
		

	BIND_ENUM_CONSTANT(NO_CONNECTION);
	BIND_ENUM_CONSTANT(CLOSED);
	BIND_ENUM_CONSTANT(CONNECTED);
	BIND_ENUM_CONSTANT(DISCONNECTED);

	BIND_ENUM_CONSTANT(ARRAY);
	BIND_ENUM_CONSTANT(DICTIONARY);
	

	BIND_ENUM_CONSTANT(COLUMNS_NAMES);
	BIND_ENUM_CONSTANT(COLUMNS_TYPES);
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

Variant MySQL::transaction( [] ){
}


Variant MySQL::transaction_prepared( {} ){
}

*/

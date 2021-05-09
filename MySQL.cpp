//*  MySQL.cpp */


#include "MySQL.h"
#include "core/io/json.h"


Variant MySQL::test(PoolByteArray p_args){



	PoolVector<uint8_t>::Read p_data = p_args.read();
	std::vector<uint8_t> data;
	

	
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());
	processData(stream);
	
	
	
	std::istream * blob ;




	
	


	return Variant();
}


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
			
		}else{
			std::shared_ptr <sql::Statement> stmt;
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

void MySQL::set_datatype(std::shared_ptr <sql::PreparedStatement> prep_stmt, Array prep_val ){

	//for (int i = 1; i <= 5; ++i) 
	for (int i = 0; i < prep_val.size() ; i++){
	
		if (prep_val[i].get_type() == Variant::Variant::NIL){
			prep_stmt->setNull(i, sql::DataType::SQLNULL);
		}

		else if (prep_val[i].get_type() == Variant::Variant::BOOL){
			prep_stmt->setBoolean(i, bool(prep_val[i]));
		}
		

		else if (prep_val[i].get_type() == Variant::Variant::INT){
			prep_stmt->setInt64(i, int64_t(prep_val[i]));
		}

/*
		else if (prep_val[i].get_type() == Variant::Variant::POOL_BYTE_ARRAY){
			PoolByteArray data = prep_val[i];
			prep_stmt->setBlob(  p_stream );
		}

*/


///--------

		else if (prep_val[i].get_type() == Variant::Variant::STRING){
			
			String str_val = String( prep_val[i] );
			sql::SQLString _sql_val = GDstr2SQLstr( str_val );
			
			// Date and Time types
			if ( is_mysql_time( str_val )) { 
				prep_stmt->setDateTime(i, _sql_val );
			}
			
			// JSON
			else if ( is_json( str_val ) ) {
			
			}
			
			//TEXT & CHAR
			else{
				prep_stmt->setString(i, _sql_val );	
			}
			
			
		}






#ifdef GODOT4		
		else if (prep_val[i].get_type() == Variant::Variant::FLOAT){
			prep_stmt->setDouble(i, double(prep_val[i]));
		}
		
#else
		else if (prep_val[i].get_type() == Variant::Variant::REAL){
			prep_stmt->setDouble(i, double(prep_val[i]));
		}
		
#endif

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
		return SQLstr2GDstr( _p_s ); 
	}


	if (value_type == "string" or value_type == "void") { 
		const sql::SQLString * p_s;
		p_s = connection_properties[ property ].get<sql::SQLString>();
		return SQLstr2GDstr( p_s ); 
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


String MySQL::SQLstr2GDstr( const sql::SQLString *p_str ) {
	const char * _str_ = p_str->c_str();
	String str = String::utf8( (char *) _str_ );
	return str;
}



String MySQL::SQLstr2GDstr( sql::SQLString &p_string ) {
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
	print_line("FINAL");	
	return "invalid";
}







bool MySQL::is_mysql_time(String time) {
	if (get_time_format( time ) == NON_TIME){
		return false;
	} 
	else {
		return true;
	}
}



MySQL::SQL_TIME MySQL::get_time_format(String time) {

	std::string s_time = time.utf8().get_data();    ///Impo
	int len = time.length();

	if (s_time.find_first_not_of( "0123456789:- " ) == std::string::npos) {

		// - 0000
		if ( len == 4 ) {
			if (s_time.find_first_not_of( "0123456789" ) == std::string::npos) {
				return YEAR;
			}
		}

		// - 00:00:00
		else if ( len == 8 ) {
			if ( time[2] ==  ':' && time[5] == ':' ) {
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 3 && String(arr_time[2]).length() == 2) {
					return TIME;
				}
			}
		}

		// - 0000-00-00
		else if ( len == 10 ) {
			if ( time[4] ==  '-' && time[7] == '-') {
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 3 && String(arr_time[2]).length() == 2) {
					return DATE;
				}
			}
		}

		// - 0000-00-00 00:00:00
		else if ( len == 19 ) {
			if ( time[4] ==  '-' && time[7] == '-' && time[13] ==  ':' && time[16] == ':' && time[10] == ' ' ) {
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 6 && String(arr_time[2]).length() == 2) {
					return DATETIME;
				}
			}
		}
	}
	return NON_TIME;
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



//--------GODOT STUFF-------------------

void MySQL::_bind_methods() {
	//--- Connection Managers
	ClassDB::bind_method(D_METHOD("get_connection_status"),&MySQL::get_connection_status);
	ClassDB::bind_method(D_METHOD("start_connection"),&MySQL::start_connection);
	ClassDB::bind_method(D_METHOD("stop_connection"),&MySQL::stop_connection);


	ClassDB::bind_method(D_METHOD("set_property", "p_property", "p_value"),&MySQL::set_property);
	ClassDB::bind_method(D_METHOD("get_property", "p_property"),&MySQL::get_property);

	ClassDB::bind_method(D_METHOD("test", "p_args"),&MySQL::test);


	ClassDB::bind_method(D_METHOD("execute", "p_sqlquery"),&MySQL::execute);
	
	
	//ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reconnect"), "set_reconnect", "can_reconnect");
	

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
}




MySQL::MySQL(){
	connection_properties["port"] = 3306;
	connection_properties["OPT_RECONNECT"] = true;
	connection_properties["CLIENT_MULTI_STATEMENTS"] = true;
}


MySQL::~MySQL(){
	stop_connection();
}


/*

Variant MySQL::set_properties(Dictionary p_options){
}


Dictionary MySQL::get_properties(Array p_options = []){
}


int MySQL::execute_query(String p_SQLquery){
}


int MySQL::execute_prepared_query(String p_SQLquery, Array p_values){
}


Array MySQL::fetch_query(String p_SQLquery, int return_like = DICTIONARY,  bool return_string = false){
}


Array MySQL::fetch_prepared_query(String p_SQLquery, Array p_values, int return_what = DICTIONARY,  bool return_string = false){
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

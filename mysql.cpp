/*  mysql.cpp */

#include "mysql.h"
#include <memory>

using namespace std;


//-------------- Connection Managers

bool MySQL::connection_start() { return check(ACT_DO); }  

bool MySQL::connection_check() { return check(ACT_CHECK); }  

bool MySQL::connection_close() { return check(ACT_CLOSE); } 

void MySQL::set_credentials( String p_host, String p_user, String p_pass )
{

	connection_properties["hostName"] = p_host.utf8().get_data(); 
	connection_properties["userName"] = p_user.utf8().get_data(); 
	connection_properties["password"] = p_pass.utf8().get_data();
}


void MySQL::set_client_options(String p_option, String p_value)
{
	shared_ptr <sql::Connection> con(connection(ACT_DO));
	sql::SQLString option = p_option.utf8().get_data();
	sql::SQLString value = p_value.utf8().get_data();
	con->setClientOption(option, value);
}


String MySQL::get_client_options(String p_option)
{
	shared_ptr <sql::Connection> con(connection(ACT_DO));
	sql::SQLString option = p_option.utf8().get_data();
	return sql2String( con->getClientOption( option ) );
}


//-------------- Query

Array MySQL::query_fetch_dictionary(String p_SQLquery, bool return_string) 
{  
	return make_query(p_SQLquery, FUNC_DICT, emptyarray, return_string); 
}

	
Array MySQL::query_fetch_array(String p_SQLquery, bool return_string) 
{  
	return make_query(p_SQLquery, FUNC_ARRAY, emptyarray, return_string); 
}


Array MySQL::query_get_columns_types(String p_SQLquery) 
{   
	return make_query(p_SQLquery, FUNC_TYPE, emptyarray, true);  
}	


Array MySQL::query_get_columns_names(String p_SQLquery) 
{  
	return make_query(p_SQLquery, FUNC_NAME, emptyarray, true); 
}	


int MySQL::query_execute(String p_SQLquery) 
{   
	make_query(p_SQLquery, FUNC_EXEC, emptyarray, false);
	int rows = afectedrows;
	afectedrows = 0;
	return  rows;
}


//-------------- Prepared Query

Array MySQL::prep_fetch_dictionary(String p_SQLquery, Array prep_val, bool return_string) 
{  
	return make_query(p_SQLquery, FUNC_DICT_PREP, prep_val, return_string); 
}

	
Array MySQL::prep_fetch_array(String p_SQLquery, Array prep_val, bool return_string )
{  
	return make_query(p_SQLquery, FUNC_ARRAY_PREP, prep_val, return_string); 
}


Array MySQL::prep_get_columns_types(String p_SQLquery, Array prep_val) 
{   
	return make_query(p_SQLquery, FUNC_TYPE_PREP, prep_val, true);  
}	


Array MySQL::prep_get_columns_names(String p_SQLquery, Array prep_val) 
{  
	return make_query(p_SQLquery, FUNC_NAME_PREP, prep_val, true); 
}	


int MySQL::prep_execute(String p_SQLquery, Array prep_val) 
{   
	make_query(p_SQLquery, FUNC_EXEC_PREP, prep_val, false);
	int rows = afectedrows;
	afectedrows = 0;
	return  rows;
}


//-------------- Database

String MySQL::get_database()
{
	shared_ptr< sql::Connection > con(connection(ACT_DO));
	if (con != NULL)
	{
		return sql2String( con->getSchema() ); 
	}
	else
	{ return (String)"Invalid Connection!";}
}


void MySQL::set_database( String p_database )
{
	sql::SQLString database = p_database.utf8().get_data();

	if(database != "")
	{
		shared_ptr< sql::Connection > con(connection(ACT_DO));
		if (con != NULL)
		{
			con->setSchema(database);	
		}else{
			 connection_properties["schema"] = database;
		}
	}
}



Array MySQL::make_query(String p_SQLquery, int type, Array prep_val, bool return_string){

	sql::SQLString SQLquery = p_SQLquery.utf8().get_data();
	Array ret;

	shared_ptr <sql::Connection> con(connection(ACT_DO));

	shared_ptr <sql::Statement> stmt;
	shared_ptr <sql::PreparedStatement> prep_stmt;
	shared_ptr <sql::ResultSet> res;
	sql::ResultSetMetaData *res_meta;

	try
	{
		//----------- EXECUTE
		if ( type == FUNC_EXEC || type == FUNC_EXEC_PREP )  // ExecuteUpdate functions
	 	{
			if ( type == FUNC_EXEC )
			{
				stmt.reset(con->createStatement());
				afectedrows = stmt->executeUpdate(SQLquery);
			}
			else
			{
			    prep_stmt.reset(con->prepareStatement(SQLquery));
				determine_datatype( prep_stmt,  prep_val);
				afectedrows = prep_stmt->executeUpdate();
			}
		}
		else // - Non executeUpdate functions
		{

			if ( (type == FUNC_TYPE || type == FUNC_TYPE || type == FUNC_DICT || type == FUNC_ARRAY) ) // Non prep statement
			{
				stmt.reset(con->createStatement());
				res.reset( stmt->executeQuery(SQLquery));
				res_meta = res->getMetaData();
			}
			else // Prep statement
			{
				prep_stmt.reset(con->prepareStatement(SQLquery));
				determine_datatype( prep_stmt,  prep_val);
				res.reset( prep_stmt->executeQuery());    //----Combo Arrays???
				res_meta = res->getMetaData();		
			}

			//----------- NAME && TYPE
			if ( type == FUNC_NAME || type == FUNC_TYPE || type == FUNC_NAME_PREP || type == FUNC_TYPE_PREP) 
			{

				
					if ( type == FUNC_TYPE || type == FUNC_TYPE_PREP )
					{
						for (int i = 1; i <= res_meta->getColumnCount(); i++)       	    
						{ 
							ret.push_back(sql2String(res_meta->getColumnTypeName(i))); 
						}
					}

					if ( type == FUNC_NAME || type == FUNC_NAME_PREP )
					{
						for (int i = 1; i <= res_meta->getColumnCount(); i++)       	    
						{ 
							ret.push_back(sql2String(res_meta->getColumnName(i))); 
						}
					}
				
			}	

			//----------- DICT && ARRAY
			if ( type == FUNC_DICT || type == FUNC_ARRAY || type == FUNC_DICT_PREP || type == FUNC_ARRAY_PREP ) 
			{
				while (res->next())
				{			
					Array line;
					Dictionary row;

					for (int i = 1; i <= res_meta->getColumnCount(); i++) 
					{

						if ( (type == FUNC_DICT || type == FUNC_DICT_PREP ) && return_string  )		// - return_string DICT
							{ row[sql2String(res_meta->getColumnName(i))] = sql2String(res->getString(i)); }

						else if ( (type == FUNC_ARRAY || type == FUNC_ARRAY_PREP) && return_string)		// - return_string ARRAY
							{ line.push_back( (sql2String(res->getString(i))) ); }


						else if ( !return_string )
							{
								int g_type = res_meta->getColumnType(i);

						//---------- INT
								if ( g_type == sql::DataType::BIT || g_type == sql::DataType::TINYINT || g_type == sql::DataType::SMALLINT || g_type == sql::DataType::MEDIUMINT || g_type == sql::DataType::INTEGER || g_type == sql::DataType::BIGINT )
								{
									if ( type == FUNC_DICT || type == FUNC_DICT_PREP )
										{ row[sql2String(res_meta->getColumnName(i))] = res->getInt(i); }
									else
										{ line.push_back( res->getInt(i) ); }
								}
						//----------  FLOAT

								else if ( g_type == sql::DataType::REAL || g_type == sql::DataType::DOUBLE || g_type == sql::DataType::DECIMAL || g_type == sql::DataType::NUMERIC ) 
								{ 
									float floteando = res->getDouble(i);

									if ( type == FUNC_DICT || type == FUNC_DICT_PREP )
										{row[sql2String(res_meta->getColumnName(i))] = floteando;} 
									else
										{line.push_back( floteando ); } 
								} 

						//----------  TIME
								else if (g_type == sql::DataType::DATE || g_type == sql::DataType::TIME || g_type == sql::DataType::TIMESTAMP || g_type == sql::DataType::YEAR )
								/*		It should return time information as a dictionary when calling "fetch_dictionary", but if the sequence of the  
								data be modified (using TIME_FORMAT or DATE_FORMAT for exemple), it will return the dictionary fields with wrong names. 
								so I prefer return the data as an array.	*/
								{
									if ( type == FUNC_DICT || type == FUNC_DICT_PREP )
										{  row[sql2String(res_meta->getColumnName(i))] = format_time(((sql2String(res->getString(i))).utf8().get_data()) , false); }
									else
										{ line.push_back( format_time( ((sql2String(res->getString(i))).utf8().get_data()) , false) ); }
								}

						//----------  STRING
								else // - ANY OTHER DATATYPE non listed above gonna be returned as STRING (Including char types, of course).
								{
									if ( type == FUNC_DICT || type == FUNC_DICT_PREP )
										{ row[sql2String(res_meta->getColumnName(i))] = sql2String(res->getString(i)); }
									else
										{ line.push_back( sql2String(res->getString(i))); }
								}

							}  // if  !return_string 

					}// for

					if ( type == FUNC_DICT || type == FUNC_DICT_PREP)
						{ ret.push_back( row ); }
					else
						{ ret.push_back( line ); }

				}  /// while 

			} // if FUNC = DICT || ARRAY || DICT_PREP || ARRAY_PREP

		} // - Non executeUpdate functions

	}  //-Try

	catch (sql::SQLException &e) { print_SQLException(e); } 
	catch (runtime_error &e) { print_runtime_error(e); 	}

	return ret;
}

shared_ptr<sql::Connection> MySQL::connection(int what)
{
	
	static sql::mysql::MySQL_Driver *driver; 
	static shared_ptr <sql::Connection> con;

	if (what == ACT_CLOSE)  
	{  if (con.get()) // != NULL
		{ 
			if (!con->isClosed())	{ con->close();  }
		}
	}

	if ( what == ACT_DO  )
	{
		if ( con == NULL || (!con->isValid()) || (!con->reconnect()) ) 
		{	
			try
			{	
				driver = sql::mysql::get_mysql_driver_instance();
				con.reset(driver->connect(connection_properties)); 
			}

			catch (sql::SQLException &e) {	print_SQLException(e); 	} 
			catch (runtime_error &e) {	print_runtime_error(e);	 }
		}
	}
	return con;
}


//-------------- Helpers

//********
bool MySQL::check(int what)
{
	shared_ptr< sql::Connection > con(connection(what));
	if (con != NULL)
	{ 
 		if (!con->isClosed())
 		{ 
			return con->isValid();	
		}
	}
	return false;
}

//********
String MySQL::sql2String(sql::SQLString p_str)
{
	const char * c = p_str.c_str();
	String str = String::utf8((char *)c);
	return str;
}


//**********
void MySQL::determine_datatype( std::shared_ptr <sql::PreparedStatement> prep_stmt , Array prep_val)
{
	for (int i = 0; i <= (prep_val.size() -1); i++) // Determine datatype      
	{
		int d = i+1;

		if 		( prep_val[i].get_type() == Variant::NIL )	{	prep_stmt->setNull(d, sql::DataType::SQLNULL);	}
				
		else if ( prep_val[i].get_type() == Variant::BOOL)	{	prep_stmt->setBoolean(d, bool(prep_val[i]));	}
								
		else if ( prep_val[i].get_type() == Variant::INT)	{	prep_stmt->setInt(d, int(prep_val[i]));	}

		else if ( prep_val[i].get_type() == Variant::REAL)	{	prep_stmt->setDouble(d, float(prep_val[i]));	}

		else  // - Gonna handle any other Godot datatype as string
		{
			String stri = String(prep_val[i]);
			sql::SQLString caracteres = stri.utf8().get_data();

			if ( is_mysql_time( stri ))  // -- If the string has the mysql time type format, this gonna be handle as Date and Time types

				{ prep_stmt->setDateTime(d, caracteres ); }
			else
				{ prep_stmt->setString(d, caracteres ); }
		}
	}
}
//********

Array MySQL::format_time(String str, bool return_string)
{

	Array datando;																		
	string strss = 	str.utf8().get_data();				
	char seps[] = ": -";
	char *token;
	token = strtok( &strss[0], seps );

	while( token != NULL )
	{
		if (return_string) { datando.push_back( String(token) ); }   //--As String
		else  { datando.push_back(atoi(token)); } 					 //--As Data

		token = strtok( NULL, seps );
	}

	return datando;
}
//********


bool MySQL::is_mysql_time(String time)
{
	string s_time = time.utf8().get_data();    ///Impo
	int len = time.length();

	if (s_time.find_first_not_of( "0123456789:- " ) == string::npos)
	{
	
	// - 0000
		if ( len == 4 )			
		{
			if (s_time.find_first_not_of( "0123456789" ) == string::npos) {	return true; }
		}

	// - 00:00:00
		else if ( len == 8 )		
		{
			if ( time[2] ==  ':' && time[5] == ':' ) 
			{
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 3 && String(arr_time[2]).length() == 2)  { return true; }
			}
		}

	// - 0000-00-00
		else if ( len == 10 )	
		{
			if ( time[4] ==  '-' && time[7] == '-')
			{
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 3 && String(arr_time[2]).length() == 2)  { return true; }
			}
		}

	// - 0000-00-00 00:00:00
		else if ( len == 19 )		
		{
			if ( time[4] ==  '-' && time[7] == '-' && time[13] ==  ':' && time[16] == ':' && time[10] == ' ' )
			{
				Array arr_time = format_time(time, true);
				if ( arr_time.size() == 6 && String(arr_time[2]).length() == 2)  { return true; }
			}
		}
	}
	return false;
}	

//********
void MySQL::print_SQLException(sql::SQLException &e) 
{	
	//(e.getErrorCode() == 1047) = No prepareted statement support at all.

	print_line("# EXCEPTION Caught ˇ");
	Variant file = __FILE__;
	Variant line = __LINE__;
	Variant func = __FUNCTION__;
		print_line("# ERR: SQLException in: "+String(file)+" in function: "+String(func)+"() on line "+String(line));
		print_line("# ERR: "+String(e.what()));
	Variant errCode = e.getErrorCode();
		print_line(" (MySQL error code: "+String(errCode)+")");
		print_line("SQLState: "+sql2String(e.getSQLState()));
} 

//********
void MySQL::print_runtime_error(runtime_error &e) 
{	
	cout << "ERROR: runtime_error in " << __FILE__;
	cout << " (" << __func__ << ") on line " << __LINE__ << endl;
	cout << "ERROR: " << e.what() << endl;
}



void MySQL::_bind_methods()
{
	//--- Connection Managers
	ClassDB::bind_method(D_METHOD("connection_start"),&MySQL::connection_start);
	ClassDB::bind_method(D_METHOD("connection_check"),&MySQL::connection_check);
	ClassDB::bind_method(D_METHOD("connection_close"),&MySQL::connection_close);

	ClassDB::bind_method(D_METHOD("set_credentials", "Host", "User", "Password"),&MySQL::set_credentials);	
	ClassDB::bind_method(D_METHOD("set_client_options", "Option", "Value"),&MySQL::set_client_options);	
	ClassDB::bind_method(D_METHOD("get_client_options", "Option"),&MySQL::get_client_options);	


	//--- Database
	ClassDB::bind_method(D_METHOD("set_database", "Database"),&MySQL::set_database);	
	ClassDB::bind_method(D_METHOD("get_database"),&MySQL::get_database);	


	//--- Prepared Query
	ClassDB::bind_method(D_METHOD("prep_execute", "SQL_execute", "Array of values"),&MySQL::prep_execute);	
	ClassDB::bind_method(D_METHOD("prep_fetch_dictionary", "SQL_query", "Array of values", "return data as string"),&MySQL::prep_fetch_dictionary);	
	ClassDB::bind_method(D_METHOD("prep_fetch_array", "SQL_query", "Array of values", "return data as string"),&MySQL::prep_fetch_array);	
	ClassDB::bind_method(D_METHOD("prep_get_columns_names", "SQL_query", "Array of values"),&MySQL::prep_get_columns_names);	
	ClassDB::bind_method(D_METHOD("prep_get_columns_types", "SQL_query", "Array of values"),&MySQL::prep_get_columns_types);	


	//---  Query
	ClassDB::bind_method(D_METHOD("query_execute", "SQL_execute"),&MySQL::query_execute);	
	ClassDB::bind_method(D_METHOD("query_fetch_dictionary", "SQL_query", "return data as string"),&MySQL::query_fetch_dictionary);	
	ClassDB::bind_method(D_METHOD("query_fetch_array", "SQL_query", "return data as string"),&MySQL::query_fetch_array);	
	ClassDB::bind_method(D_METHOD("query_get_columns_names", "SQL_query"),&MySQL::query_get_columns_names);	
	ClassDB::bind_method(D_METHOD("query_get_columns_types", "SQL_query"),&MySQL::query_get_columns_types);	

}

MySQL::MySQL(){
	connection_properties["port"] = 3306;
	connection_properties["OPT_RECONNECT"] = true;
	afectedrows = 0;
}

MySQL::~MySQL(){
	connection_close();
}

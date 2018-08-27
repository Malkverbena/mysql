/*  mysql.cpp */


#include "mysql.h"

using namespace std;



sql::mysql::MySQL_Driver *driver;  //Thread-safe
//sql::Driver *driver;  //Not Thread-safe
sql::Connection *con;
sql::Statement *stmt;
sql::ResultSet *res;
sql::ResultSetMetaData *res_meta;

sql::SQLString host;
sql::SQLString user;
sql::SQLString pass;
sql::SQLString database;

void MySQL::set_credentials(String p_host, String p_user, String p_pass){
    host = p_host.utf8().get_data();
    user = p_user.utf8().get_data();
    pass = p_pass.utf8().get_data();
}

void MySQL::select_database(String p_database){
    database = p_database.utf8().get_data();
}


//--- Common Functions
void MySQL::query(String p_sql_query){ 

	sql_tasks( p_sql_query, 0); 
}


Array MySQL::fetch_dictinary(String p_sql_query)  { return sql_tasks( p_sql_query, 1); }


Array MySQL::fetch_dictinary_string(String p_sql_query)  { return sql_tasks( p_sql_query, 2); }


Array MySQL::fetch_array_string(String p_sql_query)  { return sql_tasks( p_sql_query, 3); }


Array MySQL::fetch_array(String p_sql_query)  { return sql_tasks( p_sql_query, 4); }


Array MySQL::get_columns_name(String p_sql_query)   { return sql_tasks( p_sql_query, 5); }


Array MySQL::get_column_types(String p_sql_query)  { return sql_tasks( p_sql_query, 6); }



//-- True Gears
Array MySQL::sql_tasks(String p_sql_query, int task){

	sql::SQLString SQLquery = p_sql_query.utf8().get_data();
	Array ret;

	try {
		driver = sql::mysql::get_mysql_driver_instance(); // Thread-safe
		//driver = get_driver_instance(); //Not Thread-safe
		con = driver->connect(host, user, pass);

		if(database != "")
		{
			con->setSchema(database);
		}

		switch(task){
			case 0:  //--query		
			
				stmt = con->createStatement();
				stmt->execute(SQLquery);	
				
			break;
			
			case 1:  //--fetch_dictinary  

				stmt = con->createStatement();
				res = stmt->executeQuery(SQLquery);
				res_meta = res->getMetaData();
						
				while (res->next()){
				
					Dictionary row;
					Array datando;

					for (int j = 1; j <= res_meta->getColumnCount(); j++) 
					{
						String testype = sql2String(res_meta->getColumnTypeName(j));  
						bool othertypes = false;
						
						//Returns an Interger
						if (testype == "INT" || testype == "TINYINT" || testype == "SMALLINT"|| testype == "MEDIUMINT" || testype == "BIGINT" || testype == "INTEGER"){
		 					row[sql2String(res_meta->getColumnName(j))] = res->getInt(j);
							othertypes = true;
						}

						//Returns strings
						if (testype == "VARCHAR"|| testype == "CHAR" || testype == "TINYTEXT" || testype == "TEXT" || testype == "MEIUMTEXT" || testype == "LONGTEXT" || testype == "LONGVARCHAR") {
							row[sql2String(res_meta->getColumnName(j))] = sql2String(res->getString(j));
							othertypes = true;
						}

						//Returns a float
						if (testype == "FLOAT" || testype == "DECIMAL" || testype == "REAL" || testype == "DOUBLE" || testype == "NUMERIC"){
							float floteando = res->getDouble(j);
							row[sql2String(res_meta->getColumnName(j))] = floteando;
							othertypes = true;
						}
					
						//Returns boolean
						if (testype == "BOOL" || testype == "BOOLEAN" || testype == "BIT"){
							row[sql2String(res_meta->getColumnName(j))] = res->getBoolean(j);
							othertypes = true;
						}

						// It should return time information as a dictionary, but if the sequencing of the data be 
						// modified (using TIME_FORMAT or DATE_FORMAT for exemple), it will return the dictionary fields with wrong names. 
						// So I preferred return the data as an array.
						if ( testype == "DATETIME"|| testype == "TIMESTAMP" || testype == "DATE" || testype == "TIME" || testype == "YEAR") {
						
							string str = (sql2String(res->getString(j))).utf8().get_data();
							char seps[] = ": -";
							char *token;
							token = strtok( &str[0], seps );

							while( token != NULL )
							{
								datando.push_back(atoi(token));
								token = strtok( NULL, seps );
							}

							row[sql2String(res_meta->getColumnName(j))] = datando;
							othertypes = true;
						}


						//Return any other datatpe as string
						if (othertypes == false){
							row[sql2String(res_meta->getColumnName(j))] = sql2String(res->getString(j));
						}

					}
					ret.push_back(row);
				}
			break;
			
			case 2:  //--fetch_dictinary_string		
			
				stmt = con->createStatement();
				res = stmt->executeQuery(SQLquery);
				res_meta = res->getMetaData();

				while (res->next())
				{
					Dictionary row;
					for (int j = 1; j <= res_meta->getColumnCount(); j++) {
						row[sql2String(res_meta->getColumnName(j))] = sql2String(res->getString(j));
					}

				ret.push_back(row);
				}
			break;
		
			case 3:  //--fetch_array_string		
			
				stmt = con->createStatement();
				res = stmt->executeQuery(SQLquery);
				res_meta = res->getMetaData();

				while (res->next())
				{
					Array linha;
					for (int j = 1; j <= res_meta->getColumnCount(); j++)       	    
					{            	           	    
						linha.push_back(sql2String(res->getString(j)));               
					}

				ret.push_back(linha);
				}       			
			break;

			case 4:  //--fetch_array		
			
				stmt = con->createStatement();
				res = stmt->executeQuery(SQLquery);
				res_meta = res->getMetaData();

				while (res->next())
				{
					Array linha;
					Array datando;
					for (int j = 1; j <= res_meta->getColumnCount(); j++)       	    
					{            	           	    

						String testype = sql2String(res_meta->getColumnTypeName(j));  
						bool othertypes = false;
						
						//Returns an Interger
						if (testype == "INT" || testype == "TINYINT" || testype == "SMALLINT"|| testype == "MEDIUMINT" || testype == "BIGINT" || testype == "INTEGER"){
		 					linha.push_back(res->getInt(j));
							othertypes = true;
						}

						//Returns strings
						if (testype == "VARCHAR"|| testype == "CHAR" || testype == "TINYTEXT" || testype == "TEXT" || testype == "MEIUMTEXT" || testype == "LONGTEXT" || testype == "LONGVARCHAR") {
							linha.push_back(sql2String(res->getString(j)));
							othertypes = true;
						}

						//Returns a float
						if (testype == "FLOAT" || testype == "DECIMAL" || testype == "REAL" || testype == "DOUBLE" || testype == "NUMERIC"){
							float floteando = res->getDouble(j);
							linha.push_back(floteando);
							othertypes = true;
						}
					
						//Returns boolean
						if (testype == "BOOL" || testype == "BOOLEAN" || testype == "BIT"){
							linha.push_back(res->getBoolean(j));
							othertypes = true;
						}

						//Returns an Interger Array 
						if (testype == "DATE"|| testype == "DATETIME"|| testype == "TIME"|| testype == "TIMESTAMP"|| testype == "YEAR") 
						{
							string str = (sql2String(res->getString(j))).utf8().get_data();
							char seps[] = ": -";
							char *token;
							token = strtok( &str[0], seps );

							while( token != NULL )
							{
								datando.push_back(atoi(token));
								token = strtok( NULL, seps );
							}

							linha.push_back(datando);
							othertypes = true;
						}

						//Return any other datatpe as string
						if (othertypes == false){
							linha.push_back(sql2String(res->getString(j)));
						}				    
					}
				ret.push_back(linha);
			}   				
			break;	
		
			case 5:  //--get_columns_name		
			
				stmt = con->createStatement();
				res = stmt->executeQuery(SQLquery);
				res_meta = res->getMetaData();

				for (int j = 1; j <= res_meta->getColumnCount(); j++)       	    
				{            	           	    
					ret.push_back(sql2String(res_meta->getColumnName(j)));
				}				
			break;		
		
			case 6:  //--get_column_types		
			
				stmt = con->createStatement();
				res = stmt->executeQuery(SQLquery);
				res_meta = res->getMetaData();

				for (int t = 1; t <= res_meta->getColumnCount(); t++)       	    
				{    
					ret.push_back(sql2String(res_meta->getColumnTypeName(t)));
				}				
			break;		
		
		} //-- end switch


		delete res;
		delete stmt;
		delete con;

	}
	catch (sql::SQLException &e) {
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
	
	return ret;
}


//---Staff

String MySQL::sql2String(sql::SQLString p_str)
{
	const char * c = p_str.c_str();
	String str = String::utf8((char *)c);
	return str;
}

void MySQL::_bind_methods()
{

	ClassDB::bind_method(D_METHOD("set_credentials","hostname","username","password"),&MySQL::set_credentials);
	ClassDB::bind_method(D_METHOD("select_database","database"),&MySQL::select_database);
	ClassDB::bind_method(D_METHOD("query","sql_query"),&MySQL::query);
	ClassDB::bind_method(D_METHOD("fetch_dictinary","sql_query"),&MySQL::fetch_dictinary);
	ClassDB::bind_method(D_METHOD("fetch_dictinary_string","sql_query"),&MySQL::fetch_dictinary_string);
	ClassDB::bind_method(D_METHOD("fetch_array_string","sql_query"),&MySQL::fetch_array_string);
	ClassDB::bind_method(D_METHOD("fetch_array","sql_query"),&MySQL::fetch_array);
	ClassDB::bind_method(D_METHOD("get_columns_name","sql_query"),&MySQL::get_columns_name);
	ClassDB::bind_method(D_METHOD("get_column_types","sql_query"),&MySQL::get_column_types);
}

MySQL::MySQL() {
}


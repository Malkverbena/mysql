extends Node

var mysql

var query = "select * from employers"   #---Enter your query here

var statement_delete_table = "DROP TABLE IF EXISTS `employers`"

var statement_delete_schema = "DROP DATABASE IF EXISTS`teste`"

var statement_create_database = "CREATE SCHEMA IF NOT EXISTS `teste`"

var statement_create_table = "CREATE TABLE IF NOT EXISTS employers (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, last_name VARCHAR(20), name VARCHAR(20), phone VARCHAR(20), birth DATE)"

var statement_feed_table = "INSERT INTO employers (id, last_name, name, phone, birth) VALUES (NULL, 'wick', 'john', '(777)777-7777', '1970-12-31')"

var statement_feed_table_prep = "INSERT INTO employers (id, last_name, name, phone, birth) VALUES (?,?,?,?,?)"

var Wenzel = [null,'Wenzel', 'Dashington','(777)777-7777', '1954-12-28']
var Johnny = [null ,'Pedd','Johnny','(000)000-0000', '1963-06-09']
var Dobert = [null ,'Ne Riro', 'Dobert ','(444)444-4444','1943-08-17']

func _ready():

	print("\n ---------------------------------------- Create Set \n")
	mysql = MySQL.new()
	#--- If your Mysql server is local, you can use the ip of your server, "localhoslt" or "127.0.0.1"
	mysql.set_credentials( "127.0.0.1", "root", "mypass") 
	
	print( "Start: ", mysql.connection_start() )  #--This step is not really necessary couse the module starts automatically the connection when a query is made.
	
	#---Make yours Inserts and others queries here
	#---execute() returns an INT with the amount rows changed
	print( "changed rows: ", mysql.query_execute(statement_create_database) )
	print( "Set_database: ", mysql.set_database("teste")  )
	print( "changed rows: ", mysql.query_execute(statement_create_table) )
	print( "changed rows: ", mysql.query_execute(statement_feed_table) )

	print("\n ---------------------------------------- Tests \n")
	
	#---Start, check and close connection
	#---Setting and getting Database
	print( "Checking Connection0: ", mysql.connection_check() )
	print( "Set_database:", mysql.set_database("teste")  )
	print( "get_database:",  mysql.get_database() )
	print( "close: ", mysql.connection_close() )
	print( "Checking Connection1: ", mysql.connection_check() )

	print("\n ---------------------------------------- Fetch \n")

	print( "Start: ", mysql.connection_start() )
	print( "Checking Connection3: ", mysql.connection_check() )
	print( "Set_database:", mysql.set_database("teste")  )
	print( "get_database: ", mysql.get_database() )

	#---fetching query as dictinary. 
#	Returns an array of dictionaries containing the query result.
#	Every dictionaries represents a row in the done query.
#	If return_string is FALSE, the returned data has the same datatype of the saved data on SQL database, otherwise this will return the data as Strings
#	Otherwise all the data gonna be returned as Strings.

	var lo = mysql.query_fetch_dictionary(query, false)  
	for fk in range(lo.size()):
		print(lo[fk])
	print("\n")
	

	#---fetching query as array
#	Returns a two-dimensional array containing the query result.
#	if return_string is FALSE, the returned data has the same datatype of the saved data on SQL database, otherwise this will return the data as Strings

	var fj = mysql.query_fetch_array(query, false)
	for tk in range(fj.size()):
		print(fj[tk])
#
	#---Return the columns names, as string of course
	print(mysql.query_get_columns_names(query),"\n ")
#
	#---Return the columns types, as strings too
	print(mysql.query_get_columns_types(query),"\n ")
	
	print("\n ---------------------------------------- Prepared Fetch \n")

	"""Parameters in prepared statements are for data values ONLY -- not object 
	identifiers or any other part of a SQL statement. Just data values."""

	print( "changed rows: ", mysql.prep_execute(statement_feed_table_prep, Wenzel) )
	print( "changed rows: ", mysql.prep_execute(statement_feed_table_prep, Johnny) )
	print( "changed rows: ", mysql.prep_execute(statement_feed_table_prep, Dobert) )  

	print( mysql.prep_fetch_array("SELECT * FROM `employers` WHERE id < 3", [],false) )

	print("\n ---------------------------------------- Extras \n")

	#---Rows and Columns count can be done this way
	print("Rows: ",lo.size(),"  -  Columns: " ,lo[0].size())

	print( "changed rows: ", mysql.query_execute(statement_delete_table) )

	print( "changed rows: ", mysql.query_execute(statement_delete_schema) )

	get_tree().quit()

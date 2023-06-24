extends Node



func _ready():
	var mysql = MySQL.new()
#	mysql.set_credentials( "127.0.0.1",  "My usar name", "my pass")
	mysql.set_credentials( "127.0.0.1", "cleber", "jeovaerei") 
	var SQLexception = mysql.start_connection()


#	If the Exception is empty it means the connections is successful
	if not SQLexception.empty():
		get_tree().quit()

#	Simple execute.
	mysql.execute("DROP DATABASE IF EXISTS test")
	var db_success : bool = mysql.execute("CREATE DATABASE IF NOT EXISTS test")
	print("Database was successfully created: = ", db_success)


#	Select database
	mysql.set_property("schema", "test")

	mysql.execute("DROP TABLE IF EXISTS data")
	mysql.execute("CREATE TABLE IF NOT EXISTS data ( Id INT NOT NULL AUTO_INCREMENT, Name VARCHAR(45) NOT NULL, Age INT(3) NOT NULL, PRIMARY KEY (Id))") 

#	Get table columns info
	var meta = mysql.query("SHOW COLUMNS FROM `test`.`data`")
	for i in meta:
		print(i)

#	Using statement
	var afected_rows = mysql.update("INSERT INTO data (Name, Age) VALUES ('John', 36), ('Luke', 40), ('Tom', 25)")
	print("afected_rows: ", afected_rows)

#	Show table content
	var _data = mysql.query("SELECT * from data")
	for i in _data:
		print(i)


	mysql.stop_connection()
	get_tree().quit()

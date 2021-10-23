extends Node



func _ready():
	var mysql = MySQL.new()
	mysql.set_credentials( "127.0.0.1",  "My usar name", "my pass")
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

	print(mysql.execute("DROP TABLE IF EXISTS data"))
	mysql.execute("CREATE TABLE IF NOT EXISTS data ( Id INT NOT NULL AUTO_INCREMENT, Name VARCHAR(45) NOT NULL, Age INT(3) NOT NULL, PRIMARY KEY (Id))") 

#	Using prepared statement
	var stmt : String = "INSERT INTO data (Name, Age) VALUES (?,?)"
	var value : Array = ["John", "36"]
	mysql.execute_prepared(stmt, value)


	mysql.stop_connection()
	get_tree().quit()

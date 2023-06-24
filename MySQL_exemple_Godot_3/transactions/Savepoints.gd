extends Node

var mysql = MySQL.new()

func _ready():
	mysql.set_credentials( "127.0.0.1", "cleber", "jeovaerei") 
	mysql.start_connection()


#	It's not possible create savepoint if Auto commint is set to true
	mysql.setAutoCommit(false)  


	mysql.create_savepoint("save_point_1")
	mysql.create_savepoint("save_point_2")
	print("Created Savepoints: ", mysql.get_savepoints())
	mysql.delete_savepoint("save_point_1")
	print("Created Savepoints: ", mysql.get_savepoints())
	get_tree().quit()

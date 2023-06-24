extends Node

func _ready():
	var mysql = MySQL.new()

	mysql.set_credentials( "127.0.0.1", "cleber", "jeovaerei") 
	mysql.start_connection()

	# Create database
	mysql.execute("DROP DATABASE IF EXISTS test")
	mysql.execute("CREATE DATABASE IF NOT EXISTS test")
	mysql.set_property("schema", "test")


	mysql.execute("DROP TABLE IF EXISTS atomic")

	# Create table with atomic types
	var statement : String = """CREATE TABLE `test`.`atomic` (
	`interger` INT NULL,
	`string` VARCHAR(45) NULL,
	`float` FLOAT NULL,
	`null` INT NULL,
	`boolean` BOOLEAN NULL)"""
	mysql.execute(statement)








	get_tree().quit()

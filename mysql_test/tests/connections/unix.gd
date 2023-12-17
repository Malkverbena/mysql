extends Node


var mysql = MySQL.new()

func _ready():

	print(" ======= UNIX ======= ")

	if mysql.define(MySQL.UNIX):
		db.print_exception(mysql.get_last_error())
	if mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries):
		db.print_exception(mysql.get_last_error())
	if mysql.unix_connect():
		db.print_exception(mysql.get_last_error())

	print("is connected: ", mysql.is_server_alive())

	var result = mysql.execute("SELECT * FROM testes.basic_query;")
	print("======")
	db.print_digest(result)
	db.print_exception(mysql.get_last_error())
	db.print_result(result)
	

	get_tree().quit()

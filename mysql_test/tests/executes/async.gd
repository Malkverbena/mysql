extends Node


var mysql = MySQL.new()

func _ready():

	print(" ======= TCP ASYNC ======= ")
	if mysql.define(MySQL.TCP):
		db.print_exception(mysql.get_last_error())
	if mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries):
		db.print_exception(mysql.get_last_error())
	if mysql.tcp_connect(db.hostname, db.port, true):
		db.print_exception(mysql.get_last_error())
	
	print("IS ASYNC: ", mysql.is_async())
	print("is connected: ", mysql.is_server_alive())

	print("BEFORE EXECUTE @@@@@@@@@@")
	var result = mysql.async_execute("SELECT * FROM testes.basic_query")
	print("AFTER EXECUTE @@@@@@@@@@@")
	print("======")
	db.print_digest(result)
	db.print_exception(mysql.get_last_error())
	db.print_result(result)




	get_tree().quit()

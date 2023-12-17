extends Node

var mysql = MySQL.new()

func _ready():

	print(" ======= EXECUTE FILE ======= ")

	var define_err : Error = mysql.define(MySQL.TCP)
	if define_err:
		db.print_exception(mysql.get_last_error())
	var set_credentials_err : Error = mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries)
	if set_credentials_err:
		db.print_exception(mysql.get_last_error())

	var tcp_connect_err : Error = mysql.tcp_connect(db.hostname, db.port)
	print("\ntcp_connect ERROR: ",  tcp_connect_err)
	
	var sql_file = "res://sql/create_database.sql"
	

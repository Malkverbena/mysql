extends Node


var mysql = MySQL.new()


func _ready():
	#================================================================================================
	print(" ======= PREPARETED ======= \n\n")
	print("DEFINE ERROR: ", mysql.define(MySQL.TCP))
	print("CREDENTIALS ERROR: ", mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries))
	print("CONNECT ERROR: ", mysql.tcp_connect(db.hostname, db.port))

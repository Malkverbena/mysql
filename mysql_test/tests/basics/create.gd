
#	The function of this script is to create and configure a database for testing.


extends Node

var mysql = MySQL.new()


func _ready():
	print("DEFINE ERROR: ", mysql.define(MySQL.TCP))
	print("CREDENTIALS ERROR: ", mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries))
	print("CONNECT ERROR: ", mysql.tcp_connect(db.hostname, db.port))

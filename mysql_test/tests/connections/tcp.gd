extends Node

var mysql = MySQL.new()

func _ready():
	mysql.new_connection("tcp")

	print("\n\nTCP====================")
	mysql.set_credentials("tcp", db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries)
	mysql.tcp_connect("tcp", db.hostname, db.port)
	print(mysql.get_credentials("tcp"))

	var tcp_res : SqlResult = mysql.execute("tcp", "SELECT * FROM testes.text01")
	var tcp_arr = tcp_res.get_array()
	for a in tcp_arr:
		print(a)


	get_tree().quit()


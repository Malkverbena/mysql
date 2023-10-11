extends Node


var mysql = MySQL.new()

func _ready():
	mysql.new_connection("unix")

	print("\n\nUNIX====================")
	mysql.set_credentials("unix", db.username, db.password, db.database, db.collation, SqlCollations.disable, db.multi_queries)
		#FIX THE CRASH WHEN USING WRONG CONNECTION TYPE
	mysql.unix_connect("unix", db.unix_socket)
	print(mysql.get_credentials("unix"))

	var unix_res : Ref<SqlResult> = mysql.execute("unix", "SELECT * FROM testes.text01")
	var unix_arr = unix_res.get_array()
	for a in unix_arr:
		print(a)

	get_tree().quit()

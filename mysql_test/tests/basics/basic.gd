extends Node

var mysql = MySQL.new()

func _ready():
	
	mysql.new_connection("marco")
	mysql.new_connection("polo")
	print(mysql.get_connections())
	mysql.delete_connection("marco")
	print(mysql.get_connections())
	
	mysql.set_credentials("polo", db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries)
	mysql.tcp_connect("polo", db.hostname, db.port)
	print(mysql.get_credentials("polo"))

	var res : SqlResult = mysql.execute("polo", "SELECT * FROM testes.text01")
	
	var metadata = res.get_metadata()
	for meta in metadata:
		for m in metadata[meta]:
			print(m, "=", metadata[meta][m])
	
	
	print("get_last_insert_id: ", res.get_last_insert_id())  
	print("get_affected_rows: ", res.get_affected_rows())
	print("get_warning_count: ", res.get_warning_count())
	
	var dic = res.get_dictionary()
	for row in dic: 
		print(dic[row])

	var arr = res.get_array()
	for a in arr:
		print(a)

	get_tree().quit()

extends Node

var mysql = MySQL.new()

func _ready():

	print(" ======= TCP ======= ")

	var define_err : Error = mysql.define(MySQL.TCP)
	if define_err:
		db.print_exception(mysql.get_last_error())
	var set_credentials_err : Error = mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries)
	if set_credentials_err:
		db.print_exception(mysql.get_last_error())

	var tcp_connect_err : Error = mysql.tcp_connect(db.hostname, db.port, false)
	print("\ntcp_connect ERROR: ",  tcp_connect_err)
	if tcp_connect_err:
		print("Last Error from tcp_connect:\n")
		db.print_exception(mysql.get_last_error())
	print(mysql.is_server_alive())
	db.print_exception(mysql.get_last_error())
	
	print("\n\n")
	
	var res : SqlResult = mysql.execute("SELECT * FROM testes.teste")
	db.print_exception(mysql.get_last_error())
	if res:
		print("get_last_insert_id: ", res.get_last_insert_id())  
		print("get_affected_rows: ", res.get_affected_rows())
		print("get_warning_count: ", res.get_warning_count())
		var metadata = res.get_metadata()
		for meta in metadata:
			print("\n", metadata[meta])
			#for m in metadata[meta]:
				#print(m, "=", metadata[meta][m])

	print("\n\n")

	var dic = res.get_dictionary()
	for row in dic: 
		print(dic[row])
#
	#var arr = res.get_array()
	#for a in arr:
		#print(a)


	mysql.close_connection()
	get_tree().quit()


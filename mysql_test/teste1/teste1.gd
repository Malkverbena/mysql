extends Node

var mysql = MySQL.new()

func _ready():
	
	#mysql.new_connection("marco")
	mysql.new_connection("polo")
	print(mysql.get_connections())
	#mysql.delete_connection("marco")
	#print(mysql.get_connections())
	
	mysql.set_credentials("polo", "cleber", "jeovaerei", "testes")
	mysql.tcp_connect("polo", "localhost", "3306")
	print(mysql.get_credentials("polo"))

	var res : SqlResult = mysql.execute("polo", "SELECT * FROM testes.text01")
	
	#var metadata = res.get_metadata()
	#for meta in metadata:
		#for m in metadata[meta]:
			#print(m, "=", metadata[meta][m])
	
	
	print("get_last_insert_id: ", res.get_last_insert_id())  
	print("get_affected_rows: ", res.get_affected_rows())
	print("get_warning_count: ", res.get_warning_count())
	
	#print("\n\n")
	print(res.get_array())
	print("\n\n")
	print(res.get_dictionary())

	#var dic = res.get_dictionary()
#	print(typeof(dic))
	#for row in dic: 
		#print(dic[row])

	#print(dic)

	#var arr = res.get_array()
	#for a in arr:
		#print(a)

	get_tree().quit()

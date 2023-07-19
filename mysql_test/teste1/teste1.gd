extends Node

var mysql = MySQL.new()

func _ready():
	
#	mysql.new_connection("marco")
	mysql.new_connection("polo")
	print(mysql.get_connections())
	mysql.sql_disconnect("polo",)
#	mysql.delete_connection("marco")
#	print(mysql.get_connections())
	
	#mysql.set_credentials("polo", "cleber", "jeovaerei", "testes")
	mysql.sql_connect("polo", "localhost", "3306")
	print(mysql.get_credentials("polo"))

	var res : SqlResult = mysql.execute("polo", "SELECT * FROM testes.text01")
	
	var metadata = res.get_metadata()
	for m in metadata:
		print(metadata[m])

#	print(res.get_array())

	var dic = res.get_dictionary()
#	print(typeof(dic))
	for row in dic: 
		print(dic[row])

	print(dic)

	var arr = res.get_array()
	for a in arr:
		print(a)

	get_tree().quit()

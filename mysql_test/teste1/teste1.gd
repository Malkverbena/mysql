extends Node

var mysql = MySQL.new()

func _ready():
	var mysql = MySQL.new()
	
	mysql.new_connection("tell")
	print(mysql.get_connections())
#	get_tree().quit()

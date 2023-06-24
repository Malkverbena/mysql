extends Node

"""
More about conection 
"""

func _ready():

	var mysql = MySQL.new()

#	This is a fest way to set a connection
	mysql.set_credentials( "127.0.0.1",  "My usar name", "my pass")

	mysql.start_connection() 

	if mysql.get_connection_status() == MySQL.CONNECTED:

#		Get Metada of Connection
		var metadata : Dictionary = mysql.get_metadata()

		var meta_key : Array = metadata.keys()
		var meta_value : Array = metadata.values()
		for i in metadata.size():
			print( "Metadata: %s Value: %s" % [meta_key[i], meta_value[i]])


	mysql.stop_connection()

	get_tree().quit()

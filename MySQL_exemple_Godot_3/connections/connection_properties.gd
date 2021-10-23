extends Node

"""
This script show how get and set the connection properties
For more information visit: https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-connect-options.html"
"""

func _ready():

	var mysql = MySQL.new()

#	This is a fest way to set a connection
	mysql.set_credentials( "127.0.0.1",  "My usar name", "my pass")

	mysql.start_connection()

	if mysql.get_connection_status() == MySQL.CONNECTED:

#		Set a property by it name. In this case is selecting the database
		mysql.set_property("schema", "test")

#		Get a property by it name
		print("The data Base selected is: ", mysql.get_property("schema"))

		print("The value of 'OPT_RECONNECT' is: ", mysql.get_property("OPT_RECONNECT"))

		print("The value of 'CLIENT_MULTI_STATEMENTS' is:  ", mysql.get_property("CLIENT_MULTI_STATEMENTS"))


	mysql.stop_connection()

	get_tree().quit()

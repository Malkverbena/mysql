extends Node

"""
This script show how get and set the connection properties
For more information visit: https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-connect-options.html"
"""

func _ready():

	var mysql = MySQL.new()

#	This is a fest way to set a connection
#	mysql.set_credentials( "127.0.0.1",  "My usar name", "my pass")
	mysql.set_credentials( "127.0.0.1", "cleber", "jeovaerei") 

	mysql.start_connection()

	if mysql.get_connection_status() == MySQL.CONNECTED:

#		Set a property by it name. In this case is selecting the database
		mysql.set_property("schema", "test")

#		Get a property by it name
		print("The data Base selected is: ", mysql.get_property("schema"))
		print("The value of 'OPT_RECONNECT' is: ", mysql.get_property("OPT_RECONNECT"))
		print("The value of 'CLIENT_MULTI_STATEMENTS' is:  ", mysql.get_property("CLIENT_MULTI_STATEMENTS"))


#		Set many properties at once
		var properties := {"OPT_RECONNECT":false, "CLIENT_COMPRESS":true, "CLIENT_FOUND_ROWS":true}
		mysql.set_properties_set(properties)


#		Get many properties at once
		var credentials : Dictionary = mysql.get_properties_set(["hostName", "userName", "password", "port"])
		print("\n Credentials properties:\n", credentials)


#		Don't try set a connections at once using "get_properties_set" It won't work becouse those properties are not set at ConnectOptionsMap yet
		var other_properties  : Dictionary = mysql.get_properties_set(["OPT_RECONNECT", "CLIENT_COMPRESS", "CLIENT_FOUND_ROWS", "port"])
		print("\n Other properties:\n", other_properties)

	mysql.stop_connection()

	get_tree().quit()

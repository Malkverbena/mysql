extends Node

"""
This script shows how to connect with the data base.
"""


func _ready():
	var mysql = MySQL.new()

#	Set the host name (IP) to be connected.
	mysql.set_property("hostName", "127.0.0.1")

#	Set the sser name.
	mysql.set_property("userName", "My usar name")

#	Set the password.
	mysql.set_property("password", "My password")

##	Select the database.
#	mysql.set_property("schema", "teste")

#	Start the connection with the properties setted above.
#	Returns a dictionary with detailed information about connection errors or an empty dictionary if the connection was successful.
	var state = mysql.start_connection()
	print( "Connection exception: ",  state)

#	Return the current state of Connection. See MySQL.ConnectionStatus.
	print("Connection current state: ", mysql.get_connection_status())

#	Closes the connection and returns it current state.
	mysql.stop_connection()


	get_tree().quit()

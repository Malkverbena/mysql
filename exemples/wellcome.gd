extends Node

"""
Run this first scene to creates and set a new database for test this exemples.

Remember to edit the file 'db_utils.gd' with your credentials to be able to access your database.

Execute populate scene to populate the tables of our schema. 
"""

# Creates a new instance of MySQL.
var mysql = MySQL.new()



func _ready():
	print("  =======  Wellcome to the Godot MySQL module setup scene!!  =======")

	# Defines the connection. 
	# Here you can choose the type of connection that will be used to connect with the database
	# You also can set a certificate to use TLS.
	# You also can use this method to redefine and reset the connection. 
	# In non-TLS connections the certificate and the hostname will be ignored if you set it.
	# If you want use another certificate you  must redefine the connection.
	# Resetting the connection does not reset the credentials. Be alware!
	var define_err : Error = mysql.define(MySQL.TCP)


	# Let's handle possible errors!
	# I lovingly prepared a singleton todo so. (Check it out: db_utils.gd)
	if define_err:
		# After all mysql operations you can check if generated an error or exception.
		# Check it just after you call the function. The last_error will be apdates after each function called.
		db.print_exception(mysql.get_last_error())


	# Let's set the credeltials!
	# Once again, edit it on db_utils.gd.
	var set_credentials_err : Error = mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries)
	# Check if everyting was good!
	if set_credentials_err:
		db.print_exception(mysql.get_last_error())

	# Now we finally can connect with the database.
	# If you define the connection as TCP or TCPTLS, use the function 'tcp_connect'.
	var tcp_connect_err : Error = mysql.tcp_connect(db.hostname, db.port)
	# Checking possible erros again.
	if tcp_connect_err:
		db.print_exception(mysql.get_last_error())

	# Let's check if the connection is alive!
	print("\nCONNECTED TO SERVER: %s\n" % [mysql.is_server_alive()])

	# Now let's execute the file .sql that will create our schema.
	var file : String = "res://sql_scripts/create_database.sql"
	# This function will return an array with a SqlResult for each sql instruction executed.
	# Remember, this method works only if multi_queries=true in credentials.
	mysql.execute_sql(file)
	
	# To know if the operation was successful just check if the last_error is empty.
	if not mysql.get_last_error().is_empty():
		db.print_exception(mysql.get_last_error())
		printerr("\nSORRY, WAS NOT POSSIBLE CREATE OUR SCHEMA!!!")
	# once it was successfully executed, let's print the multiset result.
	else:
		print("\nSCHEMA SUCCESSFULY CREATED!!!")
	

	# That's all folks!
	get_tree().quit()



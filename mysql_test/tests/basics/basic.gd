extends Node

var mysql = MySQL.new()

func _ready():

	var define_err : Error = mysql.define(MySQL.TCP)
	print("\ndefine ERROR: ",  define_err)
	if define_err:
		print("Last Error from connect:")
		db.print_error(mysql.get_last_error())
	
	
	var set_credentials_err : Error = mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries)
	print("\nset_credentials ERROR: ",  set_credentials_err)
	if set_credentials_err:
		print("Last Error from set_credentials: ")
		db.print_error(mysql.get_last_error())
	
	
	print("\nGET CREDENTIALS:",  mysql.get_credentials())
	print("\nGET CONNECTION TYPE: ",  mysql.get_connection_type())
	print("\nIS ASYNC: ",  mysql.is_async())
	
	
	
	var tcp_connect_err : Error = mysql.tcp_connect(db.hostname, db.port)
	print("\ntcp_connect ERROR: ",  tcp_connect_err)
	if tcp_connect_err:
		print("Last Error from tcp_connect:\n")
		db.print_error(mysql.get_last_error())
	
	print("\nis_server_alive: ", mysql.is_server_alive())

	var close_connection_err : Error = mysql.close_connection()
	print("\nclose_connection ERROR: ",  close_connection_err)
	if close_connection_err:
		print("Last Error from close_connection:\n")
		db.print_error(mysql.get_last_error())
	
	get_tree().quit()

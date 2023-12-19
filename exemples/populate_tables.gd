extends Node


var mysql = MySQL.new()

func _ready():


	# You already know how this works.
	var define_err : Error = mysql.define(MySQL.TCP)
	if define_err:
		db.print_exception(mysql.get_last_error())
	var set_credentials_err : Error = mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries)
	if set_credentials_err:
		db.print_exception(mysql.get_last_error())
	var tcp_connect_err : Error = mysql.tcp_connect(db.hostname, db.port)
	if tcp_connect_err:
		db.print_exception(mysql.get_last_error())
	print("\nCONNECTED TO SERVER: %s\n" % [mysql.is_server_alive()])


	var statement : String =\
	"""
	"""
	var values : Array = [
		"Arthur", {"day":14, "month":10, "year":1975}, 1, 53.4,
		"Susan", {"day":30, "month":03, "year":1983}, 1, 75.4,
	]


	var result : SqlResult = mysql.execute_prepared(statement, values)







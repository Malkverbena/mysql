extends Node

var mysql = MySQL.new()


func _ready():
	#================================================================================================
	print(" ======= PREPARETED ======= \n\n")
	print("DEFINE ERROR: ", mysql.define(MySQL.TCP))
	print("CREDENTIALS ERROR: ", mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries))
	print("CONNECT ERROR: ", mysql.tcp_connect(db.hostname, db.port, false))


	#================================================================================================
	print("\n\n *** INSERT ***")
	var stmt = """
		INSERT INTO prepareted_statement 
		(int_data, string_data, float_data, bool_data) VALUES
		(?, ?, ?, ?), (?, ?, ?, ?)
	"""
	var values = [
			3, 'BBB', 2.22, true,
			34, 'CCC', 3.33, false,
		]
	
	var result : SqlResult = mysql.execute_prepared(stmt, values)
	db.print_digest(result)
	db.print_exception(mysql.get_last_error())
	
	var query = "SELECT * FROM prepareted_statement"
	var query_result : SqlResult = mysql.execute_prepared(query)
	db.print_result(query_result)


	#================================================================================================
	print("\n\n *** UPDATE *** ")
	stmt = \
	"""
		UPDATE prepareted_statement 
		SET int_data=?, string_data=?, float_data=?, bool_data=?
		WHERE id=1
	"""
	values = [3000, 'ABCABCABC', 10.01, false]
	result = mysql.execute_prepared(stmt, values)
	db.print_digest(result)
	db.print_exception(mysql.get_last_error())
	
	query = "SELECT * FROM prepareted_statement"
	query_result  = mysql.execute_prepared(query)
	db.print_result(query_result)


	#================================================================================================
	get_tree().quit()

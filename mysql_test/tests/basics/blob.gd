extends Node


var mysql = MySQL.new()
var sample = {"bb":"abcdefgh", "324":123}
var blob : PackedByteArray = var_to_bytes(sample)

func _ready():

	#================================================================================================
	print(" ======= BLOB BLOB ======= \n\n")
	print(blob, "\n")
	print("DEFINE ERROR: ", mysql.define(MySQL.TCP))
	print("CREDENTIALS ERROR: ", mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries))
	print("CONNECT ERROR: ", mysql.tcp_connect(db.hostname, db.port))
	print("STATUS: ", mysql.is_server_alive())
	
	var stmt = "INSERT INTO testes.binary (bin) VALUES (?)"
	
	var values = [blob]

	var result : SqlResult = mysql.execute_prepared(stmt, values)
	db.print_digest(result)
	db.print_exception(mysql.get_last_error())
	
	print("\n\n")
	result = mysql.execute("SELECT * FROM testes.binary")
	db.print_digest(result)
	db.print_result(result)
	
	var god_res = result.get_array()[1][1]
	print("Returned type: ", typeof(god_res) == TYPE_PACKED_BYTE_ARRAY)
	var ret = bytes_to_var(god_res)
	print("\n\nMatch: ", ret == sample, " - ",ret)

	#================================================================================================
	get_tree().quit()

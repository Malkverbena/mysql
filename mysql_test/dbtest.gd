extends Node

var username : String = "cleber"
var password : String = "jeovaerei"
var database : String = "testes"
var collation : MySQL.MysqlCollations = MySQL.default_collation
var ssl: MySQL.SslMode = MySQL.ssl_enable
var multi_queries : bool = false


var unix_socket : String = "/var/run/mysqld/mysqld.sock"
var hostname : String = "localhost"
var port : String = "3306"



func print_exception(err: Dictionary) -> void:
	if not err.is_empty():
		for e in err.size():
			print(err.keys()[e], " -> ", err.values()[e])
	else:
		print("NO ERRORS!!")



func print_metadata(result: SqlResult) -> void:
	if not result:
		print("EMPTY SQL RESULT")
		return
	var metadata : Dictionary = result.get_metadata()
	for meta in metadata:
		for m in metadata[meta]:
			print(m, "=", metadata[meta][m])


func print_result(result: SqlResult) -> void:
	print()
	if not result:
		print("\nEMPTY SQL RESULT")
		return
	var res : Dictionary = result.get_dictionary()
	for row in res:
		print(res[row])



func print_digest(result: SqlResult, full: bool = true) -> void:
	if result:
		if full:
			print("METADATA: ", result.get_metadata())
			print("CONTENT: ", result.get_dictionary())
		print("LAST INSERTED ID: ", result.get_last_insert_id())
		print("AFFECTED ROWS: ", result.get_affected_rows())
		print("WARNING COUNT: ", result.get_warning_count())
	else:
		print("EMPTY SQL RESUL!!")



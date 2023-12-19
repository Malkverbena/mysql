extends Node


var username : String = "cleber"
var password : String = "jeovaerei"
var database : String = ""
var collation : MySQL.MysqlCollations = MySQL.default_collation
var ssl: MySQL.SslMode = MySQL.ssl_enable
var multi_queries : bool = true


var unix_socket : String = "/var/run/mysqld/mysqld.sock"
var hostname : String = "localhost"
var port : String = "3306"



func print_exception(err: Dictionary) -> void:
	if not err.is_empty():
		for e in err.size():
			print(err.keys()[e], " -> ", err.values()[e])
	else:
		print("NO ERRORS!!")



func print_result(result: SqlResult) -> void:
	var res : Dictionary = result.get_dictionary()
	if res.is_empty():
		print("\nEMPTY SQL RESULT")
		return
	for row in res:
		print(res[row])


func print_metadata(result: SqlResult) -> void:
	var metadata : Dictionary = result.get_metadata()
	if metadata.is_empty():
		print("EMPTY METADATA")
		return
	for meta in metadata:
		for m in metadata[meta]:
			print(m, "=", metadata[meta][m])


func print_multiset_result(multiresult: Array) -> void:
	for r in multiresult:
		print_result(r)
		print_metadata(r)
		










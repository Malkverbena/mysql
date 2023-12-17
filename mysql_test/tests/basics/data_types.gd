extends Node

var mysql = MySQL.new()

func _ready():
	var mat = {"aa": "asfasdv", "324":123}
	var t : PackedByteArray = var_to_bytes(mat)

	
	
	#================================================================================================
	print(" ======= DATA TYPES ======= \n\n")
	print("DEFINE ERROR: ", mysql.define(MySQL.TCP))
	print("CREDENTIALS ERROR: ", mysql.set_credentials(db.username, db.password, db.database, db.collation, db.ssl, db.multi_queries))
	print("CONNECT ERROR: ", mysql.tcp_connect(db.hostname, db.port))


	var stmt = \
	"""
	INSERT INTO testes.data_types 
	(t_string, t_bool, t_real, t_double, t_blob, t_set, t_null, t_time, t_date, t_datetime) 
	VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
	"""
	
	var values = [
		"HAHAHAHAH",
		true,
		10000.9999,
		2000.0002,
		t,
		"AAAA,CCCC,EEEE",
		null,
		{"hour": 17, "minute":07, "second":23},
		{"day":04 ,"month":01, "year":1985},
		{"hour": 13, "minute":33, "second":47, "day":17 ,"month":07, "year":2017},
	]
	
	var result : SqlResult = mysql.execute_prepared(stmt, values)
	db.print_exception(mysql.get_last_error())
	db.print_digest(result)
	

	print("---------------------\n\n")
	var q : SqlResult = mysql.execute("SELECT * FROM data_types")
	db.print_exception(mysql.get_last_error())
	db.print_digest(q, false)
	var ret : Dictionary = q.get_dictionary()
	for row in ret:
		print()
		for col in ret[row]:
			if (typeof(ret[row][col]) == TYPE_PACKED_BYTE_ARRAY):
				print("%s: = %s" % [col, bytes_to_var(ret[row][col])])
			elif (typeof(ret[row][col]) == TYPE_PACKED_STRING_ARRAY):
				print("%s: = %s  *** PackedStringArray" % [col, ret[row][col]])
			elif (typeof(ret[row][col]) == TYPE_DICTIONARY):
				var time_string = Time.get_datetime_string_from_datetime_dict(ret[row][col], true)
				print("%s : %s => = %s" % [col, ret[row][col], time_string])
			else:
				print("%s: = %s" % [col, ret[row][col]])
	#
	#================================================================================================
	mysql.close_connection()
	get_tree().quit()

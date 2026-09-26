# tests/smoke_test.gd
#
# Local integration test against a real MySQL/MariaDB server. It is not a unit test: it
# covers the end-to-end path of every exposed class against a real database. Run it with:
#
#   godot --headless --script tests/smoke_test.gd
#
# from the Godot tree already compiled with this module (custom_modules=../mysql).
#
# Credentials come from environment variables, never from this file (do not commit any
# secret):
#   MYSQL_TEST_HOST (default 127.0.0.1), MYSQL_TEST_PORT (default 3306),
#   MYSQL_TEST_USER, MYSQL_TEST_PASSWORD, MYSQL_TEST_DATABASE (required).
#
# Android has no shell environment to inherit, and an exported activity strips
# command-line Intent extras by default (GodotActivity.shouldSanitizeLaunchIntent()), so
# on that platform the same names are instead read from KEY=VALUE lines of a file at
# user:// (getFilesDir(), the app's private internal storage — needs `run-as` to reach
# from adb, since it is not externally writable), e.g., for a debug-signed/debuggable
# export:
#   adb push credentials.txt /data/local/tmp/test_credentials.txt
#   adb shell run-as <package> cp /data/local/tmp/test_credentials.txt files/test_credentials.txt
# Environment variables are tried first everywhere, so desktop/Wine usage is unchanged.
#
# It creates and drops its own table (t_mysql_module_smoke_test) in the given schema and
# touches nothing else in the database.
#
# class_name lets this script also run as a packaged app's main loop (Project Settings ->
# Application -> Run -> Main Loop Type, with any minimal Main Scene set). This is how the
# Android smoke test runs it: export templates ignore `--script` by default. Godot's
# `disable_path_overrides` build option (on by default since godotengine/godot#111909)
# clears `--script`, `--main-loop`, `--path`, `--scene` and `--main-pack` at startup in a
# template build, without a warning, while the Main Loop Type project setting is still
# read. See notes/android_smoke_project/ in the mysql_dev workspace (outside this
# repository).

extends SceneTree

class_name MySQLSmokeTest

const TABLE := "t_mysql_module_smoke_test"

var failures: int = 0
var checks: int = 0


func check(condition: bool, description: String) -> void:
	checks += 1
	if condition:
		print("  OK   ", description)
	else:
		failures += 1
		print("  FAIL ", description)


func fail_and_quit(message: String) -> void:
	printerr(message)
	quit(1)


# Cached on first call: the KEY=VALUE lines of user://test_credentials.txt, if present
# (the Android fallback; see the file header). Parsed once, not once per credential.
var _credentials_file: Dictionary
var _credentials_file_read: bool = false


func _read_credentials_file() -> Dictionary:
	if _credentials_file_read:
		return _credentials_file
	_credentials_file_read = true
	var file := FileAccess.open("user://test_credentials.txt", FileAccess.READ)
	if file == null:
		return _credentials_file
	while not file.eof_reached():
		var line := file.get_line()
		var separator := line.find("=")
		if separator > 0:
			_credentials_file[line.substr(0, separator)] = line.substr(separator + 1)
	return _credentials_file


# Environment variables first (desktop/Wine); falls back to user://test_credentials.txt
# (Android, which has no shell environment to inherit and strips Intent command-line args
# from an exported activity by default).
func read_credential(name: String) -> String:
	var value := OS.get_environment(name)
	if not value.is_empty():
		return value
	return _read_credentials_file().get(name, "")


func make_config(host: String, port: int, user: String, password: String, database: String) -> MySQLConfig:
	var config := MySQLConfig.new()
	config.host = host
	config.port = port
	config.user = user
	config.set_password(password)
	config.database = database
	return config


# Reads one session-scoped status counter, for example Com_stmt_prepare.
func session_status(session: MySQLSession, name: String) -> int:
	var result: MySQLResult = session.execute_text("SHOW SESSION STATUS LIKE '%s'" % name)
	if not result.is_ok() or result.get_rows().is_empty():
		return -1
	return int(result.get_rows()[0][1])


# Waits for an asynchronous operation, and fails the whole run instead of hanging forever
# if it never finishes (for example if the I/O thread stalled).
func await_operation(operation: MySQLAsyncOperation, timeout_seconds: float = 10.0) -> MySQLResult:
	if operation.is_finished():
		return operation.get_result()
	create_timer(timeout_seconds).timeout.connect(_on_operation_timeout.bind(operation))
	return await operation.completed


func _on_operation_timeout(operation: MySQLAsyncOperation) -> void:
	if not operation.is_finished():
		printerr("smoke_test: an asynchronous operation did not finish in time.")
		quit(1)


# Waits (a frame at a time, up to about 5 seconds) until the session accepts a query again,
# for a background drain to finish. Returns whether it did.
func _wait_until_usable(session: MySQLSession) -> bool:
	for i in range(300):
		if session.execute_text("SELECT 1").is_ok():
			return true
		await process_frame
	return false


# Runs in a worker thread: leases a session from the pool, runs one query and gives the
# session back (it is released when the function returns). Returns 1 on success.
func _pool_worker(pool: MySQLPool, index: int) -> int:
	var worker_session: MySQLSession = pool.acquire()
	if not worker_session.is_db_connected():
		if not worker_session.connect_db().is_empty():
			return 0
	var worker_result: MySQLResult = worker_session.execute_text("SELECT %d" % [index])
	if worker_result.is_ok() and worker_result.get_rows()[0][0] == index:
		return 1
	return 0


func _initialize() -> void:
	var host := read_credential("MYSQL_TEST_HOST")
	if host.is_empty():
		host = "127.0.0.1"
	var port_str := read_credential("MYSQL_TEST_PORT")
	var port := 3306 if port_str.is_empty() else int(port_str)
	var user := read_credential("MYSQL_TEST_USER")
	var password := read_credential("MYSQL_TEST_PASSWORD")
	var database := read_credential("MYSQL_TEST_DATABASE")

	if user.is_empty() or password.is_empty() or database.is_empty():
		fail_and_quit("smoke_test: set MYSQL_TEST_USER, MYSQL_TEST_PASSWORD and MYSQL_TEST_DATABASE in the environment.")
		return

	print("=== 0. TLS validation ===")
	var tls_config := make_config(host, port, user, password, database)
	tls_config.transport_mode = MySQLConfig.TCP_TLS_REQUIRED
	var tls_session := MySQLSession.new()
	tls_session.set_config(tls_config)
	var tls_err: Dictionary = tls_session.connect_db()
	check(not tls_err.is_empty() and tls_err.get("is_fatal") == true, "TCP_TLS_REQUIRED rejects the self-signed certificate of the test server instead of silently accepting it (%s)" % [tls_err])

	# The rest of the tests use TCP_TLS_DISABLED on purpose: it is a local test MariaDB
	# without a trusted CA configured, and the check above already proved that TLS
	# validation works. This is not the recommended setting for production.
	var config := make_config(host, port, user, password, database)
	config.transport_mode = MySQLConfig.TCP_TLS_DISABLED

	print("=== 0b. The session keeps a copy of its config ===")
	# Regression test: the TLS context of a connection is built when the session gets its
	# config, and the transport mode is read again when it connects. Changing the config in
	# between (DISABLED -> REQUIRED) used to connect with TLS but without verifying the
	# certificate. The session now keeps its own copy, so a later change has no effect.
	var late_config := make_config(host, port, user, password, database)
	late_config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	var late_session := MySQLSession.new()
	late_session.set_config(late_config)
	late_config.transport_mode = MySQLConfig.TCP_TLS_REQUIRED
	check(late_session.get_config().transport_mode == MySQLConfig.TCP_TLS_DISABLED, "changing a config after set_config() does not change the session's copy")
	late_session.get_config().transport_mode = MySQLConfig.TCP_TLS_REQUIRED
	check(late_session.get_config().transport_mode == MySQLConfig.TCP_TLS_DISABLED, "get_config() returns a copy: changing it does not change the session")
	var late_err: Dictionary = late_session.connect_db()
	var late_cipher: MySQLResult = late_session.execute_text("SHOW SESSION STATUS LIKE 'Ssl_cipher'")
	check(late_err.is_empty() and late_cipher.is_ok() and String(late_cipher.get_rows()[0][1]).is_empty(), "the session connects with the transport mode it was configured with, not the changed one (%s)" % [late_err])
	late_session.close_db()
	var required_again := MySQLSession.new()
	required_again.set_config(late_config)
	var required_err: Dictionary = required_again.connect_db()
	check(not required_err.is_empty() and required_err.get("is_fatal") == true, "a session configured with TCP_TLS_REQUIRED still rejects the self-signed certificate (%s)" % [required_err])
	var bad_mode_config := make_config(host, port, user, password, database)
	bad_mode_config.transport_mode = MySQLConfig.TCP_TLS_REQUIRED
	bad_mode_config.set("transport_mode", 7)
	check(bad_mode_config.transport_mode == MySQLConfig.TCP_TLS_REQUIRED, "a transport_mode outside the enum is rejected (it used to mean TLS without verification)")

	print("=== 1. Connection ===")
	var session := MySQLSession.new()
	session.set_config(config)
	var err: Dictionary = session.connect_db()
	check(err.is_empty(), "connect_db() without error (%s)" % [err])
	check(session.is_db_connected(), "is_db_connected() == true")
	if not session.is_db_connected():
		fail_and_quit("smoke_test: could not connect, aborting the remaining tests.")
		return

	await _run_all_tests(session, config, host, port, user, password, database)

	print("\n=== Finishing ===")
	var close_err: Dictionary = session.close_db()
	check(close_err.is_empty(), "close_db() without error (%s)" % [close_err])
	check(not session.is_db_connected(), "is_db_connected() == false after close_db()")

	print("\n%d checks, %d failures." % [checks, failures])
	quit(1 if failures > 0 else 0)


func _run_all_tests(session: MySQLSession, config: MySQLConfig, host: String, port: int, user: String, password: String, database: String) -> void:
	print("=== 2. Test table ===")
	session.execute_text("DROP TABLE IF EXISTS " + TABLE)
	var create_result: MySQLResult = session.execute_text(
		"""
		CREATE TABLE t_mysql_module_smoke_test (
			id INT PRIMARY KEY AUTO_INCREMENT,
			flag TINYINT(1),
			small_num TINYINT(4),
			big_signed BIGINT,
			big_unsigned BIGINT UNSIGNED,
			txt VARCHAR(100),
			data BLOB,
			d DATE,
			t TIME(6),
			dt DATETIME(6),
			js JSON,
			dbl DOUBLE
		)
		"""
	)
	check(create_result.is_ok(), "CREATE TABLE (%s)" % [create_result.get_error()])

	print("=== 3. execute_text and data types ===")
	var insert_result: MySQLResult = session.execute_text(
		"""
		INSERT INTO t_mysql_module_smoke_test
			(flag, small_num, big_signed, big_unsigned, txt, data, d, t, dt, js)
		VALUES
			(1, 42, -123456789, 18446744073709551615, 'olá mundo', 0x0102FF, '2026-09-21', '25:30:15.500000', '2026-09-21 10:30:00.123456', '{"a": 1, "b": [true, null]}')
		"""
	)
	check(insert_result.is_ok(), "INSERT with mixed types (%s)" % [insert_result.get_error()])
	check(insert_result.get_affected_rows() == 1, "affected_rows == 1")
	check(insert_result.get_last_insert_id() == 1, "last_insert_id == 1")

	var select_result: MySQLResult = session.execute_text("SELECT * FROM t_mysql_module_smoke_test WHERE id = 1")
	check(select_result.is_ok(), "SELECT it back (%s)" % [select_result.get_error()])
	var cols: PackedStringArray = select_result.get_column_names()
	var rows: Array = select_result.get_rows()
	check(rows.size() == 1, "SELECT returned 1 row")
	if rows.size() == 1:
		var row: Array = rows[0]

		var flag_v = row[cols.find("flag")]
		check(typeof(flag_v) == TYPE_INT and flag_v == 1, "tinyint1_mode=false (default): TINYINT(1) is an int, not a bool (value: %s, type: %s)" % [flag_v, typeof(flag_v)])
		var small_num_v = row[cols.find("small_num")]
		check(typeof(small_num_v) == TYPE_INT and small_num_v == 42, "TINYINT(4) is an int (%s)" % [small_num_v])
		var big_signed_v = row[cols.find("big_signed")]
		check(typeof(big_signed_v) == TYPE_INT and big_signed_v == -123456789, "signed BIGINT is an int (%s)" % [big_signed_v])
		var big_unsigned_v = row[cols.find("big_unsigned")]
		check(typeof(big_unsigned_v) == TYPE_STRING and big_unsigned_v == "18446744073709551615", "BIGINT UNSIGNED above INT64_MAX is an exact String (%s)" % [big_unsigned_v])
		var txt_v = row[cols.find("txt")]
		check(typeof(txt_v) == TYPE_STRING and txt_v == "olá mundo", "utf8mb4 VARCHAR is preserved (%s)" % [txt_v])
		var data_v = row[cols.find("data")]
		check(typeof(data_v) == TYPE_PACKED_BYTE_ARRAY and data_v == PackedByteArray([1, 2, 255]), "BLOB is a PackedByteArray (%s)" % [data_v])

		var d: Dictionary = row[cols.find("d")]
		check(d.get("year") == 2026 and d.get("month") == 9 and d.get("day") == 21, "DATE becomes the right Dictionary (%s)" % [d])

		var t: Dictionary = row[cols.find("t")]
		check(t.get("hours") == 25 and t.get("minutes") == 30 and t.get("seconds") == 15 and t.get("microsecond") == 500000, "TIME above 24h with microseconds becomes the right Dictionary (%s)" % [t])

		var dt: Dictionary = row[cols.find("dt")]
		check(dt.get("year") == 2026 and dt.get("hour") == 10 and dt.get("microsecond") == 123456, "DATETIME(6) with microseconds becomes the right Dictionary (%s)" % [dt])

		print("=== 3b. json_result_mode ===")
		var json_col_index := cols.find("js")
		var js_v = row[json_col_index]
		check(typeof(js_v) == TYPE_STRING, "default json_result_mode (LAZY_PARSED_VARIANT) returns the raw String in the row (%s)" % [js_v])
		var parsed: Variant = select_result.get_parsed_json(0, 0, json_col_index)
		check(typeof(parsed) == TYPE_DICTIONARY and parsed.get("a") == 1, "get_parsed_json() parses on demand (%s)" % [parsed])
		var parsed_again: Variant = select_result.get_parsed_json(0, 0, json_col_index)
		check(parsed_again == parsed, "get_parsed_json() is idempotent (cache)")

	# json_result_mode = PARSED_VARIANT depends on the server reporting a distinct JSON
	# type in the column metadata. MariaDB does not: JSON is an alias of LONGTEXT in the
	# protocol, so PARSED_VARIANT behaves like RAW_STRING there. MySQL does report a
	# distinct JSON type, so PARSED_VARIANT there actually returns a parsed Dictionary.
	# This is not a module bug, it is a real difference between the databases: detect which
	# one is running and check the behavior that actually applies to it.
	var version_result: MySQLResult = session.execute_text("SELECT VERSION()")
	var is_mariadb: bool = String(version_result.get_rows()[0][0]).to_lower().contains("mariadb")
	# A session keeps a copy of its config, so the mode is set on a config for a new session.
	var json_config := make_config(host, port, user, password, database)
	json_config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	json_config.json_result_mode = MySQLConfig.PARSED_VARIANT
	var json_session := MySQLSession.new()
	json_session.set_config(json_config)
	json_session.connect_db()
	var select_parsed: MySQLResult = json_session.execute_text("SELECT js FROM t_mysql_module_smoke_test WHERE id = 1")
	var js_parsed_variant: Variant = select_parsed.get_rows()[0][0]
	if is_mariadb:
		check(typeof(js_parsed_variant) == TYPE_STRING, "json_result_mode = PARSED_VARIANT on MariaDB: no JSON type in the protocol, behaves like RAW_STRING (%s)" % [js_parsed_variant])
	else:
		check(typeof(js_parsed_variant) == TYPE_DICTIONARY and js_parsed_variant.get("a") == 1, "json_result_mode = PARSED_VARIANT on MySQL: a native JSON type is reported, parsed eagerly into a Dictionary (%s)" % [js_parsed_variant])
	json_session.close_db()

	print("=== 3c. Result shape ===")
	var duplicate_cols: MySQLResult = session.execute_text("SELECT 1 AS a, 2 AS a")
	check(duplicate_cols.is_ok() and duplicate_cols.get_column_names() == PackedStringArray(["a", "a"]), "duplicate column names are preserved (%s)" % [duplicate_cols.get_column_names()])
	check(duplicate_cols.get_rows()[0] == [1, 2], "duplicate columns keep both values in order (%s)" % [duplicate_cols.get_rows()[0]])
	var mutated_rows: Array = duplicate_cols.get_rows()
	mutated_rows.clear()
	check(duplicate_cols.get_rows().size() == 1, "get_rows() returns a copy: clearing it leaves the result intact")
	var update_result: MySQLResult = session.execute_text("UPDATE t_mysql_module_smoke_test SET small_num = 43 WHERE id = 1")
	check(update_result.is_ok() and update_result.get_affected_rows() == 1, "affected_rows of an UPDATE (%s)" % [update_result.get_affected_rows()])
	var empty_select: MySQLResult = session.execute_text("SELECT * FROM t_mysql_module_smoke_test WHERE id = -1")
	check(empty_select.is_ok() and empty_select.get_rows().is_empty() and empty_select.get_column_names().size() > 0, "an empty SELECT has no rows but keeps its columns")
	var sql_error: MySQLResult = session.execute_text("SELEC nothing")
	check(not sql_error.is_ok() and sql_error.get_error().get("category", "") != "", "a syntax error is reported through get_error() (%s)" % [sql_error.get_error()])

	print("=== 4. execute_formatted (no home-made escaping) ===")
	var injection_attempt := "'); DROP TABLE t_mysql_module_smoke_test; --"
	var formatted_result: MySQLResult = session.execute_formatted(
		"INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", [injection_attempt]
	)
	check(formatted_result.is_ok(), "execute_formatted with a 'dangerous' value does not break the query (%s)" % [formatted_result.get_error()])
	var still_exists: MySQLResult = session.execute_text("SELECT COUNT(*) AS n FROM t_mysql_module_smoke_test")
	check(still_exists.is_ok() and int(still_exists.get_rows()[0][0]) >= 2, "the table survives the injection attempt (COUNT = %s)" % [still_exists.get_rows()[0][0] if still_exists.is_ok() else "?"])
	var escaped_back: MySQLResult = session.execute_text("SELECT txt FROM t_mysql_module_smoke_test WHERE id = 2")
	check(escaped_back.is_ok() and escaped_back.get_rows()[0][0] == injection_attempt, "the 'dangerous' value was stored literally, as a string (%s)" % [escaped_back.get_rows()[0][0] if escaped_back.is_ok() else "?"])
	var literal_question: MySQLResult = session.execute_formatted("SELECT '?' AS a, \"it's ?\" AS b, ? AS c", [7])
	check(literal_question.is_ok() and literal_question.get_rows()[0] == ["?", "it's ?", 7], "a '?' inside a quoted literal is text, not a placeholder (%s)" % [literal_question.get_rows() if literal_question.is_ok() else literal_question.get_error()])
	var no_placeholders: MySQLResult = session.execute_formatted("SELECT 'a?b' AS a", [])
	check(no_placeholders.is_ok() and no_placeholders.get_rows()[0][0] == "a?b", "a quoted '?' alone does not demand a parameter (%s)" % [no_placeholders.get_error()])
	var commented_question: MySQLResult = session.execute_formatted("SELECT ? AS a /* why? don't */, ? AS b -- really?\n", [1, 2])
	check(commented_question.is_ok() and commented_question.get_rows()[0] == [1, 2], "a '?' or an apostrophe inside a comment is not read as SQL (%s)" % [commented_question.get_rows() if commented_question.is_ok() else commented_question.get_error()])
	# Under NO_BACKSLASH_ESCAPES a backslash is plain text inside a literal, so 'C:\' is a
	# complete literal and the '?' after it is a real placeholder.
	session.execute_text("SET @smoke_saved_sql_mode = @@SESSION.sql_mode")
	session.execute_text("SET SESSION sql_mode = CONCAT(@@SESSION.sql_mode, ',NO_BACKSLASH_ESCAPES')")
	var no_backslash: MySQLResult = session.execute_formatted("SELECT 'C:\\' AS p, ? AS n, ? AS s", [1, "a\\b'c"])
	check(no_backslash.is_ok() and no_backslash.get_rows()[0] == ["C:\\", 1, "a\\b'c"], "execute_formatted follows NO_BACKSLASH_ESCAPES (%s)" % [no_backslash.get_rows() if no_backslash.is_ok() else no_backslash.get_error()])
	session.execute_text("SET SESSION sql_mode = @smoke_saved_sql_mode")
	var backslash_again: MySQLResult = session.execute_formatted("SELECT 'it\\'s' AS q, ? AS n", [1])
	check(backslash_again.is_ok() and backslash_again.get_rows()[0] == ["it's", 1], "execute_formatted follows the SQL mode being restored (%s)" % [backslash_again.get_rows() if backslash_again.is_ok() else backslash_again.get_error()])

	print("=== 5. Parameter round trip (execute_prepared) ===")
	var blob := PackedByteArray([0, 255, 16, 0, 127])
	var param_insert: MySQLResult = session.execute_prepared(
		"INSERT INTO t_mysql_module_smoke_test (txt, data, dbl, small_num, flag) VALUES (?, ?, ?, ?, ?)",
		["param_row", blob, 0.1, null, true]
	)
	check(param_insert.is_ok(), "execute_prepared with String, PackedByteArray, float, null and bool (%s)" % [param_insert.get_error()])
	var param_back: MySQLResult = session.execute_text("SELECT data, dbl, small_num, flag FROM t_mysql_module_smoke_test WHERE txt = 'param_row'")
	if param_back.is_ok() and param_back.get_rows().size() == 1:
		var param_row: Array = param_back.get_rows()[0]
		check(param_row[0] == blob, "BLOB with NUL bytes round-trips exactly (%s)" % [param_row[0]])
		check(param_row[1] == 0.1, "DOUBLE round-trips exactly (%s)" % [param_row[1]])
		check(param_row[2] == null, "null is stored as NULL (%s)" % [param_row[2]])
		check(param_row[3] == 1, "bool true is stored as 1 (%s)" % [param_row[3]])
	else:
		check(false, "read back the parameter row (%s)" % [param_back.get_error()])

	print("=== 5b. Prepared statement cache (LRU) ===")
	# Com_stmt_prepare and Com_stmt_close are session-scoped counters on the server, so
	# they count exactly what this connection did.
	var same_sql := "SELECT ? + 1000"
	var prepares_before := session_status(session, "Com_stmt_prepare")
	for i in range(5):
		var same_result: MySQLResult = session.execute_prepared(same_sql, [i])
		check(same_result.is_ok() and same_result.get_rows()[0][0] == 1000 + i, "cached statement returns the right value on call #%d" % [i])
	var prepares_after_same := session_status(session, "Com_stmt_prepare")
	check(prepares_after_same - prepares_before == 1, "the same SQL run 5 times is prepared once (%d prepares)" % [prepares_after_same - prepares_before])

	check(make_config(host, port, user, password, database).statement_cache_size == 512, "statement_cache_size defaults to 512")

	# A dedicated session with a small statement_cache_size, so the eviction test stays fast
	# and deterministic instead of depending on the size of the default above.
	var cache_config := make_config(host, port, user, password, database)
	cache_config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	cache_config.statement_cache_size = 5
	var cache_session := MySQLSession.new()
	cache_session.set_config(cache_config)
	cache_session.connect_db()

	# Fill the cache (5) and run 2 more distinct statements: the oldest are evicted and the
	# server must be told to close them, otherwise they would leak until disconnect.
	var closes_before := session_status(cache_session, "Com_stmt_close")
	for i in range(7):
		var distinct_result: MySQLResult = cache_session.execute_prepared("SELECT ? + %d" % [i], [1])
		if not distinct_result.is_ok():
			check(false, "distinct prepared statement #%d (%s)" % [i, distinct_result.get_error()])
			break
	var closes_after := session_status(cache_session, "Com_stmt_close")
	check(closes_after - closes_before == 2, "evicting from a statement_cache_size=5 cache closes the 2 oldest statements on the server (%d closes)" % [closes_after - closes_before])

	var prepares_mid := session_status(cache_session, "Com_stmt_prepare")
	cache_session.execute_prepared("SELECT ? + 6", [1]) # Most recently used: must still be cached.
	check(session_status(cache_session, "Com_stmt_prepare") == prepares_mid, "a recently used statement is still cached after the evictions")
	cache_session.execute_prepared("SELECT ? + 0", [1]) # Used first, long ago: must have been evicted.
	check(session_status(cache_session, "Com_stmt_prepare") == prepares_mid + 1, "the least recently used statement was evicted and is prepared again")
	cache_session.close_db()

	print("=== 6. Explicit errors (never silent) ===")
	var bad_param_result: MySQLResult = session.execute_prepared("INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", [])
	check(not bad_param_result.is_ok(), "a wrong parameter count is an explicit error (%s)" % [bad_param_result.get_error()])
	var bad_type_result: MySQLResult = session.execute_formatted("INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", [{"unsupported": "type"}])
	check(not bad_type_result.is_ok(), "a Dictionary parameter is an explicit error, not a silent NULL (%s)" % [bad_type_result.get_error()])
	var too_many_result: MySQLResult = session.execute_formatted("SELECT 1", [1])
	check(not too_many_result.is_ok(), "execute_formatted with more parameters than placeholders is an error (%s)" % [too_many_result.get_error()])
	var not_connected := MySQLSession.new()
	var not_connected_result: MySQLResult = not_connected.execute_text("SELECT 1")
	check(not not_connected_result.is_ok(), "a session without config returns an error instead of crashing (%s)" % [not_connected_result.get_error()])

	print("=== 6b. Result size limit (max_result_bytes) ===")
	check(make_config(host, port, user, password, database).max_result_bytes == 0, "max_result_bytes defaults to 0 (unlimited)")

	var limit_config := make_config(host, port, user, password, database)
	limit_config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	limit_config.max_result_bytes = 100
	var limit_session := MySQLSession.new()
	limit_session.set_config(limit_config)
	limit_session.connect_db()

	var small_result: MySQLResult = limit_session.execute_text("SELECT 'ok'")
	check(small_result.is_ok(), "a result under max_result_bytes still succeeds (%s)" % [small_result.get_error()])

	var big_result: MySQLResult = limit_session.execute_text("SELECT REPEAT('x', 10000)")
	check(not big_result.is_ok(), "a result over max_result_bytes fails explicitly instead of being silently truncated (%s)" % [big_result.get_error()])
	var still_usable: MySQLResult = limit_session.execute_text("SELECT 'still_usable'")
	check(still_usable.is_ok() and still_usable.get_rows()[0][0] == "still_usable", "the connection is still usable after a max_result_bytes overflow, so the overflowing resultset was correctly drained (%s)" % [still_usable.get_error()])

	var big_prepared: MySQLResult = limit_session.execute_prepared("SELECT REPEAT(?, 10000)", ["y"])
	check(not big_prepared.is_ok(), "max_result_bytes is also enforced on execute_prepared (%s)" % [big_prepared.get_error()])

	var async_big: MySQLAsyncOperation = limit_session.async_execute_text("SELECT REPEAT('z', 10000)")
	var async_big_result: MySQLResult = await await_operation(async_big)
	check(not async_big_result.is_ok(), "max_result_bytes is also enforced on the asynchronous path (%s)" % [async_big_result.get_error()])
	var async_still_usable: MySQLAsyncOperation = limit_session.async_execute_text("SELECT 'async_still_usable'")
	var async_still_usable_result: MySQLResult = await await_operation(async_still_usable)
	check(async_still_usable_result.is_ok() and async_still_usable_result.get_rows()[0][0] == "async_still_usable", "the connection is still usable after an asynchronous max_result_bytes overflow (%s)" % [async_still_usable_result.get_error()])

	limit_session.close_db()

	print("=== 7. Transactions ===")
	var tx_commit := session.begin_transaction()
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_commit')")
	var commit_err: Dictionary = tx_commit.commit()
	check(commit_err.is_empty(), "commit() without error (%s)" % [commit_err])
	var after_commit: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'tx_commit'")
	check(after_commit.is_ok() and int(after_commit.get_rows()[0][0]) == 1, "the committed row is visible")

	var tx_rollback := session.begin_transaction()
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_rollback')")
	var rollback_err: Dictionary = tx_rollback.rollback()
	check(rollback_err.is_empty(), "rollback() without error (%s)" % [rollback_err])
	var after_rollback: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'tx_rollback'")
	check(after_rollback.is_ok() and int(after_rollback.get_rows()[0][0]) == 0, "the rolled back row is not visible")

	var tx_twice := session.begin_transaction()
	tx_twice.commit()
	check(not tx_twice.commit().is_empty(), "commit() on an already finished transaction is an error")
	check(tx_twice.is_ok() and tx_twice.get_error().is_empty(), "a transaction that started reports is_ok()")

	# A failed START TRANSACTION still returns a transaction (never null), so chaining a
	# call on it reports the error instead of crashing the script.
	var tx_failed: MySQLTransaction = not_connected.begin_transaction()
	check(tx_failed != null and not tx_failed.is_ok() and not tx_failed.get_error().is_empty(), "begin_transaction() on a session that is not connected returns a failed transaction (%s)" % [tx_failed.get_error() if tx_failed != null else "null"])
	if tx_failed != null:
		check(tx_failed.commit() == tx_failed.get_error(), "commit() on a transaction that never started returns the START TRANSACTION error")
		check(tx_failed.rollback() == tx_failed.get_error(), "rollback() on a transaction that never started returns the START TRANSACTION error")

	print("=== 7b. Automatic rollback (destructor) ===")
	var tx_auto := session.begin_transaction()
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_auto_rollback')")
	tx_auto = null # Drops the only reference without commit() or rollback(), so the destructor runs.
	var after_auto: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'tx_auto_rollback'")
	check(after_auto.is_ok() and int(after_auto.get_rows()[0][0]) == 0, "the automatic rollback (destructor without commit or rollback) undid the row")

	print("=== 7c. COMMIT refused while the session is busy ===")
	# Regression test: commit() on a busy session is refused before anything is sent. It
	# used to mark the transaction finished anyway, leaving it open on the server for good
	# (holding its locks): rollback() then said "already finished" and the destructor did
	# nothing. The transaction must stay open, and a later rollback() must end it.
	var busy_tx := session.begin_transaction()
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_busy')")
	var busy_op: MySQLAsyncOperation = session.async_execute_text("SELECT SLEEP(0.3)")
	var refused_commit: Dictionary = busy_tx.commit()
	check(not refused_commit.is_empty(), "commit() while an asynchronous operation runs is refused (%s)" % [refused_commit.get("message", "")])
	await await_operation(busy_op)
	var late_rollback: Dictionary = busy_tx.rollback()
	check(late_rollback.is_empty(), "rollback() works once the session is free again (%s)" % [late_rollback])
	var busy_row: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'tx_busy'")
	check(busy_row.is_ok() and int(busy_row.get_rows()[0][0]) == 0, "the transaction was rolled back, not left open")
	busy_tx = null

	print("=== 8. Streaming ===")
	for i in range(20):
		session.execute_prepared("INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", ["stream_%d" % i])
	var cursor: MySQLStreamingCursor = session.execute_streaming("SELECT txt FROM t_mysql_module_smoke_test WHERE txt LIKE 'stream\\_%' ORDER BY id")
	check(cursor.is_ok(), "execute_streaming() opened without error (%s)" % [cursor.get_error()])
	check(cursor.get_column_names() == PackedStringArray(["txt"]), "the cursor exposes its column names (%s)" % [cursor.get_column_names()])
	var streamed_count := 0
	while cursor.has_more():
		var batch: Array = cursor.next_batch()
		streamed_count += batch.size()
	check(streamed_count == 20, "streaming read the 20 rows in batches (read %d)" % [streamed_count])
	check(cursor.is_ok(), "cursor.is_ok() is still true after being exhausted")

	var cursor2: MySQLStreamingCursor = session.execute_streaming("SELECT txt FROM t_mysql_module_smoke_test WHERE txt LIKE 'stream\\_%'")
	cursor2.next_batch()
	cursor2.close() # Closed midway on purpose: it drains the rest by itself.
	var after_partial_stream: MySQLResult = session.execute_text("SELECT 1")
	check(after_partial_stream.is_ok(), "the connection is still usable after closing a stream midway (%s)" % [after_partial_stream.get_error()])

	# An open cursor holds the connection until it is read to the end or closed: every
	# other call on the session fails explicitly (nothing waits or queues), including an
	# asynchronous one, which must be rejected before it reaches the I/O thread.
	var holding: MySQLStreamingCursor = session.execute_streaming("SELECT txt FROM t_mysql_module_smoke_test WHERE txt LIKE 'stream\\_%'")
	var while_open: MySQLResult = session.execute_text("SELECT 1")
	check(not while_open.is_ok() and String(while_open.get_error().get("message", "")).contains("multi-function"), "a query while a cursor is open fails explicitly (%s)" % [while_open.get_error().get("message", "")])
	var async_while_open: MySQLResult = await await_operation(session.async_execute_text("SELECT 1"))
	check(not async_while_open.is_ok() and String(async_while_open.get_error().get("message", "")).contains("multi-function"), "an asynchronous call while a cursor is open fails explicitly (%s)" % [async_while_open.get_error().get("message", "")])
	check(not session.execute_streaming("SELECT 1").is_ok(), "a second cursor while one is open fails explicitly")
	var held_rows := 0
	while holding.has_more():
		held_rows += holding.next_batch().size()
	check(held_rows == 20, "the open cursor still reads all its rows after the rejected calls (%d)" % [held_rows])
	check(session.execute_text("SELECT 1").is_ok(), "a cursor read to the end no longer holds the connection, even before close()")
	holding.close()

	var bad_cursor: MySQLStreamingCursor = session.execute_streaming("SELEC nothing")
	check(not bad_cursor.is_ok() and not bad_cursor.has_more(), "a streaming syntax error is reported and the cursor has nothing to read")

	print("=== 8b. Asynchronous streaming ===")
	# async_execute_streaming() returns the cursor right away and opens it on the I/O
	# thread; each async_next_batch() is an asynchronous operation carrying one batch as a
	# MySQLResult. The first one waits for the opening.
	var async_cursor: MySQLStreamingCursor = session.async_execute_streaming("SELECT txt FROM t_mysql_module_smoke_test WHERE txt LIKE 'stream\\_%' ORDER BY id")
	check(async_cursor.is_ok() and async_cursor.has_more(), "async_execute_streaming() returns a cursor with rows to read")
	var async_streamed := 0
	var async_batches_ok := true
	while async_cursor.has_more():
		var async_batch: MySQLResult = await async_cursor.async_next_batch().completed
		async_batches_ok = async_batches_ok and async_batch.is_ok()
		async_streamed += async_batch.get_rows().size() if async_batch.is_ok() else 0
	check(async_batches_ok and async_streamed == 20, "the asynchronous cursor read the 20 rows (read %d)" % [async_streamed])
	check(async_cursor.get_column_names() == PackedStringArray(["txt"]), "the asynchronous cursor exposes its column names (%s)" % [async_cursor.get_column_names()])
	check(session.execute_text("SELECT 1").is_ok(), "an asynchronous cursor read to the end no longer holds the connection")
	var past_end: MySQLResult = await async_cursor.async_next_batch().completed
	check(past_end.is_ok() and past_end.get_rows().is_empty(), "async_next_batch() past the end returns an empty batch")

	# A result larger than one batch arrives in several, without blocking the main thread.
	const BIG_STREAM := "WITH RECURSIVE r(n) AS (SELECT 1 UNION ALL SELECT n + 1 FROM r WHERE n < 50000) SELECT n, REPEAT('x', 100) FROM r"
	session.execute_text("SET SESSION cte_max_recursion_depth = 100000")
	var big_cursor: MySQLStreamingCursor = session.async_execute_streaming(BIG_STREAM)
	var big_rows := 0
	var big_batches := 0
	var big_last := 0
	while big_cursor.has_more():
		var big_batch: MySQLResult = await big_cursor.async_next_batch().completed
		if not big_batch.is_ok():
			check(false, "a batch of a large asynchronous stream (%s)" % [big_batch.get_error()])
			break
		big_batches += 1
		big_rows += big_batch.get_rows().size()
		if not big_batch.get_rows().is_empty():
			big_last = big_batch.get_rows()[-1][0]
	var batch_target: int = config.async_batch_rows
	check(big_rows == 50000 and big_last == 50000 and big_batches > 1, "a large asynchronous stream arrives complete, in several batches (%d rows, %d batches)" % [big_rows, big_batches])
	check(big_batches <= 50000 / batch_target + 1, "the batches gather at least async_batch_rows rows each (%d batches of about %d rows for %d rows)" % [big_batches, batch_target, big_rows])

	# While a batch is in flight the I/O thread owns the connection, and between batches
	# the cursor does: every other call fails explicitly, as with the synchronous cursor.
	var held_async: MySQLStreamingCursor = session.async_execute_streaming(BIG_STREAM)
	var in_flight: MySQLResult = session.execute_text("SELECT 1")
	check(not in_flight.is_ok(), "a query while the cursor opens fails explicitly (%s)" % [in_flight.get_error().get("message", "")])
	var first_batch_op: MySQLAsyncOperation = held_async.async_next_batch()
	var second_batch_op: MySQLAsyncOperation = held_async.async_next_batch()
	var second_batch: MySQLResult = await await_operation(second_batch_op)
	check(not second_batch.is_ok(), "a second async_next_batch() while one is running fails explicitly (%s)" % [second_batch.get_error().get("message", "")])
	var first_batch: MySQLResult = await await_operation(first_batch_op)
	check(first_batch.is_ok() and not first_batch.get_rows().is_empty(), "the running batch still arrives (%s)" % [first_batch.get_error()])
	var between: MySQLResult = session.execute_text("SELECT 1")
	check(not between.is_ok() and String(between.get_error().get("message", "")).contains("multi-function"), "a query between batches fails explicitly (%s)" % [between.get_error().get("message", "")])
	check(not session.async_execute_streaming("SELECT 1").is_ok(), "a second cursor while an asynchronous one is open fails explicitly")
	check(held_async.next_batch().is_empty(), "next_batch() is not available on an asynchronous cursor")
	var closed_async: MySQLResult = await held_async.async_close().completed
	check(closed_async.is_ok(), "async_close() drains the rest of the stream (%s)" % [closed_async.get_error()])
	check(not held_async.has_more(), "a closed asynchronous cursor has nothing more to read")
	var after_async_close: MySQLResult = session.execute_text("SELECT 'after_async_close'")
	check(after_async_close.is_ok(), "the connection is usable after async_close() (%s)" % [after_async_close.get_error()])

	# close() while a batch is in flight drains once the batch arrives; a cursor released
	# midway drains in the background. Either way the session is busy until the drain ends.
	var close_early: MySQLStreamingCursor = session.async_execute_streaming(BIG_STREAM)
	var close_early_op: MySQLAsyncOperation = close_early.async_next_batch()
	close_early.close()
	await await_operation(close_early_op)
	var close_early_drain: MySQLResult = await await_operation(close_early.async_close())
	check(close_early_drain.is_ok(), "async_close() after close() during a running batch waits for the drain (%s)" % [close_early_drain.get_error()])
	check(session.execute_text("SELECT 1").is_ok(), "the session is usable once that drain finished")
	var dropped: MySQLStreamingCursor = session.async_execute_streaming(BIG_STREAM)
	await await_operation(dropped.async_next_batch())
	dropped = null # Released midway, without close().
	check(await _wait_until_usable(session), "the session is usable after an asynchronous cursor is released midway")

	# Errors and empty results.
	var bad_async: MySQLStreamingCursor = session.async_execute_streaming("SELEC nothing")
	var bad_batch: MySQLResult = await bad_async.async_next_batch().completed
	check(not bad_batch.is_ok() and not bad_async.is_ok() and not bad_async.has_more(), "an asynchronous streaming syntax error reaches the first batch and the cursor (%s)" % [bad_batch.get_error().get("message", "")])
	check(session.execute_text("SELECT 1").is_ok(), "a failed asynchronous cursor does not hold the connection")
	var empty_async: MySQLStreamingCursor = session.async_execute_streaming("SELECT txt FROM t_mysql_module_smoke_test WHERE 1 = 0")
	var empty_batch: MySQLResult = await empty_async.async_next_batch().completed
	check(empty_batch.is_ok() and empty_batch.get_rows().is_empty() and not empty_async.has_more(), "an empty asynchronous stream gives one empty batch")
	check(empty_batch.get_column_names() == PackedStringArray(["txt"]), "the empty batch still has the column names (%s)" % [empty_batch.get_column_names()])
	var no_rows_async: MySQLStreamingCursor = session.async_execute_streaming("DO 1")
	var no_rows_batch: MySQLResult = await no_rows_async.async_next_batch().completed
	check(no_rows_batch.is_ok() and no_rows_batch.get_rows().is_empty() and not no_rows_async.has_more(), "a statement without a resultset gives one empty batch (%s)" % [no_rows_batch.get_error()])
	check(session.execute_text("SELECT 1").is_ok(), "a statement without a resultset does not hold the connection")
	var sync_cursor_async: MySQLResult = await session.execute_streaming("SELECT 1").async_next_batch().completed
	check(not sync_cursor_async.is_ok(), "async_next_batch() is not available on a synchronous cursor")

	# The first batch arriving after other awaits: has_more() keeps saying so until it is
	# taken, and awaiting .completed directly still returns.
	var late: MySQLStreamingCursor = session.async_execute_streaming("SELECT 'late' AS v")
	await create_timer(0.2).timeout
	check(late.has_more(), "has_more() is true until the first batch is taken, even if it already arrived")
	var late_batch: MySQLResult = await late.async_next_batch().completed
	check(late_batch.is_ok() and late_batch.get_rows() == [["late"]], "a first batch that already arrived is still delivered (%s)" % [late_batch.get_rows() if late_batch.is_ok() else late_batch.get_error()])
	check(not late.has_more(), "has_more() is false once the only batch is taken")

	print("=== 9. Asynchronous ===")
	# Use await on the signal, not a blocking polling loop: a loop with OS.delay_msec()
	# never gives control back to the SceneTree to run frames, and running frames is what
	# processes the call_deferred queue. Such a loop would never see is_finished() become
	# true, even though the operation does complete.
	var async_op: MySQLAsyncOperation = session.async_execute_text("SELECT 'async_ok' AS v")
	var async_result: MySQLResult = await await_operation(async_op)
	check(async_op.is_finished(), "async_execute_text finished (the completed signal fired)")
	check(async_result.is_ok() and async_result.get_rows()[0][0] == "async_ok", "async_execute_text returned the right value (%s)" % [async_result.get_rows() if async_result.is_ok() else async_result.get_error()])

	var async_prepared: MySQLAsyncOperation = session.async_execute_prepared("SELECT ? + ?", [40, 2])
	var async_prepared_result: MySQLResult = await await_operation(async_prepared)
	check(async_prepared_result.is_ok() and async_prepared_result.get_rows()[0][0] == 42, "async_execute_prepared returned the right value (%s)" % [async_prepared_result.get_rows() if async_prepared_result.is_ok() else async_prepared_result.get_error()])

	var async_bad_params: MySQLAsyncOperation = session.async_execute_prepared("SELECT ?", [{"unsupported": "type"}])
	var async_bad_params_result: MySQLResult = await await_operation(async_bad_params)
	check(not async_bad_params_result.is_ok(), "async_execute_prepared with an unsupported parameter fails explicitly")

	var async_sql_error: MySQLAsyncOperation = session.async_execute_text("SELEC nothing")
	var async_sql_error_result: MySQLResult = await await_operation(async_sql_error)
	check(not async_sql_error_result.is_ok(), "a server error in an asynchronous operation reaches the result (%s)" % [async_sql_error_result.get_error()])

	# A connection runs one operation at a time. Starting a second one while the first is
	# in flight must be an explicit error on the second, never a crash or a hang, and the
	# first one must still finish normally.
	var first_op: MySQLAsyncOperation = session.async_execute_text("SELECT SLEEP(0.2)")
	var second_op: MySQLAsyncOperation = session.async_execute_text("SELECT 2")
	var second_result: MySQLResult = await await_operation(second_op)
	check(not second_result.is_ok(), "a second operation on a busy session fails explicitly (%s)" % [second_result.get_error().get("message", "")])
	var first_result: MySQLResult = await await_operation(first_op)
	check(first_result.is_ok(), "the first operation still finishes normally (%s)" % [first_result.get_error()])
	var after_busy: MySQLResult = session.execute_text("SELECT 'still_usable'")
	check(after_busy.is_ok(), "the session is still usable after a rejected concurrent operation (%s)" % [after_busy.get_error()])

	# While an asynchronous operation runs, the I/O thread owns the connection: every other
	# call on the session fails explicitly without touching it, and close_db() does not
	# close it from under the running operation.
	var owner_op: MySQLAsyncOperation = session.async_execute_text("SELECT SLEEP(0.3)")
	var busy_sync: MySQLResult = session.execute_text("SELECT 1")
	check(not busy_sync.is_ok() and busy_sync.get_error().get("category", "") == "mysql.client", "a synchronous call during an asynchronous operation fails explicitly (%s)" % [busy_sync.get_error().get("message", "")])
	check(not session.execute_formatted("SELECT ?", [1]).is_ok(), "execute_formatted during an asynchronous operation fails explicitly")
	check(not session.execute_prepared("SELECT ?", [1]).is_ok(), "execute_prepared during an asynchronous operation fails explicitly")
	check(not session.execute_streaming("SELECT 1").is_ok(), "execute_streaming during an asynchronous operation fails explicitly")
	check(not session.close_db().is_empty(), "close_db() during an asynchronous operation fails explicitly")
	check(session.is_db_connected(), "the session is still connected after a rejected close_db()")
	var owner_result: MySQLResult = await await_operation(owner_op)
	check(owner_result.is_ok(), "the asynchronous operation finishes normally after the rejected calls (%s)" % [owner_result.get_error()])
	check(session.execute_text("SELECT 1").is_ok(), "synchronous calls work again once the asynchronous operation finished")
	check(not owner_op.has_method("_complete"), "MySQLAsyncOperation does not expose _complete to scripts")

	var async_not_connected: MySQLAsyncOperation = not_connected.async_execute_text("SELECT 1")
	var async_not_connected_result: MySQLResult = await await_operation(async_not_connected)
	check(not async_not_connected_result.is_ok(), "an asynchronous call on a session without config fails explicitly")

	print("=== 9b. Fatal errors drop the connection ===")
	# Regression test: after async_timeout_ms cancels an operation, Boost.MySQL leaves the
	# connection in an unspecified state (the reply of the cancelled query is still on its
	# way). It used to stay "connected", and the next query read that reply as its own
	# result, every later result shifted by one. A fatal error must now drop the connection.
	var timeout_config := make_config(host, port, user, password, database)
	timeout_config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	timeout_config.async_timeout_ms = 300
	var timeout_session := MySQLSession.new()
	timeout_session.set_config(timeout_config)
	timeout_session.connect_db()
	var timed_out: MySQLResult = await await_operation(timeout_session.async_execute_text("SELECT SLEEP(1) AS slept"))
	check(not timed_out.is_ok() and timed_out.get_error().get("is_fatal") == true, "an operation cancelled by async_timeout_ms fails with a fatal error (%s)" % [timed_out.get_error()])
	check(not timeout_session.is_db_connected(), "the session is no longer connected after the timeout")
	var after_timeout: MySQLResult = timeout_session.execute_text("SELECT 'after_timeout' AS v")
	check(not after_timeout.is_ok(), "a query after the timeout fails explicitly instead of reading the cancelled reply (%s)" % [after_timeout.get_rows() if after_timeout.is_ok() else after_timeout.get_error().get("message", "")])
	check(timeout_session.connect_db().is_empty(), "connect_db() reconnects after the timeout")
	var reconnected: MySQLResult = timeout_session.execute_text("SELECT 'after_timeout' AS v")
	check(reconnected.is_ok() and reconnected.get_rows()[0][0] == "after_timeout", "the reconnected session gets its own result (%s)" % [reconnected.get_rows() if reconnected.is_ok() else reconnected.get_error()])
	var reprepared: MySQLResult = timeout_session.execute_prepared("SELECT ? + 1", [41])
	check(reprepared.is_ok() and reprepared.get_rows()[0][0] == 42, "execute_prepared works after the reconnection (%s)" % [reprepared.get_error()])

	# Same for a synchronous call: a connection killed on the server fails the next query
	# with a fatal (network) error, and the session reports it as disconnected.
	var killed_id: MySQLResult = timeout_session.execute_text("SELECT CONNECTION_ID()")
	if killed_id.is_ok():
		session.execute_text("KILL %d" % [int(killed_id.get_rows()[0][0])])
	var after_kill_sync: MySQLResult = timeout_session.execute_text("SELECT 1")
	check(not after_kill_sync.is_ok() and after_kill_sync.get_error().get("is_fatal") == true, "a query on a connection killed on the server fails with a fatal error (%s)" % [after_kill_sync.get_error()])
	check(not timeout_session.is_db_connected(), "the session is no longer connected after a fatal synchronous error")
	timeout_session = null

	# A pooled connection that timed out is not handed to the next lease as connected.
	var timeout_pool := MySQLPool.new()
	timeout_pool.set_config(timeout_config)
	timeout_pool.max_size = 1
	var timeout_lease: MySQLSession = timeout_pool.acquire()
	timeout_lease.connect_db()
	await await_operation(timeout_lease.async_execute_text("SELECT SLEEP(1)"))
	timeout_lease = null
	var next_lease: MySQLSession = timeout_pool.acquire()
	check(not next_lease.is_db_connected(), "the lease after a timed-out pooled connection is not connected")
	if not next_lease.is_db_connected():
		next_lease.connect_db()
	var next_lease_result: MySQLResult = next_lease.execute_text("SELECT 'own_result' AS v")
	check(next_lease_result.is_ok() and next_lease_result.get_rows()[0][0] == "own_result", "the next lease gets its own result (%s)" % [next_lease_result.get_rows() if next_lease_result.is_ok() else next_lease_result.get_error()])
	next_lease = null

	print("=== 9c. Cancelling an asynchronous operation ===")
	# cancel() stops the query on the server with KILL QUERY from a side connection: the
	# operation finishes early with the server's "query interrupted" error, and the
	# connection stays usable. A recursive CTE, because it fails when interrupted (SLEEP()
	# and BENCHMARK() just return early, without an error).
	const LONG_QUERY := "WITH RECURSIVE r(n) AS (SELECT 1 UNION ALL SELECT n + 1 FROM r WHERE n < 4000000000) SELECT COUNT(*) FROM r"
	var cancel_config := make_config(host, port, user, password, database)
	cancel_config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	cancel_config.async_timeout_ms = 0 # Only cancel() may stop the queries below.
	var cancel_session := MySQLSession.new()
	cancel_session.set_config(cancel_config)
	cancel_session.connect_db()
	cancel_session.execute_text("SET SESSION cte_max_recursion_depth = 4294967295")
	var kills_before: MySQLResult = session.execute_text("SHOW GLOBAL STATUS LIKE 'Com_kill'")
	var long_op: MySQLAsyncOperation = cancel_session.async_execute_text(LONG_QUERY)
	await create_timer(0.2).timeout
	var cancel_start := Time.get_ticks_msec()
	check(long_op.cancel(), "cancel() on a running operation returns true")
	check(long_op.cancel(), "a second cancel() returns true too")
	var cancelled: MySQLResult = await await_operation(long_op)
	var cancel_elapsed := Time.get_ticks_msec() - cancel_start
	var kills_after: MySQLResult = session.execute_text("SHOW GLOBAL STATUS LIKE 'Com_kill'")
	if kills_before.is_ok() and kills_after.is_ok():
		var kill_count := int(kills_after.get_rows()[0][1]) - int(kills_before.get_rows()[0][1])
		check(kill_count == 1, "the two cancel() calls sent exactly one KILL (%d)" % [kill_count])
	check(not cancelled.is_ok() and cancelled.get_error().get("server_message", "") == "Query execution was interrupted" and cancelled.get_error().get("is_fatal") == false, "the cancelled operation fails with the server's non-fatal \"query interrupted\" error (%s)" % [cancelled.get_error()])
	check(cancel_elapsed < 3000, "the cancelled operation finishes early (%d ms)" % [cancel_elapsed])
	check(cancel_session.is_db_connected(), "the session is still connected after a server-side cancellation")
	var after_cancel: MySQLResult = cancel_session.execute_text("SELECT 'after_cancel' AS v")
	check(after_cancel.is_ok() and after_cancel.get_rows()[0][0] == "after_cancel", "the next query gets its own result (%s)" % [after_cancel.get_rows() if after_cancel.is_ok() else after_cancel.get_error()])
	check(not long_op.cancel(), "cancel() on a finished operation returns false")
	var running_op: MySQLAsyncOperation = cancel_session.async_execute_text("SELECT 1")
	var rejected_op: MySQLAsyncOperation = cancel_session.async_execute_text("SELECT 2")
	check(not rejected_op.cancel(), "cancel() on an operation rejected as busy returns false")
	await await_operation(rejected_op)
	await await_operation(running_op)

	# cancel() works on a step of an asynchronous cursor as well: the cursor fails, and the
	# connection is free again.
	var cancel_cursor: MySQLStreamingCursor = cancel_session.async_execute_streaming(LONG_QUERY)
	var cursor_step: MySQLAsyncOperation = cancel_cursor.async_next_batch()
	await create_timer(0.2).timeout
	check(cursor_step.cancel(), "cancel() on a running cursor step returns true")
	var cancelled_step: MySQLResult = await await_operation(cursor_step)
	check(not cancelled_step.is_ok() and cancelled_step.get_error().get("server_message", "") == "Query execution was interrupted", "the cancelled cursor step fails with \"query interrupted\" (%s)" % [cancelled_step.get_error()])
	check(not cancel_cursor.is_ok() and not cancel_cursor.has_more(), "the cursor of a cancelled step reports the error and has nothing more to read")
	var after_cursor_cancel: MySQLResult = cancel_session.execute_text("SELECT 'after_cursor_cancel' AS v")
	check(after_cursor_cancel.is_ok() and after_cursor_cancel.get_rows()[0][0] == "after_cursor_cancel", "the session is usable after cancelling a cursor step (%s)" % [after_cursor_cancel.get_error()])

	# If the side connection cannot connect, cancel() falls back to cancelling locally,
	# which drops the connection. A session keeps a copy of its config, so the side
	# connection is made to fail by filling the server's max_connections: this briefly
	# refuses new connections for every client of this server. Skipped if the server allows
	# too many connections to fill quickly.
	var max_connections_result: MySQLResult = session.execute_text("SELECT @@max_connections")
	var max_connections: int = int(max_connections_result.get_rows()[0][0]) if max_connections_result.is_ok() else 0
	if max_connections <= 0 or max_connections > 1000:
		print("  SKIP the local cancel fallback: max_connections is %d." % [max_connections])
	else:
		var fallback_op: MySQLAsyncOperation = cancel_session.async_execute_text(LONG_QUERY)
		await create_timer(0.2).timeout
		var fillers: Array[MySQLSession] = []
		var filled := false
		for i in range(max_connections + 5):
			var filler := MySQLSession.new()
			filler.set_config(cancel_config)
			var filler_error: Dictionary = filler.connect_db()
			if not filler_error.is_empty():
				filled = String(filler_error.get("server_message", "")).contains("Too many connections")
				break
			fillers.append(filler)
		check(filled, "the server refuses new connections once max_connections is reached (%d opened)" % [fillers.size()])
		var fallback_start := Time.get_ticks_msec()
		check(fallback_op.cancel(), "cancel() with a side connection that cannot connect returns true")
		var fallback: MySQLResult = await await_operation(fallback_op)
		var fallback_elapsed := Time.get_ticks_msec() - fallback_start
		fillers.clear() # Frees the connections again.
		check(not fallback.is_ok() and fallback.get_error().get("is_fatal") == true, "the locally cancelled operation fails with a fatal error (%s)" % [fallback.get_error()])
		check(fallback_elapsed < 3000, "the locally cancelled operation finishes early (%d ms)" % [fallback_elapsed])
		check(not cancel_session.is_db_connected(), "the session is no longer connected after a local cancellation")
		check(cancel_session.connect_db().is_empty(), "connect_db() reconnects after a local cancellation")
		var after_fallback: MySQLResult = cancel_session.execute_text("SELECT 'after_fallback' AS v")
		check(after_fallback.is_ok() and after_fallback.get_rows()[0][0] == "after_fallback", "the reconnected session gets its own result (%s)" % [after_fallback.get_rows() if after_fallback.is_ok() else after_fallback.get_error()])
	# A local cancellation does not stop the query on the server; stop it here.
	var leftover: MySQLResult = session.execute_text("SELECT ID FROM information_schema.PROCESSLIST WHERE INFO LIKE 'WITH RECURSIVE r(n)%'")
	if leftover.is_ok():
		for leftover_row in leftover.get_rows():
			session.execute_text("KILL QUERY %d" % [int(leftover_row[0])])
	cancel_session = null

	print("=== 10. Connection pool ===")
	var pool := MySQLPool.new()
	pool.set_config(config)
	pool.max_size = 2
	var pool_session_a: MySQLSession = pool.acquire()
	var pool_session_b: MySQLSession = pool.acquire()
	check(pool_session_a != null and pool_session_b != null, "acquire() twice within max_size does not block")
	pool_session_a.connect_db()
	pool_session_b.connect_db()
	var pool_result_a: MySQLResult = pool_session_a.execute_text("SELECT 1")
	var pool_result_b: MySQLResult = pool_session_b.execute_text("SELECT 1")
	check(pool_result_a.is_ok() and pool_result_b.is_ok(), "the two pooled sessions work independently")
	var pool_async_a: MySQLAsyncOperation = pool_session_a.async_execute_text("SELECT 'pool_async'")
	var pool_async_a_result: MySQLResult = await await_operation(pool_async_a)
	check(pool_async_a_result.is_ok(), "a pooled session runs an asynchronous operation (%s)" % [pool_async_a_result.get_error()])
	# The pool hands back the connection released last, so release session A (the one that
	# ran an asynchronous operation, and whose I/O context was therefore stopped when it was
	# destroyed) after session B. The next session then reuses exactly that connection, and
	# its asynchronous call must still work.
	pool_session_b = null
	pool_session_a = null
	var pool_session_c: MySQLSession = pool.acquire()
	check(pool_session_c != null, "acquire() after releasing the previous ones returns a connection (recycled from the pool)")
	if pool_session_c != null:
		var reuse_result: MySQLResult = pool_session_c.execute_text("SELECT 'recycled'")
		check(reuse_result.is_ok(), "the recycled connection is still connected and usable (%s)" % [reuse_result.get_error()])
		var pool_async_c: MySQLAsyncOperation = pool_session_c.async_execute_text("SELECT 'recycled_async'")
		var pool_async_c_result: MySQLResult = await await_operation(pool_async_c)
		check(pool_async_c_result.is_ok() and pool_async_c_result.get_rows()[0][0] == "recycled_async", "an asynchronous operation works on a recycled pooled connection (%s)" % [pool_async_c_result.get_error()])

	print("=== 10b. Pool used from several threads ===")
	# More threads than max_size: the extra ones must block in acquire() until a session is
	# released, and every thread must finish with a correct result.
	var threaded_pool := MySQLPool.new()
	threaded_pool.set_config(config)
	threaded_pool.max_size = 2
	var worker_threads: Array[Thread] = []
	for i in range(6):
		var worker_thread := Thread.new()
		worker_thread.start(_pool_worker.bind(threaded_pool, i))
		worker_threads.append(worker_thread)
	var worker_successes := 0
	for worker_thread in worker_threads:
		worker_successes += int(worker_thread.wait_to_finish())
	check(worker_successes == 6, "6 threads sharing a pool of 2 connections all got a correct result (%d of 6)" % [worker_successes])

	print("=== 10c. Pooled connection health ===")
	# Regression test: a session dropped while an asynchronous operation is still running
	# must not hand a busy connection back to the pool. With max_size = 1, a poisoned
	# connection used to make every later lease fail with "operation in progress".
	var busy_pool := MySQLPool.new()
	busy_pool.set_config(config)
	busy_pool.max_size = 1
	var abandoned_session: MySQLSession = busy_pool.acquire()
	abandoned_session.connect_db()
	abandoned_session.async_execute_text("SELECT SLEEP(1)")
	abandoned_session = null # Dropped with the operation still in flight.
	var after_abandon: MySQLSession = busy_pool.acquire()
	if not after_abandon.is_db_connected():
		after_abandon.connect_db()
	var after_abandon_result: MySQLResult = after_abandon.execute_text("SELECT 'healthy'")
	check(after_abandon_result.is_ok(), "a lease after an abandoned asynchronous operation gets a usable connection (%s)" % [after_abandon_result.get_error()])
	after_abandon = null

	# Same, with a second call rejected while the first still runs, and awaited before the
	# session is dropped: the rejected call must not replace the running operation as the
	# one the session tracks, or the busy connection would go back to the pool.
	var twice_session: MySQLSession = busy_pool.acquire()
	if not twice_session.is_db_connected():
		twice_session.connect_db()
	twice_session.async_execute_text("SELECT SLEEP(1)")
	var twice_rejected: MySQLAsyncOperation = twice_session.async_execute_text("SELECT 1")
	var twice_rejected_result: MySQLResult = await await_operation(twice_rejected)
	check(not twice_rejected_result.is_ok(), "the second back-to-back asynchronous call is rejected")
	twice_session = null # Dropped with the first operation still in flight.
	var after_twice: MySQLSession = busy_pool.acquire()
	if not after_twice.is_db_connected():
		after_twice.connect_db()
	var after_twice_result: MySQLResult = after_twice.execute_text("SELECT 'healthy'")
	check(after_twice_result.is_ok(), "a lease after two back-to-back asynchronous calls gets a usable connection (%s)" % [after_twice_result.get_error()])
	after_twice = null

	# Regression test: prepared statements must not leak on the server across leases of
	# the same pooled connection (the statement cache belongs to the connection, not to
	# the session, and the reset between leases closes them on the server). The
	# server-wide count must not grow after the first lease.
	var stmt_pool := MySQLPool.new()
	stmt_pool.set_config(config)
	stmt_pool.max_size = 1
	var stmt_counts: Array[int] = []
	for lease in range(3):
		var leased: MySQLSession = stmt_pool.acquire()
		if not leased.is_db_connected():
			leased.connect_db()
		for k in range(5):
			leased.execute_prepared("SELECT ? + %d" % [k], [lease])
		leased = null
		var count_result: MySQLResult = session.execute_text("SHOW GLOBAL STATUS LIKE 'Prepared_stmt_count'")
		stmt_counts.append(int(count_result.get_rows()[0][1]) if count_result.is_ok() else -1)
	check(stmt_counts[0] >= 0 and stmt_counts[1] == stmt_counts[0] and stmt_counts[2] == stmt_counts[0], "prepared statements do not leak across leases of a pooled connection (Prepared_stmt_count %s)" % [stmt_counts])

	# A recycled connection is reset before a new lease: nothing the previous lease left
	# on the server (transaction, temporary table, variables, session settings, USE) may
	# reach the next one.
	var reset_pool := MySQLPool.new()
	reset_pool.set_config(config)
	reset_pool.max_size = 1
	var dirty: MySQLSession = reset_pool.acquire()
	dirty.connect_db()
	var fresh_collation: MySQLResult = dirty.execute_text("SELECT @@SESSION.collation_connection")
	dirty.execute_text("SET @smoke_leaked_variable = 42")
	dirty.execute_text("CREATE TEMPORARY TABLE smoke_leaked_temp (x INT)")
	dirty.execute_text("SET SESSION sql_mode = 'NO_BACKSLASH_ESCAPES'")
	dirty.execute_text("SET NAMES utf8mb4 COLLATE utf8mb4_bin")
	dirty.execute_text("START TRANSACTION")
	dirty.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('leaked_uncommitted')")
	var dirty_use: MySQLResult = dirty.execute_text("USE information_schema")
	check(dirty_use.is_ok(), "the dirty lease switched to another schema (%s)" % [dirty_use.get_error()])
	dirty.execute_prepared("SELECT ? + 100", [1])
	dirty = null # Released with all of the above still in place.
	var clean: MySQLSession = reset_pool.acquire()
	check(clean.is_db_connected(), "the recycled connection is still connected after the reset")
	var leaked_state: MySQLResult = clean.execute_text(
		"SELECT @smoke_leaked_variable, @@SESSION.sql_mode LIKE '%NO_BACKSLASH_ESCAPES%', DATABASE(), @@SESSION.collation_connection, @@SESSION.autocommit"
	)
	if leaked_state.is_ok():
		var state_row: Array = leaked_state.get_rows()[0]
		check(state_row[0] == null, "a user variable does not reach the next lease (%s)" % [state_row[0]])
		check(int(state_row[1]) == 0, "a session sql_mode does not reach the next lease")
		check(state_row[2] == database, "the next lease is back on the configured schema (%s)" % [state_row[2]])
		check(fresh_collation.is_ok() and state_row[3] == fresh_collation.get_rows()[0][0], "the next lease has the collation of a fresh connection (%s, fresh %s)" % [state_row[3], fresh_collation.get_rows()[0][0] if fresh_collation.is_ok() else "?"])
		check(int(state_row[4]) == 1, "autocommit is back on in the next lease")
	else:
		check(false, "reading the session state of the next lease (%s)" % [leaked_state.get_error()])
	var leaked_temp: MySQLResult = clean.execute_text("SELECT * FROM smoke_leaked_temp")
	check(not leaked_temp.is_ok(), "a temporary table does not reach the next lease")
	var leaked_row: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'leaked_uncommitted'")
	check(leaked_row.is_ok() and int(leaked_row.get_rows()[0][0]) == 0, "an open transaction of the previous lease was rolled back")
	var after_reset_formatted: MySQLResult = clean.execute_formatted("SELECT ? AS v", ["it's"])
	check(after_reset_formatted.is_ok() and after_reset_formatted.get_rows()[0][0] == "it's", "execute_formatted works after the reset (%s)" % [after_reset_formatted.get_error()])
	var after_reset_prepared: MySQLResult = clean.execute_prepared("SELECT ? + 100", [1])
	check(after_reset_prepared.is_ok() and after_reset_prepared.get_rows()[0][0] == 101, "a statement prepared by the previous lease is prepared again (%s)" % [after_reset_prepared.get_error()])
	var connection_id_result: MySQLResult = clean.execute_text("SELECT CONNECTION_ID()")
	clean = null

	# If the reset fails (here: the idle connection was killed on the server), the lease
	# gets a closed connection, and the usual is_db_connected()/connect_db() check recovers.
	if connection_id_result.is_ok():
		session.execute_text("KILL %d" % [int(connection_id_result.get_rows()[0][0])])
	var after_kill: MySQLSession = reset_pool.acquire()
	check(not after_kill.is_db_connected(), "a lease whose reset failed reports is_db_connected() == false")
	check(after_kill.connect_db().is_empty(), "connect_db() recovers a lease whose reset failed")
	var after_kill_result: MySQLResult = after_kill.execute_text("SELECT 'recovered'")
	check(after_kill_result.is_ok(), "the reconnected lease works (%s)" % [after_kill_result.get_error()])
	after_kill = null

	print("=== 11. execute_script ===")
	var script_off: Array = session.execute_script("SELECT 1")
	check(script_off.size() == 1 and not (script_off[0] as MySQLResult).is_ok(), "execute_script is refused while allow_sql_script_execution is off (the default)")
	# A session keeps a copy of its config: scripts need a session whose config allows them.
	var script_config := make_config(host, port, user, password, database)
	script_config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	script_config.allow_sql_script_execution = true
	var script_session := MySQLSession.new()
	script_session.set_config(script_config)
	script_session.connect_db()
	var script_result: Array = script_session.execute_script(
		"""
		INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('script_1');
		INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('script_2;with_semicolon');
		SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt LIKE 'script\\_%';
		"""
	)
	check(script_result.size() == 3, "execute_script returned 3 results (one per statement)")
	var all_script_ok := true
	for r in script_result:
		if not (r as MySQLResult).is_ok():
			all_script_ok = false
	check(all_script_ok, "every statement of the script ran without error")
	if script_result.size() == 3:
		var last: MySQLResult = script_result[2]
		check(int(last.get_rows()[0][0]) == 2, "both statements of the script really inserted (COUNT = %s)" % [last.get_rows()[0][0]])
		var literal: MySQLResult = session.execute_text("SELECT txt FROM t_mysql_module_smoke_test WHERE txt LIKE 'script_2%'")
		check(literal.is_ok() and literal.get_rows().size() == 1 and literal.get_rows()[0][0] == "script_2;with_semicolon", "a semicolon inside a quoted literal did not split the statement")
	var failing_script: Array = script_session.execute_script("SELECT 1; SELEC nothing; SELECT 3")
	check(failing_script.size() == 2 and (failing_script[0] as MySQLResult).is_ok() and not (failing_script[1] as MySQLResult).is_ok(), "the script stops at the first failing statement (%d results)" % [failing_script.size()])
	var commented_script: Array = script_session.execute_script(
		"""
		-- header; not a statement
		/* block; comment */ SELECT 1;
		# hash; comment
		SELECT 2; -- trailer; still a comment
		"""
	)
	var commented_ok := commented_script.size() == 2
	for r in commented_script:
		if not (r as MySQLResult).is_ok():
			commented_ok = false
	check(commented_ok, "semicolons inside comments do not split the script, comment-only fragments are dropped (%d results)" % [commented_script.size()])
	# The script itself turns NO_BACKSLASH_ESCAPES on and off: each statement must be split
	# with the SQL mode in force when it runs, not the one from before the script started.
	var mode_script: Array = script_session.execute_script(
		"""
		SET @smoke_script_sql_mode = @@SESSION.sql_mode;
		SET SESSION sql_mode = CONCAT(@@SESSION.sql_mode, ',NO_BACKSLASH_ESCAPES');
		SELECT 'C:\\' AS p;
		SET SESSION sql_mode = @smoke_script_sql_mode;
		SELECT 'it\\'s; fine' AS q;
		"""
	)
	var mode_script_ok := mode_script.size() == 5
	for r in mode_script:
		if not (r as MySQLResult).is_ok():
			mode_script_ok = false
	check(mode_script_ok, "execute_script re-reads the SQL mode between statements (%d results)" % [mode_script.size()])
	if mode_script_ok:
		check((mode_script[2] as MySQLResult).get_rows()[0][0] == "C:\\", "a backslash is plain text after the script turns on NO_BACKSLASH_ESCAPES")
		check((mode_script[4] as MySQLResult).get_rows()[0][0] == "it's; fine", "a backslash escapes again after the script restores the SQL mode")

	print("=== 12. Multiple resultsets ===")
	var multi_config := make_config(host, port, user, password, database)
	multi_config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	multi_config.allow_multi_queries = true
	var multi_session := MySQLSession.new()
	multi_session.set_config(multi_config)
	multi_session.connect_db()
	var multi_result: MySQLResult = multi_session.execute_text("SELECT 1 AS a; SELECT 2 AS b, 3 AS c")
	check(multi_result.is_ok() and multi_result.get_resultset_count() == 2, "allow_multi_queries returns both resultsets (%d)" % [multi_result.get_resultset_count()])
	if multi_result.is_ok() and multi_result.get_resultset_count() == 2:
		check(multi_result.get_column_names(1) == PackedStringArray(["b", "c"]), "each resultset keeps its own columns (%s)" % [multi_result.get_column_names(1)])
		check(multi_result.get_rows(1)[0] == [2, 3], "the second resultset has its own rows (%s)" % [multi_result.get_rows(1)[0]])
	var no_multi: MySQLResult = session.execute_text("SELECT 1; SELECT 2")
	check(not no_multi.is_ok(), "multiple statements are refused by default (allow_multi_queries is off)")
	# Regression test: streaming a multi-statement query used to loop forever once the first
	# resultset was exhausted (has_more() stayed true between resultsets, and close() spun
	# the same way). A hang here means that bug is back.
	var multi_cursor: MySQLStreamingCursor = multi_session.execute_streaming("SELECT 1; SELECT 2")
	var multi_batches := 0
	while multi_cursor.has_more() and multi_batches < 1000:
		multi_cursor.next_batch()
		multi_batches += 1
	check(multi_batches < 1000, "streaming a multi-statement query ends (%d batches)" % [multi_batches])
	multi_cursor.close()
	var after_multi_stream: MySQLResult = multi_session.execute_text("SELECT 3")
	check(after_multi_stream.is_ok(), "the connection is usable after closing a multi-statement stream (%s)" % [after_multi_stream.get_error()])
	# The same on an asynchronous cursor: it reads the first resultset only, and
	# async_close() drains the others.
	var multi_async: MySQLStreamingCursor = multi_session.async_execute_streaming("SELECT 1 AS a; SELECT 2 AS b")
	var multi_async_rows: Array = []
	var multi_async_batches := 0
	while multi_async.has_more() and multi_async_batches < 1000:
		var multi_async_batch: MySQLResult = await multi_async.async_next_batch().completed
		if multi_async_batch.is_ok():
			multi_async_rows.append_array(multi_async_batch.get_rows())
		multi_async_batches += 1
	check(multi_async_batches < 1000 and multi_async_rows == [[1]], "an asynchronous multi-statement stream reads the first resultset and ends (%s in %d batches)" % [multi_async_rows, multi_async_batches])
	var multi_async_closed: MySQLResult = await multi_async.async_close().completed
	check(multi_async_closed.is_ok(), "async_close() drains the other resultsets (%s)" % [multi_async_closed.get_error()])
	var after_multi_async: MySQLResult = multi_session.execute_text("SELECT 3")
	check(after_multi_async.is_ok() and after_multi_async.get_rows() == [[3]], "the connection is usable after an asynchronous multi-statement stream (%s)" % [after_multi_async.get_error()])
	multi_session.close_db()

	print("=== 13. Cleanup ===")
	var drop_result: MySQLResult = session.execute_text("DROP TABLE t_mysql_module_smoke_test")
	check(drop_result.is_ok(), "final DROP TABLE (%s)" % [drop_result.get_error()])

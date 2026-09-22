# SPDX-License-Identifier: MIT
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
# Android smoke test runs it: `--script`, baked into the export as
# command_line/extra_args, reaches the native layer (confirmed in logcat) but silently
# never executes on Android in this Godot build (reproduced on two different devices) —
# main_loop_type is the workaround. See notes/android_smoke_project/ in the mysql_dev
# workspace (outside this repository).

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
	return ""


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
	config.json_result_mode = MySQLConfig.PARSED_VARIANT
	var select_parsed: MySQLResult = session.execute_text("SELECT js FROM t_mysql_module_smoke_test WHERE id = 1")
	var js_parsed_variant: Variant = select_parsed.get_rows()[0][0]
	if is_mariadb:
		check(typeof(js_parsed_variant) == TYPE_STRING, "json_result_mode = PARSED_VARIANT on MariaDB: no JSON type in the protocol, behaves like RAW_STRING (%s)" % [js_parsed_variant])
	else:
		check(typeof(js_parsed_variant) == TYPE_DICTIONARY and js_parsed_variant.get("a") == 1, "json_result_mode = PARSED_VARIANT on MySQL: a native JSON type is reported, parsed eagerly into a Dictionary (%s)" % [js_parsed_variant])
	config.json_result_mode = MySQLConfig.LAZY_PARSED_VARIANT # Back to the default.

	print("=== 3c. Result shape ===")
	var duplicate_cols: MySQLResult = session.execute_text("SELECT 1 AS a, 2 AS a")
	check(duplicate_cols.is_ok() and duplicate_cols.get_column_names() == PackedStringArray(["a", "a"]), "duplicate column names are preserved (%s)" % [duplicate_cols.get_column_names()])
	check(duplicate_cols.get_rows()[0] == [1, 2], "duplicate columns keep both values in order (%s)" % [duplicate_cols.get_rows()[0]])
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

	print("=== 7b. Automatic rollback (destructor) ===")
	var tx_auto := session.begin_transaction()
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_auto_rollback')")
	tx_auto = null # Drops the only reference without commit() or rollback(), so the destructor runs.
	var after_auto: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'tx_auto_rollback'")
	check(after_auto.is_ok() and int(after_auto.get_rows()[0][0]) == 0, "the automatic rollback (destructor without commit or rollback) undid the row")

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

	var bad_cursor: MySQLStreamingCursor = session.execute_streaming("SELEC nothing")
	check(not bad_cursor.is_ok() and not bad_cursor.has_more(), "a streaming syntax error is reported and the cursor has nothing to read")

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

	var async_not_connected: MySQLAsyncOperation = not_connected.async_execute_text("SELECT 1")
	var async_not_connected_result: MySQLResult = await await_operation(async_not_connected)
	check(not async_not_connected_result.is_ok(), "an asynchronous call on a session without config fails explicitly")

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

	print("=== 11. execute_script ===")
	var script_off: Array = session.execute_script("SELECT 1")
	check(script_off.size() == 1 and not (script_off[0] as MySQLResult).is_ok(), "execute_script is refused while allow_sql_script_execution is off (the default)")
	config.allow_sql_script_execution = true
	var script_result: Array = session.execute_script(
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
	var failing_script: Array = session.execute_script("SELECT 1; SELEC nothing; SELECT 3")
	check(failing_script.size() == 2 and (failing_script[0] as MySQLResult).is_ok() and not (failing_script[1] as MySQLResult).is_ok(), "the script stops at the first failing statement (%d results)" % [failing_script.size()])

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
	multi_session.close_db()

	print("=== 13. Cleanup ===")
	var drop_result: MySQLResult = session.execute_text("DROP TABLE t_mysql_module_smoke_test")
	check(drop_result.is_ok(), "final DROP TABLE (%s)" % [drop_result.get_error()])

# type_conversion.gd
#
# Cost of turning each MySQL type into a Godot Variant: the same number of rows, one
# column of a given type, read with execute_text. The values are stored in a temporary
# table first, so the timed query only reads stored rows and the server does the same work
# for every type. The first row, INT, is the baseline; "vs INT" is the extra time per row
# compared to it: the conversion, plus the larger wire size of the wide types. JSON is read
# in the three json_result_mode modes, plus LAZY_PARSED_VARIANT with get_parsed_json()
# called on every row. Each case runs 3 times; the table shows the fastest run.

extends Control

@onready var output: RichTextLabel = $Output

const ROWS := 50000
const RUNS := 3
const FILL := "CREATE TEMPORARY TABLE bench_values AS WITH RECURSIVE r(n) AS (SELECT 1 UNION ALL SELECT n + 1 FROM r WHERE n < %d) SELECT %s AS v FROM r"

const CASES := [
	["INT", "n", -1],
	["BIGINT", "n * 1000000000", -1],
	["DOUBLE", "n * 1.5e0", -1],
	["DECIMAL (as String)", "n / 3", -1],
	["VARCHAR (100 chars)", "CONCAT(REPEAT('x', 94), LPAD(n, 6, '0'))", -1],
	["BLOB (1 KiB)", "CAST(REPEAT('x', 1024) AS BINARY)", -1],
	["DATE", "DATE('2026-01-01') + INTERVAL (n % 3650) DAY", -1],
	["DATETIME(6)", "TIMESTAMP('2026-01-01 00:00:00.000000') + INTERVAL n MICROSECOND", -1],
	["TIME(6)", "CAST(SEC_TO_TIME(n % 86400) AS TIME(6))", -1],
	["JSON, RAW_STRING", "JSON_OBJECT('id', n, 'name', CONCAT('row ', n), 'tags', JSON_ARRAY(1, 2, 3))", MySQLConfig.RAW_STRING],
	["JSON, PARSED_VARIANT", "JSON_OBJECT('id', n, 'name', CONCAT('row ', n), 'tags', JSON_ARRAY(1, 2, 3))", MySQLConfig.PARSED_VARIANT],
	["JSON, LAZY_PARSED_VARIANT", "JSON_OBJECT('id', n, 'name', CONCAT('row ', n), 'tags', JSON_ARRAY(1, 2, 3))", MySQLConfig.LAZY_PARSED_VARIANT],
	["JSON, LAZY + get_parsed_json() on every row", "JSON_OBJECT('id', n, 'name', CONCAT('row ', n), 'tags', JSON_ARRAY(1, 2, 3))", -2],
]


func _ready() -> void:
	await get_tree().process_frame
	run_benchmark()
	Bench.quit_if_headless(self)


func run_benchmark() -> void:
	var config := Bench.load_config()
	if config == null:
		log_line("[color=red]Could not load res://config.ini.[/color]")
		return
	var session := Bench.connect_session(config, log_line)
	if session == null:
		return
	session.execute_text("SET SESSION cte_max_recursion_depth = %d" % ROWS)

	log_line("## Type conversion\n")
	log_line(Bench.environment_summary(session) + "\n")
	log_line("%d rows of one column per case, stored in a temporary table and read with execute_text. Fastest of %d runs.\n" % [ROWS, RUNS])

	var rows := []
	var baseline_usec := 0
	for entry in CASES:
		var mode: int = entry[2]
		if mode == -2:
			config.json_result_mode = MySQLConfig.LAZY_PARSED_VARIANT
		elif mode >= 0:
			config.json_result_mode = mode
		session.execute_text("DROP TEMPORARY TABLE IF EXISTS bench_values")
		var fill: MySQLResult = session.execute_text(FILL % [ROWS, entry[1]])
		if not fill.is_ok():
			log_line("[color=red]%s: filling the table failed: %s[/color]" % [entry[0], fill.get_error()])
			return
		var sql := "SELECT v FROM bench_values"
		var best := -1
		for run in range(RUNS):
			var start := Time.get_ticks_usec()
			var result: MySQLResult = session.execute_text(sql)
			if not result.is_ok():
				log_line("[color=red]%s failed: %s[/color]" % [entry[0], result.get_error()])
				return
			if mode == -2:
				for row in range(result.get_rows().size()):
					result.get_parsed_json(0, row, 0)
			var elapsed := Time.get_ticks_usec() - start
			if result.get_rows().size() != ROWS:
				log_line("[color=red]%s returned %d rows[/color]" % [entry[0], result.get_rows().size()])
				return
			best = elapsed if best < 0 else mini(best, elapsed)
		session.execute_text("DROP TEMPORARY TABLE bench_values")
		if baseline_usec == 0:
			baseline_usec = best
		rows.append([
			entry[0],
			"%.1f" % (best / 1000.0),
			"%.2f" % (float(best) / ROWS),
			"%+.2f" % (float(best - baseline_usec) / ROWS),
			"%.0f" % (ROWS * 1000000.0 / best),
		])
	log_line(Bench.markdown_table(["Type", "Total (ms)", "µs/row", "vs INT (µs/row)", "Rows/s"], rows))
	session.close_db()


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print(text)

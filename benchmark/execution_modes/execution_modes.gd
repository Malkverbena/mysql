# execution_modes.gd
#
# Latency of the three ways to run SQL, on a small query that returns one row of three
# values: execute_text (the values written in the SQL), execute_formatted (the values
# escaped client-side into the SQL text) and execute_prepared (a server-side prepared
# statement). execute_prepared is measured twice: with its statement found in the
# connection's cache (the usual case), and with a cache so small that every call prepares
# the statement again and closes the evicted one (the worst case).

extends Control

@onready var output: RichTextLabel = $Output

const ITERATIONS := 2000
const WARMUP := 200


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

	# A second session whose cache holds one statement: alternating two statements evicts
	# the other one on every call.
	var miss_config := Bench.load_config()
	miss_config.statement_cache_size = 1
	var miss_session := Bench.connect_session(miss_config, log_line)
	if miss_session == null:
		return

	var cases := [
		["execute_text", func(): return session.execute_text("SELECT 1, 'text', 2.5")],
		["execute_formatted", func(): return session.execute_formatted("SELECT ?, ?, ?", [1, "text", 2.5])],
		["execute_prepared (cached statement)", func(): return session.execute_prepared("SELECT ?, ?, ?", [1, "text", 2.5])],
		["execute_prepared (prepared on every call)", _alternating_prepared(miss_session)],
	]

	log_line("## Execution modes\n")
	log_line(Bench.environment_summary(session) + "\n")
	log_line("%d timed calls per case, after %d untimed warm-up calls. One row of three values per call.\n" % [ITERATIONS, WARMUP])

	var rows := []
	for entry in cases:
		var body: Callable = entry[1]
		var check: MySQLResult = body.call()
		if not check.is_ok():
			log_line("[color=red]%s failed: %s[/color]" % [entry[0], check.get_error()])
			return
		rows.append(Bench.latency_row(entry[0], Bench.time_calls(body, ITERATIONS, WARMUP)))
	log_line(Bench.markdown_table(Bench.LATENCY_HEADERS, rows))

	miss_session.close_db()
	session.close_db()


# Alternates between two different statements on a session with a one-statement cache.
func _alternating_prepared(miss_session: MySQLSession) -> Callable:
	var state := {"flip": false}
	return func():
		state.flip = not state.flip
		var sql := "SELECT ?, ?, ? /* a */" if state.flip else "SELECT ?, ?, ? /* b */"
		return miss_session.execute_prepared(sql, [1, "text", 2.5])


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print(text)

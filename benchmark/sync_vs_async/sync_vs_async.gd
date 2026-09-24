# sync_vs_async.gd
#
# What the asynchronous path costs, and what it buys.
#
# 1. Round trip of a trivial query, synchronous and asynchronous. An asynchronous result is
#    delivered on the main thread on the next frame, so awaiting it one call at a time is
#    bound by the frame period, printed above the tables.
# 2. The longest frame while a heavy query runs. The synchronous call blocks the main thread
#    for the whole query, stalling that frame; the asynchronous one leaves the frames alone.

extends Control

@onready var output: RichTextLabel = $Output

const ROUND_TRIPS := 500
const HEAVY_ROWS := 300000
const HEAVY_QUERY := "WITH RECURSIVE r(n) AS (SELECT 1 UNION ALL SELECT n + 1 FROM r WHERE n < %d) SELECT n, CONCAT('row ', n) FROM r"

var tracking := false
var tracked_frames := 0
var longest_frame_usec := 0
var last_frame_usec := 0


func _process(_delta: float) -> void:
	var now := Time.get_ticks_usec()
	if tracking:
		tracked_frames += 1
		longest_frame_usec = maxi(longest_frame_usec, now - last_frame_usec)
	last_frame_usec = now


func _ready() -> void:
	await get_tree().process_frame
	await run_benchmark()
	Bench.quit_if_headless(self)


func run_benchmark() -> void:
	var config := Bench.load_config()
	if config == null:
		log_line("[color=red]Could not load res://config.ini.[/color]")
		return
	var session := Bench.connect_session(config, log_line)
	if session == null:
		return
	session.execute_text("SET SESSION cte_max_recursion_depth = %d" % HEAVY_ROWS)

	log_line("## Synchronous vs asynchronous\n")
	log_line(Bench.environment_summary(session) + "\n")
	var frame_period: float = await Bench.measure_frame_period(get_tree())
	log_line("Display server: %s, measured frame period: %.2f ms.\n" % [DisplayServer.get_name(), frame_period])

	# 1. Round trips.
	var sync_samples := Bench.time_calls(func(): return session.execute_text("SELECT 1"), ROUND_TRIPS, 50)
	var async_samples := PackedInt64Array()
	async_samples.resize(ROUND_TRIPS)
	for i in range(ROUND_TRIPS):
		var start := Time.get_ticks_usec()
		var result: MySQLResult = await session.async_execute_text("SELECT 1").completed
		async_samples[i] = Time.get_ticks_usec() - start
		if not result.is_ok():
			log_line("[color=red]async_execute_text failed: %s[/color]" % [result.get_error()])
			return
	async_samples.sort()
	log_line("### Round trip of `SELECT 1` (%d calls, one at a time)\n" % ROUND_TRIPS)
	log_line(Bench.markdown_table(Bench.LATENCY_HEADERS, [
		Bench.latency_row("execute_text", sync_samples),
		Bench.latency_row("async_execute_text + await", async_samples),
	]) + "\n")

	# 2. Longest frame during a heavy query.
	var rows := []
	await get_tree().process_frame
	_start_tracking()
	var sync_start := Time.get_ticks_usec()
	var sync_result: MySQLResult = session.execute_text(HEAVY_QUERY % HEAVY_ROWS)
	var sync_total := Time.get_ticks_usec() - sync_start
	await get_tree().process_frame # The frame that the call above stalled ends here.
	await get_tree().process_frame
	tracking = false
	rows.append(["execute_text", "%.1f" % (sync_total / 1000.0), str(tracked_frames), "%.1f" % (longest_frame_usec / 1000.0), str(sync_result.get_rows().size() if sync_result.is_ok() else -1)])
	sync_result = null

	_start_tracking()
	var async_start := Time.get_ticks_usec()
	var async_result: MySQLResult = await session.async_execute_text(HEAVY_QUERY % HEAVY_ROWS).completed
	var async_total := Time.get_ticks_usec() - async_start
	tracking = false
	rows.append(["async_execute_text + await", "%.1f" % (async_total / 1000.0), str(tracked_frames), "%.1f" % (longest_frame_usec / 1000.0), str(async_result.get_rows().size() if async_result.is_ok() else -1)])

	log_line("### Longest frame while a %d-row query runs\n" % HEAVY_ROWS)
	log_line(Bench.markdown_table(["Case", "Total (ms)", "Frames", "Longest frame (ms)", "Rows"], rows))
	session.close_db()


func _start_tracking() -> void:
	tracked_frames = 0
	longest_frame_usec = 0
	last_frame_usec = Time.get_ticks_usec()
	tracking = true


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print(text)

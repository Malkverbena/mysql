# streaming.gd
#
# Three ways to read a large result: all at once (execute_text), in synchronous batches
# (execute_streaming + next_batch) and in asynchronous batches (async_execute_streaming +
# async_next_batch) for several values of MySQLConfig.async_batch_rows. For each: total
# time, number of batches, frames drawn, longest frame, and the peak of memory held by the
# rows (Godot's static memory above the level before the read; needs a debug build, the
# editor or a debug export template).

extends Control

@onready var output: RichTextLabel = $Output

const ROWS := 200000
const QUERY := "WITH RECURSIVE r(n) AS (SELECT 1 UNION ALL SELECT n + 1 FROM r WHERE n < %d) SELECT n, CONCAT('row ', n) FROM r"
const BATCH_SIZES := [100, 500, 2000, 10000]

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
	session.execute_text("SET SESSION cte_max_recursion_depth = %d" % ROWS)
	var sql := QUERY % ROWS

	log_line("## Streaming\n")
	log_line(Bench.environment_summary(session) + "\n")
	var frame_period: float = await Bench.measure_frame_period(get_tree())
	log_line("%d rows of (INT, VARCHAR). Display server: %s, measured frame period: %.2f ms (each asynchronous batch takes at least one frame).\n" % [ROWS, DisplayServer.get_name(), frame_period])
	var rows := []

	# All at once.
	await get_tree().process_frame
	var baseline := OS.get_static_memory_usage()
	_start_tracking()
	var start := Time.get_ticks_usec()
	var whole: MySQLResult = session.execute_text(sql)
	var total := Time.get_ticks_usec() - start
	var held := OS.get_static_memory_usage() - baseline
	await get_tree().process_frame
	tracking = false
	rows.append(_row("execute_text (whole result)", total, 1, whole.get_rows().size() if whole.is_ok() else -1, held))
	whole = null

	# Synchronous batches.
	await get_tree().process_frame
	baseline = OS.get_static_memory_usage()
	var peak := 0
	_start_tracking()
	start = Time.get_ticks_usec()
	var cursor := session.execute_streaming(sql)
	var read := 0
	var batches := 0
	while cursor.has_more():
		var batch := cursor.next_batch()
		read += batch.size()
		batches += 1
		peak = maxi(peak, OS.get_static_memory_usage() - baseline)
	total = Time.get_ticks_usec() - start
	cursor.close()
	await get_tree().process_frame
	tracking = false
	rows.append(_row("next_batch", total, batches, read, peak))

	# Asynchronous batches.
	for batch_rows in BATCH_SIZES:
		config.async_batch_rows = batch_rows
		await get_tree().process_frame
		baseline = OS.get_static_memory_usage()
		peak = 0
		_start_tracking()
		start = Time.get_ticks_usec()
		var async_cursor := session.async_execute_streaming(sql)
		read = 0
		batches = 0
		while async_cursor.has_more():
			var result: MySQLResult = await async_cursor.async_next_batch().completed
			if not result.is_ok():
				log_line("[color=red]async_next_batch failed: %s[/color]" % [result.get_error()])
				return
			read += result.get_rows().size()
			batches += 1
			peak = maxi(peak, OS.get_static_memory_usage() - baseline)
		total = Time.get_ticks_usec() - start
		tracking = false
		rows.append(_row("async_next_batch, async_batch_rows = %d" % batch_rows, total, batches, read, peak))

	log_line(Bench.markdown_table(["Case", "Total (ms)", "Rows/s", "Batches", "Frames", "Longest frame (ms)", "Peak memory (MiB)", "Rows"], rows))
	session.close_db()


func _row(label: String, total_usec: int, batches: int, read: int, memory_bytes: int) -> Array:
	return [
		label,
		"%.1f" % (total_usec / 1000.0),
		"%.0f" % (read * 1000000.0 / total_usec if total_usec > 0 else 0.0),
		str(batches),
		str(tracked_frames),
		"%.1f" % (longest_frame_usec / 1000.0),
		"%.1f" % (memory_bytes / 1048576.0),
		str(read),
	]


func _start_tracking() -> void:
	tracked_frames = 0
	longest_frame_usec = 0
	last_frame_usec = Time.get_ticks_usec()
	tracking = true


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print(text)

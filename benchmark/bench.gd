# bench.gd
#
# Shared by every benchmark scene: loads res://config.ini into a MySQLConfig, times calls,
# and prints the results as Markdown tables. It is not a benchmark on its own.

extends RefCounted

class_name Bench


static func load_config() -> MySQLConfig:
	var ini := ConfigFile.new()
	var err := ini.load("res://config.ini")
	if err != OK:
		push_error("Could not load res://config.ini (error %d). See README.md." % err)
		return null

	var config := MySQLConfig.new()
	config.host = ini.get_value("mysql", "host", "127.0.0.1")
	config.port = ini.get_value("mysql", "port", 3306)
	config.user = ini.get_value("mysql", "user", "")
	config.set_password(ini.get_value("mysql", "password", ""))
	config.database = ini.get_value("mysql", "database", "")
	# TCP_TLS_DISABLED is only for a local benchmark (see the README), and it keeps TLS
	# encryption out of the numbers. Production use should keep the default,
	# TCP_TLS_REQUIRED.
	config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	return config


# A connected session, or null (the error goes to `logger`).
static func connect_session(config: MySQLConfig, logger: Callable) -> MySQLSession:
	var session := MySQLSession.new()
	session.set_config(config)
	var err: Dictionary = session.connect_db()
	if not err.is_empty():
		logger.call("[color=red]connect_db() failed: %s[/color]" % [err])
		return null
	return session


# Runs `body` `warmup` times untimed, then `iterations` times timed one by one. Returns
# the per-call durations in microseconds, sorted.
static func time_calls(body: Callable, iterations: int, warmup: int = 0) -> PackedInt64Array:
	for i in range(warmup):
		body.call()
	var samples := PackedInt64Array()
	samples.resize(iterations)
	for i in range(iterations):
		var start := Time.get_ticks_usec()
		body.call()
		samples[i] = Time.get_ticks_usec() - start
	samples.sort()
	return samples


# Percentile of sorted samples, 0..100.
static func percentile(sorted_samples: PackedInt64Array, p: float) -> int:
	if sorted_samples.is_empty():
		return 0
	var index := clampi(int(round((sorted_samples.size() - 1) * p / 100.0)), 0, sorted_samples.size() - 1)
	return sorted_samples[index]


static func mean(samples: PackedInt64Array) -> float:
	if samples.is_empty():
		return 0.0
	var total := 0
	for s in samples:
		total += s
	return float(total) / samples.size()


# One row of the usual latency table: mean, median, p95, p99 (all in µs) and calls per second.
static func latency_row(label: String, sorted_samples: PackedInt64Array) -> Array:
	var average := mean(sorted_samples)
	return [
		label,
		"%.1f" % average,
		str(percentile(sorted_samples, 50)),
		str(percentile(sorted_samples, 95)),
		str(percentile(sorted_samples, 99)),
		"%.0f" % (1000000.0 / average if average > 0.0 else 0.0),
	]


const LATENCY_HEADERS := ["Case", "Mean (µs)", "p50 (µs)", "p95 (µs)", "p99 (µs)", "Calls/s"]


static func markdown_table(headers: Array, rows: Array) -> String:
	var lines := PackedStringArray()
	lines.append("| " + " | ".join(PackedStringArray(headers)) + " |")
	var separator := PackedStringArray()
	for i in range(headers.size()):
		separator.append("---" if i == 0 else "---:")
	lines.append("| " + " | ".join(separator) + " |")
	for row in rows:
		var cells := PackedStringArray()
		for cell in row:
			cells.append(str(cell))
		lines.append("| " + " | ".join(cells) + " |")
	return "\n".join(lines)


# Where the numbers come from: machine, Godot build and server. Printed before every table.
static func environment_summary(session: MySQLSession) -> String:
	var version: MySQLResult = session.execute_text("SELECT VERSION()")
	var server := str(version.get_rows()[0][0]) if version.is_ok() else "unknown"
	var build := "debug" if OS.is_debug_build() else "release"
	return "CPU: %s (%d threads) | OS: %s | Godot %s (%s build) | Server: %s" % [
		OS.get_processor_name(), OS.get_processor_count(), OS.get_name(), Engine.get_version_info().string, build, server,
	]


# Average time between frames over `frames` frames, in milliseconds. An asynchronous result
# arrives on the next frame at the earliest, so this bounds how fast awaited calls can go.
static func measure_frame_period(tree: SceneTree, frames: int = 60) -> float:
	await tree.process_frame
	var start := Time.get_ticks_usec()
	for i in range(frames):
		await tree.process_frame
	return (Time.get_ticks_usec() - start) / 1000.0 / frames


static func quit_if_headless(node: Node) -> void:
	if DisplayServer.get_name() == "headless":
		node.get_tree().quit()

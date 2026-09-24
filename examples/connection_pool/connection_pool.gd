# connection_pool.gd
#
# MySQLPool.acquire() is the only method in the module safe to call from several threads
# at once. Several Godot Threads share a pool smaller than the thread count, each leasing
# a session, running one query, and releasing it back (when the leased MySQLSession goes
# out of scope) for the next thread to reuse.

extends Control

@onready var output: RichTextLabel = $Output

const THREAD_COUNT := 6
const POOL_SIZE := 2


func _ready() -> void:
	run_example()
	DemoConfig.quit_if_headless(self)


func run_example() -> void:
	var config := DemoConfig.load_config()
	if config == null:
		log_line("[color=red]Could not load res://config.ini. Check the Output panel below for the error.[/color]")
		return

	var pool := MySQLPool.new()
	pool.max_size = POOL_SIZE
	pool.set_config(config)

	log_line("%d threads sharing a pool of %d connections, each running one query ..." % [THREAD_COUNT, POOL_SIZE])

	var threads: Array[Thread] = []
	for i in range(THREAD_COUNT):
		var thread := Thread.new()
		thread.start(_worker.bind(pool, i))
		threads.append(thread)
	# Each worker returns its line instead of printing it, so every line is shown here, on
	# the main thread, in thread order, before the summary below.
	for thread in threads:
		log_line(thread.wait_to_finish())

	log_line("\n[color=green]All %d threads finished.[/color] Only %d connections were ever open at once." % [THREAD_COUNT, POOL_SIZE])


# Runs on a worker thread: never touch `output`/the scene tree from here (the same rule as
# for any other Godot node). Returns the line to print instead.
func _worker(pool: MySQLPool, index: int) -> String:
	var session := pool.acquire()
	if not session.is_db_connected():
		var err: Dictionary = session.connect_db()
		if not err.is_empty():
			return "  Thread %d: [color=red]connect_db() failed: %s[/color]" % [index, err]

	# SLEEP(0.2) makes each query take a moment, so with only 2 connections for 6
	# threads, later threads visibly have to wait for an earlier one to release its
	# connection back to the pool before acquire() returns.
	var result := session.execute_text("SELECT %d, SLEEP(0.2)" % index)
	if result.is_ok():
		return "  Thread %d: got %s back from the server" % [index, result.get_rows()[0][0]]
	return "  Thread %d: [color=red]failed: %s[/color]" % [index, result.get_error()]


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print_rich(text)

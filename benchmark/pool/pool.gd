# pool.gd
#
# Throughput of MySQLPool with 1, 2, 4 and 8 threads, each thread running the same number
# of trivial queries. Measured two ways: leasing a session for every query (each lease
# resets the connection first, one extra round trip) and leasing one session per thread
# for all of its queries. The pool has as many connections as threads, all opened before
# the clock starts.

extends Control

@onready var output: RichTextLabel = $Output

const THREAD_COUNTS := [1, 2, 4, 8]
const QUERIES_PER_THREAD := 5000


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

	log_line("## Connection pool\n")
	log_line(Bench.environment_summary(session) + "\n")
	log_line("%d `SELECT 1` queries per thread. Pool max_size = number of threads.\n" % QUERIES_PER_THREAD)
	session.close_db()

	var rows := []
	for thread_count in THREAD_COUNTS:
		for lease_per_query in [true, false]:
			var pool := MySQLPool.new()
			pool.set_config(config)
			pool.max_size = thread_count
			_open_connections(pool, thread_count)

			var threads: Array[Thread] = []
			var start := Time.get_ticks_usec()
			for t in range(thread_count):
				var thread := Thread.new()
				thread.start(_worker.bind(pool, lease_per_query))
				threads.append(thread)
			var ok := 0
			for thread in threads:
				ok += int(thread.wait_to_finish())
			var total := Time.get_ticks_usec() - start
			var queries: int = thread_count * QUERIES_PER_THREAD
			rows.append([
				"%d thread(s), %s" % [thread_count, "lease per query" if lease_per_query else "one lease per thread"],
				"%.1f" % (total / 1000.0),
				"%.0f" % (queries * 1000000.0 / total),
				"%d / %d" % [ok, queries],
			])
	log_line(Bench.markdown_table(["Case", "Total (ms)", "Queries/s", "Succeeded"], rows))


# Opens every connection of the pool up front, so connecting is not part of the timing.
func _open_connections(pool: MySQLPool, count: int) -> void:
	var leased: Array[MySQLSession] = []
	for i in range(count):
		var leased_session: MySQLSession = pool.acquire()
		if not leased_session.is_db_connected():
			leased_session.connect_db()
		leased.append(leased_session)
	leased.clear() # Back to the pool.


# Runs in a worker thread. Returns how many queries succeeded.
func _worker(pool: MySQLPool, lease_per_query: bool) -> int:
	var ok := 0
	var worker_session: MySQLSession = null
	for i in range(QUERIES_PER_THREAD):
		if worker_session == null:
			worker_session = pool.acquire()
			if not worker_session.is_db_connected():
				worker_session.connect_db()
		if worker_session.execute_text("SELECT 1").is_ok():
			ok += 1
		if lease_per_query:
			worker_session = null # Back to the pool; the next query leases again.
	return ok


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print(text)

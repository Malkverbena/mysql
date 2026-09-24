# cancel.gd
#
# MySQLAsyncOperation.cancel() stops a query that is still running. The query below keeps
# the server busy for a while (it counts a billion rows, about 20 seconds on a desktop), and
# the frame counter at the top shows that the game keeps running in the meantime.
#
# Press Cancel to stop it: the module runs KILL QUERY from a short-lived side connection,
# the operation finishes with the server's "Query execution was interrupted" error, and the
# session stays connected. A headless run presses it by itself after one second. If the
# server cannot be reached to stop the query, the module cancels it locally instead and
# drops the connection (see cancel() in the class reference).

extends Control

@onready var frame_label: Label = $Layout/Top/FrameLabel
@onready var cancel_button: Button = $Layout/Top/CancelButton
@onready var output: RichTextLabel = $Layout/Output

const LONG_QUERY := "WITH RECURSIVE r(n) AS (SELECT 1 UNION ALL SELECT n + 1 FROM r WHERE n < 1000) SELECT COUNT(*) FROM r AS a, r AS b, r AS c"
const HEADLESS_CANCEL_AFTER := 1.0

var frames := 0
var started_msec := 0
var running: MySQLAsyncOperation


func _ready() -> void:
	started_msec = Time.get_ticks_msec()
	cancel_button.pressed.connect(_on_cancel_pressed)
	cancel_button.disabled = true
	await run_example()
	cancel_button.disabled = true
	DemoConfig.quit_if_headless(self)


func _process(_delta: float) -> void:
	frames += 1
	frame_label.text = "Frame %d  (%.1f s)" % [frames, (Time.get_ticks_msec() - started_msec) / 1000.0]


func run_example() -> void:
	var config := DemoConfig.load_config()
	if config == null:
		log_line("[color=red]Could not load res://config.ini. Check the Output panel below for the error.[/color]")
		return
	# The async timeout (30 s by default) would otherwise cut the query short on a slow
	# machine, and a timeout drops the connection instead of stopping the query.
	config.async_timeout_ms = 0

	var session := MySQLSession.new()
	session.set_config(config)
	var err: Dictionary = session.connect_db()
	if not err.is_empty():
		log_line("[color=red]connect_db() failed: %s[/color]" % [err])
		return

	log_line("Starting a long query. Press Cancel to stop it ...")
	var start := Time.get_ticks_msec()
	running = session.async_execute_text(LONG_QUERY)
	cancel_button.disabled = false
	if DisplayServer.get_name() == "headless":
		get_tree().create_timer(HEADLESS_CANCEL_AFTER).timeout.connect(_on_cancel_pressed)

	var operation := running
	var result: MySQLResult
	if operation.is_finished():
		result = operation.get_result()
	else:
		result = await operation.completed
	running = null
	cancel_button.disabled = true
	var elapsed := (Time.get_ticks_msec() - start) / 1000.0

	if result.is_ok():
		log_line("The query finished before being cancelled, after %.1f s: %s rows counted." % [elapsed, result.get_rows()[0][0]])
	else:
		var error := result.get_error()
		log_line("[color=orange]The query stopped after %.1f s:[/color] %s (fatal: %s)" % [elapsed, error.get("server_message", "") if error.get("server_message", "") != "" else error.get("message", ""), error.get("is_fatal")])

	log_line("cancel() on the finished operation returns %s: there is nothing left to cancel." % [operation.cancel()])

	# A query stopped on the server leaves the connection usable.
	log_line("Session still connected: %s" % [session.is_db_connected()])
	if session.is_db_connected():
		var after: MySQLResult = session.execute_text("SELECT 'the session still works'")
		log_line("Next query: %s" % [after.get_rows()[0][0] if after.is_ok() else after.get_error()])
	session.close_db()
	log_line("[color=green]Done.[/color] %d frames were drawn while the query ran." % frames)


func _on_cancel_pressed() -> void:
	if running == null:
		return
	cancel_button.disabled = true
	log_line("Cancel pressed: cancel() returned %s." % [running.cancel()])


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print_rich(text)

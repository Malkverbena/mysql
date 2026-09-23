# async.gd
#
# async_execute_text()/async_execute_prepared() return a MySQLAsyncOperation immediately,
# without blocking the caller; await its `completed` signal to get the MySQLResult. Also
# shows what happens when a second call starts on a session that is still busy with a
# first one.

extends Control

@onready var output: RichTextLabel = $Output


# `MySQLAsyncOperation.completed` fires once and is not re-sent: if the operation already
# finished by the time this is called (for example because other work ran first), a plain
# `await op.completed` would wait for a signal that already fired and never returns.
# Checking `is_finished()` first avoids that trap — always await through a helper like
# this one, never a bare `await op.completed`.
func await_result(op: MySQLAsyncOperation) -> MySQLResult:
	if op.is_finished():
		return op.get_result()
	return await op.completed


func _ready() -> void:
	await run_example()
	DemoConfig.quit_if_headless(self)


func run_example() -> void:
	var config := DemoConfig.load_config()
	if config == null:
		log_line("[color=red]Could not load res://config.ini. Check the Output panel below for the error.[/color]")
		return

	var session := MySQLSession.new()
	session.set_config(config)
	var err: Dictionary = session.connect_db()
	if not err.is_empty():
		log_line("[color=red]connect_db() failed: %s[/color]" % [err])
		return

	log_line("Starting an asynchronous query (SELECT SLEEP(1)) ...")
	var op := session.async_execute_text("SELECT SLEEP(1)")
	log_line("async_execute_text() already returned — it did not wait for the query.")
	for i in range(3):
		log_line("  ... free to do other things while it runs on the server (%d/3)" % (i + 1))
		await get_tree().create_timer(0.2).timeout
	var result := await await_result(op)
	log_line("[color=green]Query finished.[/color] is_ok() == %s" % result.is_ok())

	log_line("\nStarting an asynchronous prepared statement ...")
	var op2 := session.async_execute_prepared("SELECT ? + ?", [21, 21])
	var result2 := await await_result(op2)
	if result2.is_ok():
		log_line("21 + 21 = %s" % result2.get_rows()[0][0])
	else:
		log_line("[color=red]async_execute_prepared() failed: %s[/color]" % [result2.get_error()])

	log_line("\nA session runs one asynchronous operation at a time. Starting a second one")
	log_line("while the first is still running (both on the same session, on purpose) ...")
	var busy_op := session.async_execute_text("SELECT SLEEP(1)")
	var rejected_op := session.async_execute_text("SELECT 1")
	var rejected_result := await await_result(rejected_op)
	log_line("Second call: is_ok() == %s (%s)" % [rejected_result.is_ok(), rejected_result.get_error()])
	await await_result(busy_op)
	log_line("First call finished normally — the rejection did not affect it.")

	session.close_db()


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print_rich(text)

# transactions.gd
#
# begin_transaction() -> commit()/rollback(). Sets up its own demo table and data on
# first run, then transfers a balance between two rows inside a transaction: once
# committed, once rolled back on purpose, printing the balances after each to show the
# difference.

extends Control

@onready var output: RichTextLabel = $Output

const TABLE := "mysql_module_example_accounts"


func _ready() -> void:
	run_example()
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

	setup_demo_table(session)
	print_balances(session, "Before the transfer:")

	log_line("\nTransferring 100 from Alice to Bob inside a transaction ...")
	var tx := session.begin_transaction()
	var r1 := session.execute_prepared("UPDATE %s SET balance = balance - ? WHERE name = ?" % TABLE, [100, "Alice"])
	var r2 := session.execute_prepared("UPDATE %s SET balance = balance + ? WHERE name = ?" % TABLE, [100, "Bob"])
	if r1.is_ok() and r2.is_ok():
		tx.commit()
		log_line("[color=green]Committed.[/color]")
	else:
		tx.rollback()
		log_line("[color=red]Rolled back: one of the updates failed (%s / %s).[/color]" % [r1.get_error(), r2.get_error()])
	print_balances(session, "After the transfer:")

	log_line("\nRunning the same transfer again, but calling rollback() on purpose this time ...")
	var tx2 := session.begin_transaction()
	session.execute_prepared("UPDATE %s SET balance = balance - ? WHERE name = ?" % TABLE, [100, "Alice"])
	session.execute_prepared("UPDATE %s SET balance = balance + ? WHERE name = ?" % TABLE, [100, "Bob"])
	tx2.rollback()
	log_line("Rolled back.")
	print_balances(session, "After the rollback (unchanged from the line above, the transfer never applied):")

	session.close_db()


func setup_demo_table(session: MySQLSession) -> void:
	session.execute_text("CREATE TABLE IF NOT EXISTS %s (name VARCHAR(50) PRIMARY KEY, balance INT NOT NULL)" % TABLE)
	# Reset on every run, so each run starts from the same balances instead of carrying over
	# the previous run's committed transfer.
	session.execute_prepared("REPLACE INTO %s (name, balance) VALUES (?, ?), (?, ?)" % TABLE, ["Alice", 1000, "Bob", 500])


func print_balances(session: MySQLSession, title: String) -> void:
	log_line(title)
	var result := session.execute_text("SELECT name, balance FROM %s ORDER BY name" % TABLE)
	for row in result.get_rows():
		log_line("  %s: %d" % [row[0], row[1]])


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print_rich(text)

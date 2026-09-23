# connect_and_configure.gd
#
# The minimal path: MySQLConfig -> MySQLSession -> connect_db() -> a query -> close_db().
# Also creates the schema named in config.ini (CREATE DATABASE IF NOT EXISTS), so the
# transactions and streaming examples have it ready when they connect directly to it.

extends Control

@onready var output: RichTextLabel = $Output


func _ready() -> void:
	run_example()
	DemoConfig.quit_if_headless(self)


func run_example() -> void:
	log_line("Loading res://config.ini ...")
	var database_name := DemoConfig.get_database_name()
	# No default schema yet: it may not exist on the server until the CREATE DATABASE
	# below runs.
	var config := DemoConfig.load_config(false)
	if config == null:
		log_line("[color=red]Could not load the configuration. Check the Output panel below for the error.[/color]")
		return
	if database_name.is_empty():
		log_line("[color=red]config.ini has no \"database\" value under [mysql].[/color]")
		return

	log_line("Connecting to %s:%d as %s (no schema selected yet) ..." % [config.host, config.port, config.user])
	var session := MySQLSession.new()
	session.set_config(config)
	var err: Dictionary = session.connect_db()
	if not err.is_empty():
		log_line("[color=red]connect_db() failed: %s[/color]" % [err])
		return
	log_line("[color=green]Connected.[/color] is_db_connected() == %s" % session.is_db_connected())

	# MySQL has no parameterized placeholder for identifiers (only for values), so the
	# schema name is quoted by hand instead: wrapped in backticks, with any backtick in
	# the name itself doubled, the standard MySQL identifier-escaping rule.
	var quoted_database := "`%s`" % database_name.replace("`", "``")
	log_line("Creating schema %s (if it does not already exist) ..." % quoted_database)
	var create_result := session.execute_text("CREATE DATABASE IF NOT EXISTS %s" % quoted_database)
	if not create_result.is_ok():
		log_line("[color=red]CREATE DATABASE failed: %s[/color]" % [create_result.get_error()])
		return
	session.execute_text("USE %s" % quoted_database)
	log_line("[color=green]Schema ready.[/color] transactions.tscn and streaming.tscn connect to it directly.")

	var result := session.execute_text("SELECT VERSION()")
	if result.is_ok():
		log_line("Server version: %s" % result.get_rows()[0][0])
	else:
		log_line("[color=red]SELECT VERSION() failed: %s[/color]" % [result.get_error()])

	session.close_db()
	log_line("Connection closed. is_db_connected() == %s" % session.is_db_connected())


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print_rich(text)

# streaming.gd
#
# execute_streaming() reads a result incrementally, in batches, instead of loading every
# row into memory at once. Populates its own demo table on first run (a few thousand
# rows), then reads it back with next_batch()/has_more(), printing progress per batch.

extends Control

@onready var output: RichTextLabel = $Output

const TABLE := "mysql_module_example_numbers"
const ROW_COUNT := 2000


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

	log_line("Streaming %d rows in batches, instead of loading them all at once ..." % ROW_COUNT)
	var cursor := session.execute_streaming("SELECT n FROM %s ORDER BY n" % TABLE)
	if not cursor.is_ok():
		log_line("[color=red]execute_streaming() failed: %s[/color]" % [cursor.get_error()])
		return

	var batch_count := 0
	var row_count := 0
	while cursor.has_more():
		var batch := cursor.next_batch()
		batch_count += 1
		row_count += batch.size()
		log_line("  Batch %d: %d rows (running total: %d)" % [batch_count, batch.size(), row_count])
	cursor.close()

	log_line("[color=green]Done.[/color] %d rows read in %d batches." % [row_count, batch_count])
	session.close_db()


func setup_demo_table(session: MySQLSession) -> void:
	session.execute_text("CREATE TABLE IF NOT EXISTS %s (n INT PRIMARY KEY)" % TABLE)
	var count: int = session.execute_text("SELECT COUNT(*) FROM %s" % TABLE).get_rows()[0][0]
	if count < ROW_COUNT:
		session.execute_text("TRUNCATE TABLE %s" % TABLE)
		log_line("Populating the demo table with %d rows (only needed once) ..." % ROW_COUNT)
		for i in range(ROW_COUNT):
			session.execute_prepared("INSERT INTO %s (n) VALUES (?)" % TABLE, [i])


func log_line(text: String) -> void:
	output.append_text(text + "\n")
	print_rich(text)

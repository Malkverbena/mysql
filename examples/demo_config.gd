# demo_config.gd
#
# Loads res://config.ini into a MySQLConfig, shared by every example scene.

extends RefCounted

class_name DemoConfig


# `include_database = false` connects without selecting a default schema yet — used by
# connect_and_configure.gd to create the schema named in config.ini before it exists.
static func load_config(include_database: bool = true) -> MySQLConfig:
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
	if include_database:
		config.database = ini.get_value("mysql", "database", "")
	# TCP_TLS_DISABLED is only for this local demo (see the README): most local development
	# MySQL/MariaDB servers do not have a certificate signed by a trusted CA. Production use
	# should keep the default, TCP_TLS_REQUIRED — see "Intended use" in
	# ../documentation/features.md.
	config.transport_mode = MySQLConfig.TCP_TLS_DISABLED
	return config


static func get_database_name() -> String:
	var ini := ConfigFile.new()
	if ini.load("res://config.ini") != OK:
		return ""
	return ini.get_value("mysql", "database", "")


# A headless run (`godot --headless --path examples res://<scene>.tscn`) has no window to
# close, so without this it would never exit. With a window (F5/F6 in the editor) the scene
# stays open, so its on-screen output can still be read.
static func quit_if_headless(node: Node) -> void:
	if DisplayServer.get_name() == "headless":
		node.get_tree().quit()

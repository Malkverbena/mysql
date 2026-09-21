# SPDX-License-Identifier: MIT
# tests/smoke_test.gd
#
# Teste local de integração contra um MySQL/MariaDB de verdade (Fase 6 do roadmap de
# reescrita). Não é um teste unitário — cobre o caminho ponta a ponta de cada classe
# exposta, contra um banco real. Roda com:
#
#   godot --headless --script tests/smoke_test.gd
#
# a partir da árvore do Godot já compilada com este módulo (custom_modules=../mysql).
#
# Credenciais vêm de variáveis de ambiente, nunca deste arquivo (não versionar
# segredo nenhum):
#   MYSQL_TEST_HOST (padrão 127.0.0.1), MYSQL_TEST_PORT (padrão 3306),
#   MYSQL_TEST_USER, MYSQL_TEST_PASSWORD, MYSQL_TEST_DATABASE (obrigatórios).
#
# Cria e derruba uma tabela própria (t_mysql_module_smoke_test) no schema indicado —
# não toca em mais nada do banco.

extends SceneTree

var failures: int = 0
var checks: int = 0


func check(condition: bool, description: String) -> void:
	checks += 1
	if condition:
		print("  OK   ", description)
	else:
		failures += 1
		print("  FAIL ", description)


func fail_and_quit(message: String) -> void:
	printerr(message)
	quit(1)


func _initialize() -> void:
	var host := OS.get_environment("MYSQL_TEST_HOST")
	if host.is_empty():
		host = "127.0.0.1"
	var port_str := OS.get_environment("MYSQL_TEST_PORT")
	var port := 3306 if port_str.is_empty() else int(port_str)
	var user := OS.get_environment("MYSQL_TEST_USER")
	var password := OS.get_environment("MYSQL_TEST_PASSWORD")
	var database := OS.get_environment("MYSQL_TEST_DATABASE")

	if user.is_empty() or password.is_empty() or database.is_empty():
		fail_and_quit("smoke_test: defina MYSQL_TEST_USER, MYSQL_TEST_PASSWORD e MYSQL_TEST_DATABASE no ambiente.")
		return

	print("=== 0. Validação de TLS (S5) ===")
	var tls_config := MySQLConfig.new()
	tls_config.host = host
	tls_config.port = port
	tls_config.user = user
	tls_config.set_password(password)
	tls_config.database = database
	tls_config.transport_mode = MySQLConfig.TCP_TLS_REQUIRED
	var tls_session := MySQLSession.new()
	tls_session.set_config(tls_config)
	var tls_err: Dictionary = tls_session.connect_db()
	check(not tls_err.is_empty() and tls_err.get("is_fatal") == true, "TCP_TLS_REQUIRED rejeita o certificado autoassinado do servidor de teste em vez de aceitar em silêncio (%s)" % [tls_err])

	# O restante dos testes usa TCP_TLS_DISABLED de propósito: é um MariaDB local, de
	# teste, sem CA confiável configurada — a checagem acima já provou que a validação
	# de TLS funciona (resolve S5). Isto não é o padrão recomendado para produção.
	var config := MySQLConfig.new()
	config.host = host
	config.port = port
	config.user = user
	config.set_password(password)
	config.database = database
	config.transport_mode = MySQLConfig.TCP_TLS_DISABLED

	print("=== 1. Conexão ===")
	var session := MySQLSession.new()
	session.set_config(config)
	var err: Dictionary = session.connect_db()
	check(err.is_empty(), "connect_db() sem erro (%s)" % [err])
	check(session.is_db_connected(), "is_db_connected() == true")
	if not session.is_db_connected():
		fail_and_quit("smoke_test: não foi possível conectar, abortando o resto dos testes.")
		return

	await _run_all_tests(session, config)

	print("\n=== Encerrando ===")
	var close_err: Dictionary = session.close_db()
	check(close_err.is_empty(), "close_db() sem erro (%s)" % [close_err])
	check(not session.is_db_connected(), "is_db_connected() == false depois de close_db()")

	print("\n%d checagens, %d falhas." % [checks, failures])
	quit(1 if failures > 0 else 0)


func _run_all_tests(session: MySQLSession, config: MySQLConfig) -> void:
	print("=== 2. Tabela de teste ===")
	session.execute_text("DROP TABLE IF EXISTS t_mysql_module_smoke_test")
	var create_result: MySQLResult = session.execute_text(
		"""
		CREATE TABLE t_mysql_module_smoke_test (
			id INT PRIMARY KEY AUTO_INCREMENT,
			flag TINYINT(1),
			small_num TINYINT(4),
			big_signed BIGINT,
			big_unsigned BIGINT UNSIGNED,
			txt VARCHAR(100),
			data BLOB,
			d DATE,
			t TIME(6),
			dt DATETIME(6),
			js JSON
		)
		"""
	)
	check(create_result.is_ok(), "CREATE TABLE (%s)" % [create_result.get_error()])

	print("=== 3. execute_text + tipos de dado ===")
	var insert_result: MySQLResult = session.execute_text(
		"""
		INSERT INTO t_mysql_module_smoke_test
			(flag, small_num, big_signed, big_unsigned, txt, data, d, t, dt, js)
		VALUES
			(1, 42, -123456789, 18446744073709551615, 'olá mundo', 0x0102FF, '2026-09-21', '25:30:15.500000', '2026-09-21 10:30:00.123456', '{"a": 1, "b": [true, null]}')
		"""
	)
	check(insert_result.is_ok(), "INSERT com tipos variados (%s)" % [insert_result.get_error()])
	check(insert_result.get_affected_rows() == 1, "affected_rows == 1")
	check(insert_result.get_last_insert_id() == 1, "last_insert_id == 1")

	var select_result: MySQLResult = session.execute_text("SELECT * FROM t_mysql_module_smoke_test WHERE id = 1")
	check(select_result.is_ok(), "SELECT de volta (%s)" % [select_result.get_error()])
	var cols: PackedStringArray = select_result.get_column_names()
	var rows: Array = select_result.get_rows()
	check(rows.size() == 1, "SELECT devolveu 1 linha")
	if rows.size() == 1:
		var row: Array = rows[0]

		var flag_v = row[cols.find("flag")]
		check(typeof(flag_v) == TYPE_INT and flag_v == 1, "tinyint1_mode=false (padrão): TINYINT(1) vem como int, não bool (valor: %s, tipo: %s)" % [flag_v, typeof(flag_v)])
		var small_num_v = row[cols.find("small_num")]
		check(typeof(small_num_v) == TYPE_INT and small_num_v == 42, "TINYINT(4) vem como int (%s)" % [small_num_v])
		var big_signed_v = row[cols.find("big_signed")]
		check(typeof(big_signed_v) == TYPE_INT and big_signed_v == -123456789, "BIGINT assinado vem como int (%s)" % [big_signed_v])
		var big_unsigned_v = row[cols.find("big_unsigned")]
		check(typeof(big_unsigned_v) == TYPE_STRING and big_unsigned_v == "18446744073709551615", "BIGINT UNSIGNED acima de INT64_MAX vem como String exata (%s)" % [big_unsigned_v])
		var txt_v = row[cols.find("txt")]
		check(typeof(txt_v) == TYPE_STRING and txt_v == "olá mundo", "VARCHAR utf8mb4 preservado (%s)" % [txt_v])
		var data_v = row[cols.find("data")]
		check(typeof(data_v) == TYPE_PACKED_BYTE_ARRAY and data_v == PackedByteArray([1, 2, 255]), "BLOB vem como PackedByteArray (%s)" % [data_v])

		var d: Dictionary = row[cols.find("d")]
		check(d.get("year") == 2026 and d.get("month") == 9 and d.get("day") == 21, "DATE vira Dictionary correto (%s)" % [d])

		var t: Dictionary = row[cols.find("t")]
		check(t.get("hours") == 25 and t.get("minutes") == 30 and t.get("seconds") == 15 and t.get("microsecond") == 500000, "TIME > 24h e com microssegundos vira Dictionary correto (%s)" % [t])

		var dt: Dictionary = row[cols.find("dt")]
		check(dt.get("year") == 2026 and dt.get("hour") == 10 and dt.get("microsecond") == 123456, "DATETIME(6) com microssegundos vira Dictionary correto (%s)" % [dt])

		print("=== 3b. json_result_mode ===")
		var json_col_index := cols.find("js")
		var js_v = row[json_col_index]
		check(typeof(js_v) == TYPE_STRING, "json_result_mode padrão (LAZY_PARSED_VARIANT) devolve String bruta na linha (%s)" % [js_v])
		var parsed: Variant = select_result.get_parsed_json(0, 0, json_col_index)
		check(typeof(parsed) == TYPE_DICTIONARY and parsed.get("a") == 1, "get_parsed_json() converte sob demanda (%s)" % [parsed])
		var parsed_again: Variant = select_result.get_parsed_json(0, 0, json_col_index)
		check(parsed_again == parsed, "get_parsed_json() é idempotente (cache)")

	# json_result_mode = PARSED_VARIANT depende do servidor reportar um tipo JSON
	# distinto na metadata da coluna. MariaDB não tem isso — JSON lá é alias de
	# LONGTEXT no protocolo (achado ao testar contra um servidor de verdade, agora
	# registrado em design-notes.md) — então PARSED_VARIANT se comporta como
	# RAW_STRING no MariaDB. Isto não é um bug do módulo, é uma diferença real entre
	# os bancos: por isso este teste confere o comportamento documentado (String),
	# não o que aconteceria num MySQL de verdade com tipo JSON nativo.
	config.json_result_mode = MySQLConfig.PARSED_VARIANT
	var select_parsed: MySQLResult = session.execute_text("SELECT js FROM t_mysql_module_smoke_test WHERE id = 1")
	var js_parsed_variant: Variant = select_parsed.get_rows()[0][0]
	check(typeof(js_parsed_variant) == TYPE_STRING, "json_result_mode = PARSED_VARIANT no MariaDB: sem tipo JSON no protocolo, comporta-se como RAW_STRING (%s)" % [js_parsed_variant])
	config.json_result_mode = MySQLConfig.LAZY_PARSED_VARIANT # devolve ao padrão

	print("=== 4. execute_formatted (sem escaping próprio) ===")
	var injection_attempt := "'); DROP TABLE t_mysql_module_smoke_test; --"
	var formatted_result: MySQLResult = session.execute_formatted(
		"INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", [injection_attempt]
	)
	check(formatted_result.is_ok(), "execute_formatted com valor 'perigoso' não quebra a query (%s)" % [formatted_result.get_error()])
	var still_exists: MySQLResult = session.execute_text("SELECT COUNT(*) AS n FROM t_mysql_module_smoke_test")
	check(still_exists.is_ok() and int(still_exists.get_rows()[0][0]) >= 2, "tabela sobrevive à tentativa de injeção (COUNT = %s)" % [still_exists.get_rows()[0][0] if still_exists.is_ok() else "?"])
	var escaped_back: MySQLResult = session.execute_text("SELECT txt FROM t_mysql_module_smoke_test WHERE id = 2")
	check(escaped_back.is_ok() and escaped_back.get_rows()[0][0] == injection_attempt, "o valor 'perigoso' foi gravado literalmente, como string (%s)" % [escaped_back.get_rows()[0][0] if escaped_back.is_ok() else "?"])

	print("=== 5. execute_prepared (cache LRU) ===")
	for i in range(3):
		var prep_result: MySQLResult = session.execute_prepared(
			"INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", ["prepared_%d" % i]
		)
		check(prep_result.is_ok(), "execute_prepared #%d (%s)" % [i, prep_result.get_error()])
	var prep_check: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt LIKE 'prepared\\_%'")
	check(prep_check.is_ok() and int(prep_check.get_rows()[0][0]) == 3, "as 3 chamadas prepared inseriram 3 linhas")

	print("=== 6. Erros explícitos (nunca silenciosos) ===")
	var bad_param_result: MySQLResult = session.execute_prepared("INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", [])
	check(not bad_param_result.is_ok(), "contagem de parâmetros errada vira erro explícito (%s)" % [bad_param_result.get_error()])
	var bad_type_result: MySQLResult = session.execute_formatted("INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", [{"nao": "suportado"}])
	check(not bad_type_result.is_ok(), "Dictionary como parâmetro vira erro explícito, não NULL silencioso (%s)" % [bad_type_result.get_error()])

	print("=== 7. Transações ===")
	var tx_commit := session.begin_transaction()
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_commit')")
	var commit_err: Dictionary = tx_commit.commit()
	check(commit_err.is_empty(), "commit() sem erro (%s)" % [commit_err])
	var after_commit: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'tx_commit'")
	check(after_commit.is_ok() and int(after_commit.get_rows()[0][0]) == 1, "linha commitada está visível")

	var tx_rollback := session.begin_transaction()
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_rollback')")
	var rollback_err: Dictionary = tx_rollback.rollback()
	check(rollback_err.is_empty(), "rollback() sem erro (%s)" % [rollback_err])
	var after_rollback: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'tx_rollback'")
	check(after_rollback.is_ok() and int(after_rollback.get_rows()[0][0]) == 0, "linha com rollback não está visível")

	print("=== 7b. Rollback automático (destrutor) ===")
	session.begin_transaction() # referência solta imediatamente, sem commit/rollback
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_auto_rollback')")
	# Não há uma segunda referência seguindo a transação anterior: ao sair de escopo do
	# argumento acima, o GDScript solta a referência e o destrutor de MySQLTransaction
	# deveria ter feito ROLLBACK antes disso — mas como não fechamos essa transação
	# antes de inserir, o INSERT acima já roda DENTRO dela. Fechamos explicitamente aqui
	# só a auto_rollback em si, forçando a liberação da referência:
	var tx_auto := session.begin_transaction()
	session.execute_text("INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('tx_auto_rollback_2')")
	tx_auto = null # solta a única referência sem commit()/rollback() -> destrutor roda
	var after_auto: MySQLResult = session.execute_text("SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt = 'tx_auto_rollback_2'")
	check(after_auto.is_ok() and int(after_auto.get_rows()[0][0]) == 0, "rollback automático (destrutor sem commit/rollback) desfez a linha")

	print("=== 8. Streaming ===")
	for i in range(20):
		session.execute_prepared("INSERT INTO t_mysql_module_smoke_test (txt) VALUES (?)", ["stream_%d" % i])
	var cursor: MySQLStreamingCursor = session.execute_streaming("SELECT txt FROM t_mysql_module_smoke_test WHERE txt LIKE 'stream\\_%' ORDER BY id")
	check(cursor.is_ok(), "execute_streaming() abriu sem erro (%s)" % [cursor.get_error()])
	var streamed_count := 0
	while cursor.has_more():
		var batch: Array = cursor.next_batch()
		streamed_count += batch.size()
	check(streamed_count == 20, "streaming leu as 20 linhas em lotes (leu %d)" % [streamed_count])
	check(cursor.is_ok(), "cursor.is_ok() ainda true depois de esgotar")

	var cursor2: MySQLStreamingCursor = session.execute_streaming("SELECT txt FROM t_mysql_module_smoke_test WHERE txt LIKE 'stream\\_%'")
	cursor2.next_batch()
	cursor2.close() # fecha no meio de propósito — drena o resto sozinho
	var after_partial_stream: MySQLResult = session.execute_text("SELECT 1")
	check(after_partial_stream.is_ok(), "conexão continua utilizável depois de fechar um streaming no meio (%s)" % [after_partial_stream.get_error()])

	print("=== 9. Assíncrono ===")
	# await na signal, não polling bloqueante: um loop com OS.delay_msec() nunca devolve
	# o controle pro SceneTree rodar frames, e é rodar frames que processa a fila do
	# call_deferred — um loop assim nunca veria is_finished() virar true (achado ao
	# testar de verdade: a operação completava direitinho, só nunca era observada).
	var async_op: MySQLAsyncOperation = session.async_execute_text("SELECT 'async_ok' AS v")
	var async_result: MySQLResult = await async_op.completed
	check(async_op.is_finished(), "async_execute_text terminou (sinal completed disparou)")
	check(async_result.is_ok() and async_result.get_rows()[0][0] == "async_ok", "async_execute_text devolveu o valor certo (%s)" % [async_result.get_rows() if async_result.is_ok() else async_result.get_error()])

	print("=== 10. Pool de conexões ===")
	var pool := MySQLPool.new()
	pool.set_config(config)
	pool.max_size = 2
	var pool_session_a: MySQLSession = pool.acquire()
	var pool_session_b: MySQLSession = pool.acquire()
	check(pool_session_a != null and pool_session_b != null, "acquire() duas vezes dentro do max_size não bloqueia")
	pool_session_a.connect_db()
	pool_session_b.connect_db()
	var pool_result_a: MySQLResult = pool_session_a.execute_text("SELECT 1")
	var pool_result_b: MySQLResult = pool_session_b.execute_text("SELECT 1")
	check(pool_result_a.is_ok() and pool_result_b.is_ok(), "as duas sessões do pool funcionam de forma independente")
	pool_session_a = null
	pool_session_b = null
	var pool_session_c: MySQLSession = pool.acquire() # uma das duas conexões acima deve ter voltado pro pool
	check(pool_session_c != null, "acquire() depois de soltar as anteriores devolve uma conexão (reciclada do pool)")

	print("=== 11. execute_script ===")
	config.allow_sql_script_execution = true
	var script_result: Array = session.execute_script(
		"""
		INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('script_1');
		INSERT INTO t_mysql_module_smoke_test (txt) VALUES ('script_2');
		SELECT COUNT(*) FROM t_mysql_module_smoke_test WHERE txt LIKE 'script\\_%';
		"""
	)
	check(script_result.size() == 3, "execute_script devolveu 3 resultados (um por instrução)")
	var all_script_ok := true
	for r in script_result:
		if not (r as MySQLResult).is_ok():
			all_script_ok = false
	check(all_script_ok, "todas as instruções do script rodaram sem erro")
	if script_result.size() == 3:
		var last: MySQLResult = script_result[2]
		check(int(last.get_rows()[0][0]) == 2, "as duas instruções do script realmente inseriram (COUNT = %s)" % [last.get_rows()[0][0]])

	print("=== 12. Limpeza ===")
	var drop_result: MySQLResult = session.execute_text("DROP TABLE t_mysql_module_smoke_test")
	check(drop_result.is_ok(), "DROP TABLE final (%s)" % [drop_result.get_error()])

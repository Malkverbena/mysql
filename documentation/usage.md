# Usage

How to use this module from GDScript, once it is compiled into your Godot build (see
[instructions.md](instructions.md)). For the full list of what it can do, see
[features.md](features.md); for the exact signature of every method, member and signal,
use Godot's own built-in help (`F1` in the editor, or hover a class name) — it is
generated from [`../doc_classes/`](../doc_classes/).

## Classes at a glance

| Class | What it is |
|---|---|
| `MySQLConfig` | Connection settings: host/port/database, credentials, `transport_mode`, timeouts and limits. Passed to a session or a pool with `set_config()`. |
| `MySQLSession` | A single connection. `connect_db()`/`close_db()`, and every way to run SQL: `execute_text`, `execute_formatted`, `execute_prepared`, `execute_streaming`, `execute_script`, `async_execute_text`, `async_execute_prepared`, `begin_transaction`. |
| `MySQLPool` | Hands out `MySQLSession` instances (`acquire()`) from a shared, thread-safe pool — one session per thread that needs one, never one session shared between threads. |
| `MySQLResult` | The outcome of a non-streaming query: `is_ok()`, `get_error()`, `get_rows()`, `get_column_names()`, `get_affected_rows()`, `get_last_insert_id()`; multi-resultset aware (`get_resultset_count()`, and a `resultset` index on the other getters). |
| `MySQLStreamingCursor` | Incremental reading of a large result: `next_batch()`/`has_more()`, without loading everything into memory. |
| `MySQLAsyncOperation` | What an `async_*` call returns: `await`able via its `completed` signal, or poll with `is_finished()`/`get_result()`. |
| `MySQLTransaction` | `commit()`/`rollback()`, obtained from `MySQLSession.begin_transaction()`. |

## Connecting

```gdscript
var config := MySQLConfig.new()
config.host = "127.0.0.1"
config.port = 3306
config.user = "my_user"
config.set_password("my_password")  # never config.password = ...; there is no getter either
config.database = "my_schema"
# config.transport_mode defaults to MySQLConfig.TCP_TLS_REQUIRED — leave it there unless
# you have a specific, trusted reason not to (see "Intended use" in features.md).

var session := MySQLSession.new()
session.set_config(config)
var err: Dictionary = session.connect_db()
if not err.is_empty():
    push_error("MySQL connect failed: %s" % err)
    return

# ... use the session ...

session.close_db()
```

`connect_db()`/`close_db()` return an empty `Dictionary` on success, or an error
`Dictionary` (keys `category`, `message`, `server_message`, `is_fatal`) on failure — the
same error shape every fallible call in this module uses.

## Running SQL

### Text (no parameters, or your own SQL string)

```gdscript
var result: MySQLResult = session.execute_text("SELECT id, name FROM users")
if not result.is_ok():
    push_error(result.get_error())
    return
for row in result.get_rows():
    print(row[0], " ", row[1])  # row is an Array, one entry per column, in get_column_names() order
```

### Formatted text (safe interpolation, no prepared statement)

```gdscript
var result: MySQLResult = session.execute_formatted(
    "SELECT * FROM users WHERE id = ? AND active = ?", [user_id, true]
)
```

`?` placeholders are filled through Boost.MySQL's own `format_sql`/`with_params` — never
string concatenation, so this is not vulnerable to SQL injection the way building the
query with `%`/`+` would be.

### Prepared statements (reused automatically)

```gdscript
var result: MySQLResult = session.execute_prepared(
    "INSERT INTO users (name, email) VALUES (?, ?)", [name, email]
)
print(result.get_last_insert_id())
```

The same SQL text reuses one prepared statement across calls (a per-session LRU cache,
`MySQLConfig.statement_cache_size`) instead of preparing it again every time.

### Multiple statements in one call

```gdscript
config.allow_sql_script_execution = true  # off by default; emits a warning when enabled
var results: Array = session.execute_script("""
    UPDATE accounts SET balance = balance - 100 WHERE id = 1;
    UPDATE accounts SET balance = balance + 100 WHERE id = 2;
""")
for r in results:
    if not r.is_ok():
        push_error(r.get_error())
```

## Asynchronous calls

```gdscript
var op: MySQLAsyncOperation = session.async_execute_text("SELECT SLEEP(1)")
var result: MySQLResult = await op.completed
```

Always `await`, never poll in a loop (`while not op.is_finished(): pass`) — that starves
the `SceneTree` of the frames that actually deliver the result. Keep the `MySQLSession`
(or the `MySQLPool` it came from) referenced until the operation finishes; if it is freed
first, its I/O thread stops mid-operation. A session runs one asynchronous operation at a
time — a second call on the same session while one is in flight fails explicitly instead
of queuing (use one session per parallel operation, e.g. from a `MySQLPool`).

## Transactions

```gdscript
var tx: MySQLTransaction = session.begin_transaction()
var r1 := session.execute_prepared("UPDATE accounts SET balance = balance - ? WHERE id = ?", [amount, from_id])
var r2 := session.execute_prepared("UPDATE accounts SET balance = balance + ? WHERE id = ?", [amount, to_id])
if r1.is_ok() and r2.is_ok():
    tx.commit()
else:
    tx.rollback()
```

If `tx` goes out of scope without either call, the module rolls back automatically and
logs a warning — a safety net, not a substitute for calling `commit()`/`rollback()`
yourself on every path.

## Streaming a large result

```gdscript
var cursor: MySQLStreamingCursor = session.execute_streaming("SELECT * FROM big_table")
while cursor.has_more():
    for row in cursor.next_batch():
        process(row)
cursor.close()
```

Reads incrementally, never loading the whole result into memory — the right tool for a
result too large to fit in one `MySQLResult` (`max_result_bytes`, below, is the guard for
everything that is *not* read this way).

## Connection pool (multithreading)

```gdscript
var pool := MySQLPool.new()
pool.set_config(config)  # same MySQLConfig every acquired session will connect with

# From any thread:
var session: MySQLSession = pool.acquire()
if not session.is_db_connected():
    session.connect_db()
var result := session.execute_text("SELECT 1")
```

Each thread must use its own acquired `MySQLSession` — never share one session between
threads at the same time. `pool.acquire()` blocks if the pool is already at
`MySQLPool.max_size`.

## Bounding result size

```gdscript
config.max_result_bytes = 10 * 1024 * 1024  # 10 MiB, default 0 = unlimited
var result := session.execute_text("SELECT * FROM potentially_huge_table")
if not result.is_ok():
    print(result.get_error())  # {"category": "mysql_module.client", "message": "Result exceeds max_result_bytes (...)", ...}
# the connection is still usable afterwards — the overflowing result is drained, not left mid-stream
```

## Multiple resultsets

A call that returns more than one resultset (a stored procedure, or `allow_multi_queries`
with several statements) exposes each one through an index:

```gdscript
var result := session.execute_text("CALL some_procedure()")
for i in result.get_resultset_count():
    print(result.get_column_names(i))
    print(result.get_rows(i))
```

## Error handling

Every fallible call follows the same shape — check `is_ok()` (or an empty `Dictionary`
for `connect_db()`/`close_db()`/`commit()`/`rollback()`), then read `get_error()`:

```gdscript
if not result.is_ok():
    var error: Dictionary = result.get_error()
    match error.category:
        "mysql.common-server":
            print("Server rejected the SQL: ", error.server_message)
        "mysql_module.client":
            print("Client-side problem (bad params, limit hit, etc.): ", error.message)
        _:
            print(error)
```

There are no exceptions anywhere in this module (`no_exception`, like Godot's own
default) and no global error state — every call carries its own result.

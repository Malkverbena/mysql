# Usage

How to use this module from GDScript, once it is compiled into your Godot build (see
[instructions.md](instructions.md)). For the full list of what it can do, see
[features.md](features.md); for the exact signature of every method, member and signal,
use Godot's own built-in help (`F1` in the editor, or hover a class name) — it is
generated from [`../doc_classes/`](../doc_classes/).

## Classes Overview

| Class | What it is |
|---|---|
| `MySQLConfig` | Connection settings: host/port/database, credentials, `transport_mode`, timeouts and limits. Passed to a session or a pool with `set_config()`. |
| `MySQLSession` | A single connection. `connect_db()`/`close_db()`, and every way to run SQL: `execute_text`, `execute_formatted`, `execute_prepared`, `execute_streaming`, `execute_script`, `async_execute_text`, `async_execute_prepared`, `begin_transaction`. |
| `MySQLPool` | Hands out `MySQLSession` instances (`acquire()`) from a shared, thread-safe pool — one session per thread that needs one, never one session shared between threads. |
| `MySQLResult` | The outcome of a non-streaming query: `is_ok()`, `get_error()`, `get_rows()`, `get_column_names()`, `get_affected_rows()`, `get_last_insert_id()`; multi-resultset aware (`get_resultset_count()`, and a `resultset` index on the other getters). |
| `MySQLStreamingCursor` | Incremental reading of a large result: `next_batch()`/`has_more()`, without loading everything into memory. |
| `MySQLAsyncOperation` | What an `async_*` call returns: `await`able via its `completed` signal, or poll with `is_finished()`/`get_result()`. |
| `MySQLTransaction` | `commit()`/`rollback()`, obtained from `MySQLSession.begin_transaction()`; `is_ok()`/`get_error()` say whether it started. |

## Connecting

```gdscript
var config = MySQLConfig.new()
config.host = "127.0.0.1"
config.port = 3306
config.user = "my_user"
config.set_password("my_password")  # never config.password = ...; there is no getter either
config.database = "my_schema"
# config.transport_mode defaults to MySQLConfig.TCP_TLS_REQUIRED — leave it there unless
# you have a specific, trusted reason not to (see "Intended use" in features.md).

var session = MySQLSession.new()
session.set_config(config)
var err = session.connect_db()
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
var result = session.execute_text("SELECT id, name FROM users")
if not result.is_ok():
    push_error(result.get_error())
    return
for row in result.get_rows():
    print(row[0], " ", row[1])  # row is an Array, one entry per column, in get_column_names() order
```

### Formatted text (safe interpolation, no prepared statement)

```gdscript
var result = session.execute_formatted(
    "SELECT * FROM users WHERE id = ? AND active = ?", [user_id, true]
)
```

`?` placeholders are filled through Boost.MySQL's own `format_sql`/`with_params` — never
string concatenation, so this is not vulnerable to SQL injection the way building the
query with `%`/`+` would be.

### Prepared statements (reused automatically)

```gdscript
var result = session.execute_prepared(
    "INSERT INTO users (name, email) VALUES (?, ?)", [name, email]
)
print(result.get_last_insert_id())
```

The same SQL text reuses one prepared statement across calls (a per-session LRU cache,
`MySQLConfig.statement_cache_size`) instead of preparing it again every time.

### Date and time parameters

`DATE`/`DATETIME`/`TIME` parameters use the same `Dictionary` shape `get_rows()` returns
for that type — round-tripping a value read earlier needs no conversion:

```gdscript
var birthday = {"year": 1990, "month": 5, "day": 12}
session.execute_prepared("UPDATE users SET birthday = ? WHERE id = ?", [birthday, user_id])

var logged_in_at = {
    "year": 2026, "month": 9, "day": 22,
    "hour": 14, "minute": 30, "second": 0, "microsecond": 0,
}
session.execute_prepared("UPDATE users SET last_login = ? WHERE id = ?", [logged_in_at, user_id])
```

A `Dictionary` that does not match the `DATE`, `DATETIME` or `TIME` shape (or has a
component out of range) fails the call explicitly — see "Parameters" in
[features.md](features.md) for the exact shapes.

### Multiple statements in one call

```gdscript
config.allow_sql_script_execution = true  # off by default; emits a warning when enabled
var results = session.execute_script("""
    UPDATE accounts SET balance = balance - 100 WHERE id = 1;
    UPDATE accounts SET balance = balance + 100 WHERE id = 2;
""")
for r in results:
    if not r.is_ok():
        push_error(r.get_error())
```

## Asynchronous calls

```gdscript
func await_result(op: MySQLAsyncOperation) -> MySQLResult:
    if op.is_finished():
        return op.get_result()
    return await op.completed

var op := session.async_execute_text("SELECT SLEEP(1)")
var result := await await_result(op)
```

Four rules apply to every asynchronous call:

* **Await through an `is_finished()` check, like `await_result()` above.** `completed`
  fires only once: if the operation finished while something else was being awaited, a
  bare `await op.completed` never returns.
* **Always `await` the operation.** Do not poll it in a loop
  (`while not op.is_finished(): pass`) — that loop blocks the `SceneTree` from processing
  frames, and processing frames is what delivers the result.
* **Keep the `MySQLSession` (or the `MySQLPool` it came from) referenced until the
  operation finishes.** Freeing it earlier stops its I/O thread mid-operation.
* **A session runs one asynchronous operation at a time.** A second call on the same
  session, started while the first one is still running, fails immediately instead of
  queuing. Use a separate session per parallel operation — for example, one leased from a
  `MySQLPool`.

See "Asynchronous methods" in [features.md](features.md) for the full explanation.

## Transactions

```gdscript
var tx = session.begin_transaction()
if not tx.is_ok():
    push_error(tx.get_error())  # START TRANSACTION itself failed.
    return
var r1 = session.execute_prepared("UPDATE accounts SET balance = balance - ? WHERE id = ?", [amount, from_id])
var r2 = session.execute_prepared("UPDATE accounts SET balance = balance + ? WHERE id = ?", [amount, to_id])
if r1.is_ok() and r2.is_ok():
    tx.commit()
else:
    tx.rollback()
```

If `tx` goes out of scope without either call, the module rolls back automatically and
logs a warning — a safety net, not a substitute for calling `commit()`/`rollback()`
yourself on every path. `begin_transaction()` never returns `null`: if `START TRANSACTION`
fails, the returned transaction has `is_ok() == false`, and `commit()`/`rollback()` on it
return that same error.

## Streaming a large result

```gdscript
var cursor = session.execute_streaming("SELECT * FROM big_table")
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
var pool = MySQLPool.new()
pool.set_config(config)  # same MySQLConfig every acquired session will connect with

# From any thread:
var session = pool.acquire()
if not session.is_db_connected():
    session.connect_db()
var result = session.execute_text("SELECT 1")
```

Each thread must use its own acquired `MySQLSession` — never share one session between
threads at the same time. `pool.acquire()` blocks if the pool is already at
`MySQLPool.max_size`.

## Bounding result size

```gdscript
config.max_result_bytes = 10 * 1024 * 1024  # 10 MiB, default 0 = unlimited
var result = session.execute_text("SELECT * FROM potentially_huge_table")
if not result.is_ok():
    print(result.get_error())  # {"category": "mysql_module.client", "message": "Result exceeds max_result_bytes (...)", ...}
# the connection is still usable afterwards — the overflowing result is drained, not left mid-stream
```

## Multiple resultsets

A call that returns more than one resultset (a stored procedure, or `allow_multi_queries`
with several statements) exposes each one through an index:

```gdscript
var result = session.execute_text("CALL some_procedure()")
for i in result.get_resultset_count():
    print(result.get_column_names(i))
    print(result.get_rows(i))
```

## Error handling

This module never throws (`no_exception`, like Godot's own default) and keeps no global
error state. Every call reports its own outcome directly, one of two ways:

* `MySQLResult` and `MySQLStreamingCursor` expose `is_ok() -> bool` and
  `get_error() -> Dictionary`.
* `connect_db()`, `close_db()`, `commit()` and `rollback()` return the error
  `Dictionary` directly: check `.is_empty()` instead of calling `is_ok()`.

The error `Dictionary` always has the same four keys:

| Key | Meaning |
|---|---|
| `category` | Where the error came from: `"mysql.common-server"` for one the server reported, `"mysql_module.client"` for one the module detected on its own (bad parameters, a limit reached, and so on) before talking to the server. |
| `message` | A description written by the module. |
| `server_message` | The server's own error text; empty for a client-side error. |
| `is_fatal` | `true` if the connection itself is no longer usable (for example, after a failed TLS handshake); `false` if the session still accepts further calls. |

```gdscript
var result = session.execute_text("SELECT * FROM users")
if not result.is_ok():
    var error = result.get_error()
    print("%s: %s" % [error.category, error.message])
    if not error.server_message.is_empty():
        print("Server said: ", error.server_message)
    return
```

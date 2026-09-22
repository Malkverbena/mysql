# Tests

Two kinds of local tests, both versioned in the repository:

| Kind | Files | Needs a server | Runs with |
|---|---|---|---|
| Unit tests (doctest) | `test_*.h` | No | The engine test runner (`--test`) |
| Integration test (GDScript) | `smoke_test.gd` | Yes, MySQL or MariaDB | `--headless --script` |

The build instructions for both are in
[`../documentation/instructions.md`](../documentation/instructions.md), "Tests" section.

## Unit tests (doctest)

Godot picks up every `tests/test_*.h` of a module when the engine is built with
`tests=yes`. They call the module's internal C++ functions directly, so they can check
things the GDScript API cannot reach.

| File | What it covers |
|---|---|
| `test_mysql_type_convert.h` | The pure `field_view` to `Variant` conversion: integer ranges, `BIGINT UNSIGNED` above `INT64_MAX`, `TINYINT(1)` and `tinyint1_mode`, UTF-8 text, blobs, floats, `DATE`/`DATETIME` with microseconds, negative and above 24h `TIME`, and every `json_result_mode`. |
| `test_sql_script.h` | The SQL script splitter behind `execute_script()`: semicolons, quoted literals, doubled quotes, backslash escapes, empty statements, multi-byte text. |

Run them with:

```bash
./bin/godot.linuxbsd.editor.double.x86_64.tests --test --test-case="*MySQL*"
```

## Integration test (`smoke_test.gd`)

It covers the end-to-end path of every class exposed by the module: connection and TLS
validation, the three ways of running SQL (`execute_text`, `execute_formatted`,
`execute_prepared`), data types (including the hard cases: `BIGINT UNSIGNED` above
`INT64_MAX`, `TIME` above 24h with microseconds, `json_result_mode`, including which
behavior applies on MySQL vs. MariaDB), the prepared statement cache (reuse and eviction
with a configurable `statement_cache_size`, measured with the server's
`Com_stmt_prepare` and `Com_stmt_close` counters), the `max_result_bytes` result size
limit (on `execute_text`, `execute_prepared` and the asynchronous path, including that
the connection stays usable afterwards), transactions (`commit`, `rollback` and automatic
rollback), streaming, asynchronous calls (`async_execute_text`, `async_execute_prepared`
and `await`), the connection pool (including asynchronous calls on a recycled
connection), multiple resultsets and `execute_script`.

### Prerequisites

- The module already built together with Godot (see
  [`../documentation/instructions.md`](../documentation/instructions.md)).
- A MySQL or MariaDB server reachable, with a user and a schema **dedicated to
  testing**. The test creates and drops its own table in that schema and touches nothing
  else. Do not use a production schema.

**Never put credentials in this file or in any other versioned file.** They come only from
environment variables, read at run time:

| Variable | Required | Default |
|---|---|---|
| `MYSQL_TEST_HOST` | No | `127.0.0.1` |
| `MYSQL_TEST_PORT` | No | `3306` |
| `MYSQL_TEST_USER` | Yes | none |
| `MYSQL_TEST_PASSWORD` | Yes | none |
| `MYSQL_TEST_DATABASE` | Yes | none |

### Running

From the Godot tree already built with `custom_modules=../mysql`:

```bash
MYSQL_TEST_HOST=127.0.0.1 \
MYSQL_TEST_PORT=3306 \
MYSQL_TEST_USER=test_user \
MYSQL_TEST_PASSWORD='your_password_here' \
MYSQL_TEST_DATABASE=test_schema \
./bin/godot.<your_binary> --headless --script ../mysql/tests/smoke_test.gd
```

It exits with code `0` if every check passes, and `1` if any check fails (or if the
required environment variables are not set). It prints `OK`/`FAIL` per check and a total
at the end.

The test is safe to run more than once: the table (`t_mysql_module_smoke_test`) is
dropped at the start (if it exists) and at the end.

### If the server uses TLS with a self-signed certificate

This is expected and part of the test: section 0 confirms that `TCP_TLS_REQUIRED`
rejects an untrusted certificate instead of silently accepting it. The rest of the test
connects with `TCP_TLS_DISABLED` on purpose, assuming a trusted local or test network.
That is not the recommended setting for production (see
[`../documentation/features.md`](../documentation/features.md)).

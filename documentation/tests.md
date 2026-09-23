# Tests

The tests live in `mysql/tests/`, both versioned in the repository. Build the module
together with Godot first (see [instructions.md](instructions.md)); this page only
covers running the tests.

| Kind | Files | Needs a server | Runs with |
|---|---|---|---|
| Unit tests (doctest) | `test_*.h` | No | The engine test runner (`--test`) |
| Integration test (GDScript) | `smoke_test.gd` | Yes, MySQL or MariaDB | `--headless --script` (desktop) or an exported APK (Android) |

## Unit tests (doctest)

Godot picks up every `tests/test_*.h` file of a module when the engine is built with
`tests=yes`. They call the module's internal C++ functions directly, so they can check
things the GDScript API cannot reach, and need no database server. `extra_suffix=tests`
keeps this build separate from the normal one:

```bash
cd godot
scons platform=linuxbsd arch=x86_64 target=editor \
    custom_modules=../mysql \
    precision=double \
    tests=yes extra_suffix=tests \
    -j"$(nproc)"

./bin/godot.linuxbsd.editor.double.x86_64.tests --test --test-case="*MySQL*"
```

| File | What it covers |
|---|---|
| `test_mysql_type_convert.h` | The pure `field_view` to `Variant` conversion: integer ranges, `BIGINT UNSIGNED` above `INT64_MAX`, `TINYINT(1)` and `tinyint1_mode`, UTF-8 text, blobs, floats, `DATE`/`DATETIME` with microseconds, negative and above 24h `TIME`, and every `json_result_mode`. |
| `test_mysql_config.h` | Range checks of the numeric `MySQLConfig` setters (`port`, `max_buffer_size`, `async_timeout_ms`, `max_result_bytes`): an out-of-range value is rejected and the previous one kept. |
| `test_sql_script.h` | The SQL script splitter behind `execute_script()`: semicolons, quoted literals, doubled quotes, backslash escapes, `--`/`#`/`/* */` comments (including versioned `/*! */` ones and comment-only fragments), `NO_BACKSLASH_ESCAPES`, one-statement-at-a-time splitting, empty statements, multi-byte text. Also the placeholder scan `execute_formatted()` uses to tell a `?` in a literal or comment from a real placeholder. |

## Integration test (`smoke_test.gd`), desktop

Runs against a real MySQL/MariaDB server. It covers the end-to-end path of every class
the module exposes: connection and TLS validation, the three ways of running SQL
(`execute_text`, `execute_formatted`, `execute_prepared`), data types (including the hard
cases: `BIGINT UNSIGNED` above `INT64_MAX`, `TIME` above 24h with microseconds,
`json_result_mode`, including which behavior applies on MySQL vs. MariaDB), the prepared
statement cache (reuse and eviction with a configurable `statement_cache_size`, measured
with the server's `Com_stmt_prepare` and `Com_stmt_close` counters), the
`max_result_bytes` result size limit (on `execute_text`, `execute_prepared` and the
asynchronous path, including that the connection stays usable afterwards), transactions
(`commit`, `rollback` and automatic rollback), streaming, asynchronous calls
(`async_execute_text`, `async_execute_prepared` and `await`), the connection pool
(including asynchronous calls on a recycled connection, a session dropped with an
asynchronous operation still running, and no prepared statement leak across leases,
measured with the server's `Prepared_stmt_count`), failed `begin_transaction()`, multiple resultsets (including streaming
one), `execute_formatted` and `execute_script` with `?`/`;` inside literals and comments and
under `NO_BACKSLASH_ESCAPES`, and `execute_script`.

### Prerequisites

- The module built together with Godot (see [instructions.md](instructions.md)).
- A MySQL or MariaDB server reachable, with a user and a schema **dedicated to
  testing**. The test creates and drops its own table in that schema and touches nothing
  else. Do not use a production schema.

**Never put credentials in this file or in any other versioned file.** They come only
from environment variables, read at run time:

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
at the end. The test is safe to run more than once: the table
(`t_mysql_module_smoke_test`) is dropped at the start (if it exists) and at the end.

### If the server uses TLS with a self-signed certificate

This is expected and part of the test: the first section confirms that
`TCP_TLS_REQUIRED` rejects an untrusted certificate instead of silently accepting it. The
rest of the test connects with `TCP_TLS_DISABLED` on purpose, assuming a trusted local or
test network — not the recommended setting for production (see
[features.md](features.md)).

## Integration test (`smoke_test.gd`), Android

An exported Android app cannot use `--script` the way desktop/Wine can — reasons and the
working alternative below. To run `tests/smoke_test.gd` on a device:

1. Build a small Godot project whose only content is `tests/smoke_test.gd` (or a symlink
   to it) and a minimal main scene (a `.tscn` with a single empty `Node` is enough).
2. In its Project Settings, set **Run > Main Loop Type** to `MySQLSmokeTest` (the
   `class_name` the script declares) — **do not** rely on `command_line/extra_args =
   "--script res://smoke_test.gd"`. That argument does reach the native layer intact
   (visible in `adb logcat`), but was found to silently never execute on Android in this
   Godot build, reproduced on two different-vendor devices. `main_loop_type` is a
   supported, documented Godot mechanism and does not have this problem; it does need
   `run/main_scene` to point at a valid scene too (a bare script is not accepted there).
3. Export a **debug** APK (`--export-debug`) with that project. `INTERNET` permission is
   required.
4. Credentials: an installed app has no shell environment to read `MYSQL_TEST_*` from.
   `smoke_test.gd` falls back to `user://test_credentials.txt` (`KEY=VALUE` lines) when
   the environment variables are unset. Push it with `run-as` (needs a debug/debuggable
   build): `adb push credentials.txt /data/local/tmp/test_credentials.txt && adb shell
   run-as <package> cp /data/local/tmp/test_credentials.txt files/test_credentials.txt`.
5. A physical device (not the emulator) reaches the host's MySQL through
   `adb reverse tcp:3306 tcp:3306`, then the default `MYSQL_TEST_HOST` (`127.0.0.1`)
   works unchanged. (`10.0.2.2` is an emulator-only alias — do not use it on a real
   device.)
6. `adb shell am start -n <package>/<launcher activity>`, then read the result from
   `adb logcat` (tag `godot`); the script prints `OK`/`FAIL` per check and calls
   `quit(0)`/`quit(1)` at the end.

## Sanitizers

Godot has built-in options: add `use_asan=yes use_ubsan=yes`, or `use_tsan=yes` (TSan
cannot be combined with ASan), to the `scons` line and run the integration test with the
resulting binary.

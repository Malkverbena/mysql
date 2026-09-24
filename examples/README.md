# mysql module examples

A Godot project demonstrating the mysql module from GDScript. Each subfolder is one
self-contained example, with its own scene.

## Prerequisites

- Godot built together with this module (see
  [`../documentation/instructions.md`](../documentation/instructions.md)) — the plain
  Godot editor download does not have `MySQLSession`/`MySQLConfig`/etc. and cannot open
  these scenes.
- A MySQL or MariaDB server reachable from this machine, and a user with permission to
  create a schema (or an already-existing empty schema **dedicated to testing**). The
  schema itself does not need to exist yet: `connect_and_configure` creates it
  (`CREATE DATABASE IF NOT EXISTS`) the first time it runs. `transactions` and
  `streaming` create and populate their own tables inside it; the other examples create
  nothing (`async_streaming` and `cancel` have the server generate their rows). None of them touches anything else.

## Setup

Edit [`config.ini`](config.ini) with your test server's connection details:

```ini
[mysql]
host = "127.0.0.1"
port = 3306
user = "your_user"
password = "your_password"
database = "your_database"
```

**Never commit a real password.** `config.ini` is versioned (every example reads it), so
before committing or pushing, put a placeholder back in the `password` field — for
example `minha_super_senha` — the way the file ships in this repository.

Every example connects with `transport_mode = MySQLConfig.TCP_TLS_DISABLED` (see
`demo_config.gd`), because a local development server usually has no certificate signed
by a trusted CA. This is a demo convenience, not the recommended setting for production —
see "Intended use" in [`../documentation/features.md`](../documentation/features.md).

## Running an example

Open this folder (`examples/`) as a Godot project with the custom-built editor, then
either:

- Open one of the scenes listed below in the editor and press **F6** ("Run Current
  Scene"), or
- Press **F5** ("Run Project") — it starts on `connect_and_configure`, the first example.

**Run `connect_and_configure` at least once before the other examples** — it is what
creates the schema named in `config.ini`.

Each scene runs automatically as soon as it starts (no button to press) and prints its
progress both to the on-screen output panel and to the editor's **Output** dock. The
window stays open when the example finishes, so the output can be read; close it to stop.

An example can also run without a window, printing to the terminal:

```bash
godot --headless --path examples res://connect_and_configure/connect_and_configure.tscn
```

In that mode each scene quits by itself once it finishes (`DemoConfig.quit_if_headless()`).
On a fresh checkout, with no `.godot/` folder yet, open the project in the editor once
first (or run `godot --headless --editor --path examples --quit-after 1`): a headless run
alone does not build the class cache, and would not find the `DemoConfig` class.

## Examples

| Folder | Scene | What it demonstrates |
|---|---|---|
| [`connect_and_configure/`](connect_and_configure/) | `connect_and_configure.tscn` | The minimal path: build a `MySQLConfig`, create a `MySQLSession`, `connect_db()`, run one query, `close_db()`. Also creates the schema named in `config.ini` if it does not exist yet. Start here. |
| [`transactions/`](transactions/) | `transactions.tscn` | `begin_transaction()`, `commit()` and `rollback()`: transfers a balance between two rows once committed and once rolled back on purpose, printing the balances after each so the difference is visible. |
| [`streaming/`](streaming/) | `streaming.tscn` | `execute_streaming()`: reads a few thousand rows back in batches with `next_batch()`/`has_more()`, instead of loading the whole result into memory at once. |
| [`async/`](async/) | `async.tscn` | `async_execute_text()`/`async_execute_prepared()`: start a query without blocking, keep doing other work, then await the result through an `is_finished()` guard. Also shows the explicit error a second call gets while the session is still busy with the first. |
| [`async_streaming/`](async_streaming/) | `async_streaming.tscn` | `async_execute_streaming()`: reads 100,000 server-generated rows in batches with `async_next_batch()`, each one awaited, while a frame counter shows the game keeps running. The **Cancel** button cancels the batch in flight and closes the cursor with `async_close()`. |
| [`cancel/`](cancel/) | `cancel.tscn` | `MySQLAsyncOperation.cancel()`: starts a query that keeps the server busy for about 20 seconds; **Cancel** stops it on the server, the operation finishes with the "Query execution was interrupted" error, and the session stays usable. A headless run presses **Cancel** by itself after one second. |
| [`connection_pool/`](connection_pool/) | `connection_pool.tscn` | `MySQLPool`: six Godot `Thread`s share a pool of two connections, each leasing a session, running one query and releasing it for the next thread. |

`demo_config.gd` (project root) is shared by every example: it loads `config.ini` into a
`MySQLConfig`, and quits a headless run once the example finishes. It is not an example on its own.

See [`../documentation/usage.md`](../documentation/usage.md) for the full range of what
the module can do, and [`../documentation/features.md`](../documentation/features.md) for
the complete feature list.

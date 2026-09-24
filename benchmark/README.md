# mysql module benchmarks

A Godot project that measures the mysql module from GDScript. Each subfolder is one
self-contained benchmark with its own scene, like the [examples](../examples/). Each scene
prints its results as a Markdown table, both on screen and to the terminal or the
editor's **Output** dock. The results measured on the development machine are in
[RESULTS.md](RESULTS.md).

## Prerequisites and setup

The same as for the examples (see [`../examples/README.md`](../examples/README.md)): Godot
built together with this module, and a MySQL or MariaDB server with a schema **dedicated to
testing**. Edit [`config.ini`](config.ini) with that server's details.

**Never commit a real password.** `config.ini` is versioned; put a placeholder back in the
`password` field (for example `minha_super_senha`) before committing or pushing.

No benchmark creates a table: the rows come from recursive CTEs, and `type_conversion` uses
a temporary table, which the server drops with the session. The connection uses
`TCP_TLS_DISABLED`, which keeps TLS out of the numbers; this is a benchmark setting, not a
recommendation for production.

## Running

Headless, printing to the terminal (each scene quits by itself when it finishes):

```bash
godot --headless --editor --path benchmark --quit-after 1
godot --headless --path benchmark res://execution_modes/execution_modes.tscn
```

The first command builds the class cache on a fresh checkout (the scenes use the `Bench`
class from `bench.gd`); it is needed once. Scenes can also run from the editor with **F6**,
where the frame period follows the monitor's refresh rate instead of the headless one, which
changes the asynchronous rows of `sync_vs_async` and `streaming`.

For numbers that mean something:

- Run against a server on the same machine or network you care about. Latency to the
  server dominates the small queries.
- Compare rows of the same table with each other. An editor build is a debug build; a
  release export template is faster in absolute terms.
- Close other heavy programs, and run a scene more than once before trusting a difference.

## Benchmarks

| Folder | What it measures |
|---|---|
| [`execution_modes/`](execution_modes/) | Latency and calls per second of `execute_text`, `execute_formatted` and `execute_prepared` (with the statement in the cache, and prepared again on every call) on a one-row query. |
| [`sync_vs_async/`](sync_vs_async/) | The round trip of a trivial query, synchronous and awaited asynchronously, and the longest frame while a 300,000-row query runs each way. |
| [`streaming/`](streaming/) | A 200,000-row result read whole, with `next_batch()`, and with `async_next_batch()` for several `async_batch_rows` values: time, batches, frames, longest frame and peak memory. |
| [`pool/`](pool/) | `MySQLPool` throughput with 1, 2, 4 and 8 threads, leasing a session per query or once per thread. |
| [`type_conversion/`](type_conversion/) | The cost per row of converting each type (`INT`, `BIGINT`, `DOUBLE`, `DECIMAL`, `VARCHAR`, `BLOB`, `DATE`, `DATETIME`, `TIME`, and `JSON` in each `json_result_mode`) into a Godot `Variant`. |

`bench.gd` (project root) is shared by every benchmark: it loads `config.ini`, times calls,
and formats the Markdown tables.

# Benchmark results

Measured on 2026-09-24 by running every scene headless (see [README.md](README.md)), against a
MySQL server on the same machine (`127.0.0.1`, TLS off). These numbers describe this machine
and this build only — an editor build is a debug build, so a release export template is
faster — and are meant for comparing the cases of one table with each other, not as absolute
figures.

CPU: AMD Ryzen 9 9950X3D 16-Core Processor (32 threads)  
OS: Linux  
Godot 4.8-dev (custom_build) (debug build)  
Server: 8.4.11-0ubuntu0.26.04.1

## Execution modes

Scene: [`execution_modes/execution_modes.tscn`](execution_modes/execution_modes.tscn)

2000 timed calls per case, after 200 untimed warm-up calls. One row of three values per call.

| Case | Mean (µs) | p50 (µs) | p95 (µs) | p99 (µs) | Calls/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| execute_text | 10.1 | 10 | 11 | 13 | 99005 |
| execute_formatted | 10.6 | 11 | 11 | 13 | 94518 |
| execute_prepared (cached statement) | 9.3 | 9 | 10 | 11 | 107596 |
| execute_prepared (prepared on every call) | 26.8 | 27 | 28 | 30 | 37300 |

## Synchronous vs asynchronous

Scene: [`sync_vs_async/sync_vs_async.tscn`](sync_vs_async/sync_vs_async.tscn)

Display server: headless, measured frame period: 6.79 ms.

### Round trip of `SELECT 1` (500 calls, one at a time)

| Case | Mean (µs) | p50 (µs) | p95 (µs) | p99 (µs) | Calls/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| execute_text | 9.3 | 9 | 11 | 11 | 107158 |
| async_execute_text + await | 6861.6 | 6900 | 6903 | 6907 | 146 |

### Longest frame while a 300000-row query runs

| Case | Total (ms) | Frames | Longest frame (ms) | Rows |
| --- | ---: | ---: | ---: | ---: |
| execute_text | 74.7 | 2 | 74.7 | 300000 |
| async_execute_text + await | 96.7 | 16 | 6.9 | 300000 |

## Streaming

Scene: [`streaming/streaming.tscn`](streaming/streaming.tscn)

200000 rows of (INT, VARCHAR). Display server: headless, measured frame period: 6.79 ms (each asynchronous batch takes at least one frame).

| Case | Total (ms) | Rows/s | Batches | Frames | Longest frame (ms) | Peak memory (MiB) | Rows |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| execute_text (whole result) | 49.4 | 4046617 | 1 | 1 | 49.4 | 55.3 | 200000 |
| next_batch | 47.5 | 4209551 | 4122 | 1 | 47.5 | 0.0 | 200000 |
| async_next_batch, async_batch_rows = 100 | 11702.5 | 17090 | 1695 | 1696 | 7.0 | 0.0 | 200000 |
| async_next_batch, async_batch_rows = 500 | 2725.5 | 73382 | 394 | 395 | 7.0 | 0.1 | 200000 |
| async_next_batch, async_batch_rows = 2000 | 690.2 | 289792 | 99 | 100 | 7.2 | 0.5 | 200000 |
| async_next_batch, async_batch_rows = 10000 | 145.7 | 1372665 | 20 | 21 | 8.2 | 2.6 | 200000 |

## Connection pool

Scene: [`pool/pool.tscn`](pool/pool.tscn)

5000 `SELECT 1` queries per thread. Pool max_size = number of threads.

| Case | Total (ms) | Queries/s | Succeeded |
| --- | ---: | ---: | ---: |
| 1 thread(s), lease per query | 169.7 | 29465 | 5000 / 5000 |
| 1 thread(s), one lease per thread | 45.3 | 110317 | 5000 / 5000 |
| 2 thread(s), lease per query | 175.6 | 56960 | 10000 / 10000 |
| 2 thread(s), one lease per thread | 48.7 | 205499 | 10000 / 10000 |
| 4 thread(s), lease per query | 199.7 | 100145 | 20000 / 20000 |
| 4 thread(s), one lease per thread | 50.0 | 399800 | 20000 / 20000 |
| 8 thread(s), lease per query | 259.8 | 153948 | 40000 / 40000 |
| 8 thread(s), one lease per thread | 62.5 | 639857 | 40000 / 40000 |

## Type conversion

Scene: [`type_conversion/type_conversion.tscn`](type_conversion/type_conversion.tscn)

50000 rows of one column per case, stored in a temporary table and read with execute_text. Fastest of 3 runs.

| Type | Total (ms) | µs/row | vs INT (µs/row) | Rows/s |
| --- | ---: | ---: | ---: | ---: |
| INT | 9.7 | 0.19 | +0.00 | 5137162 |
| BIGINT | 10.0 | 0.20 | +0.01 | 5008514 |
| DOUBLE | 16.9 | 0.34 | +0.14 | 2962261 |
| DECIMAL (as String) | 11.0 | 0.22 | +0.03 | 4539677 |
| VARCHAR (100 chars) | 11.1 | 0.22 | +0.03 | 4501666 |
| BLOB (1 KiB) | 23.7 | 0.47 | +0.28 | 2106949 |
| DATE | 22.0 | 0.44 | +0.25 | 2273864 |
| DATETIME(6) | 40.6 | 0.81 | +0.62 | 1230376 |
| TIME(6) | 31.6 | 0.63 | +0.44 | 1582779 |
| JSON, RAW_STRING | 23.6 | 0.47 | +0.28 | 2120171 |
| JSON, PARSED_VARIANT | 61.2 | 1.22 | +1.03 | 816820 |
| JSON, LAZY_PARSED_VARIANT | 23.3 | 0.47 | +0.27 | 2147490 |
| JSON, LAZY + get_parsed_json() on every row | 84.3 | 1.69 | +1.49 | 592909 |

## What the numbers say

- **Execution modes:** on a local server the three modes cost about the same, around 10 µs
  per call. A prepared statement is only worth it when it stays in the cache: preparing it on
  every call (and closing the evicted one) costs almost three times as much. Keep
  `statement_cache_size` above the number of distinct statements a session runs.
- **Synchronous vs asynchronous:** an awaited asynchronous call waits for the next frame, so
  calls awaited one after another run at one per frame. In return, the frames keep their
  period while a heavy query runs; the synchronous call stalls the frame for the whole query.
  Use the asynchronous methods for queries that can take longer than a frame, not for many
  tiny ones in a row.
- **Streaming:** `async_batch_rows` sets the trade-off. Every batch costs at least a frame,
  so small batches make a large result take many frames; large batches hold more rows in
  memory at once. Both cursors keep memory flat compared to reading the whole result.
- **Connection pool:** a lease resets the connection (one extra round trip), which costs
  more than a trivial query. Lease a session once per task, not once per query.
  Throughput grows with the number of threads.
- **Type conversion:** plain numbers and text are the cheapest. `DATE`/`DATETIME`/`TIME`
  (converted to a `Dictionary`) cost more, and so does JSON parsed into a `Variant`.
  `LAZY_PARSED_VARIANT`, the default, costs the same as `RAW_STRING` until
  `get_parsed_json()` is called on a cell.

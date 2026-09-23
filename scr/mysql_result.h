/* mysql_result.h */
#pragma once

#include "mysql_config.h"

#include "core/object/ref_counted.h"
#include "core/math/vector3i.h"
#include "core/templates/hash_map.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#include <boost/mysql/column_type.hpp>
#include <boost/mysql/metadata_collection_view.hpp>
#include <boost/mysql/rows_view.hpp>
#include <boost/mysql/string_view.hpp>

// Plain data returned by an execution: ordered metadata, rows (an `Array` of `Array`s,
// which keeps the duplicate column names a JOIN can produce, never a `Dictionary` by
// name), `affected_rows` and `last_insert_id` in 64 bits (a `String` if a `BIGINT
// UNSIGNED` goes above `INT64_MAX`), and every resultset when there are several.
// `is_ok()` and `get_error()` (a `Dictionary` with `category`, `message`, `server_message`
// and `is_fatal`) replace any ambient error state.
//
// `get_parsed_json()` is the only operation that is not fully "pure": with
// `json_result_mode == LAZY_PARSED_VARIANT` the rows keep the raw text, and the JSON is only
// parsed (and cached) when explicitly requested.
class MySQLResult : public RefCounted {
	GDCLASS(MySQLResult, RefCounted);

	struct ResultsetData {
		Vector<String> column_names;
		Vector<boost::mysql::column_type> column_types;
		Array rows; // An `Array` of `Array`s. Each row holds its values in column order.
		Variant affected_rows;
		Variant last_insert_id;
		String info;
	};

	bool ok = true;
	Dictionary error;
	Vector<ResultsetData> resultsets;
	// Keyed by (resultset, row, column).
	HashMap<Vector3i, Variant> json_cache;

	const ResultsetData *_get_resultset(int p_index) const;

protected:
	static void _bind_methods();

public:
	// Incrementally assembles a `MySQLResult` from `execution_state`
	// (`start_execution`/`read_some_rows`/`read_resultset_head`), one resultset and one row
	// batch at a time, tracking an estimated total size as it goes. Used by both the
	// synchronous execute path (a blocking loop) and the asynchronous one (chained
	// completion handlers) to enforce `max_result_bytes` by aborting as soon as the running
	// total goes over the limit, instead of letting Boost.MySQL materialize the whole result
	// first. A nested class so it can build `ResultsetData` directly.
	//
	// Not internally synchronized: used from a single thread (the caller's thread for the
	// sync path, the I/O thread for the async one), same as the rest of a `MySQLConnection`.
	class Builder {
		Vector<ResultsetData> resultsets;
		uint64_t estimated_bytes = 0;

	public:
		// Starts a new resultset from its just-read column metadata.
		void begin_resultset(const boost::mysql::metadata_collection_view &p_meta);
		// Appends a batch of rows (from `read_some_rows()`) to the current resultset, adding
		// their estimated size to the running total.
		void add_rows(const boost::mysql::rows_view &p_rows, const boost::mysql::metadata_collection_view &p_meta, const Ref<MySQLConfig> &p_config);
		// Fills in the current resultset's `affected_rows`/`last_insert_id`/`info`, available
		// once its rows are fully read (`execution_state::should_read_head()` or `complete()`).
		void end_resultset(uint64_t p_affected_rows, uint64_t p_last_insert_id, const boost::mysql::string_view &p_info);

		uint64_t get_estimated_bytes() const { return estimated_bytes; }

		// Wraps what has been built so far into an `ok` result. Called once the whole
		// execution is complete, or, on a `max_result_bytes` overflow, not at all (the caller
		// builds an error `MySQLResult` instead and discards the builder).
		Ref<MySQLResult> finish();
	};

	// Internal use by `MySQLSession`, not bound.
	static Ref<MySQLResult> from_error(const Dictionary &p_error);

	bool is_ok() const { return ok; }
	Dictionary get_error() const { return error; }

	int get_resultset_count() const { return resultsets.size(); }
	PackedStringArray get_column_names(int p_resultset = 0) const;
	Array get_rows(int p_resultset = 0) const;
	Variant get_affected_rows(int p_resultset = 0) const;
	Variant get_last_insert_id(int p_resultset = 0) const;
	String get_info(int p_resultset = 0) const;

	// Parses (with a cache) a JSON cell kept as raw text. It only makes sense with
	// `json_result_mode == LAZY_PARSED_VARIANT`; in the other modes the cell already comes
	// ready in `get_rows()`.
	Variant get_parsed_json(int p_resultset, int p_row, int p_column);
};

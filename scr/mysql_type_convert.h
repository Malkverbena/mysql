// SPDX-License-Identifier: MIT
/* mysql_type_convert.h */
#pragma once

#include "mysql_config.h"

#include "core/variant/variant.h"

#include <boost/mysql/field_view.hpp>
#include <boost/mysql/metadata.hpp>

// MySQL/MariaDB to Godot conversion. These are pure functions: they do no I/O and keep no
// state. Given a `field_view` (the value of a cell) and the column metadata, they return
// the matching `Variant`, following the rules in `documentation/features.md`
// (`tinyint1_mode`, `json_result_mode`, `BIGINT UNSIGNED` above `INT64_MAX` as a `String`,
// and `TIME`/`DATE`/`DATETIME` with correct microseconds and sign).
//
// `json_result_mode == LAZY_PARSED_VARIANT` returns the raw string here (the same as
// `RAW_STRING`). Parsing on demand, with a cache, is the job of `MySQLResult`, not of this
// pure function.
namespace mysql_module {

Variant field_to_variant(const boost::mysql::field_view &p_field, const boost::mysql::metadata &p_meta, const Ref<MySQLConfig> &p_config);

// Estimated size in bytes of a single cell, used by `max_result_bytes` to bound the total
// size of a result while it is being read. Exact for `string`/`blob` (their real length); a
// fixed small cost for every other kind (int64/uint64/float/double/date/datetime/time/null).
// It is an estimate of the `Variant` payload, not the protocol wire size.
uint64_t estimate_field_bytes(const boost::mysql::field_view &p_field);

// Used both by the field conversion (`TINYINT`, `INT`, `BIGINT`) and by
// `MySQLResult::from_boost_results()` (`affected_rows` and `last_insert_id`, which are also
// `uint64_t` in Boost.MySQL).
Variant uint64_to_variant(uint64_t p_value);

} //namespace mysql_module

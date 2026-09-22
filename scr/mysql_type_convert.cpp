/* mysql_type_convert.cpp */

#include "mysql_type_convert.h"

#include "godot_convert.h"

#include "core/io/json.h"
#include "core/variant/dictionary.h"

#include <boost/mysql/blob_view.hpp>
#include <boost/mysql/column_type.hpp>
#include <boost/mysql/string_view.hpp>

#include <cstring>

namespace mysql_module {

namespace {

Dictionary date_to_dict(const boost::mysql::date &p_date) {
	Dictionary d;
	d["year"] = p_date.year();
	d["month"] = p_date.month();
	d["day"] = p_date.day();
	return d;
}

Dictionary datetime_to_dict(const boost::mysql::datetime &p_dt) {
	Dictionary d;
	d["year"] = p_dt.year();
	d["month"] = p_dt.month();
	d["day"] = p_dt.day();
	d["hour"] = p_dt.hour();
	d["minute"] = p_dt.minute();
	d["second"] = p_dt.second();
	d["microsecond"] = (int64_t)p_dt.microsecond();
	return d;
}

Dictionary time_to_dict(const boost::mysql::time &p_time) {
	// `boost::mysql::time` is a signed `std::chrono::microseconds`. The sign of the TIME is
	// kept by decomposing the absolute value and storing the sign separately, instead of
	// truncating or getting it wrong for negative values.
	int64_t total_us = p_time.count();
	bool negative = total_us < 0;
	uint64_t magnitude = (uint64_t)(negative ? -total_us : total_us);

	Dictionary d;
	d["negative"] = negative;
	d["hours"] = (int64_t)(magnitude / 3600000000ULL);
	d["minutes"] = (int64_t)((magnitude / 60000000ULL) % 60);
	d["seconds"] = (int64_t)((magnitude / 1000000ULL) % 60);
	d["microsecond"] = (int64_t)(magnitude % 1000000ULL);
	return d;
}

bool is_width_one_tinyint(const boost::mysql::metadata &p_meta) {
	return p_meta.type() == boost::mysql::column_type::tinyint && p_meta.column_length() == 1;
}

} //namespace

Variant uint64_to_variant(uint64_t p_value) {
	// A `BIGINT UNSIGNED` above `INT64_MAX` comes as a `String` with the exact value,
	// because Godot's `Variant::INT` is a signed 64-bit integer and cannot hold it. See
	// `documentation/features.md`: the same column can return an `int` or a `String`
	// depending on the value of the row.
	if (p_value > (uint64_t)INT64_MAX) {
		return String::num_uint64(p_value);
	}
	return (int64_t)p_value;
}

Variant field_to_variant(const boost::mysql::field_view &p_field, const boost::mysql::metadata &p_meta, const Ref<MySQLConfig> &p_config) {
	if (p_field.is_null()) {
		return Variant();
	}

	if (p_field.is_int64()) {
		int64_t v = p_field.get_int64();
		if (p_config->get_tinyint1_mode() && is_width_one_tinyint(p_meta)) {
			return v != 0;
		}
		return (int64_t)v;
	}

	if (p_field.is_uint64()) {
		uint64_t v = p_field.get_uint64();
		if (p_config->get_tinyint1_mode() && is_width_one_tinyint(p_meta)) {
			return v != 0;
		}
		return uint64_to_variant(v);
	}

	if (p_field.is_string()) {
		boost::mysql::string_view sv = p_field.get_string();
		String text = to_godot_string(sv.data(), sv.size());

		if (p_meta.type() == boost::mysql::column_type::json && p_config->get_json_result_mode() == MySQLConfig::PARSED_VARIANT) {
			return JSON::parse_string(text);
		}
		// `RAW_STRING` and `LAZY_PARSED_VARIANT` return the raw text here. The lazy mode
		// parses on demand, with a cache, in `MySQLResult`, not here: this function is pure
		// and keeps no state.
		return text;
	}

	if (p_field.is_blob()) {
		boost::mysql::blob_view bv = p_field.get_blob();
		PackedByteArray bytes;
		bytes.resize((int)bv.size());
		if (!bv.empty()) {
			memcpy(bytes.ptrw(), bv.data(), bv.size());
		}
		return bytes;
	}

	if (p_field.is_float()) {
		return (double)p_field.get_float();
	}

	if (p_field.is_double()) {
		return p_field.get_double();
	}

	if (p_field.is_date()) {
		return date_to_dict(p_field.get_date());
	}

	if (p_field.is_datetime()) {
		return datetime_to_dict(p_field.get_datetime());
	}

	if (p_field.is_time()) {
		return time_to_dict(p_field.get_time());
	}

	return Variant();
}

uint64_t estimate_field_bytes(const boost::mysql::field_view &p_field) {
	if (p_field.is_string()) {
		return (uint64_t)p_field.get_string().size();
	}
	if (p_field.is_blob()) {
		return (uint64_t)p_field.get_blob().size();
	}
	if (p_field.is_null()) {
		return 0;
	}
	// int64/uint64/float/double/date/datetime/time: small, fixed-size types. 8 bytes is a
	// safe upper bound for all of them.
	return 8;
}

} //namespace mysql_module

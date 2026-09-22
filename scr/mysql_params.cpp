/* mysql_params.cpp */

#include "mysql_params.h"

#include <boost/mysql/blob_view.hpp>
#include <boost/mysql/date.hpp>
#include <boost/mysql/datetime.hpp>
#include <boost/mysql/string_view.hpp>
#include <boost/mysql/time.hpp>

#include <chrono>

namespace mysql_module {

namespace {

// Shape checks mirror `date_to_dict()`/`datetime_to_dict()`/`time_to_dict()` in
// `mysql_type_convert.cpp` exactly (same keys, singular for DATETIME's `hour`/`minute`/
// `second` vs. plural for TIME's `hours`/`minutes`/`seconds`, which is what tells the two
// apart). `size()` is checked too, so an extra or misspelled key is rejected instead of
// silently ignored.

bool is_date_dict(const Dictionary &p_dict) {
	return p_dict.size() == 3 && p_dict.has("year") && p_dict.has("month") && p_dict.has("day");
}

bool is_datetime_dict(const Dictionary &p_dict) {
	return p_dict.size() == 7 && p_dict.has("year") && p_dict.has("month") && p_dict.has("day") &&
			p_dict.has("hour") && p_dict.has("minute") && p_dict.has("second") && p_dict.has("microsecond");
}

bool is_time_dict(const Dictionary &p_dict) {
	return p_dict.size() == 5 && p_dict.has("negative") && p_dict.has("hours") && p_dict.has("minutes") &&
			p_dict.has("seconds") && p_dict.has("microsecond");
}

// `date`/`datetime` store their components as `uint16_t`/`uint8_t`/`uint32_t`. A
// too-large `int64_t` narrowed straight into one of those would wrap around instead of
// failing (for example month 300 truncates to 44, which `valid()` would then wrongly
// accept as "April"-shaped nonsense for a different reason). Checking the storage range
// first, before narrowing, turns that into an explicit error instead of a wrong value
// that happens to look plausible.
bool fits_in_range(int64_t p_value, int64_t p_max) {
	return p_value >= 0 && p_value <= p_max;
}

bool dict_to_date_field(const Dictionary &p_dict, boost::mysql::field_view &r_view, String &r_error_message) {
	int64_t year = p_dict["year"];
	int64_t month = p_dict["month"];
	int64_t day = p_dict["day"];
	if (!fits_in_range(year, UINT16_MAX) || !fits_in_range(month, UINT8_MAX) || !fits_in_range(day, UINT8_MAX)) {
		r_error_message = "DATE Dictionary parameter has a component out of range (year must fit uint16, month/day must fit uint8, and none of them may be negative).";
		return false;
	}
	boost::mysql::date value((uint16_t)year, (uint8_t)month, (uint8_t)day);
	if (!value.valid()) {
		r_error_message = "DATE Dictionary parameter is not a valid date (year must be 0-9999, month 1-12, day 1 to the last day of that month).";
		return false;
	}
	r_view = boost::mysql::field_view(value);
	return true;
}

bool dict_to_datetime_field(const Dictionary &p_dict, boost::mysql::field_view &r_view, String &r_error_message) {
	int64_t year = p_dict["year"];
	int64_t month = p_dict["month"];
	int64_t day = p_dict["day"];
	int64_t hour = p_dict["hour"];
	int64_t minute = p_dict["minute"];
	int64_t second = p_dict["second"];
	int64_t microsecond = p_dict["microsecond"];
	if (!fits_in_range(year, UINT16_MAX) || !fits_in_range(month, UINT8_MAX) || !fits_in_range(day, UINT8_MAX) ||
			!fits_in_range(hour, UINT8_MAX) || !fits_in_range(minute, UINT8_MAX) || !fits_in_range(second, UINT8_MAX) ||
			!fits_in_range(microsecond, UINT32_MAX)) {
		r_error_message = "DATETIME Dictionary parameter has a component out of range (year must fit uint16, month/day/hour/minute/second must fit uint8, microsecond must fit uint32, and none of them may be negative).";
		return false;
	}
	boost::mysql::datetime value((uint16_t)year, (uint8_t)month, (uint8_t)day, (uint8_t)hour, (uint8_t)minute, (uint8_t)second, (uint32_t)microsecond);
	if (!value.valid()) {
		r_error_message = "DATETIME Dictionary parameter is not a valid date/time (year 0-9999, month 1-12, day 1 to the last day of that month, hour 0-23, minute/second 0-59, microsecond 0-999999).";
		return false;
	}
	r_view = boost::mysql::field_view(value);
	return true;
}

bool dict_to_time_field(const Dictionary &p_dict, boost::mysql::field_view &r_view, String &r_error_message) {
	bool negative = p_dict["negative"];
	int64_t hours = p_dict["hours"];
	int64_t minutes = p_dict["minutes"];
	int64_t seconds = p_dict["seconds"];
	int64_t microsecond = p_dict["microsecond"];
	if (hours < 0 || minutes < 0 || minutes > 59 || seconds < 0 || seconds > 59 || microsecond < 0 || microsecond > 999999) {
		r_error_message = "TIME Dictionary parameter has an out-of-range component (minutes/seconds must be 0-59, microsecond must be 0-999999; hours/minutes/seconds/microsecond are always non-negative magnitudes — use \"negative\" for the sign).";
		return false;
	}
	boost::mysql::time value = std::chrono::hours(hours) + std::chrono::minutes(minutes) + std::chrono::seconds(seconds) + std::chrono::microseconds(microsecond);
	if (negative) {
		value = -value;
	}
	if (value < boost::mysql::min_time || value > boost::mysql::max_time) {
		r_error_message = "TIME Dictionary parameter is outside the MySQL range (-838:59:59.999999 to 838:59:59.999999).";
		return false;
	}
	r_view = boost::mysql::field_view(value);
	return true;
}

// A `Dictionary` parameter becomes DATE, DATETIME or TIME depending on which of the
// three shapes its keys match; anything else is an explicit error, matching every other
// unsupported parameter type.
bool dictionary_to_field_view(const Dictionary &p_dict, boost::mysql::field_view &r_view, String &r_error_message) {
	if (is_time_dict(p_dict)) {
		return dict_to_time_field(p_dict, r_view, r_error_message);
	}
	if (is_datetime_dict(p_dict)) {
		return dict_to_datetime_field(p_dict, r_view, r_error_message);
	}
	if (is_date_dict(p_dict)) {
		return dict_to_date_field(p_dict, r_view, r_error_message);
	}
	r_error_message = "Dictionary parameter does not match a supported shape: DATE {year, month, day}, "
					   "DATETIME {year, month, day, hour, minute, second, microsecond} or "
					   "TIME {negative, hours, minutes, seconds, microsecond}.";
	return false;
}

} //namespace

bool array_to_field_params(const Array &p_params, FieldParams &r_params, String &r_error_message) {
	int count = p_params.size();
	r_params.string_storage.reserve(count);
	r_params.blob_storage.reserve(count);
	r_params.views.reserve(count);

	for (int i = 0; i < count; i++) {
		const Variant &v = p_params[i];
		switch (v.get_type()) {
			case Variant::NIL:
				r_params.views.push_back(boost::mysql::field_view());
				break;

			case Variant::BOOL:
			case Variant::INT:
				r_params.views.push_back(boost::mysql::field_view((int64_t)v));
				break;

			case Variant::FLOAT:
				r_params.views.push_back(boost::mysql::field_view((double)v));
				break;

			case Variant::STRING:
			case Variant::STRING_NAME: {
				r_params.string_storage.push_back(((String)v).utf8());
				const CharString &stored = r_params.string_storage[r_params.string_storage.size() - 1];
				r_params.views.push_back(boost::mysql::field_view(boost::mysql::string_view(stored.get_data(), stored.length())));
				break;
			}

			case Variant::PACKED_BYTE_ARRAY: {
				r_params.blob_storage.push_back((PackedByteArray)v);
				const PackedByteArray &stored = r_params.blob_storage[r_params.blob_storage.size() - 1];
				r_params.views.push_back(boost::mysql::field_view(boost::mysql::blob_view(stored.ptr(), stored.size())));
				break;
			}

			case Variant::DICTIONARY: {
				boost::mysql::field_view view;
				String dict_error;
				if (!dictionary_to_field_view((Dictionary)v, view, dict_error)) {
					r_error_message = vformat("Parameter at index %d: %s", i, dict_error);
					return false;
				}
				r_params.views.push_back(view);
				break;
			}

			default:
				// `Array` and any other remaining `Variant::Type` have no defined translation.
				// Fail explicitly instead of converting to something silently wrong.
				r_error_message = vformat("Unsupported parameter type at index %d: %s.", i, Variant::get_type_name(v.get_type()));
				return false;
		}
	}
	return true;
}

} //namespace mysql_module

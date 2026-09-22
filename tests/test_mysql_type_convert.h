/* test_mysql_type_convert.h */
#pragma once

#include "../scr/mysql_type_convert.h"

#include "core/variant/dictionary.h"
#include "tests/test_macros.h"

#include <boost/mysql/blob_view.hpp>
#include <boost/mysql/column_type.hpp>
#include <boost/mysql/date.hpp>
#include <boost/mysql/datetime.hpp>
#include <boost/mysql/detail/access.hpp>
#include <boost/mysql/detail/coldef_view.hpp>
#include <boost/mysql/field_view.hpp>
#include <boost/mysql/metadata.hpp>

#include <chrono>

// Unit tests for the pure `field_view` -> `Variant` conversion. No server is needed.
// `boost::mysql::metadata` has no public constructor, so the tests build it through
// `detail::access`, the same path Boost's own tests use. If a Boost upgrade breaks this
// helper, fix it here; the module code never touches `detail`.
namespace TestMySQLTypeConvert {

static boost::mysql::metadata make_meta(boost::mysql::column_type p_type, uint32_t p_column_length = 0) {
	boost::mysql::detail::coldef_view coldef{};
	coldef.type = p_type;
	coldef.column_length = p_column_length;
	return boost::mysql::detail::access::construct<boost::mysql::metadata>(coldef, false);
}

static Ref<MySQLConfig> make_config() {
	Ref<MySQLConfig> config;
	config.instantiate();
	return config;
}

TEST_CASE("[Modules][MySQL] NULL becomes nil") {
	Ref<MySQLConfig> config = make_config();
	Variant v = mysql_module::field_to_variant(boost::mysql::field_view(), make_meta(boost::mysql::column_type::int_), config);
	CHECK(v.get_type() == Variant::NIL);
}

TEST_CASE("[Modules][MySQL] Signed integers keep their full 64-bit range") {
	Ref<MySQLConfig> config = make_config();
	boost::mysql::metadata meta = make_meta(boost::mysql::column_type::bigint);

	Variant max_value = mysql_module::field_to_variant(boost::mysql::field_view(std::int64_t(INT64_MAX)), meta, config);
	CHECK(max_value.get_type() == Variant::INT);
	CHECK(int64_t(max_value) == INT64_MAX);

	Variant min_value = mysql_module::field_to_variant(boost::mysql::field_view(std::int64_t(INT64_MIN)), meta, config);
	CHECK(min_value.get_type() == Variant::INT);
	CHECK(int64_t(min_value) == INT64_MIN);
}

TEST_CASE("[Modules][MySQL] BIGINT UNSIGNED above INT64_MAX becomes an exact String") {
	Ref<MySQLConfig> config = make_config();
	boost::mysql::metadata meta = make_meta(boost::mysql::column_type::bigint);

	Variant at_limit = mysql_module::field_to_variant(boost::mysql::field_view(std::uint64_t(INT64_MAX)), meta, config);
	CHECK(at_limit.get_type() == Variant::INT);
	CHECK(int64_t(at_limit) == INT64_MAX);

	Variant just_above = mysql_module::field_to_variant(boost::mysql::field_view(std::uint64_t(INT64_MAX) + 1), meta, config);
	CHECK(just_above.get_type() == Variant::STRING);
	CHECK(String(just_above) == "9223372036854775808");

	Variant uint64_max = mysql_module::field_to_variant(boost::mysql::field_view(std::uint64_t(UINT64_MAX)), meta, config);
	CHECK(uint64_max.get_type() == Variant::STRING);
	CHECK(String(uint64_max) == "18446744073709551615");
}

TEST_CASE("[Modules][MySQL] uint64_to_variant switches type exactly at INT64_MAX") {
	CHECK(mysql_module::uint64_to_variant(0).get_type() == Variant::INT);
	CHECK(mysql_module::uint64_to_variant(std::uint64_t(INT64_MAX)).get_type() == Variant::INT);
	CHECK(mysql_module::uint64_to_variant(std::uint64_t(INT64_MAX) + 1).get_type() == Variant::STRING);
}

TEST_CASE("[Modules][MySQL] TINYINT(1) becomes bool only when tinyint1_mode is on and the width is 1") {
	Ref<MySQLConfig> config = make_config();
	boost::mysql::metadata tinyint1 = make_meta(boost::mysql::column_type::tinyint, 1);
	boost::mysql::metadata tinyint4 = make_meta(boost::mysql::column_type::tinyint, 4);

	config->set_tinyint1_mode(false);
	CHECK(mysql_module::field_to_variant(boost::mysql::field_view(std::int64_t(1)), tinyint1, config).get_type() == Variant::INT);

	config->set_tinyint1_mode(true);
	Variant as_true = mysql_module::field_to_variant(boost::mysql::field_view(std::int64_t(1)), tinyint1, config);
	CHECK(as_true.get_type() == Variant::BOOL);
	CHECK(bool(as_true));
	Variant as_false = mysql_module::field_to_variant(boost::mysql::field_view(std::int64_t(0)), tinyint1, config);
	CHECK(as_false.get_type() == Variant::BOOL);
	CHECK_FALSE(bool(as_false));

	// A wider TINYINT must stay an integer even with the mode on.
	Variant wide = mysql_module::field_to_variant(boost::mysql::field_view(std::int64_t(1)), tinyint4, config);
	CHECK(wide.get_type() == Variant::INT);
	// A non-TINYINT column with column_length 1 must not be turned into a bool either.
	Variant other = mysql_module::field_to_variant(boost::mysql::field_view(std::int64_t(1)), make_meta(boost::mysql::column_type::smallint, 1), config);
	CHECK(other.get_type() == Variant::INT);
}

TEST_CASE("[Modules][MySQL] Text is decoded as UTF-8 using its length") {
	Ref<MySQLConfig> config = make_config();
	boost::mysql::metadata meta = make_meta(boost::mysql::column_type::varchar);

	const char *utf8_text = "caf\xc3\xa9 \xe6\x97\xa5\xe6\x9c\xac";
	Variant v = mysql_module::field_to_variant(boost::mysql::field_view(boost::mysql::string_view(utf8_text)), meta, config);
	CHECK(v.get_type() == Variant::STRING);
	CHECK(String(v) == String::utf8(utf8_text));
	CHECK(String(v).length() == 7);

	// The view is not NUL-terminated at `size`: only the first 3 bytes may be read.
	Variant truncated = mysql_module::field_to_variant(boost::mysql::field_view(boost::mysql::string_view("abcdef", 3)), meta, config);
	CHECK(String(truncated) == "abc");

	Variant empty = mysql_module::field_to_variant(boost::mysql::field_view(boost::mysql::string_view("")), meta, config);
	CHECK(empty.get_type() == Variant::STRING);
	CHECK(String(empty).is_empty());
}

TEST_CASE("[Modules][MySQL] BLOB becomes PackedByteArray with the exact bytes") {
	Ref<MySQLConfig> config = make_config();
	boost::mysql::metadata meta = make_meta(boost::mysql::column_type::blob);

	const unsigned char raw[] = { 0x00, 0xff, 0x10, 0x00, 0x7f };
	Variant v = mysql_module::field_to_variant(boost::mysql::field_view(boost::mysql::blob_view(raw, sizeof(raw))), meta, config);
	REQUIRE(v.get_type() == Variant::PACKED_BYTE_ARRAY);
	PackedByteArray bytes = v;
	REQUIRE(bytes.size() == 5);
	for (int i = 0; i < 5; i++) {
		CHECK(bytes[i] == raw[i]);
	}

	Variant empty = mysql_module::field_to_variant(boost::mysql::field_view(boost::mysql::blob_view()), meta, config);
	CHECK(empty.get_type() == Variant::PACKED_BYTE_ARRAY);
	CHECK(PackedByteArray(empty).is_empty());
}

TEST_CASE("[Modules][MySQL] FLOAT and DOUBLE become float Variants") {
	Ref<MySQLConfig> config = make_config();

	Variant f = mysql_module::field_to_variant(boost::mysql::field_view(1.5f), make_meta(boost::mysql::column_type::float_), config);
	CHECK(f.get_type() == Variant::FLOAT);
	CHECK(double(f) == doctest::Approx(1.5));

	Variant d = mysql_module::field_to_variant(boost::mysql::field_view(0.1), make_meta(boost::mysql::column_type::double_), config);
	CHECK(d.get_type() == Variant::FLOAT);
	CHECK(double(d) == 0.1);
}

TEST_CASE("[Modules][MySQL] DATE and DATETIME keep every component, including microseconds") {
	Ref<MySQLConfig> config = make_config();

	Variant date = mysql_module::field_to_variant(boost::mysql::field_view(boost::mysql::date(2026, 9, 21)), make_meta(boost::mysql::column_type::date), config);
	REQUIRE(date.get_type() == Variant::DICTIONARY);
	Dictionary date_dict = date;
	CHECK(int(date_dict["year"]) == 2026);
	CHECK(int(date_dict["month"]) == 9);
	CHECK(int(date_dict["day"]) == 21);

	Variant datetime = mysql_module::field_to_variant(boost::mysql::field_view(boost::mysql::datetime(2026, 9, 21, 13, 45, 59, 123456)), make_meta(boost::mysql::column_type::datetime), config);
	REQUIRE(datetime.get_type() == Variant::DICTIONARY);
	Dictionary datetime_dict = datetime;
	CHECK(int(datetime_dict["year"]) == 2026);
	CHECK(int(datetime_dict["month"]) == 9);
	CHECK(int(datetime_dict["day"]) == 21);
	CHECK(int(datetime_dict["hour"]) == 13);
	CHECK(int(datetime_dict["minute"]) == 45);
	CHECK(int(datetime_dict["second"]) == 59);
	CHECK(int64_t(datetime_dict["microsecond"]) == 123456);
}

TEST_CASE("[Modules][MySQL] TIME above 24h and negative values decompose correctly") {
	Ref<MySQLConfig> config = make_config();
	boost::mysql::metadata meta = make_meta(boost::mysql::column_type::time);

	using std::chrono::hours;
	using std::chrono::microseconds;
	using std::chrono::minutes;
	using std::chrono::seconds;

	// 25:03:04.000005
	microseconds positive = hours(25) + minutes(3) + seconds(4) + microseconds(5);
	Variant v = mysql_module::field_to_variant(boost::mysql::field_view(positive), meta, config);
	REQUIRE(v.get_type() == Variant::DICTIONARY);
	Dictionary d = v;
	CHECK_FALSE(bool(d["negative"]));
	CHECK(int64_t(d["hours"]) == 25);
	CHECK(int64_t(d["minutes"]) == 3);
	CHECK(int64_t(d["seconds"]) == 4);
	CHECK(int64_t(d["microsecond"]) == 5);

	// -838:59:59.999999 is the MySQL TIME limit.
	microseconds negative = -(hours(838) + minutes(59) + seconds(59) + microseconds(999999));
	Dictionary n = mysql_module::field_to_variant(boost::mysql::field_view(negative), meta, config);
	CHECK(bool(n["negative"]));
	CHECK(int64_t(n["hours"]) == 838);
	CHECK(int64_t(n["minutes"]) == 59);
	CHECK(int64_t(n["seconds"]) == 59);
	CHECK(int64_t(n["microsecond"]) == 999999);

	Dictionary zero = mysql_module::field_to_variant(boost::mysql::field_view(microseconds(0)), meta, config);
	CHECK_FALSE(bool(zero["negative"]));
	CHECK(int64_t(zero["hours"]) == 0);
}

TEST_CASE("[Modules][MySQL] JSON columns follow json_result_mode") {
	Ref<MySQLConfig> config = make_config();
	boost::mysql::metadata json_meta = make_meta(boost::mysql::column_type::json);
	boost::mysql::metadata text_meta = make_meta(boost::mysql::column_type::text);
	boost::mysql::field_view json_text(boost::mysql::string_view("{\"a\":1}"));

	config->set_json_result_mode(MySQLConfig::RAW_STRING);
	Variant raw = mysql_module::field_to_variant(json_text, json_meta, config);
	CHECK(raw.get_type() == Variant::STRING);
	CHECK(String(raw) == "{\"a\":1}");

	// The lazy mode hands the raw text here; MySQLResult parses it on demand.
	config->set_json_result_mode(MySQLConfig::LAZY_PARSED_VARIANT);
	Variant lazy = mysql_module::field_to_variant(json_text, json_meta, config);
	CHECK(lazy.get_type() == Variant::STRING);

	config->set_json_result_mode(MySQLConfig::PARSED_VARIANT);
	Variant parsed = mysql_module::field_to_variant(json_text, json_meta, config);
	REQUIRE(parsed.get_type() == Variant::DICTIONARY);
	CHECK(int(Dictionary(parsed)["a"]) == 1);

	// MariaDB reports JSON as a text type, so it is never parsed automatically.
	Variant text_column = mysql_module::field_to_variant(json_text, text_meta, config);
	CHECK(text_column.get_type() == Variant::STRING);
}

} // namespace TestMySQLTypeConvert

/* test_mysql_params.h */
#pragma once

#include "../scr/mysql_params.h"

#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "tests/test_macros.h"

#include <boost/mysql/field_view.hpp>

// Unit tests for the pure `Variant` -> `field_view` conversion (`mysql_params.h`), the
// opposite direction of `mysql_type_convert.h`. No server is needed.
namespace TestMySQLParams {

TEST_CASE("[Modules][MySQL] A Dictionary shaped like a DATE becomes a DATE field") {
	Dictionary d;
	d["year"] = 2026;
	d["month"] = 9;
	d["day"] = 21;

	Array params;
	params.push_back(d);

	mysql_module::FieldParams out;
	String error;
	REQUIRE(mysql_module::array_to_field_params(params, out, error));
	REQUIRE(out.views.size() == 1);
	REQUIRE(out.views[0].is_date());
	boost::mysql::date value = out.views[0].get_date();
	CHECK(value.year() == 2026);
	CHECK(value.month() == 9);
	CHECK(value.day() == 21);
}

TEST_CASE("[Modules][MySQL] A Dictionary shaped like a DATETIME becomes a DATETIME field, with microseconds") {
	Dictionary d;
	d["year"] = 2026;
	d["month"] = 9;
	d["day"] = 21;
	d["hour"] = 13;
	d["minute"] = 45;
	d["second"] = 59;
	d["microsecond"] = 123456;

	Array params;
	params.push_back(d);

	mysql_module::FieldParams out;
	String error;
	REQUIRE(mysql_module::array_to_field_params(params, out, error));
	REQUIRE(out.views.size() == 1);
	REQUIRE(out.views[0].is_datetime());
	boost::mysql::datetime value = out.views[0].get_datetime();
	CHECK(value.year() == 2026);
	CHECK(value.month() == 9);
	CHECK(value.day() == 21);
	CHECK(value.hour() == 13);
	CHECK(value.minute() == 45);
	CHECK(value.second() == 59);
	CHECK(value.microsecond() == 123456);
}

TEST_CASE("[Modules][MySQL] A Dictionary shaped like a TIME becomes a TIME field, sign included") {
	Dictionary positive;
	positive["negative"] = false;
	positive["hours"] = 25;
	positive["minutes"] = 3;
	positive["seconds"] = 4;
	positive["microsecond"] = 5;

	Array params;
	params.push_back(positive);

	mysql_module::FieldParams out;
	String error;
	REQUIRE(mysql_module::array_to_field_params(params, out, error));
	REQUIRE(out.views.size() == 1);
	REQUIRE(out.views[0].is_time());
	CHECK(out.views[0].get_time().count() == 90184000005LL);

	Dictionary negative;
	negative["negative"] = true;
	negative["hours"] = 838;
	negative["minutes"] = 59;
	negative["seconds"] = 59;
	negative["microsecond"] = 999999;

	Array negative_params;
	negative_params.push_back(negative);

	mysql_module::FieldParams negative_out;
	REQUIRE(mysql_module::array_to_field_params(negative_params, negative_out, error));
	CHECK(negative_out.views[0].get_time().count() == -3020399999999LL);
}

TEST_CASE("[Modules][MySQL] A DATE and a DATETIME Dictionary are told apart by their keys") {
	// Same year/month/day as the DATETIME case, but without hour/minute/second/microsecond:
	// must be read as a DATE, not rejected or confused with a DATETIME missing fields.
	Dictionary d;
	d["year"] = 2026;
	d["month"] = 1;
	d["day"] = 1;

	Array params;
	params.push_back(d);

	mysql_module::FieldParams out;
	String error;
	REQUIRE(mysql_module::array_to_field_params(params, out, error));
	CHECK(out.views[0].is_date());
	CHECK_FALSE(out.views[0].is_datetime());
}

TEST_CASE("[Modules][MySQL] An out-of-range Dictionary component is an explicit error, not a wrapped-around value") {
	Dictionary d;
	d["year"] = 2026;
	d["month"] = 300; // Out of uint8_t range on purpose: 300 wraps to 44 if cast blindly.
	d["day"] = 21;

	Array params;
	params.push_back(d);

	mysql_module::FieldParams out;
	String error;
	CHECK_FALSE(mysql_module::array_to_field_params(params, out, error));
	CHECK(error.contains("index 0"));
}

TEST_CASE("[Modules][MySQL] A calendar-invalid DATE Dictionary is an explicit error") {
	Dictionary d;
	d["year"] = 2026;
	d["month"] = 2;
	d["day"] = 30; // February never has 30 days.

	Array params;
	params.push_back(d);

	mysql_module::FieldParams out;
	String error;
	CHECK_FALSE(mysql_module::array_to_field_params(params, out, error));
	CHECK(error.is_empty() == false);
}

TEST_CASE("[Modules][MySQL] A TIME Dictionary outside the MySQL range is an explicit error") {
	Dictionary d;
	d["negative"] = false;
	d["hours"] = 839; // One hour past MySQL's TIME limit (838:59:59.999999).
	d["minutes"] = 0;
	d["seconds"] = 0;
	d["microsecond"] = 0;

	Array params;
	params.push_back(d);

	mysql_module::FieldParams out;
	String error;
	CHECK_FALSE(mysql_module::array_to_field_params(params, out, error));
}

TEST_CASE("[Modules][MySQL] A Dictionary that matches no known shape is an explicit error") {
	Dictionary d;
	d["year"] = 2026;
	d["month"] = 9;
	// Missing "day": not a DATE, not a DATETIME, not a TIME.

	Array params;
	params.push_back(d);

	mysql_module::FieldParams out;
	String error;
	CHECK_FALSE(mysql_module::array_to_field_params(params, out, error));
	CHECK(error.contains("does not match a supported shape"));
}

TEST_CASE("[Modules][MySQL] An Array parameter is still an explicit error, not a silent NULL") {
	Array inner;
	inner.push_back(1);

	Array params;
	params.push_back(inner);

	mysql_module::FieldParams out;
	String error;
	CHECK_FALSE(mysql_module::array_to_field_params(params, out, error));
	CHECK(error.contains("Array"));
}

} //namespace TestMySQLParams

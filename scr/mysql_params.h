// SPDX-License-Identifier: MIT
/* mysql_params.h */
#pragma once

#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/variant/array.h"
#include "core/variant/variant.h"

#include <boost/mysql/field_view.hpp>

// Godot to MySQL conversion (the opposite direction of `mysql_type_convert.h`). Used by
// `MySQLSession::execute_formatted()`, `execute_prepared()` and `async_execute_prepared()`.
namespace mysql_module {

// A `field_view` does not own the string or blob it points to, so `FieldParams` keeps the
// buffers alive. `CharString` and `PackedByteArray` are reference counted: moving them
// (for example when a `LocalVector` grows) never moves the bytes a `field_view` points to.
// Never write to the buffers after `array_to_field_params()` has filled `views`.
struct FieldParams {
	LocalVector<CharString> string_storage;
	LocalVector<PackedByteArray> blob_storage;
	LocalVector<boost::mysql::field_view> views;
};

// Converts every element of `p_params` to a `field_view`. Types without a defined
// translation (`Dictionary` and `Array`, so DATE/TIME/DATETIME parameters are not
// supported yet, and any other `Variant::Type`) make the function return `false` with
// `r_error_message` set. They never become a silent NULL.
bool array_to_field_params(const Array &p_params, FieldParams &r_params, String &r_error_message);

} //namespace mysql_module

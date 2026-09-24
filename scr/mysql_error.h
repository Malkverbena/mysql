/* mysql_error.h */
#pragma once

#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>

// Builds the error `Dictionary` used across the module. Every fallible operation exposes
// `is_ok()` and `get_error()` instead of a global or per-instance error state. Keys:
// `category`, `message`, `server_message` and `is_fatal`. An empty `Dictionary` means no
// error.
//
// `message` comes from `diagnostics.client_message()` when available (it never contains
// untrusted server data). `server_message` comes only from
// `diagnostics.server_message()` and is kept separate on purpose, because Boost.MySQL
// warns that it may contain untrusted input. Nothing is logged automatically.
namespace mysql_module {

Dictionary make_error_dict(const boost::mysql::error_code &p_error, const boost::mysql::diagnostics &p_diagnostics);

// For errors that do not come from a Boost.MySQL operation (client-side guards: session
// not connected, wrong parameter count, unsupported `Variant` type, etc.). The category is
// always `mysql_module.client` and `is_fatal` is always `false`.
Dictionary make_client_error_dict(const String &p_message);

} //namespace mysql_module

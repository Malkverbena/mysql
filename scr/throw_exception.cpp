/* throw_exception.cpp */

// With `-fno-exceptions` Boost defines `BOOST_NO_EXCEPTIONS` and leaves
// `boost::throw_exception()` undefined, so the program must provide it. Without exceptions
// there is no way to recover, so reaching this point is always fatal: it logs the message
// in Godot and aborts.
//
// The module only uses the `error_code` and `diagnostics` overloads of Boost.MySQL, so this
// is a safety net for invariants violated inside Boost (Asio, MySQL, ...).

#include <boost/throw_exception.hpp>

#ifdef BOOST_NO_EXCEPTIONS

#include "core/error/error_macros.h"
#include "core/variant/variant.h"

#include <exception>

namespace boost {

void throw_exception(const std::exception &p_e) {
	CRASH_NOW_MSG(vformat("Boost.MySQL: %s", p_e.what()));
}

void throw_exception(const std::exception &p_e, const boost::source_location &p_loc) {
	CRASH_NOW_MSG(vformat("Boost.MySQL: %s (%s:%d)", p_e.what(), p_loc.file_name(), (int)p_loc.line()));
}

} // namespace boost

#endif // BOOST_NO_EXCEPTIONS

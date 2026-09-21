// SPDX-License-Identifier: MIT
/* throw_exception.cpp */

// Com -fno-exceptions o Boost define BOOST_NO_EXCEPTIONS e deixa boost::throw_exception()
// sem definição: o programa precisa fornecê-la. Sem exceções não há como recuperar, então
// qualquer chegada aqui é fatal: registra a mensagem no Godot e aborta.
//
// O módulo usa só as sobrecargas com error_code/diagnostics do Boost.MySQL, então isto é
// uma rede de segurança para invariantes violados dentro do Boost (Asio, MySQL, ...).

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

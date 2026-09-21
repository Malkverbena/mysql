// SPDX-License-Identifier: MIT
/* mysql_params.cpp */

#include "mysql_params.h"

#include "godot_convert.h"

#include <boost/mysql/blob_view.hpp>
#include <boost/mysql/string_view.hpp>

namespace mysql_module {

bool array_to_field_params(const Array &p_params, FieldParams &r_params, String &r_error_message) {
	int count = p_params.size();
	// Reserva primeiro: field_view vai apontar pro conteúdo destes vetores, e um
	// realloc no meio da população invalidaria os ponteiros já criados.
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
				r_params.string_storage.push_back(to_std_string((String)v));
				const std::string &stored = r_params.string_storage.back();
				r_params.views.push_back(boost::mysql::field_view(boost::mysql::string_view(stored.data(), stored.size())));
				break;
			}

			case Variant::PACKED_BYTE_ARRAY: {
				PackedByteArray bytes = v;
				std::vector<unsigned char> storage(bytes.ptr(), bytes.ptr() + bytes.size());
				r_params.blob_storage.push_back(std::move(storage));
				const std::vector<unsigned char> &stored = r_params.blob_storage.back();
				r_params.views.push_back(boost::mysql::field_view(boost::mysql::blob_view(stored.data(), stored.size())));
				break;
			}

			default:
				// DATE/TIME/DATETIME como parâmetro (Dictionary) fica pra depois —
				// registrado como pendência, não como conversão silenciosa errada.
				r_error_message = vformat("Tipo de parâmetro não suportado no índice %d: %s", i, Variant::get_type_name(v.get_type()));
				return false;
		}
	}
	return true;
}

} //namespace mysql_module

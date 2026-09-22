// SPDX-License-Identifier: MIT
/* mysql_params.cpp */

#include "mysql_params.h"

#include <boost/mysql/blob_view.hpp>
#include <boost/mysql/string_view.hpp>

namespace mysql_module {

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

			default:
				// NOTE: DATE/TIME/DATETIME parameters (`Dictionary`) are not supported yet. Fail
				// explicitly instead of converting to something silently wrong.
				r_error_message = vformat("Unsupported parameter type at index %d: %s.", i, Variant::get_type_name(v.get_type()));
				return false;
		}
	}
	return true;
}

} //namespace mysql_module

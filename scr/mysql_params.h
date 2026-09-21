// SPDX-License-Identifier: MIT
/* mysql_params.h */
#pragma once

#include "core/variant/array.h"
#include "core/variant/variant.h"

#include <boost/mysql/field_view.hpp>

#include <string>
#include <vector>

// Conversão Godot -> MySQL (direção oposta de mysql_type_convert.h), camada interna
// (Fase 4 do roadmap). Usada por execute_formatted()/execute_prepared() em
// MySQLSession.
namespace mysql_module {

// field_view não é dono do que aponta (string/blob) — FieldParams guarda os buffers
// (string_storage/blob_storage) que os field_views referenciam, e reserva a capacidade
// exata antes de popular, porque um realloc no meio invalidaria os ponteiros dos
// field_views já criados (mesma classe de bug do S3/S12 da auditoria, aqui na direção
// contrária). Nunca cresça string_storage/blob_storage depois de array_to_field_params()
// preencher views.
struct FieldParams {
	std::vector<std::string> string_storage;
	std::vector<std::vector<unsigned char>> blob_storage;
	std::vector<boost::mysql::field_view> views;
};

// Converte cada elemento de p_params num field_view. Tipos sem tradução definida (por
// enquanto: Dictionary/Array — DATE/TIME/DATETIME como parâmetro fica pra depois — e
// qualquer outro Variant::Type não listado aqui) fazem a função devolver false com
// r_error_message preenchida; nunca vira NULL silencioso (resolve S10 da auditoria).
bool array_to_field_params(const Array &p_params, FieldParams &r_params, String &r_error_message);

} //namespace mysql_module

// SPDX-License-Identifier: MIT
/* mysql_type_convert.h */
#pragma once

#include "core/variant/variant.h"

#include "mysql_config.h"

#include <boost/mysql/field_view.hpp>
#include <boost/mysql/metadata.hpp>

// Conversão MySQL/MariaDB -> Godot, camada interna (Fase 3 do roadmap). Funções puras:
// não fazem I/O, não guardam estado — dado um field_view (valor de uma célula) e a
// metadata da coluna, devolvem o Variant correspondente, seguindo as regras decididas em
// documentation/design-notes.md (tinyint1_mode, json_result_mode, BIGINT UNSIGNED acima
// de INT64_MAX -> String, TIME/DATE/DATETIME com microssegundos e sinal corretos).
//
// json_result_mode == LAZY_PARSED_VARIANT devolve a string bruta aqui (igual a
// RAW_STRING); a conversão sob demanda com cache é responsabilidade de MySQLResult, não
// desta função pura.
namespace mysql_module {

Variant field_to_variant(const boost::mysql::field_view &p_field, const boost::mysql::metadata &p_meta, const Ref<MySQLConfig> &p_config);

// Usada tanto pela conversão de campo (TINYINT/INT/BIGINT) quanto por
// MySQLResult::from_boost_results() (affected_rows/last_insert_id, que também são
// std::uint64_t no Boost.MySQL).
Variant uint64_to_variant(std::uint64_t p_value);

} //namespace mysql_module

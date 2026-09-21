// SPDX-License-Identifier: MIT
/* godot_convert.h */
#pragma once

#include "core/string/ustring.h"

#include <string>

// Fronteira entre tipos do Godot e std::string, usada em todo o módulo (camada interna
// que conversa com o Boost — ver documentation/design-notes.md, regra de STL só aqui).
//
// Regra fixa (Fase 1 do roadmap, resolve S3/S12 da auditoria): nunca guardar o retorno
// de String::utf8().get_data() numa variável própria — o CharString temporário é
// destruído ao fim da expressão que o criou e o ponteiro fica pendente. As funções
// abaixo sempre copiam o conteúdo para um std::string antes de devolver.
namespace mysql_module {

// Copia o conteúdo UTF-8 de uma Godot String para um std::string próprio.
std::string to_std_string(const String &p_string);

// Constrói uma Godot String a partir de bytes UTF-8 do servidor (conexão sempre em
// utf8mb4 — ver design-notes.md), com comprimento explícito. Nunca assume terminador
// nulo (resolve S11 da auditoria).
String to_godot_string(const char *p_data, size_t p_len);
String to_godot_string(const std::string &p_string);

} //namespace mysql_module

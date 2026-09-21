// SPDX-License-Identifier: MIT
/* mysql_error.h */
#pragma once

#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/error_code.hpp>

// Constrói o Dictionary de erro usado em todo o módulo — is_ok()/get_error() em vez de
// estado de erro global ou por instância (resolve C5 da auditoria). Chaves: category,
// message, server_message, is_fatal. Dictionary vazio == sem erro.
//
// message vem de diagnostics.client_message() quando disponível (nunca contém dado não
// confiável do servidor); server_message vem só de diagnostics.server_message() —
// mantido separado de propósito, porque o próprio Boost.MySQL avisa que pode conter
// entrada não confiável (resolve S13 da auditoria: nada disso é logado automaticamente).
// Ver documentation/design-notes.md.
namespace mysql_module {

Dictionary make_error_dict(const boost::mysql::error_code &p_error, const boost::mysql::diagnostics &p_diagnostics);

// Erro que não vem de uma operação do Boost.MySQL (guarda de estado do cliente: sessão
// não conectada, contagem de parâmetros errada, tipo de Variant não suportado, etc.).
// category fica sempre "mysql_module.client", is_fatal sempre false.
Dictionary make_client_error_dict(const String &p_message);

} //namespace mysql_module

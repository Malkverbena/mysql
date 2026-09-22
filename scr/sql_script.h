// SPDX-License-Identifier: MIT
/* sql_script.h */
#pragma once

#include "core/string/ustring.h"
#include "core/templates/vector.h"

namespace mysql_module {

// Splits a SQL script into statements on `;`, ignoring the ones inside quoted literals
// (single quotes, double quotes and backticks, with a doubled quote or a backslash escape
// inside the literal). Statements are trimmed and empty ones are dropped.
//
// NOTE: SQL comments (`--` and `/* */`) are not understood, so a `;` or a quote inside a
// comment is treated as code. This is a known limitation.
Vector<String> split_sql_statements(const String &p_content);

} //namespace mysql_module

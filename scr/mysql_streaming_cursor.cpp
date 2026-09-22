// SPDX-License-Identifier: MIT
/* mysql_streaming_cursor.cpp */

#include "mysql_streaming_cursor.h"

#include "godot_convert.h"
#include "mysql_connection.h"
#include "mysql_error.h"
#include "mysql_session.h"
#include "mysql_type_convert.h"

#include "core/object/class_db.h"

#include <boost/mysql/metadata_collection_view.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/rows_view.hpp>

Ref<MySQLStreamingCursor> MySQLStreamingCursor::start(Ref<MySQLSession> p_session, MySQLConnection &p_connection, const Ref<MySQLConfig> &p_config, const std::string &p_sql) {
	Ref<MySQLStreamingCursor> cursor;
	cursor.instantiate();
	cursor->owner_session = p_session;
	cursor->connection = &p_connection;
	cursor->config = p_config;

	boost::mysql::error_code err;
	boost::mysql::diagnostics diag;
	p_connection.native().start_execution(p_sql, cursor->state, err, diag);
	if (err) {
		cursor->ok = false;
		cursor->error = mysql_module::make_error_dict(err, diag);
	}
	return cursor;
}

Ref<MySQLStreamingCursor> MySQLStreamingCursor::from_error(const Dictionary &p_error) {
	Ref<MySQLStreamingCursor> cursor;
	cursor.instantiate();
	cursor->ok = false;
	cursor->closed = true;
	cursor->error = p_error;
	return cursor;
}

PackedStringArray MySQLStreamingCursor::get_column_names() const {
	PackedStringArray names;
	boost::mysql::metadata_collection_view meta = state.meta();
	names.resize((int)meta.size());
	for (std::size_t i = 0; i < meta.size(); i++) {
		boost::mysql::string_view name = meta[i].column_name();
		names.set((int)i, mysql_module::to_godot_string(name.data(), name.size()));
	}
	return names;
}

Array MySQLStreamingCursor::next_batch() {
	Array out;
	if (!ok || closed || state.complete()) {
		return out;
	}

	boost::mysql::error_code err;
	boost::mysql::diagnostics diag;
	boost::mysql::rows_view batch = connection->native().read_some_rows(state, err, diag);
	if (err) {
		ok = false;
		error = mysql_module::make_error_dict(err, diag);
		return out;
	}

	boost::mysql::metadata_collection_view meta = state.meta();
	out.resize((int)batch.size());
	for (std::size_t r = 0; r < batch.size(); r++) {
		boost::mysql::row_view row = batch[r];
		Array row_array;
		row_array.resize((int)row.size());
		for (std::size_t c = 0; c < row.size(); c++) {
			row_array[(int)c] = mysql_module::field_to_variant(row[c], meta[c], config);
		}
		out[(int)r] = row_array;
	}
	return out;
}

void MySQLStreamingCursor::close() {
	if (closed) {
		return;
	}
	closed = true;
	if (!ok || !connection) {
		return;
	}
	// Drain the rest of the resultset silently. See the comment in the header.
	boost::mysql::error_code err;
	boost::mysql::diagnostics diag;
	while (!state.complete()) {
		connection->native().read_some_rows(state, err, diag);
		if (err) {
			break;
		}
	}
}

MySQLStreamingCursor::~MySQLStreamingCursor() {
	close();
}

void MySQLStreamingCursor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_ok"), &MySQLStreamingCursor::is_ok);
	ClassDB::bind_method(D_METHOD("get_error"), &MySQLStreamingCursor::get_error);
	ClassDB::bind_method(D_METHOD("has_more"), &MySQLStreamingCursor::has_more);
	ClassDB::bind_method(D_METHOD("get_column_names"), &MySQLStreamingCursor::get_column_names);
	ClassDB::bind_method(D_METHOD("next_batch"), &MySQLStreamingCursor::next_batch);
	ClassDB::bind_method(D_METHOD("close"), &MySQLStreamingCursor::close);
}

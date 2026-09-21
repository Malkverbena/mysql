// SPDX-License-Identifier: MIT
/* mysql_async_operation.cpp */

#include "mysql_async_operation.h"

#include "core/object/class_db.h"

void MySQLAsyncOperation::_complete(Ref<MySQLResult> p_result) {
	result = p_result;
	finished_flag = true;
	emit_signal("completed", result);
}

void MySQLAsyncOperation::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_complete", "result"), &MySQLAsyncOperation::_complete);
	ClassDB::bind_method(D_METHOD("is_finished"), &MySQLAsyncOperation::is_finished);
	ClassDB::bind_method(D_METHOD("get_result"), &MySQLAsyncOperation::get_result);

	ADD_SIGNAL(MethodInfo("completed", PropertyInfo(Variant::OBJECT, "result", PROPERTY_HINT_RESOURCE_TYPE, "MySQLResult")));
}

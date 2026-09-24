/* mysql_async_operation.cpp */

#include "mysql_async_operation.h"

#include "mysql_streaming_cursor.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"

void mysql_module::complete_deferred(const Ref<MySQLAsyncOperation> &p_operation, const Ref<MySQLResult> &p_result) {
	// From here on the connection is free: nothing below touches it again.
	p_operation->clear_running();
	callable_mp_static(&MySQLAsyncOperation::_deliver).call_deferred(p_operation, p_result);
}

void MySQLAsyncOperation::_deliver(Ref<MySQLAsyncOperation> p_operation, Ref<MySQLResult> p_result) {
	p_operation->_complete(p_result);
}

void MySQLAsyncOperation::_complete(Ref<MySQLResult> p_result) {
	result = p_result;
	finished_flag = true;
	Ref<MySQLStreamingCursor> stepped = cursor;
	if (stepped.is_valid()) {
		stepped->_apply_async_step(*this);
	}
	emit_signal("completed", result);
	cursor = Ref<RefCounted>();
}

bool MySQLAsyncOperation::cancel() {
	if (!is_running() || connection == nullptr) {
		return false;
	}
	if (cancel_requested.set_if_clear()) {
		mysql_module::start_async_cancel(Ref<MySQLAsyncOperation>(this));
	}
	return true;
}

void MySQLAsyncOperation::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_finished"), &MySQLAsyncOperation::is_finished);
	ClassDB::bind_method(D_METHOD("get_result"), &MySQLAsyncOperation::get_result);
	ClassDB::bind_method(D_METHOD("cancel"), &MySQLAsyncOperation::cancel);

	ADD_SIGNAL(MethodInfo("completed", PropertyInfo(Variant::OBJECT, "result", PROPERTY_HINT_RESOURCE_TYPE, "MySQLResult")));
}

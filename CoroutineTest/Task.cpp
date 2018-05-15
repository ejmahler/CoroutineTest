#include "Task.h"

Task<void> TaskPromise<void>::get_return_object() {
	auto SharedState = std::make_shared<TaskState<void>>(this);
	M_SharedState = SharedState;
	return Task<void>{ std::experimental::coroutine_handle<TaskPromise<void>>::from_promise(*this), SharedState };
}

std::string TaskState<void>::GetDebugString() const {
	if (!M_PromisePtr) {
		return "[destroyed]";
	}
	else if (!M_PromisePtr->M_DebugStringFn) {
		return "[unset]";
	}
	else {
		return M_PromisePtr->M_DebugStringFn();
	}
}

TaskState<void>::~TaskState() {
	if (M_PromisePtr) {
		M_PromisePtr->bCanceled = true;
	}
}
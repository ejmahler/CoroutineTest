#include "Task.h"

Task<void> TaskPromise<void>::get_return_object() {
	return Task<void>{ std::experimental::coroutine_handle<TaskPromise<void>>::from_promise(*this), M_SharedState };
}

std::string TaskState<void>::GetDebugName() const {
	if (M_PromisePtr) {
		return M_PromisePtr->M_DebugName;
	}
	else {
		return "[destroyed]";
	}
}
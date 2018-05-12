#include "Task.h"

Task<void> TaskPromise<void>::get_return_object() {
	return Task<void>{ std::experimental::coroutine_handle<TaskPromise<void>>::from_promise(*this), M_SharedState };
}
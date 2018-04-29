#pragma once

#include "Task.h"

struct NextFrame {
	inline bool await_ready() const { return false; }

	template<class PromiseT>
	inline void await_suspend(std::experimental::coroutine_handle<PromiseT> Handle) { Handle.promise().InnerTask = nullptr; }

	inline void await_resume() const { }
};



template<class ReturnT>
class ExecuteTaskImpl {
public:
	ExecuteTaskImpl(Task<ReturnT> &&InTask) : InnerTask(std::move(InTask)) {}

	inline bool await_ready() { return InnerTask.Poll(); }
	template<class PromiseT>
	inline void await_suspend(std::experimental::coroutine_handle<PromiseT> Handle) { Handle.promise().InnerTask = &InnerTask; }
	inline ReturnT await_resume() const { return InnerTask.GetReturnValue(); }

private:
	Task<ReturnT> InnerTask;
};
template<class ReturnT>
auto operator co_await(Task<ReturnT> &&InTask) {
	return ExecuteTaskImpl<ReturnT> { std::move(InTask) };
}
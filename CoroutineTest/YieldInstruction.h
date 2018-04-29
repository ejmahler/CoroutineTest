#pragma once

#include "Time.h"
#include "Task.h"

struct NextFrame {
	inline bool await_ready() const { return false; }

	template<class PromiseT>
	inline void await_suspend(std::experimental::coroutine_handle<PromiseT> Handle) { Handle.promise().InnerTask = nullptr; }

	inline void await_resume() const { }
};


template<class ReturnT>
class ExecuteTask {
public:
	ExecuteTask(Task<ReturnT> &&InTask) : InnerTask(std::move(InTask)) {}

	inline bool await_ready() { return InnerTask.Poll(); }
	template<class PromiseT>
	inline void await_suspend(std::experimental::coroutine_handle<PromiseT> Handle) { Handle.promise().InnerTask = &InnerTask; }
	inline ReturnT await_resume() const { return InnerTask.GetReturnValue(); }

private:
	Task<ReturnT> InnerTask;
};
template<class ReturnT>
auto operator co_await(Task<ReturnT> &&InTask) {
	return ExecuteTask<ReturnT> { std::move(InTask) };
}

class ExecuteCoroutine {
public:
	ExecuteCoroutine(Coroutine &&InTask) : InnerTask(std::move(InTask)) {}

	inline bool await_ready() { return InnerTask.Poll(); }
	template<class PromiseT>
	inline void await_suspend(std::experimental::coroutine_handle<PromiseT> Handle) { Handle.promise().InnerTask = &InnerTask; }
	inline void await_resume() const { }

private:
	Coroutine InnerTask;
};

auto operator co_await(Coroutine &&InTask) {
	return ExecuteCoroutine{ std::move(InTask) };
}

Coroutine WaitForSeconds(float SecondsToWait) {
	float BeginTime = Time::GameTime();
	while (Time::GameTimeSince(BeginTime) < SecondsToWait) {
		co_await NextFrame{};
	}
}


template<class PredicateT>
Coroutine WaitUntil(PredicateT predicate) {
	while (!predicate()) {
		co_await NextFrame{};
	}
}

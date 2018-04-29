#pragma once

#include <optional>

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
	inline ReturnT await_resume() { return InnerTask.TakeReturnValue(); }

private:
	Task<ReturnT> InnerTask;
};

template<class ReturnT>
auto operator co_await(Task<ReturnT> &&InTask) {
	return ExecuteTask<ReturnT> { std::move(InTask) };
}


// coroutine that finishes after the specified number of seconds have passed
Coroutine __WaitForSeconds_Impl(float Seconds) {
	const float BeginTime = Time::GameTime();
	while (Time::GameTimeSince(BeginTime) < Seconds) {
		co_await NextFrame{};
	}
}
Coroutine WaitForSeconds(float Seconds) { return __WaitForSeconds_Impl(Seconds).SetName("WaitForSeconds"); }



// coroutine that finishes when the provided predicate returns true.
template<class PredicateT>
Coroutine __WaitUntil_Impl(PredicateT predicate) {
	while (!predicate()) {
		co_await NextFrame{};
	}
}
template<class PredicateT>
Coroutine WaitUntil(PredicateT predicate) { return __WaitUntil_Impl(predicate).SetName("WaitUntil"); }



// Task that executes the provided coroutine until the provided timeout elapses. If the timeout expires, cancels the coroutine. Returns true if the task finished before the timeout, false if the task was canceled.
Task<bool> __Timeout_Impl(float TimeoutSeconds, Task<void> Task) {
	const float BeginTime = Time::GameTime();
	while (Time::GameTimeSince(BeginTime) < TimeoutSeconds) {
		if (Task.Poll()) {
			co_return true;
		}
		co_await NextFrame{};
	}
	co_return false;
}
Task<bool> Timeout(float TimeoutSeconds, Task<void> Task) { return __Timeout_Impl(TimeoutSeconds, std::move(Task)).SetName("Timeout"); }



// Task that executes the provided task until the provided timeout elapses. If the timeout expires, cancels the task. Returns the task's return value if the task finished before the timeout, or nullopt if the task was canceled.
template<class ReturnT>
Task<std::optional<ReturnT>> __Timeout_Impl(float TimeoutSeconds, Task<ReturnT> Task) {
	const float BeginTime = Time::GameTime();
	while (Time::GameTimeSince(BeginTime) < TimeoutSeconds) {
		if (Task.Poll()) {
			co_return Task.GetReturnValue();
		}
		co_await NextFrame{};
	}
	co_return std::optional<ReturnT>{};
}
template<class ReturnT>
Task<std::optional<ReturnT>> Timeout(float TimeoutSeconds, Task<ReturnT> Task) { return __Timeout_Impl(TimeoutSeconds, std::move(Task)).SetName("Timeout"); }




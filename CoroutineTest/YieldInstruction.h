#pragma once

#include <optional>

#include "Task.h"


class Time {
	static float S_DT;
	static float S_GameTime;

public:
	inline static float DT() { return S_DT; }
	inline static float GameTime() { return S_GameTime; }
	inline static float GameTimeSince(float EarlierTime) { return S_GameTime - EarlierTime; }

	inline static void Update(float DT) {
		S_DT = DT;
		S_GameTime += DT;
	}
};


struct NextFrame {
	inline bool await_ready() const { return false; }

	template<class PromiseT>
	inline void await_suspend(std::experimental::coroutine_handle<PromiseT> Handle) { Handle.promise().InnerTask = nullptr; }

	inline void await_resume() const { }
};


template<class ReturnT>
class ExecuteTask {
public:
	ExecuteTask(Task<ReturnT> &&InnerTask) : M_Controller(InnerTask.TakeController()), M_State(InnerTask.TakeState()) {}

	inline bool await_ready() { return M_Controller.Poll(); }
	template<class OuterPromiseT>
	inline void await_suspend(std::experimental::coroutine_handle<OuterPromiseT> OuterHandle) { OuterHandle.promise().InnerTask = &M_Controller; }
	inline ReturnT await_resume();

private:
	TaskController M_Controller;
	std::shared_ptr<TaskState<ReturnT>> M_State;
};

template<class ReturnT>
ReturnT ExecuteTask<ReturnT>::await_resume() { return *(M_State->TakeReturnValue()); }

template<>
void ExecuteTask<void>::await_resume() { }

template<class ReturnT>
auto operator co_await(Task<ReturnT> &&InTask) {
	return ExecuteTask<ReturnT> { std::move(InTask) };
}


// coroutine that finishes after the specified number of seconds have passed
inline Coroutine WaitForSeconds(float Seconds) {
	co_await SetName("WaitForSeconds");

	const float BeginTime = Time::GameTime();
	while (Time::GameTimeSince(BeginTime) < Seconds) {
		co_await NextFrame{};
	}
}



// coroutine that finishes when the provided predicate returns true.
template<class PredicateT>
Coroutine WaitUntil(PredicateT predicate) {
	co_await SetName("WaitUntil");

	while (!predicate()) {
		co_await NextFrame{};
	}
}



// Task that executes the provided coroutine until the provided timeout elapses. If the timeout expires, cancels the coroutine. Returns true if the task finished before the timeout, false if the task was canceled.
inline Task<bool> Timeout(float TimeoutSeconds, Task<void> Task) {
	// Initial iteration, to see if we even need to run
	const float BeginTime = Time::GameTime();
	if (Task.Poll()) {
		co_return true;
	}
	if (Time::GameTimeSince(BeginTime) >= TimeoutSeconds) {
		co_return false;
	}

	// We didn't early-exit, so get ready to run long-term (note that the Task's DebugName will be unset of we don't call poll() a few lines above, otherwise this would all be in the loop)
	co_await SetName("Timeout: " + Task.GetDebugName());
	while (true) {
		co_await NextFrame{};

		if (Task.Poll()) {
			co_return true;
		}
		if (Time::GameTimeSince(BeginTime) >= TimeoutSeconds) {
			co_return false;
		}
	}
}



// Task that executes the provided task until the provided timeout elapses. If the timeout expires, cancels the task. Returns the task's return value if the task finished before the timeout, or nullopt if the task was canceled.
template<class ReturnT>
Task<std::optional<ReturnT>> Timeout(float TimeoutSeconds, Task<ReturnT> Task) {
	// Initial iteration, to see if we even need to run
	const float BeginTime = Time::GameTime();
	if (Task.Poll()) {
		co_return Task.TakeReturnValue();
	}
	if (Time::GameTimeSince(BeginTime) >= TimeoutSeconds) {
		co_return std::nullopt;
	}

	// We didn't early-exit, so get ready to run long-term (note that the Task's DebugName will be unset of we don't call poll() a few lines above, otherwise this would all be in the loop)
	co_await SetName("Timeout: " + Task.GetDebugName());
	while (true) {
		co_await NextFrame{};

		if (Task.Poll()) {
			co_return Task.TakeReturnValue();
		}
		if (Time::GameTimeSince(BeginTime) >= TimeoutSeconds) {
			co_return std::nullopt;
		}
	}
}




#pragma once
#include <experimental/coroutine>
#include <string>
#include <optional>

#include "TaskController.h"

template<class ReturnT> class Task;
template<class ReturnT> class TaskState;
template<class ReturnT> struct TaskPromise;

// Void as the ReturnT won't compile for the above classes, so we'll use template specialization to cover void
// forward declare the fact that we're specializing these classes for void
template<> class TaskState<void>;
template<> struct TaskPromise<void>;





// View of a task. Stores the state of the task in a shareable way.
template<class ReturnT>
class TaskState {
public:
	TaskState(TaskPromise<ReturnT> *InPromise) : M_PromisePtr(InPromise) {}

	bool IsFinished() const { return !M_PromisePtr; }
	std::optional<ReturnT> TakeReturnValue() { return std::move(M_ReturnValue); }

	std::string GetDebugName() {
		if (M_PromisePtr) {
			return M_PromisePtr->M_DebugName;
		}
		else {
			return "[destroyed]";
		}
	}

private:
	friend TaskPromise<ReturnT>;

	TaskPromise<ReturnT> *M_PromisePtr;
	std::optional<ReturnT> M_ReturnValue; // Contains nullopt until the task finishes!
};

// Specialized for ReturnT=<void> -- the return value is meaningless here so we just omit it entirely
template<>
class TaskState<void> {
public:
	TaskState(TaskPromise<void> *InPromise) : M_PromisePtr(InPromise) {}

	bool IsFinished() const { return !M_PromisePtr; }

	std::string GetDebugName() const;

private:
	friend TaskPromise<void>;

	TaskPromise<void> *M_PromisePtr;
};





// a promise that manages a Task. The Task must return a ReturnT when it finishes
template<class ReturnT>
struct TaskPromise : public TaskPromiseBase {
	TaskPromise(void) : M_SharedState(std::make_shared<TaskState<ReturnT>>(this)) {}
	~TaskPromise() {
		M_SharedState->M_PromisePtr = nullptr;
	}

	Task<ReturnT> get_return_object() {
		return Task<ReturnT>{ std::experimental::coroutine_handle<TaskPromise<ReturnT>>::from_promise(*this), M_SharedState };
	}

	void return_value(ReturnT &&Value) {
		M_SharedState->M_ReturnValue = std::move(Value);
	}

	// Pointer to state that is shared between the promise, the task, and task views
	std::shared_ptr<TaskState<ReturnT>> M_SharedState;
};

// Specialized for ReturnT=<void> -- no return_value() because there's nothing to return
template<>
struct TaskPromise<void> : public TaskPromiseBase {
	TaskPromise(void) : M_SharedState(std::make_shared<TaskState<void>>(this)) {}
	~TaskPromise() {
		M_SharedState->M_PromisePtr = nullptr;
	}

	Task<void> get_return_object();

	// When this is not null, the coroutine has returned, and the contents of this pointer are the return value
	std::shared_ptr<TaskState<void>> M_SharedState;
};



// Bookkeeping and front-facing API for an asynchronous task
template<class ReturnT>
class Task {
public:
	using promise_type = TaskPromise<ReturnT>;

	Task(std::experimental::coroutine_handle<TaskPromise<ReturnT>> InHandle, std::shared_ptr<TaskState<ReturnT>> InView)
		: M_Controller(InHandle),
		M_State(std::move(InView))
	{}

	bool Poll() { return M_Controller.Poll(); }
	bool IsFinished() const { return M_Controller.IsFinished(); }

	ReturnT TakeReturnValue();

	TaskController TakeController() { return std::move(M_Controller); }
	std::shared_ptr<TaskState<ReturnT>> TakeState() { return std::move(M_State); }

	std::string GetDebugName() const { return M_Controller.GetDebugName(); }
	std::string GetFullDebugString() const { return M_Controller.GetFullDebugString(); }

private:
	TaskController M_Controller;
	std::shared_ptr<TaskState<ReturnT>> M_State;
};

template<class ReturnT>
ReturnT Task<ReturnT>::TakeReturnValue() { return *(M_State->TakeReturnValue()); }

template<>
inline void Task<void>::TakeReturnValue() { }

using Coroutine = Task<void>;
using CoroutineState = TaskState<void>;



// Awaiter that initializes a coroutine with some info that's difficult to get at the call-site or from the parameters
// Immediately resumes, without returning to the caller.
struct SetName {
	std::string M_Name;

	inline SetName(std::string InName) : M_Name(InName) {}

	inline bool await_ready() const { return false; }

	template<class PromiseT>
	inline bool await_suspend(std::experimental::coroutine_handle<PromiseT> Handle) {
		Handle.promise().M_DebugName = std::move(M_Name);
		return false;
	}

	inline void await_resume() const { }
};
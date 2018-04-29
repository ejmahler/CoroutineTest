#pragma once
#include <experimental/coroutine>
#include <string>

class TaskBase;
template<class ReturnT> class Task;
template<class ReturnT> struct TaskPromise;


// View of a task. Stores the state of the task in a shareable way.
template<class ReturnT>
class TaskView {
public:
	bool IsFinished() const { return bool(M_ReturnValue); }
	const ReturnT& GetReturnValue() { return *M_ReturnValue; }
	const std::string& GetDebugName() { return M_DebugName; }

private:
	friend Task<ReturnT>;
	friend TaskPromise<ReturnT>;

	std::string M_DebugName;
	std::unique_ptr<ReturnT> M_ReturnValue = nullptr;
};


// Base class for a task promise. The child classes are template types, but the base class is non-templated,
// so it's easier to get a pointer to any task promise regardless of its return type
struct TaskPromiseBase {
	inline auto initial_suspend() { return true; }
	inline auto final_suspend() { return true; }

	// Optional non-owning pointer to an inner task. If set, calls to this task's poll() method will be forward to InnerTask's poll() until it finishes
	TaskBase *InnerTask = nullptr;
};


// a promise that manages a Task. The Task must return a ReturnT when it finishes
template<class ReturnT>
struct TaskPromise : public TaskPromiseBase {
	TaskPromise(void) : M_SharedState(std::make_shared<TaskView<ReturnT>>()) {}

	Task<ReturnT> get_return_object() {
		return Task<ReturnT>{ std::experimental::coroutine_handle<TaskPromise<ReturnT>>::from_promise(*this), M_SharedState };
	}

	void return_value(ReturnT Value) {
		M_SharedState->M_ReturnValue = std::make_unique<ReturnT>(std::move(Value));
	}

	// When this is not null, the coroutine has returned, and the contents of this pointer are the return value
	std::shared_ptr<TaskView<ReturnT>> M_SharedState;
};


// Base class for all tasks.
class TaskBase
{
public:
	inline TaskBase(std::experimental::coroutine_handle<> InHandle, TaskPromiseBase *InPromisePtr) : M_Handle(InHandle), M_PromisePtr(InPromisePtr) {}
	~TaskBase();

	// Copy is not allowed, because we'd have to copy the inner coroutine state, which isn't feasible
	TaskBase(const TaskBase &other) = delete;
	TaskBase& operator=(const TaskBase &other) = delete;

	// Move is allowed
	TaskBase(TaskBase &&Other);
	TaskBase& operator=(TaskBase &&Other);

	// Resume the task, and returns true if it's finished, false otherwise. It is safe to call this on a task that has already finished.
	bool Poll();
private:
	// I would love to store just M_Handle here, but it appears like polymorphism of promises isn't allowed, and we specifically only want a pointer to the base class.
	// So we're going full hackjob and storing a blank handle plus a separate pointer to the promise.
	std::experimental::coroutine_handle<> M_Handle;
	TaskPromiseBase *M_PromisePtr;
};


// Bookkeeping and front-facing API for an asynchronous task
template<class ReturnT>
class Task : public TaskBase {	
public:
	Task(std::experimental::coroutine_handle<TaskPromise<ReturnT>> InHandle, std::shared_ptr<TaskView<ReturnT>> InView)
		: TaskBase(InHandle, &(InHandle.promise())),
		M_View(std::move(InView))
	{}

	bool IsFinished() const { return M_View->IsFinished(); }
	const ReturnT& GetReturnValue() const { return M_View->GetReturnValue(); }

private:
	std::shared_ptr<TaskView<ReturnT>> M_View;
};

template<class ReturnT, class... Params>
struct std::experimental::coroutine_traits<Task<ReturnT>, Params...> {
	using promise_type = TaskPromise<ReturnT>;
};
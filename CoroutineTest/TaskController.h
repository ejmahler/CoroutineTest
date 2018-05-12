#pragma once
#include <experimental/coroutine>

class TaskController;

// Abstract base class for a task promise
struct TaskPromiseBase {
	inline auto initial_suspend() { return true; }
	inline auto final_suspend() { return true; }

	//if this is true, resume() should never be called again
	bool bCanceled = false;

	// Optional non-owning pointer to an inner task. If set, calls to this task's poll() method will be forward to InnerTask's poll() until it finishes
	TaskController *InnerTask = nullptr;
};

class TaskController
{
public:
	TaskController() = default;
	~TaskController();

	template<class PromiseT>
	TaskController(std::experimental::coroutine_handle<PromiseT> InHandle) : M_Handle(InHandle), M_PromisePtr(&(InHandle.promise())) {}
	

	// Copy is not allowed, because we'd have to copy the inner coroutine state, which isn't feasible
	TaskController(const TaskController &other) = delete;
	TaskController& operator=(const TaskController &other) = delete;

	// Move is allowed
	TaskController(TaskController &&Other);
	TaskController& operator=(TaskController &&Other);

	// Resume the task, and returns true if it's finished, false otherwise. It is safe to call this on a task that has already finished.
	bool Poll();

	// Returns true if the task has stopped.
	bool IsFinished() const;

	// Destroy this controller's coroutine and null everything out.
	void Cleanup();
private:
	// I would love to store just M_Handle here, but it appears like polymorphism of promises isn't allowed, and we specifically only want a pointer to the base class.
	// So we're going full hackjob and storing a blank handle plus a separate pointer to the promise.
	std::experimental::coroutine_handle<> M_Handle;
	TaskPromiseBase *M_PromisePtr;
};


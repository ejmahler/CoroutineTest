#include "TaskController.h"



TaskController::~TaskController() {
	Cleanup();
}

TaskController::TaskController(TaskController &&Other) : M_Handle(Other.M_Handle), M_PromisePtr(Other.M_PromisePtr) {
	Other.M_Handle = nullptr;
	Other.M_PromisePtr = nullptr;
}
TaskController& TaskController::operator=(TaskController &&Other) {
	if (this != &Other) {
		if (M_Handle) {
			M_Handle.destroy();
		}
		M_Handle = Other.M_Handle;
		M_PromisePtr = Other.M_PromisePtr;

		Other.M_Handle = nullptr;
		Other.M_PromisePtr = nullptr;
	}
	return *this;
}

bool TaskController::Poll() {
	// If we're already finished, early-return
	if (!M_Handle) {
		return true;
	}

	// If we've been cancelled, clean up and early-return
	if (M_PromisePtr->bCanceled) {
		Cleanup();
		return true;
	}

	// If this task has an inner task, poll that task to see if it's done
	if (M_PromisePtr->InnerTask) {

		// We have an inner task. Poll it, and if it's finished, clear it
		if (M_PromisePtr->InnerTask->Poll()) {
			M_PromisePtr->InnerTask = nullptr;
		}
	}

	// If there currently isn't an inner task, we're good to resume our own task
	if (!M_PromisePtr->InnerTask) {
		M_Handle.resume();

		// If our own task returned on this resume, destroy the taks state and return true to indicate that we are done
		if (M_Handle.done()) {
			Cleanup();

			return true;
		}
	}

	// If we get to theis point the task isn't finished yet
	return false;
}

bool TaskController::IsFinished() const {
	return !M_Handle;
}

void TaskController::Cleanup() {
	if (M_Handle) {
		M_Handle.destroy();
		M_Handle = nullptr;
		M_PromisePtr = nullptr;
	}
}
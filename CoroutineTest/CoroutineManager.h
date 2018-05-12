#pragma once

#include <memory>
#include <vector>
#include <deque>

#include "Task.h"

class CoroutineManager {
	std::vector<TaskController> ActiveCoroutines;
	std::deque<TaskController> NewCoroutineQueue;

public:
	bool HasCoroutines() const {
		return ActiveCoroutines.size() > 0 || NewCoroutineQueue.size() > 0;
	}

	void Tick() {
		// Poll all running coroutines
		for (TaskController& ActiveCoroutine : ActiveCoroutines) {
			ActiveCoroutine.Poll();
		}

		// Prune finished coroutines
		ActiveCoroutines.erase(
			std::remove_if(
				ActiveCoroutines.begin(), ActiveCoroutines.end(),
				[](const TaskController& Coro) { return Coro.IsFinished(); }),
			ActiveCoroutines.end()
		);

		// Process new coroutines
		while (!NewCoroutineQueue.empty()) {
			TaskController NewCoroutine = std::move(NewCoroutineQueue.front());
			NewCoroutineQueue.pop_front();

			if (!NewCoroutine.Poll()) {
				ActiveCoroutines.emplace_back(std::move(NewCoroutine));
			}
		}
	}

	template<class ReturnT>
	std::shared_ptr<TaskState<ReturnT>> QueueCoroutine(Task<ReturnT> &&NewTask) {
		NewCoroutineQueue.emplace_back(NewTask.TakeController());
		return NewTask.TakeState();
	}
};

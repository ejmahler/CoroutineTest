#pragma once

#include <memory>
#include <vector>
#include <deque>

#include "Task.h"

class CoroutineManager {
	std::vector<Coroutine> ActiveCoroutines;
	std::deque<Coroutine> NewCoroutineQueue;

public:
	bool HasCoroutines() const {
		return ActiveCoroutines.size() > 0 || NewCoroutineQueue.size() > 0;
	}

	void Tick() {
		// Poll all running coroutines
		for (Coroutine& ActiveCoroutine : ActiveCoroutines) {
			ActiveCoroutine.Poll();
		}

		// Prune finished coroutines
		ActiveCoroutines.erase(
			std::remove_if(
				ActiveCoroutines.begin(), ActiveCoroutines.end(),
				[](const auto& Coro) { return Coro.IsFinished(); }),
			ActiveCoroutines.end()
		);

		// Process new coroutines
		while (!NewCoroutineQueue.empty()) {
			Coroutine NewCoroutine = std::move(NewCoroutineQueue.front());
			NewCoroutineQueue.pop_front();

			if (!NewCoroutine.Poll()) {
				ActiveCoroutines.emplace_back(std::move(NewCoroutine));
			}
		}
	}

	void QueueCoroutine(Coroutine &&NewCoroutine) {
		NewCoroutineQueue.emplace_back(std::move(NewCoroutine));
	}
};

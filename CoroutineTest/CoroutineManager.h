#pragma once
#include "Coroutine.h"

#include <memory>
#include <vector>
#include <deque>
#include <algorithm>

class CoroutineManager {
	std::vector<std::shared_ptr<CoroutineContainerBase>> ActiveCoroutines;
	std::deque<std::shared_ptr<CoroutineContainerBase>> NewCoroutineQueue;

public:
	bool HasCoroutines() const {
		return ActiveCoroutines.size() > 0 || NewCoroutineQueue.size() > 0;
	}

	void Tick(float DT) {
		for (std::shared_ptr<CoroutineContainerBase>& Coroutine : ActiveCoroutines) {
			Coroutine->Internal_Tick(this, DT);
			if (Coroutine->CoFinished()) {
				Coroutine->Internal_Exit(this, DT);
			}
		}

		// Process new coroutines
		while (!NewCoroutineQueue.empty()) {
			std::shared_ptr<CoroutineContainerBase> NewCoroutine = NewCoroutineQueue.front();
			NewCoroutineQueue.pop_front();

			NewCoroutine->Internal_Enter(this, DT);
			NewCoroutine->Internal_Tick(this, DT);
			if (NewCoroutine->CoFinished()) {
				NewCoroutine->Internal_Exit(this, DT);
			}
			else {
				ActiveCoroutines.push_back(NewCoroutine);
			}
		}

		std::vector<std::shared_ptr<CoroutineContainerBase>> PrunedActiveCoroutines;
		for (const auto& Coroutine : ActiveCoroutines) {
			if(!Coroutine->CoFinished()) {
				PrunedActiveCoroutines.push_back(Coroutine);
			}
		}
		ActiveCoroutines = std::move(PrunedActiveCoroutines);
	}

	void QueueCoroutine(std::shared_ptr<CoroutineContainerBase> NewCoroutine) {
		NewCoroutineQueue.push_back(NewCoroutine);
	}
};
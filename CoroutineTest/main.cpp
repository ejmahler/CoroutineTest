#include <iostream>
#include <thread>
#include <chrono>
#include <ratio>
#include <algorithm>
#include <string>
#include <experimental/generator>

#include "Time.h"
#include "Task.h"
#include "CoroutineManager.h"
#include "YieldInstruction.h"


Coroutine InnerTask0() {
	co_await SetName("InnerTask0");

	co_await WaitForSeconds(2.0f);
}

Task<int> InnerTask1() {
	co_await SetName("InnerTask1");

	co_return 1;
}


Task<int> InnerTask2() {
	co_await SetName("InnerTask2");

	co_await InnerTask0();
	co_return 2;
}

Task<int> LongTask() {
	co_await SetName("LongTask");

	co_await WaitForSeconds(2.0f);
	co_return 6;
}

Task<float> DoThing(int x) {
	co_await SetName("DoThing");

	float sum = 0;
	for (int i = 0; i < x; i++) {
		sum += i;
		co_await NextFrame{};
	}

	bool shortTimeoutFinished = co_await Timeout(1.0f, InnerTask0());
	bool longTimeoutFinished = co_await Timeout(3.0f, InnerTask0());

	std::optional<int> LongTaskResult = co_await Timeout(3.0f, LongTask());

	co_return sum + co_await InnerTask1() + co_await InnerTask2() + LongTaskResult.value_or(0);
}

struct RecursiveTask {
	RecursiveTask() = default;
	RecursiveTask(Task<RecursiveTask> &&InTask)
		: NextTask(std::make_unique<Task<RecursiveTask>>(std::move(InTask)))
	{ }

	std::unique_ptr<Task<RecursiveTask>> NextTask;
};

Task<RecursiveTask> DoRecursiveTask() {
	co_await SetName("DoRecursiveTask");

	co_await DoThing(20);
	co_return RecursiveTask{ DoRecursiveTask() };
}

std::experimental::generator<float> IntervalTimer(int interval_ms) {
	auto prev_time = std::chrono::steady_clock::now();

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
		auto current_time = std::chrono::steady_clock::now();

		float DT = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - prev_time).count();
		DT = std::min(DT, 0.4f);

		Time::Update(DT);
		prev_time = current_time;

		co_yield DT;
	}
}


int main() {
	CoroutineManager Manager;

	// Running coroutines inside a manager
	{
		auto State = Manager.QueueCoroutine(InnerTask0());

		for (float DT : IntervalTimer(200)) {
			Manager.Tick();

			if (!Manager.HasCoroutines()) {
				break;
			}
		}

		// State.IsFinished() returns true
	}

	// If we let the return value of QueueCoroutine drop, the task is canceled
	{
		Manager.QueueCoroutine(InnerTask0());

		for (float DT : IntervalTimer(200)) {
			Manager.Tick();

			if (!Manager.HasCoroutines()) {
				break;
			}
		}

		// State.IsFinished() returns true
	}

	// Tasks don't require a manager to run
	auto ActiveTask = std::make_unique<Task<RecursiveTask>>(DoRecursiveTask());
	for (float DT: IntervalTimer(200)) {
		if (ActiveTask->Poll()) {
			ActiveTask = std::move(ActiveTask->TakeReturnValue().NextTask);
			ActiveTask->Poll(); // By convention, we want every task to be polled on the frame that it starts, to let it do setup etc
		}

		std::cout << ActiveTask->GetDebugString() << std::endl;
	}
}
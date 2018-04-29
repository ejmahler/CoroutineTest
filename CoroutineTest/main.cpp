#include <iostream>
#include <thread>
#include <chrono>
#include <ratio>
#include <algorithm>
#include <string>

#include "Time.h"
#include "Task.h"
#include "CoroutineManager.h"
#include "YieldInstruction.h"


Coroutine InnerTask0() {
	co_await WaitForSeconds(2.0f);
}

Task<int> InnerTask1() {
	co_return 1;
}


Task<int> InnerTask2() {
	co_await NextFrame{};
	co_return 2;
}

Task<float> DoThing(int x) {
	float sum = 0;
	for (int i = 0; i < x; i++) {
		sum += i;
		co_await NextFrame{};
	}

	bool shortTimeoutFinished = co_await Timeout(1.0f, InnerTask0());
	bool longTimeoutFinished = co_await Timeout(3.0f, InnerTask0());

	co_return sum + co_await InnerTask1() + co_await InnerTask2();
}

struct RecursiveTask {
	RecursiveTask() = default;
	RecursiveTask(Task<RecursiveTask> &&InTask)
		: NextTask(std::make_unique<Task<RecursiveTask>>(std::move(InTask)))
	{ }

	std::unique_ptr<Task<RecursiveTask>> NextTask;
};

Task<RecursiveTask> DoRecursiveTask() {
	co_await DoThing(20);
	co_return RecursiveTask{ DoRecursiveTask() };
}


int main() {
	auto sleep_time = std::chrono::milliseconds(200);
	auto prev_time = std::chrono::steady_clock::now();

	auto ActiveTask = std::make_unique<Task<RecursiveTask>>(DoRecursiveTask());

	// Run the manager until the task finishes
	while (true) {
		if (ActiveTask->Poll()) {
			ActiveTask = ActiveTask->TakeReturnValue().NextTask;
			ActiveTask->Poll();
		}

		std::this_thread::sleep_for(sleep_time);
		auto current_time = std::chrono::steady_clock::now();

		float DT = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - prev_time).count();
		DT = std::min(DT, 0.4f);

		Time::Update(DT);

		prev_time = current_time;
	}
}
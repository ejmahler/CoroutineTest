#include <iostream>
#include <thread>
#include <chrono>
#include <ratio>
#include <algorithm>
#include <string>

#include "Task.h"
#include "CoroutineManager.h"
#include "YieldInstruction.h"

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

	co_return sum + co_await InnerTask1() + co_await InnerTask2();
}


int main() {
	auto sleep_time = std::chrono::milliseconds(200);
	auto prev_time = std::chrono::steady_clock::now();

	Task<float> TaskInstance = DoThing(20);

	// Run the manager until the task finishes
	while (!TaskInstance.Poll()) {
		std::this_thread::sleep_for(sleep_time);
		auto current_time = std::chrono::steady_clock::now();

		float DT = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - prev_time).count();
		DT = std::min(DT, 0.4f);

		prev_time = current_time;
	}

	int abc = 5;
}
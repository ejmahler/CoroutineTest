#include <iostream>
#include <thread>
#include <chrono>
#include <ratio>
#include <algorithm>
#include <string>

#include "CoroutineContainer.h"
#include "CoroutineManager.h"
#include "YieldInstruction.h"

class GameCharacter
{
public:
	int Position = 0;

};


// inner coroutine. this will pause for a constructor-specified number of seconds, then write an output to a public variable
class InnerCoroutine : public CoroutineContainer<GameCharacter> {
	float TimeToWait = 10.0f;

	CoroutineState CoExec(Context Ctx) override {
		Ctx.Owner->Position = 5;

		co_await WaitForSeconds(TimeToWait);

		Ctx.Owner->Position = 7;

		response_to_outer = "this is the response that the outer coroutine wants, it is only constructed inside the inner coroutine, this could be something like if we succeeded or failed some task, etc";
		co_return; // optional
	}

public:
	std::string response_to_outer;

	InnerCoroutine(GameCharacter *InCharacter, float InTimeToWait) : CoroutineContainer<GameCharacter>(InCharacter), TimeToWait(InTimeToWait) {}
};




// Glue code, used to allow OuterCoroutine to wait on InnerCoroutine. This class isn't normally needed, because there is a default implementation (WaitForCoroutine in YieldInstruction.h). But the default implementation doesn't have a return vlaue, so you have to specialize the default implementation if you want a return vlaue
template<>
class WaitForCoroutine<InnerCoroutine> {
	std::shared_ptr<InnerCoroutine> coroutine_ptr;
public:
	WaitForCoroutine(std::shared_ptr<InnerCoroutine> in_ptr) : coroutine_ptr(std::move(in_ptr)) { }

	bool await_ready() const { return false; }
	void await_suspend(std::experimental::coroutine_handle<CoPromise> awaitingCoroutine) {
		awaitingCoroutine.promise().CurrentYield = std::make_shared<Yield_WaitForCoroutine>(coroutine_ptr);
	}
	std::string await_resume() const {
		return coroutine_ptr->response_to_outer;
	}
};


// Outer coroutine. This will co_await the inner coroutine
class OuterCoroutine : public CoroutineContainer<GameCharacter> {
	CoroutineState CoExec(Context Ctx) override {
		for (int i = 0; i < 10; i++) {
			if (Ctx.Owner->Position > 50) {
				// We'll never hit this based on the code in OuterCoroutine and Innercoroutine, but we might hit it if something external to the coroutine ecosystem modifies it!
				co_return;
			}

			Ctx.Owner->Position = i;

			// Every third iteration, pause for a frame
			if (i % 3 == 0) {
				co_await NextFrame{};
			}
		}
		
		// Wait for the inner coroutine, setting the "time to wait" parameter in its constructor to 3 seconds
		co_await Ctx.StartCoroutine<InnerCoroutine>(3.0f);

		// Run another InnerCoroutine, but this time, we want to know what its return value is
		std::string response = co_await Ctx.StartCoroutine<InnerCoroutine>(2.0f);

		co_await WaitForSeconds(2.0f);

		co_return; //optional
	}

public:
	OuterCoroutine(GameCharacter *InEnemy) : CoroutineContainer<GameCharacter>(InEnemy) {}
};





int main() {
	auto sleep_time = std::chrono::milliseconds(200);
	auto prev_time = std::chrono::steady_clock::now();

	GameCharacter exampleCharacter{};

	// construct the manager that will run the coroutines
	CoroutineManager manager;

	// Start an OuterCoroutine
	std::shared_ptr<OuterCoroutine> coroutine_instance = std::make_shared<OuterCoroutine>( &exampleCharacter);
	manager.QueueCoroutine(coroutine_instance);

	// Run the manager until it runs out of coroutines
	while (manager.HasCoroutines()) {
		std::this_thread::sleep_for(sleep_time);
		auto current_time = std::chrono::steady_clock::now();

		float DT = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - prev_time).count();
		DT = std::min(DT, 0.4f);

		manager.Tick(DT);

		prev_time = current_time;
	}

	int abc = 5;
}
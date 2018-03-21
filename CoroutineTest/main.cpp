#include <iostream>
#include <thread>
#include <chrono>
#include <ratio>

#include "Coroutine.h"
#include "CoroutineManager.h"

class Enemy
{
public:
	int Position = 0;

};



class InnerCoroutine : public CoroutineContainer<Enemy> {
	float TimeToWait = 10.0f;

	CoroutineState CoExec(Context Ctx) override {
		Ctx.Owner->Position = 5;

		co_yield WaitForSeconds(TimeToWait);

		co_yield{};

		Ctx.Owner->Position = 6;

		co_yield WaitForSeconds(TimeToWait);

		Ctx.Owner->Position = 7;

		co_return; // optional
	}

public:
	InnerCoroutine(Enemy *InEnemy, float InTimeToWait) : CoroutineContainer<Enemy>(InEnemy), TimeToWait(InTimeToWait) {}
};

class OuterCoroutine : public CoroutineContainer<Enemy> {
	CoroutineState CoExec(Context Ctx) override {
		for (int i = 0; i < 10; i++) {
			if (Ctx.Owner->Position > 50) {
				co_return;
			}

			Ctx.Owner->Position = i;
			co_yield{};
		}

		auto inner_ptr = std::make_shared<InnerCoroutine>(Ctx.Owner, 5.0f);
		Ctx.Manager->QueueCoroutine(inner_ptr);

		co_yield WaitForCoroutine(inner_ptr);

		co_yield WaitForSeconds(2.0f);

		co_return; //optional
	}

public:
	OuterCoroutine(Enemy *InEnemy) : CoroutineContainer<Enemy>(InEnemy) {}
};

int main() {
	auto sleep_time = std::chrono::milliseconds(200);
	auto prev_time = std::chrono::steady_clock::now();

	//CoroutineContext<Enemy> c;
	Enemy exampleEnemy{};

	CoroutineManager manager;
	std::shared_ptr<OuterCoroutine> coroutine_instance = std::make_shared<OuterCoroutine>( &exampleEnemy );
	manager.QueueCoroutine(coroutine_instance);

	while (manager.HasCoroutines()) {
		std::this_thread::sleep_for(sleep_time);
		auto current_time = std::chrono::steady_clock::now();

		float DT = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - prev_time).count();
		DT = std::min(DT, 0.4f);

		manager.Tick(DT);

		prev_time = current_time;
	}
}
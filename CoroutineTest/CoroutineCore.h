#pragma once
#include <experimental/coroutine>

class CoroutineState;
class CoroutineContainerBase;
class YieldInstruction;

class CoPromise {
public:
	CoroutineState get_return_object();

	inline auto initial_suspend() {
		return std::experimental::suspend_always{};
	}
	inline auto final_suspend() {
		return std::experimental::suspend_always{};
	}

	std::shared_ptr<YieldInstruction> CurrentYield;
};


class CoroutineState
{
public:
	using handle_type = std::experimental::coroutine_handle<CoPromise>;
	using promise_type = CoPromise;

	CoroutineState() = default;
	CoroutineState(handle_type InHandle) : CoroutineHandle(InHandle) { }

	// copy isn't allowed, because we'd have to do a deep copy (not just a shallow copy) of the coroutine handle, which isn't feasible
	CoroutineState(const CoroutineState &InState) = delete;
	CoroutineState& operator=(const CoroutineState &InState) = delete;

	// CoroutineStates may be moved
	CoroutineState(CoroutineState &&InState);
	CoroutineState& operator=(CoroutineState &&InState);

	~CoroutineState();

	void Tick(float DT);

	bool CoFinished() const;
private:
	handle_type CoroutineHandle = nullptr;
};

inline CoroutineState CoPromise::get_return_object() {
	return CoroutineState{ CoroutineState::handle_type::from_promise(*this) };
}

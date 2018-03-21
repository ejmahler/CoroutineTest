#pragma once
#include <iostream>
#include <vector>
#include <functional>
#include <optional>
#include <experimental/coroutine>
#include <experimental/generator>

class CoroutineState;
class CoroutineContainerBase;

class YieldInstruction {
public:
	virtual bool ShouldResume(float DT) = 0;
};

class Yield_WaitForSeconds : public YieldInstruction {
	float RemainingTime = 0.0f;

	bool ShouldResume(float DT) override {
		RemainingTime -= DT;
		return RemainingTime <= 0.0f;
	}

public:
	Yield_WaitForSeconds(float InSeconds) : RemainingTime(InSeconds) {}
};

class Yield_WaitForCoroutine : public YieldInstruction {
public:
	std::weak_ptr<CoroutineContainerBase> coroutine_ptr;

	bool ShouldResume(float DT) override;
	Yield_WaitForCoroutine(std::shared_ptr<CoroutineContainerBase> in_ptr) : coroutine_ptr(in_ptr) {}
};



class CoPromise {
public:
	CoroutineState get_return_object();

	auto initial_suspend() {
		return std::experimental::suspend_always{};
	}
	auto final_suspend() {
		return std::experimental::suspend_always{};
	}

	void yield_value(YieldInstruction *Insruction) {
		CurrentYield = Insruction;
	}

	YieldInstruction *CurrentYield;
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

	// coroutines may only be moved
	CoroutineState(CoroutineState &&InState) : CoroutineHandle(InState.CoroutineHandle) { InState.CoroutineHandle = nullptr; }
	CoroutineState& operator=(CoroutineState &&InState) { CoroutineHandle = InState.CoroutineHandle; InState.CoroutineHandle = nullptr; return *this; }

	~CoroutineState() { 
		if (CoroutineHandle) {
			CoroutineHandle.destroy();
			CoroutineHandle = nullptr;
		}
	}

	void Tick(float DT) {
		if (CoroutineHandle && !CoroutineHandle.done()) {
			YieldInstruction *CurrentYield = CoroutineHandle.promise().CurrentYield;
			if (!CurrentYield || CurrentYield->ShouldResume(DT)) {
				CoroutineHandle.resume();
			}
		}
	}

	bool CoFinished() const { return !CoroutineHandle || CoroutineHandle.done(); }
private:
	handle_type CoroutineHandle = nullptr;
};

CoroutineState CoPromise::get_return_object() {
	return CoroutineState{ CoroutineState::handle_type::from_promise(*this) };
}

class CoroutineManager;

template<class OwnerT>
struct CoroutineContext {
	OwnerT *Owner;
	CoroutineManager *Manager;
	const float &DT;
};


class CoroutineContainerBase {
private:
	CoroutineState InternalCoroutineState;
	float MostRecentDT = 0.0f;
	bool finished = false;

	// We need a place for this to live, that isn't the stack of the coroutine. Ideally this isn't here. Don't reference it! You can get the current yield instruction through the InternalCoroutineState
	std::unique_ptr<YieldInstruction> __CurrentYield = nullptr;

	// coroutine containers may not be moved OR copied
	// they can't be copied because we would have to make deep copies of the underlying coroutine state, which isn't feasible
	// they can't be moved because we give out a pointer to our MostRecentDT variable, which would become invalid if we get moved
	// so wherever a coroutine container gets constructed, it has to stay there for life
	CoroutineContainerBase(const CoroutineContainerBase&) = delete;
	CoroutineContainerBase& operator=(const CoroutineContainerBase&) = delete;
	CoroutineContainerBase(CoroutineContainerBase&&) = delete;
	CoroutineContainerBase& operator=(CoroutineContainerBase&&) = delete;

protected:
	virtual CoroutineState Begin(CoroutineManager *Manager, const float& DT_Ptr) = 0;

	YieldInstruction* WaitForSeconds(float Seconds) {
		__CurrentYield = std::make_unique<Yield_WaitForSeconds>(Seconds);
		return __CurrentYield.get();
	}
	YieldInstruction* WaitForCoroutine(const std::shared_ptr<CoroutineContainerBase> &InPtr) {
		__CurrentYield = std::make_unique<Yield_WaitForCoroutine>(InPtr);
		return __CurrentYield.get();
	}
public:
	CoroutineContainerBase() = default;

	void Internal_Enter(CoroutineManager *Manager, float DT) {
		MostRecentDT = DT;
		InternalCoroutineState = Begin(Manager, MostRecentDT);
	}
	void Internal_Tick(CoroutineManager *Manager, float DT) {
		MostRecentDT = DT;
		InternalCoroutineState.Tick(DT);
	}
	void Internal_Exit(CoroutineManager *Manager, float DT) {
		MostRecentDT = DT;
		InternalCoroutineState = CoroutineState{};
	}

	bool CoFinished() const { return InternalCoroutineState.CoFinished(); }
};

template<class OwnerT>
class CoroutineContainer: public CoroutineContainerBase {
public:
	CoroutineContainer(OwnerT *InOwner) : M_Owner(InOwner) {}

protected:
	using Context = CoroutineContext<OwnerT>;
	virtual CoroutineState CoExec(Context Ctx) = 0;

private:
	OwnerT *M_Owner;

	CoroutineState Begin(CoroutineManager *Manager, const float& DT_Ptr) override final {
		return CoExec(Context{ M_Owner, Manager, DT_Ptr });
	}
};


bool Yield_WaitForCoroutine::ShouldResume(float DT) {
	auto strong_ptr = coroutine_ptr.lock();
	return !strong_ptr || strong_ptr->CoFinished();
}
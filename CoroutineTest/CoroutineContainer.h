#pragma once
#include <memory>

#include "CoroutineCore.h"

class CoroutineManager;

class CoroutineContainerBase {
private:
	CoroutineState InternalCoroutineState;
	float MostRecentDT = 0.0f;
	bool finished = false;

	// coroutine containers may not be moved OR copied
	// they can't be copied because we would have to make deep copies of the underlying coroutine state, which isn't feasible
	// they can't be moved because we give out a pointer to our MostRecentDT variable, which would become invalid if we get moved
	// so wherever a coroutine container gets constructed, it has to stay there for life
	CoroutineContainerBase(const CoroutineContainerBase&) = delete;
	CoroutineContainerBase& operator=(const CoroutineContainerBase&) = delete;
	CoroutineContainerBase(CoroutineContainerBase&&) = delete;
	CoroutineContainerBase& operator=(CoroutineContainerBase&&) = delete;

protected:
	virtual CoroutineState BeginCoroutine(CoroutineManager *Manager, const float& DT_Ptr) = 0;
public:
	CoroutineContainerBase() = default;

	void Internal_Enter(CoroutineManager *Manager, float DT);
	void Internal_Tick(CoroutineManager *Manager, float DT);
	void Internal_Exit(CoroutineManager *Manager, float DT);

	bool CoFinished() const;
};

template<class OwnerT>
struct CoroutineContext {
	OwnerT *Owner;
	CoroutineManager *Manager;
	const float &DT;

	// Convenience function to start a corutine, queue it, and return the new coroutine
	template<class CoroutineT, class... Args>
	std::shared_ptr<CoroutineT> StartCoroutine(Args&&... InArgs) {
		auto new_coroutine_ptr = std::make_shared<CoroutineT>(Owner, std::forward<Args>(InArgs)...);
		Manager->QueueCoroutine(new_coroutine_ptr);
		return new_coroutine_ptr;
	}
};

template<class OwnerT>
class CoroutineContainer : public CoroutineContainerBase {
public:
	CoroutineContainer(OwnerT *InOwner) : M_Owner(InOwner) {}

protected:
	using Context = CoroutineContext<OwnerT>;
	virtual CoroutineState CoExec(Context Ctx) = 0;

private:
	OwnerT * M_Owner;

	CoroutineState BeginCoroutine(CoroutineManager *Manager, const float& DT_Ptr) override final {
		return CoExec(Context{ M_Owner, Manager, DT_Ptr });
	}
};

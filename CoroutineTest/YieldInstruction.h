#pragma once
#include <memory>
#include <experimental/coroutine>

class CoroutineContainerBase;
class CoPromise;

class NextFrame {
public:
	bool await_ready() const;
	void await_suspend(std::experimental::coroutine_handle<CoPromise> awaitingCoroutine);
	void await_resume() const;
};

class YieldInstruction {
public:
	virtual bool shouldResume(float DT) = 0;
};


// Wait for a specified number of seconds
class Yield_WaitForSeconds : public YieldInstruction {
	float remainingTime = 0.0f;

	bool shouldResume(float DT) override;

public:
	Yield_WaitForSeconds(float InSeconds);
};

class WaitForSeconds {
	float Seconds = 0.0f;
public:
	WaitForSeconds(float InSeconds);

	bool await_ready() const;
	void await_suspend(std::experimental::coroutine_handle<CoPromise> awaitingCoroutine);
	void await_resume() const;
};


// Wait for another coroutine. Templated so it can be specialized for specific coroutine types
class Yield_WaitForCoroutine : public YieldInstruction {
public:
	std::weak_ptr<CoroutineContainerBase> coroutine_ptr;

	bool shouldResume(float DT) override;
	Yield_WaitForCoroutine(std::shared_ptr<CoroutineContainerBase> in_ptr);
};

template<class CoroutineT>
class WaitForCoroutine {
	std::shared_ptr<CoroutineT> coroutine_ptr;
public:
	WaitForCoroutine(std::shared_ptr<CoroutineT> in_ptr) : coroutine_ptr(std::move(in_ptr)) { }

	bool await_ready() const { return false; }
	void await_suspend(std::experimental::coroutine_handle<CoPromise> awaitingCoroutine) {
		// Don't hold on to our coroutine_ptr, since we don't need it after this!
		awaitingCoroutine.promise().CurrentYield = std::make_shared<Yield_WaitForCoroutine>(std::move(coroutine_ptr));
	}
	void await_resume() const {}
};

template<class CoroutineT>
inline WaitForCoroutine<CoroutineT> operator co_await(std::shared_ptr<CoroutineT> in_ptr) { return WaitForCoroutine<CoroutineT>{ std::move(in_ptr) }; }
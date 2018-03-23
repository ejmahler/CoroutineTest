#include "YieldInstruction.h"
#include "CoroutineContainer.h"

bool NextFrame::await_ready() const {
	return false;
}
void NextFrame::await_suspend(std::experimental::coroutine_handle<CoPromise> awaitingCoroutine) {
	awaitingCoroutine.promise().CurrentYield = nullptr;
}
void NextFrame::await_resume() const {

}


Yield_WaitForSeconds::Yield_WaitForSeconds(float InSeconds) : remainingTime(InSeconds) {}
bool Yield_WaitForSeconds::shouldResume(float DT) {
	remainingTime -= DT;
	return remainingTime <= 0.0f;
}


Yield_WaitForCoroutine::Yield_WaitForCoroutine(std::shared_ptr<CoroutineContainerBase> in_ptr) : coroutine_ptr(in_ptr) {}
bool Yield_WaitForCoroutine::shouldResume(float DT) {
	auto strong_ptr = coroutine_ptr.lock();
	return !strong_ptr || strong_ptr->CoFinished();
}



WaitForSeconds::WaitForSeconds(float in_seconds) : Seconds(in_seconds) {}

bool WaitForSeconds::await_ready() const {
	return false;
}
void WaitForSeconds::await_suspend(std::experimental::coroutine_handle<CoPromise> awaitingCoroutine) {
	awaitingCoroutine.promise().CurrentYield = std::make_shared<Yield_WaitForSeconds>(Seconds);
}
void WaitForSeconds::await_resume() const {

}
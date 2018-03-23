#include "CoroutineCore.h"
#include "YieldInstruction.h"

CoroutineState::CoroutineState(CoroutineState &&InState) {
	if (CoroutineHandle != InState.CoroutineHandle) {
		if (CoroutineHandle) {
			CoroutineHandle.destroy();
		}
		CoroutineHandle = InState.CoroutineHandle;
		InState.CoroutineHandle = nullptr;
	}
}
CoroutineState& CoroutineState::operator=(CoroutineState &&InState) {
	if (CoroutineHandle != InState.CoroutineHandle) {
		if (CoroutineHandle) {
			CoroutineHandle.destroy();
		}
		CoroutineHandle = InState.CoroutineHandle;
		InState.CoroutineHandle = nullptr;
	}
	return *this;
}

CoroutineState::~CoroutineState() {
	if (CoroutineHandle) {
		CoroutineHandle.destroy();
		CoroutineHandle = nullptr;
	}
}

void CoroutineState::Tick(float DT) {
	if (CoroutineHandle && !CoroutineHandle.done()) {
		auto CurrentYield = CoroutineHandle.promise().CurrentYield.get();
		if (!CurrentYield || CurrentYield->shouldResume(DT)) {
			CoroutineHandle.resume();
		}
	}
}

bool CoroutineState::CoFinished() const { return !CoroutineHandle || CoroutineHandle.done(); }
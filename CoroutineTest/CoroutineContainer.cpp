#include "CoroutineContainer.h"

void CoroutineContainerBase::Internal_Enter(CoroutineManager *Manager, float DT) {
	MostRecentDT = DT;
	InternalCoroutineState = BeginCoroutine(Manager, MostRecentDT);
}
void CoroutineContainerBase::Internal_Tick(CoroutineManager *Manager, float DT) {
	MostRecentDT = DT;
	InternalCoroutineState.Tick(DT);
}
void CoroutineContainerBase::Internal_Exit(CoroutineManager *Manager, float DT) {
	MostRecentDT = DT;
	InternalCoroutineState = CoroutineState{};
}

bool CoroutineContainerBase::CoFinished() const { return InternalCoroutineState.CoFinished(); }
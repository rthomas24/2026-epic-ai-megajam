// Triad Drift - records event bus payloads for Flow automation tests. Owner: Teammate 5.

#include "TriadDrift/Flow/Tests/TDFlowTestListener.h"
#include "TriadDrift/Core/TDEventBus.h"

void UTDFlowTestListener::Bind(UTDEventBus* Bus)
{
	if (!Bus)
	{
		return;
	}
	Bus->OnPhaseChanged.AddUniqueDynamic(this, &UTDFlowTestListener::OnPhase);
	Bus->OnAbilityUnlocked.AddUniqueDynamic(this, &UTDFlowTestListener::OnUnlock);
	Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDFlowTestListener::OnTake);
	Bus->OnTipRequested.AddUniqueDynamic(this, &UTDFlowTestListener::OnTip);
	Bus->OnBarkRequested.AddUniqueDynamic(this, &UTDFlowTestListener::OnBark);
}

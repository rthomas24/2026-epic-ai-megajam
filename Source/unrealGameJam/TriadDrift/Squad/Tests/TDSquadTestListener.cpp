// Triad Drift - Squad tests (Teammate 3).

#include "TriadDrift/Squad/Tests/TDSquadTestListener.h"
#include "TriadDrift/Core/TDEventBus.h"

void UTDSquadTestListener::Listen(UTDEventBus* Bus)
{
	StopListening();
	if (!Bus)
	{
		return;
	}
	BoundBus = Bus;
	Bus->OnShipSwitched.AddDynamic(this, &UTDSquadTestListener::OnSwitched);
	Bus->OnShipDowned.AddDynamic(this, &UTDSquadTestListener::OnDowned);
	Bus->OnShipRespawned.AddDynamic(this, &UTDSquadTestListener::OnRespawned);
	Bus->OnSquadWiped.AddDynamic(this, &UTDSquadTestListener::OnWiped);
	Bus->OnPowerUsed.AddDynamic(this, &UTDSquadTestListener::OnPowerUsed);
}

void UTDSquadTestListener::StopListening()
{
	if (UTDEventBus* Bus = BoundBus.Get())
	{
		Bus->OnShipSwitched.RemoveAll(this);
		Bus->OnShipDowned.RemoveAll(this);
		Bus->OnShipRespawned.RemoveAll(this);
		Bus->OnSquadWiped.RemoveAll(this);
		Bus->OnPowerUsed.RemoveAll(this);
	}
	BoundBus.Reset();
}

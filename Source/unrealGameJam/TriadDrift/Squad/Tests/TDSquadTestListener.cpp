// Triad Drift - Squad tests (Teammate 3).

#include "TriadDrift/Squad/Tests/TDSquadTestListener.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"

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
	Bus->OnHighlightMoment.AddDynamic(this, &UTDSquadTestListener::OnHighlight);
	Bus->OnBarkRequested.AddDynamic(this, &UTDSquadTestListener::OnBark);
	Bus->OnSpacecraftTeleported.AddDynamic(this, &UTDSquadTestListener::OnTeleported);
	Bus->OnActorKilled.AddDynamic(this, &UTDSquadTestListener::OnKilled);
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
		Bus->OnHighlightMoment.RemoveAll(this);
		Bus->OnBarkRequested.RemoveAll(this);
		Bus->OnSpacecraftTeleported.RemoveAll(this);
		Bus->OnActorKilled.RemoveAll(this);
	}
	BoundBus.Reset();
}

void UTDSquadTestListener::OnKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
	const ATDSquadManager* Squad = SquadForKills.Get();
	const ATDShip* Ship = Cast<ATDShip>(Killer);
	if (!Squad || !Ship || Victim == Killer)
	{
		return;
	}
	for (const ETDShipId Id : UTDShipLibrary::AllShips())
	{
		if (Squad->GetShip(Id) == Ship)
		{
			(Id == Squad->GetActiveShipId() ? PlayerKills : WingKills)++;
			return;
		}
	}
}

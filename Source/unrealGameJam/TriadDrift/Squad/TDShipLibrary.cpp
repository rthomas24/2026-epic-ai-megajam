// Triad Drift - Squad (Teammate 3). API-first stub; full ENG-04 implementation follows.

#include "TriadDrift/Squad/TDShipLibrary.h"

const FTDShipSpec& UTDShipLibrary::Get(ETDShipId Ship)
{
	static FTDShipSpec Default;
	return Default;
}

FTDShipSpec UTDShipLibrary::GetShipSpec(ETDShipId Ship)
{
	return Get(Ship);
}

const TArray<ETDShipId>& UTDShipLibrary::AllShips()
{
	static const TArray<ETDShipId> Ships = { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion };
	return Ships;
}

void UTDShipLibrary::ApplySpecToShip(ATDShip* Ship, const FTDShipSpec& Spec) {}

void UTDShipLibrary::ApplyDefaultSpecToShip(ATDShip* Ship, ETDShipId ShipId)
{
	ApplySpecToShip(Ship, Get(ShipId));
}

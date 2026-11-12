// Triad Drift - Powers (Teammate 2). Per-ship power installation.

#include "TriadDrift/Powers/TDPowerLibrary.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDRamPower.h"

TSubclassOf<UTDPowerComponent> UTDPowerLibrary::ClassFor(ETDShipId Ship, ETDPowerInput Slot)
{
	switch (Ship)
	{
	case ETDShipId::Rift:
		return Slot == ETDPowerInput::Signature1 ? UTDPortalBlasterPower::StaticClass() : nullptr;
	case ETDShipId::Lancer:
		return Slot == ETDPowerInput::Signature1 ? UTDRamPower::StaticClass() : nullptr;
	case ETDShipId::Bastion:
		if (Slot == ETDPowerInput::Signature1)
		{
			return UTDMissileSwarmPower::StaticClass();
		}
		return Slot == ETDPowerInput::Signature2 ? UTDShieldDomePower::StaticClass() : nullptr;
	default:
		return nullptr;
	}
}

void UTDPowerLibrary::InstallShipPowers(AActor* Ship, ETDShipId ShipId)
{
	if (!IsValid(Ship))
	{
		return;
	}
	for (const ETDPowerInput Slot : { ETDPowerInput::Signature1, ETDPowerInput::Signature2 })
	{
		const TSubclassOf<UTDPowerComponent> Class = ClassFor(ShipId, Slot);
		if (!Class || UTDPowerComponent::FindPower(Ship, Slot))
		{
			continue;
		}
		UTDPowerComponent* Power = NewObject<UTDPowerComponent>(Ship, Class);
		Power->Slot = Slot;
		Power->RegisterComponent();
	}
	if (!Ship->FindComponentByClass<UTDAbilityComponent>())
	{
		UTDAbilityComponent* Abilities = NewObject<UTDAbilityComponent>(Ship, TEXT("AbilitySlot"));
		Abilities->RegisterComponent();
	}
}

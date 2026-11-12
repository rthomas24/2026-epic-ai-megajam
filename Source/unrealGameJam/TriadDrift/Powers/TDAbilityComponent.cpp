// Triad Drift - Powers (Teammate 2). Unlockable-ability slot.

#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"

UTDAbilityComponent::UTDAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	AbilityClasses.Add(ETDAbilityId::GravityWell, UTDGravityWellPower::StaticClass());
	AbilityClasses.Add(ETDAbilityId::Cloak, UTDCloakPower::StaticClass());
	AbilityClasses.Add(ETDAbilityId::EMPPulse, UTDEMPPower::StaticClass());
}

void UTDAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnAbilityUnlocked.AddDynamic(this, &UTDAbilityComponent::HandleAbilityUnlocked);
	}
	RefreshFromProgress();
}

void UTDAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnAbilityUnlocked.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool UTDAbilityComponent::GetOwnerShip(ETDShipId& OutShip) const
{
	if (const ATDShip* Ship = Cast<ATDShip>(GetOwner()))
	{
		OutShip = Ship->GetShipId();
		return true;
	}
	return false;
}

void UTDAbilityComponent::RefreshFromProgress()
{
	ETDShipId MyShip;
	ITDGameFlow* Flow = UTDStatics::GetGameFlow(this);
	if (!Flow || !GetOwnerShip(MyShip))
	{
		return;
	}
	for (const TPair<ETDAbilityId, ETDShipId>& Unlock : Flow->GetProgress().UnlockedAbilities)
	{
		if (Unlock.Value == MyShip && Unlock.Key != ETDAbilityId::None)
		{
			SetAbility(Unlock.Key);
			return;
		}
	}
}

void UTDAbilityComponent::HandleAbilityUnlocked(ETDAbilityId UnlockedAbility, ETDShipId Ship)
{
	ETDShipId MyShip;
	if (GetOwnerShip(MyShip) && MyShip == Ship)
	{
		SetAbility(UnlockedAbility);
	}
}

void UTDAbilityComponent::SetAbility(ETDAbilityId NewAbility)
{
	if (NewAbility == Ability && (Power || NewAbility == ETDAbilityId::None))
	{
		return;
	}
	if (Power)
	{
		Power->EndPower(true);
		Power->DestroyComponent();
		Power = nullptr;
	}
	Ability = NewAbility;

	AActor* Owner = GetOwner();
	const TSubclassOf<UTDPowerComponent>* Class = AbilityClasses.Find(NewAbility);
	if (!Owner || !Class || !*Class)
	{
		return;
	}
	Power = NewObject<UTDPowerComponent>(Owner, *Class);
	Power->Slot = ETDPowerInput::Ability;
	Power->RegisterComponent();
}

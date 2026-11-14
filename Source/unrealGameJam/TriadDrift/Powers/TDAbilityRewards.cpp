// Triad Drift - ability reward state (board N-070; Teammate 2). See the header.

#include "TriadDrift/Powers/TDAbilityRewards.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flight/TDShip.h"

UTDAbilityRewardSubsystem* UTDAbilityRewardSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UTDAbilityRewardSubsystem>() : nullptr;
}

void UTDAbilityRewardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UTDEventBus* Bus = Collection.InitializeDependency<UTDEventBus>())
	{
		Bus->OnAbilityUnlocked.AddUniqueDynamic(this, &UTDAbilityRewardSubsystem::HandleAbilityUnlocked);
	}
}

void UTDAbilityRewardSubsystem::Deinitialize()
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (UTDEventBus* Bus = GI->GetSubsystem<UTDEventBus>())
		{
			Bus->OnAbilityUnlocked.RemoveDynamic(this, &UTDAbilityRewardSubsystem::HandleAbilityUnlocked);
		}
	}
	Super::Deinitialize();
}

void UTDAbilityRewardSubsystem::HandleAbilityUnlocked(ETDAbilityId Ability, ETDShipId Ship)
{
	if (Ability == ETDAbilityId::None)
	{
		return;
	}
	// One entry per award. The game instance only re-broadcasts when the owner changes: that is a new award.
	Rewards.RemoveAll([Ability](const FTDAbilityReward& R) { return R.Ability == Ability; });
	FTDAbilityReward& R = Rewards.AddDefaulted_GetRef();
	R.Ability = Ability;
	R.Ship = Ship;
}

const UWorld* UTDAbilityRewardSubsystem::ResolveWorld(const UObject* WorldContext) const
{
	if (const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		return W;
	}
	return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
}

bool UTDAbilityRewardSubsystem::GetFlownShip(const UWorld* World, ETDShipId& OutShip)
{
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
	if (!Ship)
	{
		return false;
	}
	OutShip = Ship->GetShipId();
	return true;
}

bool UTDAbilityRewardSubsystem::GetPendingAbilityReward(ETDAbilityId& Ability, ETDShipId& Ship, bool& bCanUseNow, const UObject* WorldContext) const
{
	Ability = ETDAbilityId::None;
	bCanUseNow = false;
	for (const FTDAbilityReward& R : Rewards)
	{
		if (!R.bPresented)
		{
			Ability = R.Ability;
			Ship = R.Ship;
			ETDShipId Flown;
			bCanUseNow = GetFlownShip(ResolveWorld(WorldContext), Flown) && Flown == R.Ship;
			return true;
		}
	}
	return false;
}

void UTDAbilityRewardSubsystem::MarkAbilityRewardPresented(ETDAbilityId Ability)
{
	for (FTDAbilityReward& R : Rewards)
	{
		if (R.Ability == Ability)
		{
			R.bPresented = true;
		}
	}
}

bool UTDAbilityRewardSubsystem::GetAbilityFlightHint(ETDAbilityId& Ability, const UObject* WorldContext) const
{
	Ability = ETDAbilityId::None;
	ETDShipId Flown;
	if (!GetFlownShip(ResolveWorld(WorldContext), Flown))
	{
		return false;
	}
	for (const FTDAbilityReward& R : Rewards)
	{
		if (!R.bFlightHintShown && R.Ship == Flown)
		{
			Ability = R.Ability;
			return true;
		}
	}
	return false;
}

void UTDAbilityRewardSubsystem::ReopenFlightHint(ETDAbilityId Ability)
{
	for (FTDAbilityReward& R : Rewards)
	{
		if (R.Ability == Ability)
		{
			R.bFlightHintShown = false;
		}
	}
}

bool UTDAbilityRewardSubsystem::GetAbilityShip(ETDAbilityId Ability, ETDShipId& OutShip) const
{
	for (const FTDAbilityReward& R : Rewards)
	{
		if (R.Ability == Ability)
		{
			OutShip = R.Ship;
			return true;
		}
	}
	return false;
}

void UTDAbilityRewardSubsystem::MarkAbilityFlightHintShown(ETDAbilityId Ability)
{
	for (FTDAbilityReward& R : Rewards)
	{
		if (R.Ability == Ability)
		{
			R.bFlightHintShown = true;
		}
	}
}

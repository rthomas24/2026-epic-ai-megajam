#include "TriadDrift/Core/TDCombatRegistry.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"

namespace
{
	bool IsLiveCombatant(const AActor* Actor, bool bIncludeUntargetable)
	{
		if (!IsValid(Actor))
		{
			return false;
		}
		const UTDHealthComponent* Health = Actor->FindComponentByClass<UTDHealthComponent>();
		if (!Health || Health->IsDead())
		{
			return false;
		}
		return bIncludeUntargetable || Health->bTargetable;
	}
}

UTDCombatRegistry* UTDCombatRegistry::Get(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UTDCombatRegistry>() : nullptr;
}

void UTDCombatRegistry::Register(AActor* Actor)
{
	if (Actor)
	{
		Actors.AddUnique(Actor);
	}
}

void UTDCombatRegistry::Unregister(AActor* Actor)
{
	Actors.RemoveAllSwap([Actor](const TWeakObjectPtr<AActor>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Actor;
	});
}

void UTDCombatRegistry::GetActorsOfTeam(ETDTeam Team, TArray<AActor*>& OutActors, bool bIncludeUntargetable) const
{
	OutActors.Reset();
	for (const TWeakObjectPtr<AActor>& Entry : Actors)
	{
		AActor* Actor = Entry.Get();
		if (IsLiveCombatant(Actor, bIncludeUntargetable) && UTDStatics::GetTeam(Actor) == Team)
		{
			OutActors.Add(Actor);
		}
	}
}

void UTDCombatRegistry::GetHostilesOf(ETDTeam Team, TArray<AActor*>& OutActors, bool bIncludeUntargetable) const
{
	OutActors.Reset();
	for (const TWeakObjectPtr<AActor>& Entry : Actors)
	{
		AActor* Actor = Entry.Get();
		if (IsLiveCombatant(Actor, bIncludeUntargetable) && UTDStatics::AreTeamsHostile(Team, UTDStatics::GetTeam(Actor)))
		{
			OutActors.Add(Actor);
		}
	}
}

AActor* UTDCombatRegistry::FindNearestHostile(ETDTeam Team, const FVector& Location, float MaxDistance, const AActor* Ignore) const
{
	AActor* Best = nullptr;
	double BestDistSq = MaxDistance > 0.f ? FMath::Square(static_cast<double>(MaxDistance)) : TNumericLimits<double>::Max();
	for (const TWeakObjectPtr<AActor>& Entry : Actors)
	{
		AActor* Actor = Entry.Get();
		if (Actor == Ignore || !IsLiveCombatant(Actor, false) || !UTDStatics::AreTeamsHostile(Team, UTDStatics::GetTeam(Actor)))
		{
			continue;
		}
		const double DistSq = FVector::DistSquared(Location, Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Actor;
		}
	}
	return Best;
}

int32 UTDCombatRegistry::CountAlive(ETDTeam Team) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Entry : Actors)
	{
		const AActor* Actor = Entry.Get();
		if (IsLiveCombatant(Actor, true) && UTDStatics::GetTeam(Actor) == Team)
		{
			++Count;
		}
	}
	return Count;
}

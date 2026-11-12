// Triad Drift - Powers (Teammate 2). World-level power bookkeeping.

#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "Engine/World.h"

UTDPowerSubsystem* UTDPowerSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDPowerSubsystem>() : nullptr;
}

bool UTDPowerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

void UTDPowerSubsystem::RegisterDome(ATDShieldDome* Dome)
{
	if (IsValid(Dome))
	{
		Domes.AddUnique(Dome);
	}
}

void UTDPowerSubsystem::UnregisterDome(ATDShieldDome* Dome)
{
	Domes.RemoveSingleSwap(Dome, EAllowShrinking::No);
}

ATDShieldDome* UTDPowerSubsystem::FindDomeCovering(const FVector& Location, ETDTeam Team) const
{
	for (ATDShieldDome* Dome : Domes)
	{
		if (IsValid(Dome) && Dome->IsDomeActive() && Dome->GetTeam() == Team && Dome->Covers(Location))
		{
			return Dome;
		}
	}
	return nullptr;
}

ATDShieldDome* UTDPowerSubsystem::GetNearestDome(const FVector& Location, ETDTeam Team) const
{
	ATDShieldDome* Best = nullptr;
	double BestSq = TNumericLimits<double>::Max();
	for (ATDShieldDome* Dome : Domes)
	{
		if (IsValid(Dome) && Dome->IsDomeActive() && Dome->GetTeam() == Team)
		{
			const double DistSq = FVector::DistSquared(Dome->GetActorLocation(), Location);
			if (DistSq < BestSq)
			{
				BestSq = DistSq;
				Best = Dome;
			}
		}
	}
	return Best;
}

void UTDPowerSubsystem::MarkTransited(const AActor* Actor)
{
	if (const UWorld* World = GetWorld())
	{
		// Drop stale entries so the map can't grow without bound (projectiles are pooled, ships few).
		if (LastTransit.Num() > 128)
		{
			const double Now = World->GetTimeSeconds();
			for (auto It = LastTransit.CreateIterator(); It; ++It)
			{
				if (!It->Key.IsValid() || Now - It->Value > 5.0)
				{
					It.RemoveCurrent();
				}
			}
		}
		LastTransit.Add(Actor, World->GetTimeSeconds());
	}
}

bool UTDPowerSubsystem::IsInTransitCooldown(const AActor* Actor, float CooldownSeconds) const
{
	const UWorld* World = GetWorld();
	const double* Last = LastTransit.Find(Actor);
	return World && Last && (World->GetTimeSeconds() - *Last) < CooldownSeconds;
}

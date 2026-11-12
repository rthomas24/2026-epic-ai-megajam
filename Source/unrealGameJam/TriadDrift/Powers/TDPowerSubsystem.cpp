// Triad Drift - Powers (Teammate 2). World-level power bookkeeping.

#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Core/TDEventBus.h"
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

void UTDPowerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UTDEventBus* Bus = UTDEventBus::Get(&InWorld))
	{
		Bus->OnSceneCut.AddUniqueDynamic(this, &UTDPowerSubsystem::HandleSceneCut);
		Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDPowerSubsystem::HandleTakeStarted);
	}
}

void UTDPowerSubsystem::Deinitialize()
{
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnSceneCut.RemoveAll(this);
		Bus->OnTakeStarted.RemoveAll(this);
	}
	Super::Deinitialize();
}

void UTDPowerSubsystem::HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds)
{
	ResetAllPowers();
}

void UTDPowerSubsystem::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	// Retakes only: a first take (new wave) keeps whatever the player has running.
	if (TakeNumber > 1)
	{
		ResetAllPowers();
	}
}

void UTDPowerSubsystem::ResetAllPowers()
{
	OnPowersReset.Broadcast();
	TArray<TObjectPtr<ATDShieldDome>> DomesCopy = Domes;
	for (ATDShieldDome* Dome : DomesCopy)
	{
		if (IsValid(Dome))
		{
			Dome->Collapse();
		}
	}
	Domes.Reset();
	for (const TWeakObjectPtr<AActor>& W : TransientActors)
	{
		if (AActor* A = W.Get())
		{
			A->Destroy();
		}
	}
	TransientActors.Reset();
	LastTransit.Reset();
}

void UTDPowerSubsystem::RegisterPortal(ATDPortal* Portal)
{
	if (Portal)
	{
		Portals.AddUnique(Portal);
	}
}

void UTDPowerSubsystem::UnregisterPortal(ATDPortal* Portal)
{
	Portals.Remove(Portal);
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
		if (IsValid(Dome) && Dome->IsDomeActive() && Dome->bProtectSquad && Dome->GetTeam() == Team && Dome->Covers(Location))
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
		if (IsValid(Dome) && Dome->IsDomeActive() && Dome->bProtectSquad && Dome->GetTeam() == Team)
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
	if (const ATDProjectile* P = Cast<ATDProjectile>(Actor))
	{
		if (const UWorld* World = GetWorld())
		{
			const_cast<ATDProjectile*>(P)->SetLastTransitTime(World->GetTimeSeconds());
		}
		return; // pooled: never key projectiles by actor
	}
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
	if (const ATDProjectile* P = Cast<ATDProjectile>(Actor))
	{
		return World && P->GetLastTransitTime() >= 0.0 && (World->GetTimeSeconds() - P->GetLastTransitTime()) < CooldownSeconds;
	}
	const double* Last = LastTransit.Find(Actor);
	return World && Last && (World->GetTimeSeconds() - *Last) < CooldownSeconds;
}

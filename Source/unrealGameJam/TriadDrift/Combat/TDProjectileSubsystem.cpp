// Triad Drift - Combat (Teammate 2). Projectile pool + live-projectile index.

#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "Engine/World.h"

UTDProjectileSubsystem* UTDProjectileSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDProjectileSubsystem>() : nullptr;
}

bool UTDProjectileSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

ATDProjectile* UTDProjectileSubsystem::Acquire()
{
	while (Pool.Num() > 0)
	{
		// FIFO: the oldest released projectile is reused first, so fading tracers finish.
		ATDProjectile* P = Pool[0];
		Pool.RemoveAt(0, 1, EAllowShrinking::No);
		if (IsValid(P))
		{
			return P;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	return World->SpawnActor<ATDProjectile>(ATDProjectile::StaticClass(), FTransform::Identity, Params);
}

void UTDProjectileSubsystem::Release(ATDProjectile* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}
	UnregisterLive(Projectile);
	if (Pool.Num() < MaxPooled)
	{
		Pool.AddUnique(Projectile);
	}
	else
	{
		Projectile->Destroy();
	}
}

void UTDProjectileSubsystem::RegisterLive(ATDProjectile* Projectile)
{
	if (IsValid(Projectile))
	{
		Live.AddUnique(Projectile);
	}
}

void UTDProjectileSubsystem::UnregisterLive(ATDProjectile* Projectile)
{
	Live.RemoveSingleSwap(Projectile, EAllowShrinking::No);
}

void UTDProjectileSubsystem::FindHostileMissiles(ETDTeam Team, const FVector& Location, float Radius, TArray<ATDProjectile*>& Out) const
{
	const float RadiusSq = Radius * Radius;
	for (ATDProjectile* P : Live)
	{
		if (IsValid(P) && P->IsActive() && P->IsMissile() && UTDStatics::AreTeamsHostile(Team, P->GetTeam())
			&& FVector::DistSquared(P->GetActorLocation(), Location) <= RadiusSq)
		{
			Out.Add(P);
		}
	}
}

void UTDProjectileSubsystem::FindMissilesHomingOn(const AActor* Target, TArray<ATDProjectile*>& Out) const
{
	if (!Target)
	{
		return;
	}
	for (ATDProjectile* P : Live)
	{
		if (IsValid(P) && P->IsActive() && P->IsMissile() && P->GetSpec().bHoming && P->GetHomingTarget() == Target)
		{
			Out.Add(P);
		}
	}
}

void UTDProjectileSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UTDEventBus* Bus = UTDEventBus::Get(&InWorld))
	{
		Bus->OnSceneCut.AddUniqueDynamic(this, &UTDProjectileSubsystem::HandleSceneCut);
		Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDProjectileSubsystem::HandleTakeStarted);
	}
}

void UTDProjectileSubsystem::HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds)
{
	KillAll(false);
}

void UTDProjectileSubsystem::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	// Retakes only: a first take (new wave) keeps whatever the player has running.
	if (TakeNumber > 1)
	{
		KillAll(false);
	}
}

int32 UTDProjectileSubsystem::KillAll(bool bPlayFX)
{
	TArray<ATDProjectile*> ToKill;
	for (ATDProjectile* P : Live)
	{
		if (IsValid(P) && P->IsActive())
		{
			ToKill.Add(P);
		}
	}
	for (ATDProjectile* P : ToKill)
	{
		P->Kill(bPlayFX);
	}
	Live.RemoveAll([](const TObjectPtr<ATDProjectile>& P) { return !IsValid(P) || !P->IsActive(); });
	return ToKill.Num();
}

void UTDProjectileSubsystem::Deinitialize()
{
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnSceneCut.RemoveAll(this);
		Bus->OnTakeStarted.RemoveAll(this);
	}
	Pool.Reset();
	Live.Reset();
	Super::Deinitialize();
}

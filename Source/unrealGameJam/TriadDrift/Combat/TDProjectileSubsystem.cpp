// Triad Drift - Combat (Teammate 2). Projectile pool + live-projectile index.

#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
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

ATDCombatFlash* UTDProjectileSubsystem::PlayPlaceholderFlash(const FVector& Location, float Radius, const FLinearColor& Color, float Seconds,
	USceneComponent* AttachTo)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return nullptr;
	}
	Flashes.RemoveAll([](const TObjectPtr<ATDCombatFlash>& F) { return !IsValid(F); });

	ATDCombatFlash* Flash = nullptr;
	for (ATDCombatFlash* Candidate : Flashes)
	{
		if (!Candidate->IsPlaying())
		{
			Flash = Candidate;
			break;
		}
	}
	if (!Flash && Flashes.Num() < MaxFlashes)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Flash = World->SpawnActor<ATDCombatFlash>(ATDCombatFlash::StaticClass(), Location, FRotator::ZeroRotator, Params);
		if (Flash)
		{
			Flashes.Add(Flash);
		}
	}
	if (!Flash && Flashes.Num() > 0)
	{
		Flash = Flashes[NextFlash++ % Flashes.Num()];
	}
	if (Flash)
	{
		Flash->Play(Location, Radius, Color, Seconds, AttachTo);
	}
	return Flash;
}

int32 UTDProjectileSubsystem::PlayPlaceholderExplosion(const FVector& Location, float Radius, const FLinearColor& Color, int32 DebrisCount)
{
	int32 Used = 0;
	const float R = FMath::Max(Radius, 100.f);
	// A small white-hot core, then the slower shell in pure team colour at a low glow. Live in a wave-5 furball the old
	// shell (15% white at glow 14) blew out to big flat pink-white discs over the player's ship: against black space any
	// saturated colour at glow 14 tonemaps to white. The shell now stays enemy red-orange / squad colour.
	Used += PlayPlaceholderFlash(Location, R * 0.3f, FMath::Lerp(Color, FLinearColor::White, 0.45f), 0.22f) ? 1 : 0;
	Used += PlayPlaceholderFlash(Location, R, Color * ExplosionShellDim, 0.6f) ? 1 : 0;
	// Debris streaks, evenly spread on a jittered sphere so they read from any camera angle.
	const int32 Count = FMath::Clamp(DebrisCount, 0, 16);
	for (int32 I = 0; I < Count; ++I)
	{
		const float Y = 1.f - 2.f * (I + 0.5f) / Count;
		const float RingR = FMath::Sqrt(FMath::Max(0.f, 1.f - Y * Y));
		const float Phi = I * 2.39996f + FMath::FRandRange(-0.3f, 0.3f); // golden angle
		const FVector Dir(FMath::Cos(Phi) * RingR, FMath::Sin(Phi) * RingR, Y);
		const FVector Velocity = Dir * R * FMath::FRandRange(2.5f, 4.f);
		ATDCombatFlash* Flash = nullptr;
		Flashes.RemoveAll([](const TObjectPtr<ATDCombatFlash>& F) { return !IsValid(F); });
		for (ATDCombatFlash* Candidate : Flashes)
		{
			if (!Candidate->IsPlaying())
			{
				Flash = Candidate;
				break;
			}
		}
		if (!Flash && Flashes.Num() < MaxFlashes)
		{
			Flash = PlayPlaceholderFlash(Location, 1.f, Color, 0.01f); // spawns (and parks) one more pooled actor
		}
		if (Flash)
		{
			Flash->PlayDebris(Location + Dir * R * 0.3f, Velocity, R * 0.6f, FMath::Clamp(R * 0.06f, 20.f, 90.f), Color * ExplosionDebrisDim, FMath::FRandRange(0.5f, 0.8f));
			++Used;
		}
	}
	return Used;
}

int32 UTDProjectileSubsystem::GetPlayingFlashCount() const
{
	int32 Count = 0;
	for (const ATDCombatFlash* Flash : Flashes)
	{
		Count += (IsValid(Flash) && Flash->IsPlaying()) ? 1 : 0;
	}
	return Count;
}

int32 UTDProjectileSubsystem::CountPlayingFlashesNear(const FVector& Location, float Radius) const
{
	int32 Count = 0;
	for (const ATDCombatFlash* Flash : Flashes)
	{
		Count += (IsValid(Flash) && Flash->IsPlaying() && FVector::DistSquared(Flash->GetActorLocation(), Location) <= Radius * Radius) ? 1 : 0;
	}
	return Count;
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

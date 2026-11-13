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

ATDCombatFlash* UTDProjectileSubsystem::AcquireFlash(const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return nullptr;
	}
	Flashes.RemoveAll([](const TObjectPtr<ATDCombatFlash>& F) { return !IsValid(F); });
	for (ATDCombatFlash* Candidate : Flashes)
	{
		if (!Candidate->IsPlaying())
		{
			return Candidate;
		}
	}
	if (Flashes.Num() < MaxFlashes)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		if (ATDCombatFlash* Flash = World->SpawnActor<ATDCombatFlash>(ATDCombatFlash::StaticClass(), Location, FRotator::ZeroRotator, Params))
		{
			Flashes.Add(Flash);
			return Flash;
		}
	}
	return Flashes.Num() > 0 ? Flashes[NextFlash++ % Flashes.Num()].Get() : nullptr;
}

ATDCombatFlash* UTDProjectileSubsystem::PlayPlaceholderFlash(const FVector& Location, float Radius, const FLinearColor& Color, float Seconds,
	USceneComponent* AttachTo)
{
	ATDCombatFlash* Flash = AcquireFlash(Location);
	if (Flash)
	{
		Flash->Play(Location, Radius, Color, Seconds, AttachTo);
	}
	return Flash;
}

int32 UTDProjectileSubsystem::PlayPlaceholderHit(const FVector& Location, float Radius, bool bShield, const FVector& Normal)
{
	int32 Used = 0;
	const float Cap = ATDCombatFlash::MaxShellRadiusPerCameraDistance;
	if (bShield)
	{
		FTDFlashLook Shield;
		Shield.Hot = FLinearColor(0.85f, 0.97f, 1.f);
		Shield.Mid = ShieldHitColor();
		Shield.Cool = FLinearColor(0.1f, 0.3f, 0.9f);
		Shield.Glow = 2.2f;
		Shield.GlowExponent = 1.5f;
		Shield.GrowFrom = 0.6f;
		Shield.GrowTo = 1.f;
		Shield.EdgePower = 0.9f;
		if (ATDCombatFlash* Flash = AcquireFlash(Location))
		{
			Flash->PlayLook(Location, Radius * 1.4f, 0.2f, Shield);
			Flash->SetCameraCap(Cap);
			++Used;
		}
		return Used;
	}
	FTDFlashLook Hull;
	Hull.Hot = FLinearColor::White;
	Hull.Mid = FLinearColor(1.f, 0.75f, 0.3f);
	Hull.Cool = FLinearColor(1.f, 0.35f, 0.05f);
	Hull.Glow = 3.f;
	Hull.GlowExponent = 2.f;
	Hull.GrowFrom = 0.5f;
	Hull.GrowTo = 0.9f;
	Hull.EdgePower = 1.6f;
	if (ATDCombatFlash* Flash = AcquireFlash(Location))
	{
		Flash->PlayLook(Location, Radius * 0.8f, 0.13f, Hull);
		Flash->SetCameraCap(Cap);
		++Used;
	}
	FTDFlashLook Spark = Hull;
	Spark.Hot = FLinearColor(1.f, 0.9f, 0.6f);
	Spark.Glow = 3.f;
	Spark.GlowExponent = 1.2f;
	const FVector Out = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	for (int32 I = 0; I < 3; ++I)
	{
		const FVector Dir = (Out + FMath::VRand() * 0.8f).GetSafeNormal();
		if (ATDCombatFlash* Flash = AcquireFlash(Location))
		{
			Flash->PlayDebrisLook(Location, Dir * Radius * FMath::FRandRange(4.f, 7.f), Radius * 0.6f, FMath::Clamp(Radius * 0.06f, 5.f, 14.f),
				FMath::FRandRange(0.18f, 0.3f), Spark);
			++Used;
		}
	}
	return Used;
}

int32 UTDProjectileSubsystem::PlayPlaceholderExplosion(const FVector& Location, float Radius, const FLinearColor& Color, int32 DebrisCount)
{
	// An arcade explosion until Nick's FX Death lands (the Lead, after T4's frame-by-frame: a flat peach disc and flat
	// ellipses, gone in 0.3 s). Additive soft glow throughout (M_TD_FlashGlow): nothing ever fades to a dark disc.
	int32 Used = 0;
	const float R = FMath::Max(Radius, 100.f);
	const FLinearColor Team = Color;
	auto Part = [this, &Used](const FVector& At, float PartRadius, float Seconds, const FTDFlashLook& Look, float Cap)
	{
		if (ATDCombatFlash* Flash = AcquireFlash(At))
		{
			Flash->PlayLook(At, PartRadius, Seconds, Look);
			Flash->SetCameraCap(Cap);
			++Used;
		}
	};
	const float ShellCap = ATDCombatFlash::MaxShellRadiusPerCameraDistance; // a near death stays under ~25% of the screen

	// 1. A white-hot core that cools through yellow to orange.
	FTDFlashLook Core;
	Core.Hot = FLinearColor::White;
	Core.Mid = FLinearColor(1.f, 0.85f, 0.35f);
	Core.Cool = FLinearColor(1.f, 0.42f, 0.08f);
	// Glows stay low enough that the soft edge survives tonemapping (live: glow 4.5-10 clipped every part to a flat ball).
	Core.Glow = 3.f;
	Core.GlowExponent = 2.f;
	Core.GrowFrom = 0.45f;
	Core.GrowTo = 1.f;
	Core.EdgePower = 2.2f;
	Part(Location, R * 0.45f, 0.32f, Core, ShellCap * 0.6f);

	// 2. The fireball shell: hot yellow-orange -> the team colour -> a dark ember red, soft-edged, easing out.
	FTDFlashLook Shell;
	Shell.Hot = FMath::Lerp(Team, FLinearColor(1.f, 0.8f, 0.4f), 0.6f);
	Shell.Mid = Team;
	Shell.Cool = Team * 0.3f;
	Shell.Glow = 1.6f;
	Shell.GlowExponent = 1.4f;
	Shell.GrowFrom = 0.35f;
	Shell.GrowTo = 1.f;
	Shell.EdgePower = 2.5f;
	Part(Location, R, 0.65f, Shell, ShellCap);

	// 3. Staggered secondary pops across a big hull.
	const int32 Pops = ExplosionPops(R);
	for (int32 I = 0; I < Pops; ++I)
	{
		FTDFlashLook Pop = Core;
		Pop.Glow = 2.4f;
		Pop.Delay = 0.12f * (I + 1);
		Part(Location + FMath::VRand() * R * FMath::FRandRange(0.35f, 0.6f), R * 0.3f, 0.3f, Pop, ShellCap * 0.6f);
	}

	// 4. Ember afterglow: a dim, soft team-coloured glow that swells in and dies over about a second.
	FTDFlashLook Ember;
	Ember.Hot = FMath::Lerp(Team, FLinearColor(1.f, 0.5f, 0.1f), 0.5f);
	Ember.Mid = Team * 0.7f;
	Ember.Cool = Team * 0.25f;
	Ember.Glow = 1.1f;
	Ember.bEmber = true;
	Ember.GrowFrom = 0.7f;
	Ember.GrowTo = 1.15f;
	Ember.Delay = 0.15f;
	Ember.EdgePower = 1.6f;
	Part(Location, R * 0.85f, 0.9f, Ember, ShellCap);

	// 5. Thin hot debris sparks flung out on a jittered sphere, fading as they slow.
	FTDFlashLook Spark;
	Spark.Hot = FLinearColor(1.f, 0.95f, 0.75f);
	Spark.Mid = FLinearColor(1.f, 0.6f, 0.15f);
	Spark.Cool = Team * 0.5f;
	Spark.Glow = 3.f;
	Spark.GlowExponent = 1.2f;
	const int32 Count = FMath::Clamp(DebrisCount, 0, 16);
	for (int32 I = 0; I < Count; ++I)
	{
		const float Y = 1.f - 2.f * (I + 0.5f) / Count;
		const float RingR = FMath::Sqrt(FMath::Max(0.f, 1.f - Y * Y));
		const float Phi = I * 2.39996f + FMath::FRandRange(-0.3f, 0.3f); // golden angle
		const FVector Dir(FMath::Cos(Phi) * RingR, FMath::Sin(Phi) * RingR, Y);
		const FVector Velocity = Dir * R * FMath::FRandRange(2.5f, 4.f);
		if (ATDCombatFlash* Flash = AcquireFlash(Location))
		{
			Flash->PlayDebrisLook(Location + Dir * R * 0.3f, Velocity, R * 0.5f, FMath::Clamp(R * 0.025f, 8.f, 30.f), FMath::FRandRange(0.5f, 0.85f), Spark);
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

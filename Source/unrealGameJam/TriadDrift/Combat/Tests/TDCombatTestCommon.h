// Triad Drift - Combat (Teammate 2). Helpers shared by the TriadDrift.Combat.* test files.
// Inline, in namespace TDCombatTest only: test .cpp files share unity blobs with every other module file.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDCombatTest
{
	/** Frame steps every frame-rate-independence test runs at (30 / 60 / 120 fps). */
	inline constexpr float kCombatFrameSteps[] = { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f };

	/** A box that blocks projectiles, with health on Team. */
	inline AActor* SpawnBody(FTDTestWorld& W, ETDTeam Team, const FVector& Location, const FVector& Extent = FVector(100.f),
		float Hull = 100.f, float Shield = 100.f, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		AActor* Actor = W.Spawn<AActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor, TEXT("Body"));
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Actor->SetRootComponent(Box);
		Box->RegisterComponent();
		Actor->SetActorLocationAndRotation(Location, Rotation);

		UTDHealthComponent* Health = NewObject<UTDHealthComponent>(Actor, TEXT("Health"));
		Health->MaxHull = Hull;
		Health->MaxShield = Shield;
		Health->DefaultTeam = Team;
		Health->RegisterComponent();
		Health->InitMaxValues(Hull, Shield, true);
		return Actor;
	}

	inline UTDWeaponComponent* AddWeapons(AActor* Actor)
	{
		UTDWeaponComponent* Weapons = NewObject<UTDWeaponComponent>(Actor, TEXT("Weapons"));
		Weapons->RegisterComponent();
		Weapons->SetRandomSeed(1234);
		return Weapons;
	}

	inline UTDTargetingComponent* AddTargeting(AActor* Actor)
	{
		UTDTargetingComponent* Targeting = NewObject<UTDTargetingComponent>(Actor, TEXT("Targeting"));
		Targeting->RegisterComponent();
		return Targeting;
	}

	inline float Shield(const AActor* A) { const UTDHealthComponent* H = UTDStatics::GetHealth(A); return H ? H->GetShield() : -1.f; }
	inline float Hull(const AActor* A) { const UTDHealthComponent* H = UTDStatics::GetHealth(A); return H ? H->GetHull() : -1.f; }
	inline float Total(const AActor* A) { return Shield(A) + Hull(A); }

	inline int32 CountLive(UWorld* World, TFunctionRef<bool(const ATDProjectile&)> Pred)
	{
		int32 N = 0;
		if (UTDProjectileSubsystem* S = UTDProjectileSubsystem::Get(World))
		{
			for (const ATDProjectile* P : S->GetLiveProjectiles())
			{
				if (P && P->IsActive() && Pred(*P))
				{
					++N;
				}
			}
		}
		return N;
	}

	inline FTransform Facing(const FVector& Location, const FVector& Dir)
	{
		return FTransform(Dir.Rotation(), Location);
	}

	/** Ticks until Pred is true or MaxSeconds pass. Returns elapsed seconds (or -1 on timeout). */
	inline float AdvanceUntil(FTDTestWorld& W, TFunctionRef<bool()> Pred, float MaxSeconds, float Step = 1.f / 60.f)
	{
		float T = 0.f;
		while (T < MaxSeconds)
		{
			W.Tick(Step);
			T += Step;
			if (Pred())
			{
				return T;
			}
		}
		return -1.f;
	}

	inline AActor* Body(FTDTestWorld& W, ETDTeam Team, const FVector& Loc, const FVector& Extent = FVector(100.f), float Hull = 100.f, float Shield = 0.f)
	{
		AActor* A = W.Spawn<AActor>(Loc);
		UBoxComponent* Box = NewObject<UBoxComponent>(A, TEXT("Body"));
		Box->SetBoxExtent(Extent);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box->SetCollisionResponseToChannel(TDCollision::Projectile, ECR_Block);
		A->SetRootComponent(Box);
		Box->RegisterComponent();
		A->SetActorLocation(Loc);
		UTDHealthComponent* H = NewObject<UTDHealthComponent>(A, TEXT("Health"));
		H->DefaultTeam = Team;
		H->RegisterComponent();
		H->InitMaxValues(Hull, Shield, true);
		return A;
	}

	inline UTDWeaponComponent* Guns(AActor* A, FName Primary, FName Secondary = NAME_None)
	{
		UTDWeaponComponent* W = NewObject<UTDWeaponComponent>(A, TEXT("Weapons"));
		W->RegisterComponent();
		W->SetRandomSeed(77);
		W->EquipWeapon(ETDWeaponSlot::Primary, Primary);
		if (!Secondary.IsNone())
		{
			W->EquipWeapon(ETDWeaponSlot::Secondary, Secondary);
		}
		return W;
	}

		
	inline ATDProjectile* Shoot(FTDTestWorld& W, FName Weapon, const FVector& From, const FVector& Dir, AActor* Shooter, ETDTeam Team, AActor* Target = nullptr)
	{
		return ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(Weapon).Projectile, FTransform(Dir.Rotation(), From), Shooter, Team, Target);
	}

	
	
	inline FVector AtAngle(float YawDeg, float Dist)
	{
		return FVector(FMath::Cos(FMath::DegreesToRadians(YawDeg)), FMath::Sin(FMath::DegreesToRadians(YawDeg)), 0.f) * Dist;
	}

}

#endif // WITH_DEV_AUTOMATION_TESTS

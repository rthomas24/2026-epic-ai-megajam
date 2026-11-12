// Triad Drift - Powers (Teammate 2). Shared spawn helpers for the Powers test files (not a UHT header).

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDPowerTest
{
	/** Every Powers test body runs once per step (30 / 60 / 120 fps); W.Advance / W.Tick / AdvanceUntil use the current one. */
	inline constexpr float kPowerFrameSteps[] = { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f };

	inline float& PowerFrameStep()
	{
		static float Step = 1.f / 60.f;
		return Step;
	}

	struct FScopedPowerFrameStep
	{
		float Previous;
		explicit FScopedPowerFrameStep(float Step) : Previous(PowerFrameStep()) { PowerFrameStep() = Step; }
		~FScopedPowerFrameStep() { PowerFrameStep() = Previous; }
	};

	/** A box with health. bSolid = blocks everything (walls); otherwise query-only and ignores all channels (never blocks flight). */
	inline AActor* SpawnBody(FTDTestWorld& W, ETDTeam Team, const FVector& Location, const FVector& Extent = FVector(100.f),
		float Hull = 100.f, float Shield = 0.f, bool bSolid = false, bool bWithHealth = true)
	{
		AActor* Actor = W.Spawn<AActor>(Location);
		if (!Actor)
		{
			return nullptr;
		}
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor, TEXT("Body"));
		Box->SetBoxExtent(Extent);
		if (bSolid)
		{
			Box->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionResponseToAllChannels(ECR_Ignore);
			Box->SetCollisionResponseToChannel(TDCollision::Projectile, ECR_Block);
		}
		Actor->SetRootComponent(Box);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
		if (bWithHealth)
		{
			UTDHealthComponent* Health = NewObject<UTDHealthComponent>(Actor, TEXT("Health"));
			Health->DefaultTeam = Team;
			Health->RegisterComponent();
			Health->InitMaxValues(Hull, Shield, true);
		}
		return Actor;
	}

	inline AActor* SpawnWall(FTDTestWorld& W, const FVector& Location, const FVector& Extent)
	{
		return SpawnBody(W, ETDTeam::Neutral, Location, Extent, 100.f, 0.f, true, false);
	}

	/** Enemy ship with its AI removed so it holds still. */
	inline ATDEnemyShip* SpawnEnemyShip(FTDTestWorld& W, FName Id, const FVector& Location)
	{
		ATDEnemyShip* Ship = W.Spawn<ATDEnemyShip>(Location);
		if (!Ship)
		{
			return nullptr;
		}
		if (AController* C = Ship->GetController())
		{
			C->UnPossess();
			C->Destroy();
		}
		Ship->InitFromSpec(Id);
		return Ship;
	}

	template <typename T>
	T* AddPower(AActor* Owner)
	{
		T* Power = NewObject<T>(Owner);
		Power->bRequirePlayerControl = false;
		Power->RegisterComponent();
		return Power;
	}

	inline float Hull(const AActor* A) { const UTDHealthComponent* H = UTDStatics::GetHealth(A); return H ? H->GetHull() : -1.f; }
	inline float Shield(const AActor* A) { const UTDHealthComponent* H = UTDStatics::GetHealth(A); return H ? H->GetShield() : -1.f; }

	inline ATDProjectile* LaunchMissile(FTDTestWorld& W, const FVector& From, const FVector& Dir, AActor* Shooter, AActor* Target)
	{
		return ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile,
			FTransform(Dir.Rotation(), From), Shooter, ETDTeam::Enemy, Target);
	}

	inline float AdvanceUntil(FTDTestWorld& W, TFunctionRef<bool()> Pred, float MaxSeconds, float Step = -1.f)
	{
		if (Step <= 0.f)
		{
			Step = PowerFrameStep();
		}
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
}

#endif

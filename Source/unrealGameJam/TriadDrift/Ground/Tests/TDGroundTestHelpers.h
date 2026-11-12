// Triad Drift - shared helpers for Ground automation tests. Owner: Teammate 5.
// Header-only so unity builds never see two copies (no file-scope helpers in test .cpp files).

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "AIController.h"

namespace TDGroundTest
{
	/** A floor (top at Z = 0) plus an optional box, in a fresh test world. */
	inline AActor* SpawnBox(FTDTestWorld& W, const FVector& Center, const FVector& SizeCm)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		AStaticMeshActor* Box = W.Spawn<AStaticMeshActor>(Center);
		if (Box && Cube)
		{
			UStaticMeshComponent* SMC = Box->GetStaticMeshComponent();
			SMC->SetMobility(EComponentMobility::Movable);
			SMC->SetStaticMesh(Cube);
			SMC->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Box->SetActorScale3D(SizeCm / 100.f);
		}
		return Box;
	}

	inline void SpawnFloor(FTDTestWorld& W)
	{
		TDGroundTest::SpawnBox(W, FVector(0.f, 0.f, -50.f), FVector(20000.f, 20000.f, 100.f));
	}

	/** Player character standing on the floor at X, possessed by a player controller, settled. */
	inline ATDFirstPersonCharacter* SpawnPlayer(FTDTestWorld& W, const FVector& At = FVector(0.f, 0.f, 95.f))
	{
		ATDFirstPersonCharacter* C = W.Spawn<ATDFirstPersonCharacter>(At);
		if (C)
		{
			// The harness's PlayerController has no ULocalPlayer, so it isn't "locally controlled" and
			// CharacterMovement wouldn't simulate. An AIController drives the same public move API.
			AAIController* AI = W.Spawn<AAIController>(At);
			AI->Possess(C);
			if (AController* PC = C->GetController())
			{
				PC->SetControlRotation(FRotator::ZeroRotator);
			}
			W.Advance(0.3f);
		}
		return C;
	}

	inline ATDCrewDummy* SpawnDummy(FTDTestWorld& W, const FVector& At, float Health = 1000.f)
	{
		ATDCrewDummy* D = W.Spawn<ATDCrewDummy>(At, FRotator(0.f, 180.f, 0.f));
		if (D)
		{
			D->MeleeInterval = 0.f;
			D->GetHealth()->InitMaxValues(Health, 0.f, true);
			D->GetHealth()->DestroyOwnerAfterDeathSeconds = -1.f;
		}
		return D;
	}

	inline float Hull(AActor* A)
	{
		const UTDHealthComponent* H = UTDStatics::GetHealth(A);
		return H ? H->GetHull() : -1.f;
	}

	/** Never write the real save from ground tests that use the TD game instance. */
	struct FNoSaveWrites
	{
		UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
		bool bWas = S->bWriteSlotOnCheckpoint;
		FNoSaveWrites() { S->bWriteSlotOnCheckpoint = false; }
		~FNoSaveWrites() { S->bWriteSlotOnCheckpoint = bWas; }
	};
	/** Galaxy resolves to an engine map (travel can start) and there is no transition map; restored after. */
	struct FScopedFakeGalaxyMap
	{
		UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
		TMap<ETDDestination, FSoftObjectPath> Maps = S->DestinationMaps;
		FSoftObjectPath Transition = S->TransitionMap;
		FScopedFakeGalaxyMap()
		{
			S->DestinationMaps.Add(ETDDestination::Galaxy, FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry")));
			S->TransitionMap.Reset();
		}
		~FScopedFakeGalaxyMap()
		{
			S->DestinationMaps = Maps;
			S->TransitionMap = Transition;
		}
	};
}


#endif // WITH_DEV_AUTOMATION_TESTS

// Triad Drift - journey-stage cheats land in a consistent, playable world. Owner: Teammate 5.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "GameFramework/PlayerController.h"

namespace TDJourneyTest
{
	/** Galaxy sandbox with the TD game instance and a real local player; save writes off. */
	struct FGalaxy
	{
		UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
		bool bWrite = S->bWriteSlotOnCheckpoint;
		FTDTestWorld W;
		UTDGameInstance* GI = nullptr;
		APlayerController* PC = nullptr;

		FGalaxy()
			: W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass())
		{
			S->bWriteSlotOnCheckpoint = false;
			GI = Cast<UTDGameInstance>(W.GetGameInstance());
			PC = W.SpawnLocalPlayerController();
			if (ATDSpaceGameMode* GM = W.GetWorld()->GetAuthGameMode<ATDSpaceGameMode>())
			{
				GM->RestartScene(); // squad + hand-over for the controller we just made
			}
			W.Advance(0.5f);
		}
		~FGalaxy() { S->bWriteSlotOnCheckpoint = bWrite; }

		bool Stage(ETDJourneyStage St, float Settle = 6.f)
		{
			const bool bOk = FTDJourney::Go(W.GetWorld(), St);
			W.Advance(Settle, 1.f / 30.f);
			return bOk;
		}
		float DistToPoint(FName Point) const
		{
			const ATDGalaxyLayout* L = ATDGalaxyLayout::Get(W.GetWorld());
			const ATDSquadManager* Sq = ATDSquadManager::Get(W.GetWorld());
			FTransform T;
			if (!L || !Sq || !Sq->GetActiveShip() || !L->FindReturnPoint(Point, T))
			{
				return -1.f;
			}
			return (float)FVector::Dist(Sq->GetActiveShip()->GetActorLocation(), T.GetLocation());
		}
	};
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyGalaxyStages, "TriadDrift.Flow.Journey.GalaxyStagesApplyInPlace", TD_TEST_FLAGS)
bool FTDJourneyGalaxyStages::RunTest(const FString& Parameters)
{
	TDJourneyTest::FGalaxy G;
	if (!TestNotNull(TEXT("GI"), G.GI) || !TestNotNull(TEXT("director"), ATDWaveDirector::Get(G.W.GetWorld()))) { return false; }
	ATDWaveDirector* Director = ATDWaveDirector::Get(G.W.GetWorld());

	TestTrue(TEXT("Waves3to4"), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::Waves3to4));
	G.W.Tick();
	const float FromOrbit = G.DistToPoint(TDReturnPoints::VerdanceOrbit); // right away: the squad flies on at cruise
	TestTrue(FString::Printf(TEXT("squad starts at Verdance orbit (%.0f cm)"), FromOrbit), FromOrbit >= 0.f && FromOrbit < 5000.f);
	G.W.Advance(6.f, 1.f / 30.f);
	TestEqual(TEXT("phase waves 3-4"), G.GI->GetProgressRef().Phase, ETDGamePhase::Waves3to4);
	TestEqual(TEXT("Verdance rewards on the right ships"), G.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);
	TestEqual(TEXT("Cloak on Rift"), G.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::Cloak), ETDShipId::Rift);
	TestEqual(TEXT("the world runs wave 3, not wave 2"), Director->GetWaveInfo().WaveNumber, 3);

	TestTrue(TEXT("player flying a ship"), G.PC->GetPawn() && G.PC->GetPawn()->IsA<ATDShip>());

	TestTrue(TEXT("Wave5"), G.Stage(ETDJourneyStage::Wave5));
	TestEqual(TEXT("wave 5 running"), Director->GetWaveInfo().WaveNumber, 5);
	TestTrue(TEXT("EMP unlocked for Lancer"), G.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::EMPPulse) == ETDShipId::Lancer);

	TestTrue(TEXT("DominionAssault"), G.Stage(ETDJourneyStage::DominionAssault, 8.f));
	const ATDDominion* Dominion = ATDDominion::Get(G.W.GetWorld());
	TestNotNull(TEXT("Dominion exists"), Dominion);
	TestTrue(TEXT("Dominion revealed in the world"), Dominion && Dominion->GetState() != ETDDominionState::Hidden);
	TestFalse(TEXT("no leftover wave running"), Director->GetWaveInfo().bWaveActive);
	TestTrue(TEXT("Dominion flagged revealed"), G.GI->GetProgressRef().bDominionRevealed);

	TestTrue(TEXT("ReactorExposed"), G.Stage(ETDJourneyStage::ReactorExposed, 2.f));
	TestTrue(TEXT("reactor exposed in the world"), Dominion && Dominion->GetState() == ETDDominionState::ReactorExposed);

	TestTrue(TEXT("Victory"), G.Stage(ETDJourneyStage::Victory, 0.5f));
	TestEqual(TEXT("victory phase"), G.GI->GetProgressRef().Phase, ETDGamePhase::Victory);

	TestTrue(TEXT("back to Waves1to2"), G.Stage(ETDJourneyStage::Waves1to2));
	TestEqual(TEXT("wave 1 running"), Director->GetWaveInfo().WaveNumber, 1);
	TestEqual(TEXT("nothing unlocked at the start"), G.GI->GetProgressRef().UnlockedAbilities.Num(), 0);
	return true;
}

// ------------------------------------------------------------------------------------------------
namespace TDJourneyTest
{
	/** The player's view (camera if there is one, else the piloted ship's nose). */
	static void PlayerView(const FGalaxy& G, FVector& Loc, FVector& Fwd, float& HalfFov)
	{
		APawn* P = G.PC ? G.PC->GetPawn() : nullptr;
		Loc = P ? P->GetActorLocation() : FVector::ZeroVector;
		Fwd = P ? P->GetActorForwardVector() : FVector::ForwardVector;
		HalfFov = 45.f;
		if (G.PC && G.PC->PlayerCameraManager)
		{
			FRotator R;
			G.PC->PlayerCameraManager->GetCameraViewPoint(Loc, R);
			Fwd = R.Vector();
			HalfFov = G.PC->PlayerCameraManager->GetFOVAngle() * 0.5f;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyDominionFraming, "TriadDrift.Flow.Journey.DominionStagesFaceTheObjective", TD_TEST_FLAGS)
bool FTDJourneyDominionFraming::RunTest(const FString& Parameters)
{
	TDJourneyTest::FGalaxy G;
	if (!TestNotNull(TEXT("GI"), G.GI)) { return false; }

	// Assault: looking at the Dominion, the hull filling a good part of the frame.
	TestTrue(TEXT("DominionAssault"), G.Stage(ETDJourneyStage::DominionAssault, 1.f));
	const ATDDominion* Dominion = ATDDominion::Get(G.W.GetWorld());
	if (!TestNotNull(TEXT("Dominion"), Dominion)) { return false; }
	FVector Loc, Fwd; float HalfFov;
	TDJourneyTest::PlayerView(G, Loc, Fwd, HalfFov);
	const FVector ToDom = Dominion->GetActorLocation() - Loc;
	const float Dot = (float)FVector::DotProduct(Fwd, ToDom.GetSafeNormal());
	TestTrue(FString::Printf(TEXT("facing the Dominion (dot %.2f)"), Dot), Dot > 0.9f);
	const float AngleOff = FMath::RadiansToDegrees(FMath::Acos(Dot));
	TestTrue(FString::Printf(TEXT("Dominion inside the view (%.0f deg off, half fov %.0f)"), AngleOff, HalfFov), AngleOff < HalfFov);
	const float HullAngle = FMath::RadiansToDegrees(FMath::Atan(Dominion->HullExtent.X / ToDom.Size()));
	TestTrue(FString::Printf(TEXT("hull fills a good part of the frame (half-size %.0f deg)"), HullAngle), HullAngle > HalfFov * 0.35f);
	TestTrue(TEXT("outside the main shield"), ToDom.Size() > Dominion->ShieldRadius * 1.2f);
	TestTrue(TEXT("ship at the stage start, not the galaxy start"), ToDom.Size() < Dominion->ShieldRadius * 3.f);

	// Reactor: lined up with the trench, the reactor straight ahead.
	TestTrue(TEXT("ReactorExposed"), G.Stage(ETDJourneyStage::ReactorExposed, 7.f));
	const AActor* Reactor = (const AActor*)Dominion->GetReactor();
	if (!TestNotNull(TEXT("reactor"), Reactor)) { return false; }
	TDJourneyTest::PlayerView(G, Loc, Fwd, HalfFov);
	const FVector ToReactor = Reactor->GetActorLocation() - Loc;
	const float RDot = (float)FVector::DotProduct(Fwd, ToReactor.GetSafeNormal());
	TestTrue(FString::Printf(TEXT("facing the reactor (dot %.2f)"), RDot), RDot > 0.9f);
	TestTrue(TEXT("reactor inside the view"), FMath::RadiansToDegrees(FMath::Acos(RDot)) < HalfFov);
	FHitResult Hit;
	FCollisionQueryParams Q(TEXT("TDReactorSight"), true, G.PC->GetPawn());
	const bool bBlocked = G.W.GetWorld()->LineTraceSingleByChannel(Hit, Loc, Reactor->GetActorLocation(), ECC_Visibility, Q) && Hit.GetActor() != Reactor;
	TestFalse(FString::Printf(TEXT("clear line of sight to the reactor (blocked by %s)"), *GetNameSafe(Hit.GetActor())), bBlocked);
	const float Along = FMath::Abs((float)FVector::DotProduct(Fwd, Dominion->GetActorForwardVector()));
	TestTrue(FString::Printf(TEXT("lined up with the trench axis (dot %.2f)"), Along), Along > 0.95f);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyPlanetStages, "TriadDrift.Flow.Journey.PlanetStagesTravelWithTheRightReturnPoint", TD_TEST_FLAGS)
bool FTDJourneyPlanetStages::RunTest(const FString& Parameters)
{
	TDJourneyTest::FGalaxy G;
	UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
	const TMap<ETDDestination, FSoftObjectPath> Maps = S->DestinationMaps;
	const FSoftObjectPath Transition = S->TransitionMap;
	S->DestinationMaps.Add(ETDDestination::Verdance, FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry")));
	S->DestinationMaps.Add(ETDDestination::Cinder, FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry")));
	S->TransitionMap.Reset();
	FString Opened;
	G.GI->OpenLevelOverride = [&Opened](const FString& P) { Opened = P; };

	TestTrue(TEXT("VerdanceOnFoot travels"), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::VerdanceOnFoot));
	TestTrue(TEXT("travel pending"), G.GI->IsTravelPending());
	TestEqual(TEXT("lands on foot at the pad"), G.GI->GetProgressRef().ReturnPoint, FName(TEXT("Verdance_Pad")));
	TestEqual(TEXT("Verdance phase"), G.GI->GetProgressRef().Phase, ETDGamePhase::Verdance);
	TestEqual(TEXT("waves 1-2 done"), G.GI->GetProgressRef().WavesCleared, 2);
	G.GI->TickFlow(1.f);
	TestEqual(TEXT("opens the planet map"), Opened, FString(TEXT("/Engine/Maps/Entry")));
	G.GI->NotifyMapLoaded(TEXT("/Engine/Maps/Entry"));

	TestTrue(TEXT("CinderArrival travels"), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::CinderArrival));
	TestEqual(TEXT("a fresh descent (no pad return point)"), G.GI->GetProgressRef().ReturnPoint, FName(NAME_None));
	TestEqual(TEXT("Verdance rewards carried"), G.GI->GetProgressRef().UnlockedAbilities.Num(), 2);
	TestFalse(TEXT("EMP not yet"), G.GI->GetProgressRef().UnlockedAbilities.Contains(ETDAbilityId::EMPPulse));

	G.GI->OpenLevelOverride = nullptr;
	S->DestinationMaps = Maps;
	S->TransitionMap = Transition;
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyCheats, "TriadDrift.Flow.Journey.StageAndUnlockAllCheats", TD_TEST_FLAGS)
bool FTDJourneyCheats::RunTest(const FString& Parameters)
{
	TDJourneyTest::FGalaxy G;
	TestTrue(TEXT("TD.Unlock All"), UTDCheatManager::DoUnlock(G.W.GetWorld(), TEXT("All"), FString()));
	TestEqual(TEXT("three abilities"), G.GI->GetProgressRef().UnlockedAbilities.Num(), 3);
	TestEqual(TEXT("each on its owner"), G.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::EMPPulse), ETDShipId::Lancer);
	TestTrue(TEXT("TD.Stage DominionAssault"), UTDCheatManager::DoStage(G.W.GetWorld(), TEXT("DominionAssault")));
	TestEqual(TEXT("phase set"), G.GI->GetProgressRef().Phase, ETDGamePhase::DominionAssault);
	TestFalse(TEXT("bogus stage refused"), UTDCheatManager::DoStage(G.W.GetWorld(), TEXT("Narnia")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// Triad Drift - journey-stage cheats land in a consistent, playable world. Owner: Teammate 5.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/Flow/TDSaveGame.h"
#include "TriadDrift/World/Tests/TDFinaleTestHelpers.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/Flow/TDObjectiveText.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flow/Tests/TDFlowTestListener.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "EngineUtils.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"

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
	// The real hull (art or placeholder), not the placeholder box: the r8 art hull is much longer.
	const float HullAngle = FMath::RadiansToDegrees(FMath::Atan(FMath::Max(Dominion->HullExtent.X, (float)Dominion->GetLocalHullBounds().GetExtent().X) / ToDom.Size()));
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
	TestFalse(TEXT("Wave6 refused"), UTDCheatManager::DoStage(G.W.GetWorld(), TEXT("Wave6")));
	// Aliases: Wave4 = the Waves3to4 stage, running wave 4.
	TestTrue(TEXT("TD.Stage Wave4"), UTDCheatManager::DoStage(G.W.GetWorld(), TEXT("wave4")));
	G.W.Advance(6.f, 1.f / 30.f);
	TestEqual(TEXT("Wave4 alias: phase"), G.GI->GetProgressRef().Phase, ETDGamePhase::Waves3to4);
	if (const ATDWaveDirector* Director = ATDWaveDirector::Get(G.W.GetWorld()))
	{
		TestEqual(TEXT("Wave4 alias: wave 4 running"), Director->GetWaveInfo().WaveNumber, 4);
	}
	TestTrue(TEXT("TD.Stage Wave2"), UTDCheatManager::DoStage(G.W.GetWorld(), TEXT("Wave2")));
	G.W.Advance(6.f, 1.f / 30.f);
	if (const ATDWaveDirector* Director = ATDWaveDirector::Get(G.W.GetWorld()))
	{
		TestEqual(TEXT("Wave2 alias: wave 2 running"), Director->GetWaveInfo().WaveNumber, 2);
	}
	return true;
}


// ------------------------------------------------------------------------------------------------
namespace TDJourneyTest
{

	/** Real time on the flow clock and game time in the world, together. */
	static void Run(FGalaxy& G, float Seconds)
	{
		for (float T = 0.f; T < Seconds; T += 0.1f)
		{
			G.W.Advance(0.1f, 1.f / 30.f);
			G.GI->TickFlow(0.1f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyObjectiveText, "TriadDrift.Flow.Journey.ObjectiveTextForEverySpaceScene", TD_TEST_FLAGS)
bool FTDJourneyObjectiveText::RunTest(const FString& Parameters)
{
	TDJourneyTest::FGalaxy G;
	UTDEventBus* Bus = UTDEventBus::Get(G.W.GetWorld());
	if (!TestNotNull(TEXT("GI"), G.GI) || !TestNotNull(TEXT("bus"), Bus)) { return false; }
	G.GI->NotifyLevelEntered(ETDDestination::Galaxy);
	UTDFlowTestListener* Log = NewObject<UTDFlowTestListener>();
	Log->AddToRoot();
	Log->Bind(Bus);
	ON_SCOPE_EXIT { Log->RemoveFromRoot(); };
	auto LastText = [&]() { return Log->ObjectiveTexts.Num() ? Log->ObjectiveTexts.Last().ToString() : FString(TEXT("<none>")); };
	auto Expect = [&](const TCHAR* What, const TCHAR* Contains)
	{
		TestTrue(FString::Printf(TEXT("%s: '%s' contains '%s'"), What, *LastText(), Contains), LastText().Contains(Contains));
		TestTrue(FString::Printf(TEXT("%s: GI remembers it"), What), G.GI->GetCurrentObjectiveText().ToString() == LastText());
	};

	TestTrue(TEXT("Waves1to2"), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::Waves1to2));
	TDJourneyTest::Run(G, 2.f);
	Expect(TEXT("wave 1"), TEXT("WAVE 1"));
	auto Tagged = [&]()
	{
		int32 N = 0;
		for (TActorIterator<AActor> It(G.W.GetWorld()); It; ++It) { if (It->ActorHasTag(TDTags::ObjectiveMarker)) { ++N; } }
		return N;
	};
	TestEqual(TEXT("markers: nothing tagged during a wave"), Tagged(), 0);
	TestTrue(TEXT("wave line names the enemy"), LastText().Contains(TEXT("NULL ARMADA")));

	// The line is broadcast on change, not every tick.
	const int32 Before = Log->ObjectiveTexts.Num();
	TDJourneyTest::Run(G, 2.f);
	TestEqual(TEXT("no spam while nothing changes"), Log->ObjectiveTexts.Num(), Before);

	// CUT! -> the new take re-announces the scene's line (a HUD rebuilt for the take is never empty).
	// The cut and the new take also go through the bus (music / mix state listens there).
	const int32 CutsBefore = Log->Cuts.Num(), TakesBefore = Log->Takes.Num();
	G.GI->ReportPlayerDefeated();
	TDJourneyTest::Run(G, 3.f);
	TestEqual(TEXT("CUT! on the bus"), Log->Cuts.Num(), CutsBefore + 1);
	TestTrue(TEXT("new take on the bus"), Log->Takes.Num() > TakesBefore);
	TestTrue(TEXT("re-broadcast after the retake"), Log->ObjectiveTexts.Num() > Before);
	Expect(TEXT("after the retake"), TEXT("WAVE 1"));

	// Level entry re-announces even when nothing changed.
	const int32 BeforeEntry = Log->ObjectiveTexts.Num();
	const int32 PhasesBefore = Log->Phases.Num();
	G.GI->NotifyLevelEntered(ETDDestination::Galaxy);
	TDJourneyTest::Run(G, 1.f);
	TestTrue(TEXT("re-broadcast after level entry"), Log->ObjectiveTexts.Num() > BeforeEntry);
	TestTrue(TEXT("phase re-broadcast on level entry"), Log->Phases.Num() > PhasesBefore);

	// Travel beat: Verdance phase in the galaxy.
	G.GI->GetMutableProgress().Phase = ETDGamePhase::Verdance;
	TDJourneyTest::Run(G, 0.5f);
	Expect(TEXT("travel"), TEXT("JUMP TO VERDANCE"));
	{
		const TArray<AActor*> M = G.GI->GetFlowMarkers();
		const ATDPlanetAtmosphere* Atmo = M.Num() == 1 ? Cast<ATDPlanetAtmosphere>(M[0]) : nullptr;
		TestTrue(TEXT("markers: the Verdance atmosphere is the objective"), Atmo && Atmo->GetDestination() == ETDDestination::Verdance && Atmo->ActorHasTag(TDTags::ObjectiveMarker));
		TestEqual(TEXT("markers: only it"), Tagged(), 1);
	}

	TestTrue(TEXT("Waves3to4"), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::Waves3to4));
	TDJourneyTest::Run(G, 2.f);
	Expect(TEXT("wave 3"), TEXT("WAVE 3"));
	TestEqual(TEXT("markers: the planet loses its tag when the wave starts"), Tagged(), 0);

	TestTrue(TEXT("DominionAssault"), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::DominionAssault));
	TDJourneyTest::Run(G, 9.f); // past the warp-in
	Expect(TEXT("assault"), TEXT("SHIELD GENERATORS  0 / 4"));
	ATDDominion* Dominion = ATDDominion::Get(G.W.GetWorld());
	if (!TestNotNull(TEXT("Dominion"), Dominion) || !TestTrue(TEXT("generators"), Dominion->GetGenerators().Num() > 0)) { return false; }
	TestEqual(TEXT("markers: every generator"), Tagged(), Dominion->GetGenerators().Num());
	TestTrue(TEXT("markers: a generator carries the tag"), Dominion->GetGenerators()[0]->ActorHasTag(TDTags::ObjectiveMarker));
	UTDHealthComponent* GenHealth = Dominion->GetGenerators()[0]->FindComponentByClass<UTDHealthComponent>();
	if (TestNotNull(TEXT("generator health"), GenHealth))
	{
		GenHealth->Kill(G.PC->GetPawn(), ETDDamageKind::Explosive);
		TDJourneyTest::Run(G, 1.f);
		Expect(TEXT("one generator down"), TEXT("1 / 4"));
		TestFalse(TEXT("markers: the dead generator loses its tag"), Dominion->GetGenerators()[0]->ActorHasTag(TDTags::ObjectiveMarker));
		TestEqual(TEXT("markers: the live ones keep it"), Tagged(), Dominion->GetGenerators().Num() - 1);
		TestEqual(TEXT("step = generators down"), Log->ObjectiveSteps.Last().Value, 1);
	}

	TestTrue(TEXT("ReactorExposed"), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::ReactorExposed));
	TDJourneyTest::Run(G, 2.f);
	Expect(TEXT("reactor"), TEXT("DESTROY THE REACTOR"));
	TestTrue(TEXT("markers: the reactor"), Dominion->GetReactor() && ((AActor*)Dominion->GetReactor())->ActorHasTag(TDTags::ObjectiveMarker));
	TestEqual(TEXT("markers: only the reactor"), Tagged(), 1);

	TestTrue(TEXT("Victory"), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::Victory));
	TDJourneyTest::Run(G, 0.5f);
	Expect(TEXT("victory"), TEXT("VICTORY"));
	TestEqual(TEXT("markers: none at victory"), Tagged(), 0);

	// Planets (computed for the planet destination; on foot the objective actors own the line).
	FTDObjectiveLine L;
	FTDProgress P = FTDJourney::MakeProgress(ETDJourneyStage::VerdanceArrival, FTDProgress());
	TestTrue(TEXT("planet arrival has a line"), FTDObjectiveText::Compute(G.W.GetWorld(), P, ETDDestination::Verdance, L));
	TestTrue(TEXT("land at the beacon"), L.Text.ToString().Contains(TEXT("LAND")));
	P = FTDJourney::MakeProgress(ETDJourneyStage::Waves3to4, FTDProgress());
	TestTrue(TEXT("objectives done on the planet"), FTDObjectiveText::Compute(G.W.GetWorld(), P, ETDDestination::Verdance, L));
	TestTrue(TEXT("take off"), L.Text.ToString().Contains(TEXT("TAKE OFF")));
	TestFalse(TEXT("main menu stays silent"), FTDObjectiveText::Compute(G.W.GetWorld(), P, ETDDestination::MainMenu, L));
	return true;
}


// ------------------------------------------------------------------------------------------------
// Enemies review: the last enemy dies inside the 1.75 s CUT delay. The flow voids that clear (the scene is
// about to restart); the retake must then rebuild the wave, so the run can never be left with no enemies
// and no phase change. Real GI, real wave director, real kills, in exactly that order.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyClearDuringCut, "TriadDrift.Flow.Journey.LastKillDuringCutNeverSoftLocks", TD_TEST_FLAGS)
bool FTDJourneyClearDuringCut::RunTest(const FString& Parameters)
{
	TDJourneyTest::FGalaxy G;
	ATDWaveDirector* Director = ATDWaveDirector::Get(G.W.GetWorld());
	if (!TestNotNull(TEXT("GI"), G.GI) || !TestNotNull(TEXT("director"), Director)) { return false; }
	for (const int32 Wave : { 2, 4, 5 })
	{
		const FString W = FString::Printf(TEXT("Wave%d"), Wave);
		TestTrue(*(TEXT("stage ") + W), UTDCheatManager::DoStage(G.W.GetWorld(), W));
		TDJourneyTest::Run(G, 6.f);
		const ETDGamePhase PhaseBefore = G.GI->GetProgressRef().Phase;
		if (!TestTrue(*(W + TEXT(" running with enemies")), Director->GetWaveInfo().bWaveActive && Director->GetWaveInfo().EnemiesAlive > 0)) { continue; }

		G.GI->ReportPlayerDefeated();                 // CUT!
		TDJourneyTest::Run(G, 0.3f);
		UTDCheatManager::DoKillAll(G.W.GetWorld());    // the last enemies die inside the delay
		TDJourneyTest::Run(G, 0.3f);
		TestTrue(*(W + TEXT(": CUT still pending")), G.GI->IsRetakePending());
		TDJourneyTest::Run(G, 6.f);                    // retake, rebuild, breather

		const FTDWaveInfo Info = Director->GetWaveInfo();
		const bool bRebuilt = Info.bWaveActive && Info.WaveNumber == Wave && Info.EnemiesAlive > 0;
		const bool bAdvanced = G.GI->GetProgressRef().WavesCleared >= Wave && G.GI->GetProgressRef().Phase != PhaseBefore;
		TestTrue(FString::Printf(TEXT("%s: retake rebuilds the wave or the clear stands (active %d, wave %d, alive %d, cleared %d)"),
			*W, Info.bWaveActive, Info.WaveNumber, Info.EnemiesAlive, G.GI->GetProgressRef().WavesCleared), bRebuilt || bAdvanced);
	}
	return true;
}


// ------------------------------------------------------------------------------------------------
// World review MAJOR 1 / 3, MINOR 4 / 5: the finale resumes sensibly. Real GI, space game mode, galaxy
// layout, squad and Dominion.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyFinaleResumes, "TriadDrift.Flow.Journey.FinaleWipesRespawnOnTheApproachAndVictoryWins", TD_TEST_FLAGS)
bool FTDJourneyFinaleResumes::RunTest(const FString& Parameters)
{
	TDJourneyTest::FGalaxy G;
	UTDEventBus* Bus = UTDEventBus::Get(G.W.GetWorld());
	if (!TestNotNull(TEXT("GI"), G.GI) || !TestNotNull(TEXT("bus"), Bus)) { return false; }
	G.GI->NotifyLevelEntered(ETDDestination::Galaxy);
	UTDFlowTestListener* Log = NewObject<UTDFlowTestListener>();
	Log->AddToRoot();
	Log->Bind(Bus);
	ON_SCOPE_EXIT { Log->RemoveFromRoot(); };

	TestTrue(TEXT("DominionAssault"), G.Stage(ETDJourneyStage::DominionAssault, 1.f));
	TestEqual(TEXT("the assault is a scene"), G.GI->GetCurrentScene(), UTDGameInstance::DominionScene);
	TestEqual(TEXT("take 1"), G.GI->GetProgressRef().Takes.FindRef(UTDGameInstance::DominionScene), 1);
	const ATDDominion* Dominion = ATDDominion::Get(G.W.GetWorld());
	ATDSquadManager* Squad = ATDSquadManager::Get(G.W.GetWorld());
	if (!TestNotNull(TEXT("Dominion"), Dominion) || !TestNotNull(TEXT("squad"), Squad)) { return false; }
	FTransform Approach, Trench;
	TestTrue(TEXT("approach resolves"), FTDJourney::FindStageStart(G.W.GetWorld(), FTDJourney::DominionApproach, Approach));
	TestTrue(TEXT("trench resolves"), FTDJourney::FindStageStart(G.W.GetWorld(), FTDJourney::DominionTrench, Trench));

	// Wipe far from the Dominion: the retake brings the squad back to the approach, as take 2 of "Dominion".
	if (APawn* Ship = Squad->GetActiveShip()) { Ship->SetActorLocation(Ship->GetActorLocation() + FVector(600000.f, 0.f, 0.f)); }
	const int32 TakesBefore = Log->Takes.Num();
	G.GI->ReportPlayerDefeated();
	// MINOR 4: a Dominion-state report inside the CUT delay is void.
	G.GI->SetDominionState(true, true);
	TestFalse(TEXT("Dominion state ignored while a CUT is pending"), G.GI->GetProgressRef().bDominionDefensesDown);
	TDJourneyTest::Run(G, UTDFlowSettings::Get()->RetakeDelaySeconds + 1.f);
	TestEqual(TEXT("take 2 of the Dominion"), G.GI->GetProgressRef().Takes.FindRef(UTDGameInstance::DominionScene), 2);
	TestTrue(TEXT("OnTakeStarted(Dominion, 2) for T4's reset"), Log->Takes.Num() > TakesBefore && Log->Takes.Last().Key == UTDGameInstance::DominionScene && Log->Takes.Last().Value == 2);
	const float FromApproach = Squad->GetActiveShip() ? (float)FVector::Dist(Squad->GetActiveShip()->GetActorLocation(), Approach.GetLocation()) : -1.f;
	TestTrue(FString::Printf(TEXT("respawned on the approach (%.0f cm)"), FromApproach), FromApproach >= 0.f && FromApproach < 6000.f);

	// Defences down: a wipe respawns lined up with the trench.
	G.GI->SetDominionState(true, true);
	G.GI->ReportPlayerDefeated();
	TDJourneyTest::Run(G, UTDFlowSettings::Get()->RetakeDelaySeconds + 1.f);
	const float FromTrench = Squad->GetActiveShip() ? (float)FVector::Dist(Squad->GetActiveShip()->GetActorLocation(), Trench.GetLocation()) : -1.f;
	TestTrue(FString::Printf(TEXT("defences down -> respawned at the trench (%.0f cm)"), FromTrench), FromTrench >= 0.f && FromTrench < 6000.f);
	TestEqual(TEXT("take 3"), G.GI->GetProgressRef().Takes.FindRef(UTDGameInstance::DominionScene), 3);

	// MINOR 5: the reactor dies from a shot in flight after the wipe: victory wins over the pending CUT.
	G.GI->ReportPlayerDefeated();
	TestTrue(TEXT("CUT pending"), G.GI->IsRetakePending());
	G.GI->SetPhase(ETDGamePhase::Victory);
	TestFalse(TEXT("victory cancels the CUT"), G.GI->IsRetakePending());
	TDJourneyTest::Run(G, UTDFlowSettings::Get()->RetakeDelaySeconds + 1.f);
	TestEqual(TEXT("still victory, no restart"), G.GI->GetProgressRef().Phase, ETDGamePhase::Victory);
	TestEqual(TEXT("no take 4"), G.GI->GetProgressRef().Takes.FindRef(UTDGameInstance::DominionScene), 3);

	// MAJOR 3: a won save never continues into the fight.
	G.GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationFinale"));
	G.GI->TakeCheckpoint(TEXT("Manual"));
	TestTrue(TEXT("won run saved"), G.GI->WriteSaveSlot(true));
	TestFalse(TEXT("no Continue for a won save"), G.GI->HasContinue());
	TestFalse(TEXT("ContinueRun refuses it"), G.GI->ContinueRun());
	G.GI->DeleteSave();
	G.GI->SetSaveSlotOverride(FString());
	return true;
}


// ------------------------------------------------------------------------------------------------
// The finale end to end (with T4's real-weapons helper): the real GI and space game mode, a real squad
// firing real weapons, generators, a wipe in the middle, a Continue with defences down, three reactor
// phases, victory; at 30 / 60 / 120 fps.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyFinaleEndToEnd, "TriadDrift.Flow.Journey.FinaleFoughtEndToEndWithWipeAndContinue", TD_TEST_FLAGS)
bool FTDJourneyFinaleEndToEnd::RunTest(const FString& Parameters)
{
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		TDJourneyTest::FGalaxy G;
		if (!TestNotNull(TEXT("GI"), G.GI)) { return false; }
		G.GI->NotifyLevelEntered(ETDDestination::Galaxy);
		G.GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationFinaleE2E"));
		G.S->bWriteSlotOnCheckpoint = true; // like the game: victory rewrites the slot (restored by FGalaxy)
		ON_SCOPE_EXIT { G.GI->DeleteSave(); G.GI->SetSaveSlotOverride(FString()); };
		auto Run = [&G, Fps](float Seconds) { for (float T = 0.f; T < Seconds; T += 1.f / Fps) { G.W.Tick(1.f / Fps); G.GI->TickFlow(1.f / Fps); } };
		const FString At = FString::Printf(TEXT("%.0f fps: "), Fps);

		TestTrue(*(At + TEXT("assault stage")), FTDJourney::Go(G.W.GetWorld(), ETDJourneyStage::DominionAssault));
		Run(8.f); // warp-in
		ATDDominion* Dom = ATDDominion::Get(G.W.GetWorld());
		ATDSquadManager* Squad = ATDSquadManager::Get(G.W.GetWorld());
		if (!TestNotNull(*(At + TEXT("Dominion")), Dom) || !TestNotNull(*(At + TEXT("squad")), Squad)) { return false; }
		TestEqual(*(At + TEXT("the assault is the Dominion scene")), G.GI->GetCurrentScene(), UTDGameInstance::DominionScene);

		// Two generators with the real weapons.
		TestTrue(*(At + TEXT("two generators down")), TDFinaleCombat::DestroyGenerators(*this, G.W, Dom, 2, Fps));
		Run(0.5f);
		TestTrue(*(At + FString::Printf(TEXT("objective line counts them ('%s')"), *G.GI->GetCurrentObjectiveText().ToString())), G.GI->GetCurrentObjectiveText().ToString().Contains(TEXT("2 / 4")));

		// A wipe in the middle: every ship dies.
		for (ATDShip* Ship : Squad->GetLivingShips()) { if (UTDHealthComponent* H = UTDStatics::GetHealth(Ship)) { H->Kill(nullptr, ETDDamageKind::Explosive); } }
		Run(UTDFlowSettings::Get()->RetakeDelaySeconds + 1.5f);
		TestEqual(*(At + TEXT("take 2 of the Dominion")), G.GI->GetProgressRef().Takes.FindRef(UTDGameInstance::DominionScene), 2);
		FTransform Approach;
		FTDJourney::FindStageStart(G.W.GetWorld(), FTDJourney::DominionApproach, Approach);
		const ATDShip* Active = TDFinaleCombat::ActiveShip(G.W);
		TestTrue(*(At + TEXT("respawned on the approach")), Active && FVector::Dist(Active->GetActorLocation(), Approach.GetLocation()) < 8000.f);
		TestEqual(*(At + TEXT("destroyed generators stay destroyed")), Dom->GetGeneratorsRemaining(), 2);

		// The last two: defences down, the trench.
		TestTrue(*(At + TEXT("the last generators down")), TDFinaleCombat::DestroyGenerators(*this, G.W, Dom, 2, Fps));
		Run(1.f);
		TestTrue(*(At + TEXT("defences down in progress")), G.GI->GetProgressRef().bDominionDefensesDown);

		// Continue with defences down: the save carries it, a restart puts the squad at the trench.
		G.GI->TakeCheckpoint(TEXT("Manual"));
		TestTrue(*(At + TEXT("saved")), G.GI->WriteSaveSlot(true));
		TestTrue(*(At + TEXT("Continue offered mid-fight")), G.GI->HasContinue());
		UTDSaveGame* Save = nullptr;
		TestTrue(*(At + TEXT("read")), G.GI->ReadSaveSlot(Save) && Save && G.GI->ApplySave(*Save));
		if (ATDSpaceGameMode* GM = G.W.GetWorld()->GetAuthGameMode<ATDSpaceGameMode>()) { GM->RestartScene(); }
		Run(1.f);
		FTransform Trench;
		FTDJourney::FindStageStart(G.W.GetWorld(), FTDJourney::DominionTrench, Trench);
		Active = TDFinaleCombat::ActiveShip(G.W);
		TestTrue(*(At + TEXT("Continue starts at the trench")), Active && FVector::Dist(Active->GetActorLocation(), Trench.GetLocation()) < 8000.f);

		// The reactor through its three phases, then victory.
		TestTrue(*(At + TEXT("reactor phases")), TDFinaleCombat::DestroyReactorPhases(*this, G.W, Dom, 3, Fps));
		Run(15.f); // the breakup plays, then victory
		TestEqual(*(At + TEXT("victory")), G.GI->GetProgressRef().Phase, ETDGamePhase::Victory);
		TestFalse(*(At + TEXT("no Continue into a won fight")), G.GI->HasContinue());
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
// ENG-33 (T3 for T5): New Game plays the intro once; the first waves wait for its OnFinished and then start
// exactly as without it. A second entry (a retake, Continue) never plays it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyIntroDefers, "TriadDrift.Flow.NewGameIntroDefersFirstWave", TD_TEST_FLAGS)
bool FTDJourneyIntroDefers::RunTest(const FString& Parameters)
{
	TDJourneyTest::FGalaxy G;
	ATDSpaceGameMode* GM = G.W.GetWorld()->GetAuthGameMode<ATDSpaceGameMode>();
	UTDIntroDirector* Intro = UTDIntroDirector::Get(G.W.GetWorld());
	if (!TestNotNull(TEXT("GI"), G.GI) || !TestNotNull(TEXT("space mode"), GM) || !TestNotNull(TEXT("intro"), Intro))
	{
		return false;
	}
	struct FAllow
	{
		UTDIntroDirector* I;
		~FAllow() { I->SetAllowWithoutViewportForTests(false); }
	} Allow{ Intro };
	Intro->SetAllowWithoutViewportForTests(true); // the test world has no game viewport

	// Continue / a loaded run never carries the intro.
	G.GI->SetSaveSlotOverride(TEXT("TD_Test_IntroNoSave")); // an empty slot: Continue fails without travelling
	G.GI->StartNewRun();
	TestTrue(TEXT("New Game: the intro is pending"), G.GI->IsIntroPending());
	TestFalse(TEXT("nothing to continue"), G.GI->ContinueRun()); // it fails, but the intro is cleared first
	TestFalse(TEXT("Continue clears it"), G.GI->IsIntroPending());

	// New Game, then the galaxy's StartPlay.
	G.GI->StartNewRun();
	GM->GetWorldTimerManager().ClearAllTimersForObject(GM);
	G.GI->SetRunTimerPaused(false); // a run in the galaxy: the timer runs (the test world never travelled)
	GM->EnterLevelForTests();
	const bool bTimerPausedWithoutIntro = false;
	TestTrue(TEXT("the intro plays"), Intro->IsPlaying());
	TestFalse(TEXT("consumed"), G.GI->IsIntroPending());
	TestEqual(TEXT("still Intro while it plays"), G.GI->GetProgressRef().Phase, ETDGamePhase::Intro);
	TestFalse(TEXT("no first-wave timer while it plays"), GM->IsFirstWaveTimerArmed());
	const float RunAtStart = G.GI->GetProgressRef().RunTimeSeconds;
	G.W.Advance(6.f, 1.f / 30.f);
	TestTrue(TEXT("6 s in: still no waves"), G.GI->GetProgressRef().Phase == ETDGamePhase::Intro && !GM->IsFirstWaveTimerArmed());
	G.GI->AdvanceRunTime(6.f);
	TestEqual(TEXT("the intro is not run time (watched or skipped, the same)"), G.GI->GetProgressRef().RunTimeSeconds, RunAtStart);

	// Skipped (or watched to the end): the same phase change and timer as a run without an intro.
	TestTrue(TEXT("skip"), Intro->Skip());
	TestEqual(TEXT("the timer is back exactly as it was"), G.GI->IsRunTimerPaused(), bTimerPausedWithoutIntro);
	G.GI->AdvanceRunTime(1.f);
	TestNearlyEqual(TEXT("the run time starts with the first waves"), G.GI->GetProgressRef().RunTimeSeconds, RunAtStart + 1.f, 0.001f);
	TestEqual(TEXT("Waves1to2 after OnFinished"), G.GI->GetProgressRef().Phase, ETDGamePhase::Waves1to2);
	TestTrue(TEXT("first-wave timer armed after OnFinished"), GM->IsFirstWaveTimerArmed());

	// A second entry (a retake reloading the level, or Continue) never plays it again.
	GM->GetWorldTimerManager().ClearAllTimersForObject(GM);
	GM->EnterLevelForTests();
	TestFalse(TEXT("no second intro"), Intro->IsPlaying());
	TestTrue(TEXT("waves armed straight away"), GM->IsFirstWaveTimerArmed());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

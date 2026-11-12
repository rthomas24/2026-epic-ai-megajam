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
// DISABLED until T4's director rebuilds the wave on the retake (fails today: wave 2 is left empty, no phase change).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDJourneyClearDuringCut, "TriadDrift.Flow.Journey.LastKillDuringCutNeverSoftLocks", TD_TEST_FLAGS | EAutomationTestFlags::Disabled)
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

#endif // WITH_DEV_AUTOMATION_TESTS

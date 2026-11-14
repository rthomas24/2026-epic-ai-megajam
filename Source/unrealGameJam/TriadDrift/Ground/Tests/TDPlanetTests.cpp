// Triad Drift - planet arrival and hand-off tests (ENG-20 ground half). Owner: Teammate 5.
// Real path: T1's rig, landing component and flight zone; T3's squad; our director and character.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Planets/TDPlanetDirector.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace TDPlanetTest
{
	/** A Verdance visit in a test world with the TD game instance, a player controller and the director. */
	struct FVisit
	{
		TDGroundTest::FNoSaveWrites NoWrites;
		FTDTestWorld W;
		UTDGameInstance* GI = nullptr;
		APlayerController* PC = nullptr;
		ATDPlanetDirector* Director = nullptr;

		explicit FVisit(FName PresetReturnPoint = NAME_None, FName Planet = TEXT("Verdance"), bool bCourse = true, const TArray<FName>& AlreadyDone = TArray<FName>(), bool bLocalController = false)
			: W(UTDGameInstance::StaticClass())
		{
			const bool bCinder = Planet == TEXT("Cinder");
			GI = Cast<UTDGameInstance>(W.GetGameInstance());
			// A local space controller (real Enhanced Input, so tests can press real keys) or a plain one.
			PC = bLocalController ? W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()) : W.SpawnPlayerController();
			if (GI)
			{
				GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationTest"));
				GI->NotifyLevelEntered(bCinder ? ETDDestination::Cinder : ETDDestination::Verdance);
				GI->GetMutableProgress().ReturnPoint = PresetReturnPoint;
				for (const FName& Id : AlreadyDone) { GI->GetMutableProgress().CompletedObjectives.Add(Id); } // a reload mid-planet
			}
			const FTransform Pad(FRotator::ZeroRotator, FVector(0.f, 0.f, 0.f));
			Director = W.GetWorld()->SpawnActorDeferred<ATDPlanetDirector>(ATDPlanetDirector::StaticClass(), Pad);
			Director->PlanetId = Planet;
			Director->PadTransform = Pad;
			Director->bBuildCourse = bCourse;
			Director->RequiredObjectives = bCinder ? TArray<FName>{ TEXT("Obj_Outpost"), TEXT("Obj_Jammer") } : TArray<FName>{ TEXT("Obj_WreckCore"), TEXT("Obj_Relay") };
			Director->FinishSpawning(Pad);
			W.Advance(0.2f);
		}

		ATDShip* Ship() const { return Director ? Director->GetPlayerShip() : nullptr; }

		/** Real landing: T1's auto-land onto the rig's player pad, until the hand-off fires. */
		bool LandForReal(float TimeoutSeconds = 12.f)
		{
			ATDShip* S = Ship();
			if (!S || !S->GetLanding() || !Director->GetRig())
			{
				return false;
			}
			if (!S->GetLanding()->LandOnPad(Director->GetRig()->GetPlayerPad()))
			{
				return false;
			}
			for (float T = 0.f; T < TimeoutSeconds && !Director->IsOnFoot(); T += 1.f / 30.f)
			{
				W.Tick(1.f / 30.f);
			}
			return Director->IsOnFoot();
		}
	};
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlanetArrival, "TriadDrift.Ground.Planet.FreshArrivalFliesInAtDescentStart", TD_TEST_FLAGS)
bool FTDPlanetArrival::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	if (!TestNotNull(TEXT("director"), V.Director) || !TestNotNull(TEXT("rig"), V.Director->GetRig())) { return false; }
	ATDShip* Ship = V.Ship();
	if (!TestNotNull(TEXT("squad spawned"), Ship)) { return false; }
	TestTrue(TEXT("player flies the lead ship"), V.PC->GetPawn() == (APawn*)Ship);
	TestTrue(TEXT("player-controlled"), Ship->IsPlayerFlown());
	TestFalse(TEXT("not landed"), Ship->GetLanding()->IsLanded());
	TestFalse(TEXT("no pilot on foot yet"), V.Director->IsOnFoot());
	TestNull(TEXT("no first-person character on arrival"), V.Director->GetFirstPerson());
	const FTransform Descent = V.Director->GetRig()->GetDescentStart();
	TestTrue(TEXT("squad starts at the rig's descent start"), FVector::Dist(Ship->GetActorLocation(), Descent.GetLocation()) < 5000.f);
	TestTrue(TEXT("high above the pad"), Ship->GetActorLocation().Z > 10000.f);
	TestEqual(TEXT("three ships"), ATDSquadManager::Get(V.W.GetWorld())->GetLivingShips().Num(), 3);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlanetLandHandOff, "TriadDrift.Ground.Planet.LandingHandsOverToFirstPerson", TD_TEST_FLAGS)
bool FTDPlanetLandHandOff::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	ATDShip* Ship = V.Ship();
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	TestTrue(TEXT("auto-land completes and hands over"), V.LandForReal());
	ATDFirstPersonCharacter* FP = V.Director->GetFirstPerson();
	if (!TestNotNull(TEXT("first-person character"), FP)) { return false; }
	TestTrue(TEXT("player possesses the character"), V.PC->GetPawn() == (APawn*)FP);
	TestTrue(TEXT("ship landed"), Ship->GetLanding()->IsLanded());
	TestFalse(TEXT("ship no longer player-controlled"), Ship->IsPlayerFlown());
	const FTransform Exit = Ship->GetLanding()->GetExitTransform();
	TestTrue(TEXT("pilot stands at the ship's exit point"), FVector::Dist(FP->GetActorLocation(), Exit.GetLocation()) < 120.f);
	TestTrue(FString::Printf(TEXT("pilot faces the course, not the ship (yaw %.0f)"), V.PC->GetControlRotation().Yaw), FMath::Abs(FRotator::NormalizeAxis(V.PC->GetControlRotation().Yaw - V.Director->PadTransform.Rotator().Yaw)) < 10.f);
	TestEqual(TEXT("ReturnPoint is the pad"), V.GI->GetProgressRef().ReturnPoint, FName(TEXT("Verdance_Pad")));
	TestEqual(TEXT("checkpoint taken on foot"), V.GI->GetLastCheckpoint().Progress.ReturnPoint, FName(TEXT("Verdance_Pad")));
	TestNotNull(TEXT("board-ship prompt"), V.Director->GetBoardPoint());
	TestTrue(TEXT("board prompt usable"), V.Director->GetBoardPoint() && V.Director->GetBoardPoint()->CanInteract(FP));

	// Both objectives done on foot: the take-off point is the objective marker and the line says so.
	V.GI->CompleteObjective(TEXT("Obj_WreckCore"), ETDAbilityId::GravityWell, ETDShipId::Bastion);
	V.GI->CompleteObjective(TEXT("Obj_Relay"), ETDAbilityId::Cloak, ETDShipId::Rift);
	V.GI->RefreshObjective(false);
	TestTrue(TEXT("board point tagged as the objective"), V.Director->GetBoardPoint() && V.Director->GetBoardPoint()->ActorHasTag(TDTags::ObjectiveMarker));
	TestTrue(FString::Printf(TEXT("character reports the line ('%s')"), *FP->GetObjectiveText().ToString()), FP->GetObjectiveText().ToString().Contains(TEXT("RETURN TO YOUR SHIP")));
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlanetReboard, "TriadDrift.Ground.Planet.ReboardTakesOff", TD_TEST_FLAGS)
bool FTDPlanetReboard::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	ATDShip* Ship = V.Ship();
	if (!TestNotNull(TEXT("ship"), Ship) || !TestTrue(TEXT("landed"), V.LandForReal())) { return false; }
	ATDFirstPersonCharacter* FP = V.Director->GetFirstPerson();
	ATDBoardShipInteractable* Board = V.Director->GetBoardPoint();
	if (!TestNotNull(TEXT("board point"), Board)) { return false; }
	Board->Interact(FP); // the prompt the player uses
	TestFalse(TEXT("off foot"), V.Director->IsOnFoot());
	TestTrue(TEXT("player possesses the ship again"), V.PC->GetPawn() == (APawn*)Ship);
	TestTrue(TEXT("player-controlled again"), Ship->IsPlayerFlown());
	TestTrue(TEXT("character put away"), FP->IsHidden() && FP->GetBlade()->IsRetracted());
	TestFalse(TEXT("board prompt off"), Board->CanInteract(FP));
	V.W.Advance(5.f);
	TestEqual(TEXT("took off and flying"), Ship->GetLanding()->GetLandingState(), ETDLandingState::Flying);
	TestTrue(TEXT("lifted off the pad"), Ship->GetActorLocation().Z > V.Director->GetRig()->GetPlayerPad()->GetActorLocation().Z + 300.f);

	// And down again: the same character comes back.
	TestTrue(TEXT("lands again"), V.LandForReal());
	TestTrue(TEXT("same pilot reused"), V.Director->GetFirstPerson() == FP);
	TestFalse(TEXT("visible again"), FP->IsHidden());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlanetRetakeOnFoot, "TriadDrift.Ground.Planet.RetakeAfterLandingStaysOnFoot", TD_TEST_FLAGS)
bool FTDPlanetRetakeOnFoot::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	ATDShip* Ship = V.Ship();
	if (!TestNotNull(TEXT("ship"), Ship) || !TestTrue(TEXT("landed"), V.LandForReal())) { return false; }
	ATDFirstPersonCharacter* FP = V.Director->GetFirstPerson();
	FP->SetActorLocation(FP->GetActorLocation() + FVector(1500.f, 0.f, 0.f)); // walked off
	FP->GetHealth()->Kill(nullptr, ETDDamageKind::Environmental);
	TestTrue(TEXT("CUT pending"), V.GI->IsRetakePending());
	V.GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);
	V.W.Advance(0.2f);
	TestFalse(TEXT("pilot alive again"), FP->IsDead());
	TestTrue(TEXT("still the player's pawn"), V.PC->GetPawn() == (APawn*)FP);
	TestTrue(TEXT("back at the pad"), FVector::Dist(FP->GetActorLocation(), Ship->GetLanding()->GetExitTransform().GetLocation()) < 150.f);
	TestTrue(TEXT("ship still parked"), Ship->GetLanding()->IsLanded());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlanetLandedStart, "TriadDrift.Ground.Planet.PadCheckpointStartsLandedOnFoot", TD_TEST_FLAGS)
bool FTDPlanetLandedStart::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(TEXT("Verdance_Pad")); // as loaded from a checkpoint taken on foot
	ATDShip* Ship = V.Ship();
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	TestTrue(TEXT("player ship snapped landed"), Ship->GetLanding()->IsLanded());
	TestTrue(TEXT("on the player pad"), Ship->GetLanding()->GetPad() == V.Director->GetRig()->GetPlayerPad());
	int32 WingmenLanded = 0;
	for (ATDShip* S : ATDSquadManager::Get(V.W.GetWorld())->GetLivingShips())
	{
		WingmenLanded += (S != Ship && S->GetLanding() && S->GetLanding()->IsLanded()) ? 1 : 0;
	}
	TestEqual(TEXT("both wingmen on their side pads"), WingmenLanded, 2);
	TestTrue(TEXT("pilot on foot"), V.Director->IsOnFoot());
	TestTrue(TEXT("player is the character"), V.PC->GetPawn() == (APawn*)V.Director->GetFirstPerson());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlanetExit, "TriadDrift.Ground.Planet.ObjectivesOpenTheCeilingToOrbit", TD_TEST_FLAGS)
bool FTDPlanetExit::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	ATDShip* Ship = V.Ship();
	ATDAtmosphereFlightZone* Zone = V.Director->GetRig() ? V.Director->GetRig()->GetZone() : nullptr;
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("zone"), Zone)) { return false; }
	TestFalse(TEXT("ceiling closed before the objectives"), Zone->IsExitAllowed());
	V.GI->CompleteObjective(TEXT("Obj_WreckCore"), ETDAbilityId::GravityWell, ETDShipId::Bastion);
	TestFalse(TEXT("still closed with one of two"), Zone->IsExitAllowed());
	V.GI->CompleteObjective(TEXT("Obj_Relay"), ETDAbilityId::Cloak, ETDShipId::Rift);
	TestTrue(TEXT("open once both are done"), Zone->IsExitAllowed());

	// Climb out through the ceiling: T1's zone requests TravelTo(Galaxy, "Verdance_Orbit").
	FString Opened;
	V.GI->OpenLevelOverride = [&Opened](const FString& P) { Opened = P; };
	TDGroundTest::FScopedFakeGalaxyMap FakeGalaxy;
	FVector Origin, Extent;
	Zone->GetActorBounds(false, Origin, Extent);
	Ship->SetActorLocation(FVector(Origin.X, Origin.Y, Origin.Z + Extent.Z + Zone->ExitHeightAboveCeiling + 2000.f));
	for (int32 i = 0; i < 90 && !Zone->IsExitTriggered(); ++i) { V.W.Tick(1.f / 30.f); }
	TestTrue(TEXT("exit triggered"), Zone->IsExitTriggered());
	TestTrue(TEXT("travel to the galaxy started"), V.GI->IsTravelPending());
	TestEqual(TEXT("return point is the planet's orbit"), V.GI->GetProgressRef().ReturnPoint, FName(TEXT("Verdance_Orbit")));
	// T1 (ENG-38): the level change waits for the climb out (the exit and the return point are set at once).
	V.GI->TickFlow(Zone->ExitClimbSeconds + UTDFlowSettings::Get()->TravelFadeSeconds + 0.1f);
	TestEqual(TEXT("opens the galaxy map"), Opened, FString(TEXT("/Engine/Maps/Entry")));
	V.GI->OpenLevelOverride = nullptr;
	return true;
}


// ================================================================================================
// ENG-25 / ENG-26: each planet finishable end to end through the real path.
// ================================================================================================
namespace TDCourseTest
{
	using TDPlanetTest::FVisit;

	struct FRun
	{
		FVisit& V;
		FAutomationTestBase& T;
		ATDFirstPersonCharacter* FP = nullptr;
		ATDPlanetCourse* Course = nullptr;
		FString LastTraced;
		bool LastCanUse = false;

		FRun(FVisit& InV, FAutomationTestBase& InT) : V(InV), T(InT) {}

		bool Land()
		{
			if (!T.TestTrue(TEXT("lands and hands over"), V.LandForReal()))
			{
				return false;
			}
			FP = V.Director->GetFirstPerson();
			Course = V.Director->GetCourse();
			if (FP)
			{
				FP->GetHealth()->SetInvulnerable(true); // a walkthrough of the route, not a fight test
			}
			return T.TestNotNull(TEXT("pilot"), FP) && T.TestNotNull(TEXT("course"), Course);
		}
		void Tick(float Seconds)
		{
			for (float S = 0.f; S < Seconds; S += 1.f / 30.f)
			{
				V.W.Tick(1.f / 30.f);
				V.GI->TickFlow(1.f / 30.f);
			}
		}
		void TeleportTo(const AActor* A, float ZUp = 120.f)
		{
			FP->RespawnAt(FTransform(FP->GetActorRotation(), A->GetActorLocation() + FVector(0.f, 0.f, ZUp)));
			Tick(0.3f);
		}
		/** Stand 160 cm from an interactable, look at it, press interact (and hold for Hold seconds). */
		void Use(AActor* A, float Hold = 0.f)
		{
			const FVector Target = A->GetActorLocation();
			// Walk up from the side the pilot is on (never through the ship or a wall).
			FVector Side = (FP->GetActorLocation() - Target).GetSafeNormal2D();
			if (Side.IsNearlyZero()) { Side = FVector(-1.f, 0.f, 0.f); }
			FP->RespawnAt(FTransform(FRotator::ZeroRotator, Target + Side * 160.f + FVector(0.f, 0.f, 40.f)));
			Tick(0.2f);
			const FVector Eye = FP->FindComponentByClass<UCameraComponent>()->GetComponentLocation();
			V.PC->SetControlRotation((Target - Eye).Rotation());
			Tick(0.1f);
			LastTraced = GetNameSafe(FP->GetInteractTarget());
			LastCanUse = Cast<ITDInteractable>(A) && Cast<ITDInteractable>(A)->CanInteract(FP);
			FP->BeginInteract();
			Tick(Hold + 0.4f);
			FP->EndInteract();
		}
		void KillAll(ATDEncounterVolume* E)
		{
			for (int32 Pass = 0; Pass < 6 && E && !E->IsCleared(); ++Pass)
			{
				for (const TWeakObjectPtr<AActor>& M : E->GetSpawned())
				{
					if (UTDHealthComponent* H = M.IsValid() ? UTDStatics::GetHealth(M.Get()) : nullptr)
					{
						if (H->IsAlive()) { H->Kill(FP, ETDDamageKind::Melee); }
					}
				}
				Tick(0.5f);
			}
		}
		template <class TProp>
		TArray<TProp*> Props(const ATDObjective* Obj, int32 Step = INDEX_NONE)
		{
			TArray<TProp*> Out;
			for (TActorIterator<TProp> It(V.W.GetWorld()); It; ++It)
			{
				if (It->LinkedObjective == Obj && (Step == INDEX_NONE || It->LinkedStep == Step)) { Out.Add(*It); }
			}
			return Out;
		}
		ATDEncounterVolume* StepFight(const ATDObjective* Obj, int32 Step)
		{
			for (TActorIterator<ATDEncounterVolume> It(V.W.GetWorld()); It; ++It)
			{
				if (It->LinkedObjective == Obj && It->LinkedStep == Step) { return *It; }
			}
			return nullptr;
		}
		bool StartAt(FName Id, bool bSkipEarlier = true)
		{
			for (const FTDTrailhead& H : Course->GetTrailheads())
			{
				if (H.Objective && H.Objective->ObjectiveId == Id && H.Marker)
				{
					for (ATDObjective* Before = H.After; bSkipEarlier && Before; )
					{
						if (!Before->IsComplete()) { Before->MarkAlreadyComplete(); }
						ATDObjective* Next = nullptr;
						for (const FTDTrailhead& O : Course->GetTrailheads()) { if (O.Objective == Before) { Next = O.After; } }
						Before = Next;
					}
					TeleportTo(H.Marker, -150.f);
					Tick(0.5f);
					return T.TestTrue(*FString::Printf(TEXT("%s starts at its trailhead (own scene)"), *Id.ToString()),
						H.Objective->IsActive() && V.GI->GetCurrentScene() == Id);
				}
			}
			return false;
		}
		int32 Step(const ATDObjective* O) const { return O ? O->GetCurrentStepIndex() : -2; }
		/** A console tap (the wreck's device at step 2, the relay's at step 1); then the relay's transfer, clearing each wave. */
		ATDArchiveTransfer* Transfer() const { return Course ? Course->GetArchiveTransfer() : nullptr; }
		void UseTerminal(const ATDObjective* Obj, int32 AtStep = 2)
		{
			for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It)
			{
				if (It->LinkedObjective == Obj && It->LinkedStep == AtStep && !It->IsA<ATDExtractionSite>()) { Use(*It, It->HoldTime); return; }
			}
		}
		/** Verdance's relay: the device connected (step 1), its transfer halves (steps 2 and 3), then back to the ship (4). */
		void ConnectAtRelay(const ATDObjective* Relay)
		{
			TeleportTo(Props<ATDStepTrigger>(Relay, 0)[0], 60.f);
			UseTerminal(Relay, 1);
		}
		void RunTransfer(const ATDObjective* Obj, float MaxSeconds = 140.f)
		{
			ATDArchiveTransfer* X = Transfer();
			const int32 First = X ? X->LinkedStep - 1 : 0;
			for (float Tm = 0.f; X && Tm < MaxSeconds && Step(Obj) >= First && Step(Obj) <= First + 1; Tm += 1.f)
			{
				for (ATDEncounterVolume* E : X->Waves) { if (E && E->IsActive()) { KillAll(E); } }
				// Anyone else at the terminal holds it too (drones from an earlier fight follow the pilot): a player clears them.
				for (TActorIterator<ATDCrewCharacter> It(V.W.GetWorld()); It; ++It)
				{
					if (!It->IsDead() && FVector::Dist2D(It->GetActorLocation(), X->GetActorLocation()) < X->PauseRadius * 2.f)
					{
						if (UTDHealthComponent* H = UTDStatics::GetHealth(*It)) { H->Kill(FP, ETDDamageKind::Melee); }
					}
				}
				Tick(1.f);
			}
			if (X && Step(Obj) >= First && Step(Obj) <= First + 1)
			{
				// Diagnose a stuck transfer: who stands at the terminal, and from which encounter.
				for (TActorIterator<ATDCrewCharacter> It(V.W.GetWorld()); It; ++It)
				{
					const float D = (float)FVector::Dist2D(It->GetActorLocation(), X->GetActorLocation());
					if (D < X->PauseRadius * 2.f)
					{
						T.AddInfo(FString::Printf(TEXT("near the terminal: %s type %d dead %d at %.0f cm, owner %s"), *It->GetName(), (int32)It->CrewType, It->IsDead(), D,
							*GetNameSafe(It->OwningEncounter.Get())));
					}
				}
				T.AddInfo(FString::Printf(TEXT("transfer stuck at %.2f, paused %d, step %d"), X->GetProgress01(), X->IsPaused(), Step(Obj)));
			}
		}
		FString Line() const { return V.GI->GetCurrentObjectiveText().ToString(); }

		/** Re-board at the board point, climb out through the ceiling, and check the galaxy travel. */
		void LeaveForOrbit(const TCHAR* Planet)
		{
			ATDAtmosphereFlightZone* Zone = V.Director->GetRig() ? V.Director->GetRig()->GetZone() : nullptr;
			T.TestTrue(TEXT("ceiling open"), Zone && Zone->IsExitAllowed());
			T.TestTrue(*FString::Printf(TEXT("line says go back ('%s')"), *Line()), Line().Contains(TEXT("RETURN TO YOUR SHIP")));
			T.TestTrue(TEXT("board point is the marker"), V.Director->GetBoardPoint() && V.Director->GetBoardPoint()->ActorHasTag(TDTags::ObjectiveMarker));
			Use(V.Director->GetBoardPoint(), V.Director->GetBoardPoint()->HoldTime);
			Tick(5.f); // lift-off: the zone only lets a flying ship out
			T.TestFalse(*FString::Printf(TEXT("back in the ship (traced %s, usable %d, board %s)"), *LastTraced, LastCanUse, *GetNameSafe(V.Director->GetBoardPoint())), V.Director->IsOnFoot());
			ATDShip* Ship = V.Ship();
			if (!Zone || !Ship) { return; }
			FString Opened;
			V.GI->OpenLevelOverride = [&Opened](const FString& P) { Opened = P; };
			TDGroundTest::FScopedFakeGalaxyMap FakeGalaxy;
			FVector Origin, Extent;
			Zone->GetActorBounds(false, Origin, Extent);
			Ship->SetActorLocation(FVector(Origin.X, Origin.Y, Origin.Z + Extent.Z + Zone->ExitHeightAboveCeiling + 2000.f));
			for (int32 i = 0; i < 90 && !Zone->IsExitTriggered(); ++i) { V.W.Tick(1.f / 30.f); }
			T.TestTrue(TEXT("travel to the galaxy"), V.GI->IsTravelPending());
			T.TestEqual(TEXT("at the planet's orbit"), V.GI->GetProgressRef().ReturnPoint, FName(*FString::Printf(TEXT("%s_Orbit"), Planet)));
			V.GI->TickFlow(Zone->ExitClimbSeconds + UTDFlowSettings::Get()->TravelFadeSeconds + 0.1f); // T1: the climb out first
			V.GI->OpenLevelOverride = nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVerdanceEndToEnd, "TriadDrift.Ground.Course.VerdanceFinishableEndToEnd", TD_TEST_FLAGS)
bool FTDVerdanceEndToEnd::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	R.Tick(0.6f);
	TestTrue(*FString::Printf(TEXT("idle line offers the research site first ('%s')"), *R.Line()), R.Line().Contains(TEXT("THE RESEARCH SITE")) && !R.Line().Contains(TEXT("THE RELAY")));
	TestEqual(TEXT("one trailhead marked (the relay waits)"), V.GI->GetFlowMarkers().Num(), 1);

	ATDObjective* Relay = R.Course->FindObjective(TEXT("Obj_Relay"));
	ATDObjective* Wreck = R.Course->FindObjective(TEXT("Obj_WreckCore"));
	if (!TestNotNull(TEXT("relay"), Relay) || !TestNotNull(TEXT("wreck"), Wreck)) { return false; }

	// The fixed order (decision 2): the device first, then the relay.
	TestFalse(TEXT("the relay waits for the device"), R.Course->StartObjectiveAt(TEXT("Obj_Relay")));
	if (!R.StartAt(TEXT("Obj_WreckCore"), false)) { return false; }
	R.TeleportTo(R.Props<ATDStepTrigger>(Wreck, 0)[0], 60.f);
	TestEqual(TEXT("at the beacon -> clear the station"), R.Step(Wreck), 1);
	ATDEncounterVolume* Site = R.StepFight(Wreck, 1);
	TestTrue(TEXT("station fight running"), Site && Site->IsActive());
	R.KillAll(Site);
	TestEqual(TEXT("station clear -> the console"), R.Step(Wreck), 2);
	R.UseTerminal(Wreck);
	TestTrue(TEXT("device recovered"), Wreck->IsComplete());
	TestEqual(TEXT("Gravity Well on Bastion"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);
	TestFalse(TEXT("nothing carried"), R.FP->IsCarrying());
	R.Tick(0.5f);
	TestTrue(*FString::Printf(TEXT("then the relay is offered ('%s')"), *R.Line()), R.Line().Contains(TEXT("THE RELAY")));

	// The relay: connect the device, hold the deck through a 75 s transfer under three waves, then back to the ship.
	if (!R.StartAt(TEXT("Obj_Relay"))) { return false; }
	R.TeleportTo(R.Props<ATDStepTrigger>(Relay, 0)[0], 60.f);
	TestEqual(TEXT("at the relay -> connect"), R.Step(Relay), 1);
	int32 Hooked = 0;
	const FDelegateHandle Hook = ATDArchiveTransfer::OnAnyTerminalActivated.AddLambda([&Hooked](AActor*) { ++Hooked; });
	R.UseTerminal(Relay, 1);
	ATDArchiveTransfer::OnAnyTerminalActivated.Remove(Hook);
	TestEqual(TEXT("Cinematic 03's hook fires once"), Hooked, 1);
	TestEqual(TEXT("console -> the transfer"), R.Step(Relay), 2);
	float P01 = -1.f; bool bPaused = true; FText Label;
	TestTrue(TEXT("T2's meter sees a running transfer"), ATDArchiveTransfer::GetArchiveTransfer(V.W.GetWorld(), P01, bPaused, Label) && P01 < 0.1f && !bPaused);
	R.RunTransfer(Relay);
	TestEqual(TEXT("transfer done -> back to the ship"), R.Step(Relay), 4);
	TestTrue(TEXT("the midpoint was a checkpoint"), Relay->GetCheckpointStep() >= 3);
	TestTrue(TEXT("the third wave (the Heavy) dropped"), R.Transfer() && R.Transfer()->Waves.Num() == 3 && R.Transfer()->Waves[2] && (R.Transfer()->Waves[2]->IsActive() || R.Transfer()->Waves[2]->IsCleared()));
	TestFalse(TEXT("meter gone after the transfer"), ATDArchiveTransfer::GetArchiveTransfer(V.W.GetWorld(), P01, bPaused, Label));
	R.FP->RespawnAt(FTransform(FVector(300.f, 0.f, 120.f)));
	R.Tick(0.5f);
	TestTrue(TEXT("relay done at the ship"), Relay->IsComplete());
	TestEqual(TEXT("Cloak on Rift"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::Cloak), ETDShipId::Rift);

	R.Tick(0.5f);
	R.LeaveForOrbit(TEXT("Verdance"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCinderEndToEnd, "TriadDrift.Ground.Course.CinderFinishableEndToEnd", TD_TEST_FLAGS)
bool FTDCinderEndToEnd::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(NAME_None, TEXT("Cinder"));
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	ATDObjective* Outpost = R.Course->FindObjective(TEXT("Obj_Outpost"));
	ATDObjective* Jammer = R.Course->FindObjective(TEXT("Obj_Jammer"));
	if (!TestNotNull(TEXT("outpost"), Outpost) || !TestNotNull(TEXT("jammer"), Jammer)) { return false; }

	// The fixed order (storyline.md section 25): the survey records, then the excavation, then the anchor.
	ATDObjective* Survey = R.Course->FindObjective(TEXT("Obj_Survey"));
	if (!TestNotNull(TEXT("survey records"), Survey)) { return false; }
	TestFalse(TEXT("the excavation waits for the records"), R.Course->StartObjectiveAt(TEXT("Obj_Outpost")));
	TestFalse(TEXT("the anchor waits too"), R.Course->StartObjectiveAt(TEXT("Obj_Jammer")));
	if (!R.StartAt(TEXT("Obj_Survey"), false)) { return false; }
	// The patrol on the way (a player fights it; the terminal works only once it is dead or far behind).
	for (TActorIterator<ATDEncounterVolume> It(V.W.GetWorld()); It; ++It)
	{
		if (It->LinkedObjective == Survey)
		{
			R.TeleportTo(*It, 0.f);
			R.Tick(0.5f);
			TestTrue(TEXT("the patrol engages on the route"), It->IsActive() || It->IsCleared());
			R.KillAll(*It);
		}
	}
	R.TeleportTo(R.Props<ATDStepTrigger>(Survey, 0)[0], 60.f);
	TestEqual(TEXT("at the station -> the records"), R.Step(Survey), 1);
	ATDSurveyTerminal* Records = R.Course->GetSurveyTerminal();
	if (!TestNotNull(TEXT("survey terminal"), Records)) { return false; }
	// Out of combat only: a crewman near the station blocks it.
	ATDCrewCharacter* Patrol = V.W.Spawn<ATDCrewCharacter>(Records->GetActorLocation() + FVector(-600.f, 300.f, 0.f));
	R.Tick(0.2f);
	TestFalse(TEXT("the terminal waits while a hostile is near"), Records->CanInteract(R.FP));
	if (Patrol) { Patrol->Destroy(); }
	R.Tick(0.2f);
	int32 Hooked = 0;
	const FDelegateHandle Hook = ATDSurveyTerminal::OnAnySurveyRecordsAccessed.AddLambda([&Hooked](AActor*) { ++Hooked; });
	R.Use(Records, Records->HoldTime);
	ATDSurveyTerminal::OnAnySurveyRecordsAccessed.Remove(Hook);
	TestEqual(TEXT("Cinematic 05's hook fires once"), Hooked, 1);
	TestTrue(TEXT("records accessed"), Survey->IsComplete());
	TestFalse(TEXT("the anchor still waits for the excavation"), R.Course->StartObjectiveAt(TEXT("Obj_Jammer")));

	// The excavation.
	if (!R.StartAt(TEXT("Obj_Outpost"), false)) { return false; }
	TArray<ATDShatterCrystal*> Conduits = R.Props<ATDShatterCrystal>(Outpost);
	TestEqual(TEXT("two conduits"), Conduits.Num(), 2);
	for (ATDShatterCrystal* C : Conduits) { UTDStatics::ApplyTDDamage(C, 500.f, ETDDamageKind::Melee, R.FP, R.FP); }
	R.Tick(0.3f);
	TestEqual(TEXT("gate breached -> yard"), R.Step(Outpost), 1);
	for (int32 Room = 1; Room <= 4; ++Room)
	{
		ATDEncounterVolume* E = R.StepFight(Outpost, Room);
		if (!TestNotNull(*FString::Printf(TEXT("room %d"), Room), E)) { return false; }
		R.TeleportTo(E, -150.f);
		R.Tick(0.5f);
		TestTrue(*FString::Printf(TEXT("room %d fight starts"), Room), E->IsActive() || E->IsCleared());
		R.KillAll(E);
		TestEqual(*FString::Printf(TEXT("room %d cleared"), Room), R.Step(Outpost), Room + 1);
	}
	ATDInteractableActor* Cache = nullptr;
	for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Outpost && It->LinkedStep == 5) { Cache = *It; } }
	if (!TestNotNull(TEXT("crystal cache"), Cache)) { return false; }
	R.Use(Cache, Cache->HoldTime);
	TestTrue(TEXT("excavation secured"), Outpost->IsComplete());
	TestEqual(TEXT("EMP on Lancer"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::EMPPulse), ETDShipId::Lancer);

	// The anchor, then the run to extraction.
	int32 AnchorDown = 0;
	const FDelegateHandle Anchor = ATDPlanetCourse::OnFieldAnchorDisabled.AddLambda([&AnchorDown]() { ++AnchorDown; });
	if (!R.StartAt(TEXT("Obj_Jammer"), false)) { ATDPlanetCourse::OnFieldAnchorDisabled.Remove(Anchor); return false; }
	R.TeleportTo(R.Props<ATDStepTrigger>(Jammer, 0)[0], 60.f);
	TestEqual(TEXT("across the bridge -> charges"), R.Step(Jammer), 1);
	TArray<ATDInteractableActor*> Charges;
	for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Jammer && It->LinkedStep == 1) { Charges.Add(*It); } }
	TestEqual(TEXT("three charge sites"), Charges.Num(), 3);
	for (ATDInteractableActor* C : Charges) { R.Use(C, C->HoldTime); }
	TestEqual(TEXT("charges set -> run"), R.Step(Jammer), 2);
	TArray<ATDCollapseRun*> Runs = R.Props<ATDCollapseRun>(Jammer, 2);
	TestTrue(TEXT("the tower is coming down"), Runs.Num() == 1 && Runs[0]->IsRunning());
	TestTrue(*FString::Printf(TEXT("countdown on the line ('%s')"), *R.Line()), R.Line().Contains(TEXT("RETURN TO EXTRACTION")));
	R.TeleportTo(R.Props<ATDStepTrigger>(Jammer, 2)[0], 60.f);
	ATDPlanetCourse::OnFieldAnchorDisabled.Remove(Anchor);
	TestTrue(TEXT("anchor disabled"), Jammer->IsComplete());
	TestEqual(TEXT("T2's pending-marker event once"), AnchorDown, 1);

	R.Tick(0.5f);
	R.LeaveForOrbit(TEXT("Cinder"));
	return true;
}

// Decision 2: nothing is carried on Verdance any more. A death in the relay transfer's first half: CUT, take 2 at the
// console with the transfer from zero and the first wave fresh; the device (the wreck) stays recovered.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseRetake, "TriadDrift.Ground.Course.DeathInTheRelayTransferRetakesAtTheConsole", TD_TEST_FLAGS)
bool FTDCourseRetake::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	ATDObjective* Wreck = R.Course->FindObjective(TEXT("Obj_WreckCore"));
	ATDObjective* Relay = R.Course->FindObjective(TEXT("Obj_Relay"));
	if (!R.StartAt(TEXT("Obj_Relay"))) { return false; }
	R.ConnectAtRelay(Relay);
	ATDArchiveTransfer* X = R.Transfer();
	if (!TestNotNull(TEXT("transfer"), X) || !TestEqual(TEXT("transfer first half"), R.Step(Relay), 2)) { return false; }
	R.Tick(8.f);
	TestTrue(TEXT("progress made"), X->GetProgress01() > 0.05f);
	R.FP->GetHealth()->SetInvulnerable(false);
	R.FP->GetHealth()->Kill(nullptr, ETDDamageKind::Energy);
	R.Tick(UTDFlowSettings::Get()->RetakeDelaySeconds + 1.f);
	TestFalse(TEXT("pilot alive"), R.FP->IsDead());
	TestTrue(TEXT("same scene restarts at the transfer"), Relay->IsActive() && R.Step(Relay) == 2);
	TestEqual(TEXT("take 2"), V.GI->GetProgressRef().Takes.FindRef(TEXT("Obj_Relay")), 2);
	TestTrue(*FString::Printf(TEXT("transfer from the start (%.2f)"), X->GetProgress01()), X->GetProgress01() < 0.1f);
	TestTrue(TEXT("respawned by the console"), FVector::Dist2D(R.FP->GetActorLocation(), X->GetActorLocation()) < 1500.f);
	TestTrue(TEXT("the first wave dropped fresh"), X->Waves[0] && X->Waves[0]->GetAliveCount() <= X->Waves[0]->CrewMix.Num());
	TestTrue(TEXT("the device stays recovered"), Wreck && Wreck->IsComplete());
	R.FP->GetHealth()->SetInvulnerable(true);
	R.RunTransfer(Relay);
	R.FP->RespawnAt(FTransform(FVector(300.f, 0.f, 120.f)));
	R.Tick(0.5f);
	TestTrue(TEXT("finished on take 2"), Relay->IsComplete());
	return true;
}

// Verdance's archive transfer (storyline.md section 18; at the relay since decision 2): it pauses while a crewman stands at the terminal, the midpoint is
// a checkpoint, and a death after it resumes the transfer from 50 % with the Heavy's wave dropping again.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDArchiveTransferPauseAndMidpoint, "TriadDrift.Ground.Course.ArchiveTransferPausesAndResumesAtTheMidpoint", TD_TEST_FLAGS)
bool FTDArchiveTransferPauseAndMidpoint::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	ATDObjective* Relay = R.Course->FindObjective(TEXT("Obj_Relay"));
	if (!R.StartAt(TEXT("Obj_Relay"))) { return false; }
	R.ConnectAtRelay(Relay);
	ATDArchiveTransfer* X = R.Transfer();
	if (!TestNotNull(TEXT("transfer"), X) || !TestEqual(TEXT("transfer step"), R.Step(Relay), 2)) { return false; }
	// Put a living crewman of the first wave at the terminal: the transfer holds.
	R.Tick(2.f);
	AActor* Crew = nullptr;
	for (const TWeakObjectPtr<AActor>& M : X->Waves[0]->GetSpawned()) { if (M.IsValid()) { Crew = M.Get(); break; } }
	if (!TestNotNull(TEXT("first wave crew"), Crew)) { return false; }
	Crew->SetActorLocation(X->GetActorLocation() + FVector(150.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	R.Tick(0.3f);
	const float Held = X->GetProgress01();
	R.Tick(2.f);
	TestTrue(TEXT("paused with a hostile at the terminal"), X->IsPaused());
	TestEqual(TEXT("no progress while paused"), X->GetProgress01(), Held, 0.02f);
	TestTrue(*FString::Printf(TEXT("the line says so ('%s')"), *R.Line()), R.Line().Contains(TEXT("PAUSED")));
	// Clear it and run to the midpoint.
	for (float T = 0.f; T < 80.f && R.Step(Relay) == 2; T += 1.f)
	{
		for (ATDEncounterVolume* E : X->Waves) { if (E && E->IsActive()) { R.KillAll(E); } }
		R.Tick(1.f);
	}
	TestEqual(TEXT("midpoint -> second half"), R.Step(Relay), 3);
	TestEqual(TEXT("the second half is the checkpoint"), Relay->GetCheckpointStep(), 3);
	R.Tick(3.f);
	TestTrue(TEXT("the Heavy's wave drops at the midpoint"), X->Waves[2]->IsActive());
	// Die: take 2 picks up at 50 %, the Heavy's wave fresh.
	R.FP->GetHealth()->SetInvulnerable(false);
	R.FP->GetHealth()->Kill(nullptr, ETDDamageKind::Energy);
	R.Tick(UTDFlowSettings::Get()->RetakeDelaySeconds + 1.5f);
	TestEqual(TEXT("take 2 at the second half"), R.Step(Relay), 3);
	TestTrue(*FString::Printf(TEXT("resumes from the midpoint (%.2f)"), X->GetProgress01()), X->GetProgress01() >= 0.5f && X->GetProgress01() < 0.6f);
	TestTrue(TEXT("the Heavy's wave again"), X->Waves[2]->IsActive());
	R.FP->GetHealth()->SetInvulnerable(true);
	R.RunTransfer(Relay);
	TestEqual(TEXT("finished on take 2 -> back to the ship"), R.Step(Relay), 4);
	return true;
}

// T4's world scatter asks the course where it may place things (WORLD-DENSITY-PLAN): never on a route, the pad, a site, an
// encounter or a spawn point; and the course's routes stay walkable (every route point is refused for scatter, and
// open ground off the routes is allowed).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseKeepOuts, "TriadDrift.Ground.Course.KeepOutsProtectRoutesSitesAndSpawns", TD_TEST_FLAGS)
bool FTDCourseKeepOuts::RunTest(const FString& Parameters)
{
	for (const TCHAR* Planet : { TEXT("Verdance"), TEXT("Cinder") })
	{
		TDPlanetTest::FVisit V(NAME_None, Planet);
		ATDPlanetCourse* Course = V.Director->GetCourse();
		if (!TestNotNull(TEXT("course"), Course)) { return false; }
		const TArray<FTDCourseKeepOut>& K = Course->GetKeepOuts();
		int32 Routes = 0, Encounters = 0, Spawns = 0, Pads = 0;
		for (const FTDCourseKeepOut& O : K)
		{
			Routes += O.Kind == TEXT("Route"); Encounters += O.Kind == TEXT("Encounter"); Spawns += O.Kind == TEXT("Spawn"); Pads += O.Kind == TEXT("Pad");
		}
		TestTrue(*FString::Printf(TEXT("%s: routes %d, encounters %d, spawns %d, pad %d"), Planet, Routes, Encounters, Spawns, Pads), Routes >= 2 && Encounters >= 3 && Spawns >= 3 && Pads == 1);
		// Every point along every route (each 2 m) is refused for a 1 m tree, and so is the pad.
		int32 Points = 0, Blocked = 0;
		for (const FTDCourseKeepOut& O : K)
		{
			for (int32 i = 0; O.Kind == TEXT("Route") && i + 1 < O.Points.Num(); ++i)
			{
				const float Len = (float)FVector::Dist2D(O.Points[i], O.Points[i + 1]);
				for (float D = 0.f; D <= Len; D += 200.f)
				{
					const FVector P = FMath::Lerp(O.Points[i], O.Points[i + 1], Len > 0.f ? D / Len : 0.f);
					++Points;
					Blocked += Course->IsScatterAllowed(P, 100.f) ? 0 : 1;
				}
			}
		}
		TestTrue(*FString::Printf(TEXT("%s: route points"), Planet), Points > 50);
		TestEqual(*FString::Printf(TEXT("%s: every route point refused"), Planet), Blocked, Points);
		TestFalse(TEXT("the pad refused"), ATDPlanetCourse::IsScatterAllowedAt(V.W.GetWorld(), Course->GetActorLocation() + FVector(800.f, 300.f, 0.f), 100.f));
		// Open ground exists for the scatter (a ring far from everything).
		int32 Open = 0;
		for (int32 a = 0; a < 36; ++a)
		{
			const FVector P = Course->GetActorLocation() + FVector(FMath::Cos(a * 10.f * PI / 180.f), FMath::Sin(a * 10.f * PI / 180.f), 0.f) * 25000.f;
			Open += Course->IsScatterAllowed(P, 100.f) ? 1 : 0;
		}
		TestTrue(*FString::Printf(TEXT("%s: open ground at 250 m (%d / 36)"), Planet, Open), Open >= 18);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseMissingNet, "TriadDrift.Ground.Course.MissingObjectiveActorNeverTrapsThePlayer", TD_TEST_FLAGS)
bool FTDCourseMissingNet::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("has no objective actor in the world"), EAutomationExpectedErrorFlags::Contains, 2);
	TDPlanetTest::FVisit V(NAME_None, TEXT("Verdance"), /*bCourse*/ false);
	TestTrue(TEXT("lands"), V.LandForReal());
	TestTrue(TEXT("both completed by the net"), V.GI->GetProgressRef().CompletedObjectives.Contains(TEXT("Obj_WreckCore")) && V.GI->GetProgressRef().CompletedObjectives.Contains(TEXT("Obj_Relay")));
	TestEqual(TEXT("rewards granted"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::Cloak), ETDShipId::Rift);
	ATDAtmosphereFlightZone* Zone = V.Director->GetRig() ? V.Director->GetRig()->GetZone() : nullptr;
	TestTrue(TEXT("the way out opens"), Zone && Zone->IsExitAllowed());
	return true;
}


// Review MAJOR 1: boarding with crew on the pilot must never CUT in flight.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlanetBoardUnderAttack, "TriadDrift.Ground.Planet.BoardingWithCrewChasingNeverCutsInFlight", TD_TEST_FLAGS)
bool FTDPlanetBoardUnderAttack::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(NAME_None, TEXT("Verdance"), /*bCourse*/ false);
	AddExpectedError(TEXT("has no objective actor in the world"), EAutomationExpectedErrorFlags::Contains, 2); // the net, no course here
	if (!TestTrue(TEXT("landed"), V.LandForReal())) { return false; }
	ATDFirstPersonCharacter* FP = V.Director->GetFirstPerson();
	ATDBoardShipInteractable* Board = V.Director->GetBoardPoint();
	if (!TestNotNull(TEXT("pilot"), FP) || !TestNotNull(TEXT("board"), Board)) { return false; }
	TArray<ATDCrewCharacter*> Crew;
	for (int32 i = 0; i < 2; ++i)
	{
		const FTransform Xf(FRotator(0.f, 180.f, 0.f), FP->GetActorLocation() + FVector(250.f, i * 150.f - 75.f, 0.f));
		ATDCrewCharacter* C = V.W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		C->CrewType = ETDCrewType::Grunt;
		C->FinishSpawning(Xf);
		Crew.Add(C);
	}
	for (float T = 0.f; T < 2.f; T += 1.f / 30.f) { V.W.Tick(1.f / 30.f); V.GI->TickFlow(1.f / 30.f); }
	TestTrue(TEXT("the grunts are fighting the pilot"), FP->GetHealth()->GetHull01() < 1.f || Crew[0]->GetController() != nullptr);
	const float HullAtBoard = FP->GetHealth()->GetHull01();
	Board->Interact(FP);
	for (float T = 0.f; T < 6.f; T += 1.f / 30.f) { V.W.Tick(1.f / 30.f); V.GI->TickFlow(1.f / 30.f); }
	TestFalse(TEXT("no CUT after take-off"), V.GI->IsRetakePending());
	TestTrue(TEXT("pilot never hit after boarding"), FP->GetHealth()->IsAlive() && FP->GetHealth()->GetHull01() >= HullAtBoard - KINDA_SMALL_NUMBER);
	TestTrue(TEXT("still flying the ship"), V.PC->GetPawn() == (APawn*)V.Ship());
	// And back on foot the pilot is a target again.
	TestTrue(TEXT("pilot invulnerable while stowed"), FP->GetHealth()->IsInvulnerable());
	return true;
}


// Review MAJOR 4: a reload with one objective already done keeps its rooms cleared, doors and gate open,
// conduits gone; the other objective still plays.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseReloadDone, "TriadDrift.Ground.Course.ReloadKeepsACompletedObjectiveOpen", TD_TEST_FLAGS)
bool FTDCourseReloadDone::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(NAME_None, TEXT("Cinder"), true, { TEXT("Obj_Outpost") });
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	R.Tick(0.5f);
	ATDObjective* Outpost = R.Course->FindObjective(TEXT("Obj_Outpost"));
	ATDObjective* Jammer = R.Course->FindObjective(TEXT("Obj_Jammer"));
	if (!TestNotNull(TEXT("outpost"), Outpost) || !TestNotNull(TEXT("jammer"), Jammer)) { return false; }
	TestTrue(TEXT("outpost complete on load"), Outpost->IsComplete());
	int32 Rooms = 0, OpenDoors = 0, Doors = 0;
	for (TActorIterator<ATDEncounterVolume> It(V.W.GetWorld()); It; ++It)
	{
		if (It->LinkedObjective != Outpost) { continue; }
		++Rooms;
		TestTrue(*FString::Printf(TEXT("%s cleared"), *It->GetName()), It->IsCleared());
		for (ATDDoor* D : It->DoorsToOpen) { ++Doors; OpenDoors += D && D->IsOpen() ? 1 : 0; }
	}
	TestTrue(TEXT("outpost rooms found"), Rooms >= 4);
	TestEqual(TEXT("every room door open"), OpenDoors, Doors);
	int32 GateOpen = 0;
	for (TActorIterator<ATDDoor> It(V.W.GetWorld()); It; ++It) { GateOpen += It->IsOpen() ? 1 : 0; }
	TestEqual(TEXT("gate and all room doors open (4)"), GateOpen, 4);
	for (ATDShatterCrystal* C : R.Props<ATDShatterCrystal>(Outpost)) { TestTrue(TEXT("conduit gone"), C->IsShattered()); }
	// Walking into a cleared room spawns nobody.
	R.TeleportTo(R.StepFight(Outpost, 2), -150.f);
	R.Tick(1.f);
	TestEqual(TEXT("no crew in a cleared room"), R.StepFight(Outpost, 2)->GetAliveCount(), 0);
	// The jammer still plays.
	TestTrue(TEXT("jammer still starts"), R.StartAt(TEXT("Obj_Jammer")));
	R.Tick(0.5f);
	TestTrue(*FString::Printf(TEXT("idle/next line is sane ('%s')"), *R.Line()), !R.Line().IsEmpty());
	return true;
}

// Review MAJOR 5: nothing can finish a step (and so an objective) inside the CUT delay; the retake replays it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseNoFinishDuringCut, "TriadDrift.Ground.Course.NothingCompletesDuringTheCutDelay", TD_TEST_FLAGS)
bool FTDCourseNoFinishDuringCut::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(NAME_None, TEXT("Cinder"));
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	ATDObjective* Jammer = R.Course->FindObjective(TEXT("Obj_Jammer"));
	if (!R.StartAt(TEXT("Obj_Jammer"))) { return false; }
	R.TeleportTo(R.Props<ATDStepTrigger>(Jammer, 0)[0], 60.f);
	for (ATDInteractableActor* C : R.Props<ATDInteractableActor>(Jammer, 1)) { R.Use(C, C->HoldTime); }
	TestEqual(TEXT("on the collapse run"), R.Step(Jammer), 2);
	// CUT! lands (another death, the timer): the exit is reached inside the delay.
	V.GI->ReportPlayerDefeated();
	TestTrue(TEXT("CUT pending"), V.GI->IsRetakePending());
	TestFalse(TEXT("a step can't complete now"), Jammer->CompleteStep(2));
	R.FP->RespawnAt(FTransform(R.Props<ATDStepTrigger>(Jammer, 2)[0]->GetActorLocation() + FVector(0.f, 0.f, 120.f)));
	for (int32 i = 0; i < 10; ++i) { V.W.Tick(1.f / 30.f); } // no flow tick: still inside the delay
	TestFalse(TEXT("not complete inside the delay"), Jammer->IsComplete());
	TestFalse(TEXT("not recorded either"), V.GI->GetProgressRef().CompletedObjectives.Contains(TEXT("Obj_Jammer")));
	R.Tick(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.5f);
	TestTrue(TEXT("take 2 replays the jammer from its charges checkpoint"), Jammer->IsActive() && R.Step(Jammer) == 1);
	// And it can be finished properly.
	for (ATDInteractableActor* C : R.Props<ATDInteractableActor>(Jammer, 1)) { R.Use(C, C->HoldTime); }
	R.TeleportTo(R.Props<ATDStepTrigger>(Jammer, 2)[0], 60.f);
	TestTrue(TEXT("finished on take 2 and recorded"), Jammer->IsComplete() && V.GI->GetProgressRef().CompletedObjectives.Contains(TEXT("Obj_Jammer")));
	return true;
}

// Reaching a checkpoint refills the pilot (the wreck site fight starts from full after the ambush), and a death in
// the site fight respawns at its edge, not at the pad.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseCheckpointRefill, "TriadDrift.Ground.Course.CheckpointRefillsAndSiteFightRespawnsAtItsEdge", TD_TEST_FLAGS)
bool FTDCourseCheckpointRefill::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	ATDObjective* Wreck = R.Course->FindObjective(TEXT("Obj_WreckCore"));
	if (!R.StartAt(TEXT("Obj_WreckCore"))) { return false; }
	R.FP->GetHealth()->SetInvulnerable(false);
	R.FP->GetHealth()->SetInvulnerableFor(0.f);
	UTDStatics::ApplyTDDamage(R.FP, 70.f, ETDDamageKind::Energy, nullptr, nullptr);
	TestTrue(TEXT("hurt by the ambush"), R.FP->GetHealth()->GetHull01() < 0.5f);
	R.TeleportTo(R.Props<ATDStepTrigger>(Wreck, 0)[0], 60.f);
	TestEqual(TEXT("at the site fight"), R.Step(Wreck), 1);
	TestEqual(TEXT("the site fight is a checkpoint"), Wreck->GetCheckpointStep(), 1);
	TestTrue(TEXT("reaching it refilled the pilot"), R.FP->GetHealth()->GetHull01() > 0.99f);
	V.GI->ReportPlayerDefeated();
	R.Tick(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.5f);
	TestEqual(TEXT("take 2 still at the site fight"), R.Step(Wreck), 1);
	TestTrue(*FString::Printf(TEXT("respawned at the site's edge, not the pad (%s)"), *R.FP->GetActorLocation().ToCompactString()),
		FVector::Dist2D(R.FP->GetActorLocation(), FVector(8300.f, 5800.f, 0.f)) < 400.f);
	return true;
}

// Journey run 2 stall: the pilot stood against the wreck core during the site fight and the grunts circled at
// The Cinder field anchor (import a78930a4: the plinth's hull refitted to the visible octagon): crew round it to reach a
// pilot on the far side, and every charge point is outside the tower's collision.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseAnchorLap, "TriadDrift.Ground.Course.CrewRoundTheFieldAnchorAndChargesAreReachable", TD_TEST_FLAGS)
bool FTDCourseAnchorLap::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(NAME_None, TEXT("Cinder"));
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	const FVector Tower(12600.f, -5000.f, 0.f);
	ATDObjective* Jammer = R.Course->FindObjective(TEXT("Obj_Jammer"));
	if (!TestNotNull(TEXT("anchor objective"), Jammer)) { return false; }
	// The charge points: each outside every blocking shape (a pawn-sized probe in front of it is free) and within reach.
	for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It)
	{
		if (It->LinkedObjective != Jammer || It->LinkedStep != 1) { continue; }
		const FVector P = It->GetActorLocation();
		const FVector D = FVector(P.X - Tower.X, P.Y - Tower.Y, 0.f).GetSafeNormal();
		const FVector Stand(P.X + D.X * 130.f, P.Y + D.Y * 130.f, 92.f);
		FCollisionQueryParams Q(SCENE_QUERY_STAT(TDAnchorProbe), false, R.FP);
		TestFalse(*FString::Printf(TEXT("%s: a pilot fits in front of it"), *It->GetName()), V.W.GetWorld()->OverlapBlockingTestByChannel(Stand, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(40.f, 90.f), Q));
		TestTrue(*FString::Printf(TEXT("%s: within interact reach from there (%.0f cm)"), *It->GetName(), FVector::Dist(Stand + FVector(0.f, 0.f, 74.f), P)), FVector::Dist(Stand + FVector(0.f, 0.f, 74.f), P) < 250.f);
	}
	// A grunt on the east side chases the pilot on the west side: it rounds the tower within 20 s.
	R.FP->SetActorLocation(Tower + FVector(-1150.f, 0.f, 95.f), false, nullptr, ETeleportType::TeleportPhysics);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ATDCrewCharacter* Grunt = V.W.GetWorld()->SpawnActor<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), FTransform(Tower + FVector(1150.f, 300.f, 100.f)), Params);
	if (!TestNotNull(TEXT("grunt"), Grunt)) { return false; }
	R.Tick(0.2f);
	if (ATDCrewAIController* AI = Cast<ATDCrewAIController>(Grunt->GetController())) { AI->SetTarget(R.FP); }
	float Best = 1e9f, Took = -1.f;
	for (float T = 0.f; T < 20.f; T += 0.25f)
	{
		R.Tick(0.25f);
		const float D = (float)FVector::Dist2D(Grunt->GetActorLocation(), R.FP->GetActorLocation());
		Best = FMath::Min(Best, D);
		if (D < 300.f) { Took = T; break; }
	}
	TestTrue(*FString::Printf(TEXT("the grunt rounds the anchor to the pilot (closest %.0f cm, at %s, took %.1f s)"), Best, *Grunt->GetActorLocation().ToCompactString(), Took), Took >= 0.f);
	return true;
}

// 1.0-1.3 m for 15 minutes without a swing. An idle pilot there must be attacked (and the crew must stay killable).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseCoreStall, "TriadDrift.Ground.Course.CrewAttackAPilotStandingAtTheWreckCore", TD_TEST_FLAGS)
bool FTDCourseCoreStall::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	ATDObjective* Wreck = R.Course->FindObjective(TEXT("Obj_WreckCore"));
	if (!R.StartAt(TEXT("Obj_WreckCore"))) { return false; }
	R.TeleportTo(R.Props<ATDStepTrigger>(Wreck, 0)[0], 60.f);
	ATDEncounterVolume* Site = R.StepFight(Wreck, 1);
	if (!TestEqual(TEXT("site fight step"), R.Step(Wreck), 1) || !TestNotNull(TEXT("site fight"), Site)) { return false; }
	R.FP->SetActorLocation(FVector(11379.f, 7218.f, R.FP->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	R.FP->GetHealth()->SetInvulnerable(false);
	R.FP->GetHealth()->SetInvulnerableFor(0.f);
	float HullMin = 1.f;
	int32 Swings = 0;
	TMap<FString, FString> States;
	for (int32 i = 0; i < 60 * 12 && !R.FP->IsDead(); ++i)
	{
		V.W.Tick(1.f / 60.f);
		V.GI->TickFlow(1.f / 60.f);
		HullMin = FMath::Min(HullMin, R.FP->GetHealth()->GetHull01());
		for (const TWeakObjectPtr<AActor>& M : Site->GetSpawned())
		{
			if (const ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get()))
			{
				Swings += C->IsTelegraphing() ? 1 : 0;
				States.Add(C->GetName(), FString::Printf(TEXT("%s at %.0f cm, attacking %d, staggered %d, grace %.1f"), *UEnum::GetValueAsString(C->CrewType),
					FVector::Dist2D(C->GetActorLocation(), R.FP->GetActorLocation()), C->IsAttacking(), C->IsStaggered(), C->GetOpeningGraceLeft()));
			}
		}
	}
	FString Report;
	for (const TPair<FString, FString>& S : States) { Report += S.Value + TEXT("; "); }
	AddInfo(Report);
	TestTrue(*FString::Printf(TEXT("the crew attack an idle pilot at the core within 12 s (lowest hull %.2f, wind-up frames %d)"), HullMin, Swings), HullMin < 0.99f || R.FP->IsDead());
	return true;
}

// Director's mercy on the real Cinder Warden room through real retakes (CUT! -> checkpoint at the arena door):
// take 1 and 2 the full Warden, take 3 at 75% with one summon, take 5 at 50% with none.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseWardenMercy, "TriadDrift.Ground.Course.WardenEasesOnRetakes", TD_TEST_FLAGS)
bool FTDCourseWardenMercy::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(NAME_None, TEXT("Cinder"));
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	ATDObjective* Outpost = R.Course->FindObjective(TEXT("Obj_Outpost"));
	if (!R.StartAt(TEXT("Obj_Outpost"))) { return false; }
	for (ATDShatterCrystal* C : R.Props<ATDShatterCrystal>(Outpost)) { UTDStatics::ApplyTDDamage(C, 500.f, ETDDamageKind::Melee, R.FP, R.FP); }
	R.Tick(0.3f);
	for (int32 Room = 1; Room <= 3; ++Room)
	{
		R.TeleportTo(R.StepFight(Outpost, Room), -150.f);
		R.Tick(0.5f);
		R.KillAll(R.StepFight(Outpost, Room));
	}
	ATDEncounterVolume* Arena = R.StepFight(Outpost, 4);
	if (!TestNotNull(TEXT("arena"), Arena) || !TestEqual(TEXT("at the Warden"), R.Step(Outpost), 4)) { return false; }
	TestEqual(TEXT("the arena carries two mercy tiers"), Arena->MercyTiers.Num(), 2);
	const float FullHealth = FTDCrewLibrary::Get(ETDCrewType::Warden).MaxHealth;
	struct FExpect { int32 Take; float Scale; int32 Summons; };
	const FExpect Expect[] = { { 1, 1.f, -1 }, { 2, 1.f, -1 }, { 3, 0.75f, 1 }, { 4, 0.75f, 1 }, { 5, 0.5f, 0 } };
	for (const FExpect& E : Expect)
	{
		R.FP->GetHealth()->SetInvulnerable(true); // only the scripted defeats count as lost takes
		R.TeleportTo(Arena, -150.f);
		R.Tick(0.5f);
		ATDCrewCharacter* Warden = nullptr;
		for (const TWeakObjectPtr<AActor>& M : Arena->GetSpawned()) { if (ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get()); C && C->CrewType == ETDCrewType::Warden) { Warden = C; } }
		if (!TestNotNull(*FString::Printf(TEXT("take %d: Warden spawned"), E.Take), Warden)) { return false; }
		TestEqual(*FString::Printf(TEXT("take %d is the fight's take"), E.Take), Arena->GetTake(), E.Take);
		TestEqual(*FString::Printf(TEXT("take %d: Warden health"), E.Take), Warden->GetHealth()->MaxHull, FullHealth * E.Scale, 0.5f);
		TestEqual(*FString::Printf(TEXT("take %d: summons allowed"), E.Take), Warden->GetSummonsLeft(), E.Summons);
		if (E.Summons == 0)
		{
			if (AController* AI = Warden->GetController()) { AI->UnPossess(); }
			TestFalse(TEXT("take 5: the Warden can't summon"), Warden->StartAttack(ETDCrewAttack::Summon, R.FP));
		}
		// Lose this take for real: CUT! and back to the arena door.
		V.GI->ReportPlayerDefeated();
		R.Tick(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.5f);
		TestEqual(*FString::Printf(TEXT("take %d lost: still at the Warden"), E.Take), R.Step(Outpost), 4);
	}
	return true;
}

// ================================================================================================
// Review G6: each planet PLAYED by the machine: a vulnerable pilot, real keys for movement (W, Shift,
// Space, Q), real blade attacks (LMB), real interact holds (E), along the real route. The only
// non-input help is turning the view (the mouse). If a step can't be done this way, a player can't either.
// ================================================================================================
namespace TDPlayedTest
{
	struct FPlayer
	{
		TDPlanetTest::FVisit& V;
		FAutomationTestBase& T;
		float Fps = 60.f;
		ATDFirstPersonCharacter* FP = nullptr;
		ATDPlanetCourse* Course = nullptr;
		bool bW = false, bShift = false;
		int32 Deaths = 0;
		FString Failed;

		FPlayer(TDPlanetTest::FVisit& InV, FAutomationTestBase& InT, float InFps) : V(InV), T(InT), Fps(InFps) {}

		APlayerController* PC() const { return V.PC; }
		void Tick(float Seconds)
		{
			const float Dt = 1.f / Fps;
			for (float S = 0.f; S < Seconds; S += Dt)
			{
				V.W.Tick(Dt);
				V.GI->TickFlow(Dt);
				if (FP && FP->IsDead())
				{
					FString Foes;
					for (TActorIterator<ATDCrewCharacter> It(V.W.GetWorld()); It; ++It)
					{
						const UTDHealthComponent* H = UTDStatics::GetHealth(*It);
						if (H && H->IsAlive() && FVector::Dist(It->GetActorLocation(), FP->GetActorLocation()) < 3000.f)
						{
							Foes += FString::Printf(TEXT("%s %.0f/%.0f at %.0f cm; "), *UEnum::GetValueAsString(It->CrewType), H->GetHull(), H->MaxHull, FVector::Dist(It->GetActorLocation(), FP->GetActorLocation()));
						}
					}
					T.AddInfo(FString::Printf(TEXT("death %d at %s: %s"), Deaths + 1, *FP->GetActorLocation().ToCompactString(), *Foes));
					++Deaths; Release(); WaitRetake();
				}
			}
		}
		void WaitRetake()
		{
			const float Dt = 1.f / Fps;
			for (float S = 0.f; S < UTDFlowSettings::Get()->RetakeDelaySeconds + 1.f; S += Dt) { V.W.Tick(Dt); V.GI->TickFlow(Dt); }
		}
		void Key(const FKey& K, bool bDown) { FTDTestWorld::InjectKey(PC(), K, bDown); }
		void Tap(const FKey& K) { Key(K, true); V.W.Tick(1.f / Fps); Key(K, false); }
		void HoldW(bool bDown) { if (bW != bDown) { Key(EKeys::W, bDown); bW = bDown; } }
		void HoldShift(bool bDown) { if (bShift != bDown) { Key(EKeys::LeftShift, bDown); bShift = bDown; } }
		bool bBlock = false;
		int32 HeavyCount = 0;
		float DodgeCooldown = 0.f;
		float Retreat = 0.f;
		int32 DodgeCount = 0;
		void HoldBlock(bool bDown) { if (bBlock != bDown) { Key(EKeys::RightMouseButton, bDown); bBlock = bDown; } }
		void Release() { HoldW(false); HoldShift(false); }
		FVector Eye() const { return FP->GetPawnViewLocation(); }
		void LookAt(const FVector& P) { PC()->SetControlRotation((P - Eye()).Rotation()); }
		void Face(const FVector& P) { PC()->SetControlRotation(FRotator(0.f, (P - FP->GetActorLocation()).Rotation().Yaw, 0.f)); }

		bool Land()
		{
			if (!T.TestTrue(TEXT("lands"), V.LandForReal())) { return false; }
			FP = V.Director->GetFirstPerson();
			Course = V.Director->GetCourse();
			Tick(0.3f);
			return FP && Course;
		}

		/** Run (W + Shift) to a point on the ground; hops when blocked. False on timeout. */
		bool RunTo(const FVector& Target, float Radius = 160.f, float Timeout = 40.f, bool bSprint = true)
		{
			float Stall = 0.f, Best = 1e12f;
			for (float S = 0.f; S < Timeout; S += 1.f / Fps)
			{
				const float D = (float)FVector::Dist2D(FP->GetActorLocation(), Target);
				if (D < Radius) { Release(); return true; }
				Face(Target);
				HoldW(true);
				HoldShift(bSprint && !FP->IsCarrying());
				if (D < Best - 10.f) { Best = D; Stall = 0.f; }
				else if ((Stall += 1.f / Fps) > 0.6f) { Tap(EKeys::SpaceBar); Stall = 0.f; }
				Tick(1.f / Fps);
			}
			Release();
			Failed = FString::Printf(TEXT("could not reach %s (stopped %.0f cm away at %s)"), *Target.ToCompactString(), Best, *FP->GetActorLocation().ToCompactString());
			return false;
		}

		/** Fight an encounter with the blade until it is cleared (approach, swing, re-face). */
		bool Fight(ATDEncounterVolume* E, float Timeout = 120.f)
		{
			if (E && !E->IsActive() && !E->IsCleared())
			{
				Failed = FString::Printf(TEXT("fight %s never started (player at %s)"), *E->GetName(), *FP->GetActorLocation().ToCompactString());
				return false;
			}
			float Swing = 0.f;
			const int32 DeathsAtStart = Deaths;
			for (float S = 0.f; S < Timeout && E && !E->IsCleared(); S += 1.f / Fps)
			{
				if (Deaths > DeathsAtStart)
				{
					Release();
					HoldBlock(false);
					Failed = FString::Printf(TEXT("pilot died in %s (the scene restarted)"), *E->GetName());
					return false; // the retake reset the room: the caller takes it again
				}
				AActor* Best = nullptr;
				float BestD = 1e12f;
				for (const TWeakObjectPtr<AActor>& M : E->GetSpawned())
				{
					const UTDHealthComponent* H = M.IsValid() ? UTDStatics::GetHealth(M.Get()) : nullptr;
					const float D = H && H->IsAlive() ? (float)FVector::Dist(M->GetActorLocation(), FP->GetActorLocation()) : 1e12f;
					if (D < BestD) { BestD = D; Best = M.Get(); }
				}
				if (!Best) { Tick(1.f / Fps); continue; }
				LookAt(Best->GetActorLocation());
				const float Flat = (float)FVector::Dist2D(Best->GetActorLocation(), FP->GetActorLocation());
				const ATDCrewCharacter* Foe = Cast<ATDCrewCharacter>(Best);
				const bool bAirborne = Best->GetActorLocation().Z - FP->GetActorLocation().Z > 150.f;
				const bool bTelegraph = Foe && Foe->IsTelegraphing() && BestD < 500.f;
				// Like a player: a hovering drone is beaten by turning its bolts back (block while looking at it);
				// a telegraphed blow is blocked, not swung into; otherwise guard up while closing in, down to swing.
				// Low on health: back off out of the fight until the regen has done its work, like a player.
				if (false && FP->GetHealth()->GetHull01() < 0.35f && Retreat <= 0.f) // retreating made the bot worse (it bled out at range)
				{
					Retreat = 7.f;
				}
				if (Retreat > 0.f)
				{
					Retreat -= 1.f / Fps;
					HoldBlock(true);
					// Back toward where we came in (the room's entry side), never into a corner.
					const FVector Box = E->Box ? E->Box->GetScaledBoxExtent() : FVector(1000.f);
					Face(E->GetActorLocation() - FVector(Box.X - 200.f, 0.f, 0.f));
					HoldW(true);
					Tick(1.f / Fps);
					if (Retreat <= 0.f || FP->GetHealth()->GetHull01() > 0.8f) { Retreat = 0.f; HoldBlock(false); }
					continue;
				}
				// The Warden's telegraphed charge / fan: sidestep with a dash (i-frames), like a player.
				if (Foe && Foe->CrewType == ETDCrewType::Warden && Foe->IsTelegraphing() && BestD < 1400.f && (DodgeCooldown -= 1.f / Fps) <= 0.f)
				{
					HoldW(false);
					const FKey Side = (++DodgeCount % 2) ? EKeys::D : EKeys::A;
					Key(Side, true);
					Tap(EKeys::Q);
					Tick(0.25f);
					Key(Side, false);
					DodgeCooldown = 1.f;
					continue;
				}
				(void)bTelegraph;
				// A drone: get under it and jump-slash.
				if (bAirborne && Flat < 260.f && FP->GetCharacterMovement()->IsMovingOnGround())
				{
					Tap(EKeys::SpaceBar);
				}
				HoldW(Flat > 170.f);
				HoldShift(false);
				HoldBlock(BestD >= 320.f);
				if ((Swing -= 1.f / Fps) <= 0.f && BestD < 320.f)
				{
					const ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(Best);
					if (Crew && Crew->CrewType == ETDCrewType::Heavy && (++HeavyCount % 2) == 0)
					{
						// A Heavy's guard only breaks to the heavy lunge: hold the attack (like a player learns to).
						Key(EKeys::LeftMouseButton, true);
						Tick(0.45f);
						Key(EKeys::LeftMouseButton, false);
						Swing = 0.5f;
					}
					else
					{
						Tap(EKeys::LeftMouseButton);
						Swing = 0.35f;
					}
				}
				Tick(1.f / Fps);
			}
			Release();
			HoldBlock(false);
			if (E && !E->IsCleared())
			{
				Failed = FString::Printf(TEXT("fight %s not cleared (%d alive, %d spawned, active %d, room step %d, objective step %d)"), *E->GetName(), E->GetAliveCount(),
					E->GetSpawned().Num(), E->IsActive(), E->LinkedStep, E->LinkedObjective ? E->LinkedObjective->GetCurrentStepIndex() : -9);
				for (const TWeakObjectPtr<AActor>& M : E->GetSpawned())
				{
					if (const ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get()); C && !C->IsDead())
					{
						Failed += FString::Printf(TEXT(" [%s alive at %s, pilot at %s]"), *UEnum::GetValueAsString(C->CrewType), *C->GetActorLocation().ToCompactString(),
							*FP->GetActorLocation().ToCompactString());
					}
				}
				return false;
			}
			return true;
		}

		/** Walk up to an interactable, look at it and hold E (tap when Hold is 0). */
		bool Use(AActor* A, float Hold, const FVector& LookPoint = FVector::ZeroVector)
		{
			// Stand at the console's side, as a player does (consoles have a steep cap: nobody stands on one).
			FVector Origin, Extent;
			A->GetActorBounds(true, Origin, Extent);
			const float Half = (float)FMath::Max(Extent.X, Extent.Y);
			FVector Side = (FP->GetActorLocation() - A->GetActorLocation()).GetSafeNormal2D();
			if (Side.IsNearlyZero()) { Side = FVector(-1.f, 0.f, 0.f); }
			if (!RunTo(A->GetActorLocation() + Side * (Half + 70.f), 80.f, 30.f, false)) { return false; }
			LookAt(LookPoint.IsZero() ? A->GetActorLocation() : LookPoint);
			Tick(0.1f);
			Key(EKeys::E, true);
			Tick(Hold + 0.3f);
			Key(EKeys::E, false);
			Tick(0.2f);
			return true;
		}

		/** A console inside a kit outpost room (Outpost_Site_*.manifest.json): in through the door on the room's +X, as a
		 *  player walks it (outside -> doorway -> the stand point), then E. The interact box sits at site (-353, 0, 53.3)
		 *  in the room's frame, which is the box's own rotation. */
		bool UseKitConsole(AActor* Box, float Hold)
		{
			const FRotator R(0.f, Box->GetActorRotation().Yaw, 0.f);
			const FVector Site = Box->GetActorLocation() - R.RotateVector(FVector(-353.f, 0.f, 53.3f));
			for (const FVector& L : { FVector(700.f, 0.f, 0.f), FVector(400.f, 0.f, 0.f), FVector(-250.f, 0.f, 0.f) })
			{
				if (!RunTo(Site + R.RotateVector(L), 60.f, 20.f, false)) { Failed = FString::Printf(TEXT("could not walk into the outpost to %s"), *L.ToString()); return false; }
			}
			LookAt(Box->GetActorLocation());
			Tick(0.1f);
			Key(EKeys::E, true);
			Tick(Hold + 0.3f);
			Key(EKeys::E, false);
			Tick(0.2f);
			// And back out through the door (the room's walls stand between the console and everywhere else).
			for (const FVector& L : { FVector(400.f, 0.f, 0.f), FVector(800.f, 0.f, 0.f) }) { RunTo(Site + R.RotateVector(L), 80.f, 15.f, false); }
			return true;
		}

		/** A charge point on the field anchor (just outside the tower's collision on its port's heading): round the tower
		 *  at 9.5 m, as a player does, to the port's side, then in to the point, look at it and hold E. A straight line from
		 *  the far side runs into the tower. */
		bool UseChargePort(ATDInteractableActor* Port, const FVector& Tower)
		{
			const FVector P0 = FP->GetActorLocation() - Tower;
			const float From = FMath::Atan2((float)P0.Y, (float)P0.X);
			const FVector D = (Port->GetActorLocation() - Tower).GetSafeNormal2D();
			const float To = FMath::Atan2((float)D.Y, (float)D.X);
			const float Turn = FMath::FindDeltaAngleRadians(From, To);
			const int32 Steps = FMath::Max(1, FMath::CeilToInt(FMath::Abs(Turn) / (PI / 6.f)));
			for (int32 i = 1; i <= Steps; ++i)
			{
				const float A = From + Turn * i / Steps;
				RunTo(Tower + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * 1150.f, 120.f, 10.f, false);
			}
			RunTo(FVector(Port->GetActorLocation().X, Port->GetActorLocation().Y, 0.f) + D * 130.f, 60.f, 10.f, false); // in front of the charge point
			LookAt(Port->GetActorLocation());
			Tick(0.1f);
			Key(EKeys::E, true);
			Tick(Port->HoldTime + 0.3f);
			Key(EKeys::E, false);
			Tick(0.2f);
			RunTo(Tower + D * 1150.f, 120.f, 10.f, false); // back off
			return Port->WasUsed();
		}

		/** Break a prop with the blade (real swings). */
		bool Break(ATDShatterCrystal* C)
		{
			if (!RunTo(C->GetActorLocation(), 150.f, 30.f)) { return false; }
			for (int32 i = 0; i < 12 && !C->IsShattered(); ++i)
			{
				LookAt(C->GetActorLocation() + FVector(0.f, 0.f, 120.f));
				Tap(EKeys::LeftMouseButton);
				Tick(0.4f);
			}
			if (!C->IsShattered()) { Failed = FString::Printf(TEXT("%s not broken by the blade"), *C->GetName()); }
			return C->IsShattered();
		}

		/** Jump from where we stand toward a point (running jump, optional double jump / dash). */
		void JumpTo(const FVector& Target, bool bDouble, bool bDash, float Run = 1.f)
		{
			Face(Target);
			HoldW(true);
			Tick(0.05f);
			Tap(EKeys::SpaceBar);
			if (bDouble) { Tick(0.3f); Tap(EKeys::SpaceBar); }
			if (bDash) { Tick(0.15f); Tap(EKeys::Q); }
			Tick(Run);
			Release();
			Tick(0.3f);
		}

		ATDEncounterVolume* FightOf(const ATDObjective* Obj, int32 Step)
		{
			for (TActorIterator<ATDEncounterVolume> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Obj && It->LinkedStep == Step) { return *It; } }
			return nullptr;
		}
		ATDEncounterVolume* ActiveFightNear(const FVector& P, float Radius)
		{
			for (TActorIterator<ATDEncounterVolume> It(V.W.GetWorld()); It; ++It)
			{
				if (It->IsActive() && FVector::Dist2D(It->GetActorLocation(), P) < Radius) { return *It; }
			}
			return nullptr;
		}
		FVector Trailhead(FName Id) const
		{
			for (const FTDTrailhead& H : Course->GetTrailheads()) { if (H.Objective && H.Objective->ObjectiveId == Id && H.Marker) { return H.Marker->GetActorLocation(); } }
			return FVector::ZeroVector;
		}
		void Trace(const ATDObjective* O, int32 Try)
		{
			FString Doors;
			for (TActorIterator<ATDDoor> It(V.W.GetWorld()); It; ++It) { Doors += FString::Printf(TEXT("%.0f:%d "), It->GetActorLocation().X, It->IsOpen()); }
			T.AddInfo(FString::Printf(TEXT("%s try %d: step %d (checkpoint %d), pilot at %s, hull %.2f, deaths %d, dead %d, doors %s"), *O->ObjectiveId.ToString(), Try,
				O->GetCurrentStepIndex(), O->GetCheckpointStep(), *FP->GetActorLocation().ToCompactString(), FP->GetHealth()->GetHull01(), Deaths, FP->IsDead(), *Doors));
		}
		bool Report(bool bOk, const TCHAR* What)
		{
			T.TestTrue(*FString::Printf(TEXT("played: %s%s%s (deaths so far %d)"), What, Failed.IsEmpty() ? TEXT("") : TEXT(" - "), *Failed, Deaths), bOk);
			Failed.Reset();
			return bOk;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVerdancePlayed, "TriadDrift.Ground.Played.Verdance", TD_TEST_FLAGS)
bool FTDVerdancePlayed::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(NAME_None, TEXT("Verdance"), true, {}, /*bLocalController*/ true);
	TDPlayedTest::FPlayer P(V, *this, 60.f);
	if (!P.Land()) { return false; }
	ATDObjective* Wreck = P.Course->FindObjective(TEXT("Obj_WreckCore"));
	ATDObjective* Relay = P.Course->FindObjective(TEXT("Obj_Relay"));
	const float Start = V.W.GetWorld()->GetTimeSeconds();

	// --- The wreck (decision 2, the fixed order first): the ambush on the way, the site fight, the device at the console. ---
	bool bOk = P.RunTo(P.Trailhead(TEXT("Obj_WreckCore")), 300.f, 60.f);
	P.Report(bOk && Wreck->IsActive(), TEXT("wreck started at its trailhead"));
	for (int32 Try = 0; Try < 6 && !Wreck->IsComplete(); ++Try)
	{
		P.Trace(Wreck, Try);
		if (Wreck->GetCurrentStepIndex() == 0)
		{
			P.RunTo(FVector(6200.f, 4600.f, 0.f), 400.f);
			if (ATDEncounterVolume* Ambush = P.ActiveFightNear(FVector(6200.f, 4600.f, 0.f), 1500.f)) { P.Fight(Ambush); }
			P.RunTo(FVector(9300.f, 6300.f, 0.f), 200.f);
		}
		if (Wreck->GetCurrentStepIndex() == 1) { P.Fight(P.FightOf(Wreck, 1)); }
		if (Wreck->GetCurrentStepIndex() == 2)
		{
			for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It)
			{
				if (It->LinkedObjective == Wreck && It->LinkedStep == 2) { P.UseKitConsole(*It, It->HoldTime); } // in through the outpost's door
			}
		}
	}
	P.Report(Wreck->IsComplete(), TEXT("signal-analysis device recovered at the survey console"));

	// --- The relay: run there, connect the device, hold the deck through the transfer, back to the ship. ---
	bOk = P.RunTo(P.Trailhead(TEXT("Obj_Relay")), 300.f, 60.f);
	P.Report(bOk && Relay->IsActive(), TEXT("relay started at its trailhead"));
	const FVector RelayDeck(8000.f, -6500.f, 0.f);
	for (int32 Try = 0; Try < 6 && !Relay->IsComplete(); ++Try)
	{
		P.Trace(Relay, Try);
		if (Relay->GetCurrentStepIndex() == 0) { P.RunTo(RelayDeck + FVector(-1400.f, 0.f, 0.f), 200.f, 60.f); P.Tick(0.3f); }
		if (Relay->GetCurrentStepIndex() == 1)
		{
			for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It)
			{
				if (It->LinkedObjective == Relay && It->LinkedStep == 1) { P.Use(*It, It->HoldTime, It->GetActorLocation()); }
			}
		}
		// The transfer: fight each wave as it drops, keeping them off the console. A death is a retake, not a failure.
		for (float Tm = 0.f; Tm < 150.f && (Relay->GetCurrentStepIndex() == 2 || Relay->GetCurrentStepIndex() == 3); Tm += 1.f)
		{
			bool bFought = false;
			if (ATDArchiveTransfer* X = P.Course->GetArchiveTransfer())
			{
				for (ATDEncounterVolume* E : X->Waves) { if (E && E->IsActive()) { if (!P.Fight(E, 60.f)) { AddInfo(FString::Printf(TEXT("transfer wave: %s"), *P.Failed)); P.Failed.Reset(); } bFought = true; } }
				if (!bFought && X->IsPaused())
				{
					if (ATDEncounterVolume* Other = P.ActiveFightNear(X->GetActorLocation(), 2500.f)) { P.Fight(Other, 60.f); bFought = true; }
				}
			}
			if (!bFought) { P.Tick(1.f); }
		}
		if (Relay->GetCurrentStepIndex() == 4) { P.RunTo(FVector(300.f, 0.f, 0.f), 300.f, 60.f, false); P.Tick(0.5f); }
	}
	P.Report(Relay->IsComplete(), TEXT("survey archive transferred at the relay and back at the ship"));
	TestEqual(TEXT("Gravity Well on Bastion"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);
	TestEqual(TEXT("Cloak on Rift"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::Cloak), ETDShipId::Rift);

	// --- Back aboard. ---
	if (ATDBoardShipInteractable* Board = V.Director->GetBoardPoint()) { P.Use(Board, Board->HoldTime); }
	P.Tick(1.f);
	TestFalse(TEXT("played: re-boarded"), V.Director->IsOnFoot());
	AddInfo(FString::Printf(TEXT("Verdance played in %.0f s of game time with %d deaths"), V.W.GetWorld()->GetTimeSeconds() - Start, P.Deaths));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCinderPlayed, "TriadDrift.Ground.Played.Cinder", TD_TEST_FLAGS)
bool FTDCinderPlayed::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V(NAME_None, TEXT("Cinder"), true, {}, /*bLocalController*/ true);
	TDPlayedTest::FPlayer P(V, *this, 60.f);
	if (!P.Land()) { return false; }
	ATDObjective* Outpost = P.Course->FindObjective(TEXT("Obj_Outpost"));
	ATDObjective* Jammer = P.Course->FindObjective(TEXT("Obj_Jammer"));
	const float Start = V.W.GetWorld()->GetTimeSeconds();
	const float HullAtStart = P.FP->GetHealth()->GetHull01();

	// --- The survey records: the marked route past the patrol to the station, the terminal once it is clear. ---
	ATDObjective* Survey = P.Course->FindObjective(TEXT("Obj_Survey"));
	P.Report(P.RunTo(P.Trailhead(TEXT("Obj_Survey")), 300.f) && Survey && Survey->IsActive(), TEXT("survey started at its trailhead"));
	for (int32 Try = 0; Try < 4 && Survey && !Survey->IsComplete(); ++Try)
	{
		if (ATDEncounterVolume* Patrol = P.ActiveFightNear(FVector(2300.f, 0.f, 0.f), 1500.f)) { P.Fight(Patrol); }
		P.RunTo(FVector(2900.f, 0.f, 0.f), 200.f, 20.f);
		if (ATDEncounterVolume* Patrol = P.ActiveFightNear(FVector(2300.f, 0.f, 0.f), 1500.f)) { P.Fight(Patrol); }
		if (ATDSurveyTerminal* T = P.Course->GetSurveyTerminal()) { P.UseKitConsole(T, T->HoldTime); }
		P.Tick(0.3f);
	}
	P.Report(Survey && Survey->IsComplete(), TEXT("survey records accessed after the patrol"));

	// --- The outpost: the stepping stones over the magma river (real jumps), conduits, three rooms, the Warden. ---
	P.Report(P.RunTo(P.Trailhead(TEXT("Obj_Outpost")), 300.f, 60.f) && Outpost->IsActive(), TEXT("outpost started at its trailhead"));
	const float HullBeforeRiver = P.FP->GetHealth()->GetHull01();
	P.RunTo(FVector(4700.f, 4200.f, 0.f), 80.f, 20.f, false);
	for (int32 i = 0; i < 3; ++i) { P.JumpTo(FVector(5050.f + i * 550.f, 4400.f + i * 300.f, 0.f), false, false, 0.5f); }
	P.JumpTo(FVector(6700.f, 5200.f, 0.f), false, false, 0.6f);
	P.Report(P.FP->GetHealth()->GetHull01() >= HullBeforeRiver - 0.01f, TEXT("crossed the river on the stones without touching magma"));
	for (int32 Try = 0; Try < 14 && !Outpost->IsComplete(); ++Try)
	{
		P.Trace(Outpost, Try);
		const int32 Step = Outpost->GetCurrentStepIndex();
		if (P.FP->GetActorLocation().X < 4800.f) // back at the pad after a retake: the stones again
		{
			P.RunTo(FVector(4700.f, 4200.f, 0.f), 80.f, 40.f, false);
			for (int32 i = 0; i < 3; ++i) { P.JumpTo(FVector(5050.f + i * 550.f, 4400.f + i * 300.f, 0.f), false, false, 0.5f); }
			P.JumpTo(FVector(6700.f, 5200.f, 0.f), false, false, 0.6f);
		}
		if (Step == 0)
		{
			P.RunTo(FVector(9600.f - 1400.f, 6000.f, 0.f), 300.f, 40.f);
			if (ATDEncounterVolume* Guards = P.ActiveFightNear(FVector(9600.f - 1400.f, 6000.f, 0.f), 1500.f)) { P.Fight(Guards); }
			for (ATDShatterCrystal* C : TArray<ATDShatterCrystal*>(TDCourseTest::FRun(V, *this).Props<ATDShatterCrystal>(Outpost)))
			{
				if (!C->IsShattered()) { P.Break(C); }
			}
			P.Tick(1.f); // the gate lifts
		}
		else if (Step >= 1 && Step <= 4)
		{
			ATDEncounterVolume* Room = P.FightOf(Outpost, Step);
			P.RunTo(Room->GetActorLocation(), 400.f, 40.f);
			P.Fight(Room, 150.f);
			P.Tick(1.f);
		}
		else if (Step == 5)
		{
			for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Outpost && It->LinkedStep == 5) { P.Use(*It, It->HoldTime); } }
		}
		P.Tick(0.2f);
	}
	P.Report(Outpost->IsComplete(), TEXT("outpost cleared, Warden down, prototype claimed"));
	TestEqual(TEXT("EMP on Lancer"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::EMPPulse), ETDShipId::Lancer);
	// Back to the ship: out through the gate and back over the stones (never through the magma).
	P.RunTo(FVector(9000.f, 6000.f, 0.f), 300.f, 60.f);
	P.RunTo(FVector(6700.f, 5200.f, 0.f), 80.f, 30.f, false);
	for (int32 i = 2; i >= 0; --i) { P.JumpTo(FVector(5050.f + i * 550.f, 4400.f + i * 300.f, 0.f), false, false, 0.5f); }
	P.JumpTo(FVector(4600.f, 4100.f, 0.f), false, false, 0.6f);
	// --- The jammer: slash the crystal wall, run the collapsing bridge, plant under fire, outrun the tower. ---
	P.Report(P.RunTo(P.Trailhead(TEXT("Obj_Jammer")), 300.f) && Jammer->IsActive(), TEXT("jammer started at its trailhead"));
	for (int32 Try = 0; Try < 4 && !Jammer->IsComplete(); ++Try)
	{
		if (Jammer->GetCurrentStepIndex() == 0)
		{
			for (ATDShatterCrystal* C : TArray<ATDShatterCrystal*>(TDCourseTest::FRun(V, *this).Props<ATDShatterCrystal>(Jammer)))
			{
				if (!C->IsShattered()) { P.Report(P.Break(C), TEXT("crystal wall slashed with the blade")); }
			}
			P.RunTo(FVector(6800.f, -5000.f, 0.f), 150.f);
			// The bridge: one straight sprint along the slabs (they fall 1.1 s after you step on them).
			P.RunTo(FVector(10400.f + 700.f, -5000.f, 0.f), 200.f, 20.f);
		}
		if (Jammer->GetCurrentStepIndex() == 1)
		{
			bool bDied = false;
			for (TActorIterator<ATDEncounterVolume> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Jammer && It->IsActive()) { bDied |= !P.Fight(*It); } }
			if (bDied) { continue; } // the retake restarted the defence
			for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It)
			{
				if (It->LinkedObjective == Jammer && It->LinkedStep == 1 && !It->WasUsed()) { P.UseChargePort(*It, FVector(12600.f, -5000.f, 0.f)); }
			}
		}
		if (Jammer->GetCurrentStepIndex() == 2)
		{
			P.RunTo(FVector(10400.f + 800.f, -5000.f - 3300.f, 0.f), 300.f, 15.f);
			P.RunTo(FVector(7000.f - 600.f, -5000.f - 3400.f, 0.f), 300.f, 15.f);
			P.Tick(0.3f);
		}
	}
	P.Report(Jammer->IsComplete(), TEXT("jammer destroyed and outrun"));

	if (ATDBoardShipInteractable* Board = V.Director->GetBoardPoint())
	{
		// T1: the parked ship is solid now (Ryan, Nov 13). Walk round its nose to the prompt's side, as a player would.
		if (ATDShip* Parked = V.Ship())
		{
			const FBox Hull = UTDLandingComponent::VisibleHullBoxLocal(Parked);
			const FVector C = Parked->GetActorLocation(), Fwd = Parked->GetActorForwardVector().GetSafeNormal2D();
			const FVector Out = (Board->GetActorLocation() - C).GetSafeNormal2D();
			const float Reach = (Hull.IsValid ? Hull.GetExtent().Size2D() : 500.f) + 400.f;
			P.RunTo(C + Fwd * Reach - Out * Reach, 250.f, 30.f);   // the far side's front corner
			P.RunTo(C + Fwd * Reach + Out * Reach, 250.f, 30.f);   // across in front of the nose
		}
		P.RunTo(Board->GetActorLocation(), 250.f, 30.f);
		P.Use(Board, Board->HoldTime);
	}
	P.Tick(1.f);
	{
		ATDBoardShipInteractable* B = V.Director->GetBoardPoint();
		ATDShip* S = V.Ship();
		AddInfo(FString::Printf(TEXT("re-board: failed '%s'; player %s; board %s (enabled %d, can %d); ship %s landed %d block %d"), *P.Failed,
			*P.FP->GetActorLocation().ToCompactString(), B ? *B->GetActorLocation().ToCompactString() : TEXT("-"), B ? (int32)B->bEnabled : -1,
			B ? (int32)B->CanInteract(P.FP) : -1, S ? *S->GetActorLocation().ToCompactString() : TEXT("-"),
			S && S->GetLanding() ? (int32)S->GetLanding()->IsLanded() : -1, S && S->GetLanding() ? (int32)S->GetLanding()->IsParkedBlockOn() : -1));
	}
	TestFalse(TEXT("played: re-boarded"), V.Director->IsOnFoot());
	AddInfo(FString::Printf(TEXT("Cinder played in %.0f s of game time with %d deaths (hull at start %.2f)"), V.W.GetWorld()->GetTimeSeconds() - Start, P.Deaths, HullAtStart));
	return true;
}


// Dressing: both planets use engineering's real props and trees (instanced), and nothing that collides
// stands on a route.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseDressing, "TriadDrift.Ground.Course.DressedWithRealPropsRoutesClear", TD_TEST_FLAGS)
bool FTDCourseDressing::RunTest(const FString& Parameters)
{
	for (const TCHAR* Planet : { TEXT("Verdance"), TEXT("Cinder") })
	{
		TDPlanetTest::FVisit V(NAME_None, Planet);
		ATDPlanetCourse* Course = V.Director->GetCourse();
		if (!TestNotNull(TEXT("course"), Course)) { return false; }
		int32 Kinds = 0, Instances = 0, Blocking = 0;
		for (const TObjectPtr<AActor>& A : Course->GetSpawned())
		{
			if (!A) { continue; }
			TInlineComponentArray<UInstancedStaticMeshComponent*> ISMs(A.Get());
			for (UInstancedStaticMeshComponent* ISM : ISMs)
			{
				if (!ISM->GetStaticMesh() || !ISM->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Game/TriadDrift/Gameplay/"))) { continue; }
				++Kinds;
				Instances += ISM->GetInstanceCount();
				if (ISM->GetCollisionEnabled() == ECollisionEnabled::NoCollision) { continue; }
				for (int32 i = 0; i < ISM->GetInstanceCount(); ++i)
				{
					FTransform X;
					ISM->GetInstanceTransform(i, X, false);
					Blocking += Course->IsRouteBlockedAt(X.GetLocation(), 800.f) ? 1 : 0;
				}
			}
		}
		TestTrue(FString::Printf(TEXT("%s: real props in use (%d kinds, %d instances)"), Planet, Kinds, Instances), Kinds >= 9 && Instances > 300);
		TestEqual(FString::Printf(TEXT("%s: nothing that collides within 8 m of a route"), Planet), Blocking, 0);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

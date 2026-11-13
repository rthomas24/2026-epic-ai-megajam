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
	V.GI->TickFlow(UTDFlowSettings::Get()->TravelFadeSeconds + 0.1f);
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
		bool StartAt(FName Id)
		{
			for (const FTDTrailhead& H : Course->GetTrailheads())
			{
				if (H.Objective && H.Objective->ObjectiveId == Id && H.Marker)
				{
					TeleportTo(H.Marker, -150.f);
					Tick(0.5f);
					return T.TestTrue(*FString::Printf(TEXT("%s starts at its trailhead (own scene)"), *Id.ToString()),
						H.Objective->IsActive() && V.GI->GetCurrentScene() == Id);
				}
			}
			return false;
		}
		int32 Step(const ATDObjective* O) const { return O ? O->GetCurrentStepIndex() : -2; }
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
			V.GI->TickFlow(UTDFlowSettings::Get()->TravelFadeSeconds + 0.1f);
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
	TestTrue(*FString::Printf(TEXT("idle line offers both ('%s')"), *R.Line()), R.Line().Contains(TEXT("THE WRECK")) && R.Line().Contains(TEXT("THE RELAY")));
	TestEqual(TEXT("both trailheads marked"), V.GI->GetFlowMarkers().Num(), 2);

	ATDObjective* Relay = R.Course->FindObjective(TEXT("Obj_Relay"));
	ATDObjective* Wreck = R.Course->FindObjective(TEXT("Obj_WreckCore"));
	if (!TestNotNull(TEXT("relay"), Relay) || !TestNotNull(TEXT("wreck"), Wreck)) { return false; }

	// Either order: the relay first.
	if (!R.StartAt(TEXT("Obj_Relay"))) { return false; }
	TestFalse(TEXT("the other trailhead waits (one scene at a time)"), R.Course->StartObjectiveAt(TEXT("Obj_WreckCore")));
	R.TeleportTo(R.Props<ATDStepTrigger>(Relay, 0)[0], 60.f);
	TestEqual(TEXT("on the plateau -> nodes"), R.Step(Relay), 1);
	R.Tick(0.5f);
	TestTrue(*FString::Printf(TEXT("node count on the line ('%s')"), *R.Line()), R.Line().Contains(TEXT("0 / 3")));
	TArray<ATDInteractableActor*> Nodes;
	for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Relay && It->LinkedStep == 1) { Nodes.Add(*It); } }
	TestEqual(TEXT("three nodes"), Nodes.Num(), 3);
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		R.Use(Nodes[i]);
		if (i == 0) { TestTrue(*FString::Printf(TEXT("count moves ('%s')"), *R.Line()), R.Line().Contains(TEXT("1 / 3"))); }
	}
	TestEqual(TEXT("nodes awake -> the relay"), R.Step(Relay), 2);
	ATDInteractableActor* RelayCore = nullptr;
	for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Relay && It->LinkedStep == 2) { RelayCore = *It; } }
	if (!TestNotNull(TEXT("relay core"), RelayCore)) { return false; }
	R.Use(RelayCore, RelayCore->HoldTime);
	TestTrue(TEXT("relay done"), Relay->IsComplete());
	TestEqual(TEXT("Cloak on Rift"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::Cloak), ETDShipId::Rift);
	R.Tick(0.5f);
	TestTrue(*FString::Printf(TEXT("then the wreck is offered ('%s')"), *R.Line()), R.Line().Contains(TEXT("THE WRECK")));

	// The wreck core.
	if (!R.StartAt(TEXT("Obj_WreckCore"))) { return false; }
	R.TeleportTo(R.Props<ATDStepTrigger>(Wreck, 0)[0], 60.f);
	TestEqual(TEXT("at the beacon -> clear the site"), R.Step(Wreck), 1);
	ATDEncounterVolume* Site = R.StepFight(Wreck, 1);
	TestTrue(TEXT("site fight running"), Site && Site->IsActive());
	R.KillAll(Site);
	TestEqual(TEXT("site clear -> extract"), R.Step(Wreck), 2);
	ATDExtractionSite* Core = nullptr;
	for (TActorIterator<ATDExtractionSite> It(V.W.GetWorld()); It; ++It) { Core = *It; }
	if (!TestNotNull(TEXT("core"), Core)) { return false; }
	TestTrue(TEXT("first wave drops in"), Core->WaveA && Core->WaveA->IsActive());
	{
		// Damage must not break this hold. Aim at the glowing core (what the eye goes to), not the console.
		const FVector Target = Core->Core->GetComponentLocation();
		R.FP->RespawnAt(FTransform(FRotator::ZeroRotator, Target + FVector(-160.f, 0.f, 40.f)));
		R.Tick(0.2f);
		V.PC->SetControlRotation((Target - R.FP->FindComponentByClass<UCameraComponent>()->GetComponentLocation()).Rotation());
		R.Tick(0.1f);
		R.FP->BeginInteract();
		R.Tick(1.f);
		TestTrue(TEXT("second wave drops when the hold starts"), Core->WaveB && Core->WaveB->IsActive());
		R.FP->GetHealth()->SetInvulnerable(false);
		UTDStatics::ApplyTDDamage(R.FP, 5.f, ETDDamageKind::Energy, nullptr, nullptr);
		R.FP->GetHealth()->SetInvulnerable(true);
		TestTrue(TEXT("hold survives damage"), R.FP->GetHoldTarget() == Core);
		R.Tick(Core->HoldTime);
		R.FP->EndInteract();
	}
	TestTrue(TEXT("carrying the core"), R.FP->IsCarrying());
	TestEqual(TEXT("carry step"), R.Step(Wreck), 3);
	R.FP->RespawnAt(FTransform(FVector(300.f, 0.f, 120.f)));
	R.Tick(0.5f);
	TestTrue(TEXT("wreck done at the ship"), Wreck->IsComplete());
	TestFalse(TEXT("hands free again"), R.FP->IsCarrying());
	TestEqual(TEXT("Gravity Well on Bastion"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);

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

	// The jammer first.
	if (!R.StartAt(TEXT("Obj_Jammer"))) { return false; }
	R.TeleportTo(R.Props<ATDStepTrigger>(Jammer, 0)[0], 60.f);
	TestEqual(TEXT("across the bridge -> charges"), R.Step(Jammer), 1);
	TArray<ATDInteractableActor*> Charges;
	for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Jammer && It->LinkedStep == 1) { Charges.Add(*It); } }
	TestEqual(TEXT("three charge sites"), Charges.Num(), 3);
	for (ATDInteractableActor* C : Charges) { R.Use(C, C->HoldTime); }
	TestEqual(TEXT("charges set -> run"), R.Step(Jammer), 2);
	TArray<ATDCollapseRun*> Runs = R.Props<ATDCollapseRun>(Jammer, 2);
	TestTrue(TEXT("the tower is coming down"), Runs.Num() == 1 && Runs[0]->IsRunning());
	TestTrue(*FString::Printf(TEXT("countdown on the line ('%s')"), *R.Line()), R.Line().Contains(TEXT("GET CLEAR")));
	R.TeleportTo(R.Props<ATDStepTrigger>(Jammer, 2)[0], 60.f);
	TestTrue(TEXT("jammer done"), Jammer->IsComplete());

	// The outpost.
	if (!R.StartAt(TEXT("Obj_Outpost"))) { return false; }
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
	ATDInteractableActor* Proto = nullptr;
	for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Outpost && It->LinkedStep == 5) { Proto = *It; } }
	if (!TestNotNull(TEXT("prototype"), Proto)) { return false; }
	R.Use(Proto, Proto->HoldTime);
	TestTrue(TEXT("outpost done"), Outpost->IsComplete());
	TestEqual(TEXT("EMP on Lancer"), V.GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::EMPPulse), ETDShipId::Lancer);

	R.Tick(0.5f);
	R.LeaveForOrbit(TEXT("Cinder"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCourseRetake, "TriadDrift.Ground.Course.DeathWhileCarryingRetakesCleanly", TD_TEST_FLAGS)
bool FTDCourseRetake::RunTest(const FString& Parameters)
{
	TDPlanetTest::FVisit V;
	TDCourseTest::FRun R(V, *this);
	if (!R.Land()) { return false; }
	ATDObjective* Wreck = R.Course->FindObjective(TEXT("Obj_WreckCore"));
	if (!R.StartAt(TEXT("Obj_WreckCore"))) { return false; }
	R.TeleportTo(R.Props<ATDStepTrigger>(Wreck, 0)[0], 60.f);
	R.KillAll(R.StepFight(Wreck, 1));
	ATDExtractionSite* Core = nullptr;
	for (TActorIterator<ATDExtractionSite> It(V.W.GetWorld()); It; ++It) { Core = *It; }
	if (!TestNotNull(TEXT("core"), Core)) { return false; }
	R.Use(Core, Core->HoldTime);
	TestTrue(TEXT("carrying"), R.FP->IsCarrying());

	// Die with the core: CUT, then a clean take 2 of the same scene.
	R.FP->GetHealth()->SetInvulnerable(false);
	R.FP->GetHealth()->Kill(nullptr, ETDDamageKind::Energy);
	R.Tick(UTDFlowSettings::Get()->RetakeDelaySeconds + 1.f);
	TestFalse(TEXT("not carrying after the retake"), R.FP->IsCarrying());
	TestFalse(TEXT("core back at the wreck"), Core->IsCarried());
	TestFalse(TEXT("pilot alive"), R.FP->IsDead());
	// The extraction is a checkpoint: take 2 restarts there, at the wreck, with fresh waves (no leftovers).
	TestTrue(TEXT("same scene restarts at the extraction checkpoint"), Wreck->IsActive() && R.Step(Wreck) == 2);
	TestEqual(TEXT("take 2"), V.GI->GetProgressRef().Takes.FindRef(TEXT("Obj_WreckCore")), 2);
	TestTrue(TEXT("respawned at the wreck"), FVector::Dist2D(R.FP->GetActorLocation(), Core->GetActorLocation()) < 1500.f);
	TestTrue(TEXT("the site fight before the checkpoint stays cleared"), R.StepFight(Wreck, 1)->IsCleared());
	TestTrue(TEXT("wave A dropped fresh, not leftovers"), Core->WaveA && Core->WaveA->GetAliveCount() <= Core->WaveA->CrewMix.Num());
	// And it can still be finished.
	R.FP->GetHealth()->SetInvulnerable(true);
	R.Use(Core, Core->HoldTime);
	R.FP->RespawnAt(FTransform(FVector(300.f, 0.f, 120.f)));
	R.Tick(0.5f);
	TestTrue(TEXT("finished on take 2"), Wreck->IsComplete());
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
				return false;
			}
			return true;
		}

		/** Walk up to an interactable, look at it and hold E (tap when Hold is 0). */
		bool Use(AActor* A, float Hold, const FVector& LookPoint = FVector::ZeroVector)
		{
			if (!RunTo(A->GetActorLocation(), 170.f, 30.f, false)) { return false; }
			LookAt(LookPoint.IsZero() ? A->GetActorLocation() : LookPoint);
			Tick(0.1f);
			Key(EKeys::E, true);
			Tick(Hold + 0.3f);
			Key(EKeys::E, false);
			Tick(0.2f);
			return true;
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

	// --- The relay: run there, climb with real jumps, wake the nodes, awaken it. ---
	bool bOk = P.RunTo(P.Trailhead(TEXT("Obj_Relay")), 300.f) && P.RunTo(FVector(5250.f, -5100.f, 0.f));
	P.Report(bOk && Relay->IsActive(), TEXT("relay started at its trailhead"));
	for (int32 Try = 0; Try < 6 && !Relay->IsComplete(); ++Try)
	{
		P.Trace(Relay, Try);
		const int32 Step = Relay->GetCurrentStepIndex();
		if (Step == 0)
		{
			P.RunTo(FVector(5250.f, -5100.f, 0.f), 160.f, 60.f);
			P.JumpTo(FVector(5700.f, -5500.f, 0.f), false, false, 0.6f);           // A (+110)
			P.JumpTo(FVector(6400.f, -6050.f, 0.f), true, false, 0.7f);            // B (+180)
			P.RunTo(FVector(6620.f, -6250.f, 0.f), 60.f, 4.f, false);             // B's far edge
			P.JumpTo(FVector(7550.f, -7000.f, 0.f), false, true, 0.9f);            // jump + dash over the gap onto C
			P.JumpTo(FVector(9200.f, -8200.f, 0.f), true, false, 1.0f);            // plateau (+270: jump + mantle)
			P.Tick(0.5f);
			if (Try == 0) { P.Report(Relay->GetCurrentStepIndex() == 1, TEXT("climbed to the relay with real jumps, double jump, dash and mantle")); }
		}
		else if (Step == 1)
		{
			for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It)
			{
				ATDInteractableActor* N = *It;
				if (N->LinkedObjective != Relay || N->LinkedStep != 1 || N->WasUsed()) { continue; }
				const FVector Dir = (N->GetActorLocation() - FVector(9200.f, -8200.f, 0.f)).GetSafeNormal2D();
				P.RunTo(FVector(9200.f, -8200.f, 0.f) + Dir * 450.f, 80.f, 10.f, false);
				P.JumpTo(N->GetActorLocation(), true, false, 0.6f);                   // onto the stub (+180)
				P.LookAt(N->GetActorLocation());
				P.Tap(EKeys::E);
				P.Tick(0.3f);
			}
		}
		else if (Step == 2)
		{
			// The drones break the hold: take them down with the blade first (jumping into them).
			// A death here is a retake (from the nodes checkpoint), not a failure: only the outcome is judged.
			if (ATDEncounterVolume* Drones = P.ActiveFightNear(FVector(9200.f, -8200.f, 0.f), 1500.f))
			{
				if (!P.Fight(Drones, 60.f)) { AddInfo(FString::Printf(TEXT("drone fight: %s"), *P.Failed)); P.Failed.Reset(); continue; }
			}
			for (TActorIterator<ATDInteractableActor> It(V.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Relay && It->LinkedStep == 2) { P.Use(*It, It->HoldTime); } }
		}
		P.Tick(0.2f);
	}
	P.Report(Relay->IsComplete(), TEXT("relay awakened"));

	// --- The wreck: the ambush on the way, the site fight, the extraction under fire, the carry. ---
	P.RunTo(FVector(9200.f, -8200.f, 0.f) + FVector(-1400.f, 0.f, 0.f), 200.f, 10.f); // off the plateau edge
	bOk = P.RunTo(P.Trailhead(TEXT("Obj_WreckCore")), 300.f, 60.f);
	P.Report(bOk && Wreck->IsActive(), TEXT("wreck started at its trailhead"));
	for (int32 Try = 0; Try < 4 && !Wreck->IsComplete(); ++Try)
	{
		if (Wreck->GetCurrentStepIndex() == 0)
		{
			P.RunTo(FVector(6200.f, 4600.f, 0.f), 400.f);
			if (ATDEncounterVolume* Ambush = P.ActiveFightNear(FVector(6200.f, 4600.f, 0.f), 1500.f)) { P.Fight(Ambush); }
			P.RunTo(FVector(9300.f, 6300.f, 0.f), 200.f);
		}
		if (Wreck->GetCurrentStepIndex() == 1) { P.Fight(P.FightOf(Wreck, 1)); }
		if (Wreck->GetCurrentStepIndex() == 2)
		{
			ATDExtractionSite* Site = nullptr;
			for (TActorIterator<ATDExtractionSite> It(V.W.GetWorld()); It; ++It) { Site = *It; }
			if (Site && Site->WaveA) { P.Fight(Site->WaveA, 60.f); }              // clear the first drop, then extract
			if (Site) { P.Use(Site, Site->HoldTime, Site->Core->GetComponentLocation()); }
		}
		if (Wreck->GetCurrentStepIndex() == 3) { P.RunTo(FVector(300.f, 0.f, 0.f), 300.f, 60.f, false); P.Tick(0.5f); }
	}
	P.Report(Wreck->IsComplete(), TEXT("wreck core extracted and carried to the ship"));
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
				if (It->LinkedObjective == Jammer && It->LinkedStep == 1 && !It->WasUsed()) { P.Use(*It, It->HoldTime); }
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
	if (ATDBoardShipInteractable* Board = V.Director->GetBoardPoint()) { P.RunTo(FVector(0.f, 0.f, 0.f), 600.f, 60.f); P.Use(Board, Board->HoldTime); }
	P.Tick(1.f);
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

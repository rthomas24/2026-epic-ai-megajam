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

		explicit FVisit(FName PresetReturnPoint = NAME_None)
			: W(UTDGameInstance::StaticClass())
		{
			GI = Cast<UTDGameInstance>(W.GetGameInstance());
			PC = W.SpawnPlayerController();
			if (GI)
			{
				GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationTest"));
				GI->NotifyLevelEntered(ETDDestination::Verdance);
				GI->GetMutableProgress().ReturnPoint = PresetReturnPoint;
			}
			const FTransform Pad(FRotator::ZeroRotator, FVector(0.f, 0.f, 0.f));
			Director = W.GetWorld()->SpawnActorDeferred<ATDPlanetDirector>(ATDPlanetDirector::StaticClass(), Pad);
			Director->PlanetId = TEXT("Verdance");
			Director->PadTransform = Pad;
			Director->RequiredObjectives = { TEXT("Obj_WreckCore"), TEXT("Obj_Relay") };
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
	TestEqual(TEXT("ReturnPoint is the pad"), V.GI->GetProgressRef().ReturnPoint, FName(TEXT("Verdance_Pad")));
	TestEqual(TEXT("checkpoint taken on foot"), V.GI->GetLastCheckpoint().Progress.ReturnPoint, FName(TEXT("Verdance_Pad")));
	TestNotNull(TEXT("board-ship prompt"), V.Director->GetBoardPoint());
	TestTrue(TEXT("board prompt usable"), V.Director->GetBoardPoint() && V.Director->GetBoardPoint()->CanInteract(FP));
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

#endif // WITH_DEV_AUTOMATION_TESTS

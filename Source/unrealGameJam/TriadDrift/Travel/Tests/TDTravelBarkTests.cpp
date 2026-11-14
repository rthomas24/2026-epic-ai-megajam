// ENG-43: T1's travel and flight barks (dialogs.md), each asked for once at its trigger, never every frame.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Barks

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDTravelBarks.h"
#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDFlightTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDT1Barks
{
	ATDShip* Flown(FTDTestWorld& W, const FVector& At)
	{
		ATDShip* Ship = W.Spawn<ATDShip>(At);
		if (Ship) { Ship->SetPlayerControlled(true); W.SpawnLocalPlayerController(Ship); }
		return Ship;
	}
	int32 N(FName Id) { return FTDCombatBarks::GetRequestCount(Id); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1BarkBoostEmpty, "TriadDrift.Travel.Barks.BoostEmptyOncePerLockout", TD_TEST_FLAGS)
bool FTDT1BarkBoostEmpty::RunTest(const FString& Parameters)
{
	using namespace TDT1Barks;
	FTDTestWorld W;
	ATDShip* Ship = Flown(W, FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	FTDCombatBarks::ResetRequestCounts();
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bBoost = true;
	float T = 0.f;
	for (; T < 30.f && N(TDBarkIdsT1::BoostEmpty) == 0; T += 1.f / 60.f) { Ship->SetFlightInput(In); W.Tick(1.f / 60.f); }
	AddInfo(FString::Printf(TEXT("boost ran dry after %.1f s held"), T));
	TestEqual(TEXT("the meter runs dry while held: one Bark_BoostEmpty"), N(TDBarkIdsT1::BoostEmpty), 1);
	for (int32 i = 0; i < 120; ++i) { Ship->SetFlightInput(In); W.Tick(1.f / 60.f); }
	TestEqual(TEXT("still held and empty: no more"), N(TDBarkIdsT1::BoostEmpty), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1BarkHyper, "TriadDrift.Travel.Barks.HyperdriveSpoolAndBlockedInCombat", TD_TEST_FLAGS)
bool FTDT1BarkHyper::RunTest(const FString& Parameters)
{
	using namespace TDT1Barks;
	{
		FTDTestWorld W;
		ATDShip* Ship = Flown(W, FVector(0.f, 0.f, 20000.f));
		if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("hyperdrive"), Ship->GetHyperdrive())) { return false; }
		W.Advance(0.5f);
		FTDCombatBarks::ResetRequestCounts();
		const bool bSpool = Ship->GetHyperdrive()->RequestSpoolFromPilot();
		TestTrue(TEXT("the pilot's jump spools"), bSpool);
		TestEqual(TEXT("Bark_Hyper_Spool once"), N(TDBarkIdsT1::HyperSpool), bSpool ? 1 : 0);
	}
	{
		FTDTestWorld W;
		ATDShip* Ship = Flown(W, FVector(0.f, 0.f, 20000.f));
		UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
		if (!Ship || !TestNotNull(TEXT("travel"), Travel)) { return false; }
		W.Advance(0.5f);
		Travel->AddHyperdriveLock(TEXT("TDT1Test"), NSLOCTEXT("TDT1", "Locked", "HYPERDRIVE LOCKED - ENEMIES IN RANGE"));
		FTDCombatBarks::ResetRequestCounts();
		TestFalse(TEXT("in combat the jump is refused"), Ship->GetHyperdrive()->RequestSpoolFromPilot());
		TestEqual(TEXT("Bark_Hyper_Blocked once per refused press"), N(TDBarkIdsT1::HyperBlocked), 1);
		TestEqual(TEXT("... and no spool line"), N(TDBarkIdsT1::HyperSpool), 0);
		Travel->RemoveHyperdriveLock(TEXT("TDT1Test"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1BarkAtmo, "TriadDrift.Travel.Barks.AtmosphereEntryOnCommit", TD_TEST_FLAGS)
bool FTDT1BarkAtmo::RunTest(const FString& Parameters)
{
	using namespace TDT1Barks;
	FTDTestWorld W;
	ATDShip* Ship = Flown(W, FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("entry"), Ship->GetAtmosphereEntry())) { return false; }
	W.Advance(0.3f);
	FTDCombatBarks::ResetRequestCounts();
	TestTrue(TEXT("entry begins"), Ship->GetAtmosphereEntry()->BeginEntry(ETDDestination::Verdance, TEXT("Verdance_Orbit"), FVector(-200000.f, 0.f, 0.f)));
	TestEqual(TEXT("Bark_Atmo_Entry once on the commit"), N(TDBarkIdsT1::AtmoEntry), 1);
	W.Advance(1.f);
	TestEqual(TEXT("not again during the dive"), N(TDBarkIdsT1::AtmoEntry), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1BarkLanding, "TriadDrift.Travel.Barks.LandingReadyAndTouchdown", TD_TEST_FLAGS)
bool FTDT1BarkLanding::RunTest(const FString& Parameters)
{
	using namespace TDT1Barks;
	FTDTestWorld W;
	ATDPlanetTravelRig* Rig = ATDPlanetTravelRig::SpawnForPlanet(W.GetWorld(), TEXT("Verdance"), FTransform(FVector(0.f, 0.f, 200.f)), { TEXT("Obj_Test") });
	if (!TestNotNull(TEXT("rig"), Rig) || !Rig->GetPlayerPad()) { return false; }
	const FTransform PadXf = Rig->GetPlayerPad()->GetActorTransform();
	ATDShip* Ship = Flown(W, PadXf.TransformPosition(FVector(-300000.f, 0.f, 3000.f))); // 3 km out: no pad in range
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Tick();
	FTDCombatBarks::ResetRequestCounts();
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	UTDLandingComponent* Land = Ship->GetLanding();
	for (int32 i = 0; i < 10; ++i) { W.Tick(); }
	TestEqual(TEXT("3 km out: no Bark_Land_Ready"), N(TDBarkIdsT1::LandReady), 0);
	auto Near = [&]()
	{
		Ship->TeleportSpacecraft(PadXf.TransformPosition(FVector(-10000.f, 0.f, 3000.f)), FRotator::ZeroRotator, false);
		Move->SetForwardSpeedImmediate(Move->GetCruiseCapSpeed() * 0.5f);
	};
	Near();
	W.Tick();
	TestEqual(TEXT("the pad in range: Bark_Land_Ready once"), N(TDBarkIdsT1::LandReady), 1);
	Near(); // as the landing tests do: 100 m out, slow
	TestTrue(TEXT("land"), Land->RequestLand());
	for (int32 i = 0; i < 60 * 12 && Land->GetLandingState() != ETDLandingState::Landed; ++i) { W.Tick(1.f / 60.f); }
	TestEqual(TEXT("landed"), Land->GetLandingState(), ETDLandingState::Landed);
	TestEqual(TEXT("touchdown: Bark_Land_Down once"), N(TDBarkIdsT1::LandDown), 1);
	TestEqual(TEXT("still one Bark_Land_Ready (not every frame of the approach)"), N(TDBarkIdsT1::LandReady), 1);
	return true;
}

#endif

// Ship audio hooks (ENG-35): the low-hull alarm on the player's flown ship, the shield-break cue on every squad ship.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.AudioHooks

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShipLowHullAlarm, "TriadDrift.Flight.AudioHooks.LowHullAlarmOnTheFlownShipOnly", TD_TEST_FLAGS)
bool FTDShipLowHullAlarm::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	Ship->SetShipVisuals(NewObject<UTDShipVisuals>()); // empty cues: the request is what's checked
	Ship->SetPlayerControlled(true);
	UTDHealthComponent* H = Ship->GetHealth();
	if (!TestNotNull(TEXT("health"), H)) { return false; }
	H->InitMaxValues(100.f, 50.f, true);
	H->ClearTimedInvulnerability();
	H->SetInvulnerable(false);
	static const FName Alarm(TEXT("LowHullLoop"));
	W.Tick();
	TestFalse(TEXT("full hull: no alarm"), Ship->IsCueLoopActive(Alarm));

	// Shield to zero: the shield-break cue once.
	const int32 BreaksBefore = Ship->GetCueRequestCount(TEXT("ShieldBreak"));
	FTDDamageInfo Hit;
	Hit.Amount = 55.f; // 50 shield + 5 hull
	Hit.Kind = ETDDamageKind::Energy;
	H->ApplyDamage(Hit);
	W.Tick();
	TestEqual(TEXT("shield to zero plays ShieldBreak once"), Ship->GetCueRequestCount(TEXT("ShieldBreak")) - BreaksBefore, 1);
	TestFalse(TEXT("hull 95%: still no alarm"), Ship->IsCueLoopActive(Alarm));

	// Below 35%: the alarm runs.
	Hit.Amount = 65.f; // hull 30
	H->SuppressShield(30.f);
	H->ApplyDamage(Hit);
	W.Tick();
	AddInfo(FString::Printf(TEXT("hull %.2f"), H->GetHull01()));
	TestTrue(TEXT("hull below 35% on the flown ship: alarm on"), H->GetHull01() < 0.35f && Ship->IsCueLoopActive(Alarm));

	// The player switches away: a wingman never sounds it; switching back brings it back.
	Ship->SetPlayerControlled(false);
	W.Tick();
	TestFalse(TEXT("switched away: alarm off"), Ship->IsCueLoopActive(Alarm));
	Ship->SetPlayerControlled(true);
	W.Tick();
	TestTrue(TEXT("switched back: alarm on"), Ship->IsCueLoopActive(Alarm));

	// Repaired a hair above the line keeps it (hysteresis), well above stops it.
	H->RepairHull(5.5f); // 35.5%
	W.Tick();
	TestTrue(TEXT("just above the line: no flicker"), Ship->IsCueLoopActive(Alarm));
	H->RepairHull(20.f);
	W.Tick();
	TestFalse(TEXT("repaired above it: alarm off"), Ship->IsCueLoopActive(Alarm));

	// Death stops it.
	Hit.Amount = 50.f;
	H->ApplyDamage(Hit);
	W.Tick();
	TestTrue(TEXT("low again: on"), Ship->IsCueLoopActive(Alarm));
	H->Kill();
	W.Tick();
	TestFalse(TEXT("dead: alarm off"), Ship->IsCueLoopActive(Alarm));
	return true;
}

#endif

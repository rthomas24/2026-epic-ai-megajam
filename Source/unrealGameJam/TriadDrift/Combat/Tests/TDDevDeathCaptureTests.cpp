// Dev death capture (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Dev

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/WorldSettings.h"
#include "TriadDrift/Combat/TDDevDeathCapture.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDevDeathCaptureHolds, "TriadDrift.Combat.Dev.CaptureOnDeathFreezesMatchingDeaths", TD_TEST_FLAGS)
bool FTDDevDeathCaptureHolds::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	if (!TestNotNull(TEXT("event bus"), Bus))
	{
		return false;
	}
	AActor* Victim = W.Spawn<AActor>(FVector::ZeroVector);

	// A filter that doesn't match: nothing happens.
	UTDDevDeathCapture* Capture = UTDDevDeathCapture::Arm(W.GetWorld(), TEXT("ShardFighter"), 1, 3.f, false);
	if (!TestNotNull(TEXT("armed"), Capture))
	{
		return false;
	}
	Bus->BroadcastActorKilled(Victim, nullptr, ETDDamageKind::Energy);
	TestEqual(TEXT("other class: not caught"), Capture->GetCaught(), 0);
	TestFalse(TEXT("other class: no hold"), Capture->IsHolding());

	// "any", two deaths: each freezes the world, then restores it exactly.
	Capture = UTDDevDeathCapture::Arm(W.GetWorld(), TEXT("any"), 2, 3.f, false);
	Bus->BroadcastActorKilled(Victim, nullptr, ETDDamageKind::Energy);
	TestEqual(TEXT("caught the death"), Capture->GetCaught(), 1);
	TestTrue(TEXT("holding"), Capture->IsHolding());
	TestTrue(TEXT("world nearly frozen"), FMath::IsNearlyEqual(W.GetWorld()->GetWorldSettings()->TimeDilation, UTDDevDeathCapture::HoldDilation, 0.001f));
	Bus->BroadcastActorKilled(Victim, nullptr, ETDDamageKind::Energy);
	TestEqual(TEXT("a death during a hold is not a second catch"), Capture->GetCaught(), 1);
	Capture->EndHold();
	TestTrue(TEXT("time restored"), FMath::IsNearlyEqual(W.GetWorld()->GetWorldSettings()->TimeDilation, 1.f, 0.001f));
	Bus->BroadcastActorKilled(Victim, nullptr, ETDDamageKind::Energy);
	TestEqual(TEXT("second catch"), Capture->GetCaught(), 2);
	Capture->EndHold();
	TestEqual(TEXT("count used up"), Capture->GetRemaining(), 0);
	Bus->BroadcastActorKilled(Victim, nullptr, ETDDamageKind::Energy);
	TestEqual(TEXT("disarmed after its count"), Capture->GetCaught(), 2);
	UTDDevDeathCapture::Disarm();
	TestNull(TEXT("off"), UTDDevDeathCapture::GetActive());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

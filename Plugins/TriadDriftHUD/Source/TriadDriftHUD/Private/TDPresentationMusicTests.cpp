#include "TDPresentationMusicSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVerdanceDucking, "TriadDrift.Audio.Verdance.OverlappingDuckReasons", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTDVerdanceDucking::RunTest(const FString& Parameters)
{
	FTDVerdanceDuckState State;
	State.bCombat = true;
	TestEqual(TEXT("combat entry uses one minus-six-dB gain"), State.Gain(), 0.5011872f);
	State.bDialogue = true;
	State.bCombat = false;
	TestEqual(TEXT("combat end cannot unduck active dialogue"), State.Gain(), 0.5011872f);
	State.bDialogue = false;
	TestEqual(TEXT("last reason ends at full gain"), State.Gain(), 1.f);
	State.bCombat = true;
	State.bDialogue = true;
	TestEqual(TEXT("overlapping reasons do not compound attenuation"), State.Gain(), 0.5011872f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBoardingLifecycle, "TriadDrift.Audio.Boarding.EndAndReentry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTDBoardingLifecycle::RunTest(const FString& Parameters)
{
	FTDBoardingMusicState State;
	TestTrue(TEXT("sequence enters once"), State.Begin());
	const uint64 Original = State.Generation;
	TestFalse(TEXT("duplicate entry cannot restart"), State.Begin());
	TestTrue(TEXT("actual end begins fade"), State.End());
	TestFalse(TEXT("duplicate end cannot extend fade"), State.End());
	TestTrue(TEXT("ownership held for entire fade"), State.bActive);
	TestFalse(TEXT("reentry during fade cannot overlap"), State.Begin());
	State.Reset();
	TestTrue(TEXT("later sequence can enter"), State.Begin());
	TestTrue(TEXT("old completion generation is invalid"), State.Generation != Original);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCreditsPlaylistLifecycle, "TriadDrift.Audio.Credits.OrderAndStaleCompletion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTDCreditsPlaylistLifecycle::RunTest(const FString& Parameters)
{
	FTDPresentationPlaylistState State;
	TestTrue(TEXT("first entry starts"), State.Begin());
	const uint64 First = State.Generation;
	TestEqual(TEXT("Cathedral first"), State.Index, 0);
	TestFalse(TEXT("duplicate entry cannot restart"), State.Begin());
	TestTrue(TEXT("natural completion advances"), State.Finish(First));
	TestEqual(TEXT("full remix second"), State.Index, 1);
	TestFalse(TEXT("duplicate old completion cannot skip remix"), State.Finish(First));
	const uint64 Second = State.Generation;
	TestTrue(TEXT("second completion finishes"), State.Finish(Second));
	TestEqual(TEXT("no third track or automatic repeat"), State.Index, INDEX_NONE);
	State.Stop();
	TestTrue(TEXT("new roll can begin after stop"), State.Begin());
	const uint64 Interrupted = State.Generation;
	State.Stop(); // Early exit/travel invalidates an already queued completion.
	TestTrue(TEXT("reentry starts first track"), State.Begin());
	TestFalse(TEXT("prior roll callback cannot advance new roll"), State.Finish(Interrupted));
	TestEqual(TEXT("new roll stays on first track"), State.Index, 0);
	TestFalse(TEXT("second track from old roll is stale too"), State.Finish(Second));
	return true;
}
#endif

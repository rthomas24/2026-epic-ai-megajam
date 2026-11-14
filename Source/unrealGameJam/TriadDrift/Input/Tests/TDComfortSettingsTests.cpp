// ENG-45 comfort settings: subtitles on/off, camera shake and speed-FOV scales (reduced motion forces both to 0).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Comfort

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1Comfort, "TriadDrift.Input.Comfort.SubtitlesShakeAndFOVScales", TD_TEST_FLAGS)
bool FTDT1Comfort::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDSettingsSubsystem* S = W.GetGameInstance() ? W.GetGameInstance()->GetSubsystem<UTDSettingsSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("settings"), S)) { return false; }
	const FTDPlayerSettings Before = S->GetSettings();
	ON_SCOPE_EXIT { S->SetSubtitlesEnabled(Before.bSubtitles); S->SetReduceMotion(Before.bReduceMotion); S->SetCameraShake(Before.CameraShake); S->SetSpeedFOV(Before.SpeedFOV); };

	TestTrue(TEXT("subtitles default on"), FTDPlayerSettings().bSubtitles);
	S->SetSubtitlesEnabled(false);
	TestFalse(TEXT("off"), S->AreSubtitlesEnabled() || UTDSettingsSubsystem::AreSubtitlesOn(W.GetWorld()));
	S->SetSubtitlesEnabled(true);
	TestTrue(TEXT("on"), UTDSettingsSubsystem::AreSubtitlesOn(W.GetWorld()));
	TestTrue(TEXT("no world: on"), UTDSettingsSubsystem::AreSubtitlesOn(nullptr));

	S->SetReduceMotion(false);
	S->SetCameraShake(0.4f);
	S->SetSpeedFOV(0.7f);
	TestEqual(TEXT("shake scale reaches every shake path"), UTDFlightFeedbackComponent::UserShakeScale, 0.4f);
	TestEqual(TEXT("FOV scale reaches every FOV path"), UTDFlightFeedbackComponent::UserSpeedFOVScale, 0.7f);
	S->SetReduceMotion(true);
	TestEqual(TEXT("reduced motion: no shake"), UTDFlightFeedbackComponent::UserShakeScale, 0.f);
	TestEqual(TEXT("reduced motion: no speed FOV"), UTDFlightFeedbackComponent::UserSpeedFOVScale, 0.f);
	S->SetReduceMotion(false);
	TestEqual(TEXT("off again: the slider's value back"), UTDFlightFeedbackComponent::UserShakeScale, 0.4f);
	return true;
}

#endif

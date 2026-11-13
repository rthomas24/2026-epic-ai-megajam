// Device detection on foot: pad keys set "using gamepad", mouse and keyboard clear it, so on-foot prompts switch.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Device

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOnFootDeviceTest, "TriadDrift.Input.Device.OnFootPadAndMouseSwitchTheDevice", TD_TEST_FLAGS)
bool FTDOnFootDeviceTest::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Pilot = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 200.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Pilot, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("controller"), PC) || !Pilot) { return false; }
	W.Tick();
	TestFalse(TEXT("starts on mouse and keyboard"), PC->IsUsingGamepad());

	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_FaceButton_Bottom, true);
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_FaceButton_Bottom, false);
	W.Tick();
	TestTrue(TEXT("a pad button switches to gamepad"), PC->IsUsingGamepad());

	FTDTestWorld::InjectKey(PC, EKeys::W, true);
	FTDTestWorld::InjectKey(PC, EKeys::W, false);
	W.Tick();
	TestFalse(TEXT("a keyboard key switches back"), PC->IsUsingGamepad());

	FTDTestWorld::InjectAxis(PC, EKeys::Gamepad_LeftX, PC->GetStickDeadzone() * 0.5f);
	W.Tick();
	TestFalse(TEXT("stick drift inside the deadzone doesn't switch"), PC->IsUsingGamepad());
	FTDTestWorld::InjectAxis(PC, EKeys::Gamepad_LeftX, 0.8f);
	W.Tick();
	TestTrue(TEXT("a real stick push switches to gamepad"), PC->IsUsingGamepad());

	FTDTestWorld::InjectAxis(PC, EKeys::MouseX, 0.5f);
	W.Tick();
	TestTrue(TEXT("a bumped mouse doesn't steal it back"), PC->IsUsingGamepad());
	FTDTestWorld::InjectAxis(PC, EKeys::MouseX, 25.f);
	W.Tick();
	TestFalse(TEXT("a real mouse move switches back"), PC->IsUsingGamepad());

	FTDTestWorld::InjectAxis(PC, EKeys::Gamepad_RightTriggerAxis, 1.f);
	W.Tick();
	TestTrue(TEXT("a trigger pull switches to gamepad"), PC->IsUsingGamepad());
	FTDTestWorld::InjectKey(PC, EKeys::LeftMouseButton, true);
	FTDTestWorld::InjectKey(PC, EKeys::LeftMouseButton, false);
	W.Tick();
	TestFalse(TEXT("a mouse click switches back"), PC->IsUsingGamepad());
	return true;
}

#endif

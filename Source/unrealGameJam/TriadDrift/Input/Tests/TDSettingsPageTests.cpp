// ENG-32: the on-foot camera setting, the Settings page and the V / View toggle.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.SettingsPage

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Input/TDSettingsPage.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Input/Tests/TDInputTestHelpers.h"
#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Variant_Shooter/UI/ShooterUI.h"

namespace TDSettingsPageTest
{
	static const TCHAR* Slot = TEXT("TD_Settings_PageTest");

	static UTDSettingsSubsystem* Fresh(FTDTestWorld& W)
	{
		UGameplayStatics::DeleteGameInSlot(Slot, 0);
		UTDSettingsSubsystem* S = UTDSettingsSubsystem::GetTDSettings(W.GetWorld());
		if (S) { S->UseSlotForTests(Slot); }
		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCameraModePersist, "TriadDrift.Input.SettingsPage.CameraModePersistsAndBroadcasts", TD_TEST_FLAGS)
bool FTDCameraModePersist::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDSettingsSubsystem* S = TDSettingsPageTest::Fresh(W);
	if (!TestNotNull(TEXT("settings"), S)) { return false; }
	ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(TDSettingsPageTest::Slot, 0); };
	UTDSettingsEventCounter* Counter = NewObject<UTDSettingsEventCounter>();
	Counter->AddToRoot();
	ON_SCOPE_EXIT { S->OnOnFootCameraModeChanged.RemoveAll(Counter); Counter->RemoveFromRoot(); };
	S->OnOnFootCameraModeChanged.AddDynamic(Counter, &UTDSettingsEventCounter::HandleCameraMode);

	TestEqual(TEXT("default: first person"), S->GetOnFootCameraMode(), ETDOnFootCamera::FirstPerson);
	S->ToggleOnFootCameraMode();
	TestEqual(TEXT("toggle: third person"), S->GetOnFootCameraMode(), ETDOnFootCamera::ThirdPerson);
	TestEqual(TEXT("one broadcast"), Counter->CameraChanges, 1);
	TestEqual(TEXT("broadcast carries the mode"), Counter->LastMode, ETDOnFootCamera::ThirdPerson);
	S->SetOnFootCameraMode(ETDOnFootCamera::ThirdPerson);
	TestEqual(TEXT("no broadcast without a change"), Counter->CameraChanges, 1);

	S->ReloadForTests(); // a restart / level travel reads it back
	TestEqual(TEXT("persisted: third person"), S->GetOnFootCameraMode(), ETDOnFootCamera::ThirdPerson);

	S->ResetToDefaults();
	TestEqual(TEXT("reset: first person"), S->GetOnFootCameraMode(), ETDOnFootCamera::FirstPerson);
	TestEqual(TEXT("reset broadcasts the change"), Counter->CameraChanges, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSettingsPageKeys, "TriadDrift.Input.SettingsPage.NavigatesWithKeysAndPadAndEveryRowChanges", TD_TEST_FLAGS)
bool FTDSettingsPageKeys::RunTest(const FString& Parameters)
{
	using EKR = FTDSettingsPageModel::EKeyResult;
	FTDTestWorld W;
	UTDSettingsSubsystem* S = TDSettingsPageTest::Fresh(W);
	if (!TestNotNull(TEXT("settings"), S)) { return false; }
	ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(TDSettingsPageTest::Slot, 0); };
	FTDSettingsPageModel M(S);
	M.Resolutions = { FIntPoint::ZeroValue, FIntPoint(1280, 720), FIntPoint(1920, 1080) };

	TestEqual(TEXT("first row: on-foot camera"), M.GetSelectedRow(), ETDSettingsRow::OnFootCamera);
	TestTrue(TEXT("first row label"), FTDSettingsPageModel::RowLabel(ETDSettingsRow::OnFootCamera).ToString().Contains(TEXT("On-foot camera")));
	TestEqual(TEXT("Up wraps to Back"), M.HandleKey(EKeys::Up), EKR::Handled);
	TestEqual(TEXT("... the last row"), M.GetSelectedRow(), ETDSettingsRow::Back);
	TestEqual(TEXT("pad D-pad down wraps to the top"), M.HandleKey(EKeys::Gamepad_DPad_Down), EKR::Handled);
	TestEqual(TEXT("... on-foot camera"), M.GetSelectedRow(), ETDSettingsRow::OnFootCamera);

	// Volumes start at 100% (the top): start them mid-range so both directions move.
	S->SetMasterVolume(0.5f);
	S->SetMusicVolume(0.5f);
	S->SetEffectsVolume(0.5f);
	// Every value row changes its setting from the keyboard (Right) and the pad (stick left / A button).
	for (int32 i = 0; i < (int32)ETDSettingsRow::Reset; ++i)
	{
		M.Select(i);
		const ETDSettingsRow Row = (ETDSettingsRow)i;
		const FString Label = FTDSettingsPageModel::RowLabel(Row).ToString();
		const FString V0 = M.RowValue(Row).ToString();
		TestEqual(*FString::Printf(TEXT("%s: Right handled"), *Label), M.HandleKey(EKeys::Right), EKR::Handled);
		const FString V1 = M.RowValue(Row).ToString();
		TestNotEqual(*FString::Printf(TEXT("%s: Right changes it (%s -> %s)"), *Label, *V0, *V1), V1, V0);
		M.HandleKey(EKeys::Gamepad_LeftStick_Left);
		const FString V2 = M.RowValue(Row).ToString();
		TestEqual(*FString::Printf(TEXT("%s: pad stick left steps back (%s)"), *Label, *V2), V2, V0);
		M.HandleKey(EKeys::Gamepad_FaceButton_Bottom);
		TestNotEqual(*FString::Printf(TEXT("%s: pad A changes it too"), *Label), M.RowValue(Row).ToString(), V0);
	}
	const FTDPlayerSettings& P = S->GetSettings();
	TestEqual(TEXT("camera row reached the setting"), P.OnFootCameraMode, ETDOnFootCamera::ThirdPerson);
	TestEqual(TEXT("mouse row reached the setting"), P.MouseSensitivity, 1.25f);
	TestTrue(TEXT("invert mouse row reached the setting"), P.bInvertMouseY);
	TestFalse(TEXT("hints row reached the setting"), P.bHintsEnabled);
	TestEqual(TEXT("master row reached the setting"), P.MasterVolume, 0.6f, 1e-4f);
	TestEqual(TEXT("dead zone row reached the setting"), P.GamepadDeadzone, 0.2f, 1e-4f);
	S->ReloadForTests();
	TestEqual(TEXT("the page's changes are saved"), S->GetSettings().MouseSensitivity, 1.25f);

	// Reset and Back.
	M.Select((int32)ETDSettingsRow::Reset);
	TestEqual(TEXT("Enter on Reset"), M.HandleKey(EKeys::Enter), EKR::Handled);
	TestEqual(TEXT("reset: mouse back to 1x"), S->GetSettings().MouseSensitivity, 1.f);
	TestEqual(TEXT("reset: first person"), S->GetSettings().OnFootCameraMode, ETDOnFootCamera::FirstPerson);
	M.Select((int32)ETDSettingsRow::Back);
	TestEqual(TEXT("A on Back closes"), M.HandleKey(EKeys::Gamepad_FaceButton_Bottom), EKR::Close);
	TestEqual(TEXT("Escape closes"), M.HandleKey(EKeys::Escape), EKR::Close);
	TestEqual(TEXT("pad B closes"), M.HandleKey(EKeys::Gamepad_FaceButton_Right), EKR::Close);
	TestEqual(TEXT("unrelated keys stay on the page"), M.HandleKey(EKeys::P), EKR::Unhandled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSettingsPageOverride, "TriadDrift.Input.SettingsPage.SettingsWidgetClassOverride", TD_TEST_FLAGS)
bool FTDSettingsPageOverride::RunTest(const FString& Parameters)
{
	UTDUIStyle* Style = NewObject<UTDUIStyle>();
	TestNull(TEXT("no style: code-built page"), UTDSettingsSubsystem::ResolveSettingsWidgetClass(nullptr));
	TestNull(TEXT("style without a settings widget: code-built page"), UTDSettingsSubsystem::ResolveSettingsWidgetClass(Style));
	Style->SettingsWidgetClass = UShooterUI::StaticClass();
	TestEqual(TEXT("style with a settings widget: that widget"), UTDSettingsSubsystem::ResolveSettingsWidgetClass(Style), (UClass*)UShooterUI::StaticClass());

	FTDTestWorld W;
	TestFalse(TEXT("headless (no viewport): the page reports it can't open"), UTDSettingsSubsystem::OpenSettingsPage(W.GetWorld()));
	TestFalse(TEXT("... and isn't open"), UTDSettingsSubsystem::IsSettingsPageOpen(W.GetWorld()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOnFootToggleKeys, "TriadDrift.Input.SettingsPage.VAndViewToggleOnFootOnly", TD_TEST_FLAGS)
bool FTDOnFootToggleKeys::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDSettingsSubsystem* S = TDSettingsPageTest::Fresh(W);
	if (!TestNotNull(TEXT("settings"), S)) { return false; }
	ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(TDSettingsPageTest::Slot, 0); };

	// In space: V is the cockpit camera, never the on-foot mode.
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("space controller"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::V, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::V, false);
	W.Tick();
	TestTrue(TEXT("space: V toggles the cockpit view"), Ship->IsCockpitView());
	TestEqual(TEXT("space: V leaves the on-foot camera alone"), S->GetOnFootCameraMode(), ETDOnFootCamera::FirstPerson);
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_Special_Left, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_Special_Left, false);
	W.Tick();
	TestEqual(TEXT("space: View leaves it alone too"), S->GetOnFootCameraMode(), ETDOnFootCamera::FirstPerson);

	// On foot: V and View toggle it.
	ATDFirstPersonCharacter* Pilot = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	PC->Possess(Pilot);
	W.Advance(0.2f);
	FTDTestWorld::InjectKey(PC, EKeys::V, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::V, false);
	W.Tick();
	TestEqual(TEXT("on foot: V switches to third person"), S->GetOnFootCameraMode(), ETDOnFootCamera::ThirdPerson);
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_Special_Left, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_Special_Left, false);
	W.Tick();
	TestEqual(TEXT("on foot: View switches back"), S->GetOnFootCameraMode(), ETDOnFootCamera::FirstPerson);

	// Back in the ship: V is the cockpit again.
	PC->Possess(Ship);
	W.Advance(0.2f);
	FTDTestWorld::InjectKey(PC, EKeys::V, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::V, false);
	W.Tick();
	TestEqual(TEXT("re-boarded: V no longer changes the on-foot camera"), S->GetOnFootCameraMode(), ETDOnFootCamera::FirstPerson);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPadYAbility, "TriadDrift.Input.SettingsPage.PadYStillReachesTheAbilityInSpace", TD_TEST_FLAGS)
bool FTDPadYAbility::RunTest(const FString& Parameters)
{
	// Pad Y opens Settings while paused (IMC_Global) and is the ship ability in flight (IMC_SpaceCommon): the
	// settings mapping must not swallow it (the gamepad pass found Y doing nothing on all three ships).
	FTDTestWorld W;
	UTDSettingsSubsystem* S = TDSettingsPageTest::Fresh(W);
	ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(TDSettingsPageTest::Slot, 0); };
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("space controller"), PC) || !TestNotNull(TEXT("settings"), S)) { return false; }
	Ship->SetPlayerControlled(true);
	PC->GetHints()->UseMemoryOnlyForTests();
	W.Tick();
	TestFalse(TEXT("ability input not seen yet"), PC->GetHints()->IsHintDone(ETDHint::Ability));
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_FaceButton_Top, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_FaceButton_Top, false);
	W.Tick();
	TestTrue(TEXT("pad Y reached the ship's ability input"), PC->GetHints()->IsHintDone(ETDHint::Ability));
	TestFalse(TEXT("... and opened no settings page in flight"), UTDSettingsSubsystem::IsSettingsPageOpen(W.GetWorld()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSettingsMigrate, "TriadDrift.Input.SettingsPage.OldSaveVersionKeepsOptions", TD_TEST_FLAGS)
bool FTDSettingsMigrate::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDSettingsSubsystem* S = TDSettingsPageTest::Fresh(W);
	if (!TestNotNull(TEXT("settings"), S)) { return false; }
	ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(TDSettingsPageTest::Slot, 0); };
	UTDSettingsSave* Old = Cast<UTDSettingsSave>(UGameplayStatics::CreateSaveGameObject(UTDSettingsSave::StaticClass()));
	Old->Version = 0; // an older layout
	Old->Settings.MouseSensitivity = 2.5f;
	Old->Settings.bInvertMouseY = true;
	Old->Settings.MasterVolume = 0.3f;
	Old->Settings.GamepadDeadzone = 0.9f; // out of today's range
	TestTrue(TEXT("old slot written"), UGameplayStatics::SaveGameToSlot(Old, TDSettingsPageTest::Slot, 0));
	S->ReloadForTests();
	const FTDPlayerSettings& P = S->GetSettings();
	TestEqual(TEXT("mouse kept"), P.MouseSensitivity, 2.5f);
	TestTrue(TEXT("invert kept"), P.bInvertMouseY);
	TestEqual(TEXT("master kept"), P.MasterVolume, 0.3f);
	TestEqual(TEXT("out-of-range value clamped"), P.GamepadDeadzone, 0.5f);
	const UTDSettingsSave* Again = Cast<UTDSettingsSave>(UGameplayStatics::LoadGameFromSlot(TDSettingsPageTest::Slot, 0));
	TestTrue(TEXT("re-saved at today's version"), Again && Again->Version == UTDSettingsSave::CurrentVersion);
	return true;
}

#endif

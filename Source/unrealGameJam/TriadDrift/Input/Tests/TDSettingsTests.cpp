// Player options. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Settings

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Kismet/GameplayStatics.h"
#include "InputActionValue.h"

/** Mouse and stick aim straight into the controller's handlers (no viewport in a headless world). */
struct FTDSettingsTestAccess
{
	ATDSpacePlayerController* PC;
	void Mouse(const FVector2D& D) { PC->OnMouseAim(FInputActionValue(D)); }
	void Stick(const FVector2D& D) { PC->OnStickAim(FInputActionValue(D)); }
	void ResetMouse() { PC->VirtualStick = FVector2D::ZeroVector; }
};

namespace TDSettingsTest
{
	static const TCHAR* Slot = TEXT("TD_Settings_UnitTest");

	struct FWorld
	{
		FTDTestWorld W;
		UTDSettingsSubsystem* S = nullptr;
		ATDShip* Ship = nullptr;
		ATDSpacePlayerController* PC = nullptr;

		bool Init()
		{
			UGameplayStatics::DeleteGameInSlot(Slot, 0);
			S = UTDSettingsSubsystem::GetTDSettings(W.GetWorld());
			if (!S) { return false; }
			S->UseSlotForTests(Slot);
			return true;
		}
		bool SpawnPilot()
		{
			Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
			PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
			if (!PC) { return false; }
			Ship->SetPlayerControlled(true);
			W.Tick();
			return true;
		}
		~FWorld() { UGameplayStatics::DeleteGameInSlot(Slot, 0); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSettingsPersist, "TriadDrift.Input.Settings.PersistAndApplyAtStartup", TD_TEST_FLAGS)
bool FTDSettingsPersist::RunTest(const FString& Parameters)
{
	TDSettingsTest::FWorld T;
	if (!TestTrue(TEXT("settings subsystem"), T.Init())) { return false; }
	T.S->SetMouseSensitivity(2.f);
	T.S->SetInvertMouseY(true);
	T.S->SetGamepadSensitivity(1.5f);
	T.S->SetGamepadDeadzone(0.3f);
	T.S->SetInvertGamepadY(true);
	T.S->SetHintsEnabled(false);
	T.S->SetMusicVolume(0.25f);
	T.S->SetEffectsVolume(0.75f);
	T.S->SetWindowMode(ETDWindowMode::Windowed);
	T.S->SetResolution(FIntPoint(1280, 720));
	T.S->SetMasterVolume(7.f); // out of range: clamped
	TestEqual(TEXT("volume clamped to 1"), T.S->GetSettings().MasterVolume, 1.f);
	T.S->SetMasterVolume(0.4f);

	// A fresh start-up reads the slot back.
	T.S->ReloadForTests();
	const FTDPlayerSettings& P = T.S->GetSettings();
	TestEqual(TEXT("mouse sensitivity"), P.MouseSensitivity, 2.f);
	TestTrue(TEXT("invert mouse"), P.bInvertMouseY);
	TestEqual(TEXT("pad sensitivity"), P.GamepadSensitivity, 1.5f);
	TestEqual(TEXT("pad dead zone"), P.GamepadDeadzone, 0.3f);
	TestTrue(TEXT("invert pad"), P.bInvertGamepadY);
	TestFalse(TEXT("hints off"), P.bHintsEnabled);
	TestEqual(TEXT("master"), P.MasterVolume, 0.4f);
	TestEqual(TEXT("music"), P.MusicVolume, 0.25f);
	TestEqual(TEXT("effects"), P.EffectsVolume, 0.75f);
	TestEqual(TEXT("window mode"), P.WindowMode, ETDWindowMode::Windowed);
	TestEqual(TEXT("resolution"), P.Resolution, FIntPoint(1280, 720));
	TestTrue(TEXT("video marked as chosen (applied at the next start-up)"), P.bVideoChosen);
	TestFalse(TEXT("defaults never force a video mode"), FTDPlayerSettings().bVideoChosen);
	TestEqual(TEXT("master volume reached the audio path"), T.S->GetAppliedMasterVolume(), 0.4f);

	// A controller that begins play afterwards (new map, new session) picks the options up.
	if (!TestTrue(TEXT("pilot"), T.SpawnPilot())) { return false; }
	const ATDSpacePlayerController* Defaults = GetDefault<ATDSpacePlayerController>();
	TestEqual(TEXT("controller: mouse sensitivity x2"), T.PC->GetMouseSensitivity(), Defaults->GetMouseSensitivity() * 2.f, 1e-6f);
	TestTrue(TEXT("controller: mouse inverted"), T.PC->IsMousePitchInverted());
	TestEqual(TEXT("controller: pad dead zone"), T.PC->GetStickDeadzone(), 0.3f);
	TestEqual(TEXT("controller: pad sensitivity"), T.PC->GetStickSensitivity(), 1.5f);
	TestTrue(TEXT("controller: pad inverted"), T.PC->IsStickPitchInverted());
	TestFalse(TEXT("controller: hints off"), T.PC->GetHints()->AreHintsEnabled());

	// Defaults back.
	T.S->ResetToDefaults();
	TestEqual(TEXT("reset: mouse 1x"), T.PC->GetMouseSensitivity(), Defaults->GetMouseSensitivity(), 1e-6f);
	TestTrue(TEXT("reset: hints on"), T.PC->GetHints()->AreHintsEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSettingsEffect, "TriadDrift.Input.Settings.EachControlTakesEffect", TD_TEST_FLAGS)
bool FTDSettingsEffect::RunTest(const FString& Parameters)
{
	TDSettingsTest::FWorld T;
	if (!TestTrue(TEXT("settings subsystem"), T.Init()) || !TestTrue(TEXT("pilot"), T.SpawnPilot())) { return false; }
	FTDSettingsTestAccess A{ T.PC };
	auto MouseAim = [&](const FVector2D& Counts)
	{
		A.ResetMouse();
		A.Mouse(Counts);
		T.W.Tick();
		return T.PC->GetCurrentFlightInput().Aim;
	};
	auto StickAim = [&](const FVector2D& Stick)
	{
		A.Stick(Stick);
		T.W.Tick();
		return T.PC->GetCurrentFlightInput().Aim;
	};

	// Mouse: sensitivity scales the aim, invert flips pitch.
	const FVector2D M1 = MouseAim(FVector2D(60.f, 60.f));
	T.S->SetMouseSensitivity(2.f);
	const FVector2D M2 = MouseAim(FVector2D(60.f, 60.f));
	TestTrue(*FString::Printf(TEXT("mouse x2 turns harder (%.3f -> %.3f)"), M1.X, M2.X), FMath::Abs(M2.X) > FMath::Abs(M1.X) * 1.3f);
	T.S->SetInvertMouseY(true);
	const FVector2D M3 = MouseAim(FVector2D(60.f, 60.f));
	TestTrue(*FString::Printf(TEXT("invert mouse flips pitch (%.3f vs %.3f)"), M2.Y, M3.Y), M2.Y * M3.Y < 0.f);

	// Gamepad: dead zone, sensitivity, invert.
	const FVector2D P1 = StickAim(FVector2D(0.3f, 0.f));
	TestTrue(*FString::Printf(TEXT("dead zone 0.15: a 0.3 push turns (%.3f)"), P1.X), P1.X > 0.01f);
	T.S->SetGamepadDeadzone(0.4f);
	const FVector2D P2 = StickAim(FVector2D(0.3f, 0.f));
	TestTrue(*FString::Printf(TEXT("dead zone 0.4: a 0.3 push does nothing (%.3f)"), P2.X), FMath::IsNearlyZero(P2.X, 1e-4f));
	T.S->SetGamepadDeadzone(0.15f);
	const FVector2D P3 = StickAim(FVector2D(0.6f, 0.f));
	T.S->SetGamepadSensitivity(1.5f);
	const FVector2D P4 = StickAim(FVector2D(0.6f, 0.f));
	TestTrue(*FString::Printf(TEXT("pad x1.5 turns harder (%.3f -> %.3f)"), P3.X, P4.X), FMath::IsNearlyEqual(P4.X, FMath::Min(1.f, P3.X * 1.5f), 0.01f));
	const FVector2D P5 = StickAim(FVector2D(0.f, 0.6f));
	T.S->SetInvertGamepadY(true);
	const FVector2D P6 = StickAim(FVector2D(0.f, 0.6f));
	TestTrue(*FString::Printf(TEXT("invert pad flips pitch (%.3f vs %.3f)"), P5.Y, P6.Y), P5.Y * P6.Y < 0.f);

	// Hints.
	T.S->SetHintsEnabled(false);
	TestFalse(TEXT("hints off reaches the hint component"), T.PC->GetHints()->AreHintsEnabled());
	T.S->SetHintsEnabled(true);
	TestTrue(TEXT("hints back on"), T.PC->GetHints()->AreHintsEnabled());

	// Audio: master reaches the device path; classes are reported (none exist yet).
	T.S->SetMasterVolume(0.3f);
	TestEqual(TEXT("master applied"), T.S->GetAppliedMasterVolume(), 0.3f);
	AddInfo(FString::Printf(TEXT("sound classes present: music %d effects %d"), T.S->HasMusicClass() ? 1 : 0, T.S->HasEffectsClass() ? 1 : 0));
	return true;
}

#endif

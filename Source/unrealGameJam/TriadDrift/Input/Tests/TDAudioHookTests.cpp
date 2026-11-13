// Front-end and ambience audio hooks (ENG-35): UI sounds from the Settings page and the pause toggle, the map's
// ambience bed under the music. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.AudioHooks

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Input/TDSettingsPage.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundWave.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUISoundHooks, "TriadDrift.Input.AudioHooks.SettingsPageAndPausePlayUISounds", TD_TEST_FLAGS)
bool FTDUISoundHooks::RunTest(const FString& Parameters)
{
	static const TCHAR* Slot = TEXT("TD_Settings_AudioHookTest");
	FTDTestWorld W;
	UTDSettingsSubsystem* S = UTDSettingsSubsystem::GetTDSettings(W.GetWorld());
	if (!TestNotNull(TEXT("settings"), S)) { return false; }
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	S->UseSlotForTests(Slot);
	ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(Slot, 0); };
	auto N = [](ETDUISound Sound) { return UTDSettingsSubsystem::GetUISoundRequestsForTests(Sound); };

	FTDSettingsPageModel M(S);
	const int32 Move0 = N(ETDUISound::Move);
	const int32 Confirm0 = N(ETDUISound::Confirm);
	M.HandleKey(EKeys::Down);
	M.HandleKey(EKeys::Gamepad_DPad_Up);
	TestEqual(TEXT("each selection move plays UIMove"), N(ETDUISound::Move) - Move0, 2);
	M.Select((int32)ETDSettingsRow::InvertMouseY);
	M.HandleKey(EKeys::Right);
	TestEqual(TEXT("changing a value plays UIConfirm"), N(ETDUISound::Confirm) - Confirm0, 1);
	M.Select((int32)ETDSettingsRow::Reset);
	M.HandleKey(EKeys::Enter);
	TestEqual(TEXT("reset plays UIConfirm"), N(ETDUISound::Confirm) - Confirm0, 2);
	M.Select((int32)ETDSettingsRow::Back);
	M.HandleKey(EKeys::Left);
	TestEqual(TEXT("left on an action row changes nothing: no confirm"), N(ETDUISound::Confirm) - Confirm0, 2);

	// The pause toggle: PauseIn on pause, PauseOut on resume.
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("controller"), PC)) { return false; }
	W.Tick();
	const int32 In0 = N(ETDUISound::PauseIn);
	const int32 Out0 = N(ETDUISound::PauseOut);
	TestTrue(TEXT("pause"), PC->SetPlayerPaused(true));
	TestEqual(TEXT("pausing plays PauseIn"), N(ETDUISound::PauseIn) - In0, 1);
	PC->SetPlayerPaused(true);
	TestEqual(TEXT("a repeated pause request plays nothing"), N(ETDUISound::PauseIn) - In0, 1);
	PC->SetPlayerPaused(false);
	TestEqual(TEXT("resuming plays PauseOut"), N(ETDUISound::PauseOut) - Out0, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAmbienceBed, "TriadDrift.Input.AudioHooks.AmbienceBedFadesInAndStopsOnTravel", TD_TEST_FLAGS)
bool FTDAmbienceBed::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("galaxy: space bed"), UTDMusicDirectorSubsystem::AmbienceSlotForMap(TEXT("UEDPIE_0_L_Galaxy")), FName(TEXT("AmbienceSpace")));
	TestEqual(TEXT("Verdance map"), UTDMusicDirectorSubsystem::AmbienceSlotForMap(TEXT("L_Planet_Verdance")), FName(TEXT("AmbienceVerdance")));
	TestEqual(TEXT("Cinder map"), UTDMusicDirectorSubsystem::AmbienceSlotForMap(TEXT("L_Planet_Cinder")), FName(TEXT("AmbienceCinder")));
	TestTrue(TEXT("menu: none"), UTDMusicDirectorSubsystem::AmbienceSlotForMap(TEXT("L_MainMenu")).IsNone());

	FTDTestWorld W;
	UTDMusicDirectorSubsystem* M = UTDMusicDirectorSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("music director"), M)) { return false; }
	USoundWave* Bed = NewObject<USoundWave>(GetTransientPackage(), TEXT("TD_TestAmbienceBed"));
	Bed->Duration = 60.f;
	Bed->bLooping = true;
	M->SetAmbienceForTests(Bed);
	M->SetMusicForTests(nullptr);
	M->SetCombatOverrideForTests(false);
	W.Tick();
	TestTrue(TEXT("the bed starts on its own (no fight needed)"), M->GetAmbienceBed() == Bed);
	W.Advance(1.f);
	TestTrue(FString::Printf(TEXT("fading in over 2 s (%.2f at 1 s)"), M->GetAmbienceEnvelope()), M->GetAmbienceEnvelope() > 0.35f && M->GetAmbienceEnvelope() < 0.65f);
	W.Advance(1.5f);
	TestEqual(TEXT("full after 2 s"), M->GetAmbienceEnvelope(), 1.f);
	M->SetTravellingForTests();
	W.Advance(1.f);
	TestTrue(TEXT("travel: fading out"), M->GetAmbienceEnvelope() < 0.6f);
	W.Advance(1.5f);
	TestEqual(TEXT("travel: silent after 2 s"), M->GetAmbienceEnvelope(), 0.f);
	return true;
}

#endif

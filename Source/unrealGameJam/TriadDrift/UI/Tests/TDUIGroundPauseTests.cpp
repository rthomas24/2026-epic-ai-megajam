// On-foot HUD and pause screen hosting (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.UI

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/UI/TDGroundHUDWidget.h"
#include "TriadDrift/UI/TDPauseMenuWidget.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "Sound/SoundWave.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDEventBus.h"

namespace TDUIGroundPauseTests
{
	/** Forces a TD.UI.* console switch for the scope of a test and restores it. */
	struct FScopedCVar
	{
		IConsoleVariable* Var = nullptr;
		int32 Previous = -1;
		FScopedCVar(const TCHAR* Name, int32 Value)
		{
			Var = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Var)
			{
				Previous = Var->GetInt();
				Var->Set(Value, ECVF_SetByCode);
			}
		}
		void Set(int32 Value) const
		{
			if (Var)
			{
				Var->Set(Value, ECVF_SetByCode);
			}
		}
		~FScopedCVar()
		{
			if (Var)
			{
				Var->Set(Previous, ECVF_SetByCode);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIGroundHUDByMode, "TriadDrift.UI.GroundHUD.ShownOnFootOnly", TD_TEST_FLAGS)
bool FTDUIGroundHUDByMode::RunTest(const FString& Parameters)
{
	using namespace TDUIGroundPauseTests;
	FScopedCVar Ground(TEXT("TD.UI.GroundHUD"), 1);
	FScopedCVar Space(TEXT("TD.UI.NativeHUD"), 0);
	FTDTestWorld W;
	ATDFirstPersonCharacter* Walker = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Walker, ATDSpacePlayerController::StaticClass());
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("walker"), Walker) || !TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("host"), Host))
	{
		return false;
	}
	Host->bAddToViewport = false;

	// 1. On foot: the ground HUD, the dev HUD in supplement mode, its surfaces claimed.
	Host->Evaluate();
	UTDGroundHUDWidget* HUD = Cast<UTDGroundHUDWidget>(Host->GetGroundHUD());
	TestNotNull(TEXT("on foot: the ground HUD is up (native class)"), HUD);
	TestNull(TEXT("on foot: no space HUD"), Host->GetSpaceHUD());
	TestTrue(TEXT("on foot: dev HUD only supplements"), ATDDebugHUD::IsRealHUDActive(W.GetWorld()));
	TestTrue(TEXT("on foot: damage chevrons claimed"), UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(W.GetWorld(), ETDHUDSurface::DamageIndicators));
	TestTrue(TEXT("on foot: interact prompt claimed"), UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(W.GetWorld(), ETDHUDSurface::InteractPrompt));

	// 2. It reads the walker: health, dash charges and their maximum, a hit with its direction.
	if (HUD)
	{
		HUD->Sample(0.1f);
		TestEqual(TEXT("dash pips = tuning"), HUD->GetView().DashMax, FMath::Max(1, Walker->Tuning.DashCharges));
		TestEqual(TEXT("dash charges"), HUD->GetView().Dash, Walker->GetDashCharges());
		TestTrue(TEXT("health read"), FMath::IsNearlyEqual(HUD->GetView().Health, Walker->GetHealth01(), 0.001f));
		AActor* Shooter = W.Spawn<AActor>(FVector(800.f, 300.f, 95.f));
		UTDStatics::ApplyTDDamage(Walker, 5.f, ETDDamageKind::Energy, Shooter, Shooter, Walker->GetActorLocation(), FVector(-1.f, 0.f, 0.f));
		HUD->Sample(0.1f);
		TestEqual(TEXT("a hit gives a damage chevron"), HUD->GetView().Hits, 1);
		TestTrue(TEXT("health follows the hit"), HUD->GetView().Health < 1.f);
		HUD->Sample(UTDGroundHUDWidget::HitSeconds + 0.1f);
		TestEqual(TEXT("the chevron expires"), HUD->GetView().Hits, 0);
	}

	// 3. Switched off: the dev HUD's text comes back.
	Ground.Set(0);
	Host->Evaluate();
	TestNull(TEXT("TD.UI.GroundHUD 0: no ground HUD"), Host->GetGroundHUD());
	TestFalse(TEXT("TD.UI.GroundHUD 0: dev HUD draws in full"), ATDDebugHUD::IsRealHUDActive(W.GetWorld()));
	Ground.Set(1);

	// 4. In a ship: no ground HUD; back on foot: it returns.
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 5000.f));
	PC->Possess(Ship);
	Host->Evaluate();
	TestNull(TEXT("in a ship: no ground HUD"), Host->GetGroundHUD());
	PC->Possess(Walker);
	Host->Evaluate();
	TestNotNull(TEXT("back on foot: the ground HUD again"), Host->GetGroundHUD());

	// 5. A cinematic (T3's intro) hides every HUD at once, blocks the pause screen, and restores exactly.
	AHUD* DevHUD = W.Spawn<AHUD>();
	PC->MyHUD = DevHUD;
	DevHUD->bShowHUD = true;
	Host->HandlePauseChanged(true);
	TestNotNull(TEXT("before: pause screen up"), Host->GetPauseScreen());
	UTDUIHostSubsystem::SetCinematicHiddenIn(W.GetWorld(), true);
	TestNull(TEXT("cinematic: ground HUD hidden at once"), Host->GetGroundHUD());
	TestNull(TEXT("cinematic: pause screen removed"), Host->GetPauseScreen());
	TestFalse(TEXT("cinematic: fallback debug HUD stops drawing"), DevHUD->bShowHUD);
	Host->Evaluate();
	Host->HandlePauseChanged(true);
	TestNull(TEXT("cinematic: stays hidden through re-evaluation"), Host->GetGroundHUD());
	TestNull(TEXT("cinematic: the pause screen can't open"), Host->GetPauseScreen());
	Host->HandlePauseChanged(false);
	UTDUIHostSubsystem::SetCinematicHiddenIn(W.GetWorld(), false);
	TestNotNull(TEXT("cinematic over: ground HUD back"), Host->GetGroundHUD());
	TestTrue(TEXT("cinematic over: debug HUD drawing restored"), DevHUD->bShowHUD);
	// Restores exactly: a HUD that was already off stays off.
	DevHUD->bShowHUD = false;
	Host->SetCinematicHidden(true);
	Host->SetCinematicHidden(false);
	TestFalse(TEXT("restores exactly: an already-hidden debug HUD stays hidden"), DevHUD->bShowHUD);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIPauseNavigation, "TriadDrift.UI.Pause.KeyboardAndPadNavigation", TD_TEST_FLAGS)
bool FTDUIPauseNavigation::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	APlayerController* PC = W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass());
	UTDPauseMenuWidget* Menu = PC ? CreateWidget<UTDPauseMenuWidget>(PC, UTDPauseMenuWidget::StaticClass()) : nullptr;
	if (!TestNotNull(TEXT("pause menu"), Menu))
	{
		return false;
	}
	Menu->bExecuteActions = false;
	TestEqual(TEXT("opens on RESUME"), Menu->GetSelected(), 0);
	// The difficulty in play (T1's API, read by reflection): a name when it is in this build, else no line.
	const bool bDifficultyApi = UTDSettingsSubsystem::StaticClass()->FindFunctionByName(TEXT("GetDifficultyInPlay")) != nullptr;
	TestEqual(TEXT("difficulty line iff the API is in the build"), !UTDPauseMenuWidget::ReadDifficultyInPlay().IsEmpty(), bDifficultyApi);
	TestTrue(TEXT("S moves down"), Menu->HandleKey(EKeys::S));
	TestEqual(TEXT("SETTINGS"), Menu->GetSelected(), 1);
	Menu->HandleKey(EKeys::Gamepad_DPad_Down);
	TestEqual(TEXT("pad down: RESTART SCENE"), Menu->GetSelected(), 2);
	Menu->HandleKey(EKeys::Up);
	Menu->HandleKey(EKeys::Gamepad_LeftStick_Up);
	Menu->HandleKey(EKeys::W);
	TestEqual(TEXT("up wraps to QUIT TO MENU"), Menu->GetSelected(), 3);

	// Quit asks first, CANCEL pre-selected.
	Menu->HandleKey(EKeys::Enter);
	TestTrue(TEXT("quit asks to confirm"), Menu->IsConfirming());
	TestEqual(TEXT("CANCEL first"), Menu->GetConfirmChoice(), 1);
	Menu->HandleKey(EKeys::Gamepad_FaceButton_Bottom);
	TestFalse(TEXT("cancel closes the prompt"), Menu->IsConfirming());
	TestTrue(TEXT("cancel does nothing"), Menu->LastAction.IsEmpty());
	Menu->HandleKey(EKeys::Enter);
	Menu->HandleKey(EKeys::Left);
	TestEqual(TEXT("left: CONFIRM"), Menu->GetConfirmChoice(), 0);
	Menu->HandleKey(EKeys::Enter);
	TestEqual(TEXT("confirmed quit"), Menu->LastAction, FString(TEXT("quit")));

	// Restart: B leaves the prompt; then confirm.
	Menu->HandleKey(EKeys::Gamepad_DPad_Up);
	TestEqual(TEXT("RESTART SCENE"), Menu->GetSelected(), 2);
	Menu->HandleKey(EKeys::SpaceBar);
	TestTrue(TEXT("restart asks to confirm"), Menu->IsConfirming());
	Menu->HandleKey(EKeys::Gamepad_FaceButton_Right);
	TestFalse(TEXT("B leaves the prompt"), Menu->IsConfirming());
	Menu->HandleKey(EKeys::Enter);
	Menu->HandleKey(EKeys::Gamepad_DPad_Right);
	Menu->HandleKey(EKeys::Enter);
	TestEqual(TEXT("confirmed restart"), Menu->LastAction, FString(TEXT("restart")));

	// Settings and resume act at once; B resumes; Escape is left to the controller's pause toggle.
	Menu->HandleKey(EKeys::Up);
	Menu->HandleKey(EKeys::Enter);
	TestTrue(TEXT("settings"), Menu->LastAction.StartsWith(TEXT("settings")));
	Menu->HandleKey(EKeys::Gamepad_FaceButton_Right);
	TestEqual(TEXT("B resumes"), Menu->LastAction, FString(TEXT("resume")));
	TestFalse(TEXT("Escape not consumed (the pause toggle closes the menu)"), Menu->HandleKey(EKeys::Escape));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIPauseScreenFollowsPause, "TriadDrift.UI.Pause.ScreenFollowsThePause", TD_TEST_FLAGS)
bool FTDUIPauseScreenFollowsPause::RunTest(const FString& Parameters)
{
	using namespace TDUIGroundPauseTests;
	FScopedCVar Pause(TEXT("TD.UI.PauseScreen"), 1);
	FTDTestWorld W;
	ATDFirstPersonCharacter* Walker = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Walker, ATDSpacePlayerController::StaticClass()));
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("host"), Host))
	{
		return false;
	}
	Host->bAddToViewport = false;
	Host->Evaluate(); // binds OnPauseChanged

	// The real pause path: the controller pauses and broadcasts; the host opens the screen.
	if (PC->SetPlayerPaused(true))
	{
		TestTrue(TEXT("paused: the pause screen is up"), Cast<UTDPauseMenuWidget>(Host->GetPauseScreen()) != nullptr);
		TestTrue(TEXT("paused: the dev HUD's PAUSED line is claimed"), UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(W.GetWorld(), ETDHUDSurface::Pause));
		PC->SetPlayerPaused(false);
		TestNull(TEXT("resumed: the pause screen is gone"), Host->GetPauseScreen());
	}
	else
	{
		AddWarning(TEXT("SetPlayerPaused refused in the test world; driving OnPauseChanged directly."));
	}
	Host->HandlePauseChanged(true);
	TestNotNull(TEXT("OnPauseChanged(true) opens it"), Host->GetPauseScreen());
	Host->HandlePauseChanged(false);
	TestNull(TEXT("OnPauseChanged(false) closes it"), Host->GetPauseScreen());
	Pause.Set(0);
	Host->HandlePauseChanged(true);
	TestNull(TEXT("TD.UI.PauseScreen 0: no screen (the dev HUD line)"), Host->GetPauseScreen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIHitConfirmSounds, "TriadDrift.UI.HitConfirmSounds", TD_TEST_FLAGS)
bool FTDUIHitConfirmSounds::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass());
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	if (!TestNotNull(TEXT("host"), Host) || !TestNotNull(TEXT("bus"), Bus))
	{
		return false;
	}
	Host->bAddToViewport = false;
	UTDUIStyle* Style = NewObject<UTDUIStyle>();
	Host->StyleOverride = Style;
	Host->Evaluate(); // binds OnHitConfirmed

	// Unset slots: silent.
	Bus->BroadcastHitConfirmed(nullptr, false);
	Bus->BroadcastHitConfirmed(nullptr, true);
	TestEqual(TEXT("unset: no hit sound"), Host->GetHitSoundsPlayed(), 0);
	TestEqual(TEXT("unset: no kill sound"), Host->GetKillSoundsPlayed(), 0);

	// Set: a hit plays HitConfirm, a kill plays KillConfirm (never both).
	USoundWave* Hit = NewObject<USoundWave>(Style);
	USoundWave* Kill = NewObject<USoundWave>(Style);
	Style->HitConfirm.Sound = Hit;
	Style->KillConfirm.Sound = Kill;
	Bus->BroadcastHitConfirmed(nullptr, false);
	Bus->BroadcastHitConfirmed(nullptr, false);
	Bus->BroadcastHitConfirmed(nullptr, true);
	TestEqual(TEXT("two hits: two hit sounds"), Host->GetHitSoundsPlayed(), 2);
	TestEqual(TEXT("one kill: one kill sound"), Host->GetKillSoundsPlayed(), 1);

	// Only the kill slot set: hits stay silent, kills still sound.
	Style->HitConfirm.Sound = nullptr;
	Bus->BroadcastHitConfirmed(nullptr, false);
	Bus->BroadcastHitConfirmed(nullptr, true);
	TestEqual(TEXT("hit slot unset: still two"), Host->GetHitSoundsPlayed(), 2);
	TestEqual(TEXT("kill slot set: two kills"), Host->GetKillSoundsPlayed(), 2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

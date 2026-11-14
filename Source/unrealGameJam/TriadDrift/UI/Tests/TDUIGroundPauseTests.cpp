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
#include "Input/HittestGrid.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SWindow.h"
#include "UObject/StrongObjectPtr.h"

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

// The Warden's bar (T5's ATDCrewCharacter::GetActiveBoss): none without a Warden; shown with name and phase; phase 2
// under half hull; a hit leaves a ghost that holds, then drains to the new hull; painted centred under the objective.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIGroundBossBar, "TriadDrift.UI.GroundHUD.WardenBossBar", TD_TEST_FLAGS)
bool FTDUIGroundBossBar::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Walker = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Walker, ATDSpacePlayerController::StaticClass());
	const TStrongObjectPtr<UTDGroundHUDWidget> Keep(PC ? CreateWidget<UTDGroundHUDWidget>(PC, UTDGroundHUDWidget::StaticClass()) : nullptr);
	UTDGroundHUDWidget* HUD = Keep.Get();
	if (!TestNotNull(TEXT("ground HUD"), HUD))
	{
		return false;
	}
	HUD->Sample(.1f);
	TestFalse(TEXT("no Warden: no bar"), HUD->GetView().bBoss);
	HUD->ForceBoss01 = .8f;
	HUD->Sample(.1f);
	TestTrue(TEXT("Warden: bar, phase 1, its name"), HUD->GetView().bBoss && HUD->GetView().BossPhase == 1 && HUD->GetView().BossName == TEXT("WARDEN"));
	HUD->ForceBoss01 = .4f;
	HUD->Sample(.05f);
	TestEqual(TEXT("under half: phase 2"), HUD->GetView().BossPhase, 2);
	TestTrue(TEXT("the hit leaves a ghost at 0.8"), FMath::IsNearlyEqual(HUD->GetBossGhost01(), .8f, .01f));
	HUD->Sample(.3f);
	TestTrue(TEXT("the ghost holds a moment"), FMath::IsNearlyEqual(HUD->GetBossGhost01(), .8f, .01f));
	for (int32 I = 0; I < 30; ++I)
	{
		HUD->Sample(.1f);
	}
	TestTrue(TEXT("then drains to the hull"), FMath::IsNearlyEqual(HUD->GetBossGhost01(), .4f, .01f));

	TSharedRef<SWidget> Slate = HUD->TakeWidget();
	TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(FVector2D(1920, 1080));
	FSlateWindowElementList List(Window);
	FHittestGrid Grid;
	Slate->SlatePrepass(1.f);
	Slate->Paint(FPaintArgs(nullptr, Grid, FVector2D::ZeroVector, FPlatformTime::Seconds(), .016f), FGeometry::MakeRoot(FVector2D(1920, 1080), FSlateLayoutTransform()),
		FSlateRect(0, 0, 1920, 1080), List, 0, FWidgetStyle(), true);
	const TPair<FName, FBox2D>* Box = HUD->LastBoxes.FindByPredicate([](const TPair<FName, FBox2D>& B) { return B.Key == TEXT("boss"); });
	if (TestNotNull(TEXT("boss bar painted"), Box))
	{
		TestTrue(TEXT("centred, under the objective panel"), FMath::IsNearlyEqual(Box->Value.GetCenter().X, 960.f, 1.f) && Box->Value.Min.Y >= 145.f);
	}
	HUD->ForceBoss01 = -1.f;
	HUD->Sample(.1f);
	TestFalse(TEXT("Warden dead: bar gone"), HUD->GetView().bBoss);
	return true;
}

// T7's playtest: the objective panel grows to fit the longest line we have (50 characters) instead of running past its
// edge, stays centred, wraps a line too long for 1000 px to two lines, and pushes the meter and the boss bar below it.
// ENG-36 on-foot story states: a counted step's pips under the objective (the reactor conduits 0/4 -> 4/4, a flash on
// each), the escape clock (T5's EscapeSeconds, not hard-coded) with the exit marker, and the host's sensor flicker
// (UNKNOWN, VERIFIED for a moment, UNKNOWN).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIGroundStoryStates, "TriadDrift.UI.GroundHUD.StepsEscapeAndSensor", TD_TEST_FLAGS)
bool FTDUIGroundStoryStates::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Walker = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Walker, ATDSpacePlayerController::StaticClass());
	const TStrongObjectPtr<UTDGroundHUDWidget> Keep(PC ? CreateWidget<UTDGroundHUDWidget>(PC, UTDGroundHUDWidget::StaticClass()) : nullptr);
	UTDGroundHUDWidget* HUD = Keep.Get();
	if (!TestNotNull(TEXT("ground HUD"), HUD))
	{
		return false;
	}
	auto Paint = [&]()
	{
		TSharedRef<SWidget> Slate = HUD->TakeWidget();
		TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(FVector2D(1920, 1080));
		FSlateWindowElementList List(Window);
		FHittestGrid Grid;
		Slate->SlatePrepass(1.f);
		Slate->Paint(FPaintArgs(nullptr, Grid, FVector2D::ZeroVector, FPlatformTime::Seconds(), .016f), FGeometry::MakeRoot(FVector2D(1920, 1080), FSlateLayoutTransform()),
			FSlateRect(0, 0, 1920, 1080), List, 0, FWidgetStyle(), true);
	};
	auto Box = [&](const TCHAR* Name) { const TPair<FName, FBox2D>* B = HUD->LastBoxes.FindByPredicate([Name](const TPair<FName, FBox2D>& X) { return X.Key == FName(Name); }); return B ? B->Value : FBox2D(ForceInit); };
	HUD->ForceObjective = TEXT("Break the reactor conduits");

	HUD->Sample(.1f);
	TestEqual(TEXT("no counted step: no pips"), HUD->GetView().StepRequired, 0);
	HUD->ForceStepRequired = 4;
	HUD->ForceStepCount = 0;
	HUD->ForceStepNoun = TEXT("CONDUITS");
	HUD->Sample(.1f);
	TestTrue(TEXT("CONDUITS 0 / 4"), HUD->GetView().StepRequired == 4 && HUD->GetView().StepCount == 0 && HUD->GetView().StepNoun == TEXT("CONDUITS"));
	HUD->ForceStepCount = 1;
	HUD->Sample(.1f);
	TestTrue(TEXT("one down: 1 / 4 and a flash"), HUD->GetView().StepCount == 1 && HUD->GetView().StepFlash01 > .5f);
	Paint();
	const FBox2D Steps = Box(TEXT("steps"));
	const FBox2D Obj = Box(TEXT("objective"));
	TestTrue(TEXT("pips painted under the objective panel, centred"), Steps.bIsValid && Obj.bIsValid && Steps.Min.Y >= Obj.Max.Y && FMath::IsNearlyEqual(Steps.GetCenter().X, 960.f, 1.f));

	HUD->ForceEscapeLeft = 72.f;
	HUD->ForceEscapeTotal = 90.f;
	HUD->ForceExitAt = FVector(5000.f, 0.f, 95.f);
	HUD->Sample(.1f);
	TestTrue(TEXT("escape: the clock reads T5's time"), HUD->GetView().bEscape && FMath::IsNearlyEqual(HUD->GetView().EscapeLeft, 72.f) && FMath::IsNearlyEqual(HUD->GetView().EscapeTotal, 90.f));
	TestTrue(TEXT("the exit marker, with a range"), HUD->GetView().bExitMarker && HUD->GetView().ExitMetres > 40.f);
	Paint();
	const FBox2D Esc = Box(TEXT("escape"));
	TestTrue(TEXT("the clock under the pips"), Esc.bIsValid && Steps.bIsValid && Esc.Min.Y >= Box(TEXT("steps")).Max.Y);
	TestTrue(TEXT("exit marker painted inside the frame"), Box(TEXT("exit")).bIsValid && Box(TEXT("exit")).Min.X >= 0.f && Box(TEXT("exit")).Max.X <= 1920.f);
	HUD->ForceEscapeLeft = -1.f;
	HUD->ForceStepRequired = -1;
	HUD->Sample(.1f);
	TestTrue(TEXT("both gone after"), !HUD->GetView().bEscape && HUD->GetView().StepRequired == 0 && !HUD->GetView().bExitMarker);

	TestTrue(TEXT("host sensor flicker"), UTDUIHostSubsystem::ShowSensorFlicker(W.GetWorld(), FVector(3000.f, 0.f, 150.f), 6.f));
	HUD->Sample(.1f);
	TestTrue(TEXT("sensor: UNKNOWN first"), HUD->GetView().bSensor && !HUD->GetView().bSensorVerified);
	W.Advance(UTDUIHostSubsystem::SensorVerifiedAt + .2f);
	HUD->Sample(.1f);
	TestTrue(TEXT("then VERIFIED for a moment"), HUD->GetView().bSensorVerified);
	W.Advance(1.f);
	HUD->Sample(.1f);
	TestTrue(TEXT("then UNKNOWN again"), HUD->GetView().bSensor && !HUD->GetView().bSensorVerified);
	W.Advance(6.f);
	HUD->Sample(.1f);
	TestFalse(TEXT("gone after its seconds"), HUD->GetView().bSensor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIGroundObjectiveFits, "TriadDrift.UI.GroundHUD.ObjectivePanelFitsItsLine", TD_TEST_FLAGS)
bool FTDUIGroundObjectiveFits::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Walker = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Walker, ATDSpacePlayerController::StaticClass());
	const TStrongObjectPtr<UTDGroundHUDWidget> Keep(PC ? CreateWidget<UTDGroundHUDWidget>(PC, UTDGroundHUDWidget::StaticClass()) : nullptr);
	UTDGroundHUDWidget* HUD = Keep.Get();
	if (!TestNotNull(TEXT("ground HUD"), HUD))
	{
		return false;
	}
	auto PaintAndGet = [&](const TCHAR* Name)
	{
		HUD->Sample(.1f);
		TSharedRef<SWidget> Slate = HUD->TakeWidget();
		TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(FVector2D(1920, 1080));
		FSlateWindowElementList List(Window);
		FHittestGrid Grid;
		Slate->SlatePrepass(1.f);
		Slate->Paint(FPaintArgs(nullptr, Grid, FVector2D::ZeroVector, FPlatformTime::Seconds(), .016f), FGeometry::MakeRoot(FVector2D(1920, 1080), FSlateLayoutTransform()),
			FSlateRect(0, 0, 1920, 1080), List, 0, FWidgetStyle(), true);
		const TPair<FName, FBox2D>* B = HUD->LastBoxes.FindByPredicate([Name](const TPair<FName, FBox2D>& X) { return X.Key == FName(Name); });
		return B ? B->Value : FBox2D(ForceInit);
	};
	HUD->ForceObjective = TEXT("Head to a signal · the research site or the relay");
	const FBox2D Long = PaintAndGet(TEXT("objective"));
	TestTrue(FString::Printf(TEXT("the 50-character line widens the panel (%.0f px)"), Long.GetSize().X), Long.GetSize().X > 506.f && Long.GetSize().X <= 1000.f);
	TestTrue(TEXT("centred"), FMath::IsNearlyEqual(Long.GetCenter().X, 960.f, 1.f));
	TestTrue(TEXT("one line"), FMath::IsNearlyEqual(Long.GetSize().Y, 92.f, 1.f));
	HUD->ForceObjective = TEXT("A very long objective line that keeps going well past anything the panel could hold on a single line at all");
	const FBox2D Wrapped = PaintAndGet(TEXT("objective"));
	TestTrue(TEXT("too long: two lines, at most 1000 px"), Wrapped.GetSize().Y > 100.f && Wrapped.GetSize().X <= 1000.f);
	HUD->ForceTransfer01 = .5f;
	HUD->ForceBoss01 = .8f;
	const FBox2D Meter = PaintAndGet(TEXT("transfer"));
	const FBox2D Boss = PaintAndGet(TEXT("boss"));
	TestTrue(TEXT("the meter sits below a two-line panel"), Meter.Min.Y >= Wrapped.Max.Y);
	TestTrue(TEXT("the boss bar below the meter"), Boss.Min.Y >= Meter.Max.Y);
	return true;
}

// Verdance's archive transfer meter (T5's ATDArchiveTransfer): none without a transfer; shown with its progress while
// one runs; paused is flagged; painted as one plate under the objective panel, inside the frame.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIGroundTransferMeter, "TriadDrift.UI.GroundHUD.ArchiveTransferMeter", TD_TEST_FLAGS)
bool FTDUIGroundTransferMeter::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Walker = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Walker, ATDSpacePlayerController::StaticClass());
	const TStrongObjectPtr<UTDGroundHUDWidget> Keep(PC ? CreateWidget<UTDGroundHUDWidget>(PC, UTDGroundHUDWidget::StaticClass()) : nullptr); // no GC mid-test
	UTDGroundHUDWidget* HUD = Keep.Get();
	if (!TestNotNull(TEXT("ground HUD"), HUD))
	{
		return false;
	}
	HUD->Sample(.1f);
	TestFalse(TEXT("no transfer in this world: no meter"), HUD->GetView().bTransfer);

	HUD->ForceTransfer01 = .63f;
	HUD->Sample(.1f);
	TestTrue(TEXT("running: meter shown"), HUD->GetView().bTransfer);
	TestTrue(TEXT("running: progress read"), FMath::IsNearlyEqual(HUD->GetView().Transfer01, .63f, .001f));
	TestFalse(TEXT("running: not paused"), HUD->GetView().bTransferPaused);
	HUD->bForceTransferPaused = true;
	HUD->Sample(.1f);
	TestTrue(TEXT("crewman at the terminal: paused"), HUD->GetView().bTransferPaused);

	TSharedRef<SWidget> Slate = HUD->TakeWidget();
	TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(FVector2D(1920, 1080));
	FSlateWindowElementList List(Window);
	FHittestGrid Grid;
	Slate->SlatePrepass(1.f);
	Slate->Paint(FPaintArgs(nullptr, Grid, FVector2D::ZeroVector, FPlatformTime::Seconds(), .016f), FGeometry::MakeRoot(FVector2D(1920, 1080), FSlateLayoutTransform()),
		FSlateRect(0, 0, 1920, 1080), List, 0, FWidgetStyle(), true);
	const TPair<FName, FBox2D>* Box = HUD->LastBoxes.FindByPredicate([](const TPair<FName, FBox2D>& B) { return B.Key == TEXT("transfer"); });
	if (TestNotNull(TEXT("meter painted"), Box))
	{
		TestTrue(TEXT("centred"), FMath::IsNearlyEqual(Box->Value.GetCenter().X, 960.f, 1.f));
		TestTrue(TEXT("under the objective panel (ends at y 145)"), Box->Value.Min.Y >= 145.f && Box->Value.Max.Y < 300.f);
	}

	HUD->ForceTransfer01 = -1.f;
	HUD->Sample(.1f);
	TestFalse(TEXT("transfer over: meter gone"), HUD->GetView().bTransfer);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIPauseNavigation, "TriadDrift.UI.Pause.KeyboardAndPadNavigation", TD_TEST_FLAGS)
bool FTDUIPauseNavigation::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	APlayerController* PC = W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass());
	const TStrongObjectPtr<UTDPauseMenuWidget> Keep(PC ? CreateWidget<UTDPauseMenuWidget>(PC, UTDPauseMenuWidget::StaticClass()) : nullptr); // no GC mid-test
	UTDPauseMenuWidget* Menu = Keep.Get();
	if (!TestNotNull(TEXT("pause menu"), Menu))
	{
		return false;
	}
	Menu->bExecuteActions = false;
	TestEqual(TEXT("opens on RESUME"), Menu->GetSelected(), 0);
	// The footer follows the device (T1's gamepad pass).
	Menu->ForceGamepad = 0;
	TestTrue(TEXT("keyboard footer"), Menu->GetFooterText().Contains(TEXT("[ENTER]")));
	Menu->ForceGamepad = 1;
	TestTrue(TEXT("pad footer: A / START, no keys"), Menu->GetFooterText().Contains(TEXT("[A]")) && Menu->GetFooterText().Contains(TEXT("[START]"))
		&& !Menu->GetFooterText().Contains(TEXT("ENTER")));
	Menu->ForceGamepad = -1;
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

	// UI sounds (T1's PlayUISound): Move per highlight change (rows and the confirm buttons), Confirm per choice, Back
	// only when a prompt is cancelled (closing by resuming plays PauseOut from the controller instead).
	TestEqual(TEXT("UI sound: Move per highlight change"), Menu->GetUISoundRequests(TEXT("Move")), 9);
	TestEqual(TEXT("UI sound: Confirm per choice"), Menu->GetUISoundRequests(TEXT("Confirm")), 8);
	TestEqual(TEXT("UI sound: Back for the cancelled prompt only"), Menu->GetUISoundRequests(TEXT("Back")), 1);
	const bool bSoundApi = UTDSettingsSubsystem::StaticClass()->FindFunctionByName(TEXT("PlayUISound")) != nullptr;
	TestEqual(TEXT("PlayUISound reaches T1's API iff it is in this build"), Menu->PlayUISound(TEXT("Move")), bSoundApi);
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
	W.Advance(.1f); // past the 60 ms hit-tick throttle
	Bus->BroadcastHitConfirmed(nullptr, false);
	Bus->BroadcastHitConfirmed(nullptr, true);
	TestEqual(TEXT("two hits 0.1 s apart: two hit sounds"), Host->GetHitSoundsPlayed(), 2);
	TestEqual(TEXT("one kill: one kill sound"), Host->GetKillSoundsPlayed(), 1);
	// Rapid fire: hits inside 60 ms of the last tick are dropped; a kill in the same frame still plays.
	const int32 Throttled = Host->GetHitSoundsThrottled();
	Bus->BroadcastHitConfirmed(nullptr, false);
	Bus->BroadcastHitConfirmed(nullptr, false);
	Bus->BroadcastHitConfirmed(nullptr, true);
	TestEqual(TEXT("rapid fire: no extra hit ticks"), Host->GetHitSoundsPlayed(), 2);
	TestEqual(TEXT("rapid fire: both throttled"), Host->GetHitSoundsThrottled(), Throttled + 2);
	TestEqual(TEXT("a kill is never throttled"), Host->GetKillSoundsPlayed(), 2);
	W.Advance(.1f);

	// Only the kill slot set: hits stay silent, kills still sound.
	Style->HitConfirm.Sound = nullptr;
	Bus->BroadcastHitConfirmed(nullptr, false);
	Bus->BroadcastHitConfirmed(nullptr, true);
	TestEqual(TEXT("hit slot unset: still two"), Host->GetHitSoundsPlayed(), 2);
	TestEqual(TEXT("kill slot set: three kills"), Host->GetKillSoundsPlayed(), 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

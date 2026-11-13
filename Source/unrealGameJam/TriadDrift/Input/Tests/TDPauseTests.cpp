// Pause: Escape / Start toggles the game pause in space and on foot (real keys), through the controller.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Pause

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

namespace TDPauseTest
{
	const FKey TapKey = EKeys::Escape;
	void Tap(FTDTestWorld& W, APlayerController* PC, float Dt)
	{
		FTDTestWorld::InjectKey(PC, TapKey, true);
		W.Tick(Dt);
		FTDTestWorld::InjectKey(PC, TapKey, false);
		W.Tick(Dt);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPauseSpace, "TriadDrift.Input.Pause.EscapeTogglesInSpace", TD_TEST_FLAGS)
bool FTDPauseSpace::RunTest(const FString& Parameters)
{
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		const float Dt = 1.f / Fps;
		const FString Tag = FString::Printf(TEXT("%.0f fps: "), Fps);
		FTDTestWorld W;
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 1500.f));
		ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
		if (!TestNotNull(*(Tag + TEXT("controller")), PC)) { return false; }
		Ship->SetPlayerControlled(true);
		W.Tick(Dt);
		UTDFlightEventCounter* Rec = NewObject<UTDFlightEventCounter>();
		Rec->AddToRoot();
		ON_SCOPE_EXIT { Rec->RemoveFromRoot(); };
		PC->OnPauseChanged.AddDynamic(Rec, &UTDFlightEventCounter::HandlePause);
		const int32& Changes = Rec->PauseChanges;
		const bool& bLast = Rec->bLastPaused;

		TDPauseTest::Tap(W, PC, Dt);
		TestTrue(*(Tag + TEXT("Escape pauses")), W.GetWorld()->IsPaused() && PC->IsGamePausedByPlayer());
		TestTrue(*(Tag + TEXT("OnPauseChanged(true)")), Changes == 1 && bLast);
		TestTrue(*(Tag + TEXT("cursor shown for the menu")), PC->bShowMouseCursor);
		const FVector P0 = Ship->GetActorLocation();
		const double T0 = W.GetWorld()->GetTimeSeconds();
		for (int32 i = 0; i < FMath::RoundToInt(Fps); ++i) { W.Tick(Dt); }
		TestTrue(*(Tag + TEXT("world time stops")), FMath::IsNearlyEqual(W.GetWorld()->GetTimeSeconds(), T0, 1e-6));
		TestTrue(*(Tag + TEXT("the ship doesn't move")), Ship->GetActorLocation().Equals(P0, 0.1f));

		TDPauseTest::Tap(W, PC, Dt); // Escape still works while paused
		TestFalse(*(Tag + TEXT("Escape again resumes")), W.GetWorld()->IsPaused() || PC->IsGamePausedByPlayer());
		TestTrue(*(Tag + TEXT("OnPauseChanged(false)")), Changes == 2 && !bLast);
		TestFalse(*(Tag + TEXT("cursor hidden again")), PC->bShowMouseCursor);
		W.Advance(0.5f, Dt);
		TestTrue(*(Tag + TEXT("time and flight resume")), W.GetWorld()->GetTimeSeconds() > T0 + 0.4 && !Ship->GetActorLocation().Equals(P0, 100.f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPauseSlowMo, "TriadDrift.Input.Pause.SlowMotionSurvivesAPause", TD_TEST_FLAGS)
bool FTDPauseSlowMo::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 1500.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("controller"), PC)) { return false; }
	W.Tick();
	AWorldSettings* Settings = W.GetWorld()->GetWorldSettings();
	Settings->SetTimeDilation(0.3f); // a director money shot in progress
	TestTrue(TEXT("pause"), PC->SetPlayerPaused(true));
	W.Advance(0.5f);
	TestTrue(TEXT("resume"), PC->SetPlayerPaused(false));
	TestEqual(TEXT("time dilation untouched by the pause"), Settings->TimeDilation, 0.3f, 0.001f);
	Settings->SetTimeDilation(1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPauseRefused, "TriadDrift.Input.Pause.RefusedDuringRetakeTravelAndVictory", TD_TEST_FLAGS)
bool FTDPauseRefused::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetWorld()->GetGameInstance());
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 1500.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("flow"), GI) || !TestNotNull(TEXT("controller"), PC)) { return false; }
	W.Tick();
	TestTrue(TEXT("normally allowed"), PC->CanPauseNow());
	GI->SetPhase(ETDGamePhase::Victory);
	TestFalse(TEXT("victory sequence: refused"), PC->SetPlayerPaused(true));
	TestFalse(TEXT("not paused"), W.GetWorld()->IsPaused());
	GI->SetPhase(ETDGamePhase::Waves1to2);
	TestTrue(TEXT("allowed again"), PC->SetPlayerPaused(true));
	TestTrue(TEXT("and resumes"), PC->SetPlayerPaused(false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPauseFoot, "TriadDrift.Input.Pause.EscapeTogglesOnFoot", TD_TEST_FLAGS)
bool FTDPauseFoot::RunTest(const FString& Parameters)
{
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		const float Dt = 1.f / Fps;
		const FString Tag = FString::Printf(TEXT("%.0f fps: "), Fps);
		FTDTestWorld W;
		ATDFirstPersonCharacter* Pilot = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 200.f));
		ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Pilot, ATDSpacePlayerController::StaticClass()));
		if (!TestNotNull(*(Tag + TEXT("controller")), PC) || !Pilot) { return false; }
		W.Tick(Dt);
		W.Tick(Dt);
		TDPauseTest::Tap(W, PC, Dt);
		TestTrue(*(Tag + TEXT("Escape on foot pauses")), W.GetWorld()->IsPaused() && PC->IsGamePausedByPlayer());
		const double T0 = W.GetWorld()->GetTimeSeconds();
		for (int32 i = 0; i < 10; ++i) { W.Tick(Dt); }
		TestTrue(*(Tag + TEXT("world time stops")), FMath::IsNearlyEqual(W.GetWorld()->GetTimeSeconds(), T0, 1e-6));
		TDPauseTest::Tap(W, PC, Dt);
		TestFalse(*(Tag + TEXT("Escape again resumes")), W.GetWorld()->IsPaused());
	}
	return true;
}


bool TDInputDebug_TurnTowardStep(APlayerController* PC, const FRotator& Target);

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInputTurnToward, "TriadDrift.Input.Agent.TurnTowardSteersThroughRealInput", TD_TEST_FLAGS)
bool FTDInputTurnToward::RunTest(const FString& Parameters)
{
	// Agent pilots (T2 / T4) need to bring a target behind them onto the nose with the same input a player uses.
	struct FCase { float Yaw; float Pitch; };
	const FCase Cases[] = { { 90.f, 0.f }, { 180.f, 0.f }, { -60.f, 35.f } };
	for (const FCase& C : Cases)
	{
		const FString Tag = FString::Printf(TEXT("bearing yaw %.0f pitch %.0f: "), C.Yaw, C.Pitch);
		FTDTestWorld W;
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 1500.f));
		ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
		if (!TestNotNull(*(Tag + TEXT("controller")), PC)) { return false; }
		Ship->SetPlayerControlled(true);
		W.Tick();
		const FRotator Target(C.Pitch, C.Yaw, 0.f);
		bool bOn = false;
		bool bUsedTheStick = false;
		int32 Frames = 0;
		for (; Frames < 60 * 6 && !bOn; ++Frames)
		{
			bOn = TDInputDebug_TurnTowardStep(PC, Target);
			W.Tick();
			bUsedTheStick |= !PC->GetCurrentFlightInput().Aim.IsNearlyZero();
		}
		TestTrue(*(Tag + FString::Printf(TEXT("nose on the bearing (%.1f s)"), Frames / 60.f)), bOn);
		TestTrue(*(Tag + TEXT("through the player's aim input (virtual stick), not a rotation set")), bUsedTheStick);
		const float Err = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Ship->GetActorForwardVector(), Target.Vector()), -1.f, 1.f)));
		TestTrue(*(Tag + FString::Printf(TEXT("within 3 deg (%.1f)"), Err)), Err < 3.f);
		W.Advance(1.f);
		const float Drift = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Ship->GetActorForwardVector(), Target.Vector()), -1.f, 1.f)));
		TestTrue(*(Tag + FString::Printf(TEXT("stays on it after letting go (%.1f deg)"), Drift)), Drift < 8.f);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// Triad Drift - the menu -> New Game -> intro -> first waves -> Continue chain, end to end in a real PIE session.
// Owner: Teammate 3. Editor builds only (UnrealEd for PIE); a packaged game never sees it.
//
// Real maps (L_MainMenu, the transition, L_Galaxy), the real menu game mode's NewGame(), real travel, the real
// StartPlay -> intro hook, a real held key to skip, the real wave director, a real save slot and a real Continue.
// Bounded (120 s hard limit), and it always ends the PIE session and deletes the save slot it wrote, pass or fail.
// If PIE cannot start in this runner it skips with a clear log line instead of failing.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Waves/TDWaveDirector.h"

namespace TDIntroE2E
{
	static constexpr double HardLimitSeconds = 120.0;
	static const TCHAR* SlotName = TEXT("TD_E2E_IntroWaves");

	struct FState
	{
		FAutomationTestBase* Test = nullptr;
		double Start = 0.0;
		bool bFailed = false;
		bool bSkipped = false;
		FDelegateHandle InitHandle;
		TWeakObjectPtr<UWorld> FirstGalaxy;
		double PressedAt = -1.0;
		double SettleFrom = -1.0;

		bool Done() const { return bFailed || bSkipped; }
		double Elapsed() const { return FPlatformTime::Seconds() - Start; }
	};

	static UWorld* PIEWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& C : GEngine->GetWorldContexts())
		{
			if (C.WorldType == EWorldType::PIE && C.World())
			{
				return C.World();
			}
		}
		return nullptr;
	}

	static bool IsGalaxy(const UWorld* W)
	{
		return W && W->GetMapName().Contains(TEXT("L_Galaxy")) && W->HasBegunPlay() && W->GetAuthGameMode();
	}

	static UTDGameInstance* GI(UWorld* W)
	{
		return W ? Cast<UTDGameInstance>(W->GetGameInstance()) : nullptr;
	}

	/** One step: Fn runs every frame until it returns true. A timeout fails the test (or skips it, if OnTimeout says so). */
	class FStep : public IAutomationLatentCommand
	{
	public:
		FStep(TSharedRef<FState> InS, const TCHAR* InName, double InTimeout, TFunction<bool(FState&)> InFn, TFunction<bool(FState&)> InOnTimeoutSkip = nullptr)
			: S(InS), Name(InName), Timeout(InTimeout), Fn(MoveTemp(InFn)), OnTimeoutSkip(MoveTemp(InOnTimeoutSkip)) {}

		virtual bool Update() override
		{
			if (S->Done())
			{
				return true;
			}
			if (StepStart < 0.0)
			{
				StepStart = FPlatformTime::Seconds();
			}
			if (Fn(*S))
			{
				return true;
			}
			if (FPlatformTime::Seconds() - StepStart > Timeout || S->Elapsed() > HardLimitSeconds)
			{
				if (OnTimeoutSkip && OnTimeoutSkip(*S))
				{
					S->bSkipped = true;
					return true;
				}
				S->Test->AddError(FString::Printf(TEXT("E2E step '%s' timed out after %.0f s (test at %.0f s)"), *Name, FPlatformTime::Seconds() - StepStart, S->Elapsed()));
				S->bFailed = true;
				return true;
			}
			return false;
		}

	private:
		TSharedRef<FState> S;
		FString Name;
		double Timeout;
		double StepStart = -1.0;
		TFunction<bool(FState&)> Fn;
		TFunction<bool(FState&)> OnTimeoutSkip;
	};

	/** Always last: end PIE (bounded), delete the slot, unhook. */
	class FCleanup : public IAutomationLatentCommand
	{
	public:
		explicit FCleanup(TSharedRef<FState> InS) : S(InS) {}

		virtual bool Update() override
		{
			if (CleanupStart < 0.0)
			{
				CleanupStart = FPlatformTime::Seconds();
				if (GEditor && GEditor->PlayWorld)
				{
					GEditor->RequestEndPlayMap();
				}
			}
			if (GEditor && GEditor->PlayWorld && FPlatformTime::Seconds() - CleanupStart < 15.0)
			{
				return false; // PIE shutting down
			}
			UGameplayStatics::DeleteGameInSlot(SlotName, UTDFlowSettings::Get()->SaveUserIndex);
			FWorldDelegates::OnPostWorldInitialization.Remove(S->InitHandle);
			if (GEditor && GEditor->PlayWorld)
			{
				S->Test->AddError(TEXT("E2E cleanup: the PIE session did not end within 15 s"));
			}
			UE_LOG(LogTriadDrift, Display, TEXT("E2E NewGameIntroWavesContinue: %s in %.0f s"),
				S->bSkipped ? TEXT("SKIPPED (PIE could not start in this runner)") : (S->bFailed ? TEXT("FAILED") : TEXT("passed")), S->Elapsed());
			return true;
		}

	private:
		TSharedRef<FState> S;
		double CleanupStart = -1.0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroE2E, "TriadDrift.Flow.E2E.NewGameIntroWavesContinue", TD_TEST_FLAGS)
bool FTDIntroE2E::RunTest(const FString& Parameters)
{
	using namespace TDIntroE2E;
	TSharedRef<FState> S = MakeShared<FState>();
	S->Test = this;
	S->Start = FPlatformTime::Seconds();

	// Headless PIE has no game viewport: let the intro play in PIE worlds anyway (it is what is under test).
	S->InitHandle = FWorldDelegates::OnPostWorldInitialization.AddLambda([](UWorld* W, const UWorld::InitializationValues)
	{
		if (W && W->WorldType == EWorldType::PIE)
		{
			if (UTDIntroDirector* Intro = W->GetSubsystem<UTDIntroDirector>())
			{
				Intro->SetAllowWithoutViewportForTests(true);
			}
		}
	});

	if (!AutomationOpenMap(TEXT("/Game/TriadDrift/Maps/L_MainMenu")))
	{
		UE_LOG(LogTriadDrift, Display, TEXT("E2E NewGameIntroWavesContinue: SKIPPED (could not open L_MainMenu in this runner)"));
		FWorldDelegates::OnPostWorldInitialization.Remove(S->InitHandle);
		return true;
	}
	// In the editor AutomationOpenMap opens the map and starts PIE on it (a second FStartPIECommand would restart it).

	// 1. The real main menu in PIE.
	ADD_LATENT_AUTOMATION_COMMAND(FStep(S, TEXT("PIE main menu"), 45.0, [](FState& St)
	{
		UWorld* W = PIEWorld();
		ATDMenuGameMode* Menu = W ? W->GetAuthGameMode<ATDMenuGameMode>() : nullptr;
		UTDGameInstance* G = GI(W);
		if (!Menu || !G || !W->HasBegunPlay())
		{
			return false;
		}
		G->SetSaveSlotOverride(SlotName);
		G->DeleteSave();
		return true;
	}, [](FState&) { return PIEWorld() == nullptr; })); // no PIE world at all: this runner cannot do PIE

	// 2. New Game, the way the menu button does it.
	ADD_LATENT_AUTOMATION_COMMAND(FStep(S, TEXT("New Game"), 5.0, [](FState& St)
	{
		UWorld* W = PIEWorld();
		ATDMenuGameMode* Menu = W ? W->GetAuthGameMode<ATDMenuGameMode>() : nullptr;
		St.Test->TestTrue(TEXT("New Game starts"), Menu && Menu->NewGame());
		St.Test->TestTrue(TEXT("New Game: the intro is pending"), GI(W) && GI(W)->IsIntroPending());
		return true;
	}));

	// 3. Real travel to the galaxy: the intro plays, nothing else has started.
	ADD_LATENT_AUTOMATION_COMMAND(FStep(S, TEXT("galaxy and intro"), 60.0, [](FState& St)
	{
		UWorld* W = PIEWorld();
		static TWeakObjectPtr<UWorld> LastLogged;
		if (W && LastLogged.Get() != W)
		{
			LastLogged = W;
			UE_LOG(LogTriadDrift, Display, TEXT("E2E: PIE world now %s (begun %d)"), *W->GetMapName(), W->HasBegunPlay());
		}
		UTDIntroDirector* Intro = IsGalaxy(W) ? UTDIntroDirector::Get(W) : nullptr;
		if (!Intro || !Intro->IsPlaying())
		{
			return false;
		}
		St.FirstGalaxy = W;
		UTDGameInstance* G = GI(W);
		const ATDWaveDirector* Waves = ATDWaveDirector::Get(W);
		const UTDUIHostSubsystem* UI = UTDUIHostSubsystem::Get(W);
		const ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W->GetFirstPlayerController());
		St.Test->TestFalse(TEXT("the intro is consumed"), G && G->IsIntroPending());
		St.Test->TestTrue(TEXT("phase stays Intro while it plays"), G && G->GetProgressRef().Phase == ETDGamePhase::Intro);
		St.Test->TestTrue(TEXT("no wave yet"), !Waves || Waves->GetCurrentWaveIndex() == INDEX_NONE);
		St.Test->TestTrue(TEXT("every HUD hidden"), UI && UI->IsCinematicHidden());
		St.Test->TestTrue(TEXT("no pause during the intro"), PC && !PC->CanPauseNow());
		return true;
	}));

	// 4. Past the first-second lock, hold a real key for 0.6 s.
	ADD_LATENT_AUTOMATION_COMMAND(FStep(S, TEXT("hold to skip"), 6.0, [](FState& St)
	{
		UWorld* W = St.FirstGalaxy.Get();
		UTDIntroDirector* Intro = UTDIntroDirector::Get(W);
		APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
		if (!Intro || !PC)
		{
			return false;
		}
		if (St.PressedAt < 0.0)
		{
			if (Intro->GetElapsed() < UTDIntroDirector::SkipLockSeconds + 0.3f)
			{
				return false;
			}
			FTDTestWorld::InjectKey(PC, EKeys::SpaceBar, true);
			St.PressedAt = FPlatformTime::Seconds();
			return false;
		}
		if (FPlatformTime::Seconds() - St.PressedAt < 0.8)
		{
			return false;
		}
		FTDTestWorld::InjectKey(PC, EKeys::SpaceBar, false);
		return true;
	}));

	// 5. Skipped: phase, HUD and pause are back.
	ADD_LATENT_AUTOMATION_COMMAND(FStep(S, TEXT("skipped"), 5.0, [](FState& St)
	{
		UWorld* W = St.FirstGalaxy.Get();
		UTDIntroDirector* Intro = UTDIntroDirector::Get(W);
		if (!Intro || Intro->IsPlaying())
		{
			return false;
		}
		UTDGameInstance* G = GI(W);
		const UTDUIHostSubsystem* UI = UTDUIHostSubsystem::Get(W);
		const ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W->GetFirstPlayerController());
		St.Test->TestTrue(TEXT("it was a skip"), Intro->WasSkipped());
		St.Test->TestTrue(TEXT("Waves1to2 after the intro"), G && G->GetProgressRef().Phase == ETDGamePhase::Waves1to2);
		St.Test->TestTrue(TEXT("the HUDs come back"), UI && !UI->IsCinematicHidden());
		St.Test->TestTrue(TEXT("pause allowed again"), PC && PC->CanPauseNow());
		St.Test->TestTrue(TEXT("the view is the player's ship"), PC && PC->GetViewTarget() == PC->GetPawn());
		return true;
	}));

	// 6. The real wave director starts the first wave.
	ADD_LATENT_AUTOMATION_COMMAND(FStep(S, TEXT("first wave"), 25.0, [](FState& St)
	{
		const ATDWaveDirector* Waves = ATDWaveDirector::Get(St.FirstGalaxy.Get());
		return Waves && Waves->GetCurrentWaveIndex() != INDEX_NONE;
	}));

	// 7. A checkpoint in the real slot, then Continue (real travel).
	ADD_LATENT_AUTOMATION_COMMAND(FStep(S, TEXT("save and Continue"), 10.0, [](FState& St)
	{
		UTDGameInstance* G = GI(St.FirstGalaxy.Get());
		if (!G)
		{
			return false;
		}
		if (!G->HasContinue())
		{
			G->SaveCheckpoint();
			return false;
		}
		St.Test->TestTrue(TEXT("Continue starts"), G->ContinueRun());
		St.Test->TestFalse(TEXT("Continue carries no intro"), G->IsIntroPending());
		return true;
	}));

	// 8. The continued galaxy: a new world, no intro, waves armed.
	ADD_LATENT_AUTOMATION_COMMAND(FStep(S, TEXT("continued galaxy"), 60.0, [](FState& St)
	{
		UWorld* W = PIEWorld();
		if (!IsGalaxy(W) || W == St.FirstGalaxy.Get())
		{
			return false;
		}
		if (St.SettleFrom < 0.0)
		{
			St.SettleFrom = FPlatformTime::Seconds();
			return false;
		}
		if (FPlatformTime::Seconds() - St.SettleFrom < 1.5)
		{
			return false;
		}
		const UTDIntroDirector* Intro = UTDIntroDirector::Get(W);
		St.Test->TestTrue(TEXT("Continue never replays the intro"), Intro && !Intro->IsPlaying() && Intro->GetElapsed() == 0.f);
		const UTDGameInstance* G = GI(W);
		St.Test->TestTrue(TEXT("the run is past the intro"), G && G->GetProgressRef().Phase != ETDGamePhase::Intro);
		const UTDUIHostSubsystem* UI = UTDUIHostSubsystem::Get(W);
		St.Test->TestTrue(TEXT("the HUD is up"), UI && !UI->IsCinematicHidden());
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FCleanup(S));
	return true;
}

#endif

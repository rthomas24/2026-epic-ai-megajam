// Triad Drift - tests for the ENG-33 cinematic intro. Owner: Teammate 3.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Rendering/SlateRenderer.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Director/Intro/Tests/TDIntroTestListener.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "LevelSequence.h"
#include "MovieScene.h"

namespace TDIntroTest
{
	struct FIntroWorld
	{
		FTDTestWorld W;
		ATDSquadManager* Squad = nullptr;
		APlayerController* PC = nullptr;
		UTDIntroDirector* Intro = nullptr;
		int32 Finished = 0;
		struct FShipState { FTransform T; FVector V; bool bCollision; bool bMoveTick; bool bAITick; };
		TArray<FShipState> Before;
		/** The squad the moment OnFinished fires (when T5's flow starts the waves). */
		TArray<FShipState> AtFinish;

		bool Init()
		{
			Squad = W.Spawn<ATDSquadManager>();
			PC = W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass());
			if (!Squad || !PC)
			{
				return false;
			}
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(-25000.f, 0.f, 0.f)));
			W.Advance(0.5f);
			Intro = UTDIntroDirector::Get(W.GetWorld());
			Before = Snapshot();
			return Intro && Cast<ATDShip>(PC->GetPawn()) != nullptr;
		}

		TArray<FShipState> Snapshot() const
		{
			TArray<FShipState> Out;
			for (ETDShipId Id : UTDShipLibrary::AllShips())
			{
				const ATDShip* S = Squad->GetShip(Id);
				const AController* C = S ? S->GetController() : nullptr;
				Out.Add({ S->GetActorTransform(), S->GetShipMovement()->Velocity, S->GetActorEnableCollision(),
					S->GetShipMovement()->IsComponentTickEnabled(), C ? C->IsActorTickEnabled() : true });
			}
			return Out;
		}

		bool Play()
		{
			return Intro->PlayForTests(FSimpleDelegate::CreateLambda([this]() { ++Finished; AtFinish = Snapshot(); }));
		}

		void Run(float Seconds, float Step = 1.f / 30.f)
		{
			for (float T = 0.f; T < Seconds; T += Step)
			{
				W.Tick(Step);
			}
		}

		void Hold(const FKey& Key, float Seconds)
		{
			FTDTestWorld::InjectKey(PC, Key, true);
			Run(Seconds);
			FTDTestWorld::InjectKey(PC, Key, false);
			W.Tick(1.f / 30.f);
		}

		/** The squad exactly as it was before the intro (the run is identical watched or skipped). */
		void CheckRestored(FAutomationTestBase& T, const TCHAR* When) const
		{
			const TArray<FShipState>& Now = AtFinish;
			T.TestEqual(FString(When) + TEXT(": a snapshot at the finish"), Now.Num(), Before.Num());
			for (int32 i = 0; i < Before.Num() && i < Now.Num(); ++i)
			{
				const FString Ship = FString::Printf(TEXT("%s, ship %d: "), When, i);
				T.TestTrue(Ship + TEXT("same place"), Now[i].T.GetLocation().Equals(Before[i].T.GetLocation(), 1.f)
					&& Now[i].T.GetRotation().Equals(Before[i].T.GetRotation(), 0.001f));
				T.TestTrue(Ship + TEXT("same velocity"), Now[i].V.Equals(Before[i].V, 1.f));
				T.TestEqual(Ship + TEXT("collision back"), Now[i].bCollision, Before[i].bCollision);
				T.TestEqual(Ship + TEXT("flight back"), Now[i].bMoveTick, Before[i].bMoveTick);
				T.TestEqual(Ship + TEXT("AI back"), Now[i].bAITick, Before[i].bAITick);
			}
		}
	};

	struct FScopedIntroCVar
	{
		explicit FScopedIntroCVar(int32 V) { Set(V); }
		~FScopedIntroCVar() { Set(1); }
		static void Set(int32 V)
		{
			if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Intro")))
			{
				C->Set(V, ECVF_SetByCode);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroShotList, "TriadDrift.Director.Intro.ShotListMatchesTheBrief", TD_TEST_FLAGS)
bool FTDIntroShotList::RunTest(const FString& Parameters)
{
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()) || !TestTrue(TEXT("plays"), F.Play()))
	{
		return false;
	}
	const TArray<FTDIntroShot>& Shots = F.Intro->GetShots();
	TestTrue(FString::Printf(TEXT("about 45-55 s (%.1f)"), F.Intro->GetTotalDuration()), F.Intro->GetTotalDuration() >= 45.f && F.Intro->GetTotalDuration() <= 55.f);
	TestEqual(TEXT("seven beats (the threat in two cuts)"), Shots.Num(), 8);
	auto Find = [&](const TCHAR* Id) { return Shots.FindByPredicate([Id](const FTDIntroShot& S) { return S.Id == FName(Id); }); };
	const FTDIntroShot* Nebula = Find(TEXT("Nebula"));
	const FTDIntroShot* Eclipse = Find(TEXT("Eclipse"));
	const FTDIntroShot* Squad = Find(TEXT("Squad"));
	const FTDIntroShot* Belt = Find(TEXT("Belt"));
	const FTDIntroShot* Gate = Find(TEXT("Gate"));
	const FTDIntroShot* Far = Find(TEXT("ThreatFar"));
	const FTDIntroShot* Close = Find(TEXT("ThreatClose"));
	if (!Nebula || !Eclipse || !Squad || !Belt || !Gate || !Far || !Close)
	{
		AddError(TEXT("a beat is missing"));
		return false;
	}
	TestTrue(TEXT("1: 35 mm, from black"), Nebula->FocalFrom == 35.f && Nebula->FadeIn >= 2.f);
	TestTrue(TEXT("2: long lens 85-135"), Eclipse->FocalFrom >= 85.f && Eclipse->FocalTo <= 135.f && Eclipse->FocalTo > Eclipse->FocalFrom);
	TestTrue(TEXT("3: 35 mm, shallow, steady (issue #49: no handheld, no swings)"), Squad->FocalFrom == 35.f && Squad->FStop <= 2.8f && Squad->Handheld == 0.f && Squad->Duration == 8.f);
	TestTrue(TEXT("4: 50 mm"), Belt->FocalFrom == 50.f);
	TestTrue(TEXT("5: 35 mm"), Gate->FocalFrom == 35.f);
	TestTrue(TEXT("6: 135 mm far, then 24 mm close"), Far->FocalFrom == 135.f && Close->FocalFrom == 24.f);
	TestTrue(TEXT("6: 10 s"), FMath::IsNearlyEqual(Far->Duration + Close->Duration, 10.f));
	TestTrue(TEXT("narration: 4 to 6 lines"), UTDIntroDirector::NarrationLines().Num() >= 4 && UTDIntroDirector::NarrationLines().Num() <= 6);

	// The lens really is that lens (Super 35 width): 24 mm is about 54.8 deg, 135 mm about 10.5 deg.
	F.Intro->DebugJumpToShot(2);
	F.W.Tick(1.f / 30.f);
	const UCameraComponent* Cam = F.Intro->GetShotCamera()->GetCameraComponent();
	TestNearlyEqual(TEXT("35 mm field of view"), Cam->FieldOfView, 39.1f, 0.5f);
	TestTrue(TEXT("depth of field set"), Cam->PostProcessSettings.bOverride_DepthOfFieldFstop && Cam->PostProcessSettings.DepthOfFieldFstop <= 2.8f);
	TestTrue(TEXT("motion blur, bloom, flare, grain"), Cam->PostProcessSettings.MotionBlurAmount > 0.f && Cam->PostProcessSettings.BloomIntensity > 0.f
		&& Cam->PostProcessSettings.LensFlareIntensity > 0.f && Cam->PostProcessSettings.FilmGrainIntensity > 0.f);
	TestEqual(TEXT("2.39 bars"), F.Intro->GetOverlay().Bars, 1.f);

	// Issue #49: one steady frame per ship, a card each; inside a frame the lens barely turns (no whip-pans), and the
	// only big changes of view are the two cuts.
	TSet<FString> Cards;
	float Closest = 1.0e9f;
	float MaxTurnInFrame = 0.f;
	int32 Cuts = 0;
	FVector LastFwd = F.Intro->GetShotCamera()->GetActorForwardVector();
	int32 LastSeg = UTDIntroDirector::SquadSegment(F.Intro->GetElapsed() - 12.f, 8.f);
	for (float T = 0.f; T < 7.9f; T += 1.f / 30.f)
	{
		F.W.Tick(1.f / 30.f);
		const FVector Fwd = F.Intro->GetShotCamera()->GetActorForwardVector();
		const float Turn = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Fwd, LastFwd), -1.f, 1.f)));
		const int32 Seg = UTDIntroDirector::SquadSegment(F.Intro->GetElapsed() - 12.f, 8.f);
		if (Seg != LastSeg) { ++Cuts; }
		else { MaxTurnInFrame = FMath::Max(MaxTurnInFrame, Turn); }
		LastSeg = Seg;
		LastFwd = Fwd;
		if (F.Intro->GetOverlay().NameCardAlpha > 0.5f)
		{
			Cards.Add(F.Intro->GetOverlay().NameCard.ToString());
		}
		for (ETDShipId Id : UTDShipLibrary::AllShips())
		{
			Closest = FMath::Min(Closest, FVector::Dist(F.Squad->GetShip(Id)->GetActorLocation(), F.Intro->GetShotCamera()->GetActorLocation()));
		}
	}
	TestEqual(TEXT("three pilot name cards"), Cards.Num(), 3);
	TestTrue(FString::Printf(TEXT("close and large, never through the lens (%.0f cm)"), Closest), Closest > 400.f && Closest < 9000.f);
	TestTrue(FString::Printf(TEXT("steady: at most %.2f deg per frame inside a shot"), MaxTurnInFrame), MaxTurnInFrame < 1.0f);
	TestEqual(TEXT("two cuts: three shots"), Cuts, 2);

	// The title at the end.
	F.Intro->DebugJumpToShot(Shots.Num() - 1);
	F.Run(1.2f);
	TestTrue(TEXT("TRIAD DRIFT on black"), F.Intro->GetOverlay().TitleAlpha > 0.5f && F.Intro->GetOverlay().Black > 0.9f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroWatched, "TriadDrift.Director.Intro.PlaysOnceAndHandsOffToTheChaseCamera", TD_TEST_FLAGS)
bool FTDIntroWatched::RunTest(const FString& Parameters)
{
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()) || !TestTrue(TEXT("plays"), F.Play()))
	{
		return false;
	}
	TestTrue(TEXT("the shot camera has the view"), F.PC->GetViewTarget() == F.Intro->GetShotCamera());
	TestFalse(TEXT("a second Play while playing is refused"), F.Intro->PlayForTests(FSimpleDelegate()));
	TestTrue(TEXT("move and look input ignored"), F.PC->IsMoveInputIgnored() && F.PC->IsLookInputIgnored());
	const UTDUIHostSubsystem* UIHost = UTDUIHostSubsystem::Get(F.W.GetWorld());
	TestTrue(TEXT("every HUD hidden (T2)"), UIHost && UIHost->IsCinematicHidden());
	const ATDSpacePlayerController* SpacePC = Cast<ATDSpacePlayerController>(F.PC);
	TestTrue(TEXT("no pause while it plays (T1)"), SpacePC && !SpacePC->CanPauseNow());
	F.Run(10.f);
	TestTrue(TEXT("the squad is frozen: no AI, no flight, no collision"), !F.Squad->GetShip(ETDShipId::Lancer)->GetActorEnableCollision()
		&& !F.Squad->GetShip(ETDShipId::Lancer)->GetShipMovement()->IsComponentTickEnabled());
	TestEqual(TEXT("not finished early"), F.Finished, 0);

	const float Total = F.Intro->GetTotalDuration();
	F.Run(Total - 10.f + 0.2f);
	TestTrue(TEXT("handing off: the view blends to the ship"), F.Intro->IsPlaying() && F.Finished == 0);
	F.Run(UTDIntroDirector::HandOffSeconds + 0.2f);
	TestEqual(TEXT("finished exactly once, after the blend"), F.Finished, 1);
	TestFalse(TEXT("over"), F.Intro->IsPlaying());
	TestFalse(TEXT("not a skip"), F.Intro->WasSkipped());
	TestTrue(TEXT("the chase camera: the player's ship"), F.PC->GetViewTarget() == F.PC->GetPawn() && Cast<ATDShip>(F.PC->GetPawn()));
	TestFalse(TEXT("input given back"), F.PC->IsMoveInputIgnored() || F.PC->IsLookInputIgnored());
	TestFalse(TEXT("the HUDs come back"), UIHost && UIHost->IsCinematicHidden());
	TestTrue(TEXT("pause allowed again"), SpacePC && SpacePC->CanPauseNow());
	F.CheckRestored(*this, TEXT("watched"));
	F.Run(2.f);
	TestEqual(TEXT("never fires again"), F.Finished, 1);
	return true;
}

/** N-039: the control frame for the music director (watched, skipped, or no intro) and the intro's clock. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroControlFrame, "TriadDrift.Director.Intro.ControlFrameEventAndClockForMusic", TD_TEST_FLAGS)
bool FTDIntroControlFrame::RunTest(const FString& Parameters)
{
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()))
	{
		return false;
	}
	UTDIntroTestListener* L = NewObject<UTDIntroTestListener>();
	L->AddToRoot();
	L->Intro = F.Intro;
	F.Intro->OnIntroFinished.AddDynamic(L, &UTDIntroTestListener::OnFinished);

	// No intro: the event still marks the control moment.
	F.Intro->NotifyRunStartedWithoutIntro();
	TestTrue(TEXT("without the intro: fired, not played"), L->Count == 1 && !L->bLastPlayed && !L->bLastSkipped);

	// Watched: the clock runs through the hand-off and the event lands on its end.
	if (!TestTrue(TEXT("plays"), F.Play()))
	{
		L->RemoveFromRoot();
		return false;
	}
	TestFalse(TEXT("no engine debug text over the film (issue #49)"), GEngine && GEngine->bEnableOnScreenDebugMessagesDisplay);
	const float Length = F.Intro->GetIntroLength();
	TestEqual(TEXT("length to control = shots + hand-off"), Length, F.Intro->GetTotalDuration() + UTDIntroDirector::HandOffSeconds, 0.001f);
	TestEqual(TEXT("49 s today"), Length, 49.f, 0.001f);
	F.Run(20.f);
	TestEqual(TEXT("the clock follows"), F.Intro->GetIntroTime(), 20.f, 0.25f);
	F.Intro->NotifyRunStartedWithoutIntro();
	TestEqual(TEXT("no 'without intro' event while it plays"), L->Count, 1);
	F.Run(F.Intro->GetTotalDuration() - 20.f + 0.5f);
	TestTrue(TEXT("in the hand-off the clock counts on past the shots"), F.Intro->IsHandingOff() && F.Intro->GetIntroTime() > F.Intro->GetTotalDuration());
	TestEqual(TEXT("not yet"), L->Count, 1);
	F.Run(UTDIntroDirector::HandOffSeconds);
	TestTrue(TEXT("watched: fired once at the control frame"), L->Count == 2 && L->bLastPlayed && !L->bLastSkipped);
	TestFalse(TEXT("fired after the intro let go"), L->bStillPlayingWhenFired);
	TestEqual(TEXT("clock 0 when not playing"), F.Intro->GetIntroTime(), 0.f);
	TestTrue(TEXT("debug text restored after the intro"), GEngine && GEngine->bEnableOnScreenDebugMessagesDisplay);

	// Skipped: fired at once.
	if (TestTrue(TEXT("plays again"), F.Play()))
	{
		F.Run(1.5f);
		TestTrue(TEXT("skip"), F.Intro->Skip());
		TestTrue(TEXT("skipped: fired"), L->Count == 3 && L->bLastPlayed && L->bLastSkipped);
	}
	L->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroWallClock, "TriadDrift.Director.Intro.RunsOnWallTimeThroughHitches", TD_TEST_FLAGS)
bool FTDIntroWallClock::RunTest(const FString& Parameters)
{
	// The music runs on the audio (wall) clock: intro time must equal summed real frame time, hitches included, or the
	// downbeat lands before control (#24). A 2 s load hitch skips frames; it never stretches the intro or skips its end.
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()))
	{
		return false;
	}
	UTDIntroTestListener* L = NewObject<UTDIntroTestListener>();
	L->AddToRoot();
	L->Intro = F.Intro;
	F.Intro->OnIntroFinished.AddDynamic(L, &UTDIntroTestListener::OnFinished);
	if (!TestTrue(TEXT("plays"), F.Play()))
	{
		L->RemoveFromRoot();
		return false;
	}
	const float Length = F.Intro->GetIntroLength();
	const float Frames[] = { 1.f / 60.f, 1.f / 30.f, 0.25f, 2.f, 1.f / 144.f, 0.7f, 0.05f, 1.f / 30.f, 3.5f, 0.12f };
	float Sum = 0.f;
	float MaxErr = 0.f;
	int32 LastShot = -1;
	bool bInOrder = true;
	int32 i = 0;
	while (F.Intro->IsPlaying() && Sum < Length + 5.f)
	{
		const float Dt = Frames[i++ % UE_ARRAY_COUNT(Frames)];
		F.W.Tick(Dt);
		Sum += Dt;
		if (F.Intro->IsPlaying())
		{
			MaxErr = FMath::Max(MaxErr, FMath::Abs(F.Intro->GetIntroTime() - Sum));
			const int32 Shot = F.Intro->GetShotIndex();
			bInOrder = bInOrder && Shot >= LastShot;
			LastShot = Shot;
		}
	}
	TestTrue(FString::Printf(TEXT("intro time tracks summed real time (max error %.4f s)"), MaxErr), MaxErr <= 0.001f);
	TestTrue(TEXT("shots only move forward"), bInOrder);
	TestEqual(TEXT("OnIntroFinished fired exactly once"), L->Count, 1);
	TestTrue(TEXT("watched, not skipped"), L->bLastPlayed && !L->bLastSkipped);
	TestEqual(TEXT("the run finish delegate once"), F.Finished, 1);
	TestTrue(FString::Printf(TEXT("finished on the first frame past the length (%.2f of %.2f s)"), Sum, Length), Sum >= Length && Sum < Length + 3.5f + 0.001f);
	F.Run(1.f);
	TestEqual(TEXT("still exactly once"), L->Count, 1);

	// One frame longer than the whole intro: the end handler still runs, once.
	if (TestTrue(TEXT("plays again"), F.Play()))
	{
		F.W.Tick(Length + 2.f);
		TestEqual(TEXT("a single huge frame finishes it once"), L->Count, 2);
		TestFalse(TEXT("and it let go"), F.Intro->IsPlaying());
		F.Run(0.5f);
		TestEqual(TEXT("no second event"), L->Count, 2);
	}
	L->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroCardFits, "TriadDrift.Director.Intro.NameCardsFitEveryScreen", TD_TEST_FLAGS)
bool FTDIntroCardFits::RunTest(const FString& Parameters)
{
	// Issue #49 (Nicholas): the pilot names and trait lines were cut through by the colour bar and, on ultrawide, ran
	// into the narration line. At 720p, 1080p and 21:9 (and a 4:3 window), every card's text sits inside the picture,
	// clear of the subtitle band, the skip prompt and its own bar.
	const bool bSlate = FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer();
	auto Measure = [bSlate](const FText& T, const FSlateFontInfo& F) -> FVector2D
	{
		if (bSlate)
		{
			return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(T, F);
		}
		return FVector2D(T.ToString().Len() * F.Size * 0.62f, F.Size * 1.35f); // a generous estimate without a renderer
	};
	if (!bSlate)
	{
		AddInfo(TEXT("no Slate renderer: text sizes are estimated"));
	}
	const FVector2D Sizes[] = { FVector2D(1280.f, 720.f), FVector2D(1920.f, 1080.f), FVector2D(2560.f, 1080.f), FVector2D(1024.f, 768.f) };
	for (const FVector2D& Size : Sizes)
	{
		for (int32 i = 0; i < 4; ++i)
		{
			const ETDShipId Id = static_cast<ETDShipId>(FMath::Min(i, 2));
			const FTDShipSpec Spec = UTDShipLibrary::GetShipSpec(Id);
			const FText Name = Spec.PilotName;
			// The fourth: a deliberately long trait line (Nicholas may lengthen them through DA_Story_Transmissions).
			const FText Role = FText::FromString((Spec.ShipName.ToString() + TEXT("  ·  ") + Spec.RoleName.ToString()).ToUpper());
			const FText Trait = i < 3 ? UTDStorySubsystem::PilotTrait(Id)
				: FText::FromString(TEXT("INTERCEPTOR  ·  FASTEST IN THE WING  ·  PRECISE  ·  RESTLESS  ·  ALWAYS RECORDING"));
			const FTDIntroCardLayout L = UTDIntroDirector::LayoutNameCard(Size, Name, Role, Trait, Measure);
			const FString At = FString::Printf(TEXT("%.0fx%.0f %s"), Size.X, Size.Y, *Trait.ToString().Left(24));
			const FBox2D Safe(FVector2D(Size.X * 0.03f, L.Picture.Min.Y), FVector2D(Size.X * 0.97f, L.Picture.Max.Y));
			for (const FBox2D* B : { &L.Name, &L.Role, &L.Trait, &L.Insignia, &L.Bar })
			{
				TestTrue(At + TEXT(": inside the picture and the safe frame"), Safe.IsInside(B->Min) && Safe.IsInside(B->Max));
				TestFalse(At + TEXT(": clear of the subtitle band"), B->Intersect(L.SubtitleBand));
				TestTrue(At + TEXT(": a gap of at least 8 px above the subtitle backing"), L.SubtitleBand.Min.Y - B->Max.Y >= 8.f);
				TestFalse(At + TEXT(": clear of HOLD TO SKIP"), B->Intersect(L.Skip));
			}
			TestTrue(At + TEXT(": the accent is beside the text, never across it"), L.Bar.Max.X < L.Name.Min.X && L.Bar.Max.X < L.Role.Min.X && L.Bar.Max.X < L.Trait.Min.X);
			TestTrue(At + TEXT(": the insignia left of the accent"), L.Insignia.Max.X < L.Bar.Min.X);
			TestTrue(At + TEXT(": name, role, trait from the top"), L.Role.Max.Y <= L.Trait.Min.Y + 0.5f);
			TestTrue(At + TEXT(": the name sits above its trait line"), L.Name.Max.Y <= L.Role.Min.Y + 0.5f);
			TestTrue(At + TEXT(": readable sizes"), L.NameFontSize >= 12 && L.RoleFontSize >= 8);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroSkip, "TriadDrift.Director.Intro.HoldToSkipByKeyMouseOrPad", TD_TEST_FLAGS)
bool FTDIntroSkip::RunTest(const FString& Parameters)
{
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()) || !TestTrue(TEXT("plays"), F.Play()))
	{
		return false;
	}
	// In the first second nothing skips, however long it is held.
	F.Hold(EKeys::SpaceBar, 0.8f);
	TestTrue(TEXT("first second: no skip"), F.Intro->IsPlaying());
	F.Run(0.5f);
	TestTrue(TEXT("a press shows HOLD TO SKIP"), F.Intro->GetOverlay().SkipHintAlpha > 0.f);
	// A tap is not a skip.
	F.Hold(EKeys::LeftMouseButton, 0.25f);
	TestTrue(TEXT("a click is not a skip"), F.Intro->IsPlaying());
	F.Hold(EKeys::Gamepad_FaceButton_Bottom, 0.3f);
	TestTrue(TEXT("a short press is not a skip"), F.Intro->IsPlaying() && F.Finished == 0);
	F.Run(3.f);
	// Held for half a second: skipped, from the keyboard...
	F.Hold(EKeys::SpaceBar, 0.6f);
	TestFalse(TEXT("held 0.5 s: skipped"), F.Intro->IsPlaying());
	TestEqual(TEXT("finished once"), F.Finished, 1);
	TestTrue(TEXT("a skip"), F.Intro->WasSkipped());
	TestTrue(TEXT("straight to the chase camera"), F.PC->GetViewTarget() == F.PC->GetPawn());
	F.CheckRestored(*this, TEXT("skipped"));

	// ...and from the mouse and the pad (fresh intros).
	for (const FKey& Key : { EKeys::LeftMouseButton, EKeys::Gamepad_FaceButton_Bottom })
	{
		const int32 Before = F.Finished;
		F.Before = F.Snapshot(); // this intro starts from where the squad is now
		TestTrue(TEXT("plays again (tests)"), F.Play());
		F.Run(1.5f);
		F.Hold(Key, 0.6f);
		TestEqual(FString::Printf(TEXT("%s skips"), *Key.ToString()), F.Finished, Before + 1);
		F.CheckRestored(*this, *Key.ToString());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroOff, "TriadDrift.Director.Intro.NothingPlaysWhenOff", TD_TEST_FLAGS)
bool FTDIntroOff::RunTest(const FString& Parameters)
{
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()))
	{
		return false;
	}
	{
		TDIntroTest::FScopedIntroCVar Off(0);
		TestFalse(TEXT("td.Intro 0: nothing to play"), F.Play());
	}
	TestFalse(TEXT("no viewport and not a test: nothing to play (headless)"), F.Intro->Play(FSimpleDelegate::CreateLambda([&F]() { ++F.Finished; })));
	F.Run(1.f);
	TestEqual(TEXT("OnFinished never fires when nothing played"), F.Finished, 0);
	TestTrue(TEXT("the view stays on the ship"), F.PC->GetViewTarget() == F.PC->GetPawn());
	return true;
}

namespace TDIntroTest
{
	/** An empty Level Sequence of a given length (an empty one never starts: Seconds 0). */
	inline ULevelSequence* MakeStubSequence(float Seconds)
	{
		ULevelSequence* Seq = NewObject<ULevelSequence>(GetTransientPackage(), NAME_None, RF_Transient);
		Seq->Initialize();
		UMovieScene* Scene = Seq->GetMovieScene();
		Scene->SetDisplayRate(FFrameRate(30, 1));
		const FFrameNumber End = (static_cast<double>(Seconds) * Scene->GetTickResolution()).FloorToFrame();
		Scene->SetPlaybackRange(FFrameNumber(0), End.Value);
		return Seq;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroSequence, "TriadDrift.Director.Intro.PresentationSequenceOverride", TD_TEST_FLAGS)
bool FTDIntroSequence::RunTest(const FString& Parameters)
{
	// Review M8: UTDUIStyle::IntroSequence plays instead of the code shots, with the same rules, and no bars of ours.
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()))
	{
		return false;
	}
	ULevelSequence* Seq = TDIntroTest::MakeStubSequence(3.f);
	F.Intro->SetSequenceOverrideForTests(Seq);
	TestTrue(TEXT("plays"), F.Play());
	TestTrue(TEXT("the sequence, not the code shots"), F.Intro->IsPlayingSequence() && !F.Intro->GetShotCamera());
	F.Run(0.5f);
	TestEqual(TEXT("no bars of ours over the sequence"), F.Intro->GetOverlay().Bars, 0.f);
	const UTDUIHostSubsystem* UI = UTDUIHostSubsystem::Get(F.W.GetWorld());
	TestTrue(TEXT("HUD hidden"), UI && UI->IsCinematicHidden());
	// Skip it (same hold rule).
	F.Run(0.8f);
	F.Hold(EKeys::SpaceBar, 0.6f);
	TestEqual(TEXT("skipped: finished once"), F.Finished, 1);
	TestTrue(TEXT("HUD restored"), UI && !UI->IsCinematicHidden());
	TestTrue(TEXT("view on the ship"), F.PC->GetViewTarget() == F.PC->GetPawn());
	F.CheckRestored(*this, TEXT("sequence skipped"));

	// Watched to its end: the hand-off follows the last frame, still without bars.
	F.Before = F.Snapshot();
	TestTrue(TEXT("plays again"), F.Play());
	bool bBarsSeen = false;
	for (float T = 0.f; T < 3.f + UTDIntroDirector::HandOffSeconds + 0.6f; T += 1.f / 30.f)
	{
		F.W.Tick(1.f / 30.f);
		bBarsSeen = bBarsSeen || (F.Intro->IsPlaying() && F.Intro->GetOverlay().Bars > 0.f);
	}
	TestEqual(TEXT("finished after its end and the hand-off"), F.Finished, 2);
	TestFalse(TEXT("never our bars over Nick's sequence"), bBarsSeen);
	F.CheckRestored(*this, TEXT("sequence watched"));

	// A sequence that never starts (empty): a warning, then the code shots play instead.
	AddExpectedMessage(TEXT("presentation sequence did not start"), EAutomationExpectedMessageFlags::Contains, 1);
	F.Intro->SetSequenceOverrideForTests(TDIntroTest::MakeStubSequence(0.f));
	TestTrue(TEXT("plays (tries the sequence)"), F.Play());
	F.Run(1.f);
	TestTrue(TEXT("falls back to the code shots"), F.Intro->IsPlaying() && !F.Intro->IsPlayingSequence() && F.Intro->GetShotCamera() != nullptr);
	TestEqual(TEXT("our bars are back for the code shots"), F.Intro->GetOverlay().Bars, 1.f);
	F.Intro->SetSequenceOverrideForTests(nullptr);
	F.Intro->Skip();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroSkipKeys, "TriadDrift.Director.Intro.SkipCountsHeldButtonsOnly", TD_TEST_FLAGS)
bool FTDIntroSkipKeys::RunTest(const FString& Parameters)
{
	// Review m9: releasing one of two held buttons never cancels the skip; a stick pushed is not a button.
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()) || !TestTrue(TEXT("plays"), F.Play()))
	{
		return false;
	}
	F.Run(1.2f);
	FTDTestWorld::InjectKey(F.PC, EKeys::Gamepad_LeftStick_Up, true);
	F.Run(0.8f);
	FTDTestWorld::InjectKey(F.PC, EKeys::Gamepad_LeftStick_Up, false);
	F.W.Tick(1.f / 30.f);
	TestTrue(TEXT("a stick held over the dead zone is not a skip"), F.Intro->IsPlaying());
	TestFalse(TEXT("nor a skip hint"), F.Intro->GetOverlay().SkipProgress > 0.f);

	FTDTestWorld::InjectKey(F.PC, EKeys::SpaceBar, true);
	FTDTestWorld::InjectKey(F.PC, EKeys::Gamepad_FaceButton_Bottom, true);
	F.Run(0.25f);
	FTDTestWorld::InjectKey(F.PC, EKeys::Gamepad_FaceButton_Bottom, false); // one let go, the other still held
	F.Run(0.4f);
	TestFalse(TEXT("skipped: the other button kept the hold"), F.Intro->IsPlaying());
	FTDTestWorld::InjectKey(F.PC, EKeys::SpaceBar, false);
	F.W.Tick(1.f / 30.f);
	TestEqual(TEXT("finished once"), F.Finished, 1);
	TestTrue(TEXT("sticks and the wheel are not skip keys"), !UTDIntroDirector::IsSkipKey(EKeys::Gamepad_RightStick_Left)
		&& !UTDIntroDirector::IsSkipKey(EKeys::MouseScrollUp) && !UTDIntroDirector::IsSkipKey(EKeys::MouseX)
		&& UTDIntroDirector::IsSkipKey(EKeys::Gamepad_FaceButton_Right) && UTDIntroDirector::IsSkipKey(EKeys::Escape));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDIntroSlate, "TriadDrift.Director.Intro.ThenTheSlate", TD_TEST_FLAGS)
bool FTDIntroSlate::RunTest(const FString& Parameters)
{
	// Review m11: scene one's slate at the hand-off ("then the slate"), not repeated by the real first take; a slate
	// that fires while the intro owns the screen is shown after it, not lost.
	TDIntroTest::FIntroWorld F;
	if (!TestTrue(TEXT("squad and player"), F.Init()) || !TestTrue(TEXT("plays"), F.Play()))
	{
		return false;
	}
	UTDCutPresentation* Cut = UTDDirectorSubsystem::Get(F.W.GetWorld())->GetCutPresentation();
	UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld());
	if (!TestNotNull(TEXT("cut presentation"), Cut) || !TestNotNull(TEXT("bus"), Bus))
	{
		return false;
	}
	// A take starts mid-intro (another system's scene): held, not drawn over the intro.
	F.Run(2.f);
	Bus->BroadcastTakeStarted(TEXT("Obj_Outpost"), 2);
	F.Run(0.5f);
	TestEqual(TEXT("not over the intro"), Cut->GetState().SlateAlpha, 0.f);
	F.Run(0.4f);
	F.Intro->Skip();
	F.Run(0.3f);
	TestTrue(TEXT("shown once the intro is over"), Cut->GetState().SlateAlpha > 0.f && Cut->GetState().SlateText.ToString().Contains(TEXT("TAKE 2")));
	F.Run(3.f);

	// Watched to the end: the slate comes up during the hand-off.
	TestTrue(TEXT("plays again"), F.Play());
	F.Intro->DebugJumpToShot(F.Intro->GetShots().Num() - 1);
	bool bSlateInHandOff = false;
	for (float T = 0.f; T < 8.f && F.Intro->IsPlaying(); T += 1.f / 30.f)
	{
		F.W.Tick(1.f / 30.f);
		bSlateInHandOff = bSlateInHandOff || (F.Intro->IsHandingOff() && Cut->GetState().SlateAlpha > 0.5f);
	}
	TestTrue(TEXT("then the slate: up during the hand-off"), bSlateInHandOff);
	TestTrue(TEXT("it is scene one, take one"), Cut->GetState().SlateText.ToString().Contains(TEXT("SCENE 1")) && Cut->GetState().SlateText.ToString().Contains(TEXT("TAKE 1")));
	// The wave's own first take moments later does not repeat it.
	F.Run(1.6f);
	TestEqual(TEXT("the slate has faded"), Cut->GetState().SlateAlpha, 0.f);
	Bus->BroadcastTakeStarted(UTDGameInstance::WaveSceneId(1), 1);
	F.Run(0.3f);
	TestEqual(TEXT("not repeated by the real first take"), Cut->GetState().SlateAlpha, 0.f);
	return true;
}

#endif

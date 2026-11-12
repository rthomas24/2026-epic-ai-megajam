// Automation tests for the Director's Cut money shots (C-03 #2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Director

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/Tests/TDSquadTestListener.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "Misc/ScopeExit.h"

namespace TDDirectorTests
{
	struct FDirectorFixture
	{
		FTDTestWorld W;
		UTDDirectorSubsystem* Director = nullptr;
		UTDSquadTestListener* Events = nullptr;
		AActor* Player = nullptr;

		FDirectorFixture()
		{
			Events = NewObject<UTDSquadTestListener>();
			Events->AddToRoot();
			Events->Listen(UTDEventBus::Get(W.GetWorld()));
			Director = UTDDirectorSubsystem::Get(W.GetWorld());
			Player = Spawn(ETDTeam::Player, FVector::ZeroVector);
			W.Tick();
		}

		~FDirectorFixture()
		{
			if (Events)
			{
				Events->StopListening();
				Events->RemoveFromRoot();
			}
		}

		AActor* Spawn(ETDTeam Team, const FVector& Location, FName Tag = NAME_None)
		{
			AActor* Actor = W.Spawn<AActor>(Location);
			if (!Actor)
			{
				return nullptr;
			}
			USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
			Actor->SetRootComponent(Root);
			Root->RegisterComponent();
			Actor->SetActorLocation(Location);
			UTDHealthComponent* H = NewObject<UTDHealthComponent>(Actor, TEXT("Health"));
			H->MaxHull = 100.f;
			H->MaxShield = 0.f;
			H->DefaultTeam = Team;
			H->RegisterComponent();
			if (!Tag.IsNone())
			{
				Actor->Tags.Add(Tag);
			}
			return Actor;
		}

		/** Spawn an enemy and kill it through the real health path (UTDEventBus::OnActorKilled). */
		AActor* KillEnemy(ETDDamageKind Kind = ETDDamageKind::Energy, FName Tag = NAME_None, AActor* Killer = nullptr,
			const FVector& Location = FVector(5000.f, 0.f, 0.f))
		{
			AActor* Enemy = Spawn(ETDTeam::Enemy, Location, Tag);
			if (UTDHealthComponent* H = UTDStatics::GetHealth(Enemy))
			{
				H->Kill(Killer ? Killer : Player, Kind);
			}
			return Enemy;
		}

		bool SlowMo() const
		{
			const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
			return Feel && Feel->IsSlowMoActive();
		}
	};
}

using namespace TDDirectorTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorRamKill, "TriadDrift.Director.RamKillIsAMoneyShot", TD_TEST_FLAGS)
bool FTDDirectorRamKill::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		FDirectorFixture F;
		if (!TestNotNull(TEXT("director subsystem exists in game worlds"), F.Director))
		{
			return false;
		}
		F.KillEnemy(ETDDamageKind::Ram);
		TestEqual(TEXT("one OnHighlightMoment"), F.Events->Highlights, 1);
		TestTrue(TEXT("kind = RamKill"), F.Events->LastHighlight == ETDHighlightKind::RamKill);
		TestTrue(TEXT("importance in (0,1]"), F.Events->LastImportance > 0.f && F.Events->LastImportance <= 1.f);
		TestTrue(TEXT("shot active"), F.Director->IsShotActive());
		TestTrue(TEXT("slow-mo requested"), F.SlowMo());
		const FTDDirectorShot& Shot = F.Director->GetCurrentShot();
		TestTrue(TEXT("0.5-1 s of slow-mo"), Shot.DurationSeconds >= 0.5f && Shot.DurationSeconds <= 1.f);
		TestTrue(TEXT("dilation is a slow-down"), Shot.Dilation > 0.f && Shot.Dilation < 1.f);
		TestTrue(TEXT("ram shot frames the rammer"), Shot.Subject.Get() == F.Player);
		F.W.Advance(1.2f, Step);
		TestFalse(TEXT("shot over"), F.Director->IsShotActive());
		TestFalse(TEXT("slow-mo over"), F.SlowMo());
		TestEqual(TEXT("shot counted"), F.Director->GetShotCount(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorMultiKill, "TriadDrift.Director.MultiKillWindow", TD_TEST_FLAGS)
bool FTDDirectorMultiKill::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		{
			FDirectorFixture F;
			F.KillEnemy();
			F.W.Advance(0.5f, Step);
			F.KillEnemy();
			TestEqual(TEXT("two kills are not a multi-kill") + Fps, F.Events->Highlights, 0);
			F.W.Advance(0.5f, Step);
			F.KillEnemy();
			TestEqual(TEXT("three kills in 1 s = multi-kill") + Fps, F.Events->Highlights, 1);
			TestTrue(TEXT("kind = MultiKill") + Fps, F.Events->LastHighlight == ETDHighlightKind::MultiKill);
			TestTrue(TEXT("below the story tier") + Fps, F.Events->LastImportance < 0.8f);
		}
		{
			FDirectorFixture F;
			F.KillEnemy();
			F.W.Advance(0.8f, Step);
			F.KillEnemy();
			F.W.Advance(0.8f, Step);
			F.KillEnemy();
			TestEqual(TEXT("three kills over 1.6 s are not a multi-kill") + Fps, F.Events->Highlights, 0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorMultiKillSustained, "TriadDrift.Director.MultiKillSustainedAndCapped", TD_TEST_FLAGS)
bool FTDDirectorMultiKillSustained::RunTest(const FString& Parameters)
{
	FDirectorFixture F;
	// Six kills in a burst: one multi-kill at three, then a fresh streak (no escalation into story beats).
	for (int32 i = 0; i < 6; ++i)
	{
		F.KillEnemy();
		F.W.Advance(0.2f);
	}
	TestEqual(TEXT("one multi-kill for the burst (the second is rate-limited)"), F.Events->Highlights, 1);
	TestTrue(TEXT("never story-tier"), F.Events->LastImportance < 0.8f);
	// A sustained fight keeps producing multi-kills once the cooldowns allow.
	F.W.Advance(7.f);
	for (int32 i = 0; i < 3; ++i)
	{
		F.KillEnemy();
		F.W.Advance(0.2f);
	}
	TestEqual(TEXT("multi-kills re-trigger in a sustained fight"), F.Events->Highlights, 2);
	// Slow-mo must not stretch the window: 3 kills spread over 1.6 s of REAL time during slow-mo.
	F.W.Advance(7.f);
	UTDFeelSubsystem::Get(F.W.GetWorld())->RequestSlowMo(5.f, 0.2f);
	F.KillEnemy();
	F.W.Advance(0.8f);
	F.KillEnemy();
	F.W.Advance(0.8f);
	F.KillEnemy();
	TestEqual(TEXT("window is real time (slow-mo doesn't stretch it)"), F.Events->Highlights, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorWingmanKills, "TriadDrift.Director.WingmanKillsDontCountForMultiKill", TD_TEST_FLAGS)
bool FTDDirectorWingmanKills::RunTest(const FString& Parameters)
{
	FDirectorFixture F;
	APlayerController* PC = F.W.SpawnPlayerController();
	ATDSquadManager* Squad = F.W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform::Identity);
	F.W.Tick();
	ATDShip* Wingman = Squad->GetShip(ETDShipId::Lancer);
	for (int32 i = 0; i < 4; ++i)
	{
		F.KillEnemy(ETDDamageKind::Kinetic, NAME_None, Wingman);
		F.W.Advance(0.2f);
	}
	TestEqual(TEXT("wingman kills never make a multi-kill"), F.Events->Highlights, 0);
	for (int32 i = 0; i < 3; ++i)
	{
		F.KillEnemy(ETDDamageKind::Kinetic, NAME_None, Squad->GetActiveShip());
		F.W.Advance(0.2f);
	}
	TestTrue(TEXT("the player's own three do"), F.Events->Highlights == 1 && F.Events->LastHighlight == ETDHighlightKind::MultiKill);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorRateLimit, "TriadDrift.Director.RateLimitedButBigMomentsPreempt", TD_TEST_FLAGS)
bool FTDDirectorRateLimit::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FDirectorFixture F;
		F.KillEnemy(ETDDamageKind::Ram); // t = 0, a 0.8 s shot -> global cooldown ends ~3.8 s, ram cooldown 6 s
		TestEqual(TEXT("first ram kill") + Fps, F.Events->Highlights, 1);
		F.W.Advance(0.3f, Step);
		F.KillEnemy(ETDDamageKind::Ram);
		TestEqual(TEXT("second ram kill during the shot is suppressed") + Fps, F.Events->Highlights, 1);
		F.W.Advance(2.f, Step); // t = 2.3: inside the 3 s global cooldown
		TestFalse(TEXT("another kind inside the global cooldown is suppressed") + Fps, F.Director->ReportHighlight(ETDHighlightKind::MissileReturn, F.Player));
		F.W.Advance(2.f, Step); // t = 4.3: global over, ram's 6 s still running
		TestFalse(TEXT("same kind still on its 6 s cooldown") + Fps, F.KillEnemy(ETDDamageKind::Ram) && F.Events->LastHighlight == ETDHighlightKind::RamKill && F.Events->Highlights == 2);
		TestTrue(TEXT("a different kind is allowed after the global cooldown") + Fps, F.Director->ReportHighlight(ETDHighlightKind::MissileReturn, F.Player));
		F.W.Advance(0.1f, Step);
		F.KillEnemy(ETDDamageKind::Energy, TDDirectorTags::Generator);
		TestTrue(TEXT("a story beat takes over any smaller shot") + Fps, F.Events->LastHighlight == ETDHighlightKind::GeneratorKill);
		const int32 Before = F.Events->Highlights;
		F.W.Advance(7.f, Step);
		F.KillEnemy(ETDDamageKind::Ram);
		TestEqual(TEXT("ram allowed again after both cooldowns") + Fps, F.Events->Highlights, Before + 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorObjectiveTags, "TriadDrift.Director.ObjectiveKillsByTag", TD_TEST_FLAGS)
bool FTDDirectorObjectiveTags::RunTest(const FString& Parameters)
{
	FDirectorFixture F;
	AActor* Carrier = F.KillEnemy(ETDDamageKind::Explosive, TDDirectorTags::Carrier);
	TestTrue(TEXT("carrier kill"), F.Events->Highlights == 1 && F.Events->LastHighlight == ETDHighlightKind::CarrierKill);
	TestTrue(TEXT("objective shot frames the victim"), F.Director->GetCurrentShot().Subject.Get() == Carrier);
	F.KillEnemy(ETDDamageKind::Explosive, TDDirectorTags::Reactor);
	TestTrue(TEXT("reactor kill preempts"), F.Events->Highlights == 2 && F.Events->LastHighlight == ETDHighlightKind::ReactorKill);
	TestEqual(TEXT("reactor is the most important beat"), F.Events->LastImportance, 1.f);

	// ENG-18: each of the four generator kills gets its beat, even back to back.
	FDirectorFixture G;
	for (int32 i = 0; i < 4; ++i)
	{
		G.KillEnemy(ETDDamageKind::Energy, TDDirectorTags::Generator);
		G.W.Advance(0.2f);
	}
	TestEqual(TEXT("four generator kills, four highlights"), G.Events->Highlights, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorEnemyKills, "TriadDrift.Director.EnemyKillsIgnored", TD_TEST_FLAGS)
bool FTDDirectorEnemyKills::RunTest(const FString& Parameters)
{
	FDirectorFixture F;
	AActor* EnemyKiller = F.Spawn(ETDTeam::Enemy, FVector(3000.f, 0.f, 0.f));
	AActor* Victim = F.Spawn(ETDTeam::Player, FVector(1000.f, 0.f, 0.f), TDDirectorTags::Reactor);
	if (UTDHealthComponent* H = UTDStatics::GetHealth(Victim))
	{
		H->Kill(EnemyKiller, ETDDamageKind::Ram);
	}
	TestEqual(TEXT("enemy kills never trigger money shots"), F.Events->Highlights, 0);
	TestFalse(TEXT("no slow-mo"), F.SlowMo());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorSceneCut, "TriadDrift.Director.SuppressedDuringRetake", TD_TEST_FLAGS)
bool FTDDirectorSceneCut::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		FDirectorFixture F;
		UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld());
		if (!TestNotNull(TEXT("bus"), Bus))
		{
			return false;
		}
		F.KillEnemy(ETDDamageKind::Ram);
		TestTrue(TEXT("slow-mo running"), F.SlowMo());
		Bus->BroadcastSceneCut(TEXT("Wave1"), 2, 1.5f);
		TestFalse(TEXT("CUT! cancels the running slow-mo"), F.SlowMo());
		TestFalse(TEXT("no highlights while the retake is pending"), F.Director->CanTriggerHighlights());
		F.W.Advance(10.f, Step);
		F.KillEnemy(ETDDamageKind::Explosive, TDDirectorTags::Reactor);
		TestEqual(TEXT("even a reactor kill is ignored"), F.Events->Highlights, 1);
		TestFalse(TEXT("and no slow-mo"), F.SlowMo());
		Bus->BroadcastTakeStarted(TEXT("Wave1"), 2);
		TestTrue(TEXT("new take: highlights allowed"), F.Director->CanTriggerHighlights());
		F.KillEnemy(ETDDamageKind::Explosive, TDDirectorTags::Reactor);
		TestEqual(TEXT("reactor kill in the new take"), F.Events->Highlights, 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCameraHint, "TriadDrift.Director.CameraHint", TD_TEST_FLAGS)
bool FTDDirectorCameraHint::RunTest(const FString& Parameters)
{
	FDirectorFixture F;
	const FVector Where(20000.f, 5000.f, 1000.f);
	AActor* Generator = F.KillEnemy(ETDDamageKind::Energy, TDDirectorTags::Generator, nullptr, Where);
	const FTDDirectorShot First = F.Director->GetCurrentShot();
	TestTrue(TEXT("focus is between victim and killer, near the victim"), FVector::Dist(First.FocusLocation, Where) < FVector::Dist(Where, F.Player->GetActorLocation()) * 0.5f);
	const float CamDist = FVector::Dist(First.SuggestedCameraLocation, First.FocusLocation);
	TestTrue(FString::Printf(TEXT("camera 2500-8000 cm from focus (%.0f)"), CamDist), CamDist > 2500.f && CamDist < 8000.f);
	const FVector Look = First.SuggestedCameraRotation.Vector();
	TestTrue(TEXT("camera looks at the focus"), FVector::DotProduct(Look, (First.FocusLocation - First.SuggestedCameraLocation).GetSafeNormal()) > 0.999f);
	TestTrue(TEXT("subject = generator"), First.Subject.Get() == Generator);

	// The next shot comes from the other side.
	F.KillEnemy(ETDDamageKind::Explosive, TDDirectorTags::Reactor, nullptr, Where);
	const FTDDirectorShot Second = F.Director->GetCurrentShot();
	const FVector SideA = (First.SuggestedCameraLocation - First.FocusLocation).GetSafeNormal();
	const FVector SideB = (Second.SuggestedCameraLocation - Second.FocusLocation).GetSafeNormal();
	TestTrue(TEXT("alternates sides"), FVector::DotProduct(SideA, SideB) < 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorReport, "TriadDrift.Director.ReportHighlightApi", TD_TEST_FLAGS)
bool FTDDirectorReport::RunTest(const FString& Parameters)
{
	FDirectorFixture F;
	TestFalse(TEXT("below minimum importance is ignored"), F.Director->ReportHighlight(ETDHighlightKind::MissileReturn, F.Player, 0.1f));
	TestTrue(TEXT("portal missile return reported by T2"), F.Director->ReportHighlight(ETDHighlightKind::MissileReturn, F.Player));
	TestTrue(TEXT("event fired"), F.Events->Highlights == 1 && F.Events->LastHighlight == ETDHighlightKind::MissileReturn);
	TestEqual(TEXT("default importance for the kind"), F.Events->LastImportance, 0.55f);
	FDirectorFixture Fresh;
	TestFalse(TEXT("no subject is refused even when nothing is rate-limited (no camera at the origin)"), Fresh.Director->ReportHighlight(ETDHighlightKind::ReactorKill, nullptr, 1.f));
	TestEqual(TEXT("and nothing fired"), Fresh.Events->Highlights, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorSwitchStrikeKill, "TriadDrift.Director.SwitchStrikeKill", TD_TEST_FLAGS)
bool FTDDirectorSwitchStrikeKill::RunTest(const FString& Parameters)
{
	FDirectorFixture F;
	APlayerController* PC = F.W.SpawnPlayerController();
	ATDSquadManager* Squad = F.W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform::Identity);
	F.W.Tick();
	Squad->NotifyPowerUsed(ETDShipId::Rift);
	F.W.Advance(0.5f);
	TestTrue(TEXT("switch arms Switch Strike"), Squad->RequestSwitchTo(ETDShipId::Lancer) && Squad->IsSwitchStrikeActive());
	F.KillEnemy(ETDDamageKind::Kinetic, NAME_None, Squad->GetActiveShip());
	TestTrue(TEXT("kill during Switch Strike = SwitchStrikeKill"), F.Events->Highlights == 1 && F.Events->LastHighlight == ETDHighlightKind::SwitchStrikeKill);
	TestTrue(TEXT("frames the player's ship"), F.Director->GetCurrentShot().Subject.Get() == Squad->GetActiveShip());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorTravel, "TriadDrift.Director.SuppressedDuringTravel", TD_TEST_FLAGS)
bool FTDDirectorTravel::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("game instance"), GI) || !TestNotNull(TEXT("director"), Director))
	{
		return false;
	}
	W.Tick();
	TestTrue(TEXT("highlights allowed before travel"), Director->CanTriggerHighlights());

	// Point Verdance at an engine map and stub the level open, as T5's flow tests do; restore after.
	UTDFlowSettings* Settings = GetMutableDefault<UTDFlowSettings>();
	const TMap<ETDDestination, FSoftObjectPath> SavedMaps = Settings->DestinationMaps;
	const FSoftObjectPath SavedTransition = Settings->TransitionMap;
	ON_SCOPE_EXIT
	{
		Settings->DestinationMaps = SavedMaps;
		Settings->TransitionMap = SavedTransition;
	};
	Settings->DestinationMaps.Add(ETDDestination::Verdance, FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry")));
	Settings->TransitionMap = FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry"));
	GI->OpenLevelOverride = [](const FString&) {};

	TestTrue(TEXT("travel starts"), GI->TravelTo(ETDDestination::Verdance, NAME_None));
	if (!GI->IsTravelPending())
	{
		AddError(TEXT("TravelTo did not leave a pending travel in the test world; the travel gate could not be exercised"));
		return false;
	}
	TestFalse(TEXT("no highlights while travel is pending"), Director->CanTriggerHighlights());
	TestFalse(TEXT("ReportHighlight refused during travel"), Director->ReportHighlight(ETDHighlightKind::ReactorKill, nullptr, 1.f));
	const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
	TestFalse(TEXT("and no slow-mo"), Feel && Feel->IsSlowMoActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorDeflectBeat, "TriadDrift.Director.PerfectDeflectIsAShortBeat", TD_TEST_FLAGS)
bool FTDDirectorDeflectBeat::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		FDirectorFixture F;
		TestTrue(TEXT("perfect deflect accepted"), F.Director->ReportHighlight(ETDHighlightKind::PerfectDeflect, F.Player, 0.6f));
		TestEqual(TEXT("ENG-22: 0.15 s beat, not a 0.5-1 s money shot"), F.Director->GetCurrentShot().DurationSeconds, 0.15f, 0.001f);
		TestTrue(TEXT("slow-mo on"), F.SlowMo());
		F.W.Advance(0.25f, Step);
		TestFalse(TEXT("over after 0.15 s"), F.SlowMo() || F.Director->IsShotActive());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

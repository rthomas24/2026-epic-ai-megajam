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
	F.W.Advance(1.2f);
	TestFalse(TEXT("shot over"), F.Director->IsShotActive());
	TestFalse(TEXT("slow-mo over"), F.SlowMo());
	TestEqual(TEXT("shot counted"), F.Director->GetShotCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorMultiKill, "TriadDrift.Director.MultiKillWindow", TD_TEST_FLAGS)
bool FTDDirectorMultiKill::RunTest(const FString& Parameters)
{
	{
		FDirectorFixture F;
		F.KillEnemy();
		F.W.Advance(0.5f);
		F.KillEnemy();
		TestEqual(TEXT("two kills are not a multi-kill"), F.Events->Highlights, 0);
		F.W.Advance(0.5f);
		F.KillEnemy();
		TestEqual(TEXT("three kills in 1 s = multi-kill"), F.Events->Highlights, 1);
		TestTrue(TEXT("kind = MultiKill"), F.Events->LastHighlight == ETDHighlightKind::MultiKill);
	}
	{
		FDirectorFixture F;
		F.KillEnemy();
		F.W.Advance(0.8f);
		F.KillEnemy();
		F.W.Advance(0.8f);
		F.KillEnemy();
		TestEqual(TEXT("three kills over 1.6 s are not a multi-kill"), F.Events->Highlights, 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorRateLimit, "TriadDrift.Director.RateLimitedButBigMomentsPreempt", TD_TEST_FLAGS)
bool FTDDirectorRateLimit::RunTest(const FString& Parameters)
{
	FDirectorFixture F;
	F.KillEnemy(ETDDamageKind::Ram);
	TestEqual(TEXT("first ram kill"), F.Events->Highlights, 1);
	F.W.Advance(0.3f);
	F.KillEnemy(ETDDamageKind::Ram);
	TestEqual(TEXT("second ram kill during the shot is suppressed"), F.Events->Highlights, 1);
	F.W.Advance(0.1f);
	F.KillEnemy(ETDDamageKind::Energy, TDDirectorTags::Generator);
	TestEqual(TEXT("a generator kill takes over the shot"), F.Events->Highlights, 2);
	TestTrue(TEXT("kind = GeneratorKill"), F.Events->LastHighlight == ETDHighlightKind::GeneratorKill);
	F.W.Advance(1.5f);
	F.KillEnemy(ETDDamageKind::Ram);
	TestEqual(TEXT("ram kill inside the global cooldown is suppressed"), F.Events->Highlights, 2);
	F.W.Advance(6.f);
	F.KillEnemy(ETDDamageKind::Ram);
	TestEqual(TEXT("allowed again after the cooldowns"), F.Events->Highlights, 3);
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
	F.W.Advance(10.f);
	F.KillEnemy(ETDDamageKind::Explosive, TDDirectorTags::Reactor);
	TestEqual(TEXT("even a reactor kill is ignored"), F.Events->Highlights, 1);
	TestFalse(TEXT("and no slow-mo"), F.SlowMo());
	Bus->BroadcastTakeStarted(TEXT("Wave1"), 2);
	TestTrue(TEXT("new take: highlights allowed"), F.Director->CanTriggerHighlights());
	F.KillEnemy(ETDDamageKind::Explosive, TDDirectorTags::Reactor);
	TestEqual(TEXT("reactor kill in the new take"), F.Events->Highlights, 2);
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
	TestFalse(TEXT("null-safe: no subject still rate-limited, not a crash"), F.Director->ReportHighlight(ETDHighlightKind::PerfectDeflect, nullptr));
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

#endif // WITH_DEV_AUTOMATION_TESTS

// Triad Drift - ENG-40 tutorial phase prompts tests. Owner: Teammate 3.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Story/TDTutorialPhases.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Waves/TDWaveDirector.h"

namespace TDTutorialPhasesTests
{
	struct FFixture
	{
		FTDTestWorld W;
		UTDGameInstance* GI = nullptr;
		UTDTutorialPhases* Phases = nullptr;
		UTDEventBus* Bus = nullptr;
		APawn* Pawn = nullptr;

		explicit FFixture(bool bTutorial)
			: W(UTDGameInstance::StaticClass())
		{
			GI = Cast<UTDGameInstance>(W.GetGameInstance());
			W.Spawn<ATDWaveDirector>();
			APlayerController* PC = W.SpawnPlayerController();
			Pawn = W.Spawn<APawn>();
			if (PC && Pawn) { PC->Possess(Pawn); }
			Phases = UTDTutorialPhases::Get(W.GetWorld());
			Bus = UTDEventBus::Get(W.GetWorld());
			if (Phases) { Phases->bForceTutorialForTests = bTutorial; Phases->bRequireFlyingShip = false; }
		}
		FString Prompt() const
		{
			FName Id; FText T;
			return UTDTutorialPhases::GetPrompt(W.GetWorld(), Id, T) ? T.ToString() : FString();
		}
	};
}

using namespace TDTutorialPhasesTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialPhasesInOrder, "TriadDrift.Story.Tutorial.PhasesTeachOneThingAtATime", TD_TEST_FLAGS)
bool FTDTutorialPhasesInOrder::RunTest(const FString& Parameters)
{
	// ENG-40 (storyline.md §13): EVASIVE MANEUVERS -> TARGET AND FIRE -> SQUAD SYSTEMS, each done by doing it, then Done.
	FFixture F(true);
	if (!TestNotNull(TEXT("phases"), F.Phases) || !TestNotNull(TEXT("bus"), F.Bus) || !TestNotNull(TEXT("GI"), F.GI))
	{
		return false;
	}
	TArray<ETDTeachPhase> Seen;
	F.Phases->OnTeachPhaseChanged.AddLambda([&Seen](ETDTeachPhase P) { Seen.Add(P); });
	TestEqual(TEXT("nothing before wave 1"), UTDTutorialPhases::GetTeachPhase(F.W.GetWorld()), ETDTeachPhase::None);
	F.Bus->BroadcastWaveStarted(1, 0.2f);
	TestEqual(TEXT("wave 1: evade"), UTDTutorialPhases::GetTeachPhase(F.W.GetWorld()), ETDTeachPhase::Evade);
	TestEqual(TEXT("prompt"), F.Prompt(), FString(TEXT("EVASIVE MANEUVERS")));
	// Not a ship pawn: no boost to observe; the timed substitute moves it on (never a hard lock).
	F.W.Advance(F.Phases->EvadeSubstituteSeconds - 2.f, 1.f / 30.f);
	TestEqual(TEXT("still evading before the substitute"), F.Phases->GetPhase(), ETDTeachPhase::Evade);
	F.W.Advance(3.f, 1.f / 30.f);
	TestEqual(TEXT("then target and fire"), F.Phases->GetPhase(), ETDTeachPhase::TargetAndFire);
	TestEqual(TEXT("its prompt counts kills"), F.Prompt(), FString(TEXT("TARGET AND FIRE  0 / 2")));
	AActor* Enemy = F.W.Spawn<AActor>();
	F.Bus->BroadcastActorKilled(Enemy, nullptr, ETDDamageKind::Energy);
	F.Bus->BroadcastActorKilled(Enemy, Enemy, ETDDamageKind::Energy);
	TestEqual(TEXT("kills by others don't count"), F.Phases->GetKills(), 0);
	F.Bus->BroadcastActorKilled(Enemy, F.Pawn, ETDDamageKind::Energy);
	F.W.Tick();
	TestEqual(TEXT("1 / 2"), F.Prompt(), FString(TEXT("TARGET AND FIRE  1 / 2")));
	F.Bus->BroadcastActorKilled(Enemy, F.Pawn, ETDDamageKind::Energy);
	F.W.Tick();
	TestEqual(TEXT("two kills: squad systems"), F.Phases->GetPhase(), ETDTeachPhase::SquadSystems);
	TestEqual(TEXT("switch first"), F.Prompt(), FString(TEXT("SQUAD SYSTEMS · SWITCH SHIPS")));
	F.Bus->BroadcastShipSwitched(ETDShipId::Lancer, ETDShipId::Rift);
	F.W.Tick();
	TestEqual(TEXT("then a power"), F.Prompt(), FString(TEXT("SQUAD SYSTEMS · USE A POWER")));
	F.Bus->BroadcastPowerUsed(ETDShipId::Lancer, TEXT("Ram"));
	F.W.Tick();
	TestEqual(TEXT("done"), F.Phases->GetPhase(), ETDTeachPhase::Done);
	TestEqual(TEXT("no prompt once done"), F.Prompt(), FString());
	TestTrue(TEXT("every change announced in order"), Seen == TArray<ETDTeachPhase>({ ETDTeachPhase::Evade, ETDTeachPhase::TargetAndFire,
		ETDTeachPhase::SquadSystems, ETDTeachPhase::Done }));
	TestTrue(TEXT("Kato marks a hostile"), F.GI->HasPlayedTransmission(TEXT("TB_Kato_Marking")));
	TestTrue(TEXT("Moss: three ships"), F.GI->HasPlayedTransmission(TEXT("TB_Moss_ThreeShips")));
	TestTrue(TEXT("sticky for the run (T4)"), F.GI->HasPlayedTransmission(TEXT("Seq_TutorialTaught")));
	F.Bus->BroadcastWaveStarted(1, 0.2f);
	TestEqual(TEXT("a retake never restarts it"), F.Phases->GetPhase(), ETDTeachPhase::Done);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialPhasesOff, "TriadDrift.Story.Tutorial.OnlyInATutorialRun", TD_TEST_FLAGS)
bool FTDTutorialPhasesOff::RunTest(const FString& Parameters)
{
	{
		// T4: a test world in Auto mode is not a tutorial run; wave tests stay deterministic.
		FFixture F(false);
		F.Bus->BroadcastWaveStarted(1, 0.2f);
		TestEqual(TEXT("Auto in a test world: none"), UTDTutorialPhases::GetTeachPhase(F.W.GetWorld()), ETDTeachPhase::None);
	}
	{
		// Taught earlier this run (a Continue after it, or the tutorial skip's state): nothing.
		FFixture F(true);
		F.GI->MarkTransmissionPlayed(TEXT("Seq_TutorialTaught"));
		F.Bus->BroadcastWaveStarted(1, 0.2f);
		TestEqual(TEXT("taught: none"), F.Phases->GetPhase(), ETDTeachPhase::None);
	}
	{
		// A run that starts at wave 2 (checkpoint) still learns the squad; skip ends it.
		FFixture F(true);
		F.Bus->BroadcastWaveStarted(2, 0.4f);
		TestEqual(TEXT("wave 2: squad systems"), F.Phases->GetPhase(), ETDTeachPhase::SquadSystems);
		F.W.Advance(F.Phases->SquadSubstituteSeconds + 1.f, 1.f / 30.f);
		TestEqual(TEXT("the substitute ends it"), F.Phases->GetPhase(), ETDTeachPhase::Done);
	}
	{
		FFixture F(true);
		F.Bus->BroadcastWaveStarted(1, 0.2f);
		F.Phases->SkipAll();
		TestEqual(TEXT("skip: done"), F.Phases->GetPhase(), ETDTeachPhase::Done);
	}
	{
		// Live walk: no time counts while the player has no ship (the intro, a cut): the substitute never runs out unseen.
		FFixture F(true);
		F.Phases->bRequireFlyingShip = true;
		F.Bus->BroadcastWaveStarted(1, 0.2f);
		F.W.Advance(F.Phases->EvadeSubstituteSeconds + 5.f, 1.f / 30.f);
		TestEqual(TEXT("no flying ship: still evading"), F.Phases->GetPhase(), ETDTeachPhase::Evade);
	}
	return true;
}

#endif

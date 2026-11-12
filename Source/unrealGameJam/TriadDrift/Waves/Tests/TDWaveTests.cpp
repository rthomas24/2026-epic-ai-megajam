// Automation tests for the wave director (ENG-14). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Waves

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace
{
	/** Spawns stand-in enemies so the director runs without flight / weapons code. */
	void TDWaveTest_UseStandIns(ATDWaveDirector* Director)
	{
		Director->SetSpawnFactory([](UWorld* World, FName, const FTransform& Transform) -> AActor*
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<ATDWaveTestEnemy>(ATDWaveTestEnemy::StaticClass(), Transform, Params);
		});
	}

	/** A player-team actor to anchor spawns and receive hull repair. */
	AActor* TDWaveTest_SpawnPlayer(FTDTestWorld& W, float MaxHull = 100.f)
	{
		AActor* Actor = W.Spawn<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		UTDHealthComponent* Health = NewObject<UTDHealthComponent>(Actor, TEXT("Health"));
		Health->MaxHull = MaxHull;
		Health->MaxShield = 0.f;
		Health->DefaultTeam = ETDTeam::Player;
		Health->RegisterComponent();
		return Actor;
	}

	ATDWaveDirector* TDWaveTest_SpawnDirector(FTDTestWorld& W, AActor* Anchor)
	{
		ATDWaveDirector* Director = W.Spawn<ATDWaveDirector>();
		Director->SpawnAnchorOverride = Anchor;
		TDWaveTest_UseStandIns(Director);
		return Director;
	}

	void TDWaveTest_KillAll(ATDWaveDirector* Director, int32 Leave = 0)
	{
		TArray<AActor*> Living = Director->GetLivingEnemies();
		for (int32 i = 0; i < Living.Num() - Leave; ++i)
		{
			if (UTDHealthComponent* H = UTDStatics::GetHealth(Living[i]))
			{
				H->Kill();
			}
		}
	}

	/** Run the current wave until every group has warped in. */
	void TDWaveTest_SpawnEverything(FTDTestWorld& W, ATDWaveDirector* Director, float MaxSeconds = 30.f)
	{
		float T = 0.f;
		while (Director->GetPendingGroupCount() > 0 && T < MaxSeconds)
		{
			W.Tick(1.f / 60.f);
			T += 1.f / 60.f;
		}
	}

	struct FTDWaveTestScopedListener
	{
		UTDWaveTestListener* L;
		explicit FTDWaveTestScopedListener(UObject* Ctx) : L(NewObject<UTDWaveTestListener>()) { L->AddToRoot(); L->Bind(Ctx); }
		~FTDWaveTestScopedListener() { L->RemoveFromRoot(); }
		UTDWaveTestListener* operator->() const { return L; }
	};

	int32 TDWaveTest_Count(const FTDWaveDef& W, FName Id)
	{
		int32 N = 0;
		for (const FTDWaveGroup& G : W.Groups) { if (G.EnemyId == Id) { N += G.Total(); } }
		return N;
	}
}

// ------------------------------------------------------------------------------------------- table

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWavesTable, "TriadDrift.Waves.Table.MatchesFeatures", TD_TEST_FLAGS)
bool FTDWavesTable::RunTest(const FString& Parameters)
{
	using namespace TDEnemyIds;
	const TArray<FTDWaveDef> T = ATDWaveDirector::MakeDefaultWaves();
	if (!TestEqual(TEXT("five waves"), T.Num(), 5)) { return false; }

	TestEqual(TEXT("W1 = 2 x 4 fighters"), TDWaveTest_Count(T[0], ShardFighter), 8);
	TestEqual(TEXT("W1 total"), T[0].TotalEnemies(), 8);
	TestEqual(TEXT("W1 intensity"), T[0].Intensity01, 0.2f);

	TestEqual(TEXT("W2 = 3 x 5 fighters"), TDWaveTest_Count(T[1], ShardFighter), 15);
	TestEqual(TEXT("W2 + 2 interceptors"), TDWaveTest_Count(T[1], HookInterceptor), 2);
	TestEqual(TEXT("W2 intensity"), T[1].Intensity01, 0.35f);
	TestTrue(TEXT("W2 reveals Verdance"), T[1].bRevealsMarker && T[1].MarkerRevealed == ETDDestination::Verdance);

	TestEqual(TEXT("W3 = 2 gunships"), TDWaveTest_Count(T[2], WallGunship), 2);
	TestEqual(TEXT("W3 + 3 x 5 fighters"), TDWaveTest_Count(T[2], ShardFighter), 15);
	TestEqual(TEXT("W3 intensity"), T[2].Intensity01, 0.55f);

	TestEqual(TEXT("W4 gunships"), TDWaveTest_Count(T[3], WallGunship), 2);
	TestEqual(TEXT("W4 bombers"), TDWaveTest_Count(T[3], MaulBomber), 2);
	TestEqual(TEXT("W4 interceptors"), TDWaveTest_Count(T[3], HookInterceptor), 4);
	TestTrue(TEXT("W4 fighters"), TDWaveTest_Count(T[3], ShardFighter) > 0);
	TestEqual(TEXT("W4 intensity"), T[3].Intensity01, 0.7f);
	TestTrue(TEXT("W4 reveals Cinder"), T[3].bRevealsMarker && T[3].MarkerRevealed == ETDDestination::Cinder);

	TestEqual(TEXT("W5 carriers"), TDWaveTest_Count(T[4], HiveCarrier), 2);
	TestTrue(TEXT("W5 escorts"), TDWaveTest_Count(T[4], ShardFighter) + TDWaveTest_Count(T[4], HookInterceptor) > 0);
	TestTrue(TEXT("W5 bombers"), TDWaveTest_Count(T[4], MaulBomber) > 0);
	TestEqual(TEXT("W5 intensity"), T[4].Intensity01, 0.9f);
	TestTrue(TEXT("W5 reveals Dominion"), T[4].bRevealsDominion);

	TestTrue(TEXT("phases"), T[0].Phase == ETDGamePhase::Waves1to2 && T[1].Phase == ETDGamePhase::Waves1to2
		&& T[2].Phase == ETDGamePhase::Waves3to4 && T[3].Phase == ETDGamePhase::Waves3to4 && T[4].Phase == ETDGamePhase::Wave5);

	for (const FTDWaveDef& W : T)
	{
		for (const FTDWaveGroup& G : W.Groups)
		{
			TestTrue(*FString::Printf(TEXT("%s uses a known enemy (%s)"), *W.SceneId.ToString(), *G.EnemyId.ToString()), UTDEnemyLibrary::HasEnemy(G.EnemyId));
		}
	}
	TestEqual(TEXT("scene ids"), T[2].SceneId, FName(TEXT("Wave3")));
	return true;
}

// ---------------------------------------------------------------------------------------- spawning

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWavesWarpIn, "TriadDrift.Waves.Spawn.WarpInBehindAndFlanking", TD_TEST_FLAGS)
bool FTDWavesWarpIn::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Player = TDWaveTest_SpawnPlayer(W);
	ATDWaveDirector* D = TDWaveTest_SpawnDirector(W, Player);
	FTDWaveTestScopedListener L(W.GetWorld());

	D->StartWave(0);
	TestTrue(TEXT("state warping in"), D->GetState() == ETDWaveState::WarpingIn);
	TestEqual(TEXT("nothing spawned during the warp-in telegraph"), D->GetEnemiesAlive(), 0);
	TestEqual(TEXT("wave 1 total"), D->GetEnemiesTotal(), 8);
	TestEqual(TEXT("OnWaveStarted(1, 0.2)"), L->Started.Num() == 1 && L->Started[0] == 1 && FMath::IsNearlyEqual(L->Intensities[0], 0.2f), true);

	W.Advance(D->WarpInSeconds - 0.1f);
	TestEqual(TEXT("still nothing before warp-in ends"), D->GetEnemiesAlive(), 0);
	W.Advance(0.2f);
	TestEqual(TEXT("first pack of 4 arrived"), D->GetEnemiesAlive(), 4);
	TestTrue(TEXT("state active"), D->GetState() == ETDWaveState::Active);
	W.Advance(2.f);
	TestEqual(TEXT("second pack arrived after its 2 s delay"), D->GetEnemiesAlive(), 8);

	const FVector Fwd = Player->GetActorForwardVector();
	int32 Followers = 0;
	for (AActor* E : D->GetLivingEnemies())
	{
		const FVector To = E->GetActorLocation() - Player->GetActorLocation();
		const float Dist = To.Size();
		TestTrue(TEXT("spawned at warp distance"), Dist > D->SpawnDistance - D->SpawnJitter - 4.f * D->FormationSpacing && Dist < D->SpawnDistance + D->SpawnJitter + 4.f * D->FormationSpacing);
		TestTrue(TEXT("never spawns ahead of the player"), FVector::DotProduct(To.GetSafeNormal(), Fwd) < 0.3f);
		ATDWaveTestEnemy* T = Cast<ATDWaveTestEnemy>(E);
		TestTrue(TEXT("initialised with its enemy id"), T && T->EnemyId == TDEnemyIds::ShardFighter);
		TestTrue(TEXT("warp-in played"), T && T->bWarpedIn);
		if (T && T->Leader.IsValid()) { ++Followers; }
	}
	TestEqual(TEXT("each pack of 4 has a leader + 3 followers"), Followers, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWavesClear, "TriadDrift.Waves.Clear.OnlyWhenAllDead", TD_TEST_FLAGS)
bool FTDWavesClear::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDWaveDirector* D = TDWaveTest_SpawnDirector(W, TDWaveTest_SpawnPlayer(W));
	FTDWaveTestScopedListener L(W.GetWorld());

	D->StartWave(0);
	TDWaveTest_SpawnEverything(W, D);
	TestEqual(TEXT("all 8 alive"), D->GetEnemiesAlive(), 8);

	TDWaveTest_KillAll(D, 1);
	W.Tick();
	TestEqual(TEXT("one left"), D->GetEnemiesAlive(), 1);
	TestTrue(TEXT("not cleared with one alive"), D->GetState() == ETDWaveState::Active);
	TestEqual(TEXT("no clear event yet"), L->Cleared.Num(), 0);

	// A carrier launch mid-wave counts too.
	ATDWaveTestEnemy* Launched = W.Spawn<ATDWaveTestEnemy>();
	D->RegisterEnemy(Launched);
	D->RegisterEnemy(Launched);	// twice is harmless
	TestEqual(TEXT("launch counted once"), D->GetEnemiesTotal(), 9);

	TDWaveTest_KillAll(D, 1);	// kills the original straggler, leaves the launched fighter
	W.Tick();
	TestTrue(TEXT("launched fighter holds the wave open"), D->GetState() == ETDWaveState::Active);

	UTDStatics::GetHealth(Launched)->Kill();
	W.Tick();
	TestEqual(TEXT("cleared"), L->Cleared.Num() == 1 && L->Cleared[0] == 1, true);
	TestTrue(TEXT("breather after wave 1 (same phase as wave 2)"), D->GetState() == ETDWaveState::Breather);

	// Destroying instead of killing also counts.
	D->StartWave(0);
	TDWaveTest_SpawnEverything(W, D);
	for (AActor* E : D->GetLivingEnemies()) { E->Destroy(); }
	W.Tick();
	TestEqual(TEXT("destroyed enemies clear the wave"), L->Cleared.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWavesBreather, "TriadDrift.Waves.Breather.SixSecondsAndRepair", TD_TEST_FLAGS)
bool FTDWavesBreather::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		AActor* Player = TDWaveTest_SpawnPlayer(W, 100.f);
		ATDWaveDirector* D = TDWaveTest_SpawnDirector(W, Player);
		FTDWaveTestScopedListener L(W.GetWorld());
		UTDHealthComponent* PH = UTDStatics::GetHealth(Player);

		D->StartWave(0);
		TDWaveTest_SpawnEverything(W, D);
		UTDStatics::ApplyTDDamage(Player, 60.f, ETDDamageKind::Energy, nullptr, nullptr);
		const float HullBefore = PH->GetHull();
		TDWaveTest_KillAll(D);
		W.Tick(Step);

		TestTrue(TEXT("breather"), D->GetState() == ETDWaveState::Breather);
		TestEqual(TEXT("breather length"), D->GetBreatherRemaining(), 6.f, 0.05f);
		TestTrue(TEXT("HUD sees the breather"), D->GetWaveInfo().bInBreather);

		W.Advance(5.8f, Step);
		TestTrue(TEXT("still in breather at 5.8 s"), D->GetState() == ETDWaveState::Breather);
		TestTrue(TEXT("hull repairs between waves"), PH->GetHull() > HullBefore + 20.f);
		TestTrue(TEXT("repair never exceeds max"), PH->GetHull() <= PH->MaxHull);

		W.Advance(0.4f, Step);
		TestEqual(TEXT("wave 2 starts after 6 s"), D->GetCurrentWaveIndex(), 1);
		TestTrue(TEXT("OnWaveStarted(2, 0.35)"), L->Started.Num() == 2 && L->Started[1] == 2 && FMath::IsNearlyEqual(L->Intensities[1], 0.35f));
		const float HullAtStart = PH->GetHull();
		W.Advance(1.f, Step);
		TestEqual(TEXT("no repair during a wave"), PH->GetHull(), HullAtStart);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWavesPhase, "TriadDrift.Waves.Phase.MarkersAndDominion", TD_TEST_FLAGS)
bool FTDWavesPhase::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("fake flow"), GI)) { return false; }
	ATDWaveDirector* D = TDWaveTest_SpawnDirector(W, TDWaveTest_SpawnPlayer(W));
	FTDWaveTestScopedListener L(W.GetWorld());

	GI->Progress.Phase = ETDGamePhase::Waves1to2;
	D->StartWave(1);
	TDWaveTest_SpawnEverything(W, D);
	TDWaveTest_KillAll(D);
	W.Tick();
	TestTrue(TEXT("wave 2 ends its phase"), D->GetState() == ETDWaveState::PhaseComplete);
	TestTrue(TEXT("Verdance marker revealed"), L->Markers.Num() == 1 && L->Markers[0] == ETDDestination::Verdance);
	TestTrue(TEXT("flow told wave 2 cleared"), GI->ReportedWaves.Contains(2));
	TestTrue(TEXT("hyperdrive tip"), GI->Tips.Contains(TDTipIds::Hyperdrive));
	W.Advance(10.f);
	TestTrue(TEXT("no auto-start of wave 3 while on the planet"), D->GetState() == ETDWaveState::PhaseComplete);

	D->StartWave(3);
	TDWaveTest_SpawnEverything(W, D);
	TDWaveTest_KillAll(D);
	W.Tick();
	TestTrue(TEXT("Cinder marker revealed"), L->Markers.Contains(ETDDestination::Cinder));

	D->StartWave(4);
	TDWaveTest_SpawnEverything(W, D);
	TDWaveTest_KillAll(D);
	W.Tick();
	TestTrue(TEXT("Dominion revealed after wave 5"), GI->Progress.bDominionRevealed && !GI->Progress.bDominionDefensesDown);
	TestTrue(TEXT("all waves complete"), D->GetState() == ETDWaveState::AllComplete);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWavesResume, "TriadDrift.Waves.Resume.FromProgress", TD_TEST_FLAGS)
bool FTDWavesResume::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("fake flow"), GI)) { return false; }
	ATDWaveDirector* D = TDWaveTest_SpawnDirector(W, TDWaveTest_SpawnPlayer(W));

	// Fresh run: Intro phase starts wave 1.
	GI->Progress = FTDProgress();
	D->StartFromProgress();
	TestEqual(TEXT("fresh run -> wave 1"), D->GetCurrentWaveIndex(), 0);
	TestTrue(TEXT("running"), D->GetWaveInfo().bWaveActive);

	D->StartFromProgress();
	TestEqual(TEXT("idempotent while running"), D->GetCurrentWaveIndex(), 0);

	// Back from Verdance: 2 cleared, phase Waves3to4 -> wave 3.
	D->StopAndClear();
	GI->Progress.WavesCleared = 2;
	GI->Progress.Phase = ETDGamePhase::Waves3to4;
	D->StartFromProgress();
	TestEqual(TEXT("resume -> wave 3"), D->GetCurrentWaveIndex(), 2);
	TestEqual(TEXT("HUD wave number is 1-based"), D->GetWaveInfo().WaveNumber, 3);

	// On a planet: no wave.
	D->StopAndClear();
	GI->Progress.WavesCleared = 4;
	GI->Progress.Phase = ETDGamePhase::Cinder;
	D->StartFromProgress();
	TestTrue(TEXT("planet phase -> idle"), D->GetState() == ETDWaveState::Idle);

	// Everything done.
	GI->Progress.WavesCleared = 5;
	GI->Progress.Phase = ETDGamePhase::DominionAssault;
	D->StartFromProgress();
	TestTrue(TEXT("all cleared -> complete"), D->GetState() == ETDWaveState::AllComplete);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWavesEvents, "TriadDrift.Waves.Events.TakesTipsAndSkip", TD_TEST_FLAGS)
bool FTDWavesEvents::RunTest(const FString& Parameters)
{
	FTDTestWorld W;	// no game flow: the director announces takes and tips itself
	ATDWaveDirector* D = TDWaveTest_SpawnDirector(W, TDWaveTest_SpawnPlayer(W));
	FTDWaveTestScopedListener L(W.GetWorld());

	D->StartWave(0);
	TestTrue(TEXT("OnTakeStarted(Wave1, 1)"), L->Takes.Num() == 1 && L->Takes[0] == TEXT("Wave1:1"));
	TestTrue(TEXT("switching tip on wave 1"), L->Tips.Contains(TDTipIds::Switching));
	TDWaveTest_SpawnEverything(W, D);

	// "CUT!": the flow announces take 2 of this scene -> the wave rebuilds.
	TDWaveTest_KillAll(D, 3);
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	Bus->BroadcastTakeStarted(TEXT("Wave1"), 2);
	TestEqual(TEXT("retake despawned the old enemies"), D->GetEnemiesAlive(), 0);
	TestTrue(TEXT("retake warps in again"), D->GetState() == ETDWaveState::WarpingIn);
	TDWaveTest_SpawnEverything(W, D);
	TestEqual(TEXT("retake has the full wave"), D->GetEnemiesAlive(), 8);
	TestEqual(TEXT("still wave 1"), D->GetWaveInfo().WaveNumber, 1);

	// Unrelated scene ids are ignored.
	Bus->BroadcastTakeStarted(TEXT("Verdance_Obj1"), 5);
	TestEqual(TEXT("other scenes don't restart the wave"), D->GetEnemiesAlive(), 8);

	// SkipWave: active -> cleared + breather; breather -> next wave.
	D->SkipWave();
	TestTrue(TEXT("skip clears"), D->GetState() == ETDWaveState::Breather);
	TestEqual(TEXT("skip despawns"), D->GetEnemiesAlive(), 0);
	TestTrue(TEXT("skip fires OnWaveCleared(1)"), L->Cleared.Contains(1));
	D->SkipWave();
	TestEqual(TEXT("skip breather -> wave 2"), D->GetCurrentWaveIndex(), 1);
	TestTrue(TEXT("wave 2 is take 1"), L->Takes.Last() == TEXT("Wave2:1"));
	TestEqual(TEXT("switch tip only once"), L->Tips.FilterByPredicate([](const FName& N) { return N == TDTipIds::Switching; }).Num(), 1);

	// Unhappy paths.
	D->StartWave(99);
	TestEqual(TEXT("bad index ignored"), D->GetCurrentWaveIndex(), 1);
	D->RegisterEnemy(nullptr);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// ENG-13 behaviour tests with real weapons, real AI and real projectiles (review M4).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies.Behaviour

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "EngineUtils.h"

namespace
{
	/** A squad ship that sits still (re-placed every frame by the caller if needed). */
	ATDSpacecraft* TDEnemyBeh_SquadShip(FTDTestWorld& W, const FVector& Loc, bool bPossess = false, float Hull = 1000.f)
	{
		ATDSpacecraft* S = W.Spawn<ATDSpacecraft>(Loc);
		S->SetTeam(ETDTeam::Player);
		S->GetHealth()->DefaultTeam = ETDTeam::Player;
		S->GetHealth()->InitMaxValues(Hull, 0.f, true);
		S->SetSpeedOverride(0.f, 0.f);
		if (bPossess) { W.SpawnPlayerController(S); }
		return S;
	}

	ATDEnemyShip* TDEnemyBeh_Enemy(FTDTestWorld& W, FName Id, const FVector& Loc, bool bAI, const FRotator& Rot = FRotator::ZeroRotator)
	{
		ATDEnemyShip* E = W.Spawn<ATDEnemyShip>(Loc, Rot);
		if (!bAI)
		{
			if (AController* C = E->GetController()) { C->UnPossess(); C->Destroy(); }
		}
		E->InitFromSpec(Id);
		return E;
	}

	ATDEnemyAIController* TDEnemyBeh_AIOf(const ATDEnemyShip* E) { return E ? Cast<ATDEnemyAIController>(E->GetController()) : nullptr; }

	int32 TDEnemyBeh_CountEnemyProjectiles(FTDTestWorld& W)
	{
		const UTDProjectileSubsystem* P = UTDProjectileSubsystem::Get(W.GetWorld());
		int32 N = 0;
		if (P) { for (const ATDProjectile* Proj : P->GetLiveProjectiles()) { if (Proj && Proj->GetTeam() == ETDTeam::Enemy) { ++N; } } }
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehGuns, "TriadDrift.Enemies.Behaviour.FighterGunsDealRealDamage", TD_TEST_FLAGS)
bool FTDBehGuns::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSpacecraft* Target = TDEnemyBeh_SquadShip(W, FVector(5000.f, 0.f, 0.f));
	ATDEnemyShip* F = TDEnemyBeh_Enemy(W, TDEnemyIds::ShardFighter, FVector::ZeroVector, false);
	F->SetSpeedOverride(0.f, 0.f);
	F->SetGunsFiring(true, Target);
	const float Seconds = 3.f;
	for (int32 i = 0; i < 60 * Seconds; ++i)
	{
		Target->SetActorLocation(FVector(5000.f, 0.f, 0.f));	// pin the target in front
		F->SetActorLocation(FVector::ZeroVector);
		W.Tick();
	}
	const float Dealt = 1000.f - Target->GetHealth()->GetHull();
	TestTrue(*FString::Printf(TEXT("pulse guns hurt the squad ship (%.0f)"), Dealt), Dealt > 0.f);
	TestTrue(TEXT("in 5-damage bolts"), FMath::IsNearlyZero(FMath::Fmod(Dealt, 5.f), 0.01f));
	TestTrue(*FString::Printf(TEXT("no faster than 4 shots/s (%.0f <= %.0f)"), Dealt, 5.f * (4.f * Seconds + 1.f)), Dealt <= 5.f * (4.f * Seconds + 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehCycle, "TriadDrift.Enemies.Behaviour.ApproachAttackBreakOffReapproach", TD_TEST_FLAGS)
bool FTDBehCycle::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDEnemyBeh_SquadShip(W, FVector(25000.f, 0.f, 0.f));
	ATDEnemyShip* F = TDEnemyBeh_Enemy(W, TDEnemyIds::ShardFighter, FVector::ZeroVector, true);
	ATDEnemyAIController* AI = TDEnemyBeh_AIOf(F);
	if (!TestNotNull(TEXT("AI"), AI)) { return false; }

	TArray<ETDEnemyAIState> Seq;
	for (int32 i = 0; i < 60 * 25; ++i)
	{
		W.Tick();
		if (Seq.Num() == 0 || Seq.Last() != AI->GetAIState()) { Seq.Add(AI->GetAIState()); }
	}
	auto Find = [&Seq](ETDEnemyAIState S, int32 From) { for (int32 i = From; i < Seq.Num(); ++i) { if (Seq[i] == S) { return i; } } return (int32)INDEX_NONE; };
	const int32 A = Find(ETDEnemyAIState::Approach, 0);
	const int32 R = Find(ETDEnemyAIState::AttackRun, FMath::Max(A, 0));
	const int32 B = Find(ETDEnemyAIState::BreakOff, FMath::Max(R, 0));
	const int32 A2 = Find(ETDEnemyAIState::Approach, FMath::Max(B, 0));
	TestTrue(TEXT("approach -> attack run -> break off -> re-approach"), A != INDEX_NONE && R > A && B > R && A2 > B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehFormation, "TriadDrift.Enemies.Behaviour.FormationAndLeaderDeath", TD_TEST_FLAGS)
bool FTDBehFormation::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDEnemyBeh_SquadShip(W, FVector(40000.f, 0.f, 0.f));
	ATDEnemyShip* Leader = TDEnemyBeh_Enemy(W, TDEnemyIds::ShardFighter, FVector::ZeroVector, true);
	ATDEnemyShip* Wing = TDEnemyBeh_Enemy(W, TDEnemyIds::ShardFighter, FVector(-900.f, 900.f, 0.f), true);
	const FVector Offset(-900.f, 900.f, 0.f);
	Wing->JoinFormation(Leader, Offset);
	TestTrue(TEXT("follower in formation"), TDEnemyBeh_AIOf(Wing)->GetAIState() == ETDEnemyAIState::Formation && TDEnemyBeh_AIOf(Wing)->GetFormationLeader() == Leader);
	W.Advance(3.f);
	const FVector Slot = Leader->GetActorTransform().TransformPositionNoScale(Offset);
	TestTrue(*FString::Printf(TEXT("follower holds its slot (%.0f cm off)"), FVector::Dist(Slot, Wing->GetActorLocation())), FVector::Dist(Slot, Wing->GetActorLocation()) < 3000.f);

	Leader->GetHealth()->Kill();
	W.Advance(0.2f);
	TestTrue(TEXT("follower goes solo when the leader dies"), TDEnemyBeh_AIOf(Wing)->GetFormationLeader() == nullptr && TDEnemyBeh_AIOf(Wing)->GetAIState() != ETDEnemyAIState::Formation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehTargets, "TriadDrift.Enemies.Behaviour.InterceptorsHuntThePlayersShip", TD_TEST_FLAGS)
bool FTDBehTargets::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSpacecraft* Wingman = TDEnemyBeh_SquadShip(W, FVector(20000.f, 0.f, 0.f));
	ATDSpacecraft* PlayerShip = TDEnemyBeh_SquadShip(W, FVector(-40000.f, 0.f, 0.f), /*bPossess*/ true);
	ATDEnemyShip* I = TDEnemyBeh_Enemy(W, TDEnemyIds::HookInterceptor, FVector::ZeroVector, true);
	ATDEnemyShip* F = TDEnemyBeh_Enemy(W, TDEnemyIds::ShardFighter, FVector(0.f, 2000.f, 0.f), true);
	if (!TestNotNull(TEXT("fighter AI"), TDEnemyBeh_AIOf(F)) || !TestNotNull(TEXT("interceptor AI"), TDEnemyBeh_AIOf(I))) { return false; }
	TDEnemyBeh_AIOf(F)->SetPrefersPlayerShip(false);
	W.Advance(1.5f);
	const ATDEnemyAIController* IAI = TDEnemyBeh_AIOf(I);
	const ATDEnemyAIController* FAI = TDEnemyBeh_AIOf(F);
	TestTrue(TEXT("interceptor targets the possessed ship"), IAI && IAI->GetTarget() == PlayerShip);
	TestTrue(TEXT("fighter targets the nearest squad ship (any ship, wingmen too)"), FAI && FAI->GetTarget() == Wingman);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehGunship, "TriadDrift.Enemies.Behaviour.GunshipFacesAndHoldsRange", TD_TEST_FLAGS)
bool FTDBehGunship::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSpacecraft* T = TDEnemyBeh_SquadShip(W, FVector(0.f, 20000.f, 0.f));	// off to the side
	ATDEnemyShip* G = TDEnemyBeh_Enemy(W, TDEnemyIds::WallGunship, FVector::ZeroVector, true);
	W.Advance(15.f);
	const FVector To = T->GetActorLocation() - G->GetActorLocation();
	TestTrue(*FString::Printf(TEXT("gunship faces its target (dot %.2f)"), FVector::DotProduct(G->GetActorForwardVector(), To.GetSafeNormal())), FVector::DotProduct(G->GetActorForwardVector(), To.GetSafeNormal()) > 0.85f);
	const float Hold = G->GetSpec().EngageRange * TDEnemyBeh_AIOf(G)->GunshipHoldRangeFraction;
	TestTrue(*FString::Printf(TEXT("holds range instead of ramming (%.0f cm, hold %.0f)"), To.Size(), Hold), To.Size() > Hold * 0.6f);
	TestTrue(TEXT("and opened fire"), G->GetFireCount(ETDEnemyAttack::HeavyBolt) + G->GetFireCount(ETDEnemyAttack::MissileSalvo) > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehMines, "TriadDrift.Enemies.Behaviour.BomberDropsFourMines", TD_TEST_FLAGS)
bool FTDBehMines::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSpacecraft* T = TDEnemyBeh_SquadShip(W, FVector(20000.f, 0.f, 0.f));
	ATDEnemyShip* B = TDEnemyBeh_Enemy(W, TDEnemyIds::MaulBomber, FVector::ZeroVector, false);
	const int32 Before = TDEnemyBeh_CountEnemyProjectiles(W);
	TestTrue(TEXT("trail starts"), B->TryStartAttack(ETDEnemyAttack::MineTrail, T));
	TestTrue(TEXT("trail is telegraphed"), B->IsTelegraphing());
	W.Advance(B->GetSpec().Heavy.TelegraphSeconds + 4 * 0.45f + 0.3f);
	TestEqual(TEXT("4 mines fired"), B->GetFireCount(ETDEnemyAttack::MineTrail), 4);
	TestTrue(TEXT("4 live enemy mines in the world"), TDEnemyBeh_CountEnemyProjectiles(W) - Before >= 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehTurret, "TriadDrift.Enemies.Behaviour.TurretTracksAndBursts", TD_TEST_FLAGS)
bool FTDBehTurret::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDEnemyTurret* T = W.Spawn<ATDEnemyTurret>(FVector::ZeroVector);
	FTDEnemyAttackSpec Guns = UTDEnemyLibrary::Get(TDEnemyIds::HiveCarrier).TurretGuns;
	T->Init(Guns, 60.f);
	ATDSpacecraft* Far = TDEnemyBeh_SquadShip(W, FVector(0.f, Guns.Range * 2.f, 0.f));
	W.Advance(2.f);
	TestEqual(TEXT("out of range: no bursts"), T->GetBurstCount(), 0);

	ATDSpacecraft* Near = TDEnemyBeh_SquadShip(W, FVector(0.f, 4000.f, 0.f));
	W.Advance(3.f);
	TestTrue(TEXT("targets the ship in range"), T->GetTarget() == Near);
	TestTrue(TEXT("turned to face it"), FVector::DotProduct(T->GetActorForwardVector(), (Near->GetActorLocation() - T->GetActorLocation()).GetSafeNormal()) > 0.95f);
	TestTrue(TEXT("fires bursts"), T->GetBurstCount() >= 1);
	TestTrue(TEXT("and hurts"), Near->GetHealth()->GetHull() < 1000.f);
	(void)Far;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehCarrierWave, "TriadDrift.Enemies.Behaviour.CarrierLaunchesHoldTheWaveOpen", TD_TEST_FLAGS)
bool FTDBehCarrierWave::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Anchor = TDEnemyBeh_SquadShip(W, FVector::ZeroVector);
	ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
	D->SpawnAnchorOverride = Anchor;
	FTDWaveDef Wave;
	Wave.SceneId = TEXT("Wave5");
	FTDWaveGroup G;
	G.EnemyId = TDEnemyIds::HiveCarrier;
	G.Zone = ETDSpawnZone::Behind;
	Wave.Groups.Add(G);
	D->SetWaveTable({ Wave });
	D->StartWave(0);
	W.Advance(D->WarpInSeconds + 0.2f);
	ATDEnemyShip* C = Cast<ATDEnemyShip>(D->GetLivingEnemies().Num() ? D->GetLivingEnemies()[0] : nullptr);
	if (!TestNotNull(TEXT("carrier spawned"), C)) { return false; }
	C->MaxLaunchedAlive = 4;

	W.Advance(C->GetSpec().Heavy.Interval * 0.5f + 0.5f);	// first bay launch
	TestTrue(TEXT("launched fighters counted by the director"), D->GetEnemiesTotal() > 1 && D->GetEnemiesAlive() > 1);
	W.Advance(30.f);
	int32 Launched = 0;
	for (AActor* A : D->GetLivingEnemies()) { if (A != C) { ++Launched; } }
	TestTrue(*FString::Printf(TEXT("launch cap holds (%d <= 4)"), Launched), Launched <= 4);

	C->GetHealth()->Kill();
	W.Advance(3.f);
	TestTrue(TEXT("dead carrier, launched fighters alive: wave still open"), D->GetState() == ETDWaveState::Active);
	for (AActor* A : D->GetLivingEnemies()) { UTDStatics::GetHealth(A)->Kill(); }
	W.Tick();
	TestTrue(TEXT("cleared once the launches die too"), D->GetState() != ETDWaveState::Active && D->GetState() != ETDWaveState::WarpingIn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehRetakePending, "TriadDrift.Enemies.Behaviour.RetakeWhileGroupsPending", TD_TEST_FLAGS)
bool FTDBehRetakePending::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Anchor = TDEnemyBeh_SquadShip(W, FVector::ZeroVector);
	ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
	D->SpawnAnchorOverride = Anchor;
	D->SetSpawnFactory([](UWorld* World, FName, const FTransform& T) -> AActor*
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ATDWaveTestEnemy>(ATDWaveTestEnemy::StaticClass(), T, P);
	});
	D->StartWave(1);	// wave 2: interceptors arrive 4 s late
	W.Advance(D->WarpInSeconds + 0.5f);
	TestTrue(TEXT("interceptors still pending"), D->GetPendingGroupCount() > 0);
	UTDEventBus::Get(W.GetWorld())->BroadcastTakeStarted(TEXT("Wave2"), 2);
	W.Advance(8.f);
	TestEqual(TEXT("retake spawns the wave exactly once"), D->GetEnemiesAlive(), D->GetWaveDef(1)->TotalEnemies());
	int32 Count = 0;
	for (TActorIterator<ATDWaveTestEnemy> It(W.GetWorld()); It; ++It) { if (!It->IsActorBeingDestroyed()) { ++Count; } }
	TestEqual(TEXT("no leftovers from take 1"), Count, D->GetWaveDef(1)->TotalEnemies());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehFps, "TriadDrift.Enemies.Behaviour.SameTimingAt30And120Fps", TD_TEST_FLAGS)
bool FTDBehFps::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		ATDSpacecraft* T = TDEnemyBeh_SquadShip(W, FVector(8000.f, 0.f, 0.f));
		ATDEnemyShip* I = TDEnemyBeh_Enemy(W, TDEnemyIds::HookInterceptor, FVector::ZeroVector, false);
		const float Tel = I->GetSpec().Heavy.TelegraphSeconds;
		I->TryStartAttack(ETDEnemyAttack::Missile, T);
		W.Advance(Tel - 0.05f, Step);
		TestEqual(*FString::Printf(TEXT("%.0f fps: not fired before the telegraph ends"), 1.f / Step), I->GetFireCount(ETDEnemyAttack::Missile), 0);
		W.Advance(0.1f, Step);
		TestEqual(*FString::Printf(TEXT("%.0f fps: fired right after"), 1.f / Step), I->GetFireCount(ETDEnemyAttack::Missile), 1);

		ATDEnemyShip* C = TDEnemyBeh_Enemy(W, TDEnemyIds::HiveCarrier, FVector(0.f, 30000.f, 0.f), false);
		C->MaxLaunchedAlive = 0;
		ATDCarrierBay* Bay = C->GetBays()[0];
		const float First = Bay->GetTimeToNextLaunch();
		W.Advance(First + C->GetSpec().Heavy.Interval * 2.f + 0.1f, Step);
		TestEqual(*FString::Printf(TEXT("%.0f fps: 3 bay launches"), 1.f / Step), Bay->GetLaunchCount(), 3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBehPowers, "TriadDrift.Enemies.Behaviour.PowerHooksOnARealGunship", TD_TEST_FLAGS)
bool FTDBehPowers::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSpacecraft* T = TDEnemyBeh_SquadShip(W, FVector(10000.f, 0.f, 0.f));
	ATDEnemyShip* G = TDEnemyBeh_Enemy(W, TDEnemyIds::WallGunship, FVector::ZeroVector, false);

	// EMP: ApplyStun(3) + DropFrontalShield(5).
	G->ApplyStun(3.f);
	G->DropFrontalShield(5.f);
	TestFalse(TEXT("EMP: no heavy attacks while stunned"), G->TryStartAttack(ETDEnemyAttack::HeavyBolt, T));
	const float Hull0 = G->GetHealth()->GetHull();
	UTDStatics::ApplyTDDamage(G, 40.f, ETDDamageKind::Energy, nullptr, nullptr, G->GetActorLocation(), FVector(-1, 0, 0));
	TestEqual(TEXT("EMP: frontal hit lands (shield down) at +25%"), Hull0 - G->GetHealth()->GetHull(), 50.f, 0.01f);
	W.Advance(3.1f);
	TestTrue(TEXT("EMP: stun over after 3 s"), G->TryStartAttack(ETDEnemyAttack::HeavyBolt, T));
	TestFalse(TEXT("EMP: frontal shield still down at 3 s"), G->IsFrontalShieldUp());
	W.Advance(2.f);
	TestTrue(TEXT("EMP: frontal shield back after 5 s"), G->IsFrontalShieldUp());

	// Gravity Well: SuppressFire.
	W.Advance(G->GetSpec().Heavy2.Interval);
	G->SuppressFire(2.f);
	TestFalse(TEXT("Gravity Well: can't fire inside"), G->TryStartAttack(ETDEnemyAttack::MissileSalvo, T));
	W.Advance(2.1f);
	TestTrue(TEXT("Gravity Well: fires again after"), G->TryStartAttack(ETDEnemyAttack::MissileSalvo, T));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

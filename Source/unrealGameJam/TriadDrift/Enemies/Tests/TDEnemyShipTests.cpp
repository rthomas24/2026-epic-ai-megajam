// Automation tests for enemy ships, AI and carrier parts (ENG-13).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "EngineUtils.h"

namespace
{
	/** Enemy ship with no pilot, so its facing stays put while we test it. */
	ATDEnemyShip* TDEnemyShipTest_SpawnEnemy(FTDTestWorld& W, FName Id, const FVector& Loc = FVector::ZeroVector, bool bKeepAI = false)
	{
		ATDEnemyShip* Ship = W.Spawn<ATDEnemyShip>(Loc);
		if (!Ship)
		{
			return nullptr;
		}
		if (!bKeepAI)
		{
			if (AController* C = Ship->GetController())
			{
				C->UnPossess();
				C->Destroy();
			}
		}
		Ship->InitFromSpec(Id);
		return Ship;
	}

	AActor* TDEnemyShipTest_SpawnSquadDummy(FTDTestWorld& W, const FVector& Loc)
	{
		AActor* Actor = W.Spawn<AActor>(Loc);
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		Actor->SetActorLocation(Loc);
		UTDHealthComponent* Health = NewObject<UTDHealthComponent>(Actor, TEXT("Health"));
		Health->MaxHull = 1000.f;
		Health->MaxShield = 0.f;
		Health->DefaultTeam = ETDTeam::Player;
		Health->RegisterComponent();
		return Actor;
	}

	float TDEnemyShipTest_Hit(AActor* Target, float Amount, const FVector& TravelDir, ETDDamageKind Kind = ETDDamageKind::Energy)
	{
		return UTDStatics::ApplyTDDamage(Target, Amount, Kind, nullptr, nullptr, Target->GetActorLocation(), TravelDir);
	}

	int32 TDEnemyShipTest_CountShips(FTDTestWorld& W, const AActor* Except)
	{
		int32 N = 0;
		for (TActorIterator<ATDEnemyShip> It(W.GetWorld()); It; ++It)
		{
			if (*It != Except && !It->IsDying()) { ++N; }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesInit, "TriadDrift.Enemies.Ship.InitFromSpec", TD_TEST_FLAGS)
bool FTDEnemiesInit::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	for (const FName Id : UTDEnemyLibrary::GetAllEnemyIds())
	{
		ATDEnemyShip* Ship = TDEnemyShipTest_SpawnEnemy(W, Id);
		if (!TestNotNull(*FString::Printf(TEXT("%s spawns"), *Id.ToString()), Ship)) { continue; }
		const FTDEnemySpec& Spec = UTDEnemyLibrary::Get(Id);
		UTDHealthComponent* H = Ship->GetHealth();
		TestEqual(*FString::Printf(TEXT("%s hull"), *Id.ToString()), H->MaxHull, Spec.Hull);
		TestEqual(*FString::Printf(TEXT("%s shield"), *Id.ToString()), H->MaxShield, Spec.Shield);
		TestTrue(*FString::Printf(TEXT("%s is on the enemy team"), *Id.ToString()), Ship->GetTeam() == ETDTeam::Enemy);
		TestNotNull(*FString::Printf(TEXT("%s has weapons"), *Id.ToString()), Ship->GetWeapons());
	}
	TestTrue(TEXT("AI controller class"), ATDEnemyShip::StaticClass()->GetDefaultObject<ATDEnemyShip>()->AIControllerClass == ATDEnemyAIController::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesFrontal, "TriadDrift.Enemies.Gunship.FrontalShieldBlocksFrontOnly", TD_TEST_FLAGS)
bool FTDEnemiesFrontal::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDEnemyShip* G = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::WallGunship);	// faces +X
	UTDHealthComponent* H = G->GetHealth();

	// From the front a shot travels along -X.
	TDEnemyShipTest_Hit(G, 100.f, FVector(-1, 0, 0));
	TestEqual(TEXT("front hit: hull untouched"), H->GetHull(), 400.f);
	TestEqual(TEXT("front hit: frontal shield 300 -> 200"), G->GetFrontalShield01(), 200.f / 300.f, 0.001f);

	TDEnemyShipTest_Hit(G, 100.f, FVector(1, 0, 0));
	TestEqual(TEXT("rear hit goes to the hull"), H->GetHull(), 300.f);
	TDEnemyShipTest_Hit(G, 50.f, FVector(0, 1, 0));
	TestEqual(TEXT("flank (90 deg) hit goes to the hull"), H->GetHull(), 250.f);

	TDEnemyShipTest_Hit(G, 250.f, FVector(-1, 0, 0));
	TestFalse(TEXT("shield broken by 250 more"), G->IsFrontalShieldUp());
	TestEqual(TEXT("overflow 50 reaches the hull"), H->GetHull(), 200.f);
	TDEnemyShipTest_Hit(G, 20.f, FVector(-1, 0, 0));
	TestEqual(TEXT("broken shield lets frontal fire through"), H->GetHull(), 180.f);

	W.Advance(UTDEnemyLibrary::Get(TDEnemyIds::WallGunship).FrontalShieldRecoverSeconds + 0.1f);
	TestTrue(TEXT("frontal shield recovers"), G->IsFrontalShieldUp());

	// EMP drops it for 5 s without draining it.
	G->DropFrontalShield(5.f);
	TDEnemyShipTest_Hit(G, 10.f, FVector(-1, 0, 0));
	TestEqual(TEXT("EMP'd shield doesn't block"), H->GetHull(), 170.f);
	W.Advance(5.1f);
	TestEqual(TEXT("back at full after EMP"), G->GetFrontalShield01(), 1.f, 0.001f);

	// Lancer's ram breaks it and lands in full.
	TDEnemyShipTest_Hit(G, 100.f, FVector(-1, 0, 0), ETDDamageKind::Ram);
	TestFalse(TEXT("ram breaks the frontal shield"), G->IsFrontalShieldUp());
	TestEqual(TEXT("ram damage lands"), H->GetHull(), 70.f);

	// Non-gunships have no frontal shield.
	ATDEnemyShip* F = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::ShardFighter, FVector(0, 5000, 0));
	TDEnemyShipTest_Hit(F, 10.f, FVector(-1, 0, 0));
	TestEqual(TEXT("fighter frontal hit lands"), F->GetHealth()->GetHull(), 20.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesTelegraph, "TriadDrift.Enemies.Attack.TelegraphPrecedesFire", TD_TEST_FLAGS)
bool FTDEnemiesTelegraph::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Target = TDEnemyShipTest_SpawnSquadDummy(W, FVector(8000, 0, 0));
	ATDEnemyShip* I = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::HookInterceptor);
	const float Telegraph = I->GetSpec().Heavy.TelegraphSeconds;

	TestTrue(TEXT("missile starts"), I->TryStartAttack(ETDEnemyAttack::Missile, Target));
	TestTrue(TEXT("telegraphing"), I->IsTelegraphing());
	TestTrue(TEXT("telegraph kind"), I->GetTelegraphAttack() == ETDEnemyAttack::Missile);
	TestEqual(TEXT("telegraph duration"), I->GetTelegraphDuration(), Telegraph);
	TestEqual(TEXT("not fired at telegraph start"), I->GetFireCount(ETDEnemyAttack::Missile), 0);

	W.Advance(Telegraph - 0.1f);
	TestEqual(TEXT("not fired during the telegraph"), I->GetFireCount(ETDEnemyAttack::Missile), 0);
	TestTrue(TEXT("telegraph progresses"), I->GetTelegraph01() > 0.7f);
	W.Advance(0.2f);
	TestEqual(TEXT("fired after the telegraph"), I->GetFireCount(ETDEnemyAttack::Missile), 1);
	TestFalse(TEXT("telegraph over"), I->IsTelegraphing());

	TestFalse(TEXT("8 s cooldown blocks a second missile"), I->TryStartAttack(ETDEnemyAttack::Missile, Target));
	W.Advance(8.f - (Telegraph + 0.1f) - 0.1f);	// total 7.9 s since the start
	TestFalse(TEXT("still cooling down at 7.9 s"), I->CanStartAttack(ETDEnemyAttack::Missile));
	W.Advance(0.15f);
	TestTrue(TEXT("ready again 8 s after the first started"), I->CanStartAttack(ETDEnemyAttack::Missile));

	// Gunship volley: telegraph, then Count heavy bolts.
	ATDEnemyShip* G = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::WallGunship, FVector(0, 6000, 0));
	TestTrue(TEXT("volley starts"), G->TryStartAttack(ETDEnemyAttack::HeavyBolt, Target));
	TestFalse(TEXT("only one big attack at a time"), G->TryStartAttack(ETDEnemyAttack::MissileSalvo, Target));
	W.Advance(G->GetSpec().Heavy.TelegraphSeconds + 1.f);
	TestEqual(TEXT("3 heavy bolts fired"), G->GetFireCount(ETDEnemyAttack::HeavyBolt), 3);
	TestFalse(TEXT("guns attack isn't a telegraphed attack"), G->TryStartAttack(ETDEnemyAttack::Guns, Target));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesStun, "TriadDrift.Enemies.Status.StunAndTrackingLost", TD_TEST_FLAGS)
bool FTDEnemiesStun::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Target = TDEnemyShipTest_SpawnSquadDummy(W, FVector(6000, 0, 0));
	ATDEnemyShip* F = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::ShardFighter);

	F->SetGunsFiring(true, Target);
	TestTrue(TEXT("guns on"), F->IsPrimaryTriggerHeld());

	F->ApplyStun(3.f);
	TestTrue(TEXT("stunned"), F->IsStunned());
	TestFalse(TEXT("stun stops firing"), F->IsPrimaryTriggerHeld());
	W.Tick();
	TestFalse(TEXT("still not firing while stunned"), F->IsPrimaryTriggerHeld());

	// +25% damage taken while stunned: 8 -> 10.
	UTDStatics::ApplyTDDamage(F, 8.f, ETDDamageKind::Energy, nullptr, nullptr);
	TestEqual(TEXT("stunned take +25%"), F->GetHealth()->GetHull(), 20.f, 0.01f);

	W.Advance(3.1f);
	TestFalse(TEXT("stun wears off"), F->IsStunned());
	TestTrue(TEXT("guns resume"), F->IsPrimaryTriggerHeld());

	// Stun cancels a telegraph.
	ATDEnemyShip* I = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::HookInterceptor, FVector(0, 3000, 0));
	I->TryStartAttack(ETDEnemyAttack::Missile, Target);
	I->ApplyStun(1.f);
	TestFalse(TEXT("stun cancels the telegraph"), I->IsTelegraphing());
	W.Advance(1.f);
	TestEqual(TEXT("cancelled attack never fires"), I->GetFireCount(ETDEnemyAttack::Missile), 0);

	// Suppress fire (gravity well).
	F->SuppressFire(1.f);
	TestFalse(TEXT("suppressed"), F->CanFire());
	W.Advance(1.1f);
	TestTrue(TEXT("suppression ends"), F->CanFire());

	// Tracking lost: the AI drops its target and can't re-acquire it.
	ATDEnemyShip* Hunter = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::ShardFighter, FVector(0, -3000, 0), /*bKeepAI*/ true);
	ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(Hunter->GetController());
	if (!TestNotNull(TEXT("enemy AI possesses spawned ships"), AI)) { return false; }
	W.Advance(1.5f);
	TestTrue(TEXT("AI acquires the squad ship"), AI->GetTarget() == Target);
	Hunter->LoseTracking(2.f);
	TestNull(TEXT("target forgotten"), AI->GetTarget());
	W.Advance(1.5f);
	TestNull(TEXT("can't re-acquire while tracking is lost"), AI->GetTarget());
	W.Advance(2.f);
	TestTrue(TEXT("re-acquires after 2 s"), AI->GetTarget() == Target);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesCarrier, "TriadDrift.Enemies.Carrier.BayLaunchCadence", TD_TEST_FLAGS)
bool FTDEnemiesCarrier::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDEnemyShip* C = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::HiveCarrier);
	C->MaxLaunchedAlive = 0;	// unlimited for counting
	TArray<ATDCarrierBay*> Bays = C->GetBays();
	if (!TestEqual(TEXT("3 bays"), Bays.Num(), 3)) { return false; }
	TestEqual(TEXT("4 turrets"), C->GetTurrets().Num(), 4);
	TestEqual(TEXT("bay hull 300"), Bays[0]->GetHealth()->MaxHull, 300.f);

	ATDCarrierBay* Bay = Bays[0];
	const float First = Bay->GetTimeToNextLaunch();
	const float Telegraph = C->GetSpec().Heavy.TelegraphSeconds;
	W.Advance(First - Telegraph + 0.05f);
	TestTrue(TEXT("bay telegraphs before launching"), Bay->IsLaunchTelegraphing());
	TestEqual(TEXT("nothing launched during the telegraph"), Bay->GetLaunchCount(), 0);
	W.Advance(Telegraph);
	TestEqual(TEXT("first launch"), Bay->GetLaunchCount(), 1);
	TestEqual(TEXT("3 fighters launched"), TDEnemyShipTest_CountShips(W, C), 3);

	W.Advance(11.8f);
	TestEqual(TEXT("not before 12 s"), Bay->GetLaunchCount(), 1);
	W.Advance(0.3f);
	TestEqual(TEXT("second launch 12 s later"), Bay->GetLaunchCount(), 2);

	// Destroying the bay stops its launches; the others keep going.
	Bay->GetHealth()->Kill();
	TestFalse(TEXT("bay dead"), Bay->IsAlive());
	TestEqual(TEXT("2 living bays"), C->GetLivingBayCount(), 2);
	const int32 OtherBefore = Bays[1]->GetLaunchCount();
	W.Advance(25.f);
	TestEqual(TEXT("dead bay never launches again"), Bay->GetLaunchCount(), 2);
	TestTrue(TEXT("other bays keep launching"), Bays[1]->GetLaunchCount() > OtherBefore);

	// Broken doors double the damage taken.
	ATDCarrierBay* B2 = Bays[2];
	UTDStatics::ApplyTDDamage(B2, 50.f, ETDDamageKind::Energy, nullptr, nullptr);
	TestEqual(TEXT("normal bay damage"), B2->GetHealth()->GetHull(), 250.f);
	B2->BreakDoors();
	UTDStatics::ApplyTDDamage(B2, 50.f, ETDDamageKind::Energy, nullptr, nullptr);
	TestEqual(TEXT("broken doors take double"), B2->GetHealth()->GetHull(), 150.f);

	// EMP pauses launches.
	const int32 B1Count = Bays[1]->GetLaunchCount();
	C->ApplyStun(13.f);
	W.Advance(12.5f);
	TestEqual(TEXT("stunned carrier doesn't launch"), Bays[1]->GetLaunchCount(), B1Count);

	// Carrier death takes its parts with it.
	C->GetHealth()->Kill();
	TestTrue(TEXT("carrier dying"), C->IsDying());
	TestEqual(TEXT("bays gone"), C->GetBays().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesDeath, "TriadDrift.Enemies.Ship.DeathTumbleThenDestroy", TD_TEST_FLAGS)
bool FTDEnemiesDeath::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDEnemyShip* G = TDEnemyShipTest_SpawnEnemy(W, TDEnemyIds::WallGunship);
	TWeakObjectPtr<ATDEnemyShip> Weak = G;
	const float Tumble = G->GetSpec().DeathTumbleSeconds;
	G->GetHealth()->Kill();
	TestTrue(TEXT("dying"), G->IsDying());
	TestFalse(TEXT("dying ships can't fire"), G->CanFire());
	W.Advance(Tumble * 0.5f);
	TestTrue(TEXT("still tumbling"), Weak.IsValid());
	W.Advance(Tumble);
	TestFalse(TEXT("exploded and destroyed"), Weak.IsValid() && !Weak->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesWaveIntegration, "TriadDrift.Enemies.Waves.RealShipsClearAWave", TD_TEST_FLAGS)
bool FTDEnemiesWaveIntegration::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Anchor = TDEnemyShipTest_SpawnSquadDummy(W, FVector::ZeroVector);
	ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
	D->SpawnAnchorOverride = Anchor;
	D->StartWave(0);
	W.Advance(D->WarpInSeconds + 2.5f);
	TArray<AActor*> Living = D->GetLivingEnemies();
	TestEqual(TEXT("8 real fighters"), Living.Num(), 8);
	for (AActor* A : Living)
	{
		const ATDEnemyShip* S = Cast<ATDEnemyShip>(A);
		TestTrue(TEXT("default class is the enemy ship, initialised"), S && S->GetEnemyId() == TDEnemyIds::ShardFighter);
		TestTrue(TEXT("AI flies it"), S && Cast<ATDEnemyAIController>(S->GetController()) != nullptr);
	}
	for (AActor* A : Living) { UTDStatics::GetHealth(A)->Kill(); }
	W.Tick();
	TestTrue(TEXT("wave cleared by real kills"), D->GetState() == ETDWaveState::Breather);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesSteer, "TriadDrift.Enemies.AI.SteeringMaths", TD_TEST_FLAGS)
bool FTDEnemiesSteer::RunTest(const FString& Parameters)
{
	const FTransform Ship = FTransform::Identity;	// at origin facing +X
	const FVector2D Right = ATDEnemyAIController::ComputeSteer(Ship, FVector(1000, 1000, 0), 25.f);
	TestTrue(TEXT("target right -> yaw right, full deflection at 45 deg"), FMath::IsNearlyEqual(Right.X, 1.f) && FMath::IsNearlyZero(Right.Y));
	const FVector2D Up = ATDEnemyAIController::ComputeSteer(Ship, FVector(1000, 0, 200), 25.f);
	TestTrue(TEXT("target slightly above -> partial pitch up"), Up.Y > 0.3f && Up.Y < 0.6f && FMath::IsNearlyZero(Up.X));
	const FVector2D Ahead = ATDEnemyAIController::ComputeSteer(Ship, FVector(5000, 0, 0), 25.f);
	TestTrue(TEXT("dead ahead -> no input"), Ahead.IsNearlyZero());
	const FVector2D Behind = ATDEnemyAIController::ComputeSteer(Ship, FVector(-5000, 10, 0), 25.f);
	TestTrue(TEXT("behind -> committed full turn"), FMath::IsNearlyEqual(FMath::Abs(Behind.X), 1.0));
	TestTrue(TEXT("on top of us -> no NaN"), !ATDEnemyAIController::ComputeSteer(Ship, FVector::ZeroVector, 25.f).ContainsNaN());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

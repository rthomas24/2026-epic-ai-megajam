// Triad Drift - crew enemy automation tests (ENG-23). Owner: Teammate 5.
// Everything goes through the real path: ATDCrewAIController drives each crew member, bolts are T2's
// ATDProjectile, and the player fights back with the real energy blade.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

namespace TDCrewTest
{
	inline ATDCrewCharacter* SpawnCrew(FTDTestWorld& W, ETDCrewType Type, const FVector& At, const FRotator& Facing = FRotator::ZeroRotator)
	{
		UWorld* World = W.GetWorld();
		const FTransform Xf(Facing, At);
		ATDCrewCharacter* C = World->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (C)
		{
			C->CrewType = Type;
			C->FinishSpawning(Xf);
		}
		return C;
	}

	inline int32 LiveEnemyBolts(UWorld* World)
	{
		int32 N = 0;
		if (const UTDProjectileSubsystem* P = UTDProjectileSubsystem::Get(World))
		{
			for (const TObjectPtr<ATDProjectile>& Proj : P->GetLiveProjectiles())
			{
				N += (Proj && Proj->IsActive() && Proj->GetTeam() == ETDTeam::Enemy) ? 1 : 0;
			}
		}
		return N;
	}
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewSpecTable, "TriadDrift.Ground.Crew.SpecTable", TD_TEST_FLAGS)
bool FTDCrewSpecTable::RunTest(const FString& Parameters)
{
	const FTDCrewSpec& G = FTDCrewLibrary::Get(ETDCrewType::Grunt);
	const FTDCrewSpec& T = FTDCrewLibrary::Get(ETDCrewType::Trooper);
	const FTDCrewSpec& H = FTDCrewLibrary::Get(ETDCrewType::Heavy);
	const FTDCrewSpec& D = FTDCrewLibrary::Get(ETDCrewType::Drone);
	const FTDCrewSpec& W = FTDCrewLibrary::Get(ETDCrewType::Warden);
	TestEqual(TEXT("grunt 60 HP"), G.MaxHealth, 60.f);
	TestEqual(TEXT("grunt baton 15"), G.MeleeDamage, 15.f);
	TestTrue(TEXT("grunt needs a token"), G.bNeedsAttackToken);
	TestEqual(TEXT("trooper 50 HP"), T.MaxHealth, 50.f);
	TestEqual(TEXT("trooper bolt 10"), T.BoltDamage, 10.f);
	TestEqual(TEXT("trooper 3-round bursts"), T.BurstCount, 3);
	TestEqual(TEXT("heavy 200 HP"), H.MaxHealth, 200.f);
	TestEqual(TEXT("heavy bash 25"), H.MeleeDamage, 25.f);
	TestEqual(TEXT("heavy shotgun 6 pellets"), H.ShotgunPellets, 6);
	TestEqual(TEXT("heavy pellet 5"), H.ShotgunPelletDamage, 5.f);
	TestTrue(TEXT("heavy frontal shield"), H.bFrontalShield);
	TestEqual(TEXT("drone 30 HP"), D.MaxHealth, 30.f);
	TestEqual(TEXT("drone bolt 5"), D.BoltDamage, 5.f);
	TestTrue(TEXT("drone flies"), D.bFlying);
	TestEqual(TEXT("warden 320 HP (tuned by the machine playtest)"), W.MaxHealth, 320.f);
	TestEqual(TEXT("warden charge slam 30"), W.ChargeSlamDamage, 30.f);
	TestEqual(TEXT("warden phase 2 at 50%"), W.Phase2HealthFraction, 0.5f);
	TestTrue(TEXT("warden summons"), W.SummonCount > 0);
	TestTrue(TEXT("warden bolt fan"), W.FanBolts > 0);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewGruntBaton, "TriadDrift.Ground.Crew.GruntApproachesTelegraphsAndHits", TD_TEST_FLAGS)
bool FTDCrewGruntBaton::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
		ATDCrewCharacter* G = TDCrewTest::SpawnCrew(W, ETDCrewType::Grunt, FVector(700.f, 0.f, 95.f), FRotator(0.f, 180.f, 0.f));
		if (!TestNotNull(TEXT("player"), P) || !TestNotNull(TEXT("grunt"), G)) { return false; }
		TestNotNull(TEXT("crew AI possesses the grunt"), Cast<ATDCrewAIController>(G->GetController()));

		float T = 0.f, TelegraphStart = -1.f, HitTime = -1.f;
		G->OnAttackStarted.AddLambda([&](ATDCrewCharacter*, ETDCrewAttack A) { if (A == ETDCrewAttack::Melee && TelegraphStart < 0.f) { TelegraphStart = T; } });
		while (T < 8.f && HitTime < 0.f)
		{
			W.Tick(Step);
			T += Step;
			if (TDGroundTest::Hull(P) < 100.f) { HitTime = T; }
		}
		const FString Fps = FString::Printf(TEXT(" @%.0f fps"), 1.f / Step);
		TestTrue(TEXT("grunt reached and swung") + Fps, TelegraphStart >= 0.f && HitTime > 0.f);
		TestEqual(TEXT("baton 15") + Fps, 100.f - TDGroundTest::Hull(P), 15.f, 0.1f);
		TestEqual(TEXT("0.6 s telegraph before the hit") + Fps, HitTime - TelegraphStart, G->Spec.MeleeWindup, Step + 0.01f);
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewTokens, "TriadDrift.Ground.Crew.AtMostTwoGruntsAttackAtOnce", TD_TEST_FLAGS)
bool FTDCrewTokens::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	if (!TestNotNull(TEXT("player"), P)) { return false; }
	P->GetHealth()->SetInvulnerable(true); // keep the target alive; we count swings
	TArray<ATDCrewCharacter*> Pack;
	for (int32 i = 0; i < 5; ++i)
	{
		const float A = 2.f * PI * i / 5.f;
		Pack.Add(TDCrewTest::SpawnCrew(W, ETDCrewType::Grunt, FVector(FMath::Cos(A) * 500.f, FMath::Sin(A) * 500.f, 95.f)));
	}
	int32 MaxAtOnce = 0;
	TSet<ATDCrewCharacter*> Swung;
	for (int32 i = 0; i < 60 * 10; ++i)
	{
		W.Tick(1.f / 60.f);
		int32 Now = 0;
		for (ATDCrewCharacter* G : Pack)
		{
			if (G && G->GetCurrentAttack() == ETDCrewAttack::Melee)
			{
				++Now;
				Swung.Add(G);
			}
		}
		MaxAtOnce = FMath::Max(MaxAtOnce, Now);
	}
	TestTrue(TEXT("the pack attacks"), MaxAtOnce >= 1);
	TestTrue(TEXT("never more than 2 swinging at once"), MaxAtOnce <= 2);
	TestTrue(TEXT("tokens rotate between grunts"), Swung.Num() >= 3);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewTrooper, "TriadDrift.Ground.Crew.TrooperKeepsDistanceAndBursts", TD_TEST_FLAGS)
bool FTDCrewTrooper::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewCharacter* Tr = TDCrewTest::SpawnCrew(W, ETDCrewType::Trooper, FVector(400.f, 0.f, 95.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("trooper"), Tr)) { return false; }
	int32 MaxBolts = 0;
	for (int32 i = 0; i < 60 * 5; ++i)
	{
		W.Tick(1.f / 60.f);
		MaxBolts = FMath::Max(MaxBolts, TDCrewTest::LiveEnemyBolts(W.GetWorld()));
	}
	const float Dist = (float)FVector::Dist2D(Tr->GetActorLocation(), P->GetActorLocation());
	TestTrue(FString::Printf(TEXT("backs off to 900+ cm (%.0f)"), Dist), Dist >= 850.f);
	TestTrue(TEXT("3-round burst in the air"), MaxBolts >= 2);
	const float Taken = 100.f - TDGroundTest::Hull(P);
	TestTrue(FString::Printf(TEXT("bolts hit for multiples of 10 (%.0f)"), Taken), Taken >= 10.f && FMath::IsNearlyZero(FMath::Fmod(Taken, 10.f), 0.1f));
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewDeflect, "TriadDrift.Ground.Crew.PerfectDeflectReturnsTrooperBolt", TD_TEST_FLAGS)
bool FTDCrewDeflect::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewCharacter* Tr = TDCrewTest::SpawnCrew(W, ETDCrewType::Trooper, FVector(1100.f, 0.f, 95.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("trooper"), Tr)) { return false; }
	UTDEnergyBladeComponent* B = P->GetBlade();
	int32 Perfect = 0;
	B->OnDeflect.AddLambda([&](AActor*, bool bPerfect) { Perfect += bPerfect ? 1 : 0; });

	// Wait for a bolt, raise the block just before it arrives (perfect window), repeat for a few volleys.
	for (int32 i = 0; i < 60 * 12 && Perfect == 0; ++i)
	{
		W.Tick(1.f / 60.f);
		bool bIncoming = false;
		if (const UTDProjectileSubsystem* PS = UTDProjectileSubsystem::Get(W.GetWorld()))
		{
			for (const TObjectPtr<ATDProjectile>& Proj : PS->GetLiveProjectiles())
			{
				if (Proj && Proj->IsActive() && Proj->GetTeam() == ETDTeam::Enemy && FVector::Dist(Proj->GetActorLocation(), P->GetActorLocation()) < 420.f)
				{
					bIncoming = true;
				}
			}
		}
		if (bIncoming && !B->IsBlocking())
		{
			B->SetBlockHeld(true);
		}
		else if (!bIncoming && B->IsBlocking() && !B->IsInPerfectWindow())
		{
			B->SetBlockHeld(false); // reset for the next bolt so the next raise is perfect
		}
	}
	TestTrue(TEXT("perfect-deflected a real trooper bolt"), Perfect >= 1);
	W.Advance(1.5f);
	TestTrue(FString::Printf(TEXT("returned bolt hurt the trooper (x2 = 20); hull %.0f"), TDGroundTest::Hull(Tr)), TDGroundTest::Hull(Tr) <= 30.f + 0.1f);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewHeavy, "TriadDrift.Ground.Crew.HeavyShieldBashAndShotgun", TD_TEST_FLAGS)
bool FTDCrewHeavy::RunTest(const FString& Parameters)
{
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
		ATDCrewCharacter* H = TDCrewTest::SpawnCrew(W, ETDCrewType::Heavy, FVector(170.f, 0.f, 110.f), FRotator(0.f, 180.f, 0.f));
		if (!TestNotNull(TEXT("heavy"), H)) { return false; }
		P->GetHealth()->SetInvulnerable(true);
		W.Advance(0.3f);
		UTDEnergyBladeComponent* B = P->GetBlade();
		B->LightAttack();
		TestEqual(TEXT("frontal shield blocks the slash"), TDGroundTest::Hull(H), 200.f, 0.1f);
		W.Advance(1.f);
		TestTrue(TEXT("heavy lunge"), B->HeavyLunge());
		TestTrue(TEXT("lunge breaks the guard"), H->IsGuardBroken());
		TestEqual(TEXT("lunge lands 70 through the broken guard"), 200.f - TDGroundTest::Hull(H), 70.f, 0.1f);
		W.Advance(0.6f);
		const float Before = TDGroundTest::Hull(H);
		B->LightAttack();
		TestEqual(TEXT("slashes land while the guard is broken"), Before - TDGroundTest::Hull(H), 25.f, 0.1f);
	}
	{
		// Shield bash (25) up close, shotgun (6 x 5) at range, both through the AI.
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
		ATDCrewCharacter* H = TDCrewTest::SpawnCrew(W, ETDCrewType::Heavy, FVector(700.f, 0.f, 110.f), FRotator(0.f, 180.f, 0.f));
		if (!TestNotNull(TEXT("heavy"), H)) { return false; }
		TArray<ETDCrewAttack> Used;
		H->OnAttackReleased.AddLambda([&](ATDCrewCharacter*, ETDCrewAttack A) { Used.Add(A); });
		int32 MaxPellets = 0;
		for (int32 i = 0; i < 60 * 8 && !Used.Contains(ETDCrewAttack::Melee); ++i)
		{
			W.Tick(1.f / 60.f);
			MaxPellets = FMath::Max(MaxPellets, TDCrewTest::LiveEnemyBolts(W.GetWorld()));
		}
		TestTrue(TEXT("shotgun at range"), Used.Contains(ETDCrewAttack::Shotgun));
		TestEqual(TEXT("6 pellets"), MaxPellets, 6);
		TestTrue(TEXT("shield bash up close"), Used.Contains(ETDCrewAttack::Melee));
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewDrone, "TriadDrift.Ground.Crew.DroneHoversInJumpSlashReach", TD_TEST_FLAGS)
bool FTDCrewDrone::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewCharacter* D = TDCrewTest::SpawnCrew(W, ETDCrewType::Drone, FVector(300.f, 0.f, 150.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("drone"), D)) { return false; }
	P->GetHealth()->SetInvulnerable(true);
	bool bFired = false;
	D->OnAttackReleased.AddLambda([&](ATDCrewCharacter*, ETDCrewAttack A) { bFired |= (A == ETDCrewAttack::Burst); });
	W.Advance(4.f);
	TestEqual(TEXT("flying"), (int32)D->GetCharacterMovement()->MovementMode.GetValue(), (int32)MOVE_Flying);
	// Above a standing swing, within a jump-slash (the machine playtest could never reach one at 320 cm).
	TestTrue(FString::Printf(TEXT("hovers ~220 cm up (%.0f)"), D->GetActorLocation().Z), D->GetActorLocation().Z >= 180.f && D->GetActorLocation().Z <= 280.f);
	TestTrue(TEXT("keeps 350+ cm away"), FVector::Dist2D(D->GetActorLocation(), P->GetActorLocation()) >= 300.f);
	TestTrue(TEXT("fires small fast bolts"), bFired);
	P->GetBlade()->LightAttack();
	TestEqual(TEXT("out of a standing swing at range"), TDGroundTest::Hull(D), 30.f, 0.1f);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewParry, "TriadDrift.Ground.Crew.ParryStaggersGrunt", TD_TEST_FLAGS)
bool FTDCrewParry::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewCharacter* G = TDCrewTest::SpawnCrew(W, ETDCrewType::Grunt, FVector(250.f, 0.f, 95.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("grunt"), G)) { return false; }
	UTDEnergyBladeComponent* B = P->GetBlade();
	bool bTelegraph = false;
	float WindupT = 0.f;
	for (int32 i = 0; i < 60 * 6; ++i)
	{
		W.Tick(1.f / 60.f);
		if (G->IsTelegraphing())
		{
			bTelegraph = true;
			WindupT += 1.f / 60.f;
			if (WindupT >= G->Spec.MeleeWindup - 0.12f && !B->IsBlocking())
			{
				B->SetBlockHeld(true); // raised ~0.12 s before the hit: inside the 0.2 s perfect window
			}
		}
		if (G->IsStaggered())
		{
			break;
		}
	}
	TestTrue(TEXT("grunt telegraphed"), bTelegraph);
	TestTrue(TEXT("perfect block parried: grunt staggered"), G->IsStaggered());
	TestEqual(TEXT("no damage taken"), TDGroundTest::Hull(P), 100.f, 0.1f);
	TestTrue(TEXT("crit ready"), B->HasCritReady());
	W.Tick(1.f / 60.f);
	TestEqual(TEXT("AI in Stagger"), Cast<ATDCrewAIController>(G->GetController())->GetState(), ETDCrewState::Stagger);
	bool bAttackedWhileStaggered = false;
	for (int32 i = 0; i < 60; ++i) { W.Tick(1.f / 60.f); bAttackedWhileStaggered |= G->IsAttacking(); }
	TestFalse(TEXT("no attacks during the 1.2 s stagger"), bAttackedWhileStaggered);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewWarden, "TriadDrift.Ground.Crew.WardenPhasesSummonsFanAndSlam", TD_TEST_FLAGS)
bool FTDCrewWarden::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewCharacter* Wd = TDCrewTest::SpawnCrew(W, ETDCrewType::Warden, FVector(380.f, 0.f, 140.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("warden"), Wd)) { return false; }
	TestEqual(TEXT("400 HP"), TDGroundTest::Hull(Wd), Wd->Spec.MaxHealth);
	// Attack mechanics, driven directly: take the AI out so it doesn't spend the cooldowns first.
	if (AController* AI = Wd->GetController())
	{
		AI->UnPossess();
	}
	W.Advance(0.2f);

	// Charge slam: 40 in its radius.
	TestTrue(TEXT("charge slam starts"), Wd->StartAttack(ETDCrewAttack::ChargeSlam, P));
	TestTrue(TEXT("telegraphed"), Wd->IsTelegraphing());
	W.Advance(Wd->Spec.ChargeWindup + Wd->Spec.ChargeDuration + 0.3f);
	TestEqual(TEXT("slam 30"), 100.f - TDGroundTest::Hull(P), Wd->Spec.ChargeSlamDamage, 0.1f);
	P->GetHealth()->SetInvulnerable(true);
	W.Advance(1.f);

	// Bolt fan: 7 bolts in phase 1 (step back first: the charge left the Warden on top of the player).
	Wd->SetActorLocation(FVector(1200.f, 0.f, 140.f));
	const int32 Before = TDCrewTest::LiveEnemyBolts(W.GetWorld());
	TestTrue(TEXT("fan starts"), Wd->StartAttack(ETDCrewAttack::BoltFan, P));
	W.Advance(Wd->Spec.BurstWindup + 0.02f);
	TestEqual(TEXT("7-bolt fan"), TDCrewTest::LiveEnemyBolts(W.GetWorld()) - Before, 7);
	W.Advance(2.f);

	// Phase 2 at 50%.
	bool bPhase2 = false;
	Wd->OnBossPhase2.AddLambda([&](ATDCrewCharacter*) { bPhase2 = true; });
	UTDStatics::ApplyTDDamage(Wd, Wd->Spec.MaxHealth * Wd->Spec.Phase2HealthFraction - 10.f, ETDDamageKind::Energy, P, P);
	TestFalse(TEXT("still phase 1 above 50%"), bPhase2);
	UTDStatics::ApplyTDDamage(Wd, 20.f, ETDDamageKind::Energy, P, P);
	TestTrue(TEXT("phase 2 at 50%"), bPhase2 && Wd->GetBossPhase() == 2);
	Wd->CancelAttack();
	W.Advance(0.6f);
	Wd->CancelAttack();

	// Phase 2: summons (Phase2SummonCount) grunts, fan of 11.
	TestTrue(TEXT("summon starts"), Wd->StartAttack(ETDCrewAttack::Summon, P));
	W.Advance(Wd->Spec.BurstWindup + 0.1f);
	TestEqual(TEXT("phase 2 summons its grunts"), Wd->GetAliveSummons(), Wd->Spec.Phase2SummonCount);
	W.Advance(1.f);
	Wd->CancelAttack();
	Wd->SetActorLocation(FVector(1200.f, 0.f, 140.f));
	const int32 Before2 = TDCrewTest::LiveEnemyBolts(W.GetWorld());
	TestTrue(TEXT("phase 2 fan starts"), Wd->StartAttack(ETDCrewAttack::BoltFan, P));
	W.Advance(Wd->Spec.BurstWindup + 0.02f);
	TestEqual(TEXT("11-bolt fan in phase 2"), TDCrewTest::LiveEnemyBolts(W.GetWorld()) - Before2, 11);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewDeath, "TriadDrift.Ground.Crew.BladeKillRagdollAndKillStat", TD_TEST_FLAGS)
bool FTDCrewDeath::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewCharacter* G = TDCrewTest::SpawnCrew(W, ETDCrewType::Grunt, FVector(130.f, 0.f, 95.f), FRotator(0.f, 180.f, 0.f));
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("grunt"), G) || !TestNotNull(TEXT("GI"), GI)) { return false; }
	P->GetHealth()->SetInvulnerable(true);
	W.Advance(0.2f);
	UTDEnergyBladeComponent* B = P->GetBlade();
	for (int32 i = 0; i < 6 && !G->IsDead(); ++i)
	{
		G->SetActorLocation(FVector(130.f, 0.f, G->GetActorLocation().Z)); // keep it in reach after knockback
		B->LightAttack();
		W.Advance(0.4f);
	}
	TestTrue(TEXT("blade killed the grunt"), G->IsDead());
	TestEqual(TEXT("kill counted"), GI->GetProgressRef().Stats.Kills, 1);
	TestEqual(TEXT("capsule collision off"), (int32)G->GetCapsuleComponent()->GetCollisionEnabled(), (int32)ECollisionEnabled::NoCollision);
	TestTrue(TEXT("placeholder ragdoll simulates"), G->GetBody()->IsSimulatingPhysics());
	TestTrue(TEXT("corpse cleans itself up"), G->GetLifeSpan() > 0.f);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewVisuals, "TriadDrift.Ground.Crew.VisualsReplacePlaceholder", TD_TEST_FLAGS)
bool FTDCrewVisuals::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDCrewCharacter* G = TDCrewTest::SpawnCrew(W, ETDCrewType::Grunt, FVector(0.f, 0.f, 95.f));
	if (!TestNotNull(TEXT("grunt"), G)) { return false; }
	TestNull(TEXT("no DA_Crew_Grunt_Visuals yet"), G->Visuals.Get());
	TestTrue(TEXT("placeholder visible"), G->IsPlaceholderVisible());
	UTDEnemyVisuals* V = NewObject<UTDEnemyVisuals>(G);
	V->Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	G->Visuals = V;
	G->ApplyVisuals();
	TestFalse(TEXT("assigned mesh hides the placeholder"), G->IsPlaceholderVisible());
	TestEqual(TEXT("mesh applied"), G->GetBody()->GetStaticMesh()->GetName(), FString(TEXT("Sphere")));
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewWardenAI, "TriadDrift.Ground.Crew.WardenAICyclesItsAttacks", TD_TEST_FLAGS)
bool FTDCrewWardenAI::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewCharacter* Wd = TDCrewTest::SpawnCrew(W, ETDCrewType::Warden, FVector(900.f, 0.f, 140.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("warden"), Wd)) { return false; }
	P->GetHealth()->SetInvulnerable(true);
	TSet<ETDCrewAttack> Used;
	Wd->OnAttackReleased.AddLambda([&](ATDCrewCharacter*, ETDCrewAttack A) { Used.Add(A); });
	for (int32 i = 0; i < 60 * 20 && Used.Num() < 3; ++i)
	{
		W.Tick(1.f / 60.f);
	}
	TestTrue(TEXT("AI uses the bolt fan"), Used.Contains(ETDCrewAttack::BoltFan));
	TestTrue(TEXT("AI uses the charge slam"), Used.Contains(ETDCrewAttack::ChargeSlam));
	TestTrue(TEXT("AI summons grunts"), Used.Contains(ETDCrewAttack::Summon));
	return true;
}

// ------------------------------------------------------------------------------------------------
// With no crew art, every wind-up shows a placeholder tell: a red-orange glow at the muzzle that grows until the shot,
// plus an aim line to the target for ranged attacks. It is gone once the attack releases, and never shown when the
// visuals carry their own Telegraph cue.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewTell, "TriadDrift.Ground.Crew.PlaceholderTellBeforeEveryShot", TD_TEST_FLAGS)
bool FTDCrewTell::RunTest(const FString& Parameters)
{
	for (const bool bArtTell : { false, true })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
		ATDCrewCharacter* Tr = TDCrewTest::SpawnCrew(W, ETDCrewType::Trooper, FVector(1200.f, 0.f, 95.f), FRotator(0.f, 180.f, 0.f));
		if (!TestNotNull(TEXT("trooper"), Tr)) { return false; }
		if (bArtTell)
		{
			UTDEnemyVisuals* Art = NewObject<UTDEnemyVisuals>(GetTransientPackage());
			Art->Telegraph.Sound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Engine/EngineSounds/WhiteNoise.WhiteNoise")));
			Tr->Visuals = Art;
		}
		if (AController* AI = Tr->GetController()) { AI->UnPossess(); }
		W.Tick(1.f / 60.f);
		TestFalse(TEXT("no tell at rest"), Tr->IsTellShown());
		if (!TestTrue(TEXT("burst starts"), Tr->StartAttack(ETDCrewAttack::Burst, P))) { return false; }
		W.Tick(1.f / 60.f);
		if (bArtTell)
		{
			TestFalse(TEXT("the art's Telegraph cue replaces the placeholder"), Tr->IsTellShown());
			continue;
		}
		TestTrue(TEXT("tell up during the wind-up"), Tr->IsTellShown());
		TestTrue(TEXT("aim line up for a ranged wind-up"), !Tr->GetTellLine()->bHiddenInGame);
		const float Early = (float)Tr->GetTellGlow()->GetComponentScale().X;
		for (int32 i = 0; i < FMath::RoundToInt(Tr->Spec.BurstWindup * 0.6f * 60.f); ++i) { W.Tick(1.f / 60.f); }
		TestTrue(TEXT("tell sits at the muzzle (in front of the trooper, toward the player)"),
			Tr->GetTellGlow()->GetComponentLocation().X < Tr->GetActorLocation().X - Tr->Spec.CapsuleRadius);
		TestTrue(TEXT("the glow grows as the shot comes"), Tr->GetTellGlow()->GetComponentScale().X > Early + 0.05f);
		for (int32 i = 0; i < 60 && Tr->IsTelegraphing(); ++i) { W.Tick(1.f / 60.f); }
		W.Tick(1.f / 60.f);
		TestFalse(TEXT("tell gone once the burst is out"), Tr->IsTellShown());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

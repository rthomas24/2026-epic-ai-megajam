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
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "SkinnedAssetCompiler.h"
#endif
#include "Animation/AnimSequence.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "AnimationRuntime.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "Engine/SkeletalMeshSocket.h"
#include "UObject/Package.h"
#include "Misc/ScopeExit.h"
#include "Materials/MaterialInterface.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/StaticMeshSocket.h"

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
	int32 MaxBolts = 0, AttackFrames = 0;
	for (int32 i = 0; i < 60 * 5; ++i)
	{
		W.Tick(1.f / 60.f);
		AttackFrames += Tr->IsAttacking() ? 1 : 0;
		MaxBolts = FMath::Max(MaxBolts, TDCrewTest::LiveEnemyBolts(W.GetWorld()));
	}
	const FVector ToP = (P->GetActorLocation() - Tr->GetActorLocation()).GetSafeNormal2D();
	AddInfo(FString::Printf(TEXT("trooper look %s, attacking frames %d, muzzle %s (actor %s), max bolts %d"), *GetNameSafe(Tr->GetMesh()->GetSkeletalMeshAsset()),
		AttackFrames, *Tr->GetMuzzleLocation(ToP).ToCompactString(), *Tr->GetActorLocation().ToCompactString(), MaxBolts));
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
// T7's Verdance playtest (Nov 13): ~32 quick slashes at the Heavy's guard did nothing, nothing taught the lunge, and the
// transfer drained through three retakes. A blocked slash clangs, the first one (and every 5th after) shows the lunge tip,
// and GuardChipsToBreak quick slashes in a row stagger the guard open.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewHeavyGuardChip, "TriadDrift.Ground.Crew.HeavyGuardClangsTeachesTheLungeAndChipsOpen", TD_TEST_FLAGS)
bool FTDCrewHeavyGuardChip::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewCharacter* H = TDCrewTest::SpawnCrew(W, ETDCrewType::Heavy, FVector(170.f, 0.f, 110.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("heavy"), H) || !TestNotNull(TEXT("pilot"), P)) { return false; }
	P->GetHealth()->SetInvulnerable(true);
	W.Advance(0.3f);
	UTDEnergyBladeComponent* B = P->GetBlade();
	int32 ShieldCues = 0;
	H->OnCueRequested.AddLambda([&ShieldCues](ATDCrewCharacter*, FName Cue) { ShieldCues += Cue == TEXT("ShieldHit") ? 1 : 0; });
	B->LightAttack();
	TestEqual(TEXT("blocked"), TDGroundTest::Hull(H), 200.f, 0.1f);
	// The look's ShieldHit cue (the effects builder's slot), or with an empty slot the engineering clang and spark.
	TestTrue(TEXT("the guard clangs"), ShieldCues == 1 && (H->Visuals && !H->Visuals->ShieldHit.IsEmpty() ? true : H->GetSoundEvents().Contains(FName(TEXT("GuardClang")))));
	TestTrue(*FString::Printf(TEXT("the lunge tip shows ('%s')"), *P->GetInteractPrompt().ToString()), P->GetInteractPrompt().ToString().Contains(TEXT("LUNGE")));
	W.Advance(4.5f);
	TestTrue(TEXT("the tip goes after 4 s"), P->GetTip().IsEmpty());
	// Quick slashes, one every 0.45 s (a player's light chain), the hint returning on the 6th.
	int32 Slashes = 1;
	bool bTipAgain = false;
	for (; Slashes < 12 && !H->IsGuardBroken(); ++Slashes)
	{
		B->LightAttack();
		bTipAgain |= Slashes == 5 && P->GetInteractPrompt().ToString().Contains(TEXT("LUNGE"));
		W.Advance(0.45f);
	}
	TestTrue(TEXT("the tip again after 5 more blocked hits"), bTipAgain);
	TestTrue(*FString::Printf(TEXT("quick slashes chip the guard open (after %d)"), Slashes), H->IsGuardBroken() && Slashes <= ATDCrewCharacter::GuardChipsToBreak + 1);
	const float Before = TDGroundTest::Hull(H);
	B->LightAttack();
	TestTrue(TEXT("then a slash lands"), TDGroundTest::Hull(H) < Before);
	return true;
}

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
	// The grunt may wear the trooper's look (when that asset exists): then it falls by animation or ragdoll.
	const bool bFalls = G->GetBody()->IsSimulatingPhysics() || G->IsToppling()
		|| (G->GetMesh()->IsVisible() && G->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationSingleNode);
	TestTrue(TEXT("dies visibly (placeholder ragdoll, a topple or a death animation)"), bFalls);
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
	const UTDEnemyVisuals* TrooperLook = ATDCrewCharacter::FindCrewVisuals(TEXT("Trooper"));
	// The project now ships a grunt look of its own (engineering-made, Gameplay/Characters/NullGrunt): the
	// test must hold with or without that content on disk.
	if (const UTDEnemyVisuals* GruntLook = ATDCrewCharacter::FindCrewVisuals(TEXT("Grunt")))
	{
		TestTrue(TEXT("the grunt wears its own look"), G->Visuals.Get() == GruntLook && !G->IsPlaceholderVisible());
	}
	else if (TrooperLook && !TrooperLook->SkeletalMesh.IsNull())
	{
		TestTrue(TEXT("no grunt look: the grunt wears the trooper's"), G->Visuals.Get() == TrooperLook && G->GetMesh()->IsVisible() && !G->IsPlaceholderVisible());
		TestEqual(TEXT("sized to a grunt"), G->GetLookScale(), 0.96f, 0.001f); // always, own look or borrowed
	}
	else
	{
		TestNull(TEXT("no DA_Crew_Grunt_Visuals yet"), G->Visuals.Get());
		TestTrue(TEXT("placeholder visible"), G->IsPlaceholderVisible());
	}
	UTDEnemyVisuals* V = NewObject<UTDEnemyVisuals>(G);
	V->Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	G->Visuals = V;
	G->ApplyVisuals();
	TestFalse(TEXT("assigned mesh hides the placeholder"), G->IsPlaceholderVisible());
	TestFalse(TEXT("and any skeletal look"), G->GetMesh()->IsVisible());
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

// ------------------------------------------------------------------------------------------------
// Crew looks (R-019): DA_Crew_<Id>_Visuals is found in Presentation (always wins) or anywhere under
// Gameplay/Characters; a skeletal look with no AnimBP is driven by the template mannequin set, faces forward
// (-90 yaw by default), takes its material overrides, and falls by hit direction on death.
namespace TDCrewLookTest
{
	inline UTDEnemyVisuals* MakeAsset(const FString& PackagePath, const FString& Name)
	{
		UPackage* Pkg = CreatePackage(*(PackagePath / Name));
		UTDEnemyVisuals* V = NewObject<UTDEnemyVisuals>(Pkg, *Name, RF_Public | RF_Standalone); // not transient: the registry ignores transient objects
		IAssetRegistry::Get()->AssetCreated(V);
		return V;
	}
	inline void Drop(UTDEnemyVisuals* V)
	{
		if (V)
		{
			IAssetRegistry::Get()->AssetDeleted(V);
			V->ClearFlags(RF_Public | RF_Standalone);
			V->MarkAsGarbage();
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewLookLookup, "TriadDrift.Ground.Crew.LookFromPresentationElseGameplayCharacters", TD_TEST_FLAGS)
bool FTDCrewLookLookup::RunTest(const FString& Parameters)
{
	const FString Stamp = FString::Printf(TEXT("%lld"), FDateTime::UtcNow().GetTicks() % 1000000000LL);
	const FName OnlyEng(*(TEXT("T5Eng") + Stamp));
	const FName Both(*(TEXT("T5Both") + Stamp));
	UTDEnemyVisuals* Eng = TDCrewLookTest::MakeAsset(TEXT("/Game/TriadDrift/Gameplay/Characters/T5Test"), FString::Printf(TEXT("DA_Crew_%s_Visuals"), *OnlyEng.ToString()));
	UTDEnemyVisuals* EngBoth = TDCrewLookTest::MakeAsset(TEXT("/Game/TriadDrift/Gameplay/Characters/T5Test/Deeper"), FString::Printf(TEXT("DA_Crew_%s_Visuals"), *Both.ToString()));
	UTDEnemyVisuals* Nick = TDCrewLookTest::MakeAsset(TEXT("/Game/TriadDrift/Presentation"), FString::Printf(TEXT("DA_Crew_%s_Visuals"), *Both.ToString()));
	ON_SCOPE_EXIT { TDCrewLookTest::Drop(Eng); TDCrewLookTest::Drop(EngBoth); TDCrewLookTest::Drop(Nick); };
	TestTrue(TEXT("an engineering look under Gameplay/Characters is found (any depth)"), ATDCrewCharacter::FindCrewVisuals(OnlyEng) == Eng);
	// Both exist: the loader hands out the Presentation asset with its unset fields filled from the engineering one
	// (a merged transient copy, R-029), never the engineering asset itself.
	const UTDEnemyVisuals* Found = ATDCrewCharacter::FindCrewVisuals(Both);
	TestTrue(TEXT("Presentation wins when both exist"), Found != nullptr && Found != EngBoth
		&& (Found == Nick || Found->GetOutermost() == GetTransientPackage()));
	TestTrue(TEXT("the same object is returned on a second lookup"), ATDCrewCharacter::FindCrewVisuals(Both) == Found);
	TestNull(TEXT("neither: null (the placeholder block)"), ATDCrewCharacter::FindCrewVisuals(FName(*(TEXT("T5None") + Stamp))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewLookSkeletalOnly, "TriadDrift.Ground.Crew.SkeletalLookWithoutAnimBPStillAnimates", TD_TEST_FLAGS)
bool FTDCrewLookSkeletalOnly::RunTest(const FString& Parameters)
{
	USkeletalMesh* Manny = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (!TestNotNull(TEXT("template mannequin mesh"), Manny) || !TestNotNull(TEXT("a material"), Mat)) { return false; }
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	UTDEnemyVisuals* Look = NewObject<UTDEnemyVisuals>(GetTransientPackage());
	Look->SkeletalMesh = Manny;
	Look->MaterialOverrides = { Mat };
	const FTransform Xf(FRotator(0.f, 180.f, 0.f), FVector(600.f, 0.f, 95.f));
	ATDCrewCharacter* Tr = W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Tr->CrewType = ETDCrewType::Trooper;
	Tr->Visuals = Look;
	Tr->FinishSpawning(Xf);
	W.Tick(1.f / 60.f);
	TestTrue(TEXT("the skeletal look is shown, the block hidden"), Tr->GetMesh()->IsVisible() && !Tr->IsPlaceholderVisible());
	TestTrue(TEXT("no AnimBP in the look: the template mannequin set drives it"), Tr->UsesFallbackAnimation());
	UAnimInstance* Anim = Tr->GetMesh()->GetAnimInstance();
	TestTrue(*FString::Printf(TEXT("the C++ loop player drives it (%s)"), *GetNameSafe(Anim ? Anim->GetClass() : nullptr)), Cast<UTDLoopBlendAnimInstance>(Anim) != nullptr);
	TestEqual(TEXT("mannequin-skeleton meshes face forward (-90 yaw)"), (float)Tr->GetMesh()->GetRelativeRotation().Yaw, -90.f, 0.01f);
	TestTrue(TEXT("material overrides apply to the skeletal look"), Tr->GetMesh()->GetMaterial(0) == Mat);
	const FVector Dir = (P->GetActorLocation() - Tr->GetActorLocation()).GetSafeNormal();
	TestTrue(TEXT("no Muzzle socket: shots start in front of the body toward the player (the hand when it is there)"),
		FVector::DotProduct(Tr->GetMuzzleLocation(Dir) - Tr->GetActorLocation(), Dir) >= Tr->Spec.CapsuleRadius);
	// A look with its own yaw keeps it.
	Look->SkeletalMeshYaw = 0.f;
	Tr->ApplyVisuals();
	TestEqual(TEXT("SkeletalMeshYaw overrides the default"), (float)Tr->GetMesh()->GetRelativeRotation().Yaw, 0.f, 0.01f);
	// Death: a fall picked from the hit direction (the mesh plays it instead of freezing).
	FTDDamageInfo Kill;
	Tr->GetHealth()->Kill(P, ETDDamageKind::Melee);
	W.Tick(1.f / 60.f);
	TestTrue(TEXT("dies visibly: the crew set's death animation, else it topples over"),
		Tr->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationSingleNode || Tr->IsToppling() || Tr->IsRagdolling());
	const FVector FeetBefore = Tr->GetMesh()->GetComponentLocation();
	for (int32 i = 0; i < 60; ++i) { W.Tick(1.f / 60.f); }
	const float Tilt = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Tr->GetMesh()->GetUpVector(), FVector::UpVector)));
	TestTrue(*FString::Printf(TEXT("lies down where it stood (tilt %.0f deg, moved %.0f cm)"), Tilt, FVector::Dist(FeetBefore, Tr->GetMesh()->GetComponentLocation())),
		Tr->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationSingleNode || (Tilt > 70.f && FVector::Dist(FeetBefore, Tr->GetMesh()->GetComponentLocation()) < 50.f));
	return true;
}

#if WITH_EDITOR // USkeletalMesh::AddSocket is editor-only: a packaged Development game compiles the tests too
// ------------------------------------------------------------------------------------------------
// A look with a Muzzle socket on the hand (the re-imported NullTrooper): the socket is only where shots start;
// they are aimed from there at the target and land. A socket that sits beside or behind the body falls back to
// the chest-front point. Covered with a runtime socket on the template mannequin, both placements.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewMuzzleSocket, "TriadDrift.Ground.Crew.MuzzleSocketShotsStillLand", TD_TEST_FLAGS)
bool FTDCrewMuzzleSocket::RunTest(const FString& Parameters)
{
	USkeletalMesh* Manny = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (!TestNotNull(TEXT("template mannequin mesh"), Manny)) { return false; }
	for (const FVector& SocketOffset : { FVector(0.f, 20.f, 0.f), FVector(0.f, -60.f, 0.f) }) // along the hand, then well off it
	{
		FTDTestWorld W; // first: making a world collects garbage, which would take the transient look below
		// One socketed copy for the whole session, held strongly and fully compiled before any use: a fresh duplicate
		// per pass was still building on a worker thread when the pass's world was torn down (IsValidIndex(-1) crash).
		static TStrongObjectPtr<USkeletalMesh> SharedCopy;
		if (!SharedCopy.IsValid())
		{
			USkeletalMesh* Dup = DuplicateObject<USkeletalMesh>(Manny, GetTransientPackage());
			USkeletalMeshSocket* NewSocket = NewObject<USkeletalMeshSocket>(Dup);
			NewSocket->SocketName = TEXT("Muzzle");
			NewSocket->BoneName = TEXT("hand_r");
			Dup->AddSocket(NewSocket);
			SharedCopy.Reset(Dup);
		}
		USkeletalMesh* WithSocket = SharedCopy.Get();
#if WITH_EDITOR
		{
			TArray<USkinnedAsset*> Pending{ WithSocket };
			FSkinnedAssetCompilingManager::Get().FinishCompilation(Pending);
		}
#endif
		USkeletalMeshSocket* Socket = WithSocket->FindSocket(TEXT("Muzzle"));
		if (!TestNotNull(TEXT("socket"), Socket)) { return false; }
		Socket->RelativeLocation = SocketOffset;
		TStrongObjectPtr<UTDEnemyVisuals> Look(NewObject<UTDEnemyVisuals>(GetTransientPackage()));
		Look->SkeletalMesh = WithSocket;

		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
		const FTransform Xf(FRotator(0.f, 180.f, 0.f), FVector(900.f, 0.f, 95.f));
		ATDCrewCharacter* Tr = W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Tr->CrewType = ETDCrewType::Trooper;
		Tr->Visuals = Look.Get();
		Tr->FinishSpawning(Xf);
		W.Tick(1.f / 60.f);
		if (!TestTrue(TEXT("the socketed look is on"), Tr->GetMesh()->IsVisible() && Tr->GetMesh()->DoesSocketExist(TEXT("Muzzle")))) { return false; }
		const FVector ToPlayer = (P->GetActorLocation() - Tr->GetActorLocation()).GetSafeNormal2D();
		const FVector Start = Tr->GetMuzzleLocation(ToPlayer);
		TestTrue(*FString::Printf(TEXT("socket %s: shots start in front of the trooper toward the player"), *SocketOffset.ToCompactString()),
			FVector::DotProduct(Start - Tr->GetActorLocation(), ToPlayer) >= Tr->Spec.CapsuleRadius);
		int32 MaxBolts = 0;
		for (int32 i = 0; i < 60 * 5; ++i)
		{
			W.Tick(1.f / 60.f);
			MaxBolts = FMath::Max(MaxBolts, TDCrewTest::LiveEnemyBolts(W.GetWorld()));
		}
		const float Taken = 100.f - TDGroundTest::Hull(P);
		TestTrue(*FString::Printf(TEXT("socket %s: bursts fly (%d in the air)"), *SocketOffset.ToCompactString(), MaxBolts), MaxBolts >= 2);
		TestTrue(*FString::Printf(TEXT("socket %s: bolts land, 10 each (%.0f)"), *SocketOffset.ToCompactString(), Taken), Taken >= 10.f && FMath::IsNearlyZero(FMath::Fmod(Taken, 10.f), 0.1f));
	}
	return true;
}

#endif // WITH_EDITOR
// ------------------------------------------------------------------------------------------------
// Playtest 17e0437: a live grunt lay collapsed (every bone at the floor) while tests passed, because they checked an
// animation mode, not the pose. Every crew type with its real look stands: head above the pelvis and above
// 140 cm x look scale over the feet, standing and walking, sampled across a second.
namespace TDCrewUprightTest
{
	inline bool Upright(FAutomationTestBase& T, const ACharacter* C, const FString& What, float MinHead)
	{
		const USkeletalMeshComponent* M = C->GetMesh();
		const float Feet = (float)(C->GetActorLocation().Z - C->GetSimpleCollisionHalfHeight());
		const float Head = (float)M->GetBoneLocation(TEXT("head")).Z - Feet;
		const float Pelvis = (float)M->GetBoneLocation(TEXT("pelvis")).Z - Feet;
		T.AddInfo(FString::Printf(TEXT("%s: head %.0f cm, pelvis %.0f cm"), *What, Head, Pelvis));
		return T.TestTrue(*FString::Printf(TEXT("%s stands: head %.0f cm (min %.0f), pelvis %.0f cm"), *What, Head, MinHead, Pelvis), Head > Pelvis + 30.f && Head > MinHead);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewUpright, "TriadDrift.Ground.Crew.EveryRealLookStandsUpright", TD_TEST_FLAGS)
bool FTDCrewUpright::RunTest(const FString& Parameters)
{
	int32 Checked = 0;
	for (const ETDCrewType Type : { ETDCrewType::Trooper, ETDCrewType::Grunt, ETDCrewType::Heavy })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		TDGroundTest::SpawnPlayer(W, FVector(-3000.f, 0.f, 95.f)); // far: no fight, just standing and walking
		ATDCrewCharacter* C = TDCrewTest::SpawnCrew(W, Type, FVector(0.f, 0.f, 120.f));
		const FString Name = UEnum::GetValueAsString(Type);
		if (!C->GetMesh()->IsVisible() || !C->GetMesh()->GetSkeletalMeshAsset())
		{
			AddInfo(FString::Printf(TEXT("%s: no skeletal look on this branch (placeholder block)"), *Name));
			continue;
		}
		++Checked;
		C->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones; // nothing renders in tests
		if (AController* AI = C->GetController()) { AI->UnPossess(); }
		const float MinHead = 140.f * C->GetLookScale();
		for (int32 i = 0; i < 60; ++i) { W.Tick(1.f / 60.f); } // the Lead's rule: judged after 1 s
		for (int32 i = 0; i < 60; ++i)
		{
			W.Tick(1.f / 60.f);
			if (i % 20 == 19) { TDCrewUprightTest::Upright(*this, C, Name + TEXT(" standing"), MinHead); }
		}
		for (int32 i = 0; i < 60; ++i)
		{
			C->AddMovementInput(FVector::ForwardVector, 1.f);
			W.Tick(1.f / 60.f);
			if (i % 20 == 19) { TDCrewUprightTest::Upright(*this, C, Name + TEXT(" walking"), MinHead); }
		}
		if (C->IsLookPoseFrozen())
		{
			AddWarning(FString::Printf(TEXT("%s: %s collapses under the mannequin animations - shown standing in its reference pose (asset needs a re-import)"),
				*Name, *GetNameSafe(C->GetMesh()->GetSkeletalMeshAsset())));
		}
		C->StartAttack(Type == ETDCrewType::Grunt ? ETDCrewAttack::Melee : ETDCrewAttack::Burst, nullptr);
		for (int32 i = 0; i < 30; ++i) { W.Tick(1.f / 60.f); }
		TDCrewUprightTest::Upright(*this, C, Name + TEXT(" attacking"), MinHead * 0.85f);
	}
	if (Checked == 0) { AddInfo(TEXT("no real crew looks on this branch: covered by SkeletalLookWithoutAnimBPStillAnimates")); }
	return true;
}

// A killed crew member with a real look lies where it fell for its corpse time (live Nov 13: corpses vanished at once).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewCorpseStays, "TriadDrift.Ground.Crew.CorpseLiesWhereItFell", TD_TEST_FLAGS)
bool FTDCrewCorpseStays::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-800.f, 0.f, 95.f));
	ATDCrewCharacter* C = TDCrewTest::SpawnCrew(W, ETDCrewType::Heavy, FVector(0.f, 0.f, 120.f));
	TWeakObjectPtr<ATDCrewCharacter> Weak(C);
	for (int32 i = 0; i < 30; ++i) { W.Tick(1.f / 60.f); }
	const FVector At = C->GetActorLocation();
	C->GetHealth()->Kill(P, ETDDamageKind::Environmental);
	for (int32 i = 0; i < 60; ++i)
	{
		W.Tick(1.f / 60.f);
		if (i % 10 == 9)
		{
			const ATDCrewCharacter* Cur = Weak.Get();
			AddInfo(FString::Printf(TEXT("t=%.2f valid %d destroying %d at %s mesh visible %d mesh at %s toppling %d"), (i + 1) / 60.f, Cur != nullptr, Cur ? Cur->IsActorBeingDestroyed() : -1,
				Cur ? *Cur->GetActorLocation().ToCompactString() : TEXT("-"), Cur ? Cur->GetMesh()->IsVisible() : -1, Cur ? *Cur->GetMesh()->GetComponentLocation().ToCompactString() : TEXT("-"), Cur ? Cur->IsToppling() : -1));
		}
	}
	TestTrue(TEXT("the corpse is still there after 1 s"), Weak.IsValid() && !Weak->IsActorBeingDestroyed());
	if (Weak.IsValid())
	{
		TestTrue(TEXT("where it fell"), FVector::Dist2D(Weak->GetMesh()->GetComponentLocation(), At) < 150.f);
	}
	return true;
}

// The animation builder (Nov 13, crew_deaths_as_played): deaths held the last frame of a 1.1 s clip that is only the start
// of a fall (a Grunt crouched with the pelvis 54 cm up, the Warden squatting at 108) and slid 1.5 m off the capsule. Now
// an in-place clip, then the ragdoll (or a topple): every body ends on the ground, where it fell.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewDeathsLand, "TriadDrift.Ground.Crew.DeathsEndOnTheGroundWhereTheyFell", TD_TEST_FLAGS)
bool FTDCrewDeathsLand::RunTest(const FString& Parameters)
{
	for (const ETDCrewType Type : { ETDCrewType::Grunt, ETDCrewType::Trooper, ETDCrewType::Heavy, ETDCrewType::Warden })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-900.f, 0.f, 95.f));
		ATDCrewCharacter* C = TDCrewTest::SpawnCrew(W, Type, FVector(0.f, 0.f, 140.f));
		if (!TestNotNull(TEXT("crew"), C)) { return false; }
		for (int32 i = 0; i < 30; ++i) { W.Tick(1.f / 60.f); }
		USkeletalMeshComponent* M = C->GetMesh();
		const FName TypeName = FTDCrewLibrary::TypeName(Type);
		if (!M || !M->IsVisible() || !M->GetSkeletalMeshAsset())
		{
			AddInfo(FString::Printf(TEXT("%s: no skeletal look in this checkout (the block tumbles)"), *TypeName.ToString()));
			continue;
		}
		const FVector At = C->GetActorLocation();
		const FName StandPelvis = M->GetBoneIndex(TEXT("pelvis")) != INDEX_NONE ? FName(TEXT("pelvis")) : M->GetBoneName(FMath::Min(1, M->GetNumBones() - 1));
		const float Standing = (float)M->GetBoneLocation(StandPelvis).Z;
		const UAnimSequenceBase* Clip = C->ChooseDeathClip(FVector(1.f, 0.f, 0.f));
		C->GetHealth()->Kill(P, ETDDamageKind::Melee);
		// Sampled after the chosen fall has played out (a ground fall is 2.57 s; a start clip hands off to the ragdoll at 0.5 s).
		const float Wait = (Clip ? FMath::Max(Clip->GetPlayLength(), ATDCrewCharacter::DeathClipSeconds) : 0.f) + 1.f;
		for (float T = 0.f; T < Wait; T += 1.f / 60.f) { W.Tick(1.f / 60.f); }
		if (!TestTrue(*FString::Printf(TEXT("%s: the corpse stays"), *TypeName.ToString()), IsValid(C) && !C->IsActorBeingDestroyed())) { continue; }
		const FName Pelvis = M->GetBoneIndex(TEXT("pelvis")) != INDEX_NONE ? FName(TEXT("pelvis")) : M->GetBoneName(FMath::Min(1, M->GetNumBones() - 1));
		const float PelvisZ = (float)M->GetBoneLocation(Pelvis).Z;
		AddInfo(FString::Printf(TEXT("%s: clip %s, ragdoll %d, toppled %d, pelvis %.0f cm up (standing %.0f), %.0f cm from where it stood"), *TypeName.ToString(), *GetNameSafe(Clip),
			C->IsRagdolling(), C->IsToppling(), PelvisZ, Standing, FVector::Dist2D(M->GetBoneLocation(Pelvis), At)));
		// Lying down: the pelvis well under its standing height (a 260 cm Warden on its back shield lies higher than a pilot).
		TestTrue(*FString::Printf(TEXT("%s: on the ground (pelvis %.0f cm up, standing %.0f)"), *TypeName.ToString(), PelvisZ, Standing), PelvisZ < FMath::Max(45.f, Standing * 0.55f));
		TestTrue(*FString::Printf(TEXT("%s: where it fell"), *TypeName.ToString()), FVector::Dist2D(M->GetBoneLocation(Pelvis), At) < 200.f);
	}
	return true;
}

// The animation builder (Nov 14): none of the per-type action clips played; every type ran the template's one clip per
// action. Each type's own move plays now (A_Crew_<Type>_<Action>, on its own skeleton), else today's clip.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewTypeClips, "TriadDrift.Ground.Crew.EachTypePlaysItsOwnActionClips", TD_TEST_FLAGS)
bool FTDCrewTypeClips::RunTest(const FString& Parameters)
{
	struct FCase { ETDCrewType Type; ETDCrewAttack Attack; const TCHAR* Want; };
	const FCase Cases[] = {
		{ ETDCrewType::Heavy, ETDCrewAttack::Melee, TEXT("A_Crew_Heavy_ShieldBash") },
		{ ETDCrewType::Trooper, ETDCrewAttack::Burst, TEXT("A_Crew_Trooper_Fire_Burst") },
		{ ETDCrewType::Grunt, ETDCrewAttack::Melee, TEXT("A_Crew_Grunt_Melee_02") },
		{ ETDCrewType::Warden, ETDCrewAttack::BoltFan, TEXT("A_Crew_Warden_Fire_01") },
		{ ETDCrewType::Warden, ETDCrewAttack::Summon, TEXT("A_Crew_Warden_Summon_01") },
	};
	for (const FCase& C : Cases)
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-150.f, 0.f, 95.f));
		ATDCrewCharacter* Crew = TDCrewTest::SpawnCrew(W, C.Type, FVector(0.f, 0.f, 140.f), FRotator(0.f, 180.f, 0.f));
		if (!TestNotNull(TEXT("crew"), Crew) || !P) { return false; }
		W.Advance(0.5f);
		if (!Crew->TypeClip(TEXT("Death_Back")) && !Crew->UsesFallbackAnimation())
		{
			AddInfo(FString::Printf(TEXT("%s: no skeletal look with the loop player here"), *FTDCrewLibrary::TypeName(C.Type).ToString()));
			continue;
		}
		const bool bClip = FPackageName::DoesPackageExist(FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/%s"), C.Want));
		Crew->SetOpeningGrace(0.f);
		TestTrue(*FString::Printf(TEXT("%s starts"), C.Want), Crew->StartAttack(C.Attack, P));
		if (bClip)
		{
			TestEqual(*FString::Printf(TEXT("its own clip (%s)"), *Crew->GetLastTypeClip().ToString()), Crew->GetLastTypeClip(), FName(C.Want));
		}
		else
		{
			AddInfo(FString::Printf(TEXT("%s not in this checkout: today's clip plays"), C.Want));
		}
	}
	return true;
}

// ENG-45: the New Game difficulty (T4's presets) scales ground combat like space: crew health (the pilot's damage out),
// every crew hit (damage in), and the size of every on-foot fight; a lone boss stays alone.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewDifficulty, "TriadDrift.Ground.Crew.DifficultyScalesGroundCombat", TD_TEST_FLAGS)
bool FTDCrewDifficulty::RunTest(const FString& Parameters)
{
	const ETDDifficulty Was = UTDDifficultySettings::GetCurrent();
	for (const ETDDifficulty D : { ETDDifficulty::Story, ETDDifficulty::Standard, ETDDifficulty::Ace })
	{
		UTDDifficultySettings::SetCurrent(D);
		const FTDDifficultyScale Scale = UTDDifficultySettings::GetScale();
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-2000.f, 0.f, 95.f));
		ATDCrewCharacter* G = TDCrewTest::SpawnCrew(W, ETDCrewType::Grunt, FVector(0.f, 0.f, 120.f));
		if (!TestNotNull(TEXT("grunt"), G) || !P) { UTDDifficultySettings::SetCurrent(Was); return false; }
		const FTDCrewSpec& Base = FTDCrewLibrary::Get(ETDCrewType::Grunt);
		const UEnum* E = StaticEnum<ETDDifficulty>();
		const FString Name = E ? E->GetNameStringByValue((int64)D) : TEXT("?");
		TestEqual(*FString::Printf(TEXT("%s: crew health"), *Name), G->GetHealth()->MaxHull, FMath::Max(1.f, Base.MaxHealth * Scale.EnemyHull), 0.01f);
		TestEqual(*FString::Printf(TEXT("%s: crew damage"), *Name), G->Spec.MeleeDamage, Base.MeleeDamage * Scale.EnemyDamage, 0.01f);
		// A three-crew fight and a one-boss fight.
		ATDEncounterVolume* Fight = W.Spawn<ATDEncounterVolume>(FVector(0.f, 3000.f, 150.f));
		ATDEncounterVolume* Boss = W.Spawn<ATDEncounterVolume>(FVector(0.f, -3000.f, 150.f));
		if (!Fight || !Boss) { UTDDifficultySettings::SetCurrent(Was); return false; }
		Fight->bTriggerOnOverlap = false; Boss->bTriggerOnOverlap = false;
		Fight->CrewMix = { ETDCrewType::Grunt }; Fight->SpawnPoints = { FVector(0.f, 0.f, 0.f), FVector(300.f, 0.f, 0.f), FVector(-300.f, 0.f, 0.f) };
		Boss->CrewMix = { ETDCrewType::Grunt }; Boss->SpawnPoints = { FVector::ZeroVector };
		Fight->Activate(); Boss->Activate();
		W.Advance(0.2f);
		TestEqual(*FString::Printf(TEXT("%s: fight size"), *Name), Fight->GetSpawned().Num(), UTDDifficultySettings::ScaleCount(3, Scale.WaveSize));
		TestEqual(*FString::Printf(TEXT("%s: a lone one stays alone"), *Name), Boss->GetSpawned().Num(), 1);
	}
	UTDDifficultySettings::SetCurrent(Was);
	return true;
}

// The animation builder's contact checks (Nov 14): a melee hit lands at the clip's contact time, a fixed time after the
// wind-up starts, swing after swing (the Grunt's varied 0.48-0.57 s against 0.45). Damage instant minus attack start.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewContactTiming, "TriadDrift.Ground.Crew.MeleeLandsAtAFixedTimeAfterTheWindupStarts", TD_TEST_FLAGS)
bool FTDCrewContactTiming::RunTest(const FString& Parameters)
{
	for (const ETDCrewType Type : { ETDCrewType::Grunt, ETDCrewType::Heavy })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(0.f, 0.f, 95.f));
		ATDCrewCharacter* C = TDCrewTest::SpawnCrew(W, Type, FVector(150.f, 0.f, 120.f), FRotator(0.f, 180.f, 0.f));
		if (!TestNotNull(TEXT("crew"), C) || !P) { return false; }
		P->GetHealth()->SetInvulnerable(true);
		C->SetOpeningGrace(0.f);
		double Started = -1.0;
		TArray<double> Gaps;
		C->OnAttackStarted.AddLambda([&Started, &W](ATDCrewCharacter*, ETDCrewAttack A) { if (A == ETDCrewAttack::Melee) { Started = W.GetWorld()->GetTimeSeconds(); } });
		for (int32 i = 0; i < 60 * 12 && Gaps.Num() < 4; ++i)
		{
			W.Tick(1.f / 60.f);
			if (C->GetLastMeleeLandedAt() > 0.0 && Started >= 0.0)
			{
				Gaps.Add(C->GetLastMeleeLandedAt() - Started);
				Started = -1.0;
				C->ClearLastMeleeLanded();
			}
		}
		const FString Name = FTDCrewLibrary::TypeName(Type).ToString();
		FString List;
		for (double G : Gaps) { List += FString::Printf(TEXT("%.3f "), G); }
		AddInfo(FString::Printf(TEXT("%s: wind-up %.2f s, hits after %s"), *Name, C->Spec.MeleeWindup, *List));
		TestTrue(*FString::Printf(TEXT("%s swung"), *Name), Gaps.Num() >= 2);
		for (double G : Gaps)
		{
			TestEqual(*FString::Printf(TEXT("%s hit lands at the wind-up's end"), *Name), (float)G, C->Spec.MeleeWindup, 1.f / 60.f + 0.001f);
		}
	}
	return true;
}

// Every per-type clip the crew code looks up resolves on that type's own skeleton (Nov 14: five Warden clips were on
// SKEL_NullCrew and fell back to the template silently). A clip that does not exist yet is fine; one on the wrong rig fails.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewClipsOnOwnRig, "TriadDrift.Ground.Crew.EveryLookedUpClipIsOnItsTypesOwnSkeleton", TD_TEST_FLAGS)
bool FTDCrewClipsOnOwnRig::RunTest(const FString& Parameters)
{
	struct FUse { ETDCrewType Type; TArray<const TCHAR*> Actions; };
	const TArray<FUse> Uses = {
		{ ETDCrewType::Grunt, { TEXT("Melee_02"), TEXT("Melee_03"), TEXT("HitReact_01"), TEXT("Charge"), TEXT("Death_Front"), TEXT("Death_Back"), TEXT("Death_Left"), TEXT("Death_Right") } },
		{ ETDCrewType::Trooper, { TEXT("Fire_Burst"), TEXT("HitReact_Front"), TEXT("HitReact_Back"), TEXT("Idle_Aim"), TEXT("Death_Front"), TEXT("Death_Back"), TEXT("Death_Left"), TEXT("Death_Right") } },
		{ ETDCrewType::Heavy, { TEXT("ShieldBash"), TEXT("Stagger"), TEXT("Idle_Guard"), TEXT("Walk_Fwd"), TEXT("Death_Back"), TEXT("Death_Left"), TEXT("Death_Right") } },
		{ ETDCrewType::Warden, { TEXT("Fire_01"), TEXT("Slam"), TEXT("GroundSlam"), TEXT("Summon_01"), TEXT("Stagger"), TEXT("Guard"), TEXT("Walk"), TEXT("Death_Back") } },
	};
	for (const FUse& U : Uses)
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDCrewCharacter* C = TDCrewTest::SpawnCrew(W, U.Type, FVector(0.f, 0.f, 140.f));
		if (!TestNotNull(TEXT("crew"), C)) { return false; }
		const FString Type = FTDCrewLibrary::TypeName(U.Type).ToString();
		if (!C->GetMesh() || !C->GetMesh()->GetSkeletalMeshAsset())
		{
			AddInfo(FString::Printf(TEXT("%s: no skeletal look here"), *Type));
			continue;
		}
		int32 Present = 0;
		for (const TCHAR* Action : U.Actions)
		{
			const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/A_Crew_%s_%s"), *Type, Action);
			if (!FPackageName::DoesPackageExist(Package)) { continue; }
			++Present;
			TestNotNull(*FString::Printf(TEXT("A_Crew_%s_%s resolves on the %s rig"), *Type, Action, *Type), C->TypeClip(Action));
		}
		AddInfo(FString::Printf(TEXT("%s: %d looked-up clips present"), *Type, Present));
	}
	return true;
}

// The animation builder's close-camera check (Nov 14): a front hit on a firing Trooper showed nothing and it kept
// firing. A hit from either side stops the burst, plays that side's react and holds it still for the react's length.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewTrooperReact, "TriadDrift.Ground.Crew.TrooperHitReactInterruptsTheBurst", TD_TEST_FLAGS)
bool FTDCrewTrooperReact::RunTest(const FString& Parameters)
{
	for (const bool bBehind : { false, true })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-600.f, 0.f, 95.f));
		ATDCrewCharacter* C = TDCrewTest::SpawnCrew(W, ETDCrewType::Trooper, FVector(0.f, 0.f, 140.f), FRotator(0.f, 180.f, 0.f));
		if (!TestNotNull(TEXT("crew"), C) || !P) { return false; }
		P->GetHealth()->SetInvulnerable(true);
		W.Advance(0.3f);
		C->SetOpeningGrace(0.f);
		TestTrue(TEXT("burst starts"), C->StartAttack(ETDCrewAttack::Burst, P));
		W.Advance(0.1f);
		// Travelling into its face (it faces the pilot) or into its back.
		const FVector Dir = bBehind ? C->GetActorForwardVector() : -C->GetActorForwardVector();
		UTDStatics::ApplyTDDamage(C, 5.f, ETDDamageKind::Melee, P, P, C->GetActorLocation() - Dir * 40.f, Dir);
		const TCHAR* Side = bBehind ? TEXT("back") : TEXT("front");
		TestFalse(*FString::Printf(TEXT("%s hit: the burst stops"), Side), C->IsAttacking());
		TestTrue(*FString::Printf(TEXT("%s hit: reacting"), Side), C->IsReacting());
		TestFalse(*FString::Printf(TEXT("%s hit: no attack during the react"), Side), C->StartAttack(ETDCrewAttack::Burst, P));
		const FString Want = FString::Printf(TEXT("A_Crew_Trooper_HitReact_%s"), bBehind ? TEXT("Back") : TEXT("Front"));
		if (C->TypeClip(bBehind ? TEXT("HitReact_Back") : TEXT("HitReact_Front")))
		{
			TestEqual(*FString::Printf(TEXT("%s hit plays %s"), Side, *Want), C->GetLastTypeClip(), FName(*Want));
		}
		else
		{
			AddInfo(FString::Printf(TEXT("%s not on this checkout's Trooper"), *Want));
		}
		W.Advance(2.5f);
		TestFalse(*FString::Printf(TEXT("%s hit: the react ends"), Side), C->IsReacting());
	}
	return true;
}

// Lead (Nov 14): a heavy blade hit staggers the Warden for its Stagger clip's length, then not again for the cooldown;
// light hits never do.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewWardenStagger, "TriadDrift.Ground.Crew.WardenStaggersOnAHeavyHitWithACooldown", TD_TEST_FLAGS)
bool FTDCrewWardenStagger::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-300.f, 0.f, 95.f));
	ATDCrewCharacter* C = TDCrewTest::SpawnCrew(W, ETDCrewType::Warden, FVector(0.f, 0.f, 140.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("warden"), C) || !P) { return false; }
	P->GetHealth()->SetInvulnerable(true);
	W.Advance(0.3f);
	const FVector Dir = -C->GetActorForwardVector();
	auto Hit = [&](float Amount) { UTDStatics::ApplyTDDamage(C, Amount, ETDDamageKind::Melee, P, P, C->GetActorLocation() - Dir * 60.f, Dir); };
	Hit(20.f);
	TestFalse(TEXT("a light hit does not stagger"), C->IsStaggered());
	Hit(C->WardenHeavyHitDamage + 5.f);
	TestTrue(TEXT("a heavy hit staggers"), C->IsStaggered());
	const UAnimSequenceBase* Clip = C->TypeClip(TEXT("Stagger"));
	const float Want = Clip ? Clip->GetPlayLength() : 1.2f;
	AddInfo(FString::Printf(TEXT("stagger %.2f s (%s)"), Want, Clip ? *Clip->GetName() : TEXT("no clip: 1.2 s")));
	W.Advance(Want - 0.1f);
	TestTrue(TEXT("still staggered near the clip's end"), C->IsStaggered());
	W.Advance(0.25f);
	TestFalse(TEXT("free after the clip"), C->IsStaggered());
	Hit(C->WardenHeavyHitDamage + 5.f);
	TestFalse(TEXT("not again inside the cooldown"), C->IsStaggered());
	W.Advance(C->WardenStaggerCooldown);
	if (!C->IsDead())
	{
		Hit(C->WardenHeavyHitDamage + 5.f);
		TestTrue(TEXT("again after the cooldown"), C->IsStaggered() || C->IsDead());
	}
	return true;
}

// The Rodin drone (Lead, Nov 13): its 79 x 90 x 88 cm look is what the blade and deflected bolts must hit, and a static-mesh
// look's Muzzle socket is where its shots start.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDroneLookAndMuzzle, "TriadDrift.Ground.Crew.DroneCapsuleCoversItsLookAndShootsFromItsMuzzle", TD_TEST_FLAGS)
bool FTDDroneLookAndMuzzle::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("pilot"), P) || !TestNotNull(TEXT("cube"), Cube)) { return false; }
	static TStrongObjectPtr<UStaticMesh> Socketed;
	if (!Socketed.IsValid())
	{
		UStaticMesh* Dup = DuplicateObject<UStaticMesh>(Cube, GetTransientPackage());
		UStaticMeshSocket* S = NewObject<UStaticMeshSocket>(Dup);
		S->SocketName = TEXT("Muzzle");
		S->RelativeLocation = FVector(70.f, 0.f, 10.f); // out of the front, a little up
		Dup->Sockets.Add(S);
		Socketed.Reset(Dup);
	}
	TStrongObjectPtr<UTDEnemyVisuals> Look(NewObject<UTDEnemyVisuals>(GetTransientPackage()));
	Look->Mesh = Socketed.Get();
	const FTransform Xf(FRotator::ZeroRotator, FVector(800.f, 0.f, 300.f));
	ATDCrewCharacter* D = W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	D->CrewType = ETDCrewType::Drone;
	D->Visuals = Look.Get();
	D->FinishSpawning(Xf);
	W.Tick(1.f / 60.f);
	TestTrue(*FString::Printf(TEXT("capsule covers the 79 x 90 x 88 cm look (r %.0f, hh %.0f)"), D->Spec.CapsuleRadius, D->Spec.CapsuleHalfHeight),
		D->Spec.CapsuleRadius >= 44.f && D->Spec.CapsuleHalfHeight >= 43.f);
	const FVector Fwd = D->GetActorForwardVector();
	const FVector Muzzle = D->GetMuzzleLocation(Fwd);
	const UStaticMeshComponent* Body = D->FindComponentByClass<UStaticMeshComponent>();
	const FVector Socket = Body && Body->DoesSocketExist(TEXT("Muzzle")) ? Body->GetSocketLocation(TEXT("Muzzle")) : FVector::ZeroVector;
	TestTrue(*FString::Printf(TEXT("shots start at the look's Muzzle socket (%s vs %s)"), *Muzzle.ToCompactString(), *Socket.ToCompactString()),
		!Socket.IsZero() && Muzzle.Equals(Socket, 2.f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

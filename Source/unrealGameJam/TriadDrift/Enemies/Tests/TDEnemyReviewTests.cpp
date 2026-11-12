// Real-actor tests for the second ENG-13 review (MAJORs 1-4): the signature attacks must actually happen
// against a real player ship, at 30, 60 and 120 fps.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies.Review

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "HAL/IConsoleManager.h"

namespace TDEnemyReview
{
	const float Rates[] = { 30.f, 60.f, 120.f };

	ATDSpacecraft* SpawnPlayer(FTDTestWorld& W, const FVector& Loc, const FRotator& Rot, float Speed)
	{
		ATDSpacecraft* S = W.Spawn<ATDSpacecraft>(Loc, Rot);
		S->SetTeam(ETDTeam::Player);
		if (UTDHealthComponent* H = S->GetHealth())
		{
			H->DefaultTeam = ETDTeam::Player;
			H->InitMaxValues(100000.f, 0.f, true);
		}
		W.SpawnPlayerController(S);
		S->SetSpeedOverride(Speed, 0.f);	// flies straight at cruise
		return S;
	}

	ATDEnemyShip* SpawnEnemy(FTDTestWorld& W, FName Id, const FVector& Loc, const FRotator& Rot)
	{
		ATDEnemyShip* E = W.Spawn<ATDEnemyShip>(Loc, Rot);
		E->InitFromSpec(Id);
		return E;
	}

	/** Review tests run at the defaults: Standard difficulty and the shipped enemy cvars (restored on exit). */
	struct FDefaults
	{
		ETDDifficulty PrevDifficulty;
		TArray<TPair<IConsoleVariable*, FString>> PrevCVars;
		explicit FDefaults(FAutomationTestBase& Test) : PrevDifficulty(UTDDifficultySettings::GetCurrent())
		{
			Test.TestEqual(TEXT("tests start at Standard (no leaked difficulty)"), (int32)PrevDifficulty, (int32)ETDDifficulty::Standard);
			UTDDifficultySettings::SetCurrent(ETDDifficulty::Standard);
			for (const TPair<const TCHAR*, const TCHAR*>& CV : { TPair<const TCHAR*, const TCHAR*>(TEXT("TD.EnemyCrossInFront"), TEXT("5000")), TPair<const TCHAR*, const TCHAR*>(TEXT("TD.EnemyOpeningPass"), TEXT("1")) })
			{
				if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(CV.Key))
				{
					PrevCVars.Emplace(V, V->GetString());
					V->Set(CV.Value);
				}
			}
		}
		~FDefaults()
		{
			UTDDifficultySettings::SetCurrent(PrevDifficulty);
			for (const TPair<IConsoleVariable*, FString>& P : PrevCVars) { P.Key->Set(*P.Value); }
		}
	};

	int32 Shots(const ATDEnemyShip* E)
	{
		const UTDWeaponComponent* Wp = E ? E->GetWeapons() : nullptr;
		return Wp ? Wp->GetSlotState(ETDWeaponSlot::Primary).ShotsFired : 0;
	}
}

// MAJOR 1: a fighter fires at a player who flies straight at it (head-on) and at one crossing its front.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewFighterFires, "TriadDrift.Enemies.Review.FighterFiresHeadOnAndFromTheSide", TD_TEST_FLAGS)
bool FTDReviewFighterFires::RunTest(const FString& Parameters)
{
	using namespace TDEnemyReview;
	FDefaults Guard(*this);
	for (const float Fps : Rates)
	{
		for (int32 Case = 0; Case < 2; ++Case)
		{
			FTDTestWorld W;
			ATDSpacecraft* Player = Case == 0
				? SpawnPlayer(W, FVector(20000.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), 6000.f)	// head-on
				: SpawnPlayer(W, FVector(-6000.f, -14000.f, 0.f), FRotator(0.f, 0.f, 0.f), 6000.f);	// crossing the fighter's front
			ATDEnemyShip* F = SpawnEnemy(W, TDEnemyIds::ShardFighter, FVector::ZeroVector, (Player->GetActorLocation()).Rotation());
			const float Hull0 = Player->GetHealth()->GetHull();
			for (int32 i = 0; i < FMath::RoundToInt(Fps * 6.f); ++i)
			{
				W.Tick(1.f / Fps);
			}
			const TCHAR* Name = Case == 0 ? TEXT("head-on") : TEXT("side");
			TestTrue(*FString::Printf(TEXT("%s @%.0f fps: the fighter fired (%d shots)"), Name, Fps, Shots(F)), Shots(F) > 0);
			// A healthy number of hits, not one: 5 damage per pulse bolt.
			const int32 Hits = FMath::RoundToInt((Hull0 - Player->GetHealth()->GetHull()) / 5.f);
			const int32 MinHits = 2;
			TestTrue(*FString::Printf(TEXT("%s @%.0f fps: and hit at least %d times in 6 s (%d hits of %d shots)"), Name, Fps, MinHits, Hits, Shots(F)), Hits >= MinHits);
		}
	}
	return true;
}

// Lead finding: head-on hits sat on a knife edge. Sweep closing speeds 8000-21 000 cm/s at 30/60/120 fps,
// 8 seeded passes each: at least 6 of 8 first passes land two or more hits (a margin, not one lucky bolt).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewHeadOnSweep, "TriadDrift.Enemies.Review.HeadOnPassLandsHitsAtEveryClosingSpeed", TD_TEST_FLAGS)
bool FTDReviewHeadOnSweep::RunTest(const FString& Parameters)
{
	using namespace TDEnemyReview;
	FDefaults Guard(*this);
	const float PlayerSpeeds[] = { 2000.f, 4000.f, 6000.f, 8000.f, 10000.f, 15000.f };	// + fighter ~6000 = 8000..21 000 closing
	for (const float Fps : Rates)
	{
		for (const float V : PlayerSpeeds)
		{
			int32 GoodPasses = 0;
			FString Detail;
			for (int32 Seed = 1; Seed <= 8; ++Seed)
			{
				FTDTestWorld W;
				ATDSpacecraft* Player = SpawnPlayer(W, FVector(26000.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), V);
				ATDEnemyShip* F = SpawnEnemy(W, TDEnemyIds::ShardFighter, FVector::ZeroVector, FRotator::ZeroRotator);
				F->SetSpawnSeed(1000 * Seed + 17);
				const ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(F->GetController());
				const float Hull0 = Player->GetHealth()->GetHull();
				bool bRan = false;
				for (int32 i = 0; i < FMath::RoundToInt(Fps * 6.f); ++i)
				{
					W.Tick(1.f / Fps);
					bRan |= AI && AI->GetAIState() == ETDEnemyAIState::AttackRun;
					if (bRan && AI && AI->GetAIState() == ETDEnemyAIState::BreakOff) { break; }
				}
				W.Advance(0.4f, 1.f / Fps);	// bolts in flight land
				const float Taken = Hull0 - Player->GetHealth()->GetHull();
				const int32 Hits = FMath::RoundToInt(Taken / 5.f);
				// Bolts only: more damage than the shots fired can deal is a collision (a ram), never a good pass.
				const bool bRammed = Taken > Shots(F) * 5.f + 0.5f;
				TestFalse(*FString::Printf(TEXT("@%.0f fps, player %.0f cm/s, seed %d: no ram (%.0f damage from %d shots)"), Fps, V, Seed, Taken, Shots(F)), bRammed);
				// Cruise against cruise (closing up to ~14 000 cm/s): two or more hits. A boosting ship head-on
				// (16 000-21 000) leaves a ~0.3 s window, one 4/s bolt: at least one hit.
				const int32 Needed = V <= 8000.f ? 2 : 1;
				GoodPasses += (Hits >= Needed && !bRammed) ? 1 : 0;
				Detail += FString::Printf(TEXT(" %d/%d"), Hits, Shots(F));
			}
			AddInfo(FString::Printf(TEXT("head-on @%.0f fps, player %.0f cm/s: hits/shots per seed%s"), Fps, V, *Detail));
			TestTrue(*FString::Printf(TEXT("@%.0f fps, player %.0f cm/s: %d of 8 passes landed enough hits (%s)"), Fps, V, GoodPasses, *Detail), GoodPasses >= 6);
		}
	}
	return true;
}

// MAJOR 2: an interceptor never launches its telegraphed missile on the player's flight line.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewNoPointBlankMissile, "TriadDrift.Enemies.Review.NoPointBlankTelegraphedMissile", TD_TEST_FLAGS)
bool FTDReviewNoPointBlankMissile::RunTest(const FString& Parameters)
{
	using namespace TDEnemyReview;
	for (const float Fps : Rates)
	{
		FTDTestWorld W;
		ATDSpacecraft* Player = SpawnPlayer(W, FVector(24000.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), 6000.f);
		ATDEnemyShip* I = SpawnEnemy(W, TDEnemyIds::HookInterceptor, FVector::ZeroVector, FRotator::ZeroRotator);
		UTDProjectileSubsystem* Proj = UTDProjectileSubsystem::Get(W.GetWorld());
		float MinLaunchDist = TNumericLimits<float>::Max();
		float MinDist = TNumericLimits<float>::Max();
		bool bWasTelegraphing = false;
		int32 MinState = -1;
		float MinT = 0.f;
		for (int32 i = 0; i < FMath::RoundToInt(Fps * 6.f); ++i)
		{
			W.Tick(1.f / Fps);
			const float Dist = FVector::Dist(I->GetActorLocation(), Player->GetActorLocation());
			if (Dist < MinDist)
			{
				MinDist = Dist;
				if (const ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(I->GetController())) { MinState = (int32)AI->GetAIState(); MinT = i / Fps; }
			}
			const bool bTele = I->IsTelegraphing();
			if (bWasTelegraphing && !bTele) { MinLaunchDist = FMath::Min(MinLaunchDist, Dist); }	// launch moment
			bWasTelegraphing = bTele;
		}
		(void)Proj;
		TestTrue(*FString::Printf(TEXT("@%.0f fps: no missile launched inside 5000 cm (closest launch %.0f)"), Fps, MinLaunchDist), MinLaunchDist > 5000.f);
		// No ram: the hulls never touch (collision radii plus a margin for a visible gap).
		const float Contact = Player->GetSimpleCollisionRadius() + I->GetSpec().Radius + 200.f;
		TestTrue(*FString::Printf(TEXT("@%.0f fps: no ram (closest %.0f, contact %.0f, at %.1f s in state %d)"), Fps, MinDist, Contact, MinT, MinState), MinDist > Contact);
	}
	return true;
}

// MAJOR 3: a gunship faces a flanking player and starts its heavy-bolt volley.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewGunshipVolley, "TriadDrift.Enemies.Review.GunshipVolleysAFlankingPlayer", TD_TEST_FLAGS)
bool FTDReviewGunshipVolley::RunTest(const FString& Parameters)
{
	using namespace TDEnemyReview;
	for (const float Fps : Rates)
	{
		FTDTestWorld W;
		// The player strafes sideways across the gunship's front at 10 000 cm (a flanking pass).
		ATDSpacecraft* Player = SpawnPlayer(W, FVector(10000.f, -9000.f, 0.f), FRotator(0.f, 90.f, 0.f), 6000.f);
		ATDEnemyShip* G = SpawnEnemy(W, TDEnemyIds::WallGunship, FVector::ZeroVector, FRotator::ZeroRotator);
		const float Cone = FMath::Cos(FMath::DegreesToRadians(UTDEnemyLibrary::Get(TDEnemyIds::WallGunship).Heavy.ConeHalfAngleDeg));
		bool bVolley = false;
		int32 InCone = 0, Counted = 0;
		for (int32 i = 0; i < FMath::RoundToInt(Fps * 4.f); ++i)
		{
			W.Tick(1.f / Fps);
			bVolley |= G->GetTelegraphAttack() == ETDEnemyAttack::HeavyBolt || G->GetAttackCooldownRemaining(ETDEnemyAttack::HeavyBolt) > 0.f;
			if (i > Fps * 1.f)	// after the first second of turning
			{
				++Counted;
				const FVector To = (Player->GetActorLocation() - G->GetActorLocation()).GetSafeNormal();
				InCone += FVector::DotProduct(To, G->GetActorForwardVector()) >= Cone ? 1 : 0;
			}
		}
		const float Frac = Counted > 0 ? float(InCone) / Counted : 0.f;
		TestTrue(*FString::Printf(TEXT("@%.0f fps: the gunship started a heavy-bolt volley at a flanking player"), Fps), bVolley);
		TestTrue(*FString::Printf(TEXT("@%.0f fps: it faces the player itself (%.0f%% of the time inside the volley cone)"), Fps, Frac * 100.f), Frac >= 0.6f);
		(void)Player;
	}
	return true;
}

// MAJOR 4: catch-up never overrides a Gravity Well (or any other owner's) speed override.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewNoCatchUpInWell, "TriadDrift.Enemies.Review.NoCatchUpOverAGravityWell", TD_TEST_FLAGS)
bool FTDReviewNoCatchUpInWell::RunTest(const FString& Parameters)
{
	using namespace TDEnemyReview;
	for (const float Fps : Rates)
	{
		FTDTestWorld W;
		SpawnPlayer(W, FVector(40000.f, 0.f, 0.f), FRotator::ZeroRotator, 6000.f);	// far: the leash wants catch-up
		ATDEnemyShip* F = SpawnEnemy(W, TDEnemyIds::ShardFighter, FVector::ZeroVector, FRotator::ZeroRotator);
		ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(F->GetController());
		if (!TestNotNull(TEXT("AI"), AI)) { return false; }
		UTDShipMovementComponent* Move = F->GetShipMovement();

		// 1) Caught first: catch-up must not start.
		Move->SetSpeedOverrideFor(TEXT("GravityWell"), 300.f, 0.2f);
		for (int32 i = 0; i < FMath::RoundToInt(Fps * 2.f); ++i) { W.Tick(1.f / Fps); }
		TestFalse(*FString::Printf(TEXT("@%.0f fps: no catch-up while held by the well"), Fps), AI->IsCatchingUp());
		TestEqual(*FString::Printf(TEXT("@%.0f fps: the well's override is the one flown"), Fps), Move->GetActiveSpeedOverrideOwner(), FName(TEXT("GravityWell")));
		TestTrue(*FString::Printf(TEXT("@%.0f fps: crawling (%.0f cm/s)"), Fps, F->GetVelocity().Size()), F->GetVelocity().Size() < 1000.f);

		// 2) Catching up first, then caught: ours is dropped, so releasing the well leaves nothing of ours.
		Move->ClearSpeedOverrideFor(TEXT("GravityWell"));
		for (int32 i = 0; i < FMath::RoundToInt(Fps * 1.f); ++i) { W.Tick(1.f / Fps); }
		Move->SetSpeedOverrideFor(TEXT("GravityWell"), 300.f, 0.2f);
		for (int32 i = 0; i < 3; ++i) { W.Tick(1.f / Fps); }
		TestFalse(*FString::Printf(TEXT("@%.0f fps: catch-up dropped once caught"), Fps), Move->HasSpeedOverrideFor(TEXT("EnemyAI")));
	}
	return true;
}

// Re-check MAJOR: a ship charging a gunship (each ship's cruise and boost) is volleyed, not ignored.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewGunshipVsCharge, "TriadDrift.Enemies.Review.GunshipVolleysAShipChargingIt", TD_TEST_FLAGS)
bool FTDReviewGunshipVsCharge::RunTest(const FString& Parameters)
{
	using namespace TDEnemyReview;
	// Bastion / Rift / Lancer cruise, then boost (cm/s).
	const float Speeds[] = { 5000.f, 6000.f, 7500.f, 10800.f, 13500.f };
	for (const float Fps : Rates)
	{
		for (const float V : Speeds)
		{
			FTDTestWorld W;
			ATDSpacecraft* Player = SpawnPlayer(W, FVector(22000.f, 400.f, 0.f), FRotator(0.f, 180.f, 0.f), V);
			ATDEnemyShip* G = SpawnEnemy(W, TDEnemyIds::WallGunship, FVector::ZeroVector, FRotator::ZeroRotator);
			const float Hull0 = Player->GetHealth()->GetHull();
			bool bVolley = false;
			const int32 Frames = FMath::RoundToInt(Fps * (20000.f / V));	// until it reaches the gunship
			for (int32 i = 0; i < Frames; ++i)
			{
				W.Tick(1.f / Fps);
				bVolley |= G->GetTelegraphAttack() == ETDEnemyAttack::HeavyBolt || G->GetAttackCooldownRemaining(ETDEnemyAttack::HeavyBolt) > 0.f;
			}
			W.Advance(0.5f, 1.f / Fps);
			const float Taken = Hull0 - Player->GetHealth()->GetHull();
			TestTrue(*FString::Printf(TEXT("@%.0f fps, %.0f cm/s: the gunship volleyed the charging ship"), Fps, V), bVolley);
			TestTrue(*FString::Printf(TEXT("@%.0f fps, %.0f cm/s: and a straight charge got hit (%.0f damage)"), Fps, V, Taken), Taken > 0.f);
		}
	}
	return true;
}

// Combat review: a gunship killed after missile 1 of its salvo launches nothing more from its wreck.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewWreckSalvo, "TriadDrift.Enemies.Review.WreckLaunchesNoSalvo", TD_TEST_FLAGS)
bool FTDReviewWreckSalvo::RunTest(const FString& Parameters)
{
	using namespace TDEnemyReview;
	for (const float Fps : Rates)
	{
		FTDTestWorld W;
		ATDSpacecraft* Player = SpawnPlayer(W, FVector(15000.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), 0.f);
		ATDEnemyShip* G = SpawnEnemy(W, TDEnemyIds::WallGunship, FVector::ZeroVector, FRotator::ZeroRotator);
		if (AController* C = G->GetController()) { C->UnPossess(); C->Destroy(); }
		UTDProjectileSubsystem* Proj = UTDProjectileSubsystem::Get(W.GetWorld());
		TSet<uint32> Seen;
		auto CountMissiles = [&]()
		{
			for (ATDProjectile* P : Proj->GetLiveProjectiles())
			{
				if (P && P->IsMissile() && P->GetOriginalShooter() == G) { Seen.Add(P->GetLaunchSerial()); }
			}
		};
		TestTrue(TEXT("salvo started"), G->TryStartAttack(ETDEnemyAttack::MissileSalvo, Player));
		for (int32 i = 0; i < FMath::RoundToInt(Fps * 4.f) && Seen.Num() == 0; ++i) { W.Tick(1.f / Fps); CountMissiles(); }
		TestEqual(*FString::Printf(TEXT("@%.0f fps: missile 1 launched"), Fps), Seen.Num(), 1);
		// The wreck is destroyed and may be garbage collected while we wait: never touch the raw pointer again.
		// CountMissiles only compares G's address with each live missile's shooter, which is safe.
		const TWeakObjectPtr<ATDEnemyShip> GunshipAlive(G);
		G->GetHealth()->Kill();
		for (int32 i = 0; i < FMath::RoundToInt(Fps * 3.f); ++i) { W.Tick(1.f / Fps); if (GunshipAlive.IsValid()) { CountMissiles(); } }
		TestEqual(*FString::Printf(TEXT("@%.0f fps: the wreck launched nothing more"), Fps), Seen.Num(), 1);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

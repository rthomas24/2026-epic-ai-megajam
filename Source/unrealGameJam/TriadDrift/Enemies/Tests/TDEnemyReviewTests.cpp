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
			TestTrue(*FString::Printf(TEXT("%s @%.0f fps: and hit (hull %.0f -> %.0f)"), Name, Fps, Hull0, Player->GetHealth()->GetHull()), Player->GetHealth()->GetHull() < Hull0);
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
		G->GetHealth()->Kill();
		for (int32 i = 0; i < FMath::RoundToInt(Fps * 3.f); ++i) { W.Tick(1.f / Fps); if (IsValid(G)) { CountMissiles(); } }
		TestEqual(*FString::Printf(TEXT("@%.0f fps: the wreck launched nothing more"), Fps), Seen.Num(), 1);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

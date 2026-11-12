// Triad Drift - Combat (Teammate 2). Tests closing the review's gaps. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Powers/TDPortalTransit.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "TriadDrift/Combat/Tests/TDCombatTestCommon.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Engine/StaticMesh.h"

namespace TDCombatTest
{


// ------------------------------------------------------------------------------------------ aiming

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapAimAssist, "TriadDrift.Combat.Aim.AimAssistThreeDegrees", TD_TEST_FLAGS)
bool FTDGapAimAssist::RunTest(const FString& Parameters)
{
	for (const float Offset : { 2.f, 5.f })
	{
		FTDTestWorld W;
		AActor* Rift = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* G = Guns(Rift, TDWeaponIds::PhasePulseLasers);
		G->MuzzleOffsets = { FVector(150.f, 0.f, 0.f) };
		AActor* Target = Body(W, ETDTeam::Enemy, FVector(10000.f, 0.f, 0.f), FVector(80.f));
		G->SetAimTarget(Target);
		G->SetAimDirection(AtAngle(Offset, 1.f)); // 2 deg = 350 cm off at 10 000 cm: a miss without assist
		G->FireOnce(ETDWeaponSlot::Primary);
		W.Advance(0.4f);
		if (Offset < 3.f)
		{
			TestEqual(TEXT("within 3 deg: assisted onto the target"), Hull(Target), 92.f, 0.01f);
		}
		else
		{
			TestEqual(TEXT("outside 3 deg: no assist, a clean miss"), Hull(Target), 100.f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapSpreadAndMuzzles, "TriadDrift.Combat.Aim.SpreadAndAlternatingMuzzles", TD_TEST_FLAGS)
bool FTDGapSpreadAndMuzzles::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Lancer = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDWeaponComponent* G = Guns(Lancer, TDWeaponIds::NeedleCannons, TDWeaponIds::SpikeRockets);
	float MaxDeg = 0.f, SumDeg = 0.f;
	for (int32 i = 0; i < 200; ++i)
	{
		G->FireOnce(ETDWeaponSlot::Primary);
		if (const ATDProjectile* P = G->GetLastFiredProjectile(0))
		{
			const float Deg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(P->GetVelocity().GetSafeNormal(), FVector::ForwardVector), -1.f, 1.f)));
			MaxDeg = FMath::Max(MaxDeg, Deg);
			SumDeg += Deg;
		}
	}
	TestTrue(TEXT("needle spread never exceeds 1.2 deg"), MaxDeg <= 1.21f);
	TestTrue(TEXT("needle spread is really there"), SumDeg / 200.f > 0.3f);

	// Phase lasers alternate left/right muzzles shot by shot.
	FTDTestWorld W2;
	AActor* Rift = Body(W2, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDWeaponComponent* G2 = Guns(Rift, TDWeaponIds::PhasePulseLasers);
	float LastY = 0.f;
	for (int32 i = 0; i < 4; ++i)
	{
		G2->FireOnce(ETDWeaponSlot::Primary);
		const ATDProjectile* P = G2->GetLastFiredProjectile(0);
		const float Y = P ? P->GetActorLocation().Y : 0.f;
		TestEqual(TEXT("muzzle offset"), FMath::Abs(Y), 60.f, 0.5f);
		if (i > 0)
		{
			TestTrue(TEXT("alternates"), FMath::Sign(Y) != FMath::Sign(LastY));
		}
		LastY = Y;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapDriftLead, "TriadDrift.Combat.Aim.DriftShotsAndLeadHit", TD_TEST_FLAGS)
bool FTDGapDriftLead::RunTest(const FString& Parameters)
{
	// 1) Drifting sideways at 6000 cm/s with the nose on a stationary target: crosshair on it = hit.
	{
		FTDTestWorld W;
		AActor* Rift = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		Cast<UPrimitiveComponent>(Rift->GetRootComponent())->ComponentVelocity = FVector(0.f, 6000.f, 0.f);
		UTDWeaponComponent* G = Guns(Rift, TDWeaponIds::PhasePulseLasers);
		G->MuzzleOffsets = { FVector(150.f, 0.f, 0.f) };
		AActor* Target = Body(W, ETDTeam::Enemy, FVector(15000.f, 0.f, 0.f), FVector(100.f));
		UTDTargetingComponent* T = NewObject<UTDTargetingComponent>(Rift, TEXT("Targeting"));
		T->RegisterComponent();
		T->SetManualTarget(Target);
		FVector Lead;
		TestTrue(TEXT("lead indicator"), T->GetLeadIndicator(Lead));
		TestTrue(TEXT("stationary target: lead marker sits on it despite the drift"), Lead.Equals(Target->GetActorLocation(), 1.f));
		G->SetAimDirection(FVector::ForwardVector);
		G->FireOnce(ETDWeaponSlot::Primary);
		W.Advance(0.6f);
		TestEqual(TEXT("drift shot hits"), Hull(Target), 92.f, 0.01f);
	}
	// 2) Drifting 60 deg off the nose, target crossing at 3000 cm/s: aim at the lead point = hit.
	for (const float Step : kCombatFrameSteps)
	{
		FTDTestWorld W;
		AActor* Rift = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		Cast<UPrimitiveComponent>(Rift->GetRootComponent())->ComponentVelocity = FVector(3000.f, 5196.f, 0.f);
		UTDWeaponComponent* G = Guns(Rift, TDWeaponIds::PhasePulseLasers);
		G->MuzzleOffsets = { FVector(0.f, 0.f, 0.f) };
		AActor* Target = Body(W, ETDTeam::Enemy, FVector(15000.f, -1500.f, 0.f), FVector(120.f));
		UProjectileMovementComponent* Move = NewObject<UProjectileMovementComponent>(Target, TEXT("Move"));
		Move->ProjectileGravityScale = 0.f;
		Move->RegisterComponent();
		Move->Velocity = FVector(0.f, 3000.f, 0.f);
		UTDTargetingComponent* T = NewObject<UTDTargetingComponent>(Rift, TEXT("Targeting"));
		T->RegisterComponent();
		T->SetManualTarget(Target);
		W.Tick(Step); // the target's movement reports its velocity once it has ticked
		FVector Lead;
		T->GetLeadIndicator(Lead);
		G->SetAimDirection((Lead - Rift->GetActorLocation()).GetSafeNormal());
		G->FireOnce(ETDWeaponSlot::Primary);
		W.Advance(0.8f, Step);
		TestEqual(*FString::Printf(TEXT("lead shot hits a crossing target while drifting (%.0f fps)"), 1.f / Step), Hull(Target), 92.f, 0.01f);
	}
	return true;
}

// ------------------------------------------------------------------------------------------ projectile lifetimes and fuses

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapRange, "TriadDrift.Combat.Projectile.RangeExpiryAndSplitAtMaxRange", TD_TEST_FLAGS)
bool FTDGapRange::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Shooter = Body(W, ETDTeam::Player, FVector(-500.f, 0.f, 0.f), FVector(50.f));
	ATDProjectile* Needle = Shoot(W, TDWeaponIds::NeedleCannons, FVector::ZeroVector, FVector::ForwardVector, Shooter, ETDTeam::Player);
	W.Advance(0.5f);
	TestTrue(TEXT("needle alive before 30 000 cm"), Needle && Needle->IsActive());
	W.Advance(0.1f);
	TestFalse(TEXT("needle gone after 30 000 cm"), Needle && Needle->IsActive());

	// Splitter at max range splits; Spike rocket at max range detonates with splash.
	AActor* NearEnd = Body(W, ETDTeam::Enemy, FVector(40000.f, 0.f, 250.f), FVector(50.f));
	Shoot(W, TDWeaponIds::SplitterBolt, FVector(0.f, 0.f, 5000.f), FVector::ForwardVector, Shooter, ETDTeam::Player);
	Shoot(W, TDWeaponIds::SpikeRockets, FVector::ZeroVector, FVector::ForwardVector, Shooter, ETDTeam::Player);
	const float SplitAt = AdvanceUntil(W, [&] { return CountLive(W.GetWorld(), [](const ATDProjectile& P) { return P.GetSpec().Damage == 12.f; }) == 5; }, 2.f);
	TestEqual(TEXT("splitter splits at 35 000 cm (1.4 s)"), SplitAt, 1.4f, 0.05f);
	W.Advance(0.2f);
	TestEqual(TEXT("rocket's end-of-range blast hit the ship 200 cm off its path"), Hull(NearEnd), 75.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapFlakFuse, "TriadDrift.Combat.Projectile.FlakFusesOnShipsAfterArming", TD_TEST_FLAGS)
bool FTDGapFlakFuse::RunTest(const FString& Parameters)
{
	// Near miss: passes 400 cm beside a fighter at 6000 cm -> proximity burst.
	{
		FTDTestWorld W;
		AActor* Bastion = Body(W, ETDTeam::Player, FVector(-500.f, 0.f, 0.f), FVector(50.f));
		AActor* Fighter = Body(W, ETDTeam::Enemy, FVector(6000.f, 500.f, 0.f), FVector(100.f));
		ATDProjectile* Flak = Shoot(W, TDWeaponIds::FlakBurst, FVector::ZeroVector, FVector::ForwardVector, Bastion, ETDTeam::Player);
		W.Advance(0.5f);
		TestFalse(TEXT("flak burst near the fighter"), Flak && Flak->IsActive());
		TestEqual(TEXT("fragment damage 15"), Hull(Fighter), 85.f, 0.01f);
	}
	// Not armed yet: a fighter 1000 cm out (reached in 0.045 s < 0.12 s arm) doesn't trigger it.
	{
		FTDTestWorld W;
		AActor* Bastion = Body(W, ETDTeam::Player, FVector(-500.f, 0.f, 0.f), FVector(50.f));
		AActor* Close = Body(W, ETDTeam::Enemy, FVector(1000.f, 300.f, 0.f), FVector(100.f));
		ATDProjectile* Flak = Shoot(W, TDWeaponIds::FlakBurst, FVector::ZeroVector, FVector::ForwardVector, Bastion, ETDTeam::Player);
		W.Advance(0.3f);
		TestTrue(TEXT("still flying"), Flak && Flak->IsActive());
		TestEqual(TEXT("unarmed flak ignored the close fighter"), Hull(Close), 100.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapMines, "TriadDrift.Combat.Projectile.PlasmaMinesArmFuseAndFizzle", TD_TEST_FLAGS)
bool FTDGapMines::RunTest(const FString& Parameters)
{
	{
		FTDTestWorld W;
		AActor* Bomber = Body(W, ETDTeam::Enemy, FVector(-3000.f, 0.f, 0.f), FVector(50.f));
		AActor* Rift = Body(W, ETDTeam::Player, FVector(450.f, 0.f, 0.f), FVector(100.f), 100.f, 100.f);
		ATDProjectile* Mine = Shoot(W, TDWeaponIds::EnemyPlasmaMines, FVector::ZeroVector, FVector::ForwardVector, Bomber, ETDTeam::Enemy);
		W.Advance(0.5f);
		TestTrue(TEXT("mine not armed at 0.5 s"), Mine && Mine->IsActive());
		W.Advance(0.4f);
		TestFalse(TEXT("armed mine fused on Rift"), Mine && Mine->IsActive());
		TestEqual(TEXT("60 damage"), Total(Rift), 140.f, 0.01f);
	}
	{
		FTDTestWorld W;
		AActor* Bomber = Body(W, ETDTeam::Enemy, FVector(-3000.f, 0.f, 0.f), FVector(50.f));
		AActor* Outside = Body(W, ETDTeam::Player, FVector(0.f, 1200.f, 0.f), FVector(100.f), 100.f, 100.f); // beyond the 500 cm fuse
		ATDProjectile* Mine = Shoot(W, TDWeaponIds::EnemyPlasmaMines, FVector::ZeroVector, FVector::ForwardVector, Bomber, ETDTeam::Enemy);
		W.Advance(19.8f, 1.f / 30.f);
		TestTrue(TEXT("mine waits 20 s"), Mine && Mine->IsActive());
		W.Advance(0.4f, 1.f / 30.f);
		TestFalse(TEXT("then fizzles"), Mine && Mine->IsActive());
		TestEqual(TEXT("fizzle does no splash"), Total(Outside), 200.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapPointDefense, "TriadDrift.Combat.Projectile.PointDefenseShootsDownMissiles", TD_TEST_FLAGS)
bool FTDGapPointDefense::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Bastion = Body(W, ETDTeam::Player, FVector(-6000.f, 0.f, 0.f), FVector(50.f));
	AActor* Carrier = Body(W, ETDTeam::Enemy, FVector(6000.f, 0.f, 0.f), FVector(300.f), 2000.f);
	ATDProjectile* Swarmer = Shoot(W, TDWeaponIds::EnemyHomingMissile, FVector(-3000.f, 0.f, 0.f), FVector::ForwardVector, Bastion, ETDTeam::Player, Carrier);
	ATDProjectile* PD = Shoot(W, TDWeaponIds::EnemyPointDefense, FVector(3000.f, 0.f, 0.f), -FVector::ForwardVector, Carrier, ETDTeam::Enemy);
	W.Advance(0.5f);
	TestFalse(TEXT("squad missile shot down"), Swarmer && Swarmer->IsActive());
	TestFalse(TEXT("point-defense round spent"), PD && PD->IsActive());
	TestEqual(TEXT("carrier untouched"), Hull(Carrier), 2000.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapPoolReuse, "TriadDrift.Combat.Projectile.PoolReuseIsANewLaunch", TD_TEST_FLAGS)
bool FTDGapPoolReuse::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Rift = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDWeaponComponent* G = Guns(Rift, TDWeaponIds::PhasePulseLasers);
	G->FireOnce(ETDWeaponSlot::Primary);
	ATDProjectile* First = G->GetLastFiredProjectile(0);
	const uint32 FirstSerial = First ? First->GetLaunchSerial() : 0;

	// A portal transit, then it dies and the same actor comes back for a new shot.
	FTDPortalTransitParams Params;
	const FTransform Entry(FRotator::ZeroRotator, FVector(500.f, 0.f, 0.f));
	const FTransform Exit(FRotator::ZeroRotator, FVector(0.f, 9000.f, 0.f));
	TestTrue(TEXT("first launch transits"), UTDPortalTransit::TransitActor(First, Entry, Exit, Rift, Params));
	First->Kill(false);
	TestNull(TEXT("dead shot no longer reported"), G->GetLastFiredProjectile(0));

	ATDProjectile* Second = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::NeedleCannons).Projectile,
		FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 3000.f)), Rift, ETDTeam::Player);
	TestTrue(TEXT("pool reused the actor"), Second == First);
	TestTrue(TEXT("new launch serial"), Second && Second->GetLaunchSerial() != FirstSerial);
	TestTrue(TEXT("new launch looks like a needle"), Second && Second->GetSpec().Damage == 4.f && !Second->WasDeflected());
	TestTrue(TEXT("reused actor is not in the old launch's portal cooldown"), UTDPortalTransit::TransitActor(Second, Entry, Exit, Rift, Params));
	TestNull(TEXT("old shot handle does not resolve to the new launch"), G->GetLastFiredProjectile(0));
	return true;
}

// ------------------------------------------------------------------------------------------ HUD data and feel

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapMarkers, "TriadDrift.Combat.Targeting.OffscreenMarkerFields", TD_TEST_FLAGS)
bool FTDGapMarkers::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* T = NewObject<UTDTargetingComponent>(Ship, TEXT("Targeting"));
	T->RegisterComponent();
	AActor* Ahead = Body(W, ETDTeam::Enemy, FVector(10000.f, 0.f, 0.f));
	AActor* Right = Body(W, ETDTeam::Enemy, FVector(0.f, 8000.f, 0.f));
	AActor* Above = Body(W, ETDTeam::Enemy, FVector(0.f, 0.f, 9000.f));
	AActor* Behind = Body(W, ETDTeam::Enemy, FVector(-7000.f, 0.f, 0.f));
	T->RefreshNow();
	auto Find = [T](const AActor* A) { return T->GetMarkers().FindByPredicate([A](const FTDTargetMarker& M) { return M.Actor.Get() == A; }); };

	const FTDTargetMarker* MA = Find(Ahead);
	TestTrue(TEXT("ahead: on screen at centre, current target"), MA && MA->bOnScreen && MA->ScreenPosition01.Equals(FVector2D(0.5f, 0.5f), 0.01f) && MA->bIsCurrentTarget);
	const FTDTargetMarker* MR = Find(Right);
	TestTrue(TEXT("right: off screen, arrow points right"), MR && !MR->bOnScreen && MR->ScreenDirection.Equals(FVector2D(1.f, 0.f), 0.01f));
	const FTDTargetMarker* MU = Find(Above);
	TestTrue(TEXT("above: off screen, arrow points up (screen y-)"), MU && !MU->bOnScreen && MU->ScreenDirection.Equals(FVector2D(0.f, -1.f), 0.01f));
	const FTDTargetMarker* MB = Find(Behind);
	TestTrue(TEXT("behind: off screen"), MB && !MB->bOnScreen);
	TestTrue(TEXT("distances"), MB && FMath::IsNearlyEqual(MB->Distance, 7000.f, 1.f));

	// Brackets follow every frame: move the target, tick once (well inside the 30 Hz selection interval).
	Ahead->SetActorLocation(FVector(10000.f, 500.f, 0.f));
	W.Tick(1.f / 120.f);
	MA = Find(Ahead);
	TestTrue(TEXT("marker re-projected this frame"), MA && FMath::IsNearlyEqual(MA->WorldLocation.Y, 500.f, 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapHitStop, "TriadDrift.Combat.Projectile.KillHitStopLasts40ms", TD_TEST_FLAGS)
bool FTDGapHitStop::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	AActor* Fighter = Body(W, ETDTeam::Enemy, FVector(1500.f, 0.f, 0.f), FVector(150.f), 5.f);
	UTDWeaponComponent* G = Guns(Ship, TDWeaponIds::BreakerBlaster);
	G->bForceReportHits = true;
	G->FireOnce(ETDWeaponSlot::Primary);
	UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
	AdvanceUntil(W, [&] { return UTDStatics::GetHealth(Fighter)->IsDead(); }, 1.f, 1.f / 240.f);
	TestTrue(TEXT("hit-stop on the kill"), Feel && Feel->GetCurrentDilation() < 1.f);
	W.Advance(0.03f, 1.f / 240.f);
	TestTrue(TEXT("still stopped at 30 ms"), Feel && Feel->GetCurrentDilation() < 1.f);
	W.Advance(0.02f, 1.f / 240.f);
	TestEqual(TEXT("normal speed after 40 ms"), Feel ? Feel->GetCurrentDilation() : 0.f, 1.f, 0.001f);
	return true;
}

// ------------------------------------------------------------------------------------------ frame-rate independence

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapMultiFps, "TriadDrift.Combat.Weapon.OverheatChargeLocksHomingAt30_60_120", TD_TEST_FLAGS)
bool FTDGapMultiFps::RunTest(const FString& Parameters)
{
	for (const float Step : kCombatFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT("%.0f fps"), 1.f / Step);
		// Overheat at 4 s, strict lockout (no shot while overheated), 1.5 s.
		{
			FTDTestWorld W;
			AActor* Rift = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
			UTDWeaponComponent* G = Guns(Rift, TDWeaponIds::PhasePulseLasers);
			G->SetTriggerHeld(ETDWeaponSlot::Primary, true);
			const float At = AdvanceUntil(W, [&] { return G->IsOverheated(ETDWeaponSlot::Primary); }, 6.f, Step);
			TestEqual(*(Fps + TEXT(": overheats at 4 s")), At, 4.f, Step + 0.01f);
			const int32 Shots = G->GetSlotState(ETDWeaponSlot::Primary).ShotsFired;
			float Locked = 0.f;
			bool bShotDuringLockout = false;
			while (G->IsOverheated(ETDWeaponSlot::Primary) && Locked < 3.f)
			{
				W.Tick(Step);
				Locked += Step;
				bShotDuringLockout |= G->IsOverheated(ETDWeaponSlot::Primary) && G->GetSlotState(ETDWeaponSlot::Primary).ShotsFired != Shots;
			}
			TestFalse(*(Fps + TEXT(": no shot during lockout")), bShotDuringLockout);
			TestEqual(*(Fps + TEXT(": 1.5 s lockout")), Locked, 1.5f, Step + 0.01f);
		}
		// Charge: 0.8 s.
		{
			FTDTestWorld W;
			AActor* Rift = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
			UTDWeaponComponent* G = Guns(Rift, TDWeaponIds::PhasePulseLasers, TDWeaponIds::SplitterBolt);
			G->SetTriggerHeld(ETDWeaponSlot::Secondary, true);
			const float At = AdvanceUntil(W, [&] { return G->GetCharge01(ETDWeaponSlot::Secondary) >= 1.f; }, 2.f, Step);
			TestEqual(*(Fps + TEXT(": full charge at 0.8 s")), At, 0.8f, Step + 0.01f);
		}
		// Six locks at 0.25 s each.
		{
			FTDTestWorld W;
			AActor* Bastion = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
			UTDTargetingComponent* T = NewObject<UTDTargetingComponent>(Bastion, TEXT("Targeting"));
			T->RegisterComponent();
			for (int32 i = 0; i < 7; ++i)
			{
				Body(W, ETDTeam::Enemy, FVector(8000.f, -2100.f + i * 700.f, 0.f));
			}
			T->BeginLockPainting();
			const float At = AdvanceUntil(W, [&] { return T->GetLockedTargets().Num() >= 6; }, 3.f, Step);
			TestEqual(*(Fps + TEXT(": 6 locks at 1.5 s")), At, 1.5f, Step + 0.01f);
		}
		// Homing missile reaches a target 18 deg off the nose.
		{
			FTDTestWorld W;
			AActor* Hook = Body(W, ETDTeam::Enemy, FVector(-500.f, 0.f, 0.f), FVector(50.f));
			AActor* Rift = Body(W, ETDTeam::Player, FVector(6000.f, 1000.f, 0.f), FVector(100.f), 100.f, 100.f);
			Shoot(W, TDWeaponIds::EnemyHomingMissile, FVector::ZeroVector, FVector::ForwardVector, Hook, ETDTeam::Enemy, Rift);
			const float At = AdvanceUntil(W, [&] { return Total(Rift) < 200.f; }, 3.f, Step);
			TestTrue(*(Fps + TEXT(": homing missile hits")), At > 0.f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapTapHeat, "TriadDrift.Combat.Weapon.TapFiringStillOverheats", TD_TEST_FLAGS)
bool FTDGapTapHeat::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Bastion = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDWeaponComponent* G = Guns(Bastion, TDWeaponIds::BreakerBlaster);
	// Tap at the max rate: press for one frame every 0.4 s.
	bool bOverheated = false;
	for (int32 Tap = 0; Tap < 30 && !bOverheated; ++Tap)
	{
		G->SetTriggerHeld(ETDWeaponSlot::Primary, true);
		W.Tick(1.f / 60.f);
		G->SetTriggerHeld(ETDWeaponSlot::Primary, false);
		W.Advance(0.4f - 1.f / 60.f);
		bOverheated = G->IsOverheated(ETDWeaponSlot::Primary);
	}
	TestTrue(TEXT("tap-firing the Breaker at full rate still overheats"), bOverheated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapTriggerSurvivesBlock, "TriadDrift.Combat.Weapon.TriggerResumesAfterBlockAndDeathStopsFire", TD_TEST_FLAGS)
bool FTDGapTriggerSurvivesBlock::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Rift = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f), 100.f, 0.f);
	UTDWeaponComponent* G = Guns(Rift, TDWeaponIds::PhasePulseLasers, TDWeaponIds::SplitterBolt);
	G->SetTriggerHeld(ETDWeaponSlot::Primary, true);
	W.Advance(0.2f);
	G->SetWeaponsBlocked(TEXT("EMP"), true);
	const int32 Before = G->GetSlotState(ETDWeaponSlot::Primary).ShotsFired;
	W.Advance(1.f);
	TestEqual(TEXT("no fire while blocked"), G->GetSlotState(ETDWeaponSlot::Primary).ShotsFired, Before);
	G->SetWeaponsBlocked(TEXT("EMP"), false);
	W.Advance(0.5f);
	TestTrue(TEXT("held trigger resumes without a re-click"), G->GetSlotState(ETDWeaponSlot::Primary).ShotsFired > Before);

	// A charged Splitter must not fire when its ship dies or is switched away.
	G->SetTriggerHeld(ETDWeaponSlot::Primary, false);
	G->SetTriggerHeld(ETDWeaponSlot::Secondary, true);
	W.Advance(1.f);
	G->CancelCharge(ETDWeaponSlot::Secondary);
	G->SetTriggerHeld(ETDWeaponSlot::Secondary, false);
	TestEqual(TEXT("cancelled charge did not fire"), G->GetSlotState(ETDWeaponSlot::Secondary).ShotsFired, 0);

	G->SetTriggerHeld(ETDWeaponSlot::Secondary, true);
	W.Advance(1.f);
	UTDStatics::GetHealth(Rift)->Kill();
	W.Tick();
	G->SetTriggerHeld(ETDWeaponSlot::Secondary, false);
	TestEqual(TEXT("a dead ship's charged shot never fires"), G->GetSlotState(ETDWeaponSlot::Secondary).ShotsFired, 0);
	const int32 PrimaryShots = G->GetSlotState(ETDWeaponSlot::Primary).ShotsFired;
	G->SetTriggerHeld(ETDWeaponSlot::Primary, true);
	W.Advance(0.5f);
	TestEqual(TEXT("dead ship's guns are silent"), G->GetSlotState(ETDWeaponSlot::Primary).ShotsFired, PrimaryShots);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapSalvoTarget, "TriadDrift.Combat.Weapon.SalvoKeepsItsTarget", TD_TEST_FLAGS)
bool FTDGapSalvoTarget::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Gunship = Body(W, ETDTeam::Enemy, FVector::ZeroVector, FVector(150.f));
	AActor* Rift = Body(W, ETDTeam::Player, FVector(8000.f, 2000.f, 0.f), FVector(100.f), 100.f, 100.f);
	UTDWeaponComponent* G = Guns(Gunship, TDWeaponIds::EnemyPulseGuns, TDWeaponIds::EnemySalvoMissile);
	G->SetAimTarget(Rift);
	G->FireOnce(ETDWeaponSlot::Secondary);
	G->SetAimTarget(nullptr); // the AI clears its target right after the call
	W.Advance(0.4f);
	int32 Homing = 0;
	for (int32 i = 0; i < G->GetLastFiredCount(); ++i)
	{
		const ATDProjectile* P = G->GetLastFiredProjectile(i);
		Homing += (P && P->GetHomingTarget() == Rift) ? 1 : 0;
	}
	TestEqual(TEXT("all 3 rippled missiles home on the captured target"), Homing, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapBreakTracking, "TriadDrift.Combat.Projectile.BreakTrackingAbandonsLock", TD_TEST_FLAGS)
bool FTDGapBreakTracking::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Hook = Body(W, ETDTeam::Enemy, FVector(-500.f, 0.f, 0.f), FVector(50.f));
	AActor* Rift = Body(W, ETDTeam::Player, FVector(8000.f, 1500.f, 0.f), FVector(100.f), 100.f, 100.f);
	ATDProjectile* M = Shoot(W, TDWeaponIds::EnemyHomingMissile, FVector::ZeroVector, FVector::ForwardVector, Hook, ETDTeam::Enemy, Rift);
	M->BreakTracking(0.5f, false);
	W.Advance(1.f);
	TestNull(TEXT("no re-acquisition after the block"), M->GetHomingTarget());
	W.Advance(1.5f);
	TestEqual(TEXT("flew straight past"), Total(Rift), 200.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapKnockbackReal, "TriadDrift.Combat.Projectile.BreakerKnockbackOnRealShipMovement", TD_TEST_FLAGS)
bool FTDGapKnockbackReal::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Bastion = Body(W, ETDTeam::Player, FVector(-500.f, 0.f, 0.f), FVector(50.f));
	auto MakeFighter = [&W](const FVector& Loc)
	{
		ATDEnemyShip* F = W.Spawn<ATDEnemyShip>(Loc);
		if (AController* C = F->GetController())
		{
			C->UnPossess();
			C->Destroy();
		}
		F->InitFromSpec(TDEnemyIds::ShardFighter);
		F->GetHealth()->InitMaxValues(1000.f, 0.f, true); // survive the 30-damage hit so the push is measurable
		F->SetActorRotation(FRotator(0.f, 180.f, 0.f)); // flying head-on down the shot line
		return F;
	};
	ATDEnemyShip* Hit = MakeFighter(FVector(2500.f, 0.f, 0.f));
	ATDEnemyShip* Control = MakeFighter(FVector(2500.f, 20000.f, 0.f));
	Shoot(W, TDWeaponIds::BreakerBlaster, FVector::ZeroVector, FVector::ForwardVector, Bastion, ETDTeam::Player);
	const float HitAt = AdvanceUntil(W, [&] { return Hit->GetHealth()->GetHull() < Hit->GetHealth()->MaxHull; }, 0.5f);
	TestTrue(TEXT("breaker hit the fighter"), HitAt > 0.f);
	W.Tick();
	const float PushX = Hit->GetVelocity().X - Control->GetVelocity().X;
	TestTrue(TEXT("fighter pushed along the shot by the real flight model"), PushX > 600.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGapMeshOffset, "TriadDrift.Combat.Projectile.ProjectileMeshOffsetIsVisualOnly", TD_TEST_FLAGS)
bool FTDGapMeshOffset::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Lancer = Body(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDWeaponFX* FX = NewObject<UTDWeaponFX>();
	const TStrongObjectPtr<UTDWeaponFX> FXRooted(FX); // a test-made asset: nothing else holds it until it's assigned
	FX->ProjectileMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cone.Cone")));
	FX->ProjectileMeshOffset = FVector(54.f, 0.f, 0.f);
	FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::SpikeRockets).Projectile;
	Spec.FX = FX;
	const FTransform At(FRotator::ZeroRotator, FVector(0.f, 0.f, 500.f));
	ATDProjectile* P = ATDProjectile::Spawn(W.GetWorld(), Spec, At, Lancer, ETDTeam::Player);
	const UStaticMeshComponent* Mesh = P ? P->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	TestTrue(TEXT("art mesh offset forward by 54 cm"), Mesh && Mesh->GetRelativeLocation().Equals(FVector(54.f, 0.f, 0.f), 0.01f));
	TestTrue(TEXT("collision centre (the actor) unchanged"), P && P->GetActorLocation().Equals(At.GetLocation(), 0.01f));
	P->Kill(false);
	ATDProjectile* Reused = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::NeedleCannons).Projectile, At, Lancer, ETDTeam::Player);
	const UStaticMeshComponent* Mesh2 = Reused ? Reused->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	// Reuse without art: the art offset is gone; the placeholder streak sits at its own trailing offset.
	const FTDStreakLook Look = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::NeedleCannons).Projectile);
	const FVector StreakOffset(-0.5f * Look.Length + 0.5f * Look.Thickness, 0.f, 0.f);
	TestTrue(TEXT("pooled reuse drops the art offset"), Reused == P && Mesh2 && Mesh2->GetRelativeLocation().Equals(StreakOffset, 0.01f));
	return true;
}

} // namespace TDCombatTest

#endif // WITH_DEV_AUTOMATION_TESTS

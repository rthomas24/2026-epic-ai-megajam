// Second independent Combat review (ENG-05 / ENG-06), minors 4-9: regression tests. Teammate 2.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Review

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "TriadDrift/Combat/Tests/TDCombatTestCommon.h"
#include "TriadDrift/Combat/Tests/TDCombatTestHelpers.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "GameFramework/PlayerController.h"

namespace TDCombatReviewTest
{
	/** First live projectile fired by Shooter, or null. */
	ATDProjectile* FirstShotOf(UWorld* World, const AActor* Shooter)
	{
		if (UTDProjectileSubsystem* S = UTDProjectileSubsystem::Get(World))
		{
			for (ATDProjectile* P : S->GetLiveProjectiles())
			{
				if (P && P->IsActive() && P->GetShooter() == Shooter)
				{
					return P;
				}
			}
		}
		return nullptr;
	}
}

/** Minor 4: the press-frame shot starts at the muzzle at any frame rate (no whole-frame head start). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewPressFrameHeadStart, "TriadDrift.Combat.Review.PressFrameShotStartsAtTheMuzzle", TD_TEST_FLAGS)
bool FTDReviewPressFrameHeadStart::RunTest(const FString& Parameters)
{
	TMap<int32, float> Ahead;
	for (const float Step : TDCombatTest::kCombatFrameSteps)
	{
		FTDTestWorld W;
		AActor* Ship = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Ship);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
		W.Tick(Step);
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
		W.Tick(Step); // the press frame
		const ATDProjectile* P = TDCombatReviewTest::FirstShotOf(W.GetWorld(), Ship);
		if (!TestNotNull(FString::Printf(TEXT("shot fired @ %.0f fps"), 1.f / Step), P))
		{
			return false;
		}
		Ahead.Add(FMath::RoundToInt(1.f / Step), (float)P->GetActorLocation().X);
	}
	const float At30 = Ahead.FindRef(30);
	const float At120 = Ahead.FindRef(120);
	AddInfo(FString::Printf(TEXT("press-frame shot X: %.0f cm @ 30 fps, %.0f cm @ 120 fps"), At30, At120));
	TestTrue(TEXT("same start point at 30 and 120 fps (within 50 cm)"), FMath::Abs(At30 - At120) <= 50.f);
	TestTrue(TEXT("at the muzzle, not a frame ahead"), At30 < 400.f);
	return true;
}

/** Minor 5: a rippled salvo stops when its shooter dies (no missiles from the wreck). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewSalvoAfterDeath, "TriadDrift.Combat.Review.NoSalvoFromAWreck", TD_TEST_FLAGS)
bool FTDReviewSalvoAfterDeath::RunTest(const FString& Parameters)
{
	for (const float Step : TDCombatTest::kCombatFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		AActor* Gunship = TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector::ZeroVector, FVector(100.f), 100.f, 0.f);
		UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Gunship);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::EnemySalvoMissile);
		Weapons->FireOnce(ETDWeaponSlot::Primary);
		const int32 AfterFirst = TDCombatTest::CountLive(W.GetWorld(), [Gunship](const ATDProjectile& P) { return P.GetShooter() == Gunship; });
		TestEqual(TEXT("first missile out") + Fps, AfterFirst, 1);
		UTDStatics::GetHealth(Gunship)->Kill();
		W.Advance(0.6f, Step);
		const int32 After = TDCombatTest::CountLive(W.GetWorld(), [Gunship](const ATDProjectile& P) { return P.GetShooter() == Gunship; });
		TestTrue(TEXT("no more missiles once dead") + Fps, After <= AfterFirst);
	}
	return true;
}

/** Minor 6: shooting a rock (no health component) never confirms a hit. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewNoHitOnRocks, "TriadDrift.Combat.Review.RocksNeverConfirmAHit", TD_TEST_FLAGS)
bool FTDReviewNoHitOnRocks::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Rock = W.Spawn<AActor>(FVector(2000.f, 0.f, 0.f));
	UBoxComponent* Box = NewObject<UBoxComponent>(Rock, TEXT("Rock"));
	Box->SetBoxExtent(FVector(300.f));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Rock->SetRootComponent(Box);
	Box->RegisterComponent();
	Rock->SetActorLocation(FVector(2000.f, 0.f, 0.f));
	AActor* Ship = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));

	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	UTDTestHitListener* Listener = NewObject<UTDTestHitListener>();
	Listener->AddToRoot();
	if (Bus)
	{
		Bus->OnHitConfirmed.AddDynamic(Listener, &UTDTestHitListener::OnHit);
	}
	FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile;
	Spec.FX = nullptr;
	ATDProjectile::Launch(W.GetWorld(), Spec, FTransform(FVector(100.f, 0.f, 0.f)), Ship, ETDTeam::Player, nullptr, FVector::ZeroVector, 1.f, true);
	W.Advance(0.3f);
	TestEqual(TEXT("no hit confirmed on a rock"), Listener->Hits, 0);
	if (Bus)
	{
		Bus->OnHitConfirmed.RemoveAll(Listener);
	}
	Listener->RemoveFromRoot();
	return true;
}

/**
 * Minor 7 (re-check C3): the current target AND every lock get a marker, even ranked beyond MaxMarkers by distance.
 * 34 near hostiles sit off the nose (outside the lock cone); 6 far ones dead ahead are painted and locked.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewMarkersKeepTargets, "TriadDrift.Combat.Review.MarkersAlwaysIncludeTheTarget", TD_TEST_FLAGS)
bool FTDReviewMarkersKeepTargets::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* Targeting = TDCombatTest::AddTargeting(Ship);
	for (int32 I = 0; I < 34; ++I)
	{
		const float Side = (I % 2 == 0) ? 1.f : -1.f;
		TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector(2000.f + I * 150.f, Side * 6000.f, 0.f), FVector(50.f));
	}
	TArray<AActor*> Far;
	for (int32 I = 0; I < 6; ++I)
	{
		Far.Add(TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector(30000.f + I * 400.f, (I - 2.5f) * 300.f, 0.f), FVector(50.f)));
	}
	W.Tick();
	Targeting->SetManualTarget(Far[5]);
	Targeting->BeginLockPainting();
	W.Advance(2.5f);
	TestTrue(TEXT("more hostiles than markers"), Targeting->MaxMarkers < 40);
	auto HasMarker = [Targeting](const AActor* A)
	{
		return Targeting->K2_GetMarkers().ContainsByPredicate([A](const FTDTargetMarker& M) { return M.Actor.Get() == A; });
	};
	TestTrue(TEXT("the (farthest) current target has a marker"), HasMarker(Far[5]));
	TestTrue(TEXT("the far group is locked"), Targeting->GetLockedTargets().Num() >= 5);
	for (AActor* Locked : Targeting->GetLockedTargets())
	{
		TestTrue(TEXT("every lock has a marker"), HasMarker(Locked));
	}
	TestTrue(TEXT("never more markers than MaxMarkers"), Targeting->K2_GetMarkers().Num() <= Targeting->MaxMarkers);
	return true;
}

/** Minor 10: ResetWeapons clears heat, overheat, ammo, charge and a salvo in flight. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewResetWeapons, "TriadDrift.Combat.Review.ResetWeaponsClearsEverything", TD_TEST_FLAGS)
bool FTDReviewResetWeapons::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Ship);
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
	Weapons->EquipWeapon(ETDWeaponSlot::Secondary, TDWeaponIds::SpikeRockets);
	Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
	W.Advance(4.5f);
	TestTrue(TEXT("overheated before the reset"), Weapons->IsOverheated(ETDWeaponSlot::Primary));
	Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false);
	Weapons->FireOnce(ETDWeaponSlot::Secondary);
	const int32 AmmoAfterShot = Weapons->GetSlotState(ETDWeaponSlot::Secondary).Ammo;
	Weapons->ResetWeapons();
	TestFalse(TEXT("not overheated"), Weapons->IsOverheated(ETDWeaponSlot::Primary));
	TestEqual(TEXT("heat 0"), Weapons->GetSlotState(ETDWeaponSlot::Primary).Heat, 0.f);
	TestTrue(TEXT("ammo refilled"), Weapons->GetSlotState(ETDWeaponSlot::Secondary).Ammo > AmmoAfterShot);
	TestEqual(TEXT("no salvo left"), Weapons->GetSlotState(ETDWeaponSlot::Secondary).SalvoRemaining, 0);
	TestTrue(TEXT("fires again at once"), Weapons->FireOnce(ETDWeaponSlot::Primary));
	return true;
}

/** Minor 10: hits are reported for a player-controlled ship, not for the same ship flown by AI. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewReportsOnlyPlayerHits, "TriadDrift.Combat.Review.HitReportsFollowPlayerControl", TD_TEST_FLAGS)
bool FTDReviewReportsOnlyPlayerHits::RunTest(const FString& Parameters)
{
	for (const bool bPlayer : { true, false })
	{
		FTDTestWorld W;
		ATDSpacecraft* Ship = W.Spawn<ATDSpacecraft>(FVector::ZeroVector);
		Ship->SetTeam(ETDTeam::Player);
		if (bPlayer)
		{
			W.SpawnPlayerController(Ship);
		}
		UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Ship);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
		Weapons->MuzzleOffsets = { FVector(300.f, 0.f, 0.f) };
		Weapons->SetAimDirection(FVector::ForwardVector);
		TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector(3000.f, 0.f, 0.f), FVector(200.f), 1000.f, 0.f);
		UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
		UTDTestHitListener* Listener = NewObject<UTDTestHitListener>();
		Listener->AddToRoot();
		if (Bus)
		{
			Bus->OnHitConfirmed.AddDynamic(Listener, &UTDTestHitListener::OnHit);
		}
		Weapons->FireOnce(ETDWeaponSlot::Primary);
		W.Advance(0.3f);
		if (bPlayer)
		{
			TestEqual(TEXT("player-flown: hit reported"), Listener->Hits, 1);
		}
		else
		{
			TestEqual(TEXT("AI-flown: no hit reported"), Listener->Hits, 0);
		}
		if (Bus)
		{
			Bus->OnHitConfirmed.RemoveAll(Listener);
		}
		Listener->RemoveFromRoot();
	}
	return true;
}

/** Minor 10: drift with a real ship's movement (not a hand-set ComponentVelocity): nose on target = hit. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewRealShipDrift, "TriadDrift.Combat.Review.RealShipDriftShotHits", TD_TEST_FLAGS)
bool FTDReviewRealShipDrift::RunTest(const FString& Parameters)
{
	for (const float Step : TDCombatTest::kCombatFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		ATDSpacecraft* Ship = W.Spawn<ATDSpacecraft>(FVector::ZeroVector);
		Ship->SetTeam(ETDTeam::Player);
		UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Ship);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
		Weapons->MuzzleOffsets = { FVector(300.f, 0.f, 0.f) };
		Weapons->SetAimDirection(FVector::ForwardVector);
		AActor* Target = TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector(15000.f, 0.f, 0.f), FVector(150.f), 1000.f, 0.f);
		Ship->GetShipMovement()->Velocity = FVector(0.f, 6000.f, 0.f); // sliding sideways, nose on the target
		W.Tick(Step);
		AddInfo(FString::Printf(TEXT("ship velocity at fire %s"), *Ship->GetVelocity().ToString()));
		const FVector Muzzle = Ship->GetActorLocation();
		Target->SetActorLocation(FVector(Muzzle.X + 15000.f, Muzzle.Y, 0.f));
		Weapons->FireOnce(ETDWeaponSlot::Primary);
		W.Advance(0.8f, Step);
		TestTrue(TEXT("drift shot hits what the nose points at") + Fps, TDCombatTest::Hull(Target) < 1000.f);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

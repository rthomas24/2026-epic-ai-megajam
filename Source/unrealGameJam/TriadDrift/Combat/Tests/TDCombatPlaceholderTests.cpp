// Placeholder streaks and flashes (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Placeholder

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "Camera/CameraShakeBase.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/Tests/TDCombatTestCommon.h"

namespace TDCombatPlaceholderTest
{
	const FName PlayerWeaponIds[] = { TDWeaponIds::PhasePulseLasers, TDWeaponIds::SplitterBolt, TDWeaponIds::NeedleCannons,
		TDWeaponIds::SpikeRockets, TDWeaponIds::BreakerBlaster, TDWeaponIds::FlakBurst };
	const FName EnemyWeaponIds[] = { TDWeaponIds::EnemyPulseGuns, TDWeaponIds::EnemyHomingMissile, TDWeaponIds::EnemyHeavyBolt,
		TDWeaponIds::EnemySalvoMissile, TDWeaponIds::EnemyPlasmaMines, TDWeaponIds::EnemyPointDefense };
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderStreakSizes, "TriadDrift.Combat.Placeholder.StreakSizes", TD_TEST_FLAGS)
bool FTDCombatPlaceholderStreakSizes::RunTest(const FString& Parameters)
{
	for (const FName Id : TDCombatPlaceholderTest::PlayerWeaponIds)
	{
		const FTDProjectileSpec& Spec = UTDWeaponLibrary::Get(Id).Projectile;
		const FTDStreakLook Look = FTDCombatPlaceholders::ComputeStreak(Spec);
		const FString Name = Id.ToString();
		TestTrue(Name + TEXT(" is a streak"), Look.bStreak);
		TestTrue(Name + TEXT(" length 250-800 cm"), Look.Length >= 250.f && Look.Length <= 800.f);
		TestTrue(Name + TEXT(" thick enough to read (>= 35 cm)"), Look.Thickness >= 35.f);
		if (!Look.bHeavy)
		{
			TestTrue(Name + TEXT(" laser/needle is 300-800 cm"), Look.Length >= 300.f && Look.Length <= 800.f);
		}
	}
	const FTDStreakLook Laser = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile);
	const FTDStreakLook Rocket = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::SpikeRockets).Projectile);
	TestTrue(TEXT("rockets are fatter than lasers"), Rocket.bHeavy && Rocket.Thickness > Laser.Thickness);

	const FTDStreakLook Missile = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile);
	TestTrue(TEXT("homing missile has a glowing head bigger than its trail"), Missile.bHead && Missile.HeadDiameter > Missile.Thickness);
	TestFalse(TEXT("lasers have no head"), Laser.bHead);

	// Playtest 3: Bastion's heavy shell must outweigh a needle on screen, and no muzzle flash hides the hull.
	const FTDStreakLook Shell = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::BreakerBlaster).Projectile);
	const FTDStreakLook Needle = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::NeedleCannons).Projectile);
	TestTrue(TEXT("heavy shell carries a hot head"), Shell.bHeavy && Shell.bHead);
	TestTrue(TEXT("far away, a heavy shell is drawn bigger than a needle"),
		Shell.Thickness * FTDCombatPlaceholders::DistanceGrowth(Shell, 20000.f) > Needle.Thickness * FTDCombatPlaceholders::DistanceGrowth(Needle, 20000.f) * 1.4f);
	const float HullRadius = 400.f;
	const float Capped = FTDCombatPlaceholders::CapMuzzleFlashRadius(FTDCombatPlaceholders::MuzzleFlashRadius(UTDWeaponLibrary::Get(TDWeaponIds::BreakerBlaster).Projectile), HullRadius);
	TestTrue(TEXT("muzzle flash at its biggest is at most a third of the hull across"), 2.f * 1.2f * Capped <= 2.f * HullRadius / 3.f + 0.01f);
	const FTDStreakLook Mine = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::EnemyPlasmaMines).Projectile);
	TestFalse(TEXT("slow mines keep their sphere"), Mine.bStreak);

	// End-on from the chase camera: thickness keeps ~11 px with distance, capped.
	TestEqual(TEXT("close shots keep their base size"), FTDCombatPlaceholders::DistanceGrowth(Laser, 1000.f), 1.f);
	const float Far = FTDCombatPlaceholders::DistanceGrowth(Laser, 20000.f);
	TestTrue(TEXT("far shots grow to stay readable"), Far > 1.f && Laser.Thickness * Far * 1280.f / (2.f * 20000.f) >= FTDCombatPlaceholders::MinStreakPixels - 0.01f);
	TestEqual(TEXT("growth is capped"), FTDCombatPlaceholders::DistanceGrowth(Laser, 1.0e6f), FTDCombatPlaceholders::MaxDistanceGrowth);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderStreakOnProjectile, "TriadDrift.Combat.Placeholder.StreakOnProjectile", TD_TEST_FLAGS)
bool FTDCombatPlaceholderStreakOnProjectile::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	auto Check = [&](FName Id, ETDTeam Team)
	{
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(Id).Projectile;
		Spec.FX = nullptr; // no art: placeholder only
		ATDProjectile* P = ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, Team);
		if (!TestNotNull(Id.ToString() + TEXT(" spawned"), P))
		{
			return;
		}
		const FTDStreakLook Look = FTDCombatPlaceholders::ComputeStreak(Spec);
		const UStaticMeshComponent* Mesh = P->GetPlaceholderMesh();
		TestTrue(Id.ToString() + TEXT(" streak visible"), Mesh && Mesh->IsVisible());
		if (Mesh && Look.bStreak)
		{
			TestTrue(Id.ToString() + TEXT(" streak length matches"), FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X * 100.f, Look.Length, 1.f));
			TestTrue(Id.ToString() + TEXT(" streak trails behind the shot"), Mesh->GetRelativeLocation().X < 0.f);
		}
		const UStaticMeshComponent* Head = P->GetPlaceholderHead();
		TestEqual(Id.ToString() + TEXT(" head only on missiles"), Head && Head->IsVisible(), Look.bHead);
		P->Kill(false);
		TestFalse(Id.ToString() + TEXT(" streak hidden when pooled"), Mesh && Mesh->IsVisible());
		TestFalse(Id.ToString() + TEXT(" head hidden when pooled"), Head && Head->IsVisible());
	};
	for (const FName Id : TDCombatPlaceholderTest::PlayerWeaponIds)
	{
		Check(Id, ETDTeam::Player);
	}
	for (const FName Id : TDCombatPlaceholderTest::EnemyWeaponIds)
	{
		Check(Id, ETDTeam::Enemy);
	}

	// A pooled actor reused by a laser after a missile drops the missile's head.
	FTDProjectileSpec MissileSpec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile;
	MissileSpec.FX = nullptr;
	ATDProjectile* M = ATDProjectile::Spawn(W.GetWorld(), MissileSpec, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, ETDTeam::Enemy);
	if (M)
	{
		M->Kill(false);
	}
	FTDProjectileSpec LaserSpec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile;
	LaserSpec.FX = nullptr;
	ATDProjectile* L = ATDProjectile::Spawn(W.GetWorld(), LaserSpec, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, ETDTeam::Player);
	TestTrue(TEXT("reused actor: no stale missile head"), L && L == M && !L->GetPlaceholderHead()->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderFlashes, "TriadDrift.Combat.Placeholder.MuzzleAndImpactFlashes", TD_TEST_FLAGS)
bool FTDCombatPlaceholderFlashes::RunTest(const FString& Parameters)
{
	for (const float Step : TDCombatTest::kCombatFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(W.GetWorld());
		if (!TestNotNull(TEXT("projectile subsystem"), Projectiles))
		{
			return false;
		}

		// Impact: a laser into a wall, no art.
		AActor* Wall = TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector(1500.f, 0.f, 0.f), FVector(100.f, 600.f, 600.f), 1000.f, 0.f);
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile;
		Spec.FX = nullptr;
		ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform(FVector::ZeroVector), nullptr, ETDTeam::Player);
		bool bFlashed = false;
		for (float T = 0.f; T < 0.3f && !bFlashed; T += Step)
		{
			W.Tick(Step);
			bFlashed = Projectiles->GetPlayingFlashCount() > 0;
		}
		TestTrue(TEXT("impact flash plays") + Fps, bFlashed && Wall != nullptr);
		W.Advance(0.5f, Step);
		TestEqual(TEXT("impact flash is short and goes away") + Fps, Projectiles->GetPlayingFlashCount(), 0);

		// Muzzle: forced placeholder-only FX, so the check never skips itself once art exists.
		AActor* Shooter = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector(0.f, 0.f, 5000.f), FVector(50.f));
		UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Shooter);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::NeedleCannons);
		const TStrongObjectPtr<UTDWeaponFX> NoArt(NewObject<UTDWeaponFX>());
		Weapons->PrimaryFX = NoArt.Get();
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
		bool bMuzzle = false;
		for (float T = 0.f; T < 0.2f && !bMuzzle; T += Step)
		{
			W.Tick(Step);
			bMuzzle = Projectiles->GetPlayingFlashCount() > 0;
		}
		TestTrue(TEXT("muzzle flash plays") + Fps, bMuzzle);
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false);
	}

	// The pool is capped: a storm of flashes reuses actors instead of spawning without bound.
	FTDTestWorld W;
	UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(W.GetWorld());
	for (int32 I = 0; I < 200; ++I)
	{
		Projectiles->PlayPlaceholderFlash(FVector(I * 10.f, 0.f, 0.f), 100.f, FLinearColor::Red, 1.f);
	}
	TestTrue(TEXT("flash pool capped"), Projectiles->GetPlayingFlashCount() <= Projectiles->MaxFlashes);
	return true;
}

namespace TDCombatPlaceholderTest
{
	const TCHAR* const EngineSound = TEXT("/Engine/EngineSounds/1kSineTonePing.1kSineTonePing");
	const TCHAR* const EngineVFX = TEXT("/Niagara/DefaultAssets/DefaultSystem.DefaultSystem");

	/** Fires one needle burst into a wall with FX and returns the most flashes seen playing at once. */
	int32 FireIntoWallAndCountFlashes(UTDWeaponFX* FX, float Step)
	{
		FTDTestWorld W;
		UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(W.GetWorld());
		TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector(3000.f, 0.f, 0.f), FVector(100.f, 800.f, 800.f), 100000.f, 0.f);
		AActor* Shooter = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Shooter);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::NeedleCannons);
		Weapons->PrimaryFX = FX;
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
		int32 Most = 0;
		for (float T = 0.f; T < 0.4f; T += Step)
		{
			W.Tick(Step);
			Most = FMath::Max(Most, Projectiles ? Projectiles->GetPlayingFlashCount() : 0);
		}
		return Most;
	}
}

/** Review MAJOR (ENG-05): a sound-only cue must not remove the placeholder flash; a VFX cue must. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderSoundOnlyCue, "TriadDrift.Combat.Placeholder.SoundOnlyCueKeepsFlash", TD_TEST_FLAGS)
bool FTDCombatPlaceholderSoundOnlyCue::RunTest(const FString& Parameters)
{
	FTDFXCue SoundOnly;
	SoundOnly.Sound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TDCombatPlaceholderTest::EngineSound));
	FTDFXCue WithVFX;
	WithVFX.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TDCombatPlaceholderTest::EngineVFX));
	TestFalse(TEXT("sound-only cue is not visual art"), FTDCombatPlaceholders::HasVisualArt(SoundOnly));
	TestTrue(TEXT("VFX cue is visual art"), FTDCombatPlaceholders::HasVisualArt(WithVFX));

	for (const float Step : TDCombatTest::kCombatFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		const TStrongObjectPtr<UTDWeaponFX> Audio(NewObject<UTDWeaponFX>());
		Audio->Muzzle = SoundOnly;
		Audio->ImpactWorld = SoundOnly;
		Audio->ImpactHull = SoundOnly;
		Audio->ImpactShield = SoundOnly;
		TestTrue(TEXT("sound-only cues keep muzzle and impact flashes") + Fps, TDCombatPlaceholderTest::FireIntoWallAndCountFlashes(Audio.Get(), Step) > 0);

		const TStrongObjectPtr<UTDWeaponFX> Visual(NewObject<UTDWeaponFX>());
		Visual->Muzzle = WithVFX;
		Visual->ImpactWorld = WithVFX;
		Visual->ImpactHull = WithVFX;
		Visual->ImpactShield = WithVFX;
		TestEqual(TEXT("VFX cues replace every placeholder flash") + Fps, TDCombatPlaceholderTest::FireIntoWallAndCountFlashes(Visual.Get(), Step), 0);
	}
	return true;
}

namespace TDCombatPlaceholderTest
{
	struct FFlashCounts { int32 Muzzle = 0; int32 Impact = 0; };

	/** One shot of WeaponId into a wall 3000 cm ahead; the most flashes seen at the muzzle and at the wall. */
	FFlashCounts FireAndCountBySite(FName WeaponId, UTDWeaponFX* FX, float Step)
	{
		FFlashCounts Out;
		FTDTestWorld W;
		UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(W.GetWorld());
		TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector(3100.f, 0.f, 0.f), FVector(100.f, 800.f, 800.f), 100000.f, 0.f);
		AActor* Shooter = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Shooter);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, WeaponId);
		Weapons->PrimaryFX = FX;
		Weapons->FireOnce(ETDWeaponSlot::Primary);
		Out.Muzzle = Projectiles ? Projectiles->CountPlayingFlashesNear(FVector::ZeroVector, 700.f) : 0;
		for (float T = 0.f; T < 0.5f; T += Step)
		{
			W.Tick(Step);
			Out.Impact = FMath::Max(Out.Impact, Projectiles ? Projectiles->CountPlayingFlashesNear(FVector(3000.f, 0.f, 0.f), 1200.f) : 0);
		}
		return Out;
	}
}

/** Review C1: each placeholder flash site follows its own cue: muzzle, impact, detonation; sound and shake never hide one. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderPerSite, "TriadDrift.Combat.Placeholder.EachFlashSiteFollowsItsOwnCue", TD_TEST_FLAGS)
bool FTDCombatPlaceholderPerSite::RunTest(const FString& Parameters)
{
	FTDFXCue Sound;
	Sound.Sound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TDCombatPlaceholderTest::EngineSound));
	FTDFXCue Shake;
	Shake.Shake = TSoftClassPtr<UCameraShakeBase>(FSoftObjectPath(TEXT("/Script/Engine.CameraShakeBase")));
	FTDFXCue Vfx;
	Vfx.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TDCombatPlaceholderTest::EngineVFX));
	TestFalse(TEXT("a shake-only cue is not visual art"), FTDCombatPlaceholders::HasVisualArt(Shake));

	for (const float Step : TDCombatTest::kCombatFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		auto Make = [](const FTDFXCue& Muzzle, const FTDFXCue& Impact, const FTDFXCue& Detonation)
		{
			UTDWeaponFX* FX = NewObject<UTDWeaponFX>();
			FX->Muzzle = Muzzle;
			FX->ImpactWorld = Impact;
			FX->ImpactHull = Impact;
			FX->ImpactShield = Impact;
			FX->Detonation = Detonation;
			return TStrongObjectPtr<UTDWeaponFX>(FX);
		};
		const FTDFXCue None;
		{
			const auto FX = Make(Sound, Vfx, None);
			const auto C = TDCombatPlaceholderTest::FireAndCountBySite(TDWeaponIds::NeedleCannons, FX.Get(), Step);
			TestTrue(TEXT("sound muzzle: muzzle flash stays") + Fps, C.Muzzle > 0);
			TestEqual(TEXT("VFX impact: no impact flash") + Fps, C.Impact, 0);
		}
		{
			const auto FX = Make(Vfx, Sound, None);
			const auto C = TDCombatPlaceholderTest::FireAndCountBySite(TDWeaponIds::NeedleCannons, FX.Get(), Step);
			TestEqual(TEXT("VFX muzzle: no muzzle flash") + Fps, C.Muzzle, 0);
			TestTrue(TEXT("sound impact: impact flash stays") + Fps, C.Impact > 0);
		}
		{
			const auto FX = Make(Shake, Shake, None);
			const auto C = TDCombatPlaceholderTest::FireAndCountBySite(TDWeaponIds::NeedleCannons, FX.Get(), Step);
			TestTrue(TEXT("shake-only muzzle keeps its flash") + Fps, C.Muzzle > 0);
			TestTrue(TEXT("shake-only impact keeps its flash") + Fps, C.Impact > 0);
		}
		// Splash weapon: the Detonation cue covers the impact.
		{
			const auto FX = Make(None, None, Sound);
			const auto C = TDCombatPlaceholderTest::FireAndCountBySite(TDWeaponIds::SpikeRockets, FX.Get(), Step);
			TestTrue(TEXT("sound detonation: impact flash stays") + Fps, C.Impact > 0);
		}
		{
			const auto FX = Make(None, None, Vfx);
			const auto C = TDCombatPlaceholderTest::FireAndCountBySite(TDWeaponIds::SpikeRockets, FX.Get(), Step);
			TestEqual(TEXT("VFX detonation: no impact flash") + Fps, C.Impact, 0);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

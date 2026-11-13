// Placeholder streaks and flashes (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Placeholder

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "Camera/CameraShakeBase.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/Tests/TDCombatTestCommon.h"
#include "TriadDrift/Flight/TDShip.h"
#include "GameFramework/Character.h"
#include "TriadDrift/Ground/Crew/TDCrewTypes.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	// Incoming hostile fire (flying toward the camera) keeps a bigger floor than the player's own outgoing shots.
	const FTDStreakLook EnemyBolt = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::EnemyPulseGuns).Projectile);
	TestTrue(TEXT("an enemy bolt flying at the camera is incoming"),
		FTDCombatPlaceholders::IsIncoming(FVector(-30000.f, 0.f, 0.f), FVector(20000.f, 0.f, 0.f), FVector::ZeroVector));
	TestFalse(TEXT("one flying away is not"),
		FTDCombatPlaceholders::IsIncoming(FVector(30000.f, 0.f, 0.f), FVector(20000.f, 0.f, 0.f), FVector::ZeroVector));
	const float IncomingPx = EnemyBolt.Thickness * FTDCombatPlaceholders::DistanceGrowth(EnemyBolt, 30000.f, FTDCombatPlaceholders::MinIncomingStreakPixels) * 1280.f / (2.f * 30000.f);
	TestTrue(FString::Printf(TEXT("incoming enemy bolt at 300 m is at least 18 px (%.1f)"), IncomingPx), IncomingPx >= FTDCombatPlaceholders::MinIncomingStreakPixels - 0.01f);
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
		const FTDStreakLook Look = Team == ETDTeam::Player ? FTDCombatPlaceholders::ComputeSquadStreak(Spec) : FTDCombatPlaceholders::ComputeStreak(Spec);
		const UStaticMeshComponent* Mesh = P->GetPlaceholderMesh();
		TestTrue(Id.ToString() + TEXT(" streak visible"), Mesh && Mesh->IsVisible());
		if (Mesh && Look.bStreak)
		{
			// At launch nothing is drawn behind the muzzle: the streak grows with the distance flown.
			TestTrue(FString::Printf(TEXT("%s streak starts at the muzzle (%.0f cm)"), *Id.ToString(), Mesh->GetRelativeScale3D().X * 100.f),
				Mesh->GetRelativeScale3D().X * 100.f <= Look.Thickness + 1.f);
			for (int32 Frame = 0; Frame < 12; ++Frame)
			{
				W.Tick(1.f / 60.f);
			}
			if (P->IsActive())
			{
				TestTrue(FString::Printf(TEXT("%s streak grows to its length (%.0f of %.0f cm)"), *Id.ToString(), Mesh->GetRelativeScale3D().X * 100.f, Look.Length),
					FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X * 100.f, Look.Length, 1.f));
			}
			TestTrue(Id.ToString() + TEXT(" streak trails behind the shot"), Mesh->GetRelativeLocation().X < 0.f);
		}
		const UStaticMeshComponent* Head = P->GetPlaceholderHead();
		const bool bTip = Team == ETDTeam::Player && Look.bStreak && !Look.bHead;
		TestEqual(Id.ToString() + TEXT(" head on missiles, tracer tip on squad light shots"), Head && Head->IsVisible(), Look.bHead || bTip);
		TestEqual(Id.ToString() + TEXT(" tracer tip"), P->HasTracerTip(), bTip);
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
	// The squad laser draws its own tracer tip (as thick as its streak), never the missile's big round head.
	const float TipThick = FTDCombatPlaceholders::ComputeSquadStreak(LaserSpec).Thickness * FTDCombatPlaceholders::TipThickness;
	TestTrue(TEXT("reused actor: no stale missile head"), L && L == M && L->HasTracerTip()
		&& FMath::IsNearlyEqual(L->GetPlaceholderHead()->GetRelativeScale3D().Y * 100.f, TipThick, 1.f));
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

/** Placeholder explosion (deaths, rockets, missiles, mines): a core, a shell and debris streaks that fly outward. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderExplosion, "TriadDrift.Combat.Placeholder.ExplosionHasCoreShellAndDebris", TD_TEST_FLAGS)
bool FTDCombatPlaceholderExplosion::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(W.GetWorld());
	const FVector Here(5000.f, 0.f, 0.f);
	const int32 Used = Projectiles->PlayPlaceholderExplosion(Here, 600.f, FLinearColor::Red, 8);
	TestEqual(TEXT("core + shell + 8 debris"), Used, 10);
	W.Advance(0.3f);
	const int32 Playing = Projectiles->GetPlayingFlashCount();
	const int32 Inside = Projectiles->CountPlayingFlashesNear(Here, 350.f);
	AddInfo(FString::Printf(TEXT("after 0.3 s: %d playing, %d within 350 cm"), Playing, Inside));
	TestTrue(TEXT("debris has flown outward"), Playing >= 8 && Playing - Inside >= 6);
	W.Advance(1.f);
	TestEqual(TEXT("all gone within a second"), Projectiles->GetPlayingFlashCount(), 0);

	// A rocket hitting a wall explodes (more than one flash at the wall), a laser only flashes.
	const TStrongObjectPtr<UTDWeaponFX> NoArt(NewObject<UTDWeaponFX>());
	const auto Rocket = TDCombatPlaceholderTest::FireAndCountBySite(TDWeaponIds::SpikeRockets, NoArt.Get(), 1.f / 60.f);
	TestTrue(FString::Printf(TEXT("rocket impact explodes (%d flashes at the wall)"), Rocket.Impact), Rocket.Impact >= 3);
	return true;
}

// Colour code (T5's on-foot journey check: Trooper bolts drew in Rift magenta as 1 m balls). Squad shots in their ship's
// colour, every enemy shot Armada red-orange, deflected shots in the deflector's colour; on-foot bolts read as bolts.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderColourCode, "TriadDrift.Combat.Placeholder.ColourCode", TD_TEST_FLAGS)
bool FTDCombatPlaceholderColourCode::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	auto DrawnColor = [](const ATDProjectile* P)
	{
		FLinearColor C = FLinearColor::Black;
		const UMaterialInstanceDynamic* MID = P ? Cast<UMaterialInstanceDynamic>(P->GetPlaceholderMesh()->GetMaterial(0)) : nullptr;
		if (MID)
		{
			MID->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), C);
		}
		return C;
	};
	auto SameHue = [](const FLinearColor& Drawn, const FLinearColor& Want)
	{
		// Drawn = colour * glow: compare the normalised colour.
		const float Scale = FMath::Max3(Drawn.R, Drawn.G, Drawn.B) / FMath::Max(FMath::Max3(Want.R, Want.G, Want.B), KINDA_SMALL_NUMBER);
		return Scale > 0.f && Drawn.Equals(Want * Scale, 0.02f * Scale);
	};
	const FLinearColor Enemy = UTDStatics::TeamColor(ETDTeam::Enemy);

	// 1. A Trooper-style bolt (the spec's default colour is Rift magenta) fired by a non-spacecraft: enemy colour, bolt size.
	AActor* Trooper = W.Spawn<ACharacter>(FVector(0.f, 0.f, 100.f)); // crew are characters
	FTDProjectileSpec Bolt;
	Bolt.Speed = 2200.f;
	Bolt.CollisionRadius = 22.f;
	Bolt.bInheritShooterVelocity = false;
	ATDProjectile* P = ATDProjectile::Launch(W.GetWorld(), Bolt, FTransform(FVector(0.f, 0.f, 1000.f)), Trooper, ETDTeam::Enemy, nullptr, FVector::ZeroVector, 1.f, false);
	if (!TestNotNull(TEXT("trooper bolt"), P))
	{
		return false;
	}
	TestTrue(TEXT("enemy bolt: look colour is the enemy colour"), P->GetPlaceholderLookColor().Equals(Enemy));
	TestTrue(FString::Printf(TEXT("enemy bolt: drawn in the enemy colour (%s)"), *DrawnColor(P).ToString()), SameHue(DrawnColor(P), Enemy));
	TestTrue(TEXT("enemy bolt: not Rift magenta"), !SameHue(DrawnColor(P), UTDStatics::ShipColor(ETDShipId::Rift)));
	TestTrue(TEXT("on foot: bolt look"), P->UsesOnFootLook());
	const FTDStreakLook& Look = P->GetStreakLook();
	TestTrue(FString::Printf(TEXT("on foot: thin bolt (%.0f cm thick, %.0f long)"), Look.Thickness, Look.Length),
		Look.bStreak && Look.Thickness <= 16.f && Look.Length >= 4.f * Look.Thickness);
	const float HeadCm = P->GetPlaceholderHead()->GetRelativeScale3D().X * 100.f;
	TestTrue(FString::Printf(TEXT("on foot: head %.0f cm, not a ball"), HeadCm), HeadCm <= 30.f);

	// 2. Deflected by a non-ship (the blade): the blade's colour. Deflected by a ship (dome / portal): that ship's colour.
	P->Deflect(Trooper, FVector(-1.f, 0.f, 0.f), ETDTeam::Player, 1.f);
	TestTrue(TEXT("deflected by the blade: blade colour"), P->GetPlaceholderLookColor().Equals(FTDCombatPlaceholders::BladeColorOf(Trooper)));
	TestTrue(TEXT("deflected by the blade: drawn in it"), SameHue(DrawnColor(P), FTDCombatPlaceholders::BladeColorOf(Trooper)));
	P->Kill(false);

	// 3. Every enemy weapon in space: Armada red-orange (mines were pink-red).
	for (const FName Id : TDCombatPlaceholderTest::EnemyWeaponIds)
	{
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(Id).Projectile;
		Spec.FX = nullptr;
		ATDProjectile* E = ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, ETDTeam::Enemy);
		if (E)
		{
			TestTrue(Id.ToString() + TEXT(": Armada hue"), FTDCombatPlaceholders::IsArmadaHue(E->GetPlaceholderLookColor()));
			TestFalse(Id.ToString() + TEXT(": space look"), E->UsesOnFootLook());
			E->Kill(false);
		}
	}

	// 4. Squad shots: the firing ship's colour, whatever the spec says (wingmen too).
	const ETDShipId Ships[] = { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion };
	for (const ETDShipId Id : Ships)
	{
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 3000.f * static_cast<int32>(Id), 0.f));
		if (!TestNotNull(TEXT("ship"), Ship))
		{
			continue;
		}
		Ship->SetShipId(Id);
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile; // Rift's colour in the spec
		Spec.FX = nullptr;
		ATDProjectile* S = ATDProjectile::Launch(W.GetWorld(), Spec, FTransform(Ship->GetActorLocation() + FVector(500.f, 0.f, 0.f)), Ship, ETDTeam::Player,
			nullptr, FVector::ZeroVector, 1.f, false);
		if (S)
		{
			const FLinearColor Want = UTDStatics::ShipColor(Id);
			TestTrue(FString::Printf(TEXT("ship %d shot: ship colour"), static_cast<int32>(Id)), S->GetPlaceholderLookColor().Equals(Want));
			TestTrue(FString::Printf(TEXT("ship %d shot: drawn in it"), static_cast<int32>(Id)), SameHue(DrawnColor(S), Want));
			TestFalse(TEXT("ship shot: space look"), S->UsesOnFootLook());
			// An enemy missile turned by this ship's dome flies back in this ship's colour.
			FTDProjectileSpec MSpec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile;
			MSpec.FX = nullptr;
			ATDProjectile* M = ATDProjectile::Spawn(W.GetWorld(), MSpec, FTransform(Ship->GetActorLocation() + FVector(2000.f, 0.f, 0.f)), nullptr, ETDTeam::Enemy);
			if (M)
			{
				M->Deflect(Ship, FVector(1.f, 0.f, 0.f), ETDTeam::Player, 1.f);
				TestTrue(TEXT("dome-deflected missile: ship colour"), M->GetPlaceholderLookColor().Equals(Want));
				M->Kill(false);
			}
			S->Kill(false);
		}
	}
	return true;
}

// Every enemy on-foot projectile type, specced as ATDCrewCharacter::FireBolt / ATDDummyTurret::FireAt launch it: Armada
// red-orange, a thin bolt (the Lead's live capture: Heavy pellets were 40 cm white-pink blobs hiding the Heavy at 5 m), no
// heavy bloom.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderOnFootBolts, "TriadDrift.Combat.Placeholder.OnFootBoltsEveryEnemyType", TD_TEST_FLAGS)
bool FTDCombatPlaceholderOnFootBolts::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Crew = W.Spawn<ACharacter>(FVector(0.f, 0.f, 100.f));
	struct FCase { FString Name; float Speed; float Radius; float MaxLength; };
	const FTDCrewSpec& Trooper = FTDCrewLibrary::Get(ETDCrewType::Trooper);
	const FTDCrewSpec& Heavy = FTDCrewLibrary::Get(ETDCrewType::Heavy);
	const FTDCrewSpec& Drone = FTDCrewLibrary::Get(ETDCrewType::Drone);
	const FTDCrewSpec& Warden = FTDCrewLibrary::Get(ETDCrewType::Warden);
	const FCase Cases[] = {
		{ TEXT("Trooper bolt"), Trooper.BoltSpeed, Trooper.BoltRadius, 80.f },
		{ TEXT("Heavy pellet"), Heavy.BoltSpeed, 16.f, 60.f }, // FireBolt(.., 16) in DoShotgun
		{ TEXT("Drone shot"), Drone.BoltSpeed, Drone.BoltRadius, 80.f },
		{ TEXT("Warden fan bolt"), Warden.BoltSpeed, Warden.BoltRadius, 80.f },
		{ TEXT("Dummy turret bolt"), 2200.f, 25.f, 80.f },
	};
	for (const FCase& C : Cases)
	{
		FTDProjectileSpec Spec;
		Spec.Speed = C.Speed;
		Spec.CollisionRadius = C.Radius;
		Spec.bInheritShooterVelocity = false;
		ATDProjectile* P = ATDProjectile::Launch(W.GetWorld(), Spec, FTransform(FVector(0.f, 0.f, 1000.f)), Crew, ETDTeam::Enemy, nullptr,
			FVector::ZeroVector, 1.f, false);
		if (!TestNotNull(C.Name, P))
		{
			continue;
		}
		const FTDStreakLook& Look = P->GetStreakLook();
		const float HeadCm = P->GetPlaceholderHead()->GetRelativeScale3D().X * 100.f;
		TestTrue(C.Name + TEXT(": Armada red-orange"), P->GetPlaceholderLookColor().Equals(UTDStatics::TeamColor(ETDTeam::Enemy)));
		TestTrue(C.Name + TEXT(": on-foot look"), P->UsesOnFootLook() && Look.bStreak);
		TestTrue(FString::Printf(TEXT("%s: %.1f cm thick (5-9)"), *C.Name, Look.Thickness), Look.Thickness >= 5.f && Look.Thickness <= 9.f);
		TestTrue(FString::Printf(TEXT("%s: %.0f cm long (40-%.0f)"), *C.Name, Look.Length, C.MaxLength), Look.Length >= 40.f && Look.Length <= C.MaxLength);
		TestTrue(FString::Printf(TEXT("%s: head %.1f cm (<= 13)"), *C.Name, HeadCm), HeadCm <= 13.f);
		const UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(P->GetPlaceholderMesh()->GetMaterial(0));
		FLinearColor Drawn = FLinearColor::Black;
		if (MID)
		{
			MID->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Drawn);
		}
		const float Peak = FMath::Max3(Drawn.R, Drawn.G, Drawn.B);
		TestTrue(FString::Printf(TEXT("%s: glow %.1f, no heavy bloom (<= %.1f)"), *C.Name, Peak, FTDCombatPlaceholders::OnFootGlow), Peak > 0.f
			&& Peak <= FTDCombatPlaceholders::OnFootGlow + 0.01f);
		const float Spark = FTDCombatPlaceholders::OnFootImpactRadius(Look);
		TestTrue(FString::Printf(TEXT("%s: impact spark %.0f cm radius (<= 30)"), *C.Name, Spark), Spark <= 30.f);
		if (C.Name.StartsWith(TEXT("Heavy")))
		{
			TestTrue(FString::Printf(TEXT("Heavy pellet about 6 cm thick (%.1f)"), Look.Thickness), FMath::IsNearlyEqual(Look.Thickness, 6.f, 1.f));
		}
		P->Kill(false);
	}

	// A real hit: a Heavy pellet into a body a few metres away plays a small, dim spark, not the space impact pop.
	AActor* Target = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector(400.f, 0.f, 1000.f), FVector(40.f));
	FTDProjectileSpec Pellet;
	Pellet.Speed = Heavy.BoltSpeed;
	Pellet.CollisionRadius = 16.f;
	Pellet.Damage = 5.f;
	Pellet.bInheritShooterVelocity = false;
	ATDProjectile::Launch(W.GetWorld(), Pellet, FTransform(FVector(0.f, 0.f, 1000.f)), Crew, ETDTeam::Enemy, nullptr, FVector::ZeroVector, 1.f, false);
	float BiggestCm = 0.f, BrightestGlow = 0.f;
	int32 Sparks = 0;
	for (int32 Frame = 0; Frame < 30 && Sparks == 0; ++Frame)
	{
		W.Tick(1.f / 60.f);
		for (TActorIterator<ATDCombatFlash> It(W.GetWorld()); It; ++It)
		{
			if (It->IsPlaying())
			{
				++Sparks;
				BiggestCm = FMath::Max(BiggestCm, It->GetActorScale3D().X * 50.f);
				const UMaterialInstanceDynamic* FMID = Cast<UMaterialInstanceDynamic>(It->FindComponentByClass<UStaticMeshComponent>()->GetMaterial(0));
				FLinearColor G = FLinearColor::Black;
				if (FMID)
				{
					FMID->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), G);
				}
				BrightestGlow = FMath::Max(BrightestGlow, FMath::Max3(G.R, G.G, G.B));
			}
		}
	}
	TestTrue(TEXT("pellet hit the body"), Target && UTDStatics::GetHealth(Target)->GetShield() < UTDStatics::GetHealth(Target)->MaxShield);
	TestTrue(TEXT("pellet hit plays a spark"), Sparks > 0);
	TestTrue(FString::Printf(TEXT("spark radius %.0f cm (<= 36)"), BiggestCm), BiggestCm <= 36.f);
	TestTrue(FString::Printf(TEXT("spark glow %.1f (<= on-foot glow)"), BrightestGlow), BrightestGlow <= FTDCombatPlaceholders::OnFootGlow + 0.01f);
	return true;
}

// No placeholder flash fills the frame: a big explosion right in front of the camera is capped at 0.3x its distance
// (the Lead's frame-10 white-out).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderFlashCap, "TriadDrift.Combat.Placeholder.FlashNeverFillsTheFrame", TD_TEST_FLAGS)
bool FTDCombatPlaceholderFlashCap::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	APlayerController* PC = W.SpawnLocalPlayerController();
	UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("camera manager"), PC ? PC->PlayerCameraManager.Get() : nullptr)
		|| !TestNotNull(TEXT("projectiles"), Projectiles))
	{
		return false;
	}
	W.Tick();
	const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
	const FVector At = Camera + FVector(250.f, 0.f, 0.f); // 2.5 m in front of the camera
	ATDCombatFlash* Near = Projectiles->PlayPlaceholderFlash(At, 600.f, FLinearColor::Red, 0.6f);
	const FVector Far = Camera + FVector(20000.f, 0.f, 0.f);
	ATDCombatFlash* Distant = Projectiles->PlayPlaceholderFlash(Far, 600.f, FLinearColor::Red, 0.6f);
	float NearMax = 0.f, FarMax = 0.f;
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		W.Tick(1.f / 60.f);
		if (Near && Near->IsPlaying())
		{
			NearMax = FMath::Max(NearMax, Near->GetActorScale3D().X * 50.f);
		}
		if (Distant && Distant->IsPlaying())
		{
			FarMax = FMath::Max(FarMax, Distant->GetActorScale3D().X * 50.f);
		}
	}
	TestTrue(FString::Printf(TEXT("near flash capped: %.0f cm radius at 250 cm (<= 76)"), NearMax), NearMax > 0.f && NearMax <= 250.f * ATDCombatFlash::MaxRadiusPerCameraDistance + 1.f);
	TestTrue(FString::Printf(TEXT("distant flash keeps its size: %.0f cm (> 600)"), FarMax), FarMax > 600.f);

	// A small hit spark 300 m away keeps a readable size (~20 px at 1080p): radius >= 0.007 x distance at its peak.
	ATDCombatFlash* Spark = Projectiles->PlayPlaceholderFlash(Camera + FVector(30000.f, 0.f, 0.f), 140.f, FLinearColor::White, 0.14f);
	float SparkMax = 0.f;
	for (int32 Frame = 0; Frame < 12; ++Frame)
	{
		W.Tick(1.f / 60.f);
		if (Spark && Spark->IsPlaying())
		{
			SparkMax = FMath::Max(SparkMax, Spark->GetActorScale3D().X * 50.f);
		}
	}
	TestTrue(FString::Printf(TEXT("300 m spark peaks at %.0f cm (>= 210)"), SparkMax), SparkMax >= 30000.f * FTDCombatPlaceholders::MinFlashRadiusPerCameraDistance - 1.f);
	return true;
}

// Squad tracers (the Lead's "bright weapon trails"): light squad shots are long tracers with a near-white tip that reads
// against a sky of their own colour; heavy squad heads stay small; enemy shots keep their look.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderSquadTracers, "TriadDrift.Combat.Placeholder.SquadTracersWithWhiteTips", TD_TEST_FLAGS)
bool FTDCombatPlaceholderSquadTracers::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	for (const FName Id : TDCombatPlaceholderTest::PlayerWeaponIds)
	{
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(Id).Projectile;
		Spec.FX = nullptr;
		const FTDStreakLook Space = FTDCombatPlaceholders::ComputeStreak(Spec);
		const FTDStreakLook Squad = FTDCombatPlaceholders::ComputeSquadStreak(Spec);
		if (!Space.bStreak)
		{
			continue;
		}
		if (Space.bHeavy)
		{
			TestTrue(Id.ToString() + TEXT(": heavy squad head no bigger than 1.15x thickness"), Squad.HeadDiameter <= Squad.Thickness * 1.15f + 0.1f);
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: squad tracer %.0f cm, longer than the %.0f cm streak"), *Id.ToString(), Squad.Length, Space.Length),
			Squad.Length >= Space.Length && Squad.Length <= FTDCombatPlaceholders::MaxSquadTracerCm);
		ATDProjectile* P = ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, ETDTeam::Player);
		if (!P)
		{
			continue;
		}
		const UMaterialInstanceDynamic* TipMID = Cast<UMaterialInstanceDynamic>(P->GetPlaceholderHead()->GetMaterial(0));
		const UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(P->GetPlaceholderMesh()->GetMaterial(0));
		FLinearColor Tip = FLinearColor::Black, Body = FLinearColor::Black;
		if (TipMID && BodyMID)
		{
			TipMID->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Tip);
			BodyMID->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Body);
		}
		// Whiter: the weakest channel lifted towards the strongest.
		auto Whiteness = [](const FLinearColor& C) { const float Max = FMath::Max3(C.R, C.G, C.B); return Max > 0.f ? FMath::Min3(C.R, C.G, C.B) / Max : 0.f; };
		TestTrue(FString::Printf(TEXT("%s: tip whiter than the body (%.2f vs %.2f)"), *Id.ToString(), Whiteness(Tip), Whiteness(Body)),
			Whiteness(Tip) >= Whiteness(Body) + 0.2f);
		TestTrue(Id.ToString() + TEXT(": tip visible"), P->GetPlaceholderHead()->IsVisible());
		P->Kill(false);
	}
	// Enemy shots: no tracer tip, no squad length.
	FTDProjectileSpec Enemy = UTDWeaponLibrary::Get(TDWeaponIds::EnemyPulseGuns).Projectile;
	Enemy.FX = nullptr;
	ATDProjectile* E = ATDProjectile::Spawn(W.GetWorld(), Enemy, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, ETDTeam::Enemy);
	TestTrue(TEXT("enemy shot: no tracer tip"), E && !E->HasTracerTip());
	TestTrue(TEXT("enemy shot: its own streak length"), E && FMath::IsNearlyEqual(E->GetStreakLook().Length, FTDCombatPlaceholders::ComputeStreak(Enemy).Length, 1.f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// Deterministic weapon-sound check (Lead, Nov 13; Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Sounds
// Replaces hunting for one-shots in au.DumpActiveSounds snapshots: every weapon's FX asset carries its sounds, and firing
// it once plays them (through FTDCombatCueLog, which every combat cue goes through).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Sound/SoundBase.h"
#include "TriadDrift/Combat/TDCombatCueLog.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"

namespace TDWeaponSoundTest
{
	bool HasSound(const FTDFXCue& Cue)
	{
		return !Cue.Sound.IsNull() && Cue.Sound.LoadSynchronous() != nullptr;
	}
}

// Every weapon's FX asset: Muzzle, ImpactHull and ImpactShield carry a sound; splash weapons also a Detonation sound.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWeaponSoundAssets, "TriadDrift.Combat.Sounds.EveryWeaponHasItsSounds", TD_TEST_FLAGS)
bool FTDWeaponSoundAssets::RunTest(const FString& Parameters)
{
	using TDWeaponSoundTest::HasSound;
	const TArray<FName> Ids = UTDWeaponLibrary::GetAllIds();
	AddInfo(FString::Printf(TEXT("%d weapons"), Ids.Num()));
	TestTrue(TEXT("12 weapons in the library (6 squad, 6 enemy)"), Ids.Num() >= 12);
	for (const FName Id : Ids)
	{
		const FTDWeaponSpec& Spec = UTDWeaponLibrary::Get(Id);
		const UTDWeaponFX* FX = FTDPowerPresentation::LoadWeaponFX(Id); // what the weapon component equips (DA_<Weapon>_FX)
		if (!TestNotNull(FString::Printf(TEXT("%s: has an FX asset"), *Id.ToString()), FX))
		{
			continue;
		}
		const bool bSplash = Spec.Projectile.SplashRadius > 0.f;
		AddInfo(FString::Printf(TEXT("%-24s %-28s muzzle %d  hull %d  shield %d  world %d  detonation %d%s"), *Id.ToString(), *FX->GetName(),
			HasSound(FX->Muzzle), HasSound(FX->ImpactHull), HasSound(FX->ImpactShield), HasSound(FX->ImpactWorld), HasSound(FX->Detonation),
			bSplash ? TEXT("  (splash)") : TEXT("")));
		TestTrue(FString::Printf(TEXT("%s: muzzle sound"), *Id.ToString()), HasSound(FX->Muzzle));
		TestTrue(FString::Printf(TEXT("%s: hull impact sound"), *Id.ToString()), HasSound(FX->ImpactHull));
		TestTrue(FString::Printf(TEXT("%s: shield impact sound"), *Id.ToString()), HasSound(FX->ImpactShield));
		if (bSplash)
		{
			if (!HasSound(FX->Detonation))
			{
				// A known gap in the fallback sound set (ENG-35, reported to the Lead Nov 13): flagged, not failing the gate.
				AddWarning(FString::Printf(TEXT("%s: splash weapon with NO detonation sound in %s (its blast is silent beyond the impact)"), *Id.ToString(), *FX->GetName()));
			}
		}
	}
	return true;
}

// Firing each weapon once plays its muzzle sound; impacts / detonations played within 3 s are listed per weapon.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWeaponSoundFire, "TriadDrift.Combat.Sounds.FiringPlaysTheSounds", TD_TEST_FLAGS)
bool FTDWeaponSoundFire::RunTest(const FString& Parameters)
{
	using namespace TDPowerTest;
	for (const FName Id : UTDWeaponLibrary::GetAllIds())
	{
		const FTDWeaponSpec& Spec = UTDWeaponLibrary::Get(Id);
		if (!FTDPowerPresentation::LoadWeaponFX(Id))
		{
			continue; // reported by EveryWeaponHasItsSounds
		}
		FTDTestWorld W;
		const bool bEnemyWeapon = Id.ToString().StartsWith(TEXT("Enemy_"));
		const ETDTeam Shooter = bEnemyWeapon ? ETDTeam::Enemy : ETDTeam::Player;
		const ETDTeam Victim = bEnemyWeapon ? ETDTeam::Player : ETDTeam::Enemy;
		AActor* Gunner = SpawnBody(W, Shooter, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* Weapons = NewObject<UTDWeaponComponent>(Gunner, TEXT("Weapons"));
		Weapons->RegisterComponent();
		Weapons->MuzzleOffsets = { FVector(150.f, 0.f, 0.f) };
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, Id);
		// A shielded target ahead (shield impact first), close enough for mines and slow bolts to reach.
		SpawnBody(W, Victim, FVector(2500.f, 0.f, 0.f), FVector(300.f), 1000.f, 50.f);
		W.Tick();
		FTDCombatCueLog::SetLogging(true);
		Weapons->FireOnce(ETDWeaponSlot::Primary);
		W.Advance(3.f);
		const TArray<FTDCombatCuePlay> Plays = FTDCombatCueLog::Get();
		FTDCombatCueLog::SetLogging(false);

		FString Heard;
		bool bMuzzle = false, bImpact = false;
		for (const FTDCombatCuePlay& Play : Plays)
		{
			Heard += FString::Printf(TEXT("%s%s "), FTDCombatCueLog::KindName(Play.Kind), Play.bHasSound ? TEXT("+snd") : TEXT("(silent)"));
			bMuzzle |= Play.Kind == ETDCombatCueKind::Muzzle && Play.bHasSound;
			bImpact |= Play.Kind != ETDCombatCueKind::Muzzle && Play.bHasSound;
		}
		AddInfo(FString::Printf(TEXT("%-24s %s"), *Id.ToString(), Heard.IsEmpty() ? TEXT("(no cue played)") : *Heard));
		TestTrue(FString::Printf(TEXT("%s: firing plays the muzzle sound"), *Id.ToString()), bMuzzle);
		if (!bImpact)
		{
			// Mines need a target to come near; homing shots need a lock: not every weapon connects in a 3 s headless shot.
			AddWarning(FString::Printf(TEXT("%s: no impact / detonation sound played in 3 s (did the shot connect?)"), *Id.ToString()));
		}
	}
	return true;
}

// The engineering DA_UI_Style's HitConfirm and KillConfirm play through the HUD host (no style override).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWeaponSoundConfirms, "TriadDrift.Combat.Sounds.HitAndKillConfirmPlay", TD_TEST_FLAGS)
bool FTDWeaponSoundConfirms::RunTest(const FString& Parameters)
{
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	if (!TestNotNull(TEXT("DA_UI_Style loads"), Style))
	{
		return false;
	}
	TestTrue(TEXT("HitConfirm has a sound"), TDWeaponSoundTest::HasSound(Style->HitConfirm));
	TestTrue(TEXT("KillConfirm has a sound"), TDWeaponSoundTest::HasSound(Style->KillConfirm));
	FTDTestWorld W;
	W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass());
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	if (!TestNotNull(TEXT("host"), Host) || !TestNotNull(TEXT("bus"), Bus))
	{
		return false;
	}
	Host->bAddToViewport = false;
	Host->Evaluate();
	Bus->BroadcastHitConfirmed(nullptr, false);
	W.Advance(.1f);
	Bus->BroadcastHitConfirmed(nullptr, true);
	TestEqual(TEXT("a hit plays HitConfirm"), Host->GetHitSoundsPlayed(), 1);
	TestEqual(TEXT("a kill plays KillConfirm"), Host->GetKillSoundsPlayed(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

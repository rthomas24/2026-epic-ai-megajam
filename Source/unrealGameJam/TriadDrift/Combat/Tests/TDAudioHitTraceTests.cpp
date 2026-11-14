// Board N-069 (Nick's Audio; Teammate 2): the audio hit trace marks each real hit once, with the cue picked from the
// shield state before the damage, against the two deterministic targets of TD.Audio.SpawnTargets.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "TriadDrift/Combat/TDAudioHitTrace.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAudioHitTraceTargets, "TriadDrift.Combat.Sounds.AudioHitTraceAndTargets", TD_TEST_FLAGS)
bool FTDAudioHitTraceTargets::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Up = nullptr;
	AActor* Down = nullptr;
	FTDAudioHitTrace::SpawnTargets(W.GetWorld(), FVector::ZeroVector, FVector::ForwardVector, 40.f, Up, Down);
	if (!TestNotNull(TEXT("shield-up target"), Up) || !TestNotNull(TEXT("shield-down target"), Down))
	{
		return false;
	}
	TestTrue(TEXT("names say which is which"), Up->GetName().Contains(TEXT("ShieldUp")) && Down->GetName().Contains(TEXT("ShieldDown")));
	const UTDHealthComponent* HUp = UTDStatics::GetHealth(Up);
	const UTDHealthComponent* HDown = UTDStatics::GetHealth(Down);
	TestTrue(TEXT("left: shield up"), HUp && HUp->IsShieldUp());
	TestTrue(TEXT("right: no shield"), HDown && !HDown->IsShieldUp());
	W.Tick();

	// A squad primary without splash (one projectile, one hit).
	FName Gun = NAME_None;
	for (const FName Id : UTDWeaponLibrary::GetAllIds())
	{
		const FTDWeaponSpec& Spec = UTDWeaponLibrary::Get(Id);
		if (!Id.ToString().StartsWith(TEXT("Enemy_")) && Spec.Projectile.SplashRadius <= 0.f)
		{
			Gun = Id;
			break;
		}
	}
	if (!TestFalse(TEXT("a squad gun"), Gun.IsNone()))
	{
		return false;
	}
	auto ShootAt = [&](AActor* Target)
	{
		const FVector From = Target->GetActorLocation() - FVector(1500.f, 0.f, 0.f);
		ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(Gun).Projectile, FTransform(FVector::ForwardVector.Rotation(), From), nullptr, ETDTeam::Player, nullptr);
		W.Advance(1.f);
	};

	ShootAt(Up);
	TestEqual(TEXT("off: no trace"), FTDAudioHitTrace::GetRecent().Num(), 0);
	FTDAudioHitTrace::SetOn(true);
	ShootAt(Up);
	ShootAt(Down);
	const TArray<FString> Lines = FTDAudioHitTrace::GetRecent();
	FTDAudioHitTrace::SetOn(false);
	for (const FString& L : Lines)
	{
		AddInfo(L);
	}
	TestEqual(TEXT("one line per hit"), Lines.Num(), 2);
	if (Lines.Num() == 2)
	{
		TestTrue(TEXT("shield-up hit: ImpactShield, shield=up"), Lines[0].Contains(TEXT("cue=ImpactShield")) && Lines[0].Contains(TEXT("shield=up")) && Lines[0].Contains(TEXT("ShieldUp")));
		TestTrue(TEXT("shield-down hit: ImpactHull, shield=down"), Lines[1].Contains(TEXT("cue=ImpactHull")) && Lines[1].Contains(TEXT("shield=down")) && Lines[1].Contains(TEXT("ShieldDown")));
		TestTrue(TEXT("timestamped"), Lines[0].Contains(TEXT("utc=")) && Lines[0].Contains(TEXT("world=")) && Lines[0].Contains(TEXT("frame=")));
	}
	TestTrue(TEXT("targets survive"), HUp && HUp->IsAlive() && HUp->IsShieldUp() && HDown && HDown->IsAlive());
	return true;
}

#endif

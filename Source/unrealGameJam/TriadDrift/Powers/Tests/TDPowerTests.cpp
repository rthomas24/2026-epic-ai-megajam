// Triad Drift - Powers (Teammate 2). ENG-10 ram, ENG-11 swarm + dome, ENG-12 abilities.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerLibrary.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Powers/Tests/TDPowerTestHelpers.h"

namespace TDPowerTest
{

// ================================================================================== ENG-10 ram

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRamDash, "TriadDrift.Powers.Ram.WindupDashSmashAndStop", TD_TEST_FLAGS)
bool FTDRamDash::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
	Lancer->SetTeam(ETDTeam::Player);
	UTDRamPower* Ram = AddPower<UTDRamPower>(Lancer);
	AActor* FighterA = SpawnBody(W, ETDTeam::Enemy, FVector(3000.f, 0.f, 0.f), FVector(100.f), 200.f);
	AActor* FighterB = SpawnBody(W, ETDTeam::Enemy, FVector(7000.f, 100.f, 0.f), FVector(100.f), 200.f);
	ATDEnemyShip* Gunship = SpawnEnemyShip(W, TDEnemyIds::WallGunship, FVector(16000.f, 0.f, 0.f));
	Gunship->SetActorRotation(FRotator(0.f, 180.f, 0.f)); // nose toward Lancer: the frontal shield faces the ram
	AActor* Behind = SpawnBody(W, ETDTeam::Enemy, FVector(22000.f, 0.f, 0.f), FVector(100.f), 200.f);

	UTDTestPowerListener* Listener = NewObject<UTDTestPowerListener>();
	Listener->AddToRoot();
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	if (Bus)
	{
		Bus->OnHighlightMoment.AddDynamic(Listener, &UTDTestPowerListener::OnHighlight);
	}

	TestTrue(TEXT("ram starts"), Ram->TryActivate());
	TestTrue(TEXT("invulnerable from the start"), Lancer->GetHealth()->IsInvulnerable());
	W.Advance(0.15f);
	TestEqual(TEXT("wind-up halfway"), Ram->GetWindup01(), 0.5f, 0.06f);
	TestFalse(TEXT("not dashing during wind-up"), Ram->IsDashing());
	W.Advance(0.2f);
	TestTrue(TEXT("dashing after 0.3 s"), Ram->IsDashing());

	const float StopAt = AdvanceUntil(W, [&] { return !Ram->IsPowerActive(); }, 1.5f);
	TestTrue(TEXT("dash ended"), StopAt > 0.f);
	TestEqual(TEXT("fighter A smashed (300 > 200)"), Hull(FighterA), 0.f);
	TestEqual(TEXT("fighter B smashed"), Hull(FighterB), 0.f);
	TestTrue(TEXT("RamKill highlight"), Listener->Highlights.Contains(ETDHighlightKind::RamKill));
	TestTrue(TEXT("big gunship stopped the dash"), Ram->WasStoppedByImpact());
	TestFalse(TEXT("gunship frontal shield broken"), Gunship->IsFrontalShieldUp());
	const UTDHealthComponent* GunHealth = Gunship->GetHealth();
	TestTrue(TEXT("gunship took ram + shockwave (>= 300)"), GunHealth && GunHealth->GetHull() <= GunHealth->MaxHull - 300.f);
	TestEqual(TEXT("ship behind the gunship untouched"), Hull(Behind), 200.f);
	TestEqual(TEXT("each target hit once"), Ram->GetHitThisDash().Num(), 3);

	W.Advance(0.3f);
	TestFalse(TEXT("vulnerable again after the grace window"), Lancer->GetHealth()->IsInvulnerable());
	W.Advance(4.f);
	TestFalse(TEXT("7 s cooldown"), Ram->CanActivate());
	W.Advance(2.f);
	TestTrue(TEXT("ready after 7 s"), Ram->CanActivate());

	if (Bus)
	{
		Bus->OnHighlightMoment.RemoveAll(Listener);
	}
	Listener->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRamFullDash, "TriadDrift.Powers.Ram.FullDashSpeedAndDuration", TD_TEST_FLAGS)
bool FTDRamFullDash::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
	Lancer->SetTeam(ETDTeam::Player);
	UTDRamPower* Ram = AddPower<UTDRamPower>(Lancer);
	const float Boost = Lancer->GetShipMovement()->GetFlightTuning().BoostSpeed;
	Ram->TryActivate();
	W.Advance(1.4f);
	TestTrue(TEXT("still dashing at 1.4 s"), Ram->IsDashing());
	TestTrue(TEXT("well beyond boost speed"), Lancer->GetVelocity().Size() > Boost * 2.f);
	W.Advance(0.15f);
	TestFalse(TEXT("dash over at 1.5 s"), Ram->IsPowerActive());
	TestFalse(TEXT("not stopped by impact in open space"), Ram->WasStoppedByImpact());
	return true;
}

// ================================================================================== ENG-11 Bastion

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSwarmLocks, "TriadDrift.Powers.Swarm.TwelveMissilesSplitAcrossPaintedLocks", TD_TEST_FLAGS)
bool FTDSwarmLocks::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* Targeting = NewObject<UTDTargetingComponent>(Bastion, TEXT("Targeting"));
	Targeting->RegisterComponent();
	UTDMissileSwarmPower* Swarm = AddPower<UTDMissileSwarmPower>(Bastion);
	AActor* Left = SpawnBody(W, ETDTeam::Enemy, FVector(8000.f, -1500.f, 0.f), FVector(150.f), 1000.f);
	AActor* Mid = SpawnBody(W, ETDTeam::Enemy, FVector(8000.f, 0.f, 0.f), FVector(150.f), 1000.f);
	AActor* Right = SpawnBody(W, ETDTeam::Enemy, FVector(8000.f, 1500.f, 0.f), FVector(150.f), 1000.f);

	W.Advance(1.f);
	TestTrue(TEXT("auto-painting while ready"), Targeting->IsPainting());
	TestEqual(TEXT("3 painted locks for the HUD"), Targeting->GetLockedTargets().Num(), 3);

	TestTrue(TEXT("swarm fires"), Swarm->TryActivate());
	TestEqual(TEXT("split across 3 targets"), Swarm->GetLastTargets().Num(), 3);
	TestFalse(TEXT("painting stops on cooldown"), Targeting->IsPainting());
	TestEqual(TEXT("locks consumed"), Targeting->GetLockedTargets().Num(), 0);

	W.Advance(3.f);
	TestEqual(TEXT("left: 4 x 18"), Hull(Left), 1000.f - 72.f, 0.01f);
	TestEqual(TEXT("mid: 4 x 18"), Hull(Mid), 1000.f - 72.f, 0.01f);
	TestEqual(TEXT("right: 4 x 18"), Hull(Right), 1000.f - 72.f, 0.01f);
	TestFalse(TEXT("9 s cooldown"), Swarm->CanActivate());
	W.Advance(6.1f);
	TestTrue(TEXT("ready after 9 s"), Swarm->CanActivate());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSwarmSoftTarget, "TriadDrift.Powers.Swarm.FallsBackToSoftTarget", TD_TEST_FLAGS)
bool FTDSwarmSoftTarget::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* Targeting = NewObject<UTDTargetingComponent>(Bastion, TEXT("Targeting"));
	Targeting->RegisterComponent();
	UTDMissileSwarmPower* Swarm = AddPower<UTDMissileSwarmPower>(Bastion);
	Swarm->bAutoPaint = false;
	AActor* Target = SpawnBody(W, ETDTeam::Enemy, FVector(8000.f, 0.f, 0.f), FVector(150.f), 1000.f);

	Targeting->RefreshNow();
	TestTrue(TEXT("soft target picked"), Targeting->GetCurrentTarget() == Target);
	Swarm->TryActivate();
	W.Advance(3.f);
	TestEqual(TEXT("all 12 on the soft target"), Hull(Target), 1000.f - 12.f * 18.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDome, "TriadDrift.Powers.Dome.ProtectsRegensBlocksAndExpires", TD_TEST_FLAGS)
bool FTDDome::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f), 180.f, 160.f);
	AActor* Wingman = SpawnBody(W, ETDTeam::Player, FVector(1500.f, 0.f, 0.f), FVector(100.f), 100.f, 100.f);
	AActor* Gunship = SpawnBody(W, ETDTeam::Enemy, FVector(9000.f, 0.f, 0.f), FVector(200.f), 400.f);
	UTDShieldDomePower* DomePower = AddPower<UTDShieldDomePower>(Bastion);
	UTDPowerSubsystem* Powers = UTDPowerSubsystem::Get(W.GetWorld());

	// Wingman's shield knocked down; its own regen starts 3 s later at 25 %/s.
	UTDStatics::ApplyTDDamage(Wingman, 60.f, ETDDamageKind::Energy, Gunship, Gunship);
	W.Advance(3.05f);
	const float ShieldBefore = Shield(Wingman);

	TestTrue(TEXT("dome up"), DomePower->TryActivate());
	ATDShieldDome* Dome = DomePower->GetDome();
	TestNotNull(TEXT("dome actor"), Dome);
	TestTrue(TEXT("wingmen can find the dome"), Powers && Powers->FindDomeCovering(Wingman->GetActorLocation()) == Dome);
	TestNull(TEXT("outside the 3000 cm radius isn't covered"), Powers ? Powers->FindDomeCovering(FVector(3500.f, 0.f, 0.f)) : nullptr);

	W.Advance(1.f);
	TestEqual(TEXT("shield regen doubled inside (50 %/s)"), Shield(Wingman) - ShieldBefore, 50.f, 4.f);

	TestEqual(TEXT("no damage inside"), UTDStatics::ApplyTDDamage(Wingman, 50.f, ETDDamageKind::Energy, Gunship, Gunship), 0.f);

	ATDProjectile* Bolt = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile,
		FTransform(FRotator(0.f, 180.f, 0.f), FVector(8500.f, 0.f, 0.f)), Gunship, ETDTeam::Enemy);
	ATDProjectile* Missile = LaunchMissile(W, FVector(8000.f, 600.f, 0.f), -FVector::ForwardVector, Gunship, Wingman);
	W.Advance(1.f);
	TestFalse(TEXT("enemy bolt destroyed on the dome"), Bolt && Bolt->IsActive());
	TestTrue(TEXT("enemy missile deflected"), Missile && Missile->IsActive() && Missile->GetTeam() == ETDTeam::Player);

	// Follows slowly: Bastion jumps 10 000 cm, the dome drifts after it.
	Bastion->SetActorLocation(FVector(0.f, 10000.f, 0.f));
	const FVector DomeBefore = Dome->GetActorLocation();
	W.Advance(1.f);
	const float Drift = FVector::Dist(Dome->GetActorLocation(), DomeBefore);
	TestTrue(TEXT("dome follows slowly (~2500 cm/s)"), Drift > 2000.f && Drift < 3000.f);

	W.Advance(3.2f);
	TestFalse(TEXT("dome gone after 6 s"), IsValid(Dome));
	TestFalse(TEXT("power ended"), DomePower->IsPowerActive());
	W.Advance(15.7f);
	TestFalse(TEXT("16 s cooldown from collapse"), DomePower->CanActivate());
	W.Advance(0.5f);
	TestTrue(TEXT("ready after 16 s"), DomePower->CanActivate());
	return true;
}

// ================================================================================== ENG-12 abilities

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAbilitySlot, "TriadDrift.Powers.Ability.SlotFilledOnUnlock", TD_TEST_FLAGS)
bool FTDAbilitySlot::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Bastion = W.Spawn<ATDShip>(FVector::ZeroVector);
	Bastion->SetShipId(ETDShipId::Bastion);
	UTDAbilityComponent* Slot = NewObject<UTDAbilityComponent>(Bastion, TEXT("AbilitySlot"));
	Slot->RegisterComponent();
	TestTrue(TEXT("starts empty"), Slot->GetAbility() == ETDAbilityId::None);
	TestNull(TEXT("no ability power"), UTDPowerComponent::FindPower(Bastion, ETDPowerInput::Ability));

	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	TestNotNull(TEXT("bus"), Bus);
	Bus->BroadcastAbilityUnlocked(ETDAbilityId::Cloak, ETDShipId::Rift);
	TestTrue(TEXT("another ship's unlock is ignored"), Slot->GetAbility() == ETDAbilityId::None);
	Bus->BroadcastAbilityUnlocked(ETDAbilityId::GravityWell, ETDShipId::Bastion);
	TestTrue(TEXT("Gravity Well installed"), Slot->GetAbility() == ETDAbilityId::GravityWell);
	TestTrue(TEXT("routable in the Ability slot"), Cast<UTDGravityWellPower>(UTDPowerComponent::FindPower(Bastion, ETDPowerInput::Ability)) != nullptr);

	Slot->SetAbility(ETDAbilityId::EMPPulse);
	TestTrue(TEXT("replaced"), Cast<UTDEMPPower>(UTDPowerComponent::FindPower(Bastion, ETDPowerInput::Ability)) != nullptr);
	TInlineComponentArray<UTDPowerComponent*> Powers(Bastion);
	int32 AbilityPowers = 0;
	for (UTDPowerComponent* P : Powers)
	{
		AbilityPowers += (IsValid(P) && P->Slot == ETDPowerInput::Ability) ? 1 : 0;
	}
	TestEqual(TEXT("only one ability power"), AbilityPowers, 1);

	// Wingmen never use powers: not player-flown -> refused.
	UTDPowerComponent* Emp = UTDPowerComponent::FindPower(Bastion, ETDPowerInput::Ability);
	TestFalse(TEXT("AI-flown ship can't fire its ability"), Emp && Emp->TryActivate());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGravityWell, "TriadDrift.Powers.Ability.GravityWellPullsDamagesImplodes", TD_TEST_FLAGS)
bool FTDGravityWell::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f));
	UTDGravityWellPower* Well = AddPower<UTDGravityWellPower>(Bastion);
	// Orb: spawned 400 cm ahead, flies 5000 cm/s for 0.8 s -> vortex centred at x = 4400.
	const FVector Centre(4400.f, 0.f, 0.f);
	AActor* Caught = SpawnBody(W, ETDTeam::Enemy, Centre + FVector(1500.f, 2000.f, 0.f), FVector(100.f), 1000.f);
	AActor* Far = SpawnBody(W, ETDTeam::Enemy, Centre + FVector(0.f, 9000.f, 0.f), FVector(100.f), 1000.f);
	AActor* Friendly = SpawnBody(W, ETDTeam::Player, Centre + FVector(0.f, -2000.f, 0.f), FVector(100.f), 100.f);

	TestTrue(TEXT("well launched"), Well->TryActivate());
	W.Advance(0.85f);
	ATDGravityWell* Vortex = Well->GetLastWell();
	TestTrue(TEXT("vortex open"), Vortex && Vortex->IsVortexActive());
	TestTrue(TEXT("vortex where the orb stopped"), Vortex && Vortex->GetActorLocation().Equals(Centre, 1.f));

	ATDProjectile* Bolt = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile,
		FTransform(FRotator::ZeroRotator, Centre + FVector(-3000.f, 3000.f, 0.f)), Far, ETDTeam::Enemy);
	W.Advance(3.f);
	TestTrue(TEXT("caught enemy pulled in"), FVector::Dist(Caught->GetActorLocation(), Centre) < 1000.f);
	TestFalse(TEXT("enemy projectile swallowed"), Bolt && Bolt->IsActive());
	TestEqual(TEXT("far enemy not pulled"), (float)FVector::Dist(Far->GetActorLocation(), Centre), 9000.f, 1.f);

	W.Advance(2.3f);
	TestFalse(TEXT("vortex gone after 5 s"), IsValid(Vortex));
	TestEqual(TEXT("10 dps for 5 s + 120 implosion"), Hull(Caught), 1000.f - 50.f - 120.f, 3.f);
	TestEqual(TEXT("far enemy untouched"), Hull(Far), 1000.f);
	TestEqual(TEXT("friendly untouched"), Hull(Friendly), 100.f);
	TestFalse(TEXT("18 s cooldown"), Well->CanActivate());
	W.Advance(12.f);
	TestTrue(TEXT("ready after 18 s"), Well->CanActivate());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCloak, "TriadDrift.Powers.Ability.CloakUntargetableFirstShotTriple", TD_TEST_FLAGS)
bool FTDCloak::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f), 100.f, 100.f);
	UTDWeaponComponent* Weapons = NewObject<UTDWeaponComponent>(Rift, TEXT("Weapons"));
	Weapons->RegisterComponent();
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
	UTDCloakPower* Cloak = AddPower<UTDCloakPower>(Rift);
	UTDShieldDomePower* OtherPower = AddPower<UTDShieldDomePower>(Rift);
	AActor* Interceptor = SpawnBody(W, ETDTeam::Enemy, FVector(30000.f, 0.f, 0.f), FVector(100.f));
	ATDProjectile* Missile = LaunchMissile(W, FVector(20000.f, 0.f, 0.f), -FVector::ForwardVector, Interceptor, Rift);
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(W.GetWorld());

	TestTrue(TEXT("cloak on"), Cloak->TryActivate());
	TestTrue(TEXT("cloaked"), Cloak->IsCloaked());
	TArray<AActor*> Hostiles;
	Registry->GetHostilesOf(ETDTeam::Enemy, Hostiles);
	TestFalse(TEXT("AI can't find Rift"), Hostiles.Contains(Rift));
	W.Advance(0.5f);
	TestTrue(TEXT("missile lost its lock"), Missile && Missile->GetHomingTarget() == nullptr);

	Weapons->FireOnce(ETDWeaponSlot::Primary);
	const ATDProjectile* Shot = Weapons->GetLastFiredProjectile(0);
	TestTrue(TEXT("first shot out does 3x"), Shot && FMath::IsNearlyEqual(Shot->GetDamageMultiplier(), 3.f));
	TestFalse(TEXT("firing breaks cloak"), Cloak->IsCloaked());
	Hostiles.Reset();
	Registry->GetHostilesOf(ETDTeam::Enemy, Hostiles);
	TestTrue(TEXT("targetable again"), Hostiles.Contains(Rift));
	W.Advance(14.8f);
	TestFalse(TEXT("15 s cooldown"), Cloak->CanActivate());
	W.Advance(0.4f);
	TestTrue(TEXT("ready after 15 s"), Cloak->CanActivate());

	// Another power breaks it too; expiring without a shot leaves no banked 3x.
	Cloak->TryActivate();
	OtherPower->TryActivate();
	TestFalse(TEXT("using another power breaks cloak"), Cloak->IsCloaked());
	Cloak->ResetCooldown();
	Cloak->TryActivate();
	W.Advance(6.1f);
	TestFalse(TEXT("cloak lasts 6 s"), Cloak->IsCloaked());
	W.Advance(0.2f);
	Weapons->FireOnce(ETDWeaponSlot::Primary);
	const ATDProjectile* Later = Weapons->GetLastFiredProjectile(0);
	TestTrue(TEXT("no 3x after an unused cloak"), Later && FMath::IsNearlyEqual(Later->GetDamageMultiplier(), 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEmp, "TriadDrift.Powers.Ability.EMPStunsDropsShieldsFizzlesMissiles", TD_TEST_FLAGS)
bool FTDEmp::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Lancer = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f), 70.f, 80.f);
	UTDEMPPower* Emp = AddPower<UTDEMPPower>(Lancer);
	ATDEnemyShip* Gunship = SpawnEnemyShip(W, TDEnemyIds::WallGunship, FVector(3000.f, 0.f, 0.f));
	ATDEnemyShip* Near = SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(0.f, 4000.f, 0.f));
	ATDEnemyShip* Distant = SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(0.f, -9000.f, 0.f));
	ATDProjectile* Missile = LaunchMissile(W, FVector(2000.f, 2000.f, 0.f), FVector(-1.f, -1.f, 0.f).GetSafeNormal(), Near, Lancer);
	TestTrue(TEXT("gunship shield starts up"), Gunship && Gunship->IsFrontalShieldUp());

	TestTrue(TEXT("pulse"), Emp->TryActivate());
	TestEqual(TEXT("2 enemies in 6000 cm"), Emp->GetLastAffectedCount(), 2);
	TestTrue(TEXT("gunship stunned"), Gunship->IsStunned());
	TestTrue(TEXT("fighter stunned"), Near->IsStunned());
	TestFalse(TEXT("distant fighter unaffected"), Distant->IsStunned());
	TestFalse(TEXT("gunship frontal shield down"), Gunship->IsFrontalShieldUp());
	TestFalse(TEXT("missile fizzled"), Missile && Missile->IsActive());

	W.Advance(3.1f);
	TestFalse(TEXT("stun lasts 3 s"), Gunship->IsStunned());
	W.Advance(1.5f);
	TestFalse(TEXT("shield still down at 4.6 s"), Gunship->IsFrontalShieldUp());
	W.Advance(9.6f);
	TestTrue(TEXT("ready after 14 s"), Emp->CanActivate());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInstallPowers, "TriadDrift.Powers.Library.InstallShipPowersIdempotent", TD_TEST_FLAGS)
bool FTDInstallPowers::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	struct FCase { ETDShipId Id; UClass* Sig1; UClass* Sig2; };
	const FCase Cases[] = {
		{ ETDShipId::Rift, UTDPortalBlasterPower::StaticClass(), nullptr },
		{ ETDShipId::Lancer, UTDRamPower::StaticClass(), nullptr },
		{ ETDShipId::Bastion, UTDMissileSwarmPower::StaticClass(), UTDShieldDomePower::StaticClass() } };
	for (const FCase& C : Cases)
	{
		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		Ship->SetShipId(C.Id);
		UTDPowerLibrary::InstallShipPowers(Ship, C.Id);
		UTDPowerLibrary::InstallShipPowers(Ship, C.Id); // second call (respawn) must not double up
		UTDPowerComponent* S1 = UTDPowerComponent::FindPower(Ship, ETDPowerInput::Signature1);
		UTDPowerComponent* S2 = UTDPowerComponent::FindPower(Ship, ETDPowerInput::Signature2);
		TestTrue(TEXT("signature 1 class"), S1 && S1->GetClass() == C.Sig1);
		TestTrue(TEXT("signature 2 class"), C.Sig2 ? (S2 && S2->GetClass() == C.Sig2) : S2 == nullptr);
		TInlineComponentArray<UTDPowerComponent*> Powers(Ship);
		TestEqual(TEXT("no duplicates"), Powers.Num(), C.Sig2 ? 2 : 1);
		TInlineComponentArray<UTDAbilityComponent*> Slots(Ship);
		TestEqual(TEXT("one ability slot"), Slots.Num(), 1);
	}
	return true;
}

} // namespace TDPowerTest

#endif // WITH_DEV_AUTOMATION_TESTS

// Triad Drift - Powers (Teammate 2). ENG-10 ram, ENG-11 swarm + dome, ENG-12 abilities.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Powers/TDPortalTransit.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Powers/Tests/TDPowerTestHelpers.h"

namespace TDPowerTest
{

// ================================================================================== ENG-10 ram

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRamDash, "TriadDrift.Powers.Ram.ThroughRealFightersStopsOnGunship", TD_TEST_FLAGS)
bool FTDRamDash::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		const FString Fps = FString::Printf(TEXT("%.0f fps: "), 1.f / Step);
		FTDTestWorld W;
		ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
		Lancer->SetTeam(ETDTeam::Player);
		UTDRamPower* Ram = AddPower<UTDRamPower>(Lancer);
		const float Boost = Lancer->GetShipMovement()->GetFlightTuning().BoostSpeed;
		// Real fighters on the line, flying head-on at Lancer: they block the Ship channel.
		// Weak: a destroyed fighter can be garbage-collected while the world ticks.
		TArray<TWeakObjectPtr<ATDEnemyShip>> Fighters;
		for (const float X : { 5000.f, 9000.f, 13000.f })
		{
			ATDEnemyShip* F = SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(X, 0.f, 0.f));
			F->SetActorRotation(FRotator(0.f, 180.f, 0.f));
			Fighters.Add(F);
		}
		ATDEnemyShip* Gunship = SpawnEnemyShip(W, TDEnemyIds::WallGunship, FVector(26000.f, 0.f, 0.f));
		Gunship->SetActorRotation(FRotator(0.f, 180.f, 0.f));
		Gunship->GetShipMovement()->SetComponentTickEnabled(false); // holds station: its own collisions can't add damage
		const float GunHull = Gunship->GetHealth()->GetHull();
		AActor* Behind = SpawnBody(W, ETDTeam::Enemy, FVector(34000.f, 0.f, 0.f), FVector(100.f), 200.f);

		TestTrue(*(Fps + TEXT("ram starts")), Ram->TryActivate());
		float MinDashSpeed = TNumericLimits<float>::Max();
		float T = 0.f;
		while (Ram->IsPowerActive() && T < 3.f)
		{
			W.Tick(Step);
			T += Step;
			if (Ram->IsDashing() && Ram->GetActiveElapsed() > Ram->WindupSeconds + 0.1f)
			{
				MinDashSpeed = FMath::Min(MinDashSpeed, (float)Lancer->GetVelocity().Size());
			}
		}
		// Regression probe: destroyed fighters may be collected mid-test; the weak pointers must cope.
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		for (const TWeakObjectPtr<ATDEnemyShip>& F : Fighters)
		{
			TestTrue(*(Fps + TEXT("fighter in the line destroyed")), !F.IsValid() || F->GetHealth()->IsDead());
		}
		TestTrue(*(Fps + TEXT("passed through: speed never dropped below 2.5x boost")), MinDashSpeed > Boost * 2.5f);
		TestTrue(*(Fps + TEXT("the gunship stopped the dash")), Ram->WasStoppedByImpact());
		TestFalse(*(Fps + TEXT("gunship frontal shield broken")), Gunship->IsFrontalShieldUp());
		TestEqual(*(Fps + TEXT("gunship takes exactly 300 (shockwave spares the stopper)")), Gunship->GetHealth()->GetHull(), GunHull - 300.f, 1.f);
		TestEqual(*(Fps + TEXT("ship behind the gunship untouched")), Hull(Behind), 200.f);
		TestTrue(*(Fps + TEXT("Lancer stopped outside the gunship")), Lancer->GetActorLocation().X < 26000.f);
		TestEqual(*(Fps + TEXT("each target hit once")), Ram->GetHitThisDash().Num(), 4);

		// Still invulnerable while the speed bleeds off, vulnerable once back to boost speed.
		W.Advance(0.2f, Step);
		TestTrue(*(Fps + TEXT("invulnerable while above boost speed")), Lancer->GetVelocity().Size() <= Boost * 1.02f || Lancer->GetHealth()->IsInvulnerable());
		W.Advance(2.f, Step);
		TestFalse(*(Fps + TEXT("vulnerable after the grace")), Lancer->GetHealth()->IsInvulnerable());
		TestFalse(*(Fps + TEXT("no ram override left behind")), Lancer->GetShipMovement()->HasSpeedOverrideFor(UTDRamPower::RamOverrideKey));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRamFullDash, "TriadDrift.Powers.Ram.InstantKickDistanceRockAndRefusals", TD_TEST_FLAGS)
bool FTDRamFullDash::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	// Open space: instant kick to 3x boost; distance = 1.2 s of dash.
	{
		FTDTestWorld W;
		ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
		Lancer->SetTeam(ETDTeam::Player);
		UTDRamPower* Ram = AddPower<UTDRamPower>(Lancer);
		const float DashSpeed = Lancer->GetShipMovement()->GetFlightTuning().BoostSpeed * 3.f;
		Ram->TryActivate();
		W.Advance(0.6f, PowerFrameStep());
		TestEqual(TEXT("3x boost speed 0.3 s into the dash"), (float)Lancer->GetVelocity().Size(), DashSpeed, DashSpeed * 0.03f);
		AdvanceUntil(W, [&] { return !Ram->IsPowerActive(); }, 2.f);
		TestEqual(TEXT("dash distance = 1.2 s at 3x boost"), Ram->GetLastDashDistance(), DashSpeed * 1.2f, DashSpeed * 1.2f * 0.06f);
		TestFalse(TEXT("open space: not stopped"), Ram->WasStoppedByImpact());
	}
	// A rock stops it with a shockwave that hits ships near the impact.
	{
		FTDTestWorld W;
		ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
		Lancer->SetTeam(ETDTeam::Player);
		UTDRamPower* Ram = AddPower<UTDRamPower>(Lancer);
		SpawnWall(W, FVector(12000.f, 0.f, 0.f), FVector(1000.f, 3000.f, 3000.f));
		AActor* Near = SpawnBody(W, ETDTeam::Enemy, FVector(10400.f, 1200.f, 0.f), FVector(100.f), 1000.f);
		Ram->TryActivate();
		const float EndAt = AdvanceUntil(W, [&] { return !Ram->IsPowerActive(); }, 2.f);
		TestTrue(TEXT("rock ends the dash early"), EndAt > 0.f && EndAt < 1.45f && Ram->WasStoppedByImpact());
		TestEqual(TEXT("shockwave hits the ship by the rock"), Hull(Near), 850.f, 0.5f);
		TestTrue(TEXT("Lancer is not inside the rock"), Lancer->GetActorLocation().X < 11000.f);
	}
	// Refusals: someone else's speed override, a busy hyperdrive.
	{
		FTDTestWorld W;
		ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
		Lancer->SetTeam(ETDTeam::Player);
		UTDRamPower* Ram = AddPower<UTDRamPower>(Lancer);
		// Another owner's override (atmosphere entry, say) underneath: the ram stacks on top and restores it.
		Lancer->SetSpeedOverrideFor(TEXT("Atmosphere"), 5000.f, 1.f);
		TestTrue(TEXT("ram stacks on another override"), Ram->TryActivate());
		W.Advance(0.5f, PowerFrameStep());
		TestTrue(TEXT("ram's own key is flown"), Lancer->GetShipMovement()->GetActiveSpeedOverrideOwner() == UTDRamPower::RamOverrideKey);
		AdvanceUntil(W, [&] { return !Ram->IsPowerActive(); }, 2.f);
		TestTrue(TEXT("the override underneath is restored, not wiped"), Lancer->GetShipMovement()->GetActiveSpeedOverrideOwner() == FName(TEXT("Atmosphere")));
		Lancer->ClearSpeedOverrideFor(TEXT("Atmosphere"));

		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 50000.f, 0.f));
		Ship->SetTeam(ETDTeam::Player);
		UTDRamPower* ShipRam = AddPower<UTDRamPower>(Ship);
		if (Ship->GetHyperdrive() && Ship->GetHyperdrive()->StartSpool())
		{
			TestFalse(TEXT("refused while the hyperdrive spools"), ShipRam->TryActivate());
		}
		else
		{
			AddInfo(TEXT("hyperdrive could not spool in a bare test world; refusal covered by IsHyperdriveBusy"));
		}
	}
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSwarmLocks, "TriadDrift.Powers.Swarm.TwelveMissilesSplitAcrossPaintedLocks", TD_TEST_FLAGS)
bool FTDSwarmLocks::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	FTDTestWorld W;
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* Targeting = NewObject<UTDTargetingComponent>(Bastion, TEXT("Targeting"));
	Targeting->RegisterComponent();
	UTDMissileSwarmPower* Swarm = AddPower<UTDMissileSwarmPower>(Bastion);
	AActor* Left = SpawnBody(W, ETDTeam::Enemy, FVector(8000.f, -1500.f, 0.f), FVector(150.f), 1000.f);
	AActor* Mid = SpawnBody(W, ETDTeam::Enemy, FVector(8000.f, 0.f, 0.f), FVector(150.f), 1000.f);
	AActor* Right = SpawnBody(W, ETDTeam::Enemy, FVector(8000.f, 1500.f, 0.f), FVector(150.f), 1000.f);

	W.Advance(1.f, PowerFrameStep());
	TestTrue(TEXT("auto-painting while ready"), Targeting->IsPainting());
	TestEqual(TEXT("3 painted locks for the HUD"), Targeting->GetLockedTargets().Num(), 3);

	TestTrue(TEXT("swarm fires"), Swarm->TryActivate());
	TestEqual(TEXT("split across 3 targets"), Swarm->GetLastTargets().Num(), 3);
	TestFalse(TEXT("painting stops on cooldown"), Targeting->IsPainting());
	TestEqual(TEXT("locks consumed"), Targeting->GetLockedTargets().Num(), 0);

	W.Advance(3.f, PowerFrameStep());
	TestEqual(TEXT("left: 4 x 18"), Hull(Left), 1000.f - 72.f, 0.01f);
	TestEqual(TEXT("mid: 4 x 18"), Hull(Mid), 1000.f - 72.f, 0.01f);
	TestEqual(TEXT("right: 4 x 18"), Hull(Right), 1000.f - 72.f, 0.01f);
	TestFalse(TEXT("9 s cooldown"), Swarm->CanActivate());
	W.Advance(6.1f, PowerFrameStep());
	TestTrue(TEXT("ready after 9 s"), Swarm->CanActivate());
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSwarmSoftTarget, "TriadDrift.Powers.Swarm.FallsBackToSoftTarget", TD_TEST_FLAGS)
bool FTDSwarmSoftTarget::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
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
	W.Advance(3.f, PowerFrameStep());
	TestEqual(TEXT("all 12 on the soft target"), Hull(Target), 1000.f - 12.f * 18.f, 0.01f);
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDome, "TriadDrift.Powers.Dome.ProtectsRegensBlocksAndExpires", TD_TEST_FLAGS)
bool FTDDome::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	FTDTestWorld W;
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f), 180.f, 160.f);
	AActor* Wingman = SpawnBody(W, ETDTeam::Player, FVector(1500.f, 0.f, 0.f), FVector(100.f), 100.f, 100.f);
	AActor* Gunship = SpawnBody(W, ETDTeam::Enemy, FVector(9000.f, 0.f, 0.f), FVector(200.f), 400.f);
	UTDShieldDomePower* DomePower = AddPower<UTDShieldDomePower>(Bastion);
	UTDPowerSubsystem* Powers = UTDPowerSubsystem::Get(W.GetWorld());

	// Wingman's shield knocked down; its own regen starts 3 s later at 25 %/s.
	UTDStatics::ApplyTDDamage(Wingman, 60.f, ETDDamageKind::Energy, Gunship, Gunship);
	W.Advance(3.05f, PowerFrameStep());
	const float ShieldBefore = Shield(Wingman);

	TestTrue(TEXT("dome up"), DomePower->TryActivate());
	ATDShieldDome* Dome = DomePower->GetDome();
	TestNotNull(TEXT("dome actor"), Dome);
	TestTrue(TEXT("wingmen can find the dome"), Powers && Powers->FindDomeCovering(Wingman->GetActorLocation()) == Dome);
	TestNull(TEXT("outside the 3000 cm radius isn't covered"), Powers ? Powers->FindDomeCovering(FVector(3500.f, 0.f, 0.f)) : nullptr);

	W.Advance(1.f, PowerFrameStep());
	TestEqual(TEXT("shield regen doubled inside (50 %/s)"), Shield(Wingman) - ShieldBefore, 50.f, 4.f);

	TestEqual(TEXT("no damage inside"), UTDStatics::ApplyTDDamage(Wingman, 50.f, ETDDamageKind::Energy, Gunship, Gunship), 0.f);

	// The bolt aims through empty dome space (no ship on its line): only the shell can stop it.
	ATDProjectile* Bolt = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile,
		FTransform(FRotator(0.f, 180.f, 0.f), FVector(8500.f, -1200.f, 0.f)), Gunship, ETDTeam::Enemy);
	ATDProjectile* Missile = LaunchMissile(W, FVector(8000.f, 600.f, 0.f), -FVector::ForwardVector, Gunship, Wingman);
	FVector BoltLast = FVector::ZeroVector;
	const float BoltDiedAt = AdvanceUntil(W, [&] { if (Bolt && Bolt->IsActive()) { BoltLast = Bolt->GetActorLocation(); return false; } return true; }, 1.f);
	TestTrue(TEXT("enemy bolt destroyed"), BoltDiedAt > 0.f);
	const float DistFromCentre = (float)FVector::Dist(BoltLast, Dome->GetActorLocation());
	TestTrue(TEXT("...on the shell, not a frame-step inside it"), DistFromCentre > 3000.f - 50.f && DistFromCentre < 3000.f + 12000.f / 60.f + 50.f);
	W.Advance(0.5f, PowerFrameStep());
	TestTrue(TEXT("enemy missile deflected"), Missile && Missile->IsActive() && Missile->GetTeam() == ETDTeam::Player);

	// Follows Bastion with a lag: never left behind at cruise.
	ATDSpacecraft* Pace = nullptr;
	Bastion->SetActorLocation(FVector(0.f, 10000.f, 0.f));
	W.Advance(0.05f, PowerFrameStep());
	TestTrue(TEXT("dome lags a sudden jump"), FVector::Dist(Dome->GetActorLocation(), Bastion->GetActorLocation()) > 5000.f);
	W.Advance(0.95f, PowerFrameStep());
	TestTrue(TEXT("...and catches up within a second"), FVector::Dist(Dome->GetActorLocation(), Bastion->GetActorLocation()) < 500.f);
	for (int32 i = 0; i < 60; ++i) // Bastion cruising at 4800 cm/s for 1 s
	{
		Bastion->SetActorLocation(Bastion->GetActorLocation() + FVector(4800.f / 60.f, 0.f, 0.f));
		W.Tick(PowerFrameStep());
	}
	TestTrue(TEXT("a cruising Bastion stays inside its own dome"), Dome->Covers(Bastion->GetActorLocation()));
	(void)Pace;

	const TWeakObjectPtr<ATDShieldDome> DomeWeak = Dome; // the dome is destroyed and may be collected
	const float GoneAt = AdvanceUntil(W, [&] { return !DomeWeak.IsValid(); }, 3.f);
	TestTrue(TEXT("dome gone after 6 s"), GoneAt > 0.f);
	TestFalse(TEXT("power ended"), DomePower->IsPowerActive());
	W.Advance(15.7f, PowerFrameStep());
	TestFalse(TEXT("16 s cooldown from collapse"), DomePower->CanActivate());
	W.Advance(0.5f, PowerFrameStep());
	TestTrue(TEXT("ready after 16 s"), DomePower->CanActivate());
	continue;
	}
	return true;
}

// ================================================================================== ENG-12 abilities

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAbilitySlot, "TriadDrift.Powers.Ability.SlotFilledOnUnlock", TD_TEST_FLAGS)
bool FTDAbilitySlot::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	FTDTestWorld W;
	ATDShip* Bastion = W.Spawn<ATDShip>(FVector::ZeroVector);
	Bastion->SetShipId(ETDShipId::Bastion);
	UTDAbilityComponent* Slot = NewObject<UTDAbilityComponent>(Bastion, TEXT("AbilitySlot"));
	Slot->RegisterComponent();
	TestTrue(TEXT("starts empty"), Slot->GetAbility() == ETDAbilityId::None);
	TestEqual(TEXT("locked ability: HUD readiness 0, not 1"), Slot->GetReadiness01(), 0.f);
	TestFalse(TEXT("locked: HasAbility false"), Slot->HasAbility());
	TestNull(TEXT("no ability power"), UTDPowerComponent::FindPower(Bastion, ETDPowerInput::Ability));

	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	TestNotNull(TEXT("bus"), Bus);
	Bus->BroadcastAbilityUnlocked(ETDAbilityId::Cloak, ETDShipId::Rift);
	TestTrue(TEXT("another ship's unlock is ignored"), Slot->GetAbility() == ETDAbilityId::None);
	Bus->BroadcastAbilityUnlocked(ETDAbilityId::GravityWell, ETDShipId::Bastion);
	TestTrue(TEXT("Gravity Well installed"), Slot->GetAbility() == ETDAbilityId::GravityWell);
	TestEqual(TEXT("earned and ready: readiness 1"), Slot->GetReadiness01(), 1.f);
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
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGravityWell, "TriadDrift.Powers.Ability.GravityWellPullsOnlyFreeShips", TD_TEST_FLAGS)
bool FTDGravityWell::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	FTDTestWorld W;
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f));
	UTDGravityWellPower* Well = AddPower<UTDGravityWellPower>(Bastion);
	// Orb: spawned 400 cm ahead, flies 5000 cm/s for 0.8 s -> vortex centred at x = 4400.
	const FVector Centre(4400.f, 0.f, 0.f);
	ATDEnemyShip* Carrier = SpawnEnemyShip(W, TDEnemyIds::HiveCarrier, Centre + FVector(0.f, -3500.f, 0.f));
	Carrier->GetHealth()->InitMaxValues(10000.f, 0.f, true);
	Carrier->GetShipMovement()->SetComponentTickEnabled(false); // any movement now would be the vortex's
	const FVector CarrierStart = Carrier->GetActorLocation();
	TArray<FVector> BayOffsets;
	for (const ATDCarrierBay* Bay : Carrier->GetBays())
	{
		BayOffsets.Add(Bay->GetActorLocation() - CarrierStart);
	}
	AActor* Far = SpawnBody(W, ETDTeam::Enemy, Centre + FVector(0.f, 9000.f, 0.f), FVector(100.f), 1000.f);

	TestTrue(TEXT("well launched"), Well->TryActivate());
	W.Advance(0.85f, PowerFrameStep());
	const TWeakObjectPtr<ATDGravityWell> Vortex = Well->GetLastWell();
	TestTrue(TEXT("vortex open where the orb stopped"), Vortex.IsValid() && Vortex->IsVortexActive() && Vortex->GetActorLocation().Equals(Centre, 1.f));
	// A fighter flying straight out of the vortex at full cruise: it must not escape.
	ATDEnemyShip* Fighter = SpawnEnemyShip(W, TDEnemyIds::ShardFighter, Centre + FVector(1500.f, 2500.f, 0.f));
	Fighter->GetHealth()->InitMaxValues(1000.f, 0.f, true);
	Fighter->SetActorRotation((Fighter->GetActorLocation() - Centre).Rotation());

	ATDProjectile* Bolt = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile,
		FTransform(FRotator::ZeroRotator, Centre + FVector(-3000.f, 3000.f, 0.f)), Far, ETDTeam::Enemy);
	const float EatenAt = AdvanceUntil(W, [&] { return !Bolt || !Bolt->IsActive(); }, 2.f);
	TestTrue(TEXT("enemy projectile swallowed (well inside its 2.5 s life)"), EatenAt > 0.f);
	W.Advance(FMath::Max(0.f, 1.2f - EatenAt), PowerFrameStep());
	bool bTurretSilenced = false;
	for (const ATDEnemyTurret* Turret : Carrier->GetTurrets())
	{
		bTurretSilenced |= Turret && Turret->IsStunned() && FVector::Dist(Turret->GetActorLocation(), Centre) < 5000.f;
	}
	TestTrue(TEXT("carrier turrets inside the vortex can't fire"), Carrier->GetTurrets().Num() == 0 || bTurretSilenced);

	W.Advance(1.8f, PowerFrameStep());
	TestTrue(TEXT("free fighter pulled into the core"), FVector::Dist(Fighter->GetActorLocation(), Centre) < 1200.f);
	TestTrue(TEXT("carrier (Boss) not moved"), Carrier->GetActorLocation().Equals(CarrierStart, 1.f));
	const TArray<ATDCarrierBay*> Bays = Carrier->GetBays();
	for (int32 i = 0; i < Bays.Num() && i < BayOffsets.Num(); ++i)
	{
		TestTrue(TEXT("bay stays on its carrier"), (Bays[i]->GetActorLocation() - Carrier->GetActorLocation()).Equals(BayOffsets[i], 1.f));
	}
	TestEqual(TEXT("far enemy not pulled"), (float)FVector::Dist(Far->GetActorLocation(), Centre), 9000.f, 1.f);

	W.Advance(2.3f, PowerFrameStep());
	TestFalse(TEXT("vortex gone after 5 s"), Vortex.IsValid());
	const float Taken = 1000.f - Fighter->GetHealth()->GetHull();
	TestTrue(TEXT("fighter: 10 dps while caught (3.5-5 s) + 120 implosion"), Taken >= 120.f + 35.f && Taken <= 120.f + 50.f);
	TestFalse(TEXT("fighter's engines released after the implosion"), Fighter->GetShipMovement()->HasSpeedOverride());
	TestTrue(TEXT("carrier took the damage it can't escape"), Carrier->GetHealth()->GetHull() <= 10000.f - 120.f);
	TestEqual(TEXT("far enemy untouched"), Hull(Far), 1000.f);
	TestFalse(TEXT("18 s cooldown"), Well->CanActivate());
	W.Advance(14.f, PowerFrameStep()); // margin: implosion kills can trigger a director slow-mo (game time runs slower)
	TestTrue(TEXT("ready after 18 s"), Well->CanActivate());
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGravityWellDominion, "TriadDrift.Powers.Ability.GravityWellLeavesDominionPartsInPlace", TD_TEST_FLAGS)
bool FTDGravityWellDominion::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	FTDTestWorld W;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	D->WarpInSeconds = 0.f;
	D->WarpIn();
	W.Tick(PowerFrameStep());
	const TArray<ATDShieldGenerator*> Gens = D->GetGenerators();
	TestTrue(TEXT("Dominion built its generators"), Gens.Num() > 0);
	if (Gens.Num() == 0)
	{
		continue;
	}
	ATDShieldGenerator* Gen = Gens[0];
	const FVector GenStart = Gen->GetActorLocation();
	const FVector ReactorStart = D->GetReactor() ? D->GetReactor()->GetActorLocation() : FVector::ZeroVector;
	// Bastion 4400 cm from the generator so the vortex opens on it.
	const FVector ToGen = (GenStart - D->GetActorLocation()).GetSafeNormal();
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, GenStart + ToGen * 4400.f, FVector(100.f));
	Bastion->SetActorRotation((-ToGen).Rotation());
	UTDGravityWellPower* Well = AddPower<UTDGravityWellPower>(Bastion);
	TestTrue(TEXT("well launched at the generator"), Well->TryActivate());
	W.Advance(6.f, PowerFrameStep());
	TestTrue(TEXT("generator not dragged off its pylon"), Gen->GetActorLocation().Equals(GenStart, 1.f));
	TestTrue(TEXT("reactor not moved"), !D->GetReactor() || D->GetReactor()->GetActorLocation().Equals(ReactorStart, 1.f));
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCloak, "TriadDrift.Powers.Ability.CloakUntargetableFirstShotTriple", TD_TEST_FLAGS)
bool FTDCloak::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
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
	W.Advance(0.5f, PowerFrameStep());
	TestTrue(TEXT("missile lost its lock"), Missile && Missile->GetHomingTarget() == nullptr);

	Weapons->FireOnce(ETDWeaponSlot::Primary);
	const ATDProjectile* Shot = Weapons->GetLastFiredProjectile(0);
	TestTrue(TEXT("first shot out does 3x"), Shot && FMath::IsNearlyEqual(Shot->GetDamageMultiplier(), 3.f));
	TestFalse(TEXT("firing breaks cloak"), Cloak->IsCloaked());
	Hostiles.Reset();
	Registry->GetHostilesOf(ETDTeam::Enemy, Hostiles);
	TestTrue(TEXT("targetable again"), Hostiles.Contains(Rift));
	W.Advance(14.8f, PowerFrameStep());
	TestFalse(TEXT("15 s cooldown"), Cloak->CanActivate());
	W.Advance(0.4f, PowerFrameStep());
	TestTrue(TEXT("ready after 15 s"), Cloak->CanActivate());

	// Another power breaks it too; expiring without a shot leaves no banked 3x.
	Cloak->TryActivate();
	OtherPower->TryActivate();
	TestFalse(TEXT("using another power breaks cloak"), Cloak->IsCloaked());
	Cloak->ResetCooldown();
	Cloak->TryActivate();
	W.Advance(6.1f, PowerFrameStep());
	TestFalse(TEXT("cloak lasts 6 s"), Cloak->IsCloaked());
	W.Advance(0.2f, PowerFrameStep());
	Weapons->FireOnce(ETDWeaponSlot::Primary);
	const ATDProjectile* Later = Weapons->GetLastFiredProjectile(0);
	TestTrue(TEXT("no 3x after an unused cloak"), Later && FMath::IsNearlyEqual(Later->GetDamageMultiplier(), 1.f));
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEmp, "TriadDrift.Powers.Ability.EMPStunsDropsShieldsFizzlesMissiles", TD_TEST_FLAGS)
bool FTDEmp::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
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

	W.Advance(3.1f, PowerFrameStep());
	TestFalse(TEXT("stun lasts 3 s"), Gunship->IsStunned());
	W.Advance(1.5f, PowerFrameStep());
	TestFalse(TEXT("shield still down at 4.6 s"), Gunship->IsFrontalShieldUp());
	W.Advance(9.6f, PowerFrameStep());
	TestTrue(TEXT("ready after 14 s"), Emp->CanActivate());
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeClearsPowers, "TriadDrift.Powers.Reset.CutClearsEveryRunningPower", TD_TEST_FLAGS)
bool FTDRetakeClearsPowers::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	FTDTestWorld W;
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	UTDPowerSubsystem* Powers = UTDPowerSubsystem::Get(W.GetWorld());
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f), 100.f, 100.f);
	UTDWeaponComponent* Weapons = NewObject<UTDWeaponComponent>(Rift, TEXT("Weapons"));
	Weapons->RegisterComponent();
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
	UTDPortalBlasterPower* Portal = AddPower<UTDPortalBlasterPower>(Rift);
	UTDCloakPower* Cloak = AddPower<UTDCloakPower>(Rift);
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector(0.f, -20000.f, 0.f), FVector(100.f));
	UTDShieldDomePower* Dome = AddPower<UTDShieldDomePower>(Bastion);
	UTDGravityWellPower* Well = AddPower<UTDGravityWellPower>(Bastion);
	AActor* Rift2 = SpawnBody(W, ETDTeam::Player, FVector(0.f, 20000.f, 0.f), FVector(50.f));
	UTDPortalBlasterPower* Blink = AddPower<UTDPortalBlasterPower>(Rift2);
	Blink->bUseBlinkFallback = true;

	auto StartAll = [&]
	{
		Portal->HandleInputPressed();
		Rift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
		Portal->HandleInputPressed();
		Rift->SetActorRotation(FRotator::ZeroRotator);
		Cloak->TryActivate();
		Dome->TryActivate();
		Well->TryActivate();
		Blink->TryActivate();
		W.Advance(1.f, PowerFrameStep());
	};
	StartAll();
	TestEqual(TEXT("two portals open"), Portal->GetPortals().Num(), 2);
	TestTrue(TEXT("cloaked"), Cloak->IsCloaked());
	TestEqual(TEXT("dome + blink field up"), Powers->GetActiveDomes().Num(), 2);
	TestTrue(TEXT("vortex open"), Well->GetLastWell() && Well->GetLastWell()->IsVortexActive());

	// A first take (a new wave) keeps everything running.
	Bus->BroadcastTakeStarted(TEXT("Wave2"), 1);
	W.Tick(PowerFrameStep());
	TestEqual(TEXT("take 1 keeps the portals"), Portal->GetPortals().Num(), 2);
	TestEqual(TEXT("take 1 keeps the domes"), Powers->GetActiveDomes().Num(), 2);

	// CUT! -> everything from the failed take is gone.
	const TWeakObjectPtr<ATDGravityWell> Vortex = Well->GetLastWell();
	Bus->BroadcastSceneCut(TEXT("Wave2"), 2, 1.f);
	W.Tick(PowerFrameStep());
	TestEqual(TEXT("portals closed"), Portal->GetPortals().Num(), 0);
	TestFalse(TEXT("cloak off"), Cloak->IsCloaked());
	TestTrue(TEXT("Rift targetable again"), UTDStatics::GetHealth(Rift)->bTargetable);
	TestEqual(TEXT("dome and blink field collapsed"), Powers->GetActiveDomes().Num(), 0);
	TestFalse(TEXT("vortex gone"), Vortex.IsValid());
	for (UTDPowerComponent* P : TArray<UTDPowerComponent*>{ Portal, Cloak, Dome, Well, Blink })
	{
		TestTrue(*FString::Printf(TEXT("%s ready for take 2"), *P->GetPowerName().ToString()), P->CanActivate());
	}
	Weapons->FireOnce(ETDWeaponSlot::Primary);
	const ATDProjectile* Shot = Weapons->GetLastFiredProjectile(0);
	TestTrue(TEXT("no 3x shot banked across the cut"), Shot && FMath::IsNearlyEqual(Shot->GetDamageMultiplier(), 1.f));

	// Respawn path (T3's RestoreShipForFlight): ResetPower ends the cloak and frees Rift.
	W.Advance(0.5f, PowerFrameStep());
	Cloak->TryActivate();
	Cloak->ResetPower();
	TestTrue(TEXT("ResetPower: targetable"), UTDStatics::GetHealth(Rift)->bTargetable && !Cloak->IsCloaked() && Cloak->CanActivate());
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPortalWindowRestart, "TriadDrift.Powers.Portal.WindowFromSecondPortalCloakBreaksStatsCount", TD_TEST_FLAGS)
bool FTDPortalWindowRestart::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	// A late B still gets the full 12 s; recycling doesn't extend it.
	{
		FTDTestWorld W;
		AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDPortalBlasterPower* Portal = AddPower<UTDPortalBlasterPower>(Rift);
		Portal->HandleInputPressed();
		W.Advance(8.f, PowerFrameStep());
		Rift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
		Portal->HandleInputPressed();
		W.Advance(11.5f, PowerFrameStep());
		TestTrue(TEXT("pair still open 11.5 s after B"), Portal->IsPowerActive() && Portal->GetPortals().Num() == 2);
		Rift->SetActorRotation(FRotator(0.f, -90.f, 0.f));
		Portal->HandleInputPressed(); // recycle
		W.Advance(0.7f, PowerFrameStep());
		TestFalse(TEXT("closed 12 s after B even though a third shot recycled one"), Portal->IsPowerActive());
	}
	// Cloak: firing the blaster again while it is active breaks the cloak too.
	{
		FTDTestWorld W;
		AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDPortalBlasterPower* Portal = AddPower<UTDPortalBlasterPower>(Rift);
		UTDCloakPower* Cloak = AddPower<UTDCloakPower>(Rift);
		Portal->HandleInputPressed();
		Cloak->TryActivate();
		TestTrue(TEXT("cloaked with a portal open"), Cloak->IsCloaked());
		Rift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
		Portal->HandleInputPressed();
		TestFalse(TEXT("second portal press breaks cloak"), Cloak->IsCloaked());
	}
	// MissilesReturned really reaches the run stats (real game instance).
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		const int32 Before = GI ? GI->GetProgressRef().Stats.MissilesReturned : 0;
		AActor* Hook = SpawnBody(W, ETDTeam::Enemy, FVector(20000.f, 0.f, 0.f), FVector(100.f));
		AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		ATDProjectile* M = LaunchMissile(W, FVector(5000.f, 0.f, 0.f), -FVector::ForwardVector, Hook, Rift);
		const FTransform Entry(FRotator(0.f, 180.f, 0.f), FVector(5000.f, 0.f, 0.f));
		const FTransform Exit(FRotator(0.f, 90.f, 0.f), FVector(0.f, 9000.f, 0.f));
		TestTrue(TEXT("missile transits"), UTDPortalTransit::TransitActor(M, Entry, Exit, Rift, FTDPortalTransitParams()));
		TestEqual(TEXT("MissilesReturned +1"), GI ? GI->GetProgressRef().Stats.MissilesReturned : -1, Before + 1);
	}
	continue;
	}
	return true;
}

} // namespace TDPowerTest

#endif // WITH_DEV_AUTOMATION_TESTS

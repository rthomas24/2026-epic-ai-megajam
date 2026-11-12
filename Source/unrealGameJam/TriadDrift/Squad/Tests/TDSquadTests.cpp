// Automation tests for Squad (ENG-04, ENG-07, ENG-08). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Squad

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/Squad/Tests/TDSquadTestListener.h"

namespace TDSquadTests
{
	/** A spawned squad with a player controller and an event listener. */
	struct FSquadFixture
	{
		FTDTestWorld W;
		ATDSquadManager* Squad = nullptr;
		APlayerController* PC = nullptr;
		UTDSquadTestListener* Events = nullptr;

		explicit FSquadFixture(bool bSpawnSquad = true)
		{
			Events = NewObject<UTDSquadTestListener>();
			Events->AddToRoot();
			Events->Listen(UTDEventBus::Get(W.GetWorld()));

			PC = W.SpawnPlayerController();
			Squad = W.Spawn<ATDSquadManager>();
			if (Squad)
			{
				Squad->SetSquadPlayerController(PC);
				if (bSpawnSquad)
				{
					Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
				}
			}
			W.Tick();
		}

		~FSquadFixture()
		{
			if (Events)
			{
				Events->StopListening();
				Events->RemoveFromRoot();
			}
		}

		ATDShip* Ship(ETDShipId Id) const { return Squad ? Squad->GetShip(Id) : nullptr; }
		UTDHealthComponent* Health(ETDShipId Id) const { return UTDStatics::GetHealth(Ship(Id)); }
		ATDWingmanAIController* Wing(ETDShipId Id) const
		{
			const ATDShip* S = Ship(Id);
			return S ? Cast<ATDWingmanAIController>(S->GetController()) : nullptr;
		}

		void Kill(ETDShipId Id) const
		{
			if (UTDHealthComponent* H = Health(Id))
			{
				H->Kill();
			}
		}

		/** A stationary Enemy-team target with a health component (no enemy-ship code needed). */
		AActor* SpawnEnemy(const FVector& Location, float Hull = 400.f)
		{
			AActor* Actor = W.Spawn<AActor>(Location);
			if (!Actor)
			{
				return nullptr;
			}
			USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
			Actor->SetRootComponent(Root);
			Root->RegisterComponent();
			Actor->SetActorLocation(Location);
			UTDHealthComponent* H = NewObject<UTDHealthComponent>(Actor, TEXT("Health"));
			H->MaxHull = Hull;
			H->MaxShield = 0.f;
			H->DefaultTeam = ETDTeam::Enemy;
			H->RegisterComponent();
			return Actor;
		}
	};
}

using namespace TDSquadTests;

// ------------------------------------------------------------------------------------------ ENG-04

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadShipSpecs, "TriadDrift.Squad.ShipLibrary.SpecsMatchFeatures", TD_TEST_FLAGS)
bool FTDSquadShipSpecs::RunTest(const FString& Parameters)
{
	const FTDShipSpec& R = UTDShipLibrary::Get(ETDShipId::Rift);
	const FTDShipSpec& L = UTDShipLibrary::Get(ETDShipId::Lancer);
	const FTDShipSpec& B = UTDShipLibrary::Get(ETDShipId::Bastion);

	TestEqual(TEXT("Rift hull"), R.MaxHull, 100.f);      TestEqual(TEXT("Rift shield"), R.MaxShield, 100.f);
	TestEqual(TEXT("Lancer hull"), L.MaxHull, 70.f);     TestEqual(TEXT("Lancer shield"), L.MaxShield, 80.f);
	TestEqual(TEXT("Bastion hull"), B.MaxHull, 180.f);   TestEqual(TEXT("Bastion shield"), B.MaxShield, 160.f);
	TestEqual(TEXT("Rift cruise"), R.CruiseSpeed, 6000.f);     TestEqual(TEXT("Rift boost"), R.BoostSpeed, 11000.f);
	TestEqual(TEXT("Lancer cruise"), L.CruiseSpeed, 7500.f);   TestEqual(TEXT("Lancer boost"), L.BoostSpeed, 15000.f);
	TestEqual(TEXT("Bastion cruise"), B.CruiseSpeed, 4800.f);  TestEqual(TEXT("Bastion boost"), B.BoostSpeed, 8500.f);
	TestEqual(TEXT("turn rates"), FVector(R.TurnRateDeg, L.TurnRateDeg, B.TurnRateDeg), FVector(120.f, 100.f, 75.f));
	TestEqual(TEXT("boost meters"), FVector(R.BoostMeterSeconds, L.BoostMeterSeconds, B.BoostMeterSeconds), FVector(3.f, 4.f, 2.5f));
	TestTrue(TEXT("Lancer regenerates boost fastest"), L.BoostRegenSeconds < R.BoostRegenSeconds && L.BoostRegenSeconds < B.BoostRegenSeconds);
	TestEqual(TEXT("Rift weapons"), R.PrimaryWeapon.ToString() + R.SecondaryWeapon.ToString(), FString(TEXT("PhasePulseLasersSplitterBolt")));
	TestEqual(TEXT("Lancer weapons"), L.PrimaryWeapon.ToString() + L.SecondaryWeapon.ToString(), FString(TEXT("NeedleCannonsSpikeRockets")));
	TestEqual(TEXT("Bastion weapons"), B.PrimaryWeapon.ToString() + B.SecondaryWeapon.ToString(), FString(TEXT("BreakerBlasterFlakBurst")));
	TestTrue(TEXT("abilities"), R.UnlockableAbility == ETDAbilityId::Cloak && L.UnlockableAbility == ETDAbilityId::EMPPulse
		&& B.UnlockableAbility == ETDAbilityId::GravityWell);
	TestTrue(TEXT("Get returns the requested ship"), R.Ship == ETDShipId::Rift && L.Ship == ETDShipId::Lancer && B.Ship == ETDShipId::Bastion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSpecApplied, "TriadDrift.Squad.ShipLibrary.AppliedToSpawnedShips", TD_TEST_FLAGS)
bool FTDSquadSpecApplied::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	for (const ETDShipId Id : UTDShipLibrary::AllShips())
	{
		const FTDShipSpec& Spec = UTDShipLibrary::Get(Id);
		ATDShip* Ship = F.Ship(Id);
		if (!TestNotNull(TEXT("ship spawned"), Ship))
		{
			return false;
		}
		TestTrue(TEXT("ship id set"), Ship->GetShipId() == Id);
		TestTrue(TEXT("player team"), Ship->GetTeam() == ETDTeam::Player);
		const UTDHealthComponent* H = F.Health(Id);
		TestTrue(TEXT("hull/shield applied"), H && H->MaxHull == Spec.MaxHull && H->MaxShield == Spec.MaxShield && H->GetHull01() == 1.f);
		const FTDFlightTuning& T = Ship->GetShipMovement()->GetFlightTuning();
		TestEqual(TEXT("cruise applied"), T.CruiseSpeed, Spec.CruiseSpeed);
		TestEqual(TEXT("boost applied"), T.BoostSpeed, Spec.BoostSpeed);
		TestEqual(TEXT("turn rate applied"), T.TurnRate, Spec.TurnRateDeg);
		TestEqual(TEXT("boost meter applied"), T.BoostDuration, Spec.BoostMeterSeconds);
		if (const UTDWeaponComponent* Weapons = Ship->FindComponentByClass<UTDWeaponComponent>())
		{
			TestTrue(TEXT("primary equipped"), Weapons->GetWeaponId(ETDWeaponSlot::Primary) == Spec.PrimaryWeapon);
			TestTrue(TEXT("secondary equipped"), Weapons->GetWeaponId(ETDWeaponSlot::Secondary) == Spec.SecondaryWeapon);
		}
		else
		{
			AddWarning(TEXT("ship has no UTDWeaponComponent yet (T1 adds it); weapon equip not checked"));
		}
	}
	return true;
}

// ------------------------------------------------------------------------------------------ ENG-07

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchSameFrame, "TriadDrift.Squad.Switch.SameFrameAndKeepsVelocity", TD_TEST_FLAGS)
bool FTDSquadSwitchSameFrame::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.W.Advance(1.f); // get everyone moving
	ATDShip* Rift = F.Ship(ETDShipId::Rift);
	ATDShip* Lancer = F.Ship(ETDShipId::Lancer);
	if (!TestNotNull(TEXT("ships"), Rift) || !TestNotNull(TEXT("ships"), Lancer))
	{
		return false;
	}
	TestTrue(TEXT("player starts in Rift"), F.PC->GetPawn() == Rift);
	TestTrue(TEXT("Lancer starts with a wingman"), F.Wing(ETDShipId::Lancer) != nullptr);

	const FVector RiftVel = Rift->GetShipMovement()->Velocity;
	const FVector LancerVel = Lancer->GetShipMovement()->Velocity;
	TestTrue(TEXT("ships are moving before the switch"), RiftVel.Size() > 500.f && LancerVel.Size() > 500.f);

	// No tick between the request and the checks: the switch must be complete within the call.
	TestTrue(TEXT("switch accepted"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	TestTrue(TEXT("player possesses Lancer this frame"), F.PC->GetPawn() == Lancer);
	TestTrue(TEXT("active ship is Lancer"), F.Squad->GetActiveShip() == Lancer);
	TestTrue(TEXT("Rift handed to its wingman this frame"), F.Wing(ETDShipId::Rift) != nullptr);
	TestTrue(TEXT("Rift keeps its velocity"), Rift->GetShipMovement()->Velocity.Equals(RiftVel, 1.f));
	TestTrue(TEXT("Lancer keeps its velocity"), Lancer->GetShipMovement()->Velocity.Equals(LancerVel, 1.f));
	TestTrue(TEXT("Lancer is camera ship"), Lancer->IsPlayerFlown() && !Rift->IsPlayerFlown());
	TestEqual(TEXT("one OnShipSwitched"), F.Events->Switches, 1);
	TestTrue(TEXT("event carries new/old"), F.Events->LastNew == ETDShipId::Lancer && F.Events->LastOld == ETDShipId::Rift);

	// One frame later, still moving at about the same speed (no stall from the handover).
	F.W.Tick();
	TestTrue(TEXT("Rift still flying after handover"), Rift->GetShipMovement()->GetSpeed() > RiftVel.Size() * 0.8f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchCooldown, "TriadDrift.Squad.Switch.Cooldown", TD_TEST_FLAGS)
bool FTDSquadSwitchCooldown::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	TestTrue(TEXT("first switch"), F.Squad->RequestSwitchNext());
	TestTrue(TEXT("next = Lancer"), F.Squad->GetActiveShipId() == ETDShipId::Lancer);
	TestFalse(TEXT("blocked immediately"), F.Squad->RequestSwitchNext());
	TestTrue(TEXT("cooldown reported"), F.Squad->GetSwitchCooldown01() > 0.9f);
	F.W.Advance(0.3f);
	TestFalse(TEXT("blocked at 0.3 s"), F.Squad->RequestSwitchPrev());
	F.W.Advance(0.12f);
	TestTrue(TEXT("allowed after 0.4 s"), F.Squad->RequestSwitchPrev());
	TestTrue(TEXT("prev = Rift"), F.Squad->GetActiveShipId() == ETDShipId::Rift);
	F.W.Advance(0.45f);
	TestTrue(TEXT("prev wraps to Bastion"), F.Squad->RequestSwitchPrev() && F.Squad->GetActiveShipId() == ETDShipId::Bastion);
	F.W.Advance(0.45f);
	TestFalse(TEXT("switching to the active ship is a no-op"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadDownedUnselectable, "TriadDrift.Squad.Switch.DownedShipUnselectable", TD_TEST_FLAGS)
bool FTDSquadDownedUnselectable::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.Kill(ETDShipId::Lancer);
	F.W.Tick();
	TestTrue(TEXT("Lancer is down"), !F.Squad->IsShipAlive(ETDShipId::Lancer));
	TestEqual(TEXT("one OnShipDowned"), F.Events->Downed, 1);
	TestFalse(TEXT("can't switch to a downed ship"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	TestTrue(TEXT("next skips the downed ship"), F.Squad->RequestSwitchNext());
	TestTrue(TEXT("landed on Bastion"), F.Squad->GetActiveShipId() == ETDShipId::Bastion);
	TestEqual(TEXT("two living ships"), F.Squad->GetLivingShips().Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadAutoSwitch, "TriadDrift.Squad.Death.AutoSwitchToHealthiest", TD_TEST_FLAGS)
bool FTDSquadAutoSwitch::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	// Bastion hurt (0.7 of its total), Lancer untouched (1.0): Lancer is the healthiest.
	UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Bastion), 100.f, ETDDamageKind::Energy, nullptr, nullptr);
	F.Kill(ETDShipId::Rift);
	F.W.Tick();
	TestTrue(TEXT("still in Rift during the 0.6 s tumble"), F.Squad->GetActiveShipId() == ETDShipId::Rift);
	TestTrue(TEXT("Rift is dying"), F.Squad->GetShipState(ETDShipId::Rift) == ETDSquadSlotState::Dying);
	F.W.Advance(0.7f);
	TestTrue(TEXT("auto-switched to Lancer (healthiest)"), F.Squad->GetActiveShipId() == ETDShipId::Lancer);
	TestTrue(TEXT("player possesses Lancer"), F.PC->GetPawn() == F.Ship(ETDShipId::Lancer));
	TestTrue(TEXT("Rift hidden while downed"), F.Ship(ETDShipId::Rift)->IsHidden());
	TestTrue(TEXT("respawn countdown visible"), F.Squad->GetRespawnSeconds(ETDShipId::Rift) > 10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRespawn, "TriadDrift.Squad.Death.RespawnAfter12sWith2sInvulnerability", TD_TEST_FLAGS)
bool FTDSquadRespawn::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.Kill(ETDShipId::Bastion);
	F.W.Tick();
	F.W.Advance(11.8f);
	TestTrue(TEXT("still down at ~11.8 s"), !F.Squad->IsShipAlive(ETDShipId::Bastion));
	TestEqual(TEXT("no respawn yet"), F.Events->Respawned, 0);
	F.W.Advance(0.3f);
	TestTrue(TEXT("back at 12 s"), F.Squad->IsShipAlive(ETDShipId::Bastion));
	TestEqual(TEXT("OnShipRespawned"), F.Events->Respawned, 1);
	const UTDHealthComponent* H = F.Health(ETDShipId::Bastion);
	TestTrue(TEXT("full health"), H && H->IsAlive() && H->GetHull01() == 1.f && H->GetShield01() == 1.f);
	TestTrue(TEXT("invulnerable right after respawn"), H && H->IsInvulnerable());
	TestFalse(TEXT("visible again"), F.Ship(ETDShipId::Bastion)->IsHidden());
	TestTrue(TEXT("wingman flies it again"), F.Wing(ETDShipId::Bastion) != nullptr);
	F.W.Advance(1.8f);
	TestTrue(TEXT("still invulnerable at 1.8 s"), H && H->IsInvulnerable());
	F.W.Advance(0.4f);
	TestFalse(TEXT("vulnerable after 2 s"), H && H->IsInvulnerable());
	TestTrue(TEXT("selectable again"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchInGrace, "TriadDrift.Squad.Switch.SwitchInInvulnerability", TD_TEST_FLAGS)
bool FTDSquadSwitchInGrace::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	const UTDHealthComponent* H = F.Health(ETDShipId::Bastion);
	TestTrue(TEXT("2 s grace on switch-in"), H && H->IsInvulnerable());
	F.W.Advance(2.2f);
	TestFalse(TEXT("grace over"), H && H->IsInvulnerable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadWipe, "TriadDrift.Squad.Death.SquadWipeEvent", TD_TEST_FLAGS)
bool FTDSquadWipe::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.Kill(ETDShipId::Lancer);
	F.Kill(ETDShipId::Bastion);
	F.W.Tick();
	TestEqual(TEXT("no wipe with one ship left"), F.Events->Wipes, 0);
	F.Kill(ETDShipId::Rift);
	F.W.Tick();
	TestEqual(TEXT("OnSquadWiped once"), F.Events->Wipes, 1);
	TestEqual(TEXT("no living ships"), F.Squad->GetLivingShips().Num(), 0);
	F.W.Advance(1.f);
	TestEqual(TEXT("wipe not repeated"), F.Events->Wipes, 1);

	// A fresh take: SpawnSquad brings everyone back.
	F.Squad->SpawnSquad(FTransform::Identity);
	F.W.Tick();
	TestEqual(TEXT("all three back"), F.Squad->GetLivingShips().Num(), 3);
	TestTrue(TEXT("player in the starting ship"), F.PC->GetPawn() == F.Ship(ETDShipId::Rift));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRetake, "TriadDrift.Squad.Death.SpawnSquadIsARetake", TD_TEST_FLAGS)
bool FTDSquadRetake::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	// Make a mess: player in Lancer, Lancer hurt, Bastion dying, Rift downed, cooldown running.
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Lancer), 120.f, ETDDamageKind::Energy, nullptr, nullptr);
	F.Kill(ETDShipId::Rift);
	F.W.Advance(1.f);
	F.Kill(ETDShipId::Bastion);
	F.W.Tick();
	TestTrue(TEXT("Rift downed"), F.Squad->GetShipState(ETDShipId::Rift) == ETDSquadSlotState::Downed);
	TestTrue(TEXT("Bastion dying"), F.Squad->GetShipState(ETDShipId::Bastion) == ETDSquadSlotState::Dying);
	const int32 RespawnsBefore = F.Events->Respawned;

	const FVector Origin(50000.f, 0.f, 2000.f);
	F.Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, Origin));

	// Everything is reset within the call.
	TestEqual(TEXT("all three alive"), F.Squad->GetLivingShips().Num(), 3);
	TestTrue(TEXT("player back in the starting ship"), F.PC->GetPawn() == F.Ship(ETDShipId::Rift) && F.Squad->GetActiveShipId() == ETDShipId::Rift);
	TestTrue(TEXT("leader at the origin"), F.Ship(ETDShipId::Rift)->GetActorLocation().Equals(Origin, 1.f));
	for (const ETDShipId Id : UTDShipLibrary::AllShips())
	{
		const UTDHealthComponent* H = F.Health(Id);
		TestTrue(TEXT("full health"), H && H->IsAlive() && H->GetHull01() == 1.f && H->GetShield01() == 1.f);
		TestFalse(TEXT("visible"), F.Ship(Id)->IsHidden());
		TestEqual(TEXT("no respawn timer"), F.Squad->GetRespawnSeconds(Id), 0.f);
		TestTrue(TEXT("near the origin"), FVector::Dist(F.Ship(Id)->GetActorLocation(), Origin) < 3000.f);
	}
	TestEqual(TEXT("switch cooldown cleared"), F.Squad->GetSwitchCooldown01(), 0.f);
	TestFalse(TEXT("no Switch Strike carried over"), F.Squad->IsSwitchStrikeActive());
	const ATDWingmanAIController* BastionWing = F.Wing(ETDShipId::Bastion);
	const ATDWingmanAIController* LancerWing = F.Wing(ETDShipId::Lancer);
	TestTrue(TEXT("Bastion's wingman flies again"), BastionWing && BastionWing->GetWingmanState() != ETDWingmanState::Downed);
	TestTrue(TEXT("Lancer back under AI"), LancerWing != nullptr);
	TestTrue(TEXT("can switch immediately"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));

	// The old take's timers must not fire later.
	F.W.Advance(13.f);
	TestEqual(TEXT("no stale respawns"), F.Events->Respawned, RespawnsBefore);
	TestEqual(TEXT("still three alive"), F.Squad->GetLivingShips().Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchStrike, "TriadDrift.Squad.SwitchStrike.WindowAndMultiplier", TD_TEST_FLAGS)
bool FTDSquadSwitchStrike::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld());

	// Too late: 1.6 s after the power.
	if (Bus) { Bus->BroadcastPowerUsed(ETDShipId::Rift, TEXT("PortalBlaster")); }
	F.W.Advance(1.6f);
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	TestFalse(TEXT("no Switch Strike outside 1.5 s"), F.Squad->IsSwitchStrikeActive());

	// In time: 1.0 s after the power (from the event bus, as T2 broadcasts it).
	F.W.Advance(0.5f);
	if (Bus) { Bus->BroadcastPowerUsed(ETDShipId::Lancer, TEXT("AfterburnerRam")); }
	F.W.Advance(1.0f);
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	ATDShip* Bastion = F.Ship(ETDShipId::Bastion);
	TestTrue(TEXT("Switch Strike active"), F.Squad->IsSwitchStrikeActive());
	TestEqual(TEXT("+25% on the new ship"), F.Squad->GetDamageMultiplier(Bastion), 1.25f);
	TestEqual(TEXT("not on the old ship"), F.Squad->GetDamageMultiplier(F.Ship(ETDShipId::Lancer)), 1.f);
	const UTDWeaponComponent* Weapons = Bastion ? Bastion->FindComponentByClass<UTDWeaponComponent>() : nullptr;
	if (Weapons)
	{
		TestEqual(TEXT("weapon multiplier set"), Weapons->GetOutgoingDamageMultiplier(), 1.25f, 0.001f);
	}
	F.W.Advance(3.8f);
	TestTrue(TEXT("still active at 3.8 s"), F.Squad->IsSwitchStrikeActive());
	F.W.Advance(0.3f);
	TestFalse(TEXT("over after 4 s"), F.Squad->IsSwitchStrikeActive());
	TestEqual(TEXT("multiplier back to 1"), F.Squad->GetDamageMultiplier(Bastion), 1.f);
	if (Weapons)
	{
		TestEqual(TEXT("weapon multiplier cleared"), Weapons->GetOutgoingDamageMultiplier(), 1.f, 0.001f);
	}

	// One power arms one switch only.
	if (Bus) { Bus->BroadcastPowerUsed(ETDShipId::Bastion, TEXT("MissileSwarm")); }
	F.W.Advance(0.5f);
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Rift));
	TestTrue(TEXT("armed"), F.Squad->IsSwitchStrikeActive());
	F.W.Advance(0.45f);
	TestTrue(TEXT("switch again"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	TestFalse(TEXT("second switch doesn't re-arm from the same power"), F.Squad->IsSwitchStrikeActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadHUD, "TriadDrift.Squad.HUD.SquadCards", TD_TEST_FLAGS)
bool FTDSquadHUD::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.Kill(ETDShipId::Lancer);
	F.W.Tick();
	const TArray<FTDShipHUD> Cards = F.Squad->GetSquadHUD();
	if (!TestEqual(TEXT("three cards"), Cards.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("order"), Cards[0].Ship == ETDShipId::Rift && Cards[1].Ship == ETDShipId::Lancer && Cards[2].Ship == ETDShipId::Bastion);
	TestTrue(TEXT("Rift is player controlled"), Cards[0].bPlayerControlled && !Cards[1].bPlayerControlled);
	TestTrue(TEXT("Lancer down with a countdown"), !Cards[1].bAlive && Cards[1].RespawnSeconds > 11.f);
	TestTrue(TEXT("Bastion healthy"), Cards[2].bAlive && Cards[2].Health01 == 1.f && Cards[2].Shield01 == 1.f);
	TestTrue(TEXT("abilities listed"), Cards[2].Ability == ETDAbilityId::GravityWell);
	TestTrue(TEXT("manager found by Get"), ATDSquadManager::Get(F.W.GetWorld()) == F.Squad);
	return true;
}

// ------------------------------------------------------------------------------------------ ENG-08

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadFormation, "TriadDrift.Squad.Wingmen.HoldFormation", TD_TEST_FLAGS)
bool FTDSquadFormation::RunTest(const FString& Parameters)
{
	// Same result at 30, 60 and 120 fps.
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		FSquadFixture F;
		F.W.Advance(8.f, Step);
		for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
		{
			const ATDShip* Ship = F.Ship(Id);
			const ATDWingmanAIController* Wing = F.Wing(Id);
			if (!TestNotNull(TEXT("wingman"), Wing) || !Ship)
			{
				return false;
			}
			const FString Label = FString::Printf(TEXT("%s @ %.0f fps"), *UEnum::GetValueAsString(Id), 1.f / Step);
			const float Dist = FVector::Dist(Ship->GetActorLocation(), F.Squad->GetFormationSlotLocation(Id));
			TestTrue(FString::Printf(TEXT("%s within 1500 cm of its slot (%.0f)"), *Label, Dist), Dist < 1500.f);
			TestTrue(Label + TEXT(" in formation state"), Wing->GetWingmanState() == ETDWingmanState::Formation);
			const float LeaderDist = FVector::Dist(Ship->GetActorLocation(), F.Ship(ETDShipId::Rift)->GetActorLocation());
			TestTrue(Label + TEXT(" 1200-3000 cm from the leader"), LeaderDist > 1200.f && LeaderDist < 3000.f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadEngage, "TriadDrift.Squad.Wingmen.EngageWithoutPowers", TD_TEST_FLAGS)
bool FTDSquadEngage::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 0.f, 0.f), 100000.f);
	F.W.Advance(3.f);
	int32 Engaged = 0;
	for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
	{
		const ATDWingmanAIController* Wing = F.Wing(Id);
		if (Wing && Wing->GetWingmanState() == ETDWingmanState::Engage && Wing->GetCurrentTarget() == Enemy)
		{
			++Engaged;
		}
	}
	TestEqual(TEXT("both wingmen engage the only enemy"), Engaged, 2);
	F.W.Advance(6.f);
	const UTDHealthComponent* H = UTDStatics::GetHealth(Enemy);
	if (F.Ship(ETDShipId::Lancer)->FindComponentByClass<UTDWeaponComponent>())
	{
		TestTrue(TEXT("wingmen hit the enemy"), H && H->GetHull01() < 1.f);
	}
	TestEqual(TEXT("wingmen never fired a power"), F.Events->PowersUsed, 0);
	TestFalse(TEXT("no Switch Strike from wingmen"), F.Squad->IsSwitchStrikeActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadFightAfterSwitch, "TriadDrift.Squad.Wingmen.KeepFightingAfterSwitch", TD_TEST_FLAGS)
bool FTDSquadFightAfterSwitch::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 2000.f, 0.f), 100000.f);
	F.W.Advance(3.f);
	TestTrue(TEXT("switch to Lancer"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	F.W.Advance(1.5f);
	const ATDWingmanAIController* RiftWing = F.Wing(ETDShipId::Rift);
	const ATDWingmanAIController* BastionWing = F.Wing(ETDShipId::Bastion);
	TestTrue(TEXT("Rift's new wingman picks up the fight"), RiftWing && RiftWing->GetWingmanState() == ETDWingmanState::Engage && RiftWing->GetCurrentTarget() == Enemy);
	TestTrue(TEXT("Bastion keeps fighting"), BastionWing && BastionWing->GetWingmanState() == ETDWingmanState::Engage);
	TestNull(TEXT("Lancer has no AI while the player flies it"), F.Wing(ETDShipId::Lancer));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadFocus, "TriadDrift.Squad.Wingmen.FocusCommand", TD_TEST_FLAGS)
bool FTDSquadFocus::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.SpawnEnemy(FVector(6000.f, 0.f, 0.f), 100000.f);
	AActor* Far = F.SpawnEnemy(FVector(14000.f, -4000.f, 0.f), 100000.f);
	F.W.Advance(1.f);
	F.Squad->IssueFocusCommand(Far);
	F.W.Advance(0.6f);
	TestTrue(TEXT("Lancer focuses"), F.Wing(ETDShipId::Lancer) && F.Wing(ETDShipId::Lancer)->GetCurrentTarget() == Far);
	TestTrue(TEXT("Bastion focuses"), F.Wing(ETDShipId::Bastion) && F.Wing(ETDShipId::Bastion)->GetCurrentTarget() == Far);
	TestTrue(TEXT("focus active"), F.Squad->GetFocusTarget() == Far);
	F.W.Advance(7.6f);
	TestNull(TEXT("focus expires after 8 s"), F.Squad->GetFocusTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadHyperdriveFollow, "TriadDrift.Squad.Wingmen.FollowHyperdriveNotRam", TD_TEST_FLAGS)
bool FTDSquadHyperdriveFollow::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 0.f, 0.f), 100000.f);
	F.W.Advance(2.f);
	ATDShip* Leader = F.Ship(ETDShipId::Rift);

	// A ram-speed dash (3x Lancer boost) is not a hyperdrive jump: wingmen keep fighting on their own.
	Leader->SetSpeedOverride(45000.f, 0.4f);
	F.W.Advance(0.5f);
	Leader->ClearSpeedOverride();
	const ATDWingmanAIController* Wing = F.Wing(ETDShipId::Bastion);
	TestTrue(TEXT("still engaged after a ram-speed dash"), Wing && Wing->GetWingmanState() == ETDWingmanState::Engage && Wing->GetCurrentTarget() == Enemy);

	// Hyperdrive (80 000 cm/s): wingmen ride along in their slots.
	Leader->SetSpeedOverride(80000.f, 0.1f);
	F.W.Advance(2.f);
	for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
	{
		const float Dist = FVector::Dist(F.Ship(Id)->GetActorLocation(), F.Squad->GetFormationSlotLocation(Id));
		TestTrue(FString::Printf(TEXT("%s jumps in formation (%.0f cm from slot)"), *UEnum::GetValueAsString(Id), Dist), Dist < 2000.f);
	}
	Leader->ClearSpeedOverride();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadFormationFastLeader, "TriadDrift.Squad.Wingmen.HoldFormationBehindFasterLeader", TD_TEST_FLAGS)
bool FTDSquadFormationFastLeader::RunTest(const FString& Parameters)
{
	// The player flies Lancer at full throttle (cruise 7500). Bastion cruises at only 4800 and
	// Rift at 6000: both must still hold their slots, through the catch-up allowance.
	for (const float Step : { 1.f / 30.f, 1.f / 120.f })
	{
		FSquadFixture F;
		F.W.Advance(0.5f, Step);
		TestTrue(TEXT("switch to Lancer"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
		ATDShip* Leader = F.Ship(ETDShipId::Lancer);
		FTDFlightInput Full;
		Full.Throttle = 1.f;
		Leader->SetFlightInput(Full);
		F.W.Advance(14.f, Step);

		TestTrue(TEXT("leader really is at Lancer cruise"), Leader->GetShipMovement()->GetSpeed() > 7000.f);
		for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Bastion })
		{
			const float Dist = FVector::Dist(F.Ship(Id)->GetActorLocation(), F.Squad->GetFormationSlotLocation(Id));
			TestTrue(FString::Printf(TEXT("%s @ %.0f fps holds its slot behind a faster leader (%.0f cm)"),
				*UEnum::GetValueAsString(Id), 1.f / Step, Dist), Dist < 1500.f);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS


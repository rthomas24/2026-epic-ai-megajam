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
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "EnhancedPlayerInput.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/Squad/Tests/TDSquadTestListener.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Powers/TDShieldDome.h"

namespace TDSquadTests
{
	/** A spawned squad with a player controller and an event listener. */
	/** Frame steps every time-based test runs at (30 / 60 / 120 fps). */
	static const float GFrameSteps[] = { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f };

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

		/** Direct kill (ignores invulnerability). For tests about what happens AFTER a death. */
		void Kill(ETDShipId Id) const
		{
			if (UTDHealthComponent* H = Health(Id))
			{
				H->Kill();
			}
		}

		/** Kill through the real damage path: an enemy's hit via UTDStatics::ApplyTDDamage. */
		bool KillWithDamage(ETDShipId Id)
		{
			if (!DamageSource)
			{
				DamageSource = SpawnEnemy(FVector(0.f, 0.f, 900000.f), 1.f); // far outside any engage range
			}
			UTDStatics::ApplyTDDamage(Ship(Id), 100000.f, ETDDamageKind::Energy, DamageSource, DamageSource);
			const UTDHealthComponent* H = Health(Id);
			return H && H->IsDead();
		}

		AActor* DamageSource = nullptr;

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
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		F.Squad->OnSwitchRefused.AddDynamic(F.Events, &UTDSquadTestListener::OnSwitchRefused);
		TestTrue(TEXT("first switch") + Fps, F.Squad->RequestSwitchNext());
		TestTrue(TEXT("next = Lancer") + Fps, F.Squad->GetActiveShipId() == ETDShipId::Lancer);
		// m14: a second Tab during the cooldown is refused NOW but buffered: it lands when the cooldown ends.
		TestFalse(TEXT("blocked immediately") + Fps, F.Squad->RequestSwitchNext());
		TestTrue(TEXT("refusal: cooldown") + Fps, F.Events->LastRefusal == ETDSwitchRefusal::Cooldown);
		TestTrue(TEXT("cooldown reported") + Fps, F.Squad->GetSwitchCooldown01() > 0.9f);
		F.W.Advance(0.34f, Step);
		TestTrue(TEXT("still Lancer at 0.34 s") + Fps, F.Squad->GetActiveShipId() == ETDShipId::Lancer);
		F.W.Advance(0.1f, Step);
		TestTrue(TEXT("double-tap reached the third ship after 0.4 s") + Fps, F.Squad->GetActiveShipId() == ETDShipId::Bastion);
		TestEqual(TEXT("two switch events") + Fps, F.Events->Switches, 2);
		// Nothing buffered: a fresh press after the cooldown switches at once, wrapping.
		F.W.Advance(0.45f, Step);
		TestTrue(TEXT("next wraps to Rift") + Fps, F.Squad->RequestSwitchNext() && F.Squad->GetActiveShipId() == ETDShipId::Rift);
		F.W.Advance(0.45f, Step);
		TestFalse(TEXT("switching to the active ship is a no-op") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Rift));
		TestTrue(TEXT("refusal: already active") + Fps, F.Events->LastRefusal == ETDSwitchRefusal::AlreadyActive);
		F.W.Advance(1.f, Step);
		TestTrue(TEXT("a no-op press is not buffered") + Fps, F.Squad->GetActiveShipId() == ETDShipId::Rift);
		TestEqual(TEXT("exactly three switch events") + Fps, F.Events->Switches, 3);
	}
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
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		// Lancer (next in switch order) is hurt; Bastion is untouched: the auto-switch must skip Lancer.
		UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Lancer), 60.f, ETDDamageKind::Energy, F.SpawnEnemy(FVector(0.f, 0.f, 900000.f), 1.f), nullptr);
		TestTrue(TEXT("Lancer really hurt") + Fps, F.Health(ETDShipId::Lancer)->GetShield01() < 1.f);
		TestTrue(TEXT("Rift killed by real damage") + Fps, F.KillWithDamage(ETDShipId::Rift));
		F.W.Tick(Step);
		TestTrue(TEXT("still in Rift during the 0.6 s tumble") + Fps, F.Squad->GetActiveShipId() == ETDShipId::Rift);
		TestTrue(TEXT("Rift is dying") + Fps, F.Squad->GetShipState(ETDShipId::Rift) == ETDSquadSlotState::Dying);
		F.W.Advance(0.5f, Step);
		TestTrue(TEXT("still tumbling at 0.5 s") + Fps, F.Squad->GetActiveShipId() == ETDShipId::Rift);
		F.W.Advance(0.2f, Step);
		TestTrue(TEXT("auto-switched to Bastion (healthiest, not next in order)") + Fps, F.Squad->GetActiveShipId() == ETDShipId::Bastion);
		TestTrue(TEXT("player possesses Bastion") + Fps, F.PC->GetPawn() == F.Ship(ETDShipId::Bastion));
		TestTrue(TEXT("Rift hidden while downed") + Fps, F.Ship(ETDShipId::Rift)->IsHidden());
		TestTrue(TEXT("respawn countdown visible") + Fps, F.Squad->GetRespawnSeconds(ETDShipId::Rift) > 10.f);
		TestTrue(TEXT("auto-switch cut flagged") + Fps, F.Squad->GetLastSwitchCut().bAutoSwitch);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRespawn, "TriadDrift.Squad.Death.RespawnAfter12sWith2sInvulnerability", TD_TEST_FLAGS)
bool FTDSquadRespawn::RunTest(const FString& Parameters)
{
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		TestTrue(TEXT("Bastion killed by real damage") + Fps, F.KillWithDamage(ETDShipId::Bastion));
		F.W.Advance(11.8f, Step);
		TestTrue(TEXT("still down at 11.8 s") + Fps, !F.Squad->IsShipAlive(ETDShipId::Bastion));
		TestEqual(TEXT("no respawn yet") + Fps, F.Events->Respawned, 0);
		F.W.Advance(0.3f, Step);
		TestTrue(TEXT("back at 12 s") + Fps, F.Squad->IsShipAlive(ETDShipId::Bastion));
		TestEqual(TEXT("OnShipRespawned") + Fps, F.Events->Respawned, 1);
		const UTDHealthComponent* H = F.Health(ETDShipId::Bastion);
		TestTrue(TEXT("full health") + Fps, H && H->IsAlive() && H->GetHull01() == 1.f && H->GetShield01() == 1.f);
		TestFalse(TEXT("visible again") + Fps, F.Ship(ETDShipId::Bastion)->IsHidden());
		TestTrue(TEXT("wingman flies it again") + Fps, F.Wing(ETDShipId::Bastion) != nullptr);

		// Invulnerability proven by real damage, not just a flag: 1.8 s in, a hit does nothing...
		F.W.Advance(1.8f, Step);
		UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Bastion), 50.f, ETDDamageKind::Energy, F.DamageSource, F.DamageSource);
		TestTrue(TEXT("no damage taken at 1.8 s") + Fps, H && H->GetShield01() == 1.f && H->GetHull01() == 1.f);
		// ...2.2 s in, the same hit lands.
		F.W.Advance(0.4f, Step);
		UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Bastion), 50.f, ETDDamageKind::Energy, F.DamageSource, F.DamageSource);
		TestTrue(TEXT("damage lands after 2 s") + Fps, H && H->GetShield01() < 1.f);
		TestTrue(TEXT("selectable again") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchInGrace, "TriadDrift.Squad.Switch.SwitchInInvulnerability", TD_TEST_FLAGS)
bool FTDSquadSwitchInGrace::RunTest(const FString& Parameters)
{
	// M4: manual switch = 0.35 s grace, once per ship per 6 s; auto-switch (death) = 2 s.
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		AActor* Enemy = F.SpawnEnemy(FVector(0.f, 0.f, 900000.f), 1.f);
		auto Hit = [&](ETDShipId Id) { UTDStatics::ApplyTDDamage(F.Ship(Id), 20.f, ETDDamageKind::Energy, Enemy, Enemy); };
		auto Shield = [&](ETDShipId Id) { return F.Health(Id)->GetShield01(); };

		TestTrue(TEXT("manual switch") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Bastion));
		F.W.Advance(0.2f, Step);
		Hit(ETDShipId::Bastion);
		TestEqual(TEXT("0.35 s grace on a manual switch-in") + Fps, Shield(ETDShipId::Bastion), 1.f);
		F.W.Advance(0.3f, Step);
		Hit(ETDShipId::Bastion);
		TestTrue(TEXT("grace over after 0.35 s") + Fps, Shield(ETDShipId::Bastion) < 1.f);

		// Spam: back into Bastion within 6 s gets NO grace.
		TestTrue(TEXT("out") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Rift));
		F.W.Advance(0.45f, Step);
		TestTrue(TEXT("back in") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Bastion));
		const float Before = Shield(ETDShipId::Bastion);
		Hit(ETDShipId::Bastion);
		TestTrue(TEXT("no re-armed grace within 6 s (Tab spam can't farm invulnerability)") + Fps, Shield(ETDShipId::Bastion) < Before);

		// Auto-switch on death: the ship you land in gets the full 2 s.
		F.W.Advance(0.5f, Step);
		TestTrue(TEXT("player into Lancer") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Lancer));
		F.W.Advance(0.5f, Step);
		F.Kill(ETDShipId::Lancer);
		F.W.Advance(0.7f, Step);
		const ETDShipId Landed = F.Squad->GetActiveShipId();
		TestTrue(TEXT("auto-switched") + Fps, Landed != ETDShipId::Lancer);
		F.W.Advance(1.5f, Step);
		const float LandedBefore = Shield(Landed);
		Hit(Landed);
		TestEqual(TEXT("2 s grace after an auto-switch") + Fps, Shield(Landed), LandedBefore);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadWipe, "TriadDrift.Squad.Death.SquadWipeEvent", TD_TEST_FLAGS)
bool FTDSquadWipe::RunTest(const FString& Parameters)
{
	for (const float Step : GFrameSteps)
	{
		FSquadFixture F;
		F.Kill(ETDShipId::Lancer);
		F.Kill(ETDShipId::Bastion);
		F.W.Tick(Step);
		TestEqual(TEXT("no wipe with one ship left"), F.Events->Wipes, 0);
		F.Kill(ETDShipId::Rift);
		F.W.Tick(Step);
		TestEqual(TEXT("OnSquadWiped once"), F.Events->Wipes, 1);
		TestEqual(TEXT("no living ships"), F.Squad->GetLivingShips().Num(), 0);
		F.W.Advance(1.f, Step);
		TestEqual(TEXT("wipe not repeated"), F.Events->Wipes, 1);

		// A fresh take: SpawnSquad brings everyone back.
		F.Squad->SpawnSquad(FTransform::Identity);
		F.W.Tick(Step);
		TestEqual(TEXT("all three back"), F.Squad->GetLivingShips().Num(), 3);
		TestTrue(TEXT("player in the starting ship"), F.PC->GetPawn() == F.Ship(ETDShipId::Rift));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRetake, "TriadDrift.Squad.Death.SpawnSquadIsARetake", TD_TEST_FLAGS)
bool FTDSquadRetake::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	// Make a mess: player in Lancer, Lancer hurt, Bastion dying, Rift downed, cooldown running.
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	F.W.Advance(0.5f); // past the 0.35 s switch-in grace
	UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Lancer), 120.f, ETDDamageKind::Energy, F.SpawnEnemy(FVector(0.f, 0.f, 900000.f), 1.f), nullptr);
	TestTrue(TEXT("the damage really landed"), F.Health(ETDShipId::Lancer)->GetHull01() < 1.f);
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
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld());
		if (!TestNotNull(TEXT("bus"), Bus))
		{
			return false;
		}

		// Too late: 1.6 s after the power.
		Bus->BroadcastPowerUsed(ETDShipId::Rift, TEXT("PortalBlaster"));
		F.W.Advance(1.6f, Step);
		TestTrue(TEXT("switch") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Lancer));
		TestFalse(TEXT("no Switch Strike outside 1.5 s") + Fps, F.Squad->IsSwitchStrikeActive());

		// In time: 1.4 s after the power.
		F.W.Advance(0.5f, Step);
		Bus->BroadcastPowerUsed(ETDShipId::Lancer, TEXT("AfterburnerRam"));
		F.W.Advance(1.4f, Step);
		TestTrue(TEXT("switch") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Bastion));
		ATDShip* Bastion = F.Ship(ETDShipId::Bastion);
		TestTrue(TEXT("Switch Strike active") + Fps, F.Squad->IsSwitchStrikeActive());
		TestEqual(TEXT("+25% on the new ship") + Fps, F.Squad->GetDamageMultiplier(Bastion), 1.25f);
		TestEqual(TEXT("not on the old ship") + Fps, F.Squad->GetDamageMultiplier(F.Ship(ETDShipId::Lancer)), 1.f);
		const UTDWeaponComponent* Weapons = Bastion ? Bastion->FindComponentByClass<UTDWeaponComponent>() : nullptr;
		if (!TestNotNull(TEXT("weapons") + Fps, Weapons))
		{
			return false;
		}
		TestEqual(TEXT("weapon multiplier set (projectiles use it)") + Fps, Weapons->GetOutgoingDamageMultiplier(), 1.25f, 0.001f);
		F.W.Advance(3.8f, Step);
		TestTrue(TEXT("still active at 3.8 s") + Fps, F.Squad->IsSwitchStrikeActive());
		F.W.Advance(0.3f, Step);
		TestFalse(TEXT("over after 4 s") + Fps, F.Squad->IsSwitchStrikeActive());
		TestEqual(TEXT("weapon multiplier cleared") + Fps, Weapons->GetOutgoingDamageMultiplier(), 1.f, 0.001f);

		// One power arms one switch only.
		Bus->BroadcastPowerUsed(ETDShipId::Bastion, TEXT("MissileSwarm"));
		F.W.Advance(0.5f, Step);
		TestTrue(TEXT("switch") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Rift));
		TestTrue(TEXT("armed") + Fps, F.Squad->IsSwitchStrikeActive());
		F.W.Advance(0.45f, Step);
		TestTrue(TEXT("switch again") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Lancer));
		TestFalse(TEXT("second switch doesn't re-arm from the same power") + Fps, F.Squad->IsSwitchStrikeActive());
		TestEqual(TEXT("old ship's multiplier cleared on the next switch") + Fps,
			F.Ship(ETDShipId::Rift)->FindComponentByClass<UTDWeaponComponent>()->GetOutgoingDamageMultiplier(), 1.f, 0.001f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchStrikeRealPower, "TriadDrift.Squad.SwitchStrike.ArmedByRealPower", TD_TEST_FLAGS)
bool FTDSquadSwitchStrikeRealPower::RunTest(const FString& Parameters)
{
	// The real path: T2's power component on the player's ship fires, broadcasts OnPowerUsed, and the
	// switch that follows gets Switch Strike. No direct calls into the squad manager.
	FSquadFixture F;
	F.W.Advance(0.5f);
	ATDShip* Rift = F.Ship(ETDShipId::Rift);
	UTDPowerComponent* Power = UTDPowerComponent::FindPower(Rift, ETDPowerInput::Signature1);
	if (!TestNotNull(TEXT("Rift carries its signature power (installed by the squad)"), Power))
	{
		return false;
	}
	UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Signature1, true);
	F.W.Tick();
	UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Signature1, false);
	F.W.Advance(0.3f);
	TestTrue(TEXT("the power really fired (OnPowerUsed broadcast)"), F.Events->PowersUsed >= 1);
	TestTrue(TEXT("power is on cooldown"), Power->GetCooldown01() < 1.f || Power->IsPowerActive());
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	TestTrue(TEXT("Switch Strike armed by the real power"), F.Squad->IsSwitchStrikeActive());
	// ENG-07: a power started on one ship keeps running after you switch away.
	F.W.Advance(0.5f);
	TestTrue(TEXT("Rift's power kept running or cooling down after the switch"), Power->IsPowerActive() || Power->GetCooldown01() < 1.f);
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
	for (const float Step : GFrameSteps)
	{
		FSquadFixture F;
		AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 0.f, 0.f), 100000.f);
		F.W.Advance(3.f, Step);
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
		F.W.Advance(6.f, Step);
		const UTDHealthComponent* H = UTDStatics::GetHealth(Enemy);
		TestNotNull(TEXT("wingman ships have weapons"), F.Ship(ETDShipId::Lancer)->FindComponentByClass<UTDWeaponComponent>());
		TestTrue(TEXT("wingmen hit the enemy with real projectiles"), H && H->GetHull01() < 1.f);
		TestEqual(TEXT("no OnPowerUsed while only wingmen fought"), F.Events->PowersUsed, 0);
		// Prove it on T2's real components: every power and ability on the wingman ships is untouched.
		for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
		{
			for (const ETDPowerInput Slot : { ETDPowerInput::Signature1, ETDPowerInput::Signature2, ETDPowerInput::Ability })
			{
				if (const UTDPowerComponent* Power = UTDPowerComponent::FindPower(F.Ship(Id), Slot))
				{
					TestTrue(TEXT("wingman power never activated"), !Power->IsPowerActive() && Power->GetCooldown01() == 1.f);
				}
			}
		}
		TestNotNull(TEXT("the wingman ships do carry powers (so the check above is real)"),
			UTDPowerComponent::FindPower(F.Ship(ETDShipId::Lancer), ETDPowerInput::Signature1));
		TestFalse(TEXT("no Switch Strike from wingmen"), F.Squad->IsSwitchStrikeActive());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadFightAfterSwitch, "TriadDrift.Squad.Wingmen.KeepFightingAfterSwitch", TD_TEST_FLAGS)
bool FTDSquadFightAfterSwitch::RunTest(const FString& Parameters)
{
	for (const float Step : GFrameSteps)
	{
		FSquadFixture F;
		AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 2000.f, 0.f), 100000.f);
		F.W.Advance(3.f, Step);
		TestTrue(TEXT("switch to Lancer"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
		F.W.Advance(1.5f, Step);
		const ATDWingmanAIController* RiftWing = F.Wing(ETDShipId::Rift);
		const ATDWingmanAIController* BastionWing = F.Wing(ETDShipId::Bastion);
		TestTrue(TEXT("Rift's new wingman picks up the fight"), RiftWing && RiftWing->GetWingmanState() == ETDWingmanState::Engage && RiftWing->GetCurrentTarget() == Enemy);
		TestTrue(TEXT("Bastion keeps fighting"), BastionWing && BastionWing->GetWingmanState() == ETDWingmanState::Engage);
		TestNull(TEXT("Lancer has no AI while the player flies it"), F.Wing(ETDShipId::Lancer));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadFocus, "TriadDrift.Squad.Wingmen.FocusCommand", TD_TEST_FLAGS)
bool FTDSquadFocus::RunTest(const FString& Parameters)
{
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		AActor* Near = F.SpawnEnemy(FVector(6000.f, 0.f, 0.f), 100000.f);
		AActor* Far = F.SpawnEnemy(FVector(14000.f, -4000.f, 0.f), 100000.f);
		F.W.Advance(1.f, Step);
		TestTrue(TEXT("without orders the wingmen take the near enemy first") + Fps,
			F.Wing(ETDShipId::Lancer) && F.Wing(ETDShipId::Bastion)
			&& (F.Wing(ETDShipId::Lancer)->GetCurrentTarget() == Near || F.Wing(ETDShipId::Bastion)->GetCurrentTarget() == Near));
		F.Squad->IssueFocusCommand(Far);
		F.W.Advance(0.6f, Step);
		TestTrue(TEXT("Lancer focuses") + Fps, F.Wing(ETDShipId::Lancer) && F.Wing(ETDShipId::Lancer)->GetCurrentTarget() == Far);
		TestTrue(TEXT("Bastion focuses") + Fps, F.Wing(ETDShipId::Bastion) && F.Wing(ETDShipId::Bastion)->GetCurrentTarget() == Far);
		F.W.Advance(7.f, Step);
		TestTrue(TEXT("focus still active at 7.6 s") + Fps, F.Squad->GetFocusTarget() == Far);
		F.W.Advance(0.6f, Step);
		TestNull(TEXT("focus expires after 8 s") + Fps, F.Squad->GetFocusTarget());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadInputPath, "TriadDrift.Squad.Switch.ThroughPlayerInput", TD_TEST_FLAGS)
bool FTDSquadInputPath::RunTest(const FString& Parameters)
{
	// The real path: Tab on T1's ATDSpacePlayerController -> ATDSquadManager::RequestSwitchNext.
	FTDTestWorld W;
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("space player controller"), PC) || !Squad)
	{
		return false;
	}
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform::Identity);
	W.Tick();
	UEnhancedPlayerInput* Input = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
	const UInputAction* Next = PC->FindActionByName(TEXT("IA_SwitchNext"));
	if (!TestNotNull(TEXT("enhanced player input"), Input) || !TestNotNull(TEXT("IA_SwitchNext"), Next))
	{
		return false;
	}
	Input->InjectInputForAction(Next, FInputActionValue(true));
	W.Tick();
	W.Tick();
	TestTrue(TEXT("Tab switched to the next ship"), Squad->GetActiveShipId() == ETDShipId::Lancer);
	TestTrue(TEXT("and the player controller possesses it"), PC->GetPawn() == Squad->GetShip(ETDShipId::Lancer));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadHyperdriveFollow, "TriadDrift.Squad.Wingmen.FollowHyperdriveNotRam", TD_TEST_FLAGS)
bool FTDSquadHyperdriveFollow::RunTest(const FString& Parameters)
{
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 0.f, 0.f), 100000.f);
		F.W.Advance(2.f, Step);
		ATDShip* Leader = F.Ship(ETDShipId::Rift);

		// A ram-speed dash (3x Lancer boost) is not a hyperdrive jump: wingmen keep fighting on their own.
		Leader->SetSpeedOverride(45000.f, 0.4f);
		F.W.Advance(0.5f, Step);
		Leader->ClearSpeedOverride();
		const ATDWingmanAIController* Wing = F.Wing(ETDShipId::Bastion);
		TestTrue(TEXT("still engaged after a ram-speed dash") + Fps, Wing && Wing->GetWingmanState() == ETDWingmanState::Engage && Wing->GetCurrentTarget() == Enemy);

		// The real hyperdrive: spool 1.2 s, then the jump at 80 000 cm/s. Wingmen ride in their slots.
		UTDHyperdriveComponent* Hyper = Leader->GetHyperdrive();
		TestTrue(TEXT("spool") + Fps, Hyper && Hyper->StartSpool());
		F.W.Advance(3.f, Step);
		TestTrue(TEXT("jumping") + Fps, Hyper && Hyper->IsJumping() && Leader->GetShipMovement()->GetSpeed() > 60000.f);
		for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
		{
			const float Dist = FVector::Dist(F.Ship(Id)->GetActorLocation(), F.Squad->GetFormationSlotLocation(Id));
			TestTrue(FString::Printf(TEXT("%s jumps in formation (%.0f cm from slot)%s"), *UEnum::GetValueAsString(Id), Dist, *Fps), Dist < 2000.f);
		}
		// Drop-out brakes the leader hard: the wingmen stay pinned through it and arrive in the V.
		TestTrue(TEXT("drop out") + Fps, Hyper->RequestDropOut());
		F.W.Advance(1.2f, Step);
		TestTrue(TEXT("jump over") + Fps, Hyper->GetState() == ETDHyperdriveState::Idle);
		for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
		{
			const float Dist = FVector::Dist(F.Ship(Id)->GetActorLocation(), F.Squad->GetFormationSlotLocation(Id));
			TestTrue(FString::Printf(TEXT("%s arrives in formation, no overshoot (%.0f cm)%s"), *UEnum::GetValueAsString(Id), Dist, *Fps), Dist < 4000.f);
		}
	}
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchCut, "TriadDrift.Squad.Switch.CutPayload", TD_TEST_FLAGS)
bool FTDSquadSwitchCut::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.Squad->OnSwitchCut.AddDynamic(F.Events, &UTDSquadTestListener::OnSwitchCut);
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	// Same call, no tick: the cut payload is already out and control already moved.
	TestEqual(TEXT("one cut"), F.Events->Cuts, 1);
	const FTDSwitchCut& Cut = F.Events->LastCut;
	TestTrue(TEXT("new/old"), Cut.NewShip == ETDShipId::Lancer && Cut.OldShip == ETDShipId::Rift);
	TestTrue(TEXT("actors"), Cut.NewShipActor == F.Ship(ETDShipId::Lancer) && Cut.OldShipActor == F.Ship(ETDShipId::Rift));
	TestEqual(TEXT("pilot"), Cut.PilotName.ToString(), FString(TEXT("KATO")));
	TestEqual(TEXT("ship"), Cut.ShipName.ToString(), FString(TEXT("LANCER")));
	TestTrue(TEXT("name card 'KATO · LANCER'"), Cut.NameCardText.ToString().StartsWith(TEXT("KATO")) && Cut.NameCardText.ToString().EndsWith(TEXT("LANCER")));
	TestTrue(TEXT("squad colour"), Cut.SquadColor.Equals(UTDShipLibrary::Get(ETDShipId::Lancer).SquadColor));
	TestFalse(TEXT("player switch, not auto"), Cut.bAutoSwitch);
	TestEqual(TEXT("1-frame flash by default"), Cut.Timing.FlashFrames, 1);
	TestEqual(TEXT("switch index"), Cut.SwitchIndex, 1);
	TestTrue(TEXT("player already in Lancer"), F.PC->GetPawn() == F.Ship(ETDShipId::Lancer));
	TestFalse(TEXT("cameras differ (a real cut)"), Cut.FromCamera.GetLocation().Equals(Cut.ToCamera.GetLocation(), 10.f));
	TestTrue(TEXT("from-camera is behind/at the old ship"), FVector::Dist(Cut.FromCamera.GetLocation(), F.Ship(ETDShipId::Rift)->GetActorLocation()) < 3000.f);
	TestTrue(TEXT("to-camera is behind/at the new ship"), FVector::Dist(Cut.ToCamera.GetLocation(), F.Ship(ETDShipId::Lancer)->GetActorLocation()) < 3000.f);
	TestTrue(TEXT("to-camera looks the new ship's way"), FVector::DotProduct(Cut.ToCamera.GetRotation().GetForwardVector(), F.Ship(ETDShipId::Lancer)->GetActorForwardVector()) > 0.7f);
	TestTrue(TEXT("GetLastSwitchCut matches"), F.Squad->GetLastSwitchCut().SwitchIndex == 1);

	F.W.Advance(0.5f);
	TestTrue(TEXT("quick second switch"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	TestTrue(TEXT("flagged as a quick cut"), F.Events->LastCut.bQuickCut && F.Events->LastCut.SwitchIndex == 2);

	F.Kill(ETDShipId::Bastion);
	F.W.Advance(0.7f);
	TestEqual(TEXT("auto-switch cuts too"), F.Events->Cuts, 3);
	TestTrue(TEXT("flagged as auto-switch"), F.Events->LastCut.bAutoSwitch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSpool, "TriadDrift.Squad.Wingmen.FormUpOnHyperdriveSpool", TD_TEST_FLAGS)
bool FTDSquadSpool::RunTest(const FString& Parameters)
{
	// Real T1 hyperdrive on the player's ship (no bus broadcasts).
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 0.f, 0.f), 100000.f);
		F.W.Advance(3.f, Step);
		TestTrue(TEXT("engaged before the spool") + Fps, F.Wing(ETDShipId::Lancer)->GetWingmanState() == ETDWingmanState::Engage);
		UTDHyperdriveComponent* Hyper = F.Ship(ETDShipId::Rift)->GetHyperdrive();
		if (!TestNotNull(TEXT("hyperdrive") + Fps, Hyper) || !TestTrue(TEXT("spool starts") + Fps, Hyper->StartSpool()))
		{
			return false;
		}
		F.W.Advance(0.5f, Step);
		for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
		{
			const ATDWingmanAIController* Wing = F.Wing(Id);
			TestTrue(TEXT("forming up for the jump") + Fps, Wing && Wing->IsFormingUpForJump() && Wing->GetWingmanState() != ETDWingmanState::Engage);
			TestNull(TEXT("dropped its target") + Fps, Wing ? Wing->GetCurrentTarget() : nullptr);
		}
		// Cancel the spool (hit / H released): back to the fight, with nothing latched.
		Hyper->CancelSpool();
		F.W.Advance(1.5f, Step);
		TestFalse(TEXT("no longer forming up") + Fps, F.Wing(ETDShipId::Lancer)->IsFormingUpForJump());
		TestTrue(TEXT("re-engages after a cancelled spool") + Fps, F.Wing(ETDShipId::Lancer)->GetCurrentTarget() == Enemy);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSpoolDeath, "TriadDrift.Squad.Wingmen.KeepFightingAfterLeaderDiesMidSpool", TD_TEST_FLAGS)
bool FTDSquadSpoolDeath::RunTest(const FString& Parameters)
{
	// B2: the player's ship dies mid-spool; the new leader isn't jumping, so the wingmen must fight.
	FSquadFixture F;
	AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 0.f, 0.f), 100000.f);
	F.W.Advance(2.f);
	UTDHyperdriveComponent* Hyper = F.Ship(ETDShipId::Rift)->GetHyperdrive();
	TestTrue(TEXT("spool starts"), Hyper && Hyper->StartSpool());
	F.W.Advance(0.4f);
	TestTrue(TEXT("Rift killed mid-spool"), F.KillWithDamage(ETDShipId::Rift));
	F.W.Advance(0.8f);
	TestTrue(TEXT("auto-switched off the dead ship"), F.Squad->GetActiveShipId() != ETDShipId::Rift);
	TestTrue(TEXT("the dead ship's hyperdrive stopped"), Hyper->GetState() == ETDHyperdriveState::Idle);
	F.W.Advance(1.5f);
	const ETDShipId Wingman = F.Squad->GetActiveShipId() == ETDShipId::Lancer ? ETDShipId::Bastion : ETDShipId::Lancer;
	TestFalse(TEXT("remaining wingman isn't stuck forming up"), F.Wing(Wingman)->IsFormingUpForJump());
	TestTrue(TEXT("and fights"), F.Wing(Wingman)->GetCurrentTarget() == Enemy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadPassive, "TriadDrift.Squad.Wingmen.PassiveForLanding", TD_TEST_FLAGS)
bool FTDSquadPassive::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.SpawnEnemy(FVector(9000.f, 0.f, 0.f), 100000.f);
	F.W.Advance(3.f);
	F.Squad->SetWingmenPassive(true);
	ATDShip* Lancer = F.Ship(ETDShipId::Lancer);
	const FTDFlightInput Before = Lancer->GetFlightInput();
	F.W.Advance(1.f);
	TestTrue(TEXT("passive flag"), F.Squad->AreWingmenPassive() && F.Wing(ETDShipId::Lancer)->IsPassive());
	TestNull(TEXT("no target while passive"), F.Wing(ETDShipId::Lancer)->GetCurrentTarget());
	TestTrue(TEXT("no new flight input while passive"), Lancer->GetFlightInput().Aim == Before.Aim && Lancer->GetFlightInput().Throttle == Before.Throttle);
	F.Squad->SetWingmenPassive(false);
	F.W.Advance(1.5f);
	TestNotNull(TEXT("fights again when active"), F.Wing(ETDShipId::Lancer)->GetCurrentTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadHUDWaveInfo, "TriadDrift.Squad.HUD.WaveInfoCachedLookup", TD_TEST_FLAGS)
bool FTDSquadHUDWaveInfo::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	TestEqual(TEXT("no director: default wave info"), F.Squad->GetWaveInfo().WaveNumber, 0);
	ATDWaveDirector* Director = F.W.Spawn<ATDWaveDirector>();
	if (!TestNotNull(TEXT("director"), Director))
	{
		return false;
	}
	F.W.Advance(1.1f); // a miss is retried after 1 s, not every frame
	const FTDWaveInfo Expected = Director->GetWaveInfo();
	const FTDWaveInfo Got = F.Squad->GetWaveInfo();
	TestTrue(TEXT("HUD forwards the director's wave info"), Got.WaveNumber == Expected.WaveNumber && Got.WaveCount == Expected.WaveCount
		&& Got.EnemiesAlive == Expected.EnemiesAlive && Got.bWaveActive == Expected.bWaveActive);
	TestTrue(TEXT("wave table is known"), Got.WaveCount > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadPowersInstalled, "TriadDrift.Squad.ShipLibrary.PowersInstalledOnce", TD_TEST_FLAGS)
bool FTDSquadPowersInstalled::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	auto Count = [](const AActor* A) { TInlineComponentArray<UTDPowerComponent*> P(A); return P.Num(); };
	TestNotNull(TEXT("Rift: portal blaster"), Cast<UTDPortalBlasterPower>(UTDPowerComponent::FindPower(F.Ship(ETDShipId::Rift), ETDPowerInput::Signature1)));
	TestNotNull(TEXT("Lancer: ram"), Cast<UTDRamPower>(UTDPowerComponent::FindPower(F.Ship(ETDShipId::Lancer), ETDPowerInput::Signature1)));
	TestNotNull(TEXT("Bastion: missile swarm"), Cast<UTDMissileSwarmPower>(UTDPowerComponent::FindPower(F.Ship(ETDShipId::Bastion), ETDPowerInput::Signature1)));
	TestNotNull(TEXT("Bastion: shield dome (hold)"), Cast<UTDShieldDomePower>(UTDPowerComponent::FindPower(F.Ship(ETDShipId::Bastion), ETDPowerInput::Signature2)));
	for (const ETDShipId Id : UTDShipLibrary::AllShips())
	{
		TestNotNull(TEXT("ability slot"), F.Ship(Id)->FindComponentByClass<UTDAbilityComponent>());
	}
	const int32 BastionPowers = Count(F.Ship(ETDShipId::Bastion));
	// Death, respawn and a retake must not stack duplicate power components.
	F.Kill(ETDShipId::Bastion);
	F.W.Advance(12.5f);
	F.Squad->SpawnSquad(FTransform::Identity);
	F.W.Tick();
	TestEqual(TEXT("no duplicate powers after respawn + retake"), Count(F.Ship(ETDShipId::Bastion)), BastionPowers);
	TestTrue(TEXT("Bastion carries both signature powers"), BastionPowers >= 2);
	// HUD shows real cooldowns: the player's power on cooldown after use.
	ATDShip* Rift = F.Ship(ETDShipId::Rift);
	UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Signature1, true);
	F.W.Tick();
	UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Signature1, false);
	F.W.Advance(0.2f);
	TestTrue(TEXT("HUD power cooldown reflects real use"), F.Squad->GetSquadHUD()[0].PowerCooldown01 < 1.f);
	TestEqual(TEXT("HUD shows a wingman's power ready"), F.Squad->GetSquadHUD()[1].PowerCooldown01, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRespawnResetsWeapons, "TriadDrift.Squad.Death.RespawnResetsWeapons", TD_TEST_FLAGS)
bool FTDSquadRespawnResetsWeapons::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	ATDShip* Lancer = F.Ship(ETDShipId::Lancer);
	UTDWeaponComponent* W = Lancer->GetWeapons();
	if (!TestNotNull(TEXT("weapons"), W))
	{
		return false;
	}
	const int32 FullAmmo = W->GetAmmo(ETDWeaponSlot::Secondary);
	for (int32 i = 0; i < 3; ++i)
	{
		W->FireOnce(ETDWeaponSlot::Secondary);
	}
	TestTrue(TEXT("ammo spent"), W->GetAmmo(ETDWeaponSlot::Secondary) < FullAmmo);
	TestTrue(TEXT("killed by real damage"), F.KillWithDamage(ETDShipId::Lancer));
	F.W.Advance(12.3f);
	TestTrue(TEXT("respawned"), F.Squad->IsShipAlive(ETDShipId::Lancer));
	TestEqual(TEXT("ammo refilled on respawn"), W->GetAmmo(ETDWeaponSlot::Secondary), FullAmmo);
	TestEqual(TEXT("heat cleared on respawn"), W->GetHeat01(ETDWeaponSlot::Primary), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadHUDTargeting, "TriadDrift.Squad.HUD.TargetingForwardsActiveShip", TD_TEST_FLAGS)
bool FTDSquadHUDTargeting::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.SpawnEnemy(FVector(8000.f, 0.f, 0.f), 100000.f);
	F.W.Advance(0.5f);
	const UTDTargetingComponent* T = F.Ship(ETDShipId::Rift)->GetTargeting();
	if (!TestNotNull(TEXT("targeting"), T))
	{
		return false;
	}
	TestEqual(TEXT("markers forwarded from the active ship"), F.Squad->GetTargetMarkers().Num(), T->GetMarkers().Num());
	TestTrue(TEXT("missile warning forwarded"), F.Squad->GetMissileWarning() == T->GetMissileWarning());
	FVector A, B;
	TestEqual(TEXT("lead indicator forwarded"), F.Squad->GetLeadIndicator(A), T->GetLeadIndicator(B));
	TestTrue(TEXT("current target forwarded"), F.Squad->GetCurrentTarget() == T->GetCurrentTarget());
	// After a switch, the HUD follows the new active ship.
	F.W.Advance(0.5f);
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	TestTrue(TEXT("HUD follows the new ship"), F.Squad->GetCurrentTarget() == F.Ship(ETDShipId::Lancer)->GetTargeting()->GetCurrentTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadPowersDataDriven, "TriadDrift.Squad.ShipLibrary.PowersComeFromTheSpec", TD_TEST_FLAGS)
bool FTDSquadPowersDataDriven::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("spec lists Rift's power class"), UTDShipLibrary::Get(ETDShipId::Rift).Signature1PowerClass == UTDPortalBlasterPower::StaticClass());
	TestTrue(TEXT("spec lists Bastion's dome on Signature2"), UTDShipLibrary::Get(ETDShipId::Bastion).Signature2PowerClass == UTDShieldDomePower::StaticClass());
	TestTrue(TEXT("Lancer has no second signature"), UTDShipLibrary::Get(ETDShipId::Lancer).Signature2PowerClass == nullptr);

	// An override (a BP child or Nick's data would do the same) swaps the class without code changes.
	FSquadFixture F(/*bSpawnSquad*/ false);
	FTDShipSpec Custom = UTDShipLibrary::Get(ETDShipId::Lancer);
	Custom.Signature1PowerClass = UTDPortalBlasterPower::StaticClass();
	F.Squad->ShipSpecOverrides.Add(ETDShipId::Lancer, Custom);
	F.Squad->SpawnSquad(FTransform::Identity);
	F.W.Tick();
	TestNotNull(TEXT("override installed"), Cast<UTDPortalBlasterPower>(UTDPowerComponent::FindPower(F.Ship(ETDShipId::Lancer), ETDPowerInput::Signature1)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadWingmanCannotFirePowers, "TriadDrift.Squad.Wingmen.CannotFirePowersEvenIfAsked", TD_TEST_FLAGS)
bool FTDSquadWingmanCannotFirePowers::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.W.Advance(0.5f);
	// Push the power input straight at the AI-flown ships: the components exist, but must refuse.
	for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
	{
		ATDShip* Ship = F.Ship(Id);
		TestNotNull(TEXT("AI-flown"), F.Wing(Id));
		for (const ETDPowerInput Slot : { ETDPowerInput::Signature1, ETDPowerInput::Signature2 })
		{
			UTDPowerComponent* Power = UTDPowerComponent::FindPower(Ship, Slot);
			if (!Power)
			{
				continue;
			}
			UTDPowerComponent::RoutePowerInput(Ship, Slot, true);
			F.W.Advance(0.6f);
			UTDPowerComponent::RoutePowerInput(Ship, Slot, false);
			TestFalse(TEXT("TryActivate refused on an AI ship"), Power->TryActivate());
			F.W.Tick();
			TestTrue(TEXT("power did not start on an AI-flown ship"), !Power->IsPowerActive() && Power->GetCooldown01() == 1.f);
		}
	}
	TestEqual(TEXT("no OnPowerUsed"), F.Events->PowersUsed, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadPowerSurvivesSwitch, "TriadDrift.Squad.Switch.PowerKeepsRunningAfterSwitch", TD_TEST_FLAGS)
bool FTDSquadPowerSurvivesSwitch::RunTest(const FString& Parameters)
{
	// ENG-07: "A power started on one ship finishes after you switch away (dome stays up)".
	FSquadFixture F;
	TestTrue(TEXT("player into Bastion"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	ATDShip* Bastion = F.Ship(ETDShipId::Bastion);
	UTDPowerComponent* Dome = UTDPowerComponent::FindPower(Bastion, ETDPowerInput::Signature2);
	if (!TestNotNull(TEXT("dome power"), Dome))
	{
		return false;
	}
	UTDPowerComponent::RoutePowerInput(Bastion, ETDPowerInput::Signature2, true);
	F.W.Advance(0.6f);
	UTDPowerComponent::RoutePowerInput(Bastion, ETDPowerInput::Signature2, false);
	F.W.Tick();
	const UTDPowerSubsystem* Powers = UTDPowerSubsystem::Get(F.W.GetWorld());
	TestTrue(TEXT("dome is up"), Powers && Powers->GetActiveDomes().Num() == 1);
	F.W.Advance(0.5f);
	TestTrue(TEXT("switch away mid-power"), F.Squad->RequestSwitchTo(ETDShipId::Rift));
	F.W.Advance(2.f);
	TestTrue(TEXT("dome stays up after the switch"), Powers && Powers->GetActiveDomes().Num() == 1);
	TestTrue(TEXT("Bastion now AI-flown"), F.Wing(ETDShipId::Bastion) != nullptr);
	F.W.Advance(5.f);
	TestTrue(TEXT("dome finishes on its own timer (6 s)"), Powers && Powers->GetActiveDomes().Num() == 0);
	TestTrue(TEXT("and goes on cooldown"), Dome->GetCooldown01() < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadPowersThroughInput, "TriadDrift.Squad.Switch.PowersThroughPlayerInput", TD_TEST_FLAGS)
bool FTDSquadPowersThroughInput::RunTest(const FString& Parameters)
{
	// The full real path for each ship: Switch1/2/3 key -> squad switch; Q -> controller ->
	// RoutePowerInput -> the ship's signature power fires; E -> the unlocked ability fires.
	FTDTestWorld W;
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("space player controller"), PC) || !Squad)
	{
		return false;
	}
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform::Identity);
	W.Tick();
	UEnhancedPlayerInput* Input = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
	if (!TestNotNull(TEXT("enhanced input"), Input))
	{
		return false;
	}
	const TPair<ETDShipId, const TCHAR*> Ships[] = {
		{ ETDShipId::Rift, TEXT("IA_Switch1") }, { ETDShipId::Lancer, TEXT("IA_Switch2") }, { ETDShipId::Bastion, TEXT("IA_Switch3") } };
	const TPair<ETDShipId, ETDAbilityId> Abilities[] = {
		{ ETDShipId::Rift, ETDAbilityId::Cloak }, { ETDShipId::Lancer, ETDAbilityId::EMPPulse }, { ETDShipId::Bastion, ETDAbilityId::GravityWell } };
	for (int32 i = 0; i < 3; ++i)
	{
		const ETDShipId Id = Ships[i].Key;
		const FString Name = UEnum::GetValueAsString(Id);
		W.Advance(0.5f); // past the switch cooldown
		if (Id != Squad->GetActiveShipId())
		{
			Input->InjectInputForAction(PC->FindActionByName(Ships[i].Value), FInputActionValue(true));
			W.Tick();
			W.Tick();
		}
		ATDShip* Ship = Squad->GetShip(Id);
		TestTrue(Name + TEXT(": switched by key"), Squad->GetActiveShipId() == Id && PC->GetPawn() == Ship);

		// Q tap -> Signature1.
		UTDPowerComponent* Power = UTDPowerComponent::FindPower(Ship, ETDPowerInput::Signature1);
		if (!TestNotNull(Name + TEXT(": signature power installed"), Power))
		{
			continue;
		}
		Input->InjectInputForAction(PC->FindActionByName(TEXT("IA_Power")), FInputActionValue(true));
		W.Tick();
		W.Tick();
		W.Advance(0.1f);
		TestTrue(Name + TEXT(": Q fired the signature power"), Power->IsPowerActive() || Power->GetCooldown01() < 1.f);

		// E -> the unlocked ability.
		if (UTDAbilityComponent* AbilityComp = Ship->FindComponentByClass<UTDAbilityComponent>())
		{
			AbilityComp->SetAbility(Abilities[i].Value);
			UTDPowerComponent* AbilityPower = AbilityComp->GetAbilityPower();
			if (TestNotNull(Name + TEXT(": ability installed"), AbilityPower))
			{
				Input->InjectInputForAction(PC->FindActionByName(TEXT("IA_Ability")), FInputActionValue(true));
				W.Tick();
				W.Tick();
				TestTrue(Name + TEXT(": E fired the ability"), AbilityPower->IsPowerActive() || AbilityPower->GetCooldown01() < 1.f);
			}
		}
	}
	return true;
}

// ------------------------------------------------------------------------------------------ ENG-08 behaviours

namespace TDSquadTests
{
	/** Angle in degrees between two directions. */
	static float AngleDeg(const FVector& A, const FVector& B)
	{
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(A.GetSafeNormal(), B.GetSafeNormal()), -1.f, 1.f)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadThreatFirst, "TriadDrift.Squad.Wingmen.TargetPriorityThreatToPlayerFirst", TD_TEST_FLAGS)
bool FTDSquadThreatFirst::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	// Near: closest to the Lancer wingman, but harmless. Threat: further away, but it just shot the player.
	AActor* Near = F.SpawnEnemy(FVector(4000.f, -3000.f, 0.f), 100000.f);
	AActor* Threat = F.SpawnEnemy(FVector(16000.f, 6000.f, 0.f), 100000.f);
	UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Rift), 1.f, ETDDamageKind::Energy, Threat, Threat);
	F.W.Advance(0.6f);
	TestTrue(TEXT("the player's attacker is engaged first by a wingman"),
		F.Wing(ETDShipId::Lancer)->GetCurrentTarget() == Threat || F.Wing(ETDShipId::Bastion)->GetCurrentTarget() == Threat);
	TestTrue(TEXT("Lancer, nearest to the harmless one, still goes for the threat"), F.Wing(ETDShipId::Lancer)->GetCurrentTarget() == Threat);
	TestNotNull(TEXT("(near enemy exists)"), Near);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadLowestHealth, "TriadDrift.Squad.Wingmen.TargetPriorityLowestHealthBreaksTies", TD_TEST_FLAGS)
bool FTDSquadLowestHealth::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.Kill(ETDShipId::Bastion); // one wingman, so the "spread targets" rule can't interfere
	F.W.Tick();
	const FVector LancerLoc = F.Ship(ETDShipId::Lancer)->GetActorLocation();
	// Two enemies at the same distance from the Lancer wingman; one is hurt.
	AActor* Healthy = F.SpawnEnemy(LancerLoc + FVector(9000.f, 2500.f, 0.f), 1000.f);
	AActor* Hurt = F.SpawnEnemy(LancerLoc + FVector(9000.f, -2500.f, 0.f), 1000.f);
	UTDStatics::ApplyTDDamage(Hurt, 600.f, ETDDamageKind::Energy, F.Ship(ETDShipId::Rift), F.Ship(ETDShipId::Rift));
	F.W.Advance(0.6f);
	TestTrue(TEXT("same distance band: the hurt enemy is picked"), F.Wing(ETDShipId::Lancer)->GetCurrentTarget() == Hurt);
	TestNotNull(TEXT("(healthy enemy exists)"), Healthy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadEvadeOnShieldBreak, "TriadDrift.Squad.Wingmen.EvadeOnShieldBreak", TD_TEST_FLAGS)
bool FTDSquadEvadeOnShieldBreak::RunTest(const FString& Parameters)
{
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		AActor* Enemy = F.SpawnEnemy(FVector(9000.f, 0.f, 0.f), 100000.f);
		F.W.Advance(2.f, Step);
		TestTrue(TEXT("engaged") + Fps, F.Wing(ETDShipId::Lancer)->GetWingmanState() == ETDWingmanState::Engage);
		// Break Lancer's 80-point shield with a real hit.
		UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Lancer), 85.f, ETDDamageKind::Energy, Enemy, Enemy);
		F.W.Tick(Step);
		F.W.Tick(Step);
		TestTrue(TEXT("evades when the shield breaks") + Fps, F.Wing(ETDShipId::Lancer)->GetWingmanState() == ETDWingmanState::Evade);
		TestTrue(TEXT("boosting away") + Fps, F.Ship(ETDShipId::Lancer)->GetFlightInput().bBoost);
		F.W.Advance(2.f, Step);
		TestTrue(TEXT("evade ends after 1.8 s") + Fps, F.Wing(ETDShipId::Lancer)->GetWingmanState() != ETDWingmanState::Evade);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadFollowPortal, "TriadDrift.Squad.Wingmen.FollowLeaderThroughPortal", TD_TEST_FLAGS)
bool FTDSquadFollowPortal::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.W.Advance(1.f);
	ATDShip* Leader = F.Ship(ETDShipId::Rift);
	// A portal / gate jump: T1's TeleportSpacecraft broadcasts OnSpacecraftTeleported on the bus.
	Leader->TeleportSpacecraft(FVector(250000.f, 40000.f, 0.f), FRotator(0.f, 90.f, 0.f), true);
	F.W.Tick();
	for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
	{
		const float Dist = FVector::Dist(F.Ship(Id)->GetActorLocation(), F.Squad->GetFormationSlotLocation(Id));
		TestTrue(FString::Printf(TEXT("%s followed through the portal (%.0f cm from slot)"), *UEnum::GetValueAsString(Id), Dist), Dist < 3000.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadAvoidObstacle, "TriadDrift.Squad.Wingmen.AvoidRegisteredObstacle", TD_TEST_FLAGS)
bool FTDSquadAvoidObstacle::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	UTDWorldSubsystem* WorldSub = F.W.GetWorld()->GetSubsystem<UTDWorldSubsystem>();
	if (!TestNotNull(TEXT("world subsystem"), WorldSub))
	{
		return false;
	}
	// An asteroid right on the Lancer wingman's line (its slot is at y = -1600), clear of the leader's.
	const FVector Rock(9000.f, -1600.f, 150.f);
	const float RockRadius = 1100.f;
	WorldSub->AddObstacle(Rock, RockRadius);
	float Closest = TNumericLimits<float>::Max();
	for (int32 i = 0; i < 240; ++i)
	{
		F.W.Tick();
		Closest = FMath::Min(Closest, FVector::Dist(F.Ship(ETDShipId::Lancer)->GetActorLocation(), Rock));
	}
	TestTrue(FString::Printf(TEXT("Lancer steered around the asteroid (closest %.0f cm, radius %.0f)"), Closest, RockRadius), Closest > RockRadius);
	const float SlotDist = FVector::Dist(F.Ship(ETDShipId::Lancer)->GetActorLocation(), F.Squad->GetFormationSlotLocation(ETDShipId::Lancer));
	TestTrue(TEXT("and rejoined formation afterwards"), SlotDist < 3000.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSeparation, "TriadDrift.Squad.Wingmen.KeepApartFromSquadmates", TD_TEST_FLAGS)
bool FTDSquadSeparation::RunTest(const FString& Parameters)
{
	FSquadFixture F(/*bSpawnSquad*/ false);
	// Force both wingmen to want the SAME slot: only teammate avoidance keeps them apart.
	F.Squad->FormationRightOffset = F.Squad->FormationLeftOffset;
	F.Squad->SpawnSquad(FTransform::Identity);
	float Closest = TNumericLimits<float>::Max();
	for (int32 i = 0; i < 480; ++i)
	{
		F.W.Tick();
		const float D = FVector::Dist(F.Ship(ETDShipId::Lancer)->GetActorLocation(), F.Ship(ETDShipId::Bastion)->GetActorLocation());
		if (i > 60)
		{
			Closest = FMath::Min(Closest, D);
		}
	}
	// Collision spheres are 220 cm (contact at 440 cm; squad ships don't block each other, M7): demand a clear gap.
	TestTrue(FString::Printf(TEXT("wingmen sharing a slot keep apart (closest %.0f cm)"), Closest), Closest > 500.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadDomeDrift, "TriadDrift.Squad.Wingmen.HurtWingmanSeeksShieldDome", TD_TEST_FLAGS)
bool FTDSquadDomeDrift::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.W.Advance(0.5f);
	// Hurt Lancer below 40% with a real hit, then raise a player dome off to the side.
	UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Lancer), 110.f, ETDDamageKind::Energy, F.SpawnEnemy(FVector(0.f, 0.f, 900000.f), 1.f), nullptr);
	TestTrue(TEXT("Lancer hurt"), UTDStatics::GetHealth(F.Ship(ETDShipId::Lancer))->GetHull01() < 0.6f);
	AActor* DomeAnchor = F.W.Spawn<AActor>(F.Ship(ETDShipId::Rift)->GetActorLocation() + FVector(9000.f, -9000.f, 0.f));
	USceneComponent* Root = NewObject<USceneComponent>(DomeAnchor, TEXT("Root"));
	DomeAnchor->SetRootComponent(Root);
	Root->RegisterComponent();
	DomeAnchor->SetActorLocation(F.Ship(ETDShipId::Rift)->GetActorLocation() + FVector(9000.f, -9000.f, 0.f));
	ATDShieldDome* Dome = ATDShieldDome::SpawnDome(F.W.GetWorld(), DomeAnchor, ETDTeam::Player, 3000.f, 20.f);
	if (!TestNotNull(TEXT("dome"), Dome))
	{
		return false;
	}
	F.W.Advance(1.f);
	bool bInside = false;
	for (int32 i = 0; i < 600 && !bInside; ++i)
	{
		F.W.Tick();
		bInside = FVector::Dist(F.Ship(ETDShipId::Lancer)->GetActorLocation(), Dome->GetActorLocation()) < Dome->GetRadius();
	}
	TestTrue(TEXT("hurt wingman flew into the dome"), bInside);
	const float BastionToDome = FVector::Dist(F.Ship(ETDShipId::Bastion)->GetActorLocation(), Dome->GetActorLocation());
	TestTrue(TEXT("healthy Bastion stays in formation instead"), BastionToDome > Dome->GetRadius());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadAimError, "TriadDrift.Squad.Wingmen.AimErrorTwoToFourDegrees", TD_TEST_FLAGS)
bool FTDSquadAimError::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	AActor* Enemy = F.SpawnEnemy(FVector(14000.f, 0.f, 0.f), 1000000.f); // stationary: lead = position
	F.W.Advance(1.5f);
	const UTDWeaponComponent* W = F.Ship(ETDShipId::Lancer)->GetWeapons();
	float MinErr = 180.f, MaxErr = 0.f;
	int32 Samples = 0;
	for (int32 i = 0; i < 180; ++i)
	{
		// The wingman aims during its tick from where it was BEFORE this frame's movement.
		const FVector AimedFrom = F.Ship(ETDShipId::Lancer)->GetActorLocation();
		F.W.Tick();
		if (F.Wing(ETDShipId::Lancer)->GetWingmanState() != ETDWingmanState::Engage)
		{
			continue;
		}
		const FVector Exact = Enemy->GetActorLocation() - AimedFrom;
		if (Exact.Size() < 3000.f)
		{
			continue; // at pass-by range the line of sight swings more in one frame than the error itself
		}
		const float Err = AngleDeg(W->GetAimDirection(), Exact);
		MinErr = FMath::Min(MinErr, Err);
		MaxErr = FMath::Max(MaxErr, Err);
		++Samples;
	}
	TestTrue(TEXT("sampled while engaging"), Samples > 60);
	TestTrue(FString::Printf(TEXT("aim error within 2-4 degrees (%.2f .. %.2f)"), MinErr, MaxErr), MinErr >= 1.9f && MaxErr <= 4.1f);
	TestTrue(TEXT("the error varies (re-rolled, not a fixed offset)"), MaxErr - MinErr > 0.3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadCatchUpBoost, "TriadDrift.Squad.Wingmen.BoostToCatchUpBeyond8000", TD_TEST_FLAGS)
bool FTDSquadCatchUpBoost::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.W.Advance(2.f);
	ATDShip* Lancer = F.Ship(ETDShipId::Lancer);
	const FVector Slot = F.Squad->GetFormationSlotLocation(ETDShipId::Lancer);
	const FVector Back = -F.Ship(ETDShipId::Rift)->GetActorForwardVector();
	Lancer->SetActorLocation(Slot + Back * 12000.f);
	F.W.Advance(0.3f);
	TestTrue(TEXT("boosts when more than 8000 cm behind"), Lancer->GetFlightInput().bBoost);
	F.W.Advance(10.f);
	const float Dist = FVector::Dist(Lancer->GetActorLocation(), F.Squad->GetFormationSlotLocation(ETDShipId::Lancer));
	TestTrue(FString::Printf(TEXT("caught up (%.0f cm)"), Dist), Dist < 1500.f);
	TestFalse(TEXT("no boost once back in the slot"), Lancer->GetFlightInput().bBoost);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadHullRepair, "TriadDrift.Squad.Death.HullRepairsBetweenWavesOnly", TD_TEST_FLAGS)
bool FTDSquadHullRepair::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld());
	AActor* Enemy = F.SpawnEnemy(FVector(0.f, 0.f, 900000.f), 1.f);
	UTDHealthComponent* H = F.Health(ETDShipId::Bastion);
	UTDStatics::ApplyTDDamage(F.Ship(ETDShipId::Bastion), 260.f, ETDDamageKind::Energy, Enemy, Enemy); // shield 160 + 100 hull
	const float HullAfterHit = H->GetHull();
	Bus->BroadcastWaveStarted(1, 0.2f);
	F.W.Advance(5.f);
	TestEqual(TEXT("no hull regeneration in combat"), H->GetHull(), HullAfterHit, 0.01f);
	Bus->BroadcastWaveCleared(1);
	F.W.Advance(5.f);
	TestTrue(TEXT("hull repairs between waves (~8/s)"), H->GetHull() > HullAfterHit + 30.f && H->GetHull() < HullAfterHit + 50.f);
	Bus->BroadcastWaveStarted(2, 0.35f);
	const float HullAtWave2 = H->GetHull();
	F.W.Advance(3.f);
	TestEqual(TEXT("repair stops when the next wave starts"), H->GetHull(), HullAtWave2, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRamEndsAfterSwitch, "TriadDrift.Squad.Switch.RamEndsCleanlyAfterSwitch", TD_TEST_FLAGS)
bool FTDSquadRamEndsAfterSwitch::RunTest(const FString& Parameters)
{
	// T2: the ram finishes its 1.2 s dash under the AI, then the speed override must be gone.
	FSquadFixture F;
	TestTrue(TEXT("player into Lancer"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	ATDShip* Lancer = F.Ship(ETDShipId::Lancer);
	UTDPowerComponent* Ram = UTDPowerComponent::FindPower(Lancer, ETDPowerInput::Signature1);
	if (!TestNotNull(TEXT("ram installed"), Ram))
	{
		return false;
	}
	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
	F.W.Tick();
	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
	F.W.Advance(0.5f); // past the 0.3 s wind-up, mid-dash
	TestTrue(TEXT("ram dashing"), Ram->IsPowerActive() && Lancer->GetShipMovement()->HasSpeedOverride());
	TestTrue(TEXT("switch away mid-dash"), F.Squad->RequestSwitchTo(ETDShipId::Rift));
	TestNotNull(TEXT("Lancer under AI during the rest of the dash"), F.Wing(ETDShipId::Lancer));
	F.W.Advance(1.5f);
	TestFalse(TEXT("ram over"), Ram->IsPowerActive());
	TestFalse(TEXT("speed override cleared"), Lancer->GetShipMovement()->HasSpeedOverride());
	// Speed after the dash is T1/T2's call (overspeed bleed); logged for them, not asserted here.
	AddInfo(FString::Printf(TEXT("Lancer speed 1.5 s after a ram + switch: %.0f cm/s"), Lancer->GetShipMovement()->GetSpeed()));
	return true;
}

// ------------------------------------------------------------------------------------------ review fixes

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadAimClearedOnHandover, "TriadDrift.Squad.Switch.WingmanAimTargetClearedOnHandover", TD_TEST_FLAGS)
bool FTDSquadAimClearedOnHandover::RunTest(const FString& Parameters)
{
	// M1: nothing of the wingman's aim may follow the ship into the player's hands.
	FSquadFixture F;
	AActor* A = F.SpawnEnemy(FVector(9000.f, -9000.f, 0.f), 100000.f);
	F.W.Advance(2.f);
	ATDShip* Lancer = F.Ship(ETDShipId::Lancer);
	UTDWeaponComponent* W = Lancer->GetWeapons();
	TestTrue(TEXT("wingman engaging A"), F.Wing(ETDShipId::Lancer)->GetCurrentTarget() == A);
	TestNull(TEXT("AI never sets an explicit aim target (m7: keeps its aim error)"), W->GetAimTarget());
	TestTrue(TEXT("real switch into Lancer"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	TestNull(TEXT("no aim target for the player"), W->GetAimTarget());
	TestTrue(TEXT("AI aim direction cleared (falls back to the nose)"), W->GetAimDirection().Equals(Lancer->GetActorForwardVector(), 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadDeathTumble, "TriadDrift.Squad.Death.TumbleParksAndRespawnResetsFlight", TD_TEST_FLAGS)
bool FTDSquadDeathTumble::RunTest(const FString& Parameters)
{
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		F.W.Advance(1.f, Step);
		ATDShip* Rift = F.Ship(ETDShipId::Rift);
		// The player is holding full stick and boost when the ship dies.
		FTDFlightInput Held;
		Held.Aim = FVector2D(1.f, 1.f);
		Held.bBoost = true;
		Rift->SetFlightInput(Held);
		F.W.Tick(Step);
		const FRotator RotAtDeath = Rift->GetActorRotation();
		const FVector LocAtDeath = Rift->GetActorLocation();
		TestTrue(TEXT("killed") + Fps, F.KillWithDamage(ETDShipId::Rift));
		F.W.Tick(Step);
		TestTrue(TEXT("flight parked during the tumble (stick and boost do nothing)") + Fps, Rift->IsFlightParked());
		TestTrue(TEXT("weapons blocked") + Fps, Rift->GetWeapons()->AreWeaponsBlocked());
		F.W.Advance(0.3f, Step);
		TestTrue(TEXT("visibly tumbling") + Fps, !Rift->GetActorRotation().Equals(RotAtDeath, 20.f));
		TestTrue(TEXT("drifting on") + Fps, FVector::Dist(Rift->GetActorLocation(), LocAtDeath) > 300.f);
		TestFalse(TEXT("still visible during the tumble") + Fps, Rift->IsHidden());
		F.W.Advance(0.4f, Step);
		TestTrue(TEXT("hidden and still parked once down") + Fps, Rift->IsHidden() && Rift->IsFlightParked());
		// Check the reset in the frame the ship comes back (its wingman may legitimately boost after).
		for (int32 i = 0; i < 2000 && !F.Squad->IsShipAlive(ETDShipId::Rift); ++i)
		{
			F.W.Tick(Step);
		}
		TestTrue(TEXT("respawned") + Fps, F.Squad->IsShipAlive(ETDShipId::Rift));
		TestFalse(TEXT("flight running again") + Fps, Rift->IsFlightParked());
		TestFalse(TEXT("weapons unblocked") + Fps, Rift->GetWeapons()->AreWeaponsBlocked());
		TestTrue(TEXT("latched input cleared") + Fps, !Rift->GetFlightInput().bBoost || F.Wing(ETDShipId::Rift) != nullptr);
		TestEqual(TEXT("boost meter full") + Fps, Rift->GetBoost01(), 1.f, 0.01f);
		TestFalse(TEXT("no speed override") + Fps, Rift->GetShipMovement()->HasSpeedOverride());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRetakeMidJump, "TriadDrift.Squad.Death.RetakeMidJumpStartsClean", TD_TEST_FLAGS)
bool FTDSquadRetakeMidJump::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.W.Advance(0.5f);
	ATDShip* Rift = F.Ship(ETDShipId::Rift);
	UTDHyperdriveComponent* Hyper = Rift->GetHyperdrive();
	TestTrue(TEXT("spool"), Hyper && Hyper->StartSpool());
	F.W.Advance(2.5f);
	TestTrue(TEXT("jumping"), Hyper->IsJumping());
	F.Kill(ETDShipId::Lancer);
	F.Kill(ETDShipId::Bastion);
	F.Kill(ETDShipId::Rift);
	F.W.Tick();
	F.Squad->SpawnSquad(FTransform::Identity);
	F.W.Advance(0.1f);
	TestTrue(TEXT("take 2 does not start at jump speed"), Rift->GetShipMovement()->GetSpeed() < 20000.f);
	F.W.Advance(1.f);
	TestTrue(TEXT("hyperdrive idle in the new take"), Hyper->GetState() == ETDHyperdriveState::Idle);
	TestTrue(TEXT("flying at cruise"), Rift->GetShipMovement()->GetSpeed() < 8000.f && !Rift->IsFlightParked());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchUnderSlowMo, "TriadDrift.Squad.Switch.CooldownIsRealTimeUnderSlowMo", TD_TEST_FLAGS)
bool FTDSquadSwitchUnderSlowMo::RunTest(const FString& Parameters)
{
	// M5: a money shot must not swallow Tab presses.
	FSquadFixture F;
	TestTrue(TEXT("first switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	UTDFeelSubsystem::Get(F.W.GetWorld())->RequestSlowMo(3.f, 0.2f);
	F.W.Advance(0.45f); // real seconds: only ~0.09 s of game time pass
	TestTrue(TEXT("slow-mo running"), UTDFeelSubsystem::Get(F.W.GetWorld())->IsSlowMoActive());
	TestTrue(TEXT("switch allowed after 0.4 s of real time"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSwitchGuards, "TriadDrift.Squad.Switch.RefusedMidJumpAndLanding", TD_TEST_FLAGS)
bool FTDSquadSwitchGuards::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	F.Squad->OnSwitchRefused.AddDynamic(F.Events, &UTDSquadTestListener::OnSwitchRefused);
	F.W.Advance(0.5f);
	ATDShip* Rift = F.Ship(ETDShipId::Rift);

	// Mid-spool.
	UTDHyperdriveComponent* Hyper = Rift->GetHyperdrive();
	TestTrue(TEXT("spool"), Hyper && Hyper->StartSpool());
	TestFalse(TEXT("no manual switch while spooling"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	TestTrue(TEXT("refusal reason: hyperdrive"), F.Events->LastRefusal == ETDSwitchRefusal::HyperdriveActive);
	Hyper->CancelSpool();
	TestTrue(TEXT("switch allowed once idle"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	F.W.Advance(0.5f);
	TestTrue(TEXT("back to Rift"), F.Squad->RequestSwitchTo(ETDShipId::Rift));
	F.W.Advance(0.5f);

	// Landing.
	ATDLandingPad* Pad = F.W.Spawn<ATDLandingPad>(Rift->GetActorLocation() + FVector(3000.f, 0.f, -500.f));
	UTDLandingComponent* Landing = Rift->GetLanding();
	if (!TestNotNull(TEXT("landing component"), Landing) || !TestNotNull(TEXT("pad"), Pad))
	{
		return false;
	}
	TestTrue(TEXT("landing starts"), Landing->LandOnPad(Pad));
	F.W.Tick();
	TestFalse(TEXT("no manual switch while landing"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	TestTrue(TEXT("refusal reason: landing"), F.Events->LastRefusal == ETDSwitchRefusal::Landing);
	TestFalse(TEXT("HUD-ready reason text from T1"), F.Squad->GetLastRefusalText().IsEmpty());

	// Death still auto-switches, landing or not (landing makes the ship invulnerable, so a direct kill).
	F.Kill(ETDShipId::Rift);
	F.W.Advance(0.8f);
	TestTrue(TEXT("auto-switch still works"), F.Squad->GetActiveShipId() != ETDShipId::Rift);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRollingLeader, "TriadDrift.Squad.Wingmen.RollingOrTurningLeaderNoCollisions", TD_TEST_FLAGS)
bool FTDSquadRollingLeader::RunTest(const FString& Parameters)
{
	// M7: a barrel roll or a hard turn must not swing the slots through the player or each other.
	for (const int32 Mode : { 0, 1 })
	{
		const FString Label = Mode == 0 ? TEXT(" (barrel roll)") : TEXT(" (hard turn)");
		FSquadFixture F;
		F.W.Advance(3.f);
		ATDShip* Leader = F.Ship(ETDShipId::Rift);
		FTDFlightInput In;
		if (Mode == 0)
		{
			In.Roll = 1.f;
		}
		else
		{
			In.Aim = FVector2D(1.f, 0.f);
		}
		Leader->SetFlightInput(In);
		float ClosestToLeader = TNumericLimits<float>::Max();
		float ClosestPair = TNumericLimits<float>::Max();
		const FVector SlotBefore = F.Squad->GetFormationSlotLocation(ETDShipId::Lancer) - Leader->GetActorLocation();
		for (int32 i = 0; i < 360; ++i)
		{
			F.W.Tick();
			const FVector L = Leader->GetActorLocation();
			ClosestToLeader = FMath::Min(ClosestToLeader, FMath::Min(FVector::Dist(F.Ship(ETDShipId::Lancer)->GetActorLocation(), L),
				FVector::Dist(F.Ship(ETDShipId::Bastion)->GetActorLocation(), L)));
			ClosestPair = FMath::Min(ClosestPair, FVector::Dist(F.Ship(ETDShipId::Lancer)->GetActorLocation(), F.Ship(ETDShipId::Bastion)->GetActorLocation()));
		}
		TestTrue(FString::Printf(TEXT("wingmen never touch the player (closest %.0f cm)%s"), ClosestToLeader, *Label), ClosestToLeader > 600.f);
		TestTrue(FString::Printf(TEXT("wingmen never touch each other (closest %.0f cm)%s"), ClosestPair, *Label), ClosestPair > 600.f);
		if (Mode == 0)
		{
			const FVector SlotAfter = F.Squad->GetFormationSlotLocation(ETDShipId::Lancer) - Leader->GetActorLocation();
			TestTrue(TEXT("a roll doesn't move the slot (world-up frame)"), FMath::Abs(SlotAfter.Z - SlotBefore.Z) < 300.f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadHoldPowerThroughSwitch, "TriadDrift.Squad.Switch.HoldingPowerThenSwitchIsSafe", TD_TEST_FLAGS)
bool FTDSquadHoldPowerThroughSwitch::RunTest(const FString& Parameters)
{
	// m15 through the real controller: hold Q on Bastion (hold = shield dome), Tab away before the
	// 0.35 s hold threshold, keep Q held, release. Nothing may fire on the ship you left, and the
	// held key must not trigger anything strange on the new one.
	FTDTestWorld W;
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("space player controller"), PC) || !Squad)
	{
		return false;
	}
	UTDSquadTestListener* Events = NewObject<UTDSquadTestListener>();
	Events->AddToRoot();
	Events->Listen(UTDEventBus::Get(W.GetWorld()));
	ON_SCOPE_EXIT { Events->StopListening(); Events->RemoveFromRoot(); };
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform::Identity);
	W.Tick();
	UEnhancedPlayerInput* Input = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
	if (!TestNotNull(TEXT("enhanced input"), Input))
	{
		return false;
	}
	TestTrue(TEXT("into Bastion"), Squad->RequestSwitchTo(ETDShipId::Bastion));
	W.Advance(0.5f);
	const UInputAction* Q = PC->FindActionByName(TEXT("IA_Power"));
	auto HoldQ = [&](float Seconds)
	{
		for (int32 i = 0, N = FMath::CeilToInt(Seconds * 60.f); i < N; ++i)
		{
			Input->InjectInputForAction(Q, FInputActionValue(true));
			W.Tick();
		}
	};
	HoldQ(0.2f);
	Input->InjectInputForAction(PC->FindActionByName(TEXT("IA_Switch1")), FInputActionValue(true));
	W.Tick();
	TestTrue(TEXT("switched to Rift mid-hold"), Squad->GetActiveShipId() == ETDShipId::Rift);
	HoldQ(0.5f); // keep holding past the 0.35 s threshold
	W.Advance(0.5f); // release
	const UTDPowerSubsystem* Powers = UTDPowerSubsystem::Get(W.GetWorld());
	TestTrue(TEXT("no dome on the ship we left"), Powers && Powers->GetActiveDomes().Num() == 0);
	TestFalse(TEXT("Bastion's dome power never started"), UTDPowerComponent::FindPower(Squad->GetShip(ETDShipId::Bastion), ETDPowerInput::Signature2)->IsPowerActive());
	TestTrue(TEXT("at most the new ship's own power fired"), Events->PowersUsed <= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRealFlowWipe, "TriadDrift.Squad.Death.WipeThroughRealGameFlow", TD_TEST_FLAGS)
bool FTDSquadRealFlowWipe::RunTest(const FString& Parameters)
{
	// Wipe -> UTDGameInstance "CUT!" -> retake -> SpawnSquad, with T5's real game instance and game mode.
	FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
	W.Advance(0.5f);
	ATDSquadManager* Squad = ATDSquadManager::Get(W.GetWorld());
	if (!TestNotNull(TEXT("game mode spawned the squad manager"), Squad))
	{
		return false;
	}
	TestEqual(TEXT("squad spawned"), Squad->GetLivingShips().Num(), 3);
	UTDSquadTestListener* Events = NewObject<UTDSquadTestListener>();
	Events->AddToRoot();
	Events->Listen(UTDEventBus::Get(W.GetWorld()));
	ON_SCOPE_EXIT { Events->StopListening(); Events->RemoveFromRoot(); };
	for (const ETDShipId Id : UTDShipLibrary::AllShips())
	{
		if (UTDHealthComponent* H = UTDStatics::GetHealth(Squad->GetShip(Id)))
		{
			H->Kill();
		}
	}
	W.Tick();
	TestEqual(TEXT("wipe fired once"), Events->Wipes, 1);
	const UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	TestTrue(TEXT("flow took the defeat: retake pending"), GI && GI->IsRetakePending());
	// The flow's CUT delay runs on the core ticker (real time); drive it alongside the world, as T5's tests do.
	UTDGameInstance* MutableGI = Cast<UTDGameInstance>(W.GetGameInstance());
	const int32 Frames = FMath::CeilToInt((UTDFlowSettings::Get()->RetakeDelaySeconds + 1.5f) * 60.f);
	for (int32 i = 0; i < Frames; ++i)
	{
		W.Tick();
		if (MutableGI)
		{
			MutableGI->TickFlow(1.f / 60.f);
		}
	}
	TestEqual(TEXT("take 2: the flow respawned all three"), Squad->GetLivingShips().Num(), 3);
	TestFalse(TEXT("retake done"), GI && GI->IsRetakePending());
	W.Advance(13.f);
	TestEqual(TEXT("no stale respawn events from take 1"), Events->Respawned, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadThroughGate, "TriadDrift.Squad.Wingmen.SquadThroughRealPortalGate", TD_TEST_FLAGS)
bool FTDSquadThroughGate::RunTest(const FString& Parameters)
{
	// T4's real gate pair: in through A (facing +X at the origin), out of B (far away, facing +Y).
	FSquadFixture F(/*bSpawnSquad*/ false);
	ATDPortalGate* A = F.W.Spawn<ATDPortalGate>(FVector::ZeroVector, FRotator::ZeroRotator);
	ATDPortalGate* B = F.W.Spawn<ATDPortalGate>(FVector(100000.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f));
	if (!TestNotNull(TEXT("gates"), A) || !TestNotNull(TEXT("gates"), B))
	{
		return false;
	}
	ATDPortalGate::LinkGates(A, B);
	F.Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(-6000.f, 0.f, 0.f)));
	F.W.Advance(3.f);
	ATDShip* Leader = F.Ship(ETDShipId::Rift);
	TestTrue(TEXT("leader came out of B"), FVector::Dist(Leader->GetActorLocation(), B->GetActorLocation()) < 30000.f);
	const int32 TransitsAfterJump = A->GetTransitCount() + B->GetTransitCount();
	F.W.Advance(3.f);
	for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion })
	{
		const FVector P = F.Ship(Id)->GetActorLocation();
		TestTrue(FString::Printf(TEXT("%s followed out of B"), *UEnum::GetValueAsString(Id)), FVector::Dist(P, B->GetActorLocation()) < 30000.f);
		TestTrue(FString::Printf(TEXT("%s is on the exit side of B's plane"), *UEnum::GetValueAsString(Id)),
			FVector::DotProduct(P - B->GetActorLocation(), B->GetActorForwardVector()) > 0.f);
	}
	TestEqual(TEXT("nobody bounced back through the gates"), A->GetTransitCount() + B->GetTransitCount(), TransitsAfterJump);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadManualTargetCarry, "TriadDrift.Squad.Switch.ManualTargetCarriesOver", TD_TEST_FLAGS)
bool FTDSquadManualTargetCarry::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	AActor* Picked = F.SpawnEnemy(FVector(12000.f, 3000.f, 0.f), 100000.f);
	F.W.Advance(0.5f);
	UTDTargetingComponent* RiftTargeting = F.Ship(ETDShipId::Rift)->GetTargeting();
	RiftTargeting->SetManualTarget(Picked);
	TestTrue(TEXT("player picked a target by hand"), RiftTargeting->IsManualTarget());
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	const UTDTargetingComponent* LancerTargeting = F.Ship(ETDShipId::Lancer)->GetTargeting();
	TestTrue(TEXT("the manual target survives the cut"), LancerTargeting->GetCurrentTarget() == Picked && LancerTargeting->IsManualTarget());
	TestTrue(TEXT("HUD shows it"), F.Squad->GetCurrentTarget() == Picked);
	TestTrue(TEXT("HUD hyperdrive charge forwarded"), F.Squad->GetHyperdriveCharge01() == F.Ship(ETDShipId::Lancer)->GetHyperdrive()->GetCharge01());
	TestEqual(TEXT("HUD altitude forwarded"), F.Squad->GetAltitude(), F.Ship(ETDShipId::Lancer)->GetAltitudeMeters());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadEvadeOnMissile, "TriadDrift.Squad.Wingmen.EvadeOnIncomingMissile", TD_TEST_FLAGS)
bool FTDSquadEvadeOnMissile::RunTest(const FString& Parameters)
{
	for (const float Step : GFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FSquadFixture F;
		F.W.Advance(1.f, Step);
		ATDShip* Lancer = F.Ship(ETDShipId::Lancer);
		TestTrue(TEXT("in formation, not evading") + Fps, F.Wing(ETDShipId::Lancer)->GetWingmanState() != ETDWingmanState::Evade);
		// A real enemy homing missile, launched at the Lancer wingman from ahead.
		AActor* Shooter = F.SpawnEnemy(Lancer->GetActorLocation() + Lancer->GetActorForwardVector() * 30000.f, 1000.f);
		ATDProjectile* Missile = TDPowerTest::LaunchMissile(F.W, Lancer->GetActorLocation() + Lancer->GetActorForwardVector() * 12000.f,
			-Lancer->GetActorForwardVector(), Shooter, Lancer);
		if (!TestNotNull(TEXT("missile") + Fps, Missile))
		{
			return false;
		}
		bool bEvaded = false;
		for (int32 i = 0; i < FMath::CeilToInt(0.5f / Step) && !bEvaded; ++i)
		{
			F.W.Tick(Step);
			bEvaded = F.Wing(ETDShipId::Lancer)->GetWingmanState() == ETDWingmanState::Evade;
		}
		TestTrue(TEXT("breaks off when a missile homes on it") + Fps, bEvaded);
		TestFalse(TEXT("Bastion (not targeted) doesn't") + Fps, F.Wing(ETDShipId::Bastion)->GetWingmanState() == ETDWingmanState::Evade);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS


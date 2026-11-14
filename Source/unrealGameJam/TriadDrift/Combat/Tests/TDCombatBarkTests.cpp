// T2's dialogs.md bark triggers (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Barks
// Each trigger asks once per condition: these count the asks (FTDCombatBarks::GetRequestCount), not what the squad's
// limiter then plays (that is T3's, tested in TriadDrift.Squad).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDPortalTransit.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDCombatBarkTests
{
	struct FSquadFixture
	{
		FTDTestWorld W;
		ATDSquadManager* Squad = nullptr;

		bool Init(FAutomationTestBase& T)
		{
			FTDCombatBarks::ResetRequestCounts();
			Squad = W.Spawn<ATDSquadManager>();
			ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
			if (!T.TestNotNull(TEXT("squad"), Squad) || !T.TestNotNull(TEXT("controller"), PC))
			{
				return false;
			}
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20000.f)));
			W.Advance(.3f);
			return T.TestNotNull(TEXT("a flown ship"), Squad->GetActiveShip());
		}

		ATDShip* Fly(ETDShipId Ship)
		{
			if (Squad->GetActiveShipId() != Ship)
			{
				Squad->RequestSwitchTo(Ship);
				W.Advance(1.f);
			}
			return Squad->GetActiveShipId() == Ship ? Squad->GetActiveShip() : nullptr;
		}

		static int32 N(FName Id) { return FTDCombatBarks::GetRequestCount(Id); }
	};
}

using namespace TDCombatBarkTests;

// Ammo: one AmmoLow on the shot that crosses 25%, one AmmoOut on the shot that empties the rack; dry presses add none.
// Heat: one WeaponHot per overheat. A wingman's weapons never ask (only the flown pilot speaks for the player's guns).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksWeapons, "TriadDrift.Combat.Barks.AmmoAndHeatOncePerCondition", TD_TEST_FLAGS)
bool FTDBarksWeapons::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	ATDShip* Flown = F.Squad->GetActiveShip();
	UTDWeaponComponent* Weapons = Flown->FindComponentByClass<UTDWeaponComponent>();
	if (!TestNotNull(TEXT("weapons"), Weapons))
	{
		return false;
	}
	Weapons->EquipWeapon(ETDWeaponSlot::Secondary, TDWeaponIds::SpikeRockets);
	const int32 Max = Weapons->GetAmmo(ETDWeaponSlot::Secondary);
	int32 Shots = 0;
	while (Weapons->GetAmmo(ETDWeaponSlot::Secondary) > 0 && Shots < 100)
	{
		if (Weapons->FireOnce(ETDWeaponSlot::Secondary))
		{
			++Shots;
		}
		F.W.Advance(.6f);
	}
	TestTrue(FString::Printf(TEXT("emptied the rack (%d of %d)"), Shots, Max), Weapons->GetAmmo(ETDWeaponSlot::Secondary) == 0);
	TestEqual(TEXT("AmmoLow asked once"), F.N(TDBarkIdsT2::AmmoLow), 1);
	TestEqual(TEXT("AmmoOut asked once"), F.N(TDBarkIdsT2::AmmoOut), 1);
	for (int32 i = 0; i < 4; ++i)
	{
		Weapons->FireOnce(ETDWeaponSlot::Secondary);
		F.W.Advance(.1f);
	}
	TestEqual(TEXT("dry presses and regen trickle: AmmoOut still once"), F.N(TDBarkIdsT2::AmmoOut), 1);
	// A full rack (re-equip = refill) re-arms both: emptying it again asks each once more.
	Weapons->EquipWeapon(ETDWeaponSlot::Secondary, TDWeaponIds::SpikeRockets);
	for (int32 k = 0; k < 100 && Weapons->GetAmmo(ETDWeaponSlot::Secondary) > 0; ++k)
	{
		Weapons->FireOnce(ETDWeaponSlot::Secondary);
		F.W.Advance(.6f);
	}
	TestEqual(TEXT("refilled and emptied: AmmoLow twice"), F.N(TDBarkIdsT2::AmmoLow), 2);
	TestEqual(TEXT("refilled and emptied: AmmoOut twice"), F.N(TDBarkIdsT2::AmmoOut), 2);

	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
	Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
	for (float T = 0.f; T < 10.f && !Weapons->IsOverheated(ETDWeaponSlot::Primary); T += 1.f / 60.f)
	{
		F.W.Tick();
	}
	TestTrue(TEXT("overheated"), Weapons->IsOverheated(ETDWeaponSlot::Primary));
	F.W.Advance(.3f); // still locked out, trigger still held
	Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false);
	TestEqual(TEXT("WeaponHot asked once for one overheat"), F.N(TDBarkIdsT2::WeaponHot), 1);

	// A wingman's gun overheating: no line.
	for (int32 I = 0; I < 3; ++I)
	{
		ATDShip* Other = F.Squad->GetShip(static_cast<ETDShipId>(I));
		if (Other && Other != Flown)
		{
			UTDWeaponComponent* W2 = Other->FindComponentByClass<UTDWeaponComponent>();
			if (W2)
			{
				W2->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
				for (int32 k = 0; k < 200 && !W2->IsOverheated(ETDWeaponSlot::Primary); ++k)
				{
					W2->FireOnce(ETDWeaponSlot::Primary);
					F.W.Advance(.05f);
				}
				TestTrue(TEXT("wingman gun overheated"), W2->IsOverheated(ETDWeaponSlot::Primary));
			}
			break;
		}
	}
	TestEqual(TEXT("wingman overheat: no WeaponHot"), F.N(TDBarkIdsT2::WeaponHot), 1);
	return true;
}

// Lancer: RamGo once per activation; PowerNotReady once per cooldown however often Q is pressed (and again next
// cooldown); PowerReady on 1 in 2 cooldown ends; AbilityLocked once for a burst of E presses with nothing earned.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksPowers, "TriadDrift.Combat.Barks.PowerReadyNotReadyLocked", TD_TEST_FLAGS)
bool FTDBarksPowers::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	ATDShip* Lancer = F.Fly(ETDShipId::Lancer);
	UTDPowerComponent* Ram = Lancer ? UTDPowerComponent::FindPower(Lancer, ETDPowerInput::Signature1) : nullptr;
	if (!TestNotNull(TEXT("flying Lancer with its ram"), Ram))
	{
		return false;
	}
	auto Cooldown = [&]()
	{
		for (float T = 0.f; T < 30.f && Ram->GetCooldown01() < 1.f; T += .1f)
		{
			F.W.Advance(.1f);
		}
	};
	Cooldown();
	const int32 Ready0 = F.N(TDBarkIdsT2::PowerReady);
	TestTrue(TEXT("ram fires"), UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true) && Ram->GetCooldown01() < 1.f);
	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
	TestEqual(TEXT("RamGo once"), F.N(TDBarkIdsT2::RamGo), 1);
	F.W.Advance(2.f); // dash over, cooling
	for (int32 i = 0; i < 4; ++i)
	{
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
		F.W.Advance(.1f);
	}
	TestEqual(TEXT("PowerNotReady once for four presses"), F.N(TDBarkIdsT2::PowerNotReady), 1);
	Cooldown();
	TestEqual(TEXT("PowerReady: the first cooldown end asks"), F.N(TDBarkIdsT2::PowerReady), Ready0 + 1);

	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
	TestEqual(TEXT("RamGo again for a second activation"), F.N(TDBarkIdsT2::RamGo), 2);
	F.W.Advance(2.f);
	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
	TestEqual(TEXT("PowerNotReady again: a new cooldown"), F.N(TDBarkIdsT2::PowerNotReady), 2);
	Cooldown();
	TestEqual(TEXT("PowerReady: 1 in 2, the second end is quiet"), F.N(TDBarkIdsT2::PowerReady), Ready0 + 1);

	UTDAbilityComponent* Ability = Lancer->FindComponentByClass<UTDAbilityComponent>();
	if (TestNotNull(TEXT("ability slot"), Ability))
	{
		Ability->SetAbility(ETDAbilityId::None);
		for (int32 i = 0; i < 5; ++i)
		{
			UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Ability, true);
			UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Ability, false);
			F.W.Advance(.2f);
		}
		TestEqual(TEXT("AbilityLocked once for five presses"), F.N(TDBarkIdsT2::AbilityLocked), 1);

		// EMP with nothing near: "Pulse out." (not the 3+ stunned line).
		Ability->SetAbility(ETDAbilityId::EMPPulse);
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Ability, true);
		TestEqual(TEXT("EMP_Fire once"), F.N(TDBarkIdsT2::EMPFire), 1);
		TestEqual(TEXT("EMP_Hit not asked (nobody stunned)"), F.N(TDBarkIdsT2::EMPHit), 0);
	}
	return true;
}

// A hostile homing missile launched at the flown ship asks MissileIncoming once (a wingman says it), however often it
// re-targets; one launched at a wingman asks nothing; a player missile asks nothing.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksMissile, "TriadDrift.Combat.Barks.MissileIncomingOncePerMissile", TD_TEST_FLAGS)
bool FTDBarksMissile::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	ATDShip* Flown = F.Squad->GetActiveShip();
	ATDShip* Wing = nullptr;
	for (int32 I = 0; I < 3 && !Wing; ++I)
	{
		ATDShip* S = F.Squad->GetShip(static_cast<ETDShipId>(I));
		Wing = S != Flown ? S : nullptr;
	}
	FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile;
	TestTrue(TEXT("enemy missile spec homes"), Spec.bIsMissile && Spec.bHoming);
	const FTransform From(FRotator::ZeroRotator, Flown->GetActorLocation() + FVector(-30000.f, 0.f, 0.f));
	ATDProjectile* M = ATDProjectile::Spawn(F.W.GetWorld(), Spec, From, nullptr, ETDTeam::Enemy, Flown);
	TestEqual(TEXT("launched at the flown ship: asked once"), F.N(TDBarkIdsT2::MissileIncoming), 1);
	if (M)
	{
		M->SetHomingTarget(Flown);
		M->SetHomingTarget(Wing);
		M->SetHomingTarget(Flown);
	}
	TestEqual(TEXT("re-targets on the same missile: still once"), F.N(TDBarkIdsT2::MissileIncoming), 1);
	ATDProjectile::Spawn(F.W.GetWorld(), Spec, From, nullptr, ETDTeam::Enemy, Wing);
	TestEqual(TEXT("at a wingman: nothing"), F.N(TDBarkIdsT2::MissileIncoming), 1);
	ATDProjectile::Spawn(F.W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::SpikeRockets).Projectile, From, Wing, ETDTeam::Player, Flown);
	TestEqual(TEXT("a player missile: nothing"), F.N(TDBarkIdsT2::MissileIncoming), 1);
	ATDProjectile::Spawn(F.W.GetWorld(), Spec, From, nullptr, ETDTeam::Enemy, Flown);
	TestEqual(TEXT("a second missile at the flown ship: asked again"), F.N(TDBarkIdsT2::MissileIncoming), 2);
	return true;
}

// dialogs.md Bark_MissileEvaded (T3's ask): a warned missile that ends without reaching the flown ship (shot down,
// expired, returned) has the flown pilot say it once; one that hits the flown ship says nothing; an unwarned one nothing.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksMissileEvaded, "TriadDrift.Combat.Barks.MissileEvadedWhenItMisses", TD_TEST_FLAGS)
bool FTDBarksMissileEvaded::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	ATDShip* Flown = F.Squad->GetActiveShip();
	ATDShip* Wing = nullptr;
	for (int32 I = 0; I < 3 && !Wing; ++I)
	{
		ATDShip* S = F.Squad->GetShip(static_cast<ETDShipId>(I));
		Wing = S != Flown ? S : nullptr;
	}
	const FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile;
	const FTransform Far(FRotator::ZeroRotator, Flown->GetActorLocation() + FVector(-30000.f, 0.f, 0.f));
	ATDProjectile* ShotDown = ATDProjectile::Spawn(F.W.GetWorld(), Spec, Far, nullptr, ETDTeam::Enemy, Flown);
	if (ShotDown) { ShotDown->Kill(true); }
	TestEqual(TEXT("shot down before reaching the flown ship: evaded once"), F.N(TDBarkIdsT2::MissileEvaded), 1);
	if (ATDProjectile* AtWing = ATDProjectile::Spawn(F.W.GetWorld(), Spec, Far, nullptr, ETDTeam::Enemy, Wing)) { AtWing->Kill(true); }
	TestEqual(TEXT("a missile at a wingman (never warned): nothing"), F.N(TDBarkIdsT2::MissileEvaded), 1);

	// Launched point-blank at the flown ship: it hits, nothing evaded.
	const FVector Dir = FVector::ForwardVector;
	const FTransform Close(Dir.Rotation(), Flown->GetActorLocation() - Dir * 1500.f);
	ATDProjectile* Hits = ATDProjectile::Spawn(F.W.GetWorld(), Spec, Close, nullptr, ETDTeam::Enemy, Flown);
	TestNotNull(TEXT("close missile"), Hits);
	F.W.Advance(2.f);
	TestEqual(TEXT("it reached the flown ship: no evaded line"), F.N(TDBarkIdsT2::MissileEvaded), 1);
	return true;
}

// Portals: the flown ship through a portal asks PortalThrough once per transit (the re-entry cooldown refuses an
// immediate second one); an enemy missile sent back asks PortalMissileReturn once (it is the player's afterwards).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksPortal, "TriadDrift.Combat.Barks.PortalThroughAndReturn", TD_TEST_FLAGS)
bool FTDBarksPortal::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	ATDShip* Flown = F.Squad->GetActiveShip();
	const FVector Here = Flown->GetActorLocation();
	const FTransform Entry(FRotator::ZeroRotator, Here);
	const FTransform Exit(FRotator(0.f, 90.f, 0.f), Here + FVector(0.f, 60000.f, 0.f));
	const FTDPortalTransitParams Params;
	TestTrue(TEXT("flown ship transits"), UTDPortalTransit::TransitActor(Flown, Entry, Exit, Flown, Params));
	UTDPortalTransit::TransitActor(Flown, Exit, Entry, Flown, Params); // refused: re-entry cooldown
	TestEqual(TEXT("PortalThrough once"), F.N(TDBarkIdsT2::PortalThrough), 1);

	FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile;
	ATDProjectile* M = ATDProjectile::Spawn(F.W.GetWorld(), Spec, FTransform(FRotator::ZeroRotator, Here + FVector(0.f, -20000.f, 0.f)), nullptr, ETDTeam::Enemy);
	if (TestNotNull(TEXT("missile"), M))
	{
		const FTransform E2(FRotator::ZeroRotator, M->GetActorLocation());
		TestTrue(TEXT("missile transits"), UTDPortalTransit::TransitActor(M, E2, Exit, Flown, Params));
		F.W.Advance(1.5f);
		UTDPortalTransit::TransitActor(M, FTransform(FRotator::ZeroRotator, M->GetActorLocation()), Entry, Flown, Params);
	}
	TestEqual(TEXT("PortalMissileReturn once"), F.N(TDBarkIdsT2::PortalMissileReturn), 1);
	return true;
}

// Rift's cloak: CloakOn on activation; the shot that breaks it asks CloakAmbush once (later shots: nothing).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksCloak, "TriadDrift.Combat.Barks.CloakOnAndAmbush", TD_TEST_FLAGS)
bool FTDBarksCloak::RunTest(const FString& Parameters)
{
	FSquadFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	ATDShip* Rift = F.Fly(ETDShipId::Rift);
	UTDAbilityComponent* Ability = Rift ? Rift->FindComponentByClass<UTDAbilityComponent>() : nullptr;
	UTDWeaponComponent* Weapons = Rift ? Rift->FindComponentByClass<UTDWeaponComponent>() : nullptr;
	if (!TestNotNull(TEXT("Rift ability"), Ability) || !TestNotNull(TEXT("Rift weapons"), Weapons))
	{
		return false;
	}
	Ability->SetAbility(ETDAbilityId::Cloak);
	F.W.Tick();
	TestTrue(TEXT("cloak on"), UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Ability, true) && Ability->GetAbilityPower()->IsPowerActive());
	TestEqual(TEXT("CloakOn once"), F.N(TDBarkIdsT2::CloakOn), 1);
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
	Weapons->FireOnce(ETDWeaponSlot::Primary);
	F.W.Advance(.5f);
	Weapons->FireOnce(ETDWeaponSlot::Primary);
	TestEqual(TEXT("CloakAmbush once"), F.N(TDBarkIdsT2::CloakAmbush), 1);
	return true;
}

#endif

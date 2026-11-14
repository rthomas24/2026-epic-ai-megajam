// Space pickups (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Pickups

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/IConsoleManager.h"
#include "TriadDrift/Combat/Pickups/TDPickupSubsystem.h"
#include "TriadDrift/Combat/Pickups/TDPickupVisuals.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDPickupTests
{
	struct FScopedPickupsCVar
	{
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Pickups"));
		int32 Old = 1;
		explicit FScopedPickupsCVar(int32 Value) { if (Var) { Old = Var->GetInt(); Var->Set(Value, ECVF_SetByCode); } }
		~FScopedPickupsCVar() { if (Var) { Var->Set(Old, ECVF_SetByCode); } }
	};

	/** Small hits until the shield is gone and the hull is under Hull01 (damage passes modifiers: no single sized hit). */
	void HurtTo(AActor* Ship, float Hull01)
	{
		UTDHealthComponent* H = UTDStatics::GetHealth(Ship);
		for (int32 I = 0; H && I < 2000 && (H->GetShield01() > 0.f || H->GetHull01() > Hull01); ++I)
		{
			UTDStatics::ApplyTDDamage(Ship, 1.f, ETDDamageKind::Energy, nullptr, nullptr, Ship->GetActorLocation(), FVector::ForwardVector);
		}
	}

	struct FFixture
	{
		FScopedPickupsCVar On{ 1 };
		FTDTestWorld W;
		ATDSquadManager* Squad = nullptr;
		UTDPickupSubsystem* Pickups = nullptr;
		ATDShip* Flown = nullptr;

		bool Init(FAutomationTestBase& T)
		{
			Squad = W.Spawn<ATDSquadManager>();
			ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
			if (!T.TestNotNull(TEXT("squad"), Squad) || !T.TestNotNull(TEXT("controller"), PC))
			{
				return false;
			}
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20000.f)));
			W.Advance(.3f);
			Pickups = UTDPickupSubsystem::Get(W.GetWorld());
			Flown = Squad->GetActiveShip();
			if (!T.TestNotNull(TEXT("pickup subsystem"), Pickups) || !T.TestNotNull(TEXT("flown ship"), Flown))
			{
				return false;
			}
			Pickups->Rng.Initialize(1234);
			return true;
		}

		ATDShip* Wingman() const
		{
			for (int32 I = 0; I < 3; ++I)
			{
				if (ATDShip* S = Squad->GetShip(static_cast<ETDShipId>(I)); S && S != Flown)
				{
					return S;
				}
			}
			return nullptr;
		}

		ATDEnemyShip* Enemy(FName Id = TDEnemyIds::ShardFighter)
		{
			return TDPowerTest::SpawnEnemyShip(W, Id, Flown->GetActorLocation() + FVector(400000.f, 0.f, 0.f)); // far: never pulled in
		}
	};
}

using namespace TDPickupTests;

// Drops: at most CapPerTypePerWave of each type per wave and MaxAlive at once; a new wave resets the caps; only a squad
// kill drops (no killer = the Dominion's breakup, an enemy killer = nothing).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPickupCaps, "TriadDrift.Combat.Pickups.DropCapsAndKillers", TD_TEST_FLAGS)
bool FTDPickupCaps::RunTest(const FString& Parameters)
{
	FFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	F.Pickups->Tuning.FighterChance = 1.f;
	F.Pickups->Tuning.MaxAlive = 100;
	ATDEnemyShip* E = F.Enemy();
	int32 Made = 0;
	for (int32 I = 0; I < 30; ++I)
	{
		Made += F.Pickups->HandleKill(E, F.Flown);
	}
	TestEqual(TEXT("30 sure kills: 2 of each type, 8 in all"), Made, 8);
	for (int32 T = 0; T < 4; ++T)
	{
		TestEqual(*FString::Printf(TEXT("type %d capped at 2"), T), F.Pickups->GetDropsThisWave(static_cast<ETDPickupType>(T)), 2);
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld()))
	{
		Bus->BroadcastWaveStarted(2, .5f);
	}
	TestEqual(TEXT("a new wave resets the caps"), F.Pickups->HandleKill(E, F.Flown), 1);
	F.Pickups->ClearAll();
	F.Pickups->Tuning.MaxAlive = 3;
	for (int32 I = 0; I < 10; ++I)
	{
		F.Pickups->HandleKill(E, F.Flown);
	}
	TestTrue(TEXT("never more than 3 alive"), F.Pickups->GetPickups().Num() <= 3);
	F.Pickups->ClearAll();
	TestEqual(TEXT("no killer (the breakup): nothing"), F.Pickups->HandleKill(E, nullptr), 0);
	TestEqual(TEXT("an enemy killer: nothing"), F.Pickups->HandleKill(E, F.Enemy()), 0);
	ATDEnemyShip* Carrier = F.Enemy(TDEnemyIds::HiveCarrier);
	if (UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld()))
	{
		Bus->BroadcastWaveStarted(3, .5f);
	}
	TestEqual(TEXT("a carrier drops two"), F.Pickups->HandleKill(Carrier, F.Flown), 2);
	return true;
}

// The safety repair: under 35 % hull the next kill drops a repair even at 0 % chance, once per scene; a new take re-arms it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPickupSafety, "TriadDrift.Combat.Pickups.SafetyRepairOncePerScene", TD_TEST_FLAGS)
bool FTDPickupSafety::RunTest(const FString& Parameters)
{
	FFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	F.Pickups->Tuning.FighterChance = 0.f;
	ATDEnemyShip* E = F.Enemy();
	TestEqual(TEXT("healthy: no safety drop"), F.Pickups->HandleKill(E, F.Flown), 0);
	UTDHealthComponent* H = UTDStatics::GetHealth(F.Flown);
	HurtTo(F.Flown, .3f);
	TestTrue(FString::Printf(TEXT("under 35 %% (%.2f)"), H->GetHull01()), H->GetHull01() < .35f && H->IsAlive());
	F.W.Tick();
	TestEqual(TEXT("hurt: the next kill drops one"), F.Pickups->HandleKill(E, F.Flown), 1);
	TestTrue(TEXT("...a repair"), F.Pickups->GetPickups().Num() == 1 && F.Pickups->GetPickups()[0]->GetType() == ETDPickupType::Hull);
	F.W.Tick();
	TestEqual(TEXT("once per scene"), F.Pickups->HandleKill(E, F.Flown), 0);
	if (UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld()))
	{
		Bus->OnTakeStarted.Broadcast(TEXT("Wave1"), 2);
	}
	F.W.Tick();
	TestEqual(TEXT("a new take re-arms it"), F.Pickups->HandleKill(E, F.Flown), 1);
	return true;
}

// Each effect applies to the right ships and expires: hull +30 % on the flown ship, shield full on it and half on the
// wingmen, the signature power ready, overcharge x1.4 for 8 s that never stacks.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPickupEffects, "TriadDrift.Combat.Pickups.EffectsApplyAndExpire", TD_TEST_FLAGS)
bool FTDPickupEffects::RunTest(const FString& Parameters)
{
	FFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	UTDHealthComponent* H = UTDStatics::GetHealth(F.Flown);
	ATDShip* Wing = F.Wingman();
	UTDHealthComponent* WH = Wing ? UTDStatics::GetHealth(Wing) : nullptr;
	if (!TestNotNull(TEXT("wingman"), WH))
	{
		return false;
	}
	HurtTo(F.Flown, .5f);
	HurtTo(Wing, 1.f); // shield only
	const float Hull0 = H->GetHull01();
	TestTrue(TEXT("hull repair"), F.Pickups->ApplyEffect(ETDPickupType::Hull, F.Flown));
	TestTrue(FString::Printf(TEXT("+30 %% hull (%.2f -> %.2f)"), Hull0, H->GetHull01()), FMath::IsNearlyEqual(H->GetHull01(), Hull0 + .3f, .01f));
	TestTrue(TEXT("shield cell"), F.Pickups->ApplyEffect(ETDPickupType::Shield, F.Flown));
	TestTrue(TEXT("full shield on the flown ship"), FMath::IsNearlyEqual(H->GetShield01(), 1.f, .01f));
	TestTrue(FString::Printf(TEXT("half on a wingman (%.2f)"), WH->GetShield01()), FMath::IsNearlyEqual(WH->GetShield01(), .5f, .02f));

	UTDPowerComponent* Sig = UTDPowerComponent::FindPower(F.Flown, ETDPowerInput::Signature1);
	if (TestNotNull(TEXT("signature power"), Sig))
	{
		Sig->bRequirePlayerControl = false;
		Sig->TryActivate();
		Sig->EndPower(false);
		F.W.Tick();
		TestTrue(TEXT("cooling"), Sig->GetCooldown01() < 1.f);
		F.Pickups->ApplyEffect(ETDPickupType::Power, F.Flown);
		TestTrue(TEXT("power cell: ready now"), Sig->GetCooldown01() >= 1.f);
	}

	UTDWeaponComponent* Weapons = F.Flown->FindComponentByClass<UTDWeaponComponent>();
	const float Base = Weapons->GetOutgoingDamageMultiplier();
	F.Pickups->ApplyEffect(ETDPickupType::Overcharge, F.Flown);
	TestTrue(TEXT("overcharge x1.4"), FMath::IsNearlyEqual(Weapons->GetOutgoingDamageMultiplier(), Base * 1.4f, .001f));
	F.W.Advance(5.f);
	F.Pickups->ApplyEffect(ETDPickupType::Overcharge, F.Flown);
	TestTrue(TEXT("a second one refreshes, never stacks"), FMath::IsNearlyEqual(Weapons->GetOutgoingDamageMultiplier(), Base * 1.4f, .001f));
	TestTrue(TEXT("timer refreshed to 8 s"), F.Pickups->GetOverchargeRemaining() > 7.5f);
	F.W.Advance(8.2f);
	TestTrue(TEXT("expired: back to the base multiplier"), FMath::IsNearlyEqual(Weapons->GetOutgoingDamageMultiplier(), Base, .001f));
	TestEqual(TEXT("no time left"), F.Pickups->GetOverchargeRemaining(), 0.f);
	return true;
}

// Collection: only the flown ship takes a pickup (pulled in, then touched); one next to a wingman stays; lifetimes run
// out; a CUT clears every pickup and the overcharge.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPickupCollect, "TriadDrift.Combat.Pickups.FlownShipOnlyAndCutClears", TD_TEST_FLAGS)
bool FTDPickupCollect::RunTest(const FString& Parameters)
{
	FFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	ATDShip* Wing = F.Wingman();
	// Wingmen fly formation near the leader: put this pickup beside the wingman, far from the flown ship.
	const FVector Away = F.Flown->GetActorLocation() + FVector(0.f, 0.f, 300000.f);
	Wing->SetActorLocation(Away);
	ATDPickup* NearWing = UTDPickupSubsystem::SpawnPickup(F.W.GetWorld(), ETDPickupType::Hull, Away + FVector(200.f, 0.f, 0.f));
	ATDPickup* NearMe = UTDPickupSubsystem::SpawnPickup(F.W.GetWorld(), ETDPickupType::Shield, F.Flown->GetActorLocation() + FVector(4000.f, 0.f, 0.f));
	TestNotNull(TEXT("spawned"), NearWing);
	F.W.Tick(.016f);
	Wing->SetActorLocation(Away); // keep it there against formation flying
	for (int32 I = 0; I < 60; ++I)
	{
		F.W.Tick(.016f);
		Wing->SetActorLocation(Away);
	}
	TestEqual(TEXT("40 m away: pulled in and taken by the flown ship"), F.Pickups->GetCollected(ETDPickupType::Shield), 1);
	TestEqual(TEXT("next to a wingman: never taken"), F.Pickups->GetCollected(ETDPickupType::Hull), 0);
	TestTrue(TEXT("...still there"), IsValid(NearWing) && F.Pickups->GetPickups().Contains(NearWing));
	F.W.Advance(F.Pickups->Tuning.LifetimeSeconds + .5f);
	TestEqual(TEXT("lifetime ran out"), F.Pickups->GetPickups().Num(), 0);

	UTDPickupSubsystem::SpawnPickup(F.W.GetWorld(), ETDPickupType::Power, F.Flown->GetActorLocation() + FVector(300000.f, 0.f, 0.f));
	F.Pickups->ApplyEffect(ETDPickupType::Overcharge, F.Flown);
	if (UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld()))
	{
		Bus->OnSceneCut.Broadcast(TEXT("Wave1"), 2, 1.f);
	}
	TestEqual(TEXT("CUT: no pickups"), F.Pickups->GetPickups().Num(), 0);
	TestEqual(TEXT("CUT: no overcharge"), F.Pickups->GetOverchargeRemaining(), 0.f);
	TestTrue(TEXT("CUT: multiplier back"), FMath::IsNearlyEqual(F.Flown->FindComponentByClass<UTDWeaponComponent>()->GetOutgoingDamageMultiplier(), 1.f, .3f));
	return true;
}

// The effects builder's look slot (DA_Pickup_Visuals): an asset with empty cues keeps the primitive shapes and collects
// cleanly (null-safe); no asset at all does the same.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPickupLookSlot, "TriadDrift.Combat.Pickups.LookSlotNullSafe", TD_TEST_FLAGS)
bool FTDPickupLookSlot::RunTest(const FString& Parameters)
{
	FFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	const TStrongObjectPtr<UTDPickupVisuals> Empty(NewObject<UTDPickupVisuals>(GetTransientPackage()));
	UTDPickupVisuals::SetOverrideForTests(Empty.Get());
	ON_SCOPE_EXIT { UTDPickupVisuals::SetOverrideForTests(nullptr); };
	for (int32 T = 0; T < 4; ++T)
	{
		ATDPickup* P = UTDPickupSubsystem::SpawnPickup(F.W.GetWorld(), static_cast<ETDPickupType>(T), F.Flown->GetActorLocation() + FVector(300000.f, 0.f, 0.f));
		TestTrue(*FString::Printf(TEXT("type %d: spawned, primitive look"), T), P && !P->HasArtLook());
		TestTrue(*FString::Printf(TEXT("type %d: collected with an empty look"), T), F.Pickups->Collect(P, F.Flown));
	}
	return true;
}

// td.Pickups 0: nothing spawns, kills drop nothing, and pickups already out are cleared on the next tick.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPickupSwitch, "TriadDrift.Combat.Pickups.SwitchOffIsTodaysGame", TD_TEST_FLAGS)
bool FTDPickupSwitch::RunTest(const FString& Parameters)
{
	FFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	UTDPickupSubsystem::SpawnPickup(F.W.GetWorld(), ETDPickupType::Hull, F.Flown->GetActorLocation() + FVector(300000.f, 0.f, 0.f));
	F.Pickups->ApplyEffect(ETDPickupType::Overcharge, F.Flown);
	{
		FScopedPickupsCVar Off(0);
		F.W.Tick();
		TestEqual(TEXT("off: pickups cleared"), F.Pickups->GetPickups().Num(), 0);
		TestEqual(TEXT("off: overcharge ended"), F.Pickups->GetOverchargeRemaining(), 0.f);
		TestNull(TEXT("off: nothing spawns"), UTDPickupSubsystem::SpawnPickup(F.W.GetWorld(), ETDPickupType::Hull, FVector::ZeroVector));
		F.Pickups->Tuning.FighterChance = 1.f;
		TestEqual(TEXT("off: kills drop nothing"), F.Pickups->HandleKill(F.Enemy(), F.Flown), 0);
	}
	F.W.Tick();
	TestNotNull(TEXT("back on: spawns again"), UTDPickupSubsystem::SpawnPickup(F.W.GetWorld(), ETDPickupType::Hull, F.Flown->GetActorLocation() + FVector(300000.f, 0.f, 0.f)));
	return true;
}

#endif

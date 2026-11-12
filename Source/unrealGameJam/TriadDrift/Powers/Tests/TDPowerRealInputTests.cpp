// Second Powers review, minor 7: every power pressed through the real input path on the real squad ship
// (bRequirePlayerControl stays true), once under slow motion, and switching away mid-cloak / mid-dome.
// Teammate 2. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers.RealInput

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDPowerRealInputTest
{
	/** A real squad flown by a local player, all three abilities unlocked. */
	struct FSquad
	{
		FTDTestWorld W;
		APlayerController* PC = nullptr;
		ATDSquadManager* Squad = nullptr;
		float Step = 1.f / 60.f;

		explicit FSquad(float InStep) : Step(InStep)
		{
			PC = W.SpawnLocalPlayerController();
			Squad = W.Spawn<ATDSquadManager>();
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform::Identity);
			if (UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld()))
			{
				Bus->BroadcastAbilityUnlocked(ETDAbilityId::Cloak, ETDShipId::Rift);
				Bus->BroadcastAbilityUnlocked(ETDAbilityId::EMPPulse, ETDShipId::Lancer);
				Bus->BroadcastAbilityUnlocked(ETDAbilityId::GravityWell, ETDShipId::Bastion);
			}
			W.Advance(1.1f, Step); // past the switch cooldown
		}

		ATDShip* Fly(ETDShipId Id)
		{
			if (Squad->GetActiveShipId() != Id)
			{
				W.Advance(1.1f, Step);
				Squad->RequestSwitchTo(Id);
				W.Tick(Step);
			}
			return Squad->GetShip(Id);
		}

		/** Press and release through UTDPowerComponent::RoutePowerInput, as the player controller does. */
		bool Press(ATDShip* Ship, ETDPowerInput Slot, float HoldSeconds = 0.f)
		{
			const bool bRouted = UTDPowerComponent::RoutePowerInput(Ship, Slot, true);
			W.Tick(Step);
			if (HoldSeconds > 0.f)
			{
				W.Advance(HoldSeconds, Step);
			}
			UTDPowerComponent::RoutePowerInput(Ship, Slot, false);
			W.Tick(Step);
			return bRouted;
		}

		template <typename T>
		T* Power(ATDShip* Ship, ETDPowerInput Slot) const { return Cast<T>(UTDPowerComponent::FindPower(Ship, Slot)); }
	};

	int32 LiveMissilesOf(UWorld* World, const AActor* Shooter)
	{
		int32 N = 0;
		if (UTDProjectileSubsystem* S = UTDProjectileSubsystem::Get(World))
		{
			for (const ATDProjectile* P : S->GetLiveProjectiles())
			{
				N += (P && P->IsActive() && P->IsMissile() && P->GetShooter() == Shooter) ? 1 : 0;
			}
		}
		return N;
	}

	/** Presses every power on its real ship; Dilation < 1 runs it all in slow motion. */
	void PressEveryPower(FAutomationTestBase& Test, float Step, float Dilation)
	{
		const FString Tag = FString::Printf(TEXT(" @ %.0f fps, dilation %.2f"), 1.f / Step, Dilation);
		FSquad S(Step);
		UGameplayStatics::SetGlobalTimeDilation(S.W.GetWorld(), Dilation);

		ATDShip* Rift = S.Fly(ETDShipId::Rift);
		Test.TestTrue(TEXT("Rift: Q routed") + Tag, S.Press(Rift, ETDPowerInput::Signature1));
		UTDPortalBlasterPower* Portal = S.Power<UTDPortalBlasterPower>(Rift, ETDPowerInput::Signature1);
		Test.TestTrue(TEXT("Rift: portal A open") + Tag, Portal && Portal->GetPortals().Num() == 1);
		S.Press(Rift, ETDPowerInput::Ability);
		UTDCloakPower* Cloak = S.Power<UTDCloakPower>(Rift, ETDPowerInput::Ability);
		Test.TestTrue(TEXT("Rift: cloaked") + Tag, Cloak && Cloak->IsCloaked());

		ATDShip* Lancer = S.Fly(ETDShipId::Lancer);
		S.Press(Lancer, ETDPowerInput::Signature1);
		UTDRamPower* Ram = S.Power<UTDRamPower>(Lancer, ETDPowerInput::Signature1);
		bool bDashed = false;
		for (float T = 0.f; T < 1.f / Dilation && !bDashed; T += Step)
		{
			S.W.Tick(Step);
			bDashed = Ram && Ram->IsDashing();
		}
		Test.TestTrue(TEXT("Lancer: ram dashes") + Tag, bDashed);
		S.W.Advance(2.f / Dilation, Step);
		S.Press(Lancer, ETDPowerInput::Ability);
		UTDEMPPower* EMP = S.Power<UTDEMPPower>(Lancer, ETDPowerInput::Ability);
		Test.TestTrue(TEXT("Lancer: EMP fired (on cooldown)") + Tag, EMP && EMP->GetCooldown01() < 1.f);

		ATDShip* Bastion = S.Fly(ETDShipId::Bastion);
		S.Press(Bastion, ETDPowerInput::Signature1);
		Test.TestEqual(TEXT("Bastion: twelve swarm missiles") + Tag, LiveMissilesOf(S.W.GetWorld(), Bastion), 12);
		S.Press(Bastion, ETDPowerInput::Signature2);
		UTDShieldDomePower* Dome = S.Power<UTDShieldDomePower>(Bastion, ETDPowerInput::Signature2);
		Test.TestTrue(TEXT("Bastion: dome up") + Tag, Dome && Dome->GetDome() != nullptr);
		S.Press(Bastion, ETDPowerInput::Ability);
		UTDGravityWellPower* Well = S.Power<UTDGravityWellPower>(Bastion, ETDPowerInput::Ability);
		Test.TestTrue(TEXT("Bastion: well launched") + Tag, Well && Well->GetLastWell() != nullptr);

		// Slow motion stretches real time, not game time: the 6 s dome is still up at 5 s of game time.
		const TWeakObjectPtr<ATDShieldDome> DomeActor = Dome ? Dome->GetDome() : nullptr;
		S.W.Advance(5.f / Dilation, Step);
		Test.TestTrue(TEXT("dome still up after 5 s of game time") + Tag, DomeActor.IsValid() && Dome->IsPowerActive());
		S.W.Advance(1.5f / Dilation, Step);
		Test.TestFalse(TEXT("dome gone after 6.5 s of game time") + Tag, Dome && Dome->IsPowerActive());
		UGameplayStatics::SetGlobalTimeDilation(S.W.GetWorld(), 1.f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerRealInputAll, "TriadDrift.Powers.RealInput.EveryPowerOnItsRealShip", TD_TEST_FLAGS)
bool FTDPowerRealInputAll::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		TDPowerRealInputTest::PressEveryPower(*this, Step, 1.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerRealInputSlowMo, "TriadDrift.Powers.RealInput.EveryPowerInSlowMotion", TD_TEST_FLAGS)
bool FTDPowerRealInputSlowMo::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		TDPowerRealInputTest::PressEveryPower(*this, Step, 0.25f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerRealInputSwitchMid, "TriadDrift.Powers.RealInput.SwitchAwayMidCloakAndMidDome", TD_TEST_FLAGS)
bool FTDPowerRealInputSwitchMid::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		TDPowerRealInputTest::FSquad S(Step);

		// Mid-cloak: Rift becomes a wingman. The cloak runs out on its own and Rift is targetable again; the new
		// ship's presses never reach Rift's powers.
		ATDShip* Rift = S.Fly(ETDShipId::Rift);
		S.Press(Rift, ETDPowerInput::Ability);
		UTDCloakPower* Cloak = S.Power<UTDCloakPower>(Rift, ETDPowerInput::Ability);
		TestTrue(TEXT("cloaked before the switch") + Fps, Cloak && Cloak->IsCloaked());
		ATDShip* Lancer = S.Fly(ETDShipId::Lancer);
		S.Press(Lancer, ETDPowerInput::Ability);
		TestTrue(TEXT("Lancer's ability press reached Lancer (EMP on cooldown)") + Fps,
			S.Power<UTDPowerComponent>(Lancer, ETDPowerInput::Ability)->GetCooldown01() < 1.f);
		S.W.Advance(12.f, Step);
		TestFalse(TEXT("cloak ended on its own after the switch") + Fps, Cloak && Cloak->IsCloaked());
		TestTrue(TEXT("Rift targetable again") + Fps, UTDStatics::GetHealth(Rift) && UTDStatics::GetHealth(Rift)->bTargetable);

		// Mid-dome: Bastion's dome keeps protecting after the player leaves Bastion, then collapses on time.
		ATDShip* Bastion = S.Fly(ETDShipId::Bastion);
		S.Press(Bastion, ETDPowerInput::Signature2);
		UTDShieldDomePower* Dome = S.Power<UTDShieldDomePower>(Bastion, ETDPowerInput::Signature2);
		const TWeakObjectPtr<ATDShieldDome> DomeActor = Dome ? Dome->GetDome() : nullptr;
		TestTrue(TEXT("dome up") + Fps, DomeActor.IsValid());
		S.Fly(ETDShipId::Rift);
		TestTrue(TEXT("dome still up after switching to Rift") + Fps, DomeActor.IsValid() && Dome->IsPowerActive());
		S.W.Advance(7.f, Step);
		TestFalse(TEXT("dome collapsed on time") + Fps, Dome && Dome->IsPowerActive());
	}
	return true;
}

/**
 * Switching away mid-ram or with one portal placed (Lead's question): the ram's wind-up is cancelled and a dash
 * ends at once (never AI-flown at dash speed, never left invulnerable); a lone portal closes; a full pair stays.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerRealInputSwitchMidRamPortal, "TriadDrift.Powers.RealInput.SwitchAwayMidRamAndHalfPortal", TD_TEST_FLAGS)
bool FTDPowerRealInputSwitchMidRamPortal::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		for (const bool bDuringDash : { false, true })
		{
			const FString Tag = FString::Printf(TEXT(" @ %.0f fps, %s"), 1.f / Step, bDuringDash ? TEXT("mid-dash") : TEXT("mid-wind-up"));
			TDPowerRealInputTest::FSquad S(Step);
			ATDShip* Lancer = S.Fly(ETDShipId::Lancer);
			S.W.Advance(1.1f, Step); // switch cooldown passes before the press
			UTDRamPower* Ram = S.Power<UTDRamPower>(Lancer, ETDPowerInput::Signature1);
			UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
			S.W.Tick(Step);
			UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
			if (bDuringDash)
			{
				for (float T = 0.f; T < 1.f && !(Ram && Ram->IsDashing()); T += Step)
				{
					S.W.Tick(Step);
				}
				TestTrue(TEXT("dashing before the switch") + Tag, Ram && Ram->IsDashing());
			}
			else
			{
				TestTrue(TEXT("winding up before the switch") + Tag, Ram && Ram->IsPowerActive() && !Ram->IsDashing());
			}
			TestTrue(TEXT("switch allowed") + Tag, S.Squad->RequestSwitchTo(ETDShipId::Rift));
			S.W.Tick(Step);
			S.W.Tick(Step);
			TestFalse(TEXT("ram over once Lancer is AI-flown") + Tag, Ram && Ram->IsPowerActive());
			TestFalse(TEXT("no Ram speed override left") + Tag, Lancer->GetShipMovement()->HasSpeedOverrideFor(TEXT("Ram")));
			bool bEverDashedUnderAI = false;
			for (float T = 0.f; T < 2.f; T += Step)
			{
				S.W.Tick(Step);
				bEverDashedUnderAI |= Ram && Ram->IsDashing();
			}
			TestFalse(TEXT("never dashes under AI") + Tag, bEverDashedUnderAI);
			const UTDHealthComponent* Health = UTDStatics::GetHealth(Lancer);
			TestFalse(TEXT("not left invulnerable") + Tag, Health && Health->IsInvulnerable());
			TestTrue(TEXT("back to normal speed") + Tag, Lancer->GetVelocity().Size() <= Lancer->GetShipMovement()->GetFlightTuning().BoostSpeed * 1.05f);
		}

		// One portal placed, then a switch: it closes. A full pair stays open (the confirmed rule).
		for (const int32 Placed : { 1, 2 })
		{
			const FString Tag = FString::Printf(TEXT(" @ %.0f fps, %d portal(s)"), 1.f / Step, Placed);
			TDPowerRealInputTest::FSquad S(Step);
			ATDShip* Rift = S.Fly(ETDShipId::Rift);
			S.W.Advance(1.1f, Step);
			UTDPortalBlasterPower* Portal = S.Power<UTDPortalBlasterPower>(Rift, ETDPowerInput::Signature1);
			S.Press(Rift, ETDPowerInput::Signature1);
			if (Placed == 2)
			{
				Rift->SetActorRotation(Rift->GetActorRotation() + FRotator(0.f, 90.f, 0.f));
				S.Press(Rift, ETDPowerInput::Signature1);
			}
			TestEqual(TEXT("portals placed") + Tag, Portal ? Portal->GetPortals().Num() : -1, Placed);
			TestTrue(TEXT("switch allowed") + Tag, S.Squad->RequestSwitchTo(ETDShipId::Lancer));
			S.W.Advance(0.2f, Step);
			if (Placed == 1)
			{
				TestFalse(TEXT("lone portal closed: no orphan") + Tag, Portal && Portal->IsPowerActive());
				TestEqual(TEXT("no portal left") + Tag, Portal ? Portal->GetPortals().Num() : -1, 0);
			}
			else
			{
				TestTrue(TEXT("a full pair keeps running after the switch") + Tag, Portal && Portal->IsPowerActive() && Portal->GetPortals().Num() == 2);
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

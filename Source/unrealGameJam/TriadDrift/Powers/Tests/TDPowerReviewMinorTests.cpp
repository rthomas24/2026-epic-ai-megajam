// Second independent review, minors in the powers lane (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers.Review2

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/MovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDPowerReview2
{
	/** The squad flown through the real routing (as TDPowerRealInputTests). */
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
			W.Advance(1.1f, Step);
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
		void Press(ATDShip* Ship, ETDPowerInput Slot)
		{
			UTDPowerComponent::RoutePowerInput(Ship, Slot, true);
			W.Tick(Step);
			UTDPowerComponent::RoutePowerInput(Ship, Slot, false);
			W.Tick(Step);
		}
	};
}

// ENG-10: a switch mid-dash leaves the AI-flown Lancer with no post-dash grace (was up to 1.5 s of invulnerability).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReview2RamSwitch, "TriadDrift.Powers.Review2.RamSwitchMidDashLeavesNoGrace", TD_TEST_FLAGS)
bool FTDReview2RamSwitch::RunTest(const FString& Parameters)
{
	for (const float Step : TDPowerTest::kPowerFrameSteps)
	{
		const FString Tag = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		TDPowerReview2::FSquad S(Step);
		ATDShip* Lancer = S.Fly(ETDShipId::Lancer);
		UTDRamPower* Ram = Cast<UTDRamPower>(UTDPowerComponent::FindPower(Lancer, ETDPowerInput::Signature1));
		if (!TestNotNull(TEXT("ram") + Tag, Ram))
		{
			continue;
		}
		S.W.Advance(1.1f, Step); // past the switch cooldown, so the switch below is accepted
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
		for (int32 I = 0; I < 120 && !Ram->IsDashing(); ++I)
		{
			S.W.Tick(Step);
		}
		TestTrue(TEXT("dashing") + Tag, Ram->IsDashing());
		S.Squad->RequestSwitchTo(ETDShipId::Rift);
		S.W.Tick(Step);
		TestEqual(TEXT("switched to Rift") + Tag, S.Squad->GetActiveShipId(), ETDShipId::Rift);
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
		TestFalse(TEXT("ram over on the next tick") + Tag, Ram->IsPowerActive());
		TestEqual(TEXT("no post-dash grace on the next tick") + Tag, Ram->GetPostDashGrace(), 0.f);
		S.W.Advance(0.25f, Step); // at most the last in-dash renewal (0.2 s) remains; see the Core ask in the report
		const UTDHealthComponent* Health = UTDStatics::GetHealth(Lancer);
		TestFalse(TEXT("not invulnerable 0.25 s after the switch") + Tag, Health && Health->IsInvulnerable());
	}
	return true;
}

// ENG-09: a dead Rift places no portal B (the press path bypassed CanActivate's owner rules).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReview2DeadRiftPortal, "TriadDrift.Powers.Review2.DeadRiftPlacesNoPortalB", TD_TEST_FLAGS)
bool FTDReview2DeadRiftPortal::RunTest(const FString& Parameters)
{
	using namespace TDPowerTest;
	FTDTestWorld W;
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f), 100.f, 0.f);
	UTDPortalBlasterPower* Power = AddPower<UTDPortalBlasterPower>(Rift);
	Power->HandleInputPressed();
	TestEqual(TEXT("portal A"), Power->GetPortals().Num(), 1);
	UTDStatics::ApplyTDDamage(Rift, 10000.f, ETDDamageKind::Energy, nullptr, nullptr, FVector::ZeroVector, FVector::ZeroVector, true);
	TestTrue(TEXT("Rift is dead"), UTDStatics::GetHealth(Rift)->IsDead());
	Rift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
	Power->HandleInputPressed();
	TestEqual(TEXT("no portal B from a dead Rift"), Power->GetPortals().Num(), 1);
	return true;
}

// ENG-09: pairing clears the crossing history (stale positions could teleport a ship that never touched the disc).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReview2PartnerClears, "TriadDrift.Powers.Review2.PairingClearsCrossingHistory", TD_TEST_FLAGS)
bool FTDReview2PartnerClears::RunTest(const FString& Parameters)
{
	TDPowerReview2::FSquad S(1.f / 60.f);
	ATDShip* Rift = S.Fly(ETDShipId::Rift);
	UTDPortalBlasterPower* Power = Cast<UTDPortalBlasterPower>(UTDPowerComponent::FindPower(Rift, ETDPowerInput::Signature1));
	if (!TestNotNull(TEXT("portal blaster"), Power))
	{
		return false;
	}
	S.Press(Rift, ETDPowerInput::Signature1);
	Rift->SetActorRotation(Rift->GetActorRotation() + FRotator(0.f, 90.f, 0.f));
	S.Press(Rift, ETDPowerInput::Signature1);
	if (!TestEqual(TEXT("a pair"), Power->GetPortals().Num(), 2))
	{
		return false;
	}
	ATDPortal* A = Power->GetPortals()[0].Get();
	ATDPortal* B = Power->GetPortals()[1].Get();
	S.W.Advance(0.2f, S.Step);
	TestTrue(TEXT("a paired portal tracks the squad"), A && A->GetTrackedCount() > 0);
	if (A && B)
	{
		A->SetPartner(nullptr);
		TestEqual(TEXT("unpairing clears the history"), A->GetTrackedCount(), 0);
		S.W.Advance(0.2f, S.Step);
		A->SetPartner(B);
		TestEqual(TEXT("re-pairing starts from a clean history"), A->GetTrackedCount(), 0);
	}
	return true;
}

// ENG-10: ram reach is the distance to the hull's box: passing a long hull's short side is not a hit.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReview2RamReachBox, "TriadDrift.Powers.Review2.RamReachUsesTheHullBox", TD_TEST_FLAGS)
bool FTDReview2RamReachBox::RunTest(const FString& Parameters)
{
	using namespace TDPowerTest;
	for (const float Step : kPowerFrameSteps)
	{
		FScopedPowerFrameStep StepScope(Step);
		const FString Tag = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		AActor* Lancer = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f), 100.f, 0.f);
		UTDRamPower* Ram = AddPower<UTDRamPower>(Lancer);
		// A long hull (40 m along X, 4 m wide), centred 9 m to the side of the dash line: its largest extent (20 m) as a
		// sphere reached the line; its box is 7 m away.
		AActor* LongHull = SpawnBody(W, ETDTeam::Enemy, FVector(6000.f, 900.f, 0.f), FVector(2000.f, 200.f, 200.f), 1000.f, 0.f);
		// A hull right on the line is still hit.
		AActor* OnLine = SpawnBody(W, ETDTeam::Enemy, FVector(14000.f, 0.f, 0.f), FVector(100.f), 1000.f, 0.f);
		Ram->TryActivate();
		W.Advance(3.f, Step);
		TestEqual(TEXT("the long hull beside the line is untouched") + Tag, Hull(LongHull), 1000.f);
		TestTrue(TEXT("the hull on the line is hit") + Tag, Hull(OnLine) < 1000.f);
	}
	return true;
}

// ENG-06: locks age outside the 25 degree lock cone itself (the old 5 degree margin made it 30).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReview2LockCone, "TriadDrift.Powers.Review2.LocksAgeOutsideTheTwentyFiveDegreeCone", TD_TEST_FLAGS)
bool FTDReview2LockCone::RunTest(const FString& Parameters)
{
	using namespace TDPowerTest;
	for (const float OffDeg : { 22.f, 27.5f })
	{
		FTDTestWorld W;
		AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDTargetingComponent* Targeting = NewObject<UTDTargetingComponent>(Bastion, TEXT("Targeting"));
		Targeting->RegisterComponent();
		AddPower<UTDMissileSwarmPower>(Bastion);
		ATDEnemyShip* Target = SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(9000.f, 0.f, 0.f));
		if (UMovementComponent* Move = Target ? Target->FindComponentByClass<UMovementComponent>() : nullptr)
		{
			Move->StopMovementImmediately();
			Move->Deactivate();
		}
		W.Advance(1.f, PowerFrameStep());
		TestTrue(FString::Printf(TEXT("locked (%.1f deg)"), OffDeg), Targeting->GetLockedTargets().Contains(Target));
		Bastion->SetActorRotation(FRotator(0.f, OffDeg, 0.f));
		W.Advance(2.f, PowerFrameStep());
		const bool bKept = Targeting->GetLockedTargets().Contains(Target);
		if (OffDeg < Targeting->LockConeDeg)
		{
			TestTrue(FString::Printf(TEXT("inside the cone (%.1f deg): kept"), OffDeg), bKept);
		}
		else
		{
			TestFalse(FString::Printf(TEXT("outside the cone (%.1f deg): aged out"), OffDeg), bKept);
		}
	}
	return true;
}

// A power press while the player's ship tumbles: it works and aims where the ship points at that moment.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReview2TumblingPress, "TriadDrift.Powers.Review2.PowerPressWhileTumbling", TD_TEST_FLAGS)
bool FTDReview2TumblingPress::RunTest(const FString& Parameters)
{
	TDPowerReview2::FSquad S(1.f / 60.f);
	auto Tumble = [&S](ATDShip* Ship, float Seconds)
	{
		for (float T = 0.f; T < Seconds; T += S.Step)
		{
			Ship->AddActorLocalRotation(FRotator(540.f * S.Step, 720.f * S.Step, 900.f * S.Step)); // a hard spin on every axis
			S.W.Tick(S.Step);
		}
	};
	// Rift: a portal placed mid-tumble opens where the ship points at the press.
	ATDShip* Rift = S.Fly(ETDShipId::Rift);
	Tumble(Rift, 0.5f);
	UTDPortalBlasterPower* Portal = Cast<UTDPortalBlasterPower>(UTDPowerComponent::FindPower(Rift, ETDPowerInput::Signature1));
	if (TestNotNull(TEXT("portal blaster"), Portal))
	{
		S.Press(Rift, ETDPowerInput::Signature1);
		TestEqual(TEXT("tumbling Rift: portal A placed"), Portal->GetPortals().Num(), 1);
		if (const ATDPortal* A = Portal->GetPortals().Num() ? Portal->GetPortals()[0].Get() : nullptr)
		{
			TestFalse(TEXT("portal transform is finite"), A->GetActorLocation().ContainsNaN() || A->GetActorRotation().ContainsNaN());
		}
	}
	// Lancer: a ram pressed mid-tumble dashes, and the dash ends by itself.
	ATDShip* Lancer = S.Fly(ETDShipId::Lancer);
	Tumble(Lancer, 0.5f);
	UTDRamPower* Ram = Cast<UTDRamPower>(UTDPowerComponent::FindPower(Lancer, ETDPowerInput::Signature1));
	if (TestNotNull(TEXT("ram"), Ram))
	{
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
		bool bDashed = false;
		for (int32 I = 0; I < 120; ++I)
		{
			Lancer->AddActorLocalRotation(FRotator(540.f * S.Step, 720.f * S.Step, 900.f * S.Step));
			S.W.Tick(S.Step);
			bDashed |= Ram->IsDashing();
		}
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
		TestTrue(TEXT("tumbling Lancer: the ram dashes"), bDashed);
		S.W.Advance(3.f, S.Step);
		TestFalse(TEXT("the ram ends by itself"), Ram->IsPowerActive());
		TestFalse(TEXT("ship transform is finite"), Lancer->GetActorLocation().ContainsNaN());
	}
	return true;
}

// The dome after a switch: Bastion goes AI, the dome stays up, and a wingman inside it takes no damage from fire outside.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReview2DomeSwitchWingman, "TriadDrift.Powers.Review2.DomeKeepsShieldingAWingmanAfterASwitch", TD_TEST_FLAGS)
bool FTDReview2DomeSwitchWingman::RunTest(const FString& Parameters)
{
	TDPowerReview2::FSquad S(1.f / 60.f);
	ATDShip* Bastion = S.Fly(ETDShipId::Bastion);
	UTDShieldDomePower* DomePower = Cast<UTDShieldDomePower>(UTDPowerComponent::FindPower(Bastion, ETDPowerInput::Signature2));
	if (!TestNotNull(TEXT("dome power"), DomePower))
	{
		return false;
	}
	S.Press(Bastion, ETDPowerInput::Signature2);
	ATDShieldDome* Dome = DomePower->GetDome();
	if (!TestNotNull(TEXT("dome up"), Dome))
	{
		return false;
	}
	// Switch away: Bastion is now a wingman, the dome keeps going.
	S.Fly(ETDShipId::Lancer);
	TestTrue(TEXT("dome still up after the switch"), Dome->IsDomeActive());
	// Rift, a wingman, sits inside the dome; an enemy bolt comes at it from outside.
	ATDShip* Rift = S.Squad->GetShip(ETDShipId::Rift);
	const FVector Centre = Dome->GetActorLocation();
	Rift->SetActorLocation(Centre + FVector(Dome->GetRadius() * 0.3f, 0.f, 0.f));
	const UTDHealthComponent* Health = UTDStatics::GetHealth(Rift);
	const float Before = Health->GetHull() + Health->GetShield();
	FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile;
	Spec.FX = nullptr;
	const FVector From = Centre + FVector(Dome->GetRadius() * 2.f, 0.f, 0.f);
	const FVector Dir = (Rift->GetActorLocation() - From).GetSafeNormal();
	ATDProjectile::Launch(S.W.GetWorld(), Spec, FTransform(Dir.Rotation(), From), nullptr, ETDTeam::Enemy, nullptr, FVector::ZeroVector, 1.f, false);
	for (int32 I = 0; I < 120; ++I)
	{
		Rift->SetActorLocation(Centre + FVector(Dome->GetRadius() * 0.3f, 0.f, 0.f)); // hold it inside
		S.W.Tick(S.Step);
	}
	TestTrue(TEXT("dome still up"), Dome->IsDomeActive());
	TestEqual(TEXT("the wingman inside took no damage"), Health->GetHull() + Health->GetShield(), Before);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

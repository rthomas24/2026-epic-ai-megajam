// Second Powers review, MAJOR (ENG-10): a REAL ram dash into carrier bays, turrets and shield generators.
// Teammate 2. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers.Ram.Parts

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/TDShieldGenerator.h"

namespace TDRamPartTest
{
	enum class EPart : uint8 { Bay, Turret, Generator };

	/**
	 * A real squad with a local player flying Lancer, 2500 cm in front of a real part, nose on it; the ram is pressed
	 * through the power input path (bRequirePlayerControl stays true). Returns the part, and whether the ram hit it.
	 */
	bool RamInto(FAutomationTestBase& Test, EPart Which, float Step, const FString& Label)
	{
		FTDTestWorld W;
		APlayerController* PC = W.SpawnLocalPlayerController();
		ATDSquadManager* Squad = W.Spawn<ATDSquadManager>(FVector(0.f, 0.f, -60000.f));
		Squad->SetSquadPlayerController(PC);
		Squad->SpawnSquad(FTransform(FVector(0.f, 0.f, -60000.f)));
		W.Advance(1.1f, Step); // past the switch cooldown
		Squad->RequestSwitchTo(ETDShipId::Lancer);
		W.Tick(Step);
		ATDShip* Lancer = Squad->GetShip(ETDShipId::Lancer);
		if (!Test.TestTrue(Label + TEXT(": flying Lancer"), Lancer && PC->GetPawn() == Lancer))
		{
			return false;
		}

		AActor* Part = nullptr;
		FVector Out = FVector::ForwardVector;
		if (Which == EPart::Generator)
		{
			ATDShieldGenerator* Gen = W.Spawn<ATDShieldGenerator>(FVector(20000.f, 0.f, 0.f));
			Gen->Init(5000.f, 0);
			Part = Gen;
			Out = -FVector::ForwardVector;
		}
		else
		{
			ATDEnemyShip* Carrier = W.Spawn<ATDEnemyShip>(FVector(20000.f, 0.f, 0.f));
			if (AController* C = Carrier->GetController())
			{
				C->UnPossess();
				C->Destroy();
			}
			Carrier->InitFromSpec(TDEnemyIds::HiveCarrier);
			Carrier->MaxLaunchedAlive = 0;
			// Socket-mounted parts (an art hull with Bay_N / Turret_N) face their socket's +X: ram along it. Sphere-mounted
			// parts face radially out of the carrier (T4, agreed with T2).
			auto OutOf = [Carrier](const AActor* P)
			{
				return Carrier->HasSocketMountedParts() ? P->GetActorForwardVector() : (P->GetActorLocation() - Carrier->GetActorLocation()).GetSafeNormal();
			};
			TArray<AActor*> Candidates, Others;
			for (ATDCarrierBay* B : Carrier->GetBays()) { (Which == EPart::Bay ? Candidates : Others).Add(B); }
			for (ATDEnemyTurret* T : Carrier->GetTurrets()) { (Which == EPart::Turret ? Candidates : Others).Add(T); }
			if (Which == EPart::Bay && Candidates.Num() > 1)
			{
				Candidates.Swap(0, 1); // bay 1 first, as before
			}
			// A clear run: since the ship builder's hull-surface sockets (2d333749), Bay_0 stands on Turret_0's outward line,
			// and the dash rightly stops on the bay first. Ram a part nothing else covers.
			for (AActor* C : Candidates)
			{
				if (!IsValid(C) || FMath::Abs(OutOf(C).Z) > .7f) { continue; } // a level run (a near-vertical dive is not this test)
				const FVector From = C->GetActorLocation() + OutOf(C) * 2500.f;
				bool bClear = true;
				for (const AActor* O : Others)
				{
					FVector OC, OE;
					O->GetActorBounds(true, OC, OE);
					bClear &= !IsValid(O) || FMath::PointDistToSegment(OC, From, C->GetActorLocation()) > OE.Size() + 800.f;
				}
				for (const AActor* O : Candidates)
				{
					if (O == C || !IsValid(O)) { continue; }
					FVector OC, OE;
					O->GetActorBounds(true, OC, OE);
					bClear &= FMath::PointDistToSegment(OC, From, C->GetActorLocation()) > OE.Size() + 800.f;
				}
				if (bClear) { Part = C; break; }
			}
			if (!Part && Candidates.Num() > 0)
			{
				Part = Candidates[0];
				Test.AddWarning(Label + TEXT(": no part with a clear approach; ramming the first"));
			}
			if (Part)
			{
				Out = OutOf(Part);
				Test.AddInfo(Label + TEXT(": ramming ") + Part->GetName() + TEXT(" along ") + (-Out).ToCompactString());
			}
		}
		if (!Test.TestNotNull(Label + TEXT(": part spawned"), Part))
		{
			return false;
		}
		W.Tick(Step);

		const FVector Start = Part->GetActorLocation() + Out * 2500.f;
		Lancer->SetActorLocationAndRotation(Start, (-Out).Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
		Lancer->GetShipMovement()->Velocity = -Out * 3000.f;
		UTDRamPower* Ram = Cast<UTDRamPower>(UTDPowerComponent::FindPower(Lancer, ETDPowerInput::Signature1));
		if (!Test.TestNotNull(Label + TEXT(": Lancer's ram"), Ram))
		{
			return false;
		}
		const UTDHealthComponent* PartHealth = UTDStatics::GetHealth(Part);
		const float HullBefore = PartHealth ? PartHealth->GetHull() : 0.f;
		Test.TestTrue(Label + TEXT(": ram pressed through the input path"), UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true));
		W.Tick(Step);
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
		bool bHit = false;
		float MinDist = TNumericLimits<float>::Max();
		int32 DashFrames = 0;
		FString Track;
		for (float T = 0.f; T < 3.f && !bHit; T += Step)
		{
			W.Tick(Step);
			const float D = FVector::Dist(Lancer->GetActorLocation(), Part->GetActorLocation());
			MinDist = FMath::Min(MinDist, D);
			DashFrames += Ram->IsDashing() ? 1 : 0;
			if (T < .5f) { Track += FString::Printf(TEXT("%.0f%s "), D, Ram->IsDashing() ? TEXT("d") : TEXT("")); }
			bHit = Ram->GetHitThisDash().ContainsByPredicate([Part](const TWeakObjectPtr<AActor>& A) { return A.Get() == Part; });
		}
		Test.AddInfo(Label + FString::Printf(TEXT(": closest %.0f cm, %d dash frames; first 0.5 s: %s"), MinDist, DashFrames, *Track));
		{
			FString Hits;
			for (const TWeakObjectPtr<AActor>& A : Ram->GetHitThisDash())
			{
				Hits += A.IsValid() ? A->GetName() + TEXT(" ") : TEXT("? ");
			}
			Test.AddInfo(Label + FString::Printf(TEXT(": hit this dash [%s], Lancer ended %.0f cm from the part"), *Hits,
				FVector::Dist(Lancer->GetActorLocation(), Part->GetActorLocation())));
		}
		Test.TestTrue(Label + TEXT(": the ram hit the part"), bHit);
		Test.TestTrue(Label + FString::Printf(TEXT(": the part's hull dropped (%.0f -> %.0f)"), HullBefore, PartHealth ? PartHealth->GetHull() : 0.f),
			PartHealth && PartHealth->GetHull() < HullBefore);
		if (const ATDCarrierBay* Bay = Cast<ATDCarrierBay>(Part))
		{
			Test.TestTrue(Label + TEXT(": bay doors broken"), Bay->AreDoorsBroken());
		}
		return bHit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRamParts, "TriadDrift.Powers.Ram.Parts.RealDashHitsBayTurretGenerator", TD_TEST_FLAGS)
bool FTDRamParts::RunTest(const FString& Parameters)
{
	const float Steps[] = { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f };
	for (const float Step : Steps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		TDRamPartTest::RamInto(*this, TDRamPartTest::EPart::Bay, Step, TEXT("bay") + Fps);
		TDRamPartTest::RamInto(*this, TDRamPartTest::EPart::Turret, Step, TEXT("turret") + Fps);
		TDRamPartTest::RamInto(*this, TDRamPartTest::EPart::Generator, Step, TEXT("generator") + Fps);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

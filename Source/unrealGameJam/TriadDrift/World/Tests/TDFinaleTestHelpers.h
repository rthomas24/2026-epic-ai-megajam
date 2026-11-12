// Triad Drift - World (Teammate 4). Combat half of the end-to-end finale test (T5 adds the flow half in
// Flow/Tests/TDJourneyTests.cpp). The REAL active squad ship flies to a stand-off point, steers its nose onto
// each target with its flight input and holds its REAL primary weapon: generators from outside the hull,
// then the reactor down the trench mouth, phase by phase. The squad is made invulnerable while it runs.
#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDFinaleCombat
{
	inline const FName FlyOwner(TEXT("FinaleTestPilot"));

	/** The ship the player flies right now (a retake or switch may change it). */
	inline ATDShip* ActiveShip(FTDTestWorld& W)
	{
		return Cast<ATDShip>(UGameplayStatics::GetPlayerPawn(W.GetWorld(), 0));
	}

	/** Flies the active ship at Target from StandOff and holds the primary until bDone() or Seconds run out. */
	template <typename TDoneFn>
	bool Attack(FTDTestWorld& W, AActor* Target, const FVector& StandOff, float Fps, float Seconds, TDoneFn&& bDone)
	{
		for (int32 i = 0; i < FMath::RoundToInt(Fps * Seconds); ++i)
		{
			if (bDone()) { return true; }
			ATDShip* Ship = ActiveShip(W);
			if (!Ship || !IsValid(Target)) { W.Tick(1.f / Fps); continue; }
			if (UTDHealthComponent* H = Ship->GetHealth()) { H->SetInvulnerable(true); }
			const float Dist = FVector::Dist(Ship->GetActorLocation(), Target->GetActorLocation());
			// Start (or restart after a pass) at the stand-off point, nose on the target: a fresh attack run.
			if (i == 0 || Dist < 2500.f || Dist > FVector::Dist(StandOff, Target->GetActorLocation()) * 1.6f)
			{
				Ship->TeleportSpacecraft(StandOff, (Target->GetActorLocation() - StandOff).Rotation(), false);
			}
			Ship->SetSpeedOverrideFor(FlyOwner, 1500.f, 1.f);	// a slow strafing run, flown by the flight model
			FTDFlightInput In;
			In.Aim = ATDEnemyAIController::ComputeSteer(Ship->GetActorTransform(), Target->GetActorLocation(), 20.f);
			Ship->SetFlightInput(In);
			if (UTDWeaponComponent* Wp = Ship->GetWeapons())
			{
				Wp->SetAimTarget(Target);
				Wp->SetAimDirection((Target->GetActorLocation() - Ship->GetActorLocation()).GetSafeNormal());
				Wp->SetTriggerHeld(ETDWeaponSlot::Primary, true);
			}
			W.Tick(1.f / Fps);
		}
		return bDone();
	}

	inline void Release(FTDTestWorld& W)
	{
		if (ATDShip* Ship = ActiveShip(W))
		{
			Ship->ClearSpeedOverrideFor(FlyOwner);
			if (UTDWeaponComponent* Wp = Ship->GetWeapons()) { Wp->SetTriggerHeld(ETDWeaponSlot::Primary, false); Wp->SetAimTarget(nullptr); }
		}
	}

	/** Destroys Count live generators with the active ship's real weapons. False on a timeout. */
	inline bool DestroyGenerators(FAutomationTestBase& Test, FTDTestWorld& W, ATDDominion* Dom, int32 Count, float Fps)
	{
		if (!Dom) { return false; }
		int32 Done = 0;
		for (ATDShieldGenerator* G : Dom->GetGenerators())
		{
			if (Done >= Count) { break; }
			if (!G || !G->IsAlive()) { continue; }
			const FVector Out = (G->GetActorLocation() - Dom->GetActorLocation()).GetSafeNormal2D();
			const FVector StandOff = G->GetActorLocation() + Out * 7000.f + FVector(0.f, 0.f, 2500.f);
			const TWeakObjectPtr<ATDShieldGenerator> Weak(G);
			const bool bKilled = Attack(W, G, StandOff, Fps, 60.f, [Weak] { return !Weak.IsValid() || !Weak->IsAlive(); });
			Test.TestTrue(*FString::Printf(TEXT("@%.0f fps: generator %d destroyed by the squad's real weapons"), Fps, Done + 1), bKilled);
			if (!bKilled) { Release(W); return false; }
			++Done;
		}
		Release(W);
		return Done >= Count;
	}

	/** Takes the exposed reactor through Phases phases, firing down the trench mouth. False on a timeout. */
	inline bool DestroyReactorPhases(FAutomationTestBase& Test, FTDTestWorld& W, ATDDominion* Dom, int32 Phases, float Fps)
	{
		ATDDominionReactor* R = Dom ? Dom->GetReactor() : nullptr;
		if (!R || !R->IsExposed()) { Test.AddError(TEXT("reactor not exposed")); return false; }
		// The trench mouth: ahead of the reactor along the hull's axis and above it (as TD.World.Goto Reactor).
		const FVector StandOff = R->GetActorLocation() + Dom->GetActorForwardVector() * 9000.f + FVector(0.f, 0.f, 4000.f);
		const TWeakObjectPtr<ATDDominionReactor> Weak(R);
		const int32 StartPhase = R->GetPhase();
		const int32 TargetPhase = StartPhase + Phases;	// PhaseCount + 1 = destroyed
		const bool bOk = Attack(W, R, StandOff, Fps, 90.f, [Weak, TargetPhase] { return !Weak.IsValid() || Weak->GetPhase() >= TargetPhase; });
		Release(W);
		Test.TestTrue(*FString::Printf(TEXT("@%.0f fps: reactor taken from phase %d through %d phase(s) by real weapons"), Fps, StartPhase, Phases), bOk);
		return bOk;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS

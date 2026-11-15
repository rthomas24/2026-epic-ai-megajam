// Triad Drift - aim assist hit rate (issue #201; Teammate 2).
//
// TDCombat.AimAssist.HitRate: the same scripted "mediocre driver" flies each ship through waves 1, 3 and 5 with the
// assist off, then on, and logs shots, hits and hit % per weapon slot. The driver aims at the target's body (no lead)
// with up to 7 degrees of error re-rolled twice a second, fires its primary whenever the target is within 10 degrees of
// its aim at 80-260 m, and its secondary every 2.5 s when lined up. Its reticle is the targeting view (as a player's
// is). Wingmen are passive and the flown ship is invulnerable so only the player's shooting is measured. Informational:
// it fails only when the assist makes a ship worse.

#include "Misc/AutomationTest.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDAccuracyStats.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "HAL/IConsoleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace TDAimHitRate
{
	struct FResult
	{
		int32 Shots[2] = { 0, 0 };
		int32 Hits[2] = { 0, 0 };
	};

	void SetAssist(bool bOn)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Aim.Enabled")))
		{
			V->Set(bOn ? 1 : 0, ECVF_SetByCode);
		}
	}

	FResult Run(ETDShipId ShipId, int32 WaveIndex, bool bAssist, float Seconds)
	{
		FResult R;
		SetAssist(bAssist);
		const ETDDifficulty Prev = UTDDifficultySettings::GetCurrent();
		UTDDifficultySettings::SetCurrent(ETDDifficulty::Standard);
		{
			FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
			APlayerController* PC = W.SpawnPlayerController();
			ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform::Identity);
			Squad->RequestSwitchTo(ShipId);
			Squad->SetWingmenPassive(true);
			ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
			const float Step = 1.f / 60.f;
			W.Tick(Step);
			D->StartWave(WaveIndex);
			W.Advance(2.f, Step); // warp-in

			FRandomStream Rng(2026 + WaveIndex * 31 + static_cast<int32>(ShipId) * 7);
			FVector Jitter = FVector::ZeroVector;
			float Reroll = 0.f, Secondary = 0.f;
			FTDAccuracyStats::Reset(0.0);
			const int32 Ship = static_cast<int32>(ShipId);
			for (float T = 0.f; T < Seconds; T += Step)
			{
				ATDShip* Flown = Squad->GetActiveShip();
				if (!Flown) { W.Tick(Step); continue; }
				if (UTDHealthComponent* H = Flown->GetHealth()) { H->SetInvulnerable(true); }
				UTDWeaponComponent* Guns = Flown->GetWeapons();
				UTDTargetingComponent* Targeting = Flown->GetTargeting();
				const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(W.GetWorld());
				AActor* Target = Registry ? Registry->FindNearestHostile(ETDTeam::Player, Flown->GetActorLocation(), 0.f, Flown) : nullptr;
				FTDFlightInput In;
				if (!Target || !Guns)
				{
					Flown->SetFlightInput(In);
					if (Guns) { Guns->SetTriggerHeld(ETDWeaponSlot::Primary, false); }
					W.Tick(Step);
					continue;
				}
				Reroll -= Step;
				if (Reroll <= 0.f)
				{
					Reroll = .5f;
					Jitter = Rng.GetUnitVector() * FMath::Tan(FMath::DegreesToRadians(Rng.FRandRange(0.f, 7.f)));
				}
				const FVector To = Target->GetActorLocation() - Flown->GetActorLocation();
				const float Dist = To.Size();
				const FVector Aim = (To.GetSafeNormal() + Jitter).GetSafeNormal();
				// Keep the fight at 100-250 m: close in when far, ease off when near.
				In.Aim = ATDEnemyAIController::ComputeSteer(Flown->GetActorTransform(), Flown->GetActorLocation() + Aim * 10000.f, 25.f);
				In.Throttle = Dist > 20000.f ? 1.f : (Dist < 10000.f ? -1.f : 0.f);
				Flown->SetFlightInput(In);
				Guns->SetAimDirection(Aim);
				if (Targeting) { Targeting->SetViewOverride(Flown->GetActorLocation(), Aim.Rotation()); }
				const float OffDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Aim, To.GetSafeNormal()), -1.f, 1.f)));
				const bool bLinedUp = OffDeg <= 10.f && Dist >= 8000.f && Dist <= 26000.f;
				Guns->SetTriggerHeld(ETDWeaponSlot::Primary, bLinedUp);
				Secondary -= Step;
				if (bLinedUp && Secondary <= 0.f)
				{
					Secondary = 2.5f;
					Guns->SetTriggerHeld(ETDWeaponSlot::Secondary, true);
				}
				else
				{
					Guns->SetTriggerHeld(ETDWeaponSlot::Secondary, false);
				}
				W.Tick(Step);
			}
			W.Advance(1.5f, Step); // shots in flight land
			for (int32 Slot = 0; Slot < 2; ++Slot)
			{
				R.Shots[Slot] = FTDAccuracyStats::Shots[Ship][Slot];
				R.Hits[Slot] = FTDAccuracyStats::Hits[Ship][Slot];
			}
			D->StopAndClear();
		}
		UTDDifficultySettings::SetCurrent(Prev);
		SetAssist(true);
		return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAimAssistHitRate, "TDCombat.AimAssist.HitRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDAimAssistHitRate::RunTest(const FString& Parameters)
{
	using namespace TDAimHitRate;
	const ETDShipId Ships[] = { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion };
	const TCHAR* Names[] = { TEXT("Rift"), TEXT("Lancer"), TEXT("Bastion") };
	const float Seconds = Parameters.IsEmpty() ? 30.f : FCString::Atof(*Parameters);
	for (int32 S = 0; S < 3; ++S)
	{
		int32 Total[2][2] = { { 0, 0 }, { 0, 0 } }; // [assist][shots, hits]
		for (const int32 Wave : { 0, 2, 4 })
		{
			const FResult Off = Run(Ships[S], Wave, false, Seconds);
			const FResult On = Run(Ships[S], Wave, true, Seconds);
			auto Pct = [](int32 H, int32 N) { return N > 0 ? 100.f * H / N : 0.f; };
			AddInfo(FString::Printf(TEXT("AIMRATE %-7s wave %d | OFF primary %4d/%4d %5.1f%%  secondary %3d/%3d %5.1f%% | ON primary %4d/%4d %5.1f%%  secondary %3d/%3d %5.1f%%"),
				Names[S], Wave + 1,
				Off.Hits[0], Off.Shots[0], Pct(Off.Hits[0], Off.Shots[0]), Off.Hits[1], Off.Shots[1], Pct(Off.Hits[1], Off.Shots[1]),
				On.Hits[0], On.Shots[0], Pct(On.Hits[0], On.Shots[0]), On.Hits[1], On.Shots[1], Pct(On.Hits[1], On.Shots[1])));
			Total[0][0] += Off.Shots[0] + Off.Shots[1]; Total[0][1] += Off.Hits[0] + Off.Hits[1];
			Total[1][0] += On.Shots[0] + On.Shots[1]; Total[1][1] += On.Hits[0] + On.Hits[1];
		}
		const float OffPct = Total[0][0] > 0 ? 100.f * Total[0][1] / Total[0][0] : 0.f;
		const float OnPct = Total[1][0] > 0 ? 100.f * Total[1][1] / Total[1][0] : 0.f;
		AddInfo(FString::Printf(TEXT("AIMRATE %-7s all three waves: OFF %.1f%% (%d shots) -> ON %.1f%% (%d shots)"), Names[S], OffPct, Total[0][0], OnPct, Total[1][0]));
		TestTrue(FString::Printf(TEXT("%s: the assist does not lower the hit rate"), Names[S]), OnPct + 2.f >= OffPct);
	}
	return true;
}

#endif

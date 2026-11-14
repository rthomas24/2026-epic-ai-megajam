// Triad Drift - Squad (Teammate 3). ENG-43: the bark rows dialogs.md gives T3 (player kill, multi-kill, shield down and
// back, the breather, "on your six", Command on a retake). Every line goes through RequestSquadBark / the limiter, so
// priorities, quiet stretches and per-id cooldowns decide what plays.

#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDSquadBarks.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

bool ATDSquadManager::RequestBarkFromAWingman(FName BarkId, ETDBarkPriority Priority)
{
	TArray<ETDShipId> Wingmen;
	for (const FTDSquadSlot& Slot : Slots)
	{
		if (Slot.Ship && Slot.ShipId != ActiveShipId && Slot.State == ETDSquadSlotState::Alive)
		{
			Wingmen.Add(Slot.ShipId);
		}
	}
	return Wingmen.Num() > 0 && RequestSquadBark(BarkId, Wingmen[FMath::RandHelper(Wingmen.Num())], Priority);
}

void ATDSquadManager::NotePlayerKill()
{
	const double Now = RealNow();
	PlayerKillTimes.Add(Now);
	PlayerKillTimes.RemoveAll([Now](double T) { return Now - T > 4.0; });
	if (PlayerKillTimes.Num() >= 3)
	{
		PlayerKillTimes.Reset(); // one call per streak
		RequestBarkFromAWingman(TDBarkIds::MultiKill, ETDBarkPriority::High);
		return;
	}
	if (FMath::FRand() < PlayerKillBarkChance)
	{
		RequestBarkFromAWingman(TDBarkIds::PlayerKill, ETDBarkPriority::Flavour);
	}
}

void ATDSquadManager::HandleShieldBroken(UTDHealthComponent* Health, const FTDDamageInfo& /*Info*/)
{
	for (const FTDSquadSlot& Slot : Slots)
	{
		if (Slot.Ship && Health && Health->GetOwner() == Slot.Ship && Slot.State == ETDSquadSlotState::Alive)
		{
			ShieldsBroken.Add(Slot.ShipId, RealNow());
			ShieldDownPending.Add(Slot.ShipId);
			return;
		}
	}
}

void ATDSquadManager::HandleTakeStartedForBarks(FName /*SceneId*/, int32 TakeNumber)
{
	// The first line of a retake after "CUT!" (Command), one take in two.
	if (TakeNumber >= 2 && FMath::FRand() < RetakeBarkChance && BarkLimiter.TryConsume(BarkTuning, TDBarkIds::CmdRetake, ETDBarkPriority::Normal, RealNow()))
	{
		TDSquad::RequestBark(this, TDBarkIds::CmdRetake, TEXT("Command"));
	}
}

void ATDSquadManager::TickMoreBarks(double Now)
{
	// Shield down: said a tick later, so a hit that also drops the hull low gives way to the low-hull line.
	for (const ETDShipId Id : ShieldDownPending)
	{
		const FTDSquadSlot* Slot = FindSlot(Id);
		if (Slot && Slot->Ship && Slot->State == ETDSquadSlotState::Alive && TDSquad::GetHull01(Slot->Ship) >= BarkTuning.LowHullFraction)
		{
			RequestSquadBark(TDBarkIds::ShieldDown, Id, ETDBarkPriority::High);
		}
	}
	ShieldDownPending.Reset();
	// Shield back: full again after a break (that pilot). Flavour waits for a quiet radio: retried while the shield
	// stays full, given up 15 s after the break.
	for (auto It = ShieldsBroken.CreateIterator(); It; ++It)
	{
		const FTDSquadSlot* Slot = FindSlot(It->Key);
		const UTDHealthComponent* H = Slot && Slot->Ship ? UTDStatics::GetHealth(Slot->Ship) : nullptr;
		if (!H || Slot->State != ETDSquadSlotState::Alive || Now - It->Value > 15.0)
		{
			It.RemoveCurrent();
			continue;
		}
		if (H->MaxShield > 0.f && H->GetShield() >= H->MaxShield - 0.5f && RequestSquadBark(TDBarkIds::ShieldBack, It->Key, ETDBarkPriority::Flavour))
		{
			It.RemoveCurrent();
		}
	}
	// The breather: 4 s into the gap between waves, if the banter isn't talking.
	if (BreatherAt > 0.0 && Now >= BreatherAt)
	{
		BreatherAt = -1.0;
		if (bBetweenWaves && BanterSteps.Num() == 0)
		{
			RequestBarkFromAWingman(TDBarkIds::Breather, ETDBarkPriority::Flavour);
		}
	}
	// On your six: one hostile ship holds behind the flown ship (within range, in the rear cone) for 3 s.
	if (Now >= NextSixCheck)
	{
		NextSixCheck = Now + 0.25;
		const ATDShip* Player = GetActiveShip();
		AActor* Behind = nullptr;
		if (Player)
		{
			const FVector P = Player->GetActorLocation();
			const FVector Fwd = Player->GetActorForwardVector();
			float Best = OnMySixRange;
			for (TActorIterator<APawn> It(GetWorld()); It; ++It)
			{
				APawn* Pawn = *It;
				if (!Pawn || Pawn == Player || !UTDStatics::AreHostile(Player, Pawn))
				{
					continue;
				}
				const FVector To = Pawn->GetActorLocation() - P;
				const float D = To.Size();
				if (D < Best && FVector::DotProduct(To / FMath::Max(D, 1.f), Fwd) < -0.7f)
				{
					Best = D;
					Behind = Pawn;
				}
			}
		}
		if (!Behind || Behind != SixEnemy.Get())
		{
			SixEnemy = Behind;
			SixSince = Behind ? Now : -1.0;
		}
		else if (SixSince >= 0.0 && Now - SixSince >= OnMySixSeconds)
		{
			SixSince = -1.0; // once per tail
			RequestBarkFromAWingman(TDBarkIds::OnMySix, ETDBarkPriority::High);
		}
	}
}

bool ATDSquadManager::CommandSays(FName BarkId, ETDBarkPriority Priority)
{
	if (!BarkLimiter.TryConsume(BarkTuning, BarkId, Priority, RealNow()))
	{
		return false;
	}
	TDSquad::RequestBark(this, BarkId, TEXT("Command"));
	return true;
}

void ATDSquadManager::HandleAbilityUnlockedForBarks(ETDAbilityId /*Ability*/, ETDShipId Ship)
{
	// The pilot of the ship that earned it (Wren: Cloak, Kato: EMP Pulse, Moss: Gravity Well; one row per pilot).
	RequestSquadBark(TDBarkIds::AbilityUnlocked, Ship, ETDBarkPriority::High);
}

void ATDSquadManager::HandleObjectiveForBarks(FName ObjectiveId, int32 /*Step*/, FText /*Text*/)
{
	// Space objectives only (the planets' own lines are T5's; waves have their slate): an objective line that changes to
	// another means the last one is done ("Copy, Triad. Good work.") and, if nothing says that, the new one is set.
	const FString Id = ObjectiveId.ToString();
	const bool bSpace = !ObjectiveId.IsNone() && !Id.StartsWith(TEXT("Wave")) && !Id.StartsWith(TEXT("Return_")) && !Id.StartsWith(TEXT("Obj_"))
		&& !Id.StartsWith(TEXT("Victory"));
	if (ObjectiveId == LastSpaceObjective)
	{
		return; // a step of the same objective
	}
	const bool bHadOne = !LastSpaceObjective.IsNone();
	LastSpaceObjective = bSpace ? ObjectiveId : NAME_None;
	if (bHadOne && CommandSays(TDBarkIds::CmdObjectiveDone, ETDBarkPriority::Normal))
	{
		return;
	}
	if (bSpace)
	{
		CommandSays(TDBarkIds::CmdObjectiveNew, ETDBarkPriority::Normal);
	}
}

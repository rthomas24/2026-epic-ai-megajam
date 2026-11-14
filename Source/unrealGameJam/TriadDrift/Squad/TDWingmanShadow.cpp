// Triad Drift - Squad. Issue #201: shadow mode for the wingman AI, and wingmen that use their powers.
//
// Shadow ON : the leash and the target range are short and centred on the player (TDWingmanAIController.cpp PickTarget).
// Shadow OFF: PickTargetSolo below: the wingman's own choice inside the capped solo leash.
// Powers    : UpdatePowers, a few times a second, in both modes. Every power goes through the player's own power
//             component and cooldown (TDSquad::ActivateWingPower -> UTDPowerComponent::TryActivateFromAI), plus an AI
//             pad so they never spam. Every use logs: "TDSQUAD wingman <ship> used <power> on <target> because <reason>".

#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/Squad/TDShadowMode.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/World.h"

namespace TDWingShadow
{
	constexpr int32 SlotSignature = 0;
	constexpr int32 SlotSignature2 = 1;
	constexpr int32 SlotAbility = 2;

	/** Lancer's ram: the target must be this well lined up, and between these distances (cm). */
	constexpr float RamConeDeg = 7.f;
	constexpr float RamMinDistance = 2500.f;
	constexpr float RamMaxDistance = 12000.f;
	/** The dash is ended this far past the target (cm). */
	constexpr float RamOvershoot = 1500.f;
	/** Bastion's swarm: targets within this range and cone of the nose (cm, deg), at most this many. */
	constexpr float SwarmRange = 20000.f;
	constexpr float SwarmConeDeg = 55.f;
	constexpr int32 SwarmMaxTargets = 6;
	/** "Under fire": hit within this many seconds. "Low": under this health. */
	constexpr float UnderFireSeconds = 2.f;
	constexpr float DomeHealth01 = 0.6f;
	constexpr float CloakHealth01 = 0.45f;
	/** Rift's portal against a missile needs the missile at least this far past the portal's range (cm). */
	constexpr float PortalMissileMargin = 1000.f;

	static FString ShipName(ETDShipId Id)
	{
		return UEnum::GetDisplayValueAsText(Id).ToString();
	}

	static int32 CountWithin(const TArray<AActor*>& Hostiles, const FVector& Center, float Radius)
	{
		int32 N = 0;
		for (const AActor* H : Hostiles)
		{
			if (IsValid(H) && !TDSquad::IsDead(H) && FVector::DistSquared(H->GetActorLocation(), Center) <= FMath::Square(Radius))
			{
				++N;
			}
		}
		return N;
	}
}

// ---------------------------------------------------------------------------------------------
// Shadow mode
// ---------------------------------------------------------------------------------------------

bool ATDWingmanAIController::IsShadowMode() const
{
	const ATDSquadManager* S = Squad.Get();
	return !S || S->IsShadowMode();
}

float ATDWingmanAIController::GetLeash() const
{
	return IsShadowMode() ? FMath::Min(LeashDistance, TDShadow::ShadowLeash()) : TDShadow::SoloLeash();
}

void ATDWingmanAIController::NotifyShadowModeChanged()
{
	NextTargetEvalTime = Now();
	bRegroupCommitted = false;
}

AActor* ATDWingmanAIController::PickTargetSolo(ATDShip* Ship, ATDShip* Leader) const
{
	bDefendingPlayer = false; // on its own: nobody covers the player
	TDSquad::GetPlayerHostiles(GetWorld(), ScratchHostiles);
	if (ScratchHostiles.Num() == 0)
	{
		return nullptr;
	}
	const ATDSquadManager* S = Squad.Get();
	const AActor* OtherWingTarget = nullptr;
	if (S)
	{
		for (const FTDSquadSlot& Slot : S->GetSlots())
		{
			const ATDShip* Other = Slot.Ship;
			if (Slot.State == ETDSquadSlotState::Alive && Other && Other != Ship && Other != Leader)
			{
				if (const ATDWingmanAIController* OtherAI = Cast<ATDWingmanAIController>(Other->GetController()))
				{
					OtherWingTarget = OtherAI->GetCurrentTarget();
				}
			}
		}
	}

	const FVector SelfLoc = Ship->GetActorLocation();
	const float HuntSq = FMath::Square(TDShadow::SoloHuntRange());
	// Never pick something it could not reach without breaking the leash (a small margin: guns reach further).
	const float LeashSq = FMath::Square(GetLeash() * 0.95f);
	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (AActor* H : ScratchHostiles)
	{
		if (!IsValid(H) || TDSquad::IsDead(H))
		{
			continue;
		}
		const FVector HLoc = H->GetActorLocation();
		const float DistSq = FVector::DistSquared(SelfLoc, HLoc);
		if (DistSq > HuntSq || (Leader && FVector::DistSquared(HLoc, Leader->GetActorLocation()) > LeashSq))
		{
			continue;
		}
		// Nearest first (2000 cm bands), the weaker one inside a band; whoever is hunting THIS wingman is the
		// highest threat; spread out from the other wingman; stick with the current target.
		float Score = FMath::FloorToFloat(FMath::Sqrt(DistSq) / 2000.f) + TDSquad::GetHealth01(H) * 0.99f;
		if (TDSquad::GetEnemyTarget(H) == Ship)
		{
			Score -= 2.f;
		}
		if (H == OtherWingTarget)
		{
			Score += 2.f;
		}
		if (H == CurrentTarget.Get())
		{
			Score -= 0.75f;
		}
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = H;
		}
	}
	return Best;
}

// ---------------------------------------------------------------------------------------------
// Powers
// ---------------------------------------------------------------------------------------------

bool ATDWingmanAIController::IsPowerReadyForAI(const ATDShip* Ship, int32 Slot) const
{
	TDSquad::FWingPower P;
	if (Slot < 0 || Slot > 2 || !TDSquad::GetWingPower(Ship, Slot, P) || !P.bReady)
	{
		return false;
	}
	// The power's own cooldown has passed (bReady); the AI waits a little longer than a player would.
	return Now() - LastPowerUseTime[Slot] >= P.CycleSeconds * TDShadow::PowerCooldownPad();
}

bool ATDWingmanAIController::UsePower(ATDShip* Ship, int32 Slot, const AActor* Target, const FString& Reason, FName BarkId)
{
	TDSquad::FWingPower P;
	if (!TDSquad::GetWingPower(Ship, Slot, P) || !TDSquad::ActivateWingPower(Ship, Slot))
	{
		return false;
	}
	const double T = Now();
	LastPowerUseTime[Slot] = T;
	LastAnyPowerTime = T;
	++PowerUseCount[Slot];
	UE_LOG(LogTriadDrift, Display, TEXT("TDSQUAD wingman %s used %s on %s because %s (%s)"), *TDWingShadow::ShipName(ShipId), *P.Name.ToString(),
		Target ? *Target->GetName() : TEXT("no target"), *Reason, IsShadowMode() ? TEXT("shadow") : TEXT("solo"));
	// The pilot's own line for this power (dialogs.md), through the squad's limiter.
	if (ATDSquadManager* S = Squad.Get(); S && !BarkId.IsNone())
	{
		S->RequestSquadBark(BarkId, ShipId, ETDBarkPriority::Normal);
	}
	return true;
}

void ATDWingmanAIController::UpdatePowers(ATDShip* Ship, ATDShip* Leader)
{
	const double T = Now();

	// A ram this AI started: end the dash once it is past the target (or the target is gone), or over the leash.
	// The dash covers several hundred metres if left alone.
	if (bAIRamming)
	{
		TDSquad::FWingPower Ram;
		if (!TDSquad::GetWingPower(Ship, TDWingShadow::SlotSignature, Ram) || !Ram.bActive)
		{
			bAIRamming = false;
			RamTarget.Reset();
		}
		else if (TDSquad::IsRamDashing(Ship))
		{
			const AActor* RT = RamTarget.Get();
			const FVector Loc = Ship->GetActorLocation();
			const bool bPast = !RT || TDSquad::IsDead(RT)
				|| FVector::DotProduct(RT->GetActorLocation() - Loc, Ship->GetActorForwardVector()) < -TDWingShadow::RamOvershoot;
			const bool bOverLeash = Leader && FVector::Dist(Loc, Leader->GetActorLocation()) > GetLeash();
			if (bPast || bOverLeash)
			{
				TDSquad::EndWingPower(Ship, TDWingShadow::SlotSignature);
				bAIRamming = false;
				RamTarget.Reset();
			}
		}
	}

	if (T < NextPowerThinkTime)
	{
		return;
	}
	NextPowerThinkTime = T + 0.25;
	if (!TDShadow::PowersEnabled() || bHyperdriveFormUp || T - LastAnyPowerTime < TDShadow::PowerGlobalGap()
		|| !TDSquad::WingmanPowersAllowed(this) || TDSquad::GetHyperdriveState(Ship) != ETDHyperdriveState::Idle)
	{
		return;
	}
	switch (ShipId)
	{
	case ETDShipId::Lancer: ThinkLancerPowers(Ship, Leader); break;
	case ETDShipId::Bastion: ThinkBastionPowers(Ship, Leader); break;
	case ETDShipId::Rift: ThinkRiftPowers(Ship, Leader); break;
	default: break;
	}
}

void ATDWingmanAIController::ThinkLancerPowers(ATDShip* Ship, ATDShip* Leader)
{
	const FVector Loc = Ship->GetActorLocation();

	// EMP Pulse (Cinder): a crowd inside the pulse, or a missile on us with anything in reach.
	if (IsPowerReadyForAI(Ship, TDWingShadow::SlotAbility))
	{
		const float Radius = TDSquad::GetEMPRadius(Ship) * 0.9f;
		if (Radius > 0.f)
		{
			TDSquad::GetPlayerHostiles(GetWorld(), ScratchHostiles);
			const int32 InReach = TDWingShadow::CountWithin(ScratchHostiles, Loc, Radius);
			const bool bMissile = TDSquad::IsMissileIncoming(Ship);
			if (InReach >= TDShadow::ClusterCount() || (bMissile && InReach >= 1))
			{
				const FString Reason = InReach >= TDShadow::ClusterCount()
					? FString::Printf(TEXT("%d enemies within %.0f m"), InReach, Radius / 100.f)
					: FString::Printf(TEXT("a missile is on it and %d enemy within %.0f m"), InReach, Radius / 100.f);
				if (UsePower(Ship, TDWingShadow::SlotAbility, CurrentTarget.Get(), Reason, InReach >= 3 ? TDBarkIdsT2::EMPHit : TDBarkIdsT2::EMPFire))
				{
					return;
				}
			}
		}
	}

	// Afterburner ram: only at a target that is lined up, with a clear line, and that keeps the dash inside the leash.
	AActor* Target = CurrentTarget.Get();
	if (State != ETDWingmanState::Engage || !Target || TDSquad::IsDead(Target) || !IsPowerReadyForAI(Ship, TDWingShadow::SlotSignature))
	{
		return;
	}
	const FVector TLoc = Target->GetActorLocation();
	const FVector To = TLoc - Loc;
	const float Dist = To.Size();
	if (Dist < TDWingShadow::RamMinDistance || Dist > TDWingShadow::RamMaxDistance)
	{
		return;
	}
	const FVector Fwd = Ship->GetActorForwardVector();
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Fwd, To / Dist), -1.f, 1.f)));
	if (Angle > TDWingShadow::RamConeDeg)
	{
		return;
	}
	if (Leader)
	{
		const FVector LLoc = Leader->GetActorLocation();
		// Not across the player's nose, and not at something that would drag the dash over the leash.
		if (FVector::DistSquared(FMath::ClosestPointOnSegment(LLoc, Loc, TLoc + Fwd * TDWingShadow::RamOvershoot), LLoc) < FMath::Square(1200.f)
			|| FVector::Dist(TLoc, LLoc) > GetLeash())
		{
			return;
		}
	}
	FVector Unused;
	if (TDSquad::FindRegisteredAvoidance(GetWorld(), Loc, Fwd, Dist + TDWingShadow::RamOvershoot, AvoidanceShipRadius, Unused))
	{
		return; // a rock or a hull in the way
	}
	if (UsePower(Ship, TDWingShadow::SlotSignature, Target,
		FString::Printf(TEXT("it is lined up %.0f m ahead, %.0f deg off the nose"), Dist / 100.f, Angle), TDBarkIdsT2::RamGo))
	{
		bAIRamming = true;
		RamTarget = Target;
	}
}

void ATDWingmanAIController::ThinkBastionPowers(ATDShip* Ship, ATDShip* Leader)
{
	const FVector Loc = Ship->GetActorLocation();
	const ATDSquadManager* S = Squad.Get();

	// Shield dome: the player or a wingman inside its reach is under fire and hurt, or has a missile on it.
	if (S && IsPowerReadyForAI(Ship, TDWingShadow::SlotSignature2))
	{
		const float Reach = TDSquad::GetDomeRadius(Ship) * 0.85f;
		const ATDShip* Saved = nullptr;
		FString Why;
		for (const FTDSquadSlot& Slot : S->GetSlots())
		{
			const ATDShip* Mate = Slot.Ship;
			if (Slot.State != ETDSquadSlotState::Alive || !Mate || FVector::DistSquared(Mate->GetActorLocation(), Loc) > FMath::Square(Reach))
			{
				continue;
			}
			const float Health = TDSquad::GetHealth01(Mate);
			const bool bUnderFire = TDSquad::GetTimeSinceLastDamage(Mate) <= TDWingShadow::UnderFireSeconds;
			const bool bMissile = TDSquad::IsMissileIncoming(Mate);
			if ((bUnderFire && Health < TDWingShadow::DomeHealth01) || bMissile)
			{
				// The player's ship first, then whoever is worst off.
				if (!Saved || Mate == Leader)
				{
					Saved = Mate;
					Why = bMissile
						? FString::Printf(TEXT("a missile is on %s%s"), *TDWingShadow::ShipName(Slot.ShipId), Mate == Leader ? TEXT(" (the player)") : TEXT(""))
						: FString::Printf(TEXT("%s%s is under fire at %.0f%% health"), *TDWingShadow::ShipName(Slot.ShipId), Mate == Leader ? TEXT(" (the player)") : TEXT(""), Health * 100.f);
				}
			}
		}
		if (Saved && UsePower(Ship, TDWingShadow::SlotSignature2, Saved, Why, TDBarkIdsT2::DomeUp))
		{
			return;
		}
	}

	AActor* Target = CurrentTarget.Get();
	if (State != ETDWingmanState::Engage || !Target || TDSquad::IsDead(Target))
	{
		return;
	}
	TDSquad::GetPlayerHostiles(GetWorld(), ScratchHostiles);
	const FVector Fwd = Ship->GetActorForwardVector();

	// Gravity Well (Verdance): a cluster where the orb will open.
	if (IsPowerReadyForAI(Ship, TDWingShadow::SlotAbility))
	{
		float Flight = 0.f, Radius = 0.f;
		TDSquad::GetGravityWellReach(Ship, Flight, Radius);
		if (Radius > 0.f)
		{
			const FVector Opens = Loc + TDSquad::GetAimDirection(Ship) * Flight;
			const int32 Caught = TDWingShadow::CountWithin(ScratchHostiles, Opens, Radius * 0.8f);
			if (Caught >= TDShadow::ClusterCount()
				&& UsePower(Ship, TDWingShadow::SlotAbility, Target, FString::Printf(TEXT("%d enemies within %.0f m of where the well opens"), Caught, Radius * 0.8f / 100.f), TDBarkIdsT2::GravityFire))
			{
				return;
			}
		}
	}

	// Missile swarm: a cluster ahead, or one tough target.
	if (IsPowerReadyForAI(Ship, TDWingShadow::SlotSignature))
	{
		const float CosCone = FMath::Cos(FMath::DegreesToRadians(TDWingShadow::SwarmConeDeg));
		TArray<AActor*> Picks;
		Picks.Reserve(ScratchHostiles.Num());
		for (AActor* H : ScratchHostiles)
		{
			if (!IsValid(H) || TDSquad::IsDead(H))
			{
				continue;
			}
			const FVector To = H->GetActorLocation() - Loc;
			const float D = To.Size();
			if (D <= TDWingShadow::SwarmRange && D > KINDA_SMALL_NUMBER && FVector::DotProduct(Fwd, To / D) >= CosCone)
			{
				Picks.Add(H);
			}
		}
		if (!Picks.Contains(Target))
		{
			return; // the swarm leaves the nose: only when its own target is in front of it
		}
		Picks.Sort([&Loc](const AActor& A, const AActor& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), Loc) < FVector::DistSquared(B.GetActorLocation(), Loc);
		});
		const bool bCluster = Picks.Num() >= TDShadow::ClusterCount();
		const bool bTough = TDSquad::GetMaxHull(Target) >= TDShadow::ToughHull() && TDSquad::GetHealth01(Target) > 0.25f;
		if (!bCluster && !bTough)
		{
			return;
		}
		if (bCluster)
		{
			Picks.SetNum(FMath::Min(Picks.Num(), TDWingShadow::SwarmMaxTargets));
			Picks.AddUnique(Target);
		}
		else
		{
			Picks.Reset();
			Picks.Add(Target); // all twelve at the tough one
		}
		TDSquad::SetSwarmTargets(Ship, Picks);
		UsePower(Ship, TDWingShadow::SlotSignature, Target, bCluster
			? FString::Printf(TEXT("a cluster of %d ahead"), Picks.Num())
			: FString::Printf(TEXT("it is a tough target (%.0f hull, %.0f%% left)"), TDSquad::GetMaxHull(Target), TDSquad::GetHealth01(Target) * 100.f),
			TDBarkIdsT2::SwarmFire);
	}
}

void ATDWingmanAIController::ThinkRiftPowers(ATDShip* Ship, ATDShip* Leader)
{
	const FVector Loc = Ship->GetActorLocation();
	float MissileDist = 0.f;
	const AActor* Missile = TDSquad::GetIncomingMissile(Ship, MissileDist);

	// Portal B: the second press, at whoever Rift is fighting (the missile that goes in at A comes out there).
	if (bPortalBPending)
	{
		bPortalBPending = false;
		TDSquad::FWingPower Portals;
		if (TDSquad::GetWingPower(Ship, TDWingShadow::SlotSignature, Portals) && Portals.bActive && TDSquad::GetOpenPortalCount(Ship) == 1)
		{
			const AActor* Aim = CurrentTarget.Get();
			if (!Aim)
			{
				Aim = Missile ? Missile->GetInstigator() : nullptr;
			}
			FVector Dir = Aim ? (Aim->GetActorLocation() - Loc).GetSafeNormal() : -Ship->GetActorForwardVector();
			if (Dir.IsNearlyZero())
			{
				Dir = Ship->GetActorRightVector();
			}
			TDSquad::SetAim(Ship, nullptr, Dir);
			TDSquad::PressWingPower(Ship, TDWingShadow::SlotSignature);
			UE_LOG(LogTriadDrift, Display, TEXT("TDSQUAD wingman Rift placed portal B toward %s (%d open)"), Aim ? *Aim->GetName() : TEXT("open space"),
				TDSquad::GetOpenPortalCount(Ship));
			return;
		}
	}

	// Portal A in the missile's path: only when the missile is still beyond the portal's range, so it has to cross it.
	if (Missile && TDShadow::PortalsEnabled() && IsPowerReadyForAI(Ship, TDWingShadow::SlotSignature))
	{
		const float Range = TDSquad::GetPortalRange(Ship);
		if (Range > 0.f && MissileDist >= Range + TDWingShadow::PortalMissileMargin && MissileDist <= Range * 2.5f)
		{
			TDSquad::SetAim(Ship, nullptr, (Missile->GetActorLocation() - Loc).GetSafeNormal());
			if (UsePower(Ship, TDWingShadow::SlotSignature, Missile,
				FString::Printf(TEXT("a missile is homing on it from %.0f m: portal A in its path"), MissileDist / 100.f), TDBarkIdsT2::PortalOpen))
			{
				bPortalBPending = true;
				return;
			}
		}
	}

	// Cloak (Verdance): hurt and under fire, or a missile too close for anything else. Missiles lose their lock.
	TDSquad::FWingPower Ability;
	if (IsPowerReadyForAI(Ship, TDWingShadow::SlotAbility) && TDSquad::GetWingPower(Ship, TDWingShadow::SlotAbility, Ability) && Ability.Ability == ETDAbilityId::Cloak)
	{
		const float Health = TDSquad::GetHealth01(Ship);
		const bool bHurt = Health < TDWingShadow::CloakHealth01 && TDSquad::GetTimeSinceLastDamage(Ship) <= TDWingShadow::UnderFireSeconds;
		if (bHurt || Missile)
		{
			UsePower(Ship, TDWingShadow::SlotAbility, bHurt ? TDSquad::GetLastAttacker(Ship) : Missile, bHurt
				? FString::Printf(TEXT("it is under fire at %.0f%% health"), Health * 100.f)
				: FString::Printf(TEXT("a missile is %.0f m out"), MissileDist / 100.f), TDBarkIdsT2::CloakOn);
		}
	}
}

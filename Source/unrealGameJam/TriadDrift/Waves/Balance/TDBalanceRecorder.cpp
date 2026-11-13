// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Waves/Balance/TDBalanceRecorder.h"

#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"

// ----------------------------------------------------------------------------------------- recorder

void UTDBalanceRecorder::Begin(UObject* WorldContext)
{
	Context = WorldContext;
	if (UTDEventBus* Bus = UTDEventBus::Get(WorldContext))
	{
		Bus->OnActorKilled.AddUniqueDynamic(this, &UTDBalanceRecorder::HandleKilled);
		Bus->OnShipDowned.AddUniqueDynamic(this, &UTDBalanceRecorder::HandleShipDowned);
		Bus->OnSquadWiped.AddUniqueDynamic(this, &UTDBalanceRecorder::HandleSquadWiped);
	}
}

void UTDBalanceRecorder::End()
{
	if (UTDEventBus* Bus = UTDEventBus::Get(Context.Get()))
	{
		Bus->OnActorKilled.RemoveDynamic(this, &UTDBalanceRecorder::HandleKilled);
		Bus->OnShipDowned.RemoveDynamic(this, &UTDBalanceRecorder::HandleShipDowned);
		Bus->OnSquadWiped.RemoveDynamic(this, &UTDBalanceRecorder::HandleSquadWiped);
	}
	for (const TWeakObjectPtr<UTDHealthComponent>& H : Watched)
	{
		if (H.IsValid()) { H->OnDamaged.RemoveDynamic(this, &UTDBalanceRecorder::HandleDamaged); }
	}
	Watched.Reset();
}

void UTDBalanceRecorder::WatchSquad(ATDSquadManager* Squad)
{
	if (!Squad)
	{
		return;
	}
	for (ATDShip* Ship : Squad->GetLivingShips())
	{
		UTDHealthComponent* H = Ship ? Ship->GetHealth() : nullptr;
		if (H && !Watched.Contains(H))
		{
			H->OnDamaged.AddUniqueDynamic(this, &UTDBalanceRecorder::HandleDamaged);
			Watched.Add(H);
		}
	}
}

FTDPilotSkill FTDPilotSkill::Weak()
{
	FTDPilotSkill S;
	S.Name = TEXT("Weak");
	S.AimErrorDeg = 7.f;
	S.ReactionSeconds = 0.9f;
	S.FireConeDeg = 5.f;
	S.SwitchEverySeconds = 0.f;
	S.PowerEverySeconds = 0.f;
	S.bHuntsBays = false;
	return S;
}

FTDPilotSkill FTDPilotSkill::Drifter()
{
	FTDPilotSkill S = Weak();
	S.Name = TEXT("Drifter");
	S.bDrifter = true;
	return S;
}

FTDPilotSkill FTDPilotSkill::Evader()
{
	FTDPilotSkill S;	// Average aim and reaction
	S.Name = TEXT("Evader");
	S.bEvader = true;
	S.SwitchEverySeconds = 0.f;	// T2's pilot: one ship per run
	S.PowerEverySeconds = 7.f;	// powers on a timer, not cleverly
	return S;
}

FTDPilotSkill FTDPilotSkill::Passive()
{
	FTDPilotSkill S = Weak();
	S.Name = TEXT("Passive");
	S.bPassive = true;
	return S;
}

FTDPilotSkill FTDPilotSkill::Strong()
{
	FTDPilotSkill S;
	S.Name = TEXT("Strong");
	S.AimErrorDeg = 1.f;
	S.ReactionSeconds = 0.15f;
	S.FireConeDeg = 10.f;
	S.SwitchEverySeconds = 15.f;
	S.PowerEverySeconds = 6.f;
	return S;
}

void UTDBalanceRecorder::HandleKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
	if (Cast<ATDCarrierBay>(Victim)) { ++Current.BaysKilled; return; }
	if (Cast<ATDEnemyTurret>(Victim)) { ++Current.TurretsKilled; return; }
	if (const ATDEnemyShip* E = Cast<ATDEnemyShip>(Victim))
	{
		if (E->GetRole() == ETDEnemyRole::Carrier) { ++Current.CarriersKilled; }
	}
	if (UTDStatics::GetTeam(Victim) != ETDTeam::Enemy || !Victim || !Victim->IsA<APawn>())
	{
		return;	// count ships, not turrets/bays/generators
	}
	++Current.Kills;
	Current.KillsByKind.FindOrAdd(Kind)++;
	if (const ATDShip* Ship = Cast<ATDShip>(Killer))
	{
		Current.KillsByShip.FindOrAdd(Ship->GetShipId())++;
	}
}

void UTDBalanceRecorder::HandleShipDowned(ETDShipId Ship)
{
	++Current.ShipDowns;
	if (Current.FirstDownSeconds < 0.f) { Current.FirstDownSeconds = Current.SecondsTotal; }
	if (Ship == LastActiveShip) { ++Current.ActiveShipDowns; }
}

void UTDBalanceRecorder::HandleSquadWiped()
{
	++Wipes;
}

void UTDBalanceRecorder::HandleDamaged(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	const float D = ShieldDamage + HullDamage;
	Current.DamageTaken += D;
	if (Health && Health->MaxHull > 0.f && Health->GetOwner() && Health->GetOwner() == UGameplayStatics::GetPlayerPawn(Health, 0))
	{
		Current.ActiveHullLost01 += HullDamage / Health->MaxHull;
	}
	FName Source = TEXT("Collision/Other");
	const AActor* By = Info.Instigator ? Info.Instigator.Get() : nullptr;
	if (!By)
	{
		if (const ATDProjectile* P = Cast<ATDProjectile>(Info.Causer.Get())) { By = P->GetOriginalShooter(); }
	}
	if (const ATDEnemyShip* E = Cast<ATDEnemyShip>(By)) { Source = E->GetEnemyId(); }
	else if (Cast<ATDEnemyTurret>(By)) { Source = TEXT("Turret"); }
	Current.DamageBySource.FindOrAdd(Source) += D;
}

// -------------------------------------------------------------------------------------------- pilot

AActor* FTDBalancePilot::PickTarget(UWorld* World, const AActor* Ship) const
{
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(World);
	if (!Registry || !Ship)
	{
		return nullptr;
	}
	AActor* Near = Registry->FindNearestHostile(ETDTeam::Player, Ship->GetActorLocation(), 0.f, Ship);
	// A pilot who knows the carrier: swat fighters within 4000 cm, otherwise go for the bays.
	if (Skill.bHuntsBays && (!Near || FVector::Dist(Near->GetActorLocation(), Ship->GetActorLocation()) > 4000.f))
	{
		float Best = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<AActor>& Weak : Registry->GetAll())
		{
			ATDCarrierBay* Bay = Cast<ATDCarrierBay>(Weak.Get());
			if (!Bay || !Bay->IsAlive() || Bay->IsSpent()) { continue; }
			const float D = FVector::DistSquared(Bay->GetActorLocation(), Ship->GetActorLocation());
			if (D < Best) { Best = D; Near = Bay; }
		}
	}
	return Near;
}

void FTDBalancePilot::Tick(UWorld* World, ATDSquadManager* Squad, float DeltaSeconds, FTDBalanceWaveStats& Stats)
{
	ATDShip* Ship = Squad ? Squad->GetActiveShip() : nullptr;
	if (!World || !Ship)
	{
		return;
	}

	SwitchTimer += DeltaSeconds;
	if (Skill.SwitchEverySeconds > 0.f && SwitchTimer >= Skill.SwitchEverySeconds)
	{
		SwitchTimer = 0.f;
		if (UTDWeaponComponent* W = Ship->GetWeapons()) { W->SetTriggerHeld(ETDWeaponSlot::Primary, false); }
		if (Squad->RequestSwitchNext()) { ++Stats.Switches; }
		Ship = Squad->GetActiveShip();
		if (!Ship) { return; }
	}

	// Readability: is the fight in front of the player? The chase camera's view, roughly: 90 deg horizontal
	// FOV at 16:9 gives 45 deg either side and about 28 deg up/down of the nose.
	Stats.SecondsTotal += DeltaSeconds;
	if (const UTDCombatRegistry* Reg = UTDCombatRegistry::Get(World))
	{
		const FVector Me = Ship->GetActorLocation();
		const FVector Fwd = Ship->GetActorForwardVector();
		const FVector Right = Ship->GetActorRightVector();
		const FVector Up = Ship->GetActorUpVector();
		const float TanH = FMath::Tan(FMath::DegreesToRadians(45.f));
		const float TanV = FMath::Tan(FMath::DegreesToRadians(28.f));
		int32 OnScreen = 0;
		int32 Alive = 0;
		for (const TWeakObjectPtr<AActor>& Weak : Reg->GetAll())
		{
			const ATDEnemyShip* E = Cast<ATDEnemyShip>(Weak.Get());
			if (!E || E->IsDying()) { continue; }
			++Alive;
			const FVector To = E->GetActorLocation() - Me;
			const float Ahead = FVector::DotProduct(To, Fwd);
			if (Ahead > 100.f && To.SizeSquared() < FMath::Square(30000.f)
				&& FMath::Abs(FVector::DotProduct(To, Right)) <= Ahead * TanH && FMath::Abs(FVector::DotProduct(To, Up)) <= Ahead * TanV)
			{
				++OnScreen;
			}
		}
		Stats.EnemyOnScreenCountSeconds += OnScreen * DeltaSeconds;
		if (OnScreen > 0)
		{
			Stats.SecondsEnemyOnScreen += DeltaSeconds;
			Stats.CurrentEmptySeconds = 0.f;
			if (Stats.FirstContactSeconds < 0.f) { Stats.FirstContactSeconds = Stats.SecondsTotal; }
		}
		else if (Alive > 0)
		{
			Stats.CurrentEmptySeconds += DeltaSeconds;
			Stats.LongestEmptySeconds = FMath::Max(Stats.LongestEmptySeconds, Stats.CurrentEmptySeconds);
		}
	}

	if (Skill.bPassive)
	{
		FTDFlightInput Cruise;
		Cruise.Throttle = 0.f;
		Ship->SetFlightInput(Cruise);
		return;
	}

	if (Skill.bDrifter)
	{
		// T2's script: cruise, a 35 degree turn (about 1 s at a quarter deflection) every 11 s, fire 2 s on / 1 s off.
		StyleTimer += DeltaSeconds;
		if (StyleTimer >= 11.f)
		{
			StyleTimer = 0.f;
			TurnRemaining = 1.f;
			TurnSign = Rng.FRand() < 0.5f ? -1.f : 1.f;
		}
		FTDFlightInput In;
		In.Throttle = 0.f;
		if (TurnRemaining > 0.f)
		{
			TurnRemaining -= DeltaSeconds;
			In.Aim = FVector2D(0.35f * TurnSign, 0.f);
		}
		Ship->SetFlightInput(In);
		if (UTDWeaponComponent* Wp = Ship->GetWeapons())
		{
			Wp->SetAimTarget(nullptr);
			Wp->SetAimDirection(Ship->GetActorForwardVector());
			Wp->SetTriggerHeld(ETDWeaponSlot::Primary, FMath::Fmod(StyleTimer, 3.f) < 2.f);
		}
		return;
	}

	// Reaction time: the pilot only re-picks its target every ReactionSeconds.
	ReactionTimer -= DeltaSeconds;
	if (ReactionTimer <= 0.f || !Target.IsValid() || !UTDStatics::GetHealth(Target.Get()) || !UTDStatics::GetHealth(Target.Get())->IsAlive())
	{
		ReactionTimer = Skill.ReactionSeconds;
		Target = PickTarget(World, Ship);
	}
	AActor* T = Target.Get();
	UTDWeaponComponent* Weapons = Ship->GetWeapons();

	FTDFlightInput In;
	if (!T)
	{
		Stats.SecondsNoTarget += DeltaSeconds;
		In.Throttle = 0.f;
		Ship->SetFlightInput(In);
		if (Weapons) { Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false); }
		return;
	}
	const bool bBay = T->IsA<ATDCarrierBay>();
	(bBay ? Stats.SecondsOnBays : Stats.SecondsOnShips) += DeltaSeconds;

	AimRerollTimer -= DeltaSeconds;
	if (AimRerollTimer <= 0.f)
	{
		AimRerollTimer = 0.5f;
		AimJitter = Rng.GetUnitVector() * FMath::Tan(FMath::DegreesToRadians(Skill.AimErrorDeg));
	}

	const FVector To = T->GetActorLocation() - Ship->GetActorLocation();
	const float Dist = To.Size();
	const AActor* Mover = T->GetAttachParentActor() ? T->GetAttachParentActor() : T;
	const FVector Lead = T->GetActorLocation() + Mover->GetVelocity() * FMath::Clamp(Dist / 40000.f, 0.f, 1.f);
	const FVector AimDir = ((Lead - Ship->GetActorLocation()).GetSafeNormal() + AimJitter).GetSafeNormal();

	FVector Goal = Lead;
	if (bBay && Dist < StandOffDistance)
	{
		Goal = Ship->GetActorLocation() - To.GetSafeNormal() * 10000.f + Ship->GetActorUpVector() * 3000.f;	// back out, then turn in again
	}
	else if (Dist < 3000.f)
	{
		Goal = Ship->GetActorLocation() + Ship->GetActorRightVector() * 10000.f + Ship->GetActorForwardVector() * 4000.f;	// pass, don't ram
	}
	In.Aim = ATDEnemyAIController::ComputeSteer(Ship->GetActorTransform(), Goal, 25.f);
	In.Throttle = (Dist < 6000.f || (bBay && Dist < StandOffDistance * 1.3f)) ? 0.f : 1.f;
	In.bBoost = Dist > 25000.f;
	if (Skill.bEvader)
	{
		// T2's agent pilot rules (its live runs are the reference player): turn at most ~60 deg/s toward the
		// target, no boost, a real dodge (T1's RequestDodge, its own 1.5 s cooldown) to alternating sides on a
		// fresh hit, and a dodge pulse every ~6 s even without hits.
		In.Aim.X = FMath::Clamp(In.Aim.X, -0.6f, 0.6f);
		In.Aim.Y = FMath::Clamp(In.Aim.Y, -0.45f, 0.45f);
		In.bBoost = false;
		float HealthSum = -1.f;
		if (const UTDHealthComponent* H = Ship->GetHealth()) { HealthSum = H->GetShield() + H->GetHull(); }
		const bool bSameShip = LastShip.Get() == Ship;
		const bool bHit = bSameShip && LastHealthSum >= 0.f && HealthSum < LastHealthSum - 0.5f;
		LastShip = Ship;
		LastHealthSum = HealthSum;
		// DodgeTimer = reaction countdown after a fresh hit (-1 = none pending).
		WeavePhase += DeltaSeconds;
		if (bHit && DodgeTimer < 0.f) { DodgeTimer = Skill.ReactionSeconds; }
		bool bDodgeNow = false;
		if (DodgeTimer >= 0.f)
		{
			DodgeTimer -= DeltaSeconds;
			if (DodgeTimer < 0.f) { bDodgeNow = true; DodgeTimer = -1.f; }
		}
		if (bDodgeNow || WeavePhase >= 6.f)
		{
			if (UTDShipMovementComponent* Move = Ship->GetShipMovement())
			{
				if (Move->RequestDodge(DodgeSign)) { DodgeSign = -DodgeSign; }
			}
			WeavePhase = 0.f;
		}
	}
	Ship->SetFlightInput(In);

	if (Weapons)
	{
		const float Dot = FVector::DotProduct(To.GetSafeNormal(), Ship->GetActorForwardVector());
		const bool bLinedUp = Dot >= FMath::Cos(FMath::DegreesToRadians(Skill.FireConeDeg)) && Dist <= FireRange;
		Weapons->SetAimTarget(T);
		Weapons->SetAimDirection(AimDir);
		if (Skill.bEvader)
		{
			const FVector Local = Ship->GetActorTransform().InverseTransformVectorNoScale(To.GetSafeNormal());
			const float YawOff = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Local.Y), FMath::Max(KINDA_SMALL_NUMBER, Local.X)));
			const float PitchOff = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Local.Z), FMath::Max(KINDA_SMALL_NUMBER, Local.X)));
			StyleTimer += DeltaSeconds;
			const bool bBurstOn = FMath::Fmod(StyleTimer, 1.2f) < 0.9f;
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, Local.X > 0.f && YawOff <= 20.f && PitchOff <= 15.f && Dist <= FireRange && bBurstOn);
		}
		else
		{
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, bLinedUp);
		}
		SecondaryTimer += DeltaSeconds;
		if (bLinedUp && SecondaryTimer >= SecondaryEverySeconds)
		{
			SecondaryTimer = 0.f;
			Weapons->FireOnce(ETDWeaponSlot::Secondary);
		}
	}

	// Signature powers, the way a player would: when something is close.
	PowerTimer += DeltaSeconds;
	if (Skill.PowerEverySeconds > 0.f && PowerTimer >= Skill.PowerEverySeconds && Dist < 12000.f)
	{
		PowerTimer = 0.f;
		TArray<UTDPowerComponent*> Powers;
		Ship->GetComponents<UTDPowerComponent>(Powers);
		for (UTDPowerComponent* P : Powers)
		{
			if (P && P->TryActivate()) { ++Stats.PowersUsed; break; }
		}
	}
}

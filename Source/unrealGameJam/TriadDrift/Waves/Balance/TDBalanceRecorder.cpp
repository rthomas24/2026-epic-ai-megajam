// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Waves/Balance/TDBalanceRecorder.h"

#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
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

void UTDBalanceRecorder::HandleKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
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

void UTDBalanceRecorder::HandleShipDowned(ETDShipId /*Ship*/)
{
	++Current.ShipDowns;
}

void UTDBalanceRecorder::HandleSquadWiped()
{
	++Wipes;
}

void UTDBalanceRecorder::HandleDamaged(UTDHealthComponent* /*Health*/, const FTDDamageInfo& /*Info*/, float ShieldDamage, float HullDamage)
{
	Current.DamageTaken += ShieldDamage + HullDamage;
}

// -------------------------------------------------------------------------------------------- pilot

void FTDBalancePilot::Tick(UWorld* World, ATDSquadManager* Squad, float DeltaSeconds)
{
	ATDShip* Ship = Squad ? Squad->GetActiveShip() : nullptr;
	if (!World || !Ship)
	{
		return;
	}

	SwitchTimer += DeltaSeconds;
	if (SwitchTimer >= SwitchEverySeconds)
	{
		SwitchTimer = 0.f;
		if (UTDWeaponComponent* W = Ship->GetWeapons()) { W->SetTriggerHeld(ETDWeaponSlot::Primary, false); }
		Squad->RequestSwitchNext();
		Ship = Squad->GetActiveShip();
		if (!Ship) { return; }
	}

	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(World);
	AActor* Target = Registry ? Registry->FindNearestHostile(ETDTeam::Player, Ship->GetActorLocation(), 0.f, Ship) : nullptr;
	// Like a player: with a carrier around, fighters within 4000 cm are swatted, otherwise go for its bays.
	if (Registry && (!Target || FVector::Dist(Target->GetActorLocation(), Ship->GetActorLocation()) > 4000.f))
	{
		float Best = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<AActor>& Weak : Registry->GetAll())
		{
			const ATDCarrierBay* Bay = Cast<ATDCarrierBay>(Weak.Get());
			if (!Bay || !Bay->IsAlive()) { continue; }
			const float D = FVector::DistSquared(Bay->GetActorLocation(), Ship->GetActorLocation());
			if (D < Best) { Best = D; Target = const_cast<ATDCarrierBay*>(Bay); }
		}
	}
	UTDWeaponComponent* Weapons = Ship->GetWeapons();

	FTDFlightInput In;
	if (!Target)
	{
		In.Throttle = 0.f;
		Ship->SetFlightInput(In);
		if (Weapons) { Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false); }
		return;
	}

	const FVector To = Target->GetActorLocation() - Ship->GetActorLocation();
	const float Dist = To.Size();
	const FVector Aim = Target->GetActorLocation() + Target->GetVelocity() * FMath::Clamp(Dist / 40000.f, 0.f, 1.f);
	FVector Goal = Aim;
	const bool bStandOff = Target->IsA<ATDCarrierBay>();	// shoot bays from outside point-defense range
	if (bStandOff && Dist < StandOffDistance)
	{
		Goal = Ship->GetActorLocation() - To.GetSafeNormal() * 10000.f + Ship->GetActorUpVector() * 3000.f;	// back out, then turn in again
	}
	else if (Dist < 3000.f)
	{
		Goal = Ship->GetActorLocation() + Ship->GetActorRightVector() * 10000.f + Ship->GetActorForwardVector() * 4000.f;	// pass, don't ram
	}
	In.Aim = ATDEnemyAIController::ComputeSteer(Ship->GetActorTransform(), Goal, 25.f);
	In.Throttle = (Dist < 6000.f || (bStandOff && Dist < StandOffDistance * 1.3f)) ? 0.f : 1.f;
	In.bBoost = Dist > 25000.f;
	Ship->SetFlightInput(In);

	if (Weapons)
	{
		const float Dot = FVector::DotProduct(To.GetSafeNormal(), Ship->GetActorForwardVector());
		const bool bLinedUp = Dot >= FMath::Cos(FMath::DegreesToRadians(FireConeDeg)) && Dist <= FireRange;
		Weapons->SetAimTarget(Target);
		Weapons->SetAimDirection((Aim - Ship->GetActorLocation()).GetSafeNormal());
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, bLinedUp);
		SecondaryTimer += DeltaSeconds;
		if (bLinedUp && SecondaryTimer >= SecondaryEverySeconds)
		{
			SecondaryTimer = 0.f;
			Weapons->FireOnce(ETDWeaponSlot::Secondary);
		}
	}
}

// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Enemies/TDEnemyAIController.h"

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/World/TDWorldSubsystem.h"

#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	int32 GTDEnemyAIDebug = 0;
	const FName TDEnemyAIOverride(TEXT("EnemyAI"));	// T1 keyed speed overrides: ours never clears an EMP / gravity well
	FAutoConsoleVariableRef CVarTDEnemyAIDebug(TEXT("TD.EnemyAIDebug"), GTDEnemyAIDebug, TEXT("Draw enemy AI state and target lines (1 = on)."));

	FTDFlightInput TDEnemyAI_MakeInput(const FVector2D& Aim, float Throttle01, bool bBoost)
	{
		FTDFlightInput In;
		In.Aim = Aim;
		In.Throttle = FMath::Clamp(Throttle01, 0.f, 1.f) * 2.f - 1.f;	// 0..1 -> min..cruise
		In.bBoost = bBoost;
		return In;
	}
}

ATDEnemyAIController::ATDEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bAttachToPawn = true;
	Rng.Initialize(9001);
}

ATDEnemyShip* ATDEnemyAIController::GetEnemyShip() const
{
	return Cast<ATDEnemyShip>(GetPawn());
}

void ATDEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (InPawn)
	{
		Rng.Initialize(static_cast<int32>(InPawn->GetUniqueID()));
	}
	bPrefersPlayer = Rng.FRand() < PreferPlayerShipChance;
	SetState(ETDEnemyAIState::Approach);
	RetargetTimer = 0.f;
}

void ATDEnemyAIController::OnUnPossess()
{
	if (ATDEnemyShip* Ship = GetEnemyShip())
	{
		Ship->SetGunsFiring(false, nullptr);
	}
	Super::OnUnPossess();
}

void ATDEnemyAIController::SetState(ETDEnemyAIState NewState)
{
	if (State != NewState)
	{
		State = NewState;
		StateTime = 0.f;
		bRunReachedGunRange = false;
	}
}

void ATDEnemyAIController::SetTarget(AActor* NewTarget)
{
	const ATDEnemyShip* Ship = GetEnemyShip();
	if (Ship && Ship->IsTrackingLost())
	{
		return;
	}
	Target = NewTarget;
	RetargetTimer = RetargetInterval;
}

void ATDEnemyAIController::ForgetTarget()
{
	Target.Reset();
	RetargetTimer = 0.f;
	if (ATDEnemyShip* Ship = GetEnemyShip())
	{
		Ship->SetGunsFiring(false, nullptr);
	}
	if (State == ETDEnemyAIState::AttackRun || State == ETDEnemyAIState::Approach)
	{
		SetState(ETDEnemyAIState::Idle);
	}
}

void ATDEnemyAIController::SetFormationLeader(ATDEnemyShip* Leader, FVector LocalOffset)
{
	if (Leader == GetPawn())
	{
		Leader = nullptr;
	}
	FormationLeader = Leader;
	FormationOffset = LocalOffset;
	SetState(Leader ? ETDEnemyAIState::Formation : ETDEnemyAIState::Approach);
}

// --------------------------------------------------------------------------------------- targeting

bool ATDEnemyAIController::IsValidTarget(const AActor* Candidate) const
{
	if (!IsValid(Candidate))
	{
		return false;
	}
	const UTDHealthComponent* H = UTDStatics::GetHealth(Candidate);
	if (H && (!H->IsAlive() || !H->bTargetable))
	{
		return false;	// dead or cloaked
	}
	return UTDStatics::AreHostile(GetPawn(), Candidate);
}

AActor* ATDEnemyAIController::ChooseTarget() const
{
	const ATDEnemyShip* Ship = GetEnemyShip();
	if (!Ship)
	{
		return nullptr;
	}

	// Interceptors hunt the ship the player is flying.
	if (Ship->GetRole() == ETDEnemyRole::Interceptor)
	{
		APawn* PlayerShip = UGameplayStatics::GetPlayerPawn(this, 0);
		if (IsValidTarget(PlayerShip) && FVector::DistSquared(PlayerShip->GetActorLocation(), Ship->GetActorLocation()) <= FMath::Square(AcquireRange * 1.5f))
		{
			return PlayerShip;
		}
	}

	// Spread the pressure: about half the pilots prefer the ship the player is flying (when it's in
	// range), the rest take the nearest squad ship, so wingmen get attacked too.
	if (bPrefersPlayer)
	{
		APawn* PlayerShip = UGameplayStatics::GetPlayerPawn(this, 0);
		if (IsValidTarget(PlayerShip) && FVector::DistSquared(PlayerShip->GetActorLocation(), Ship->GetActorLocation()) <= FMath::Square(AcquireRange))
		{
			return PlayerShip;
		}
	}

	// Stick with the current target unless it's gone or drifted far out.
	if (IsValidTarget(Target.Get()) && FVector::DistSquared(Target->GetActorLocation(), Ship->GetActorLocation()) <= FMath::Square(AcquireRange * 1.2f))
	{
		return Target.Get();
	}

	// Any squad ship (wingmen too), nearest first.
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (AActor* Near = Registry ? Registry->FindNearestHostile(ETDTeam::Enemy, Ship->GetActorLocation(), AcquireRange, Ship) : nullptr)
	{
		return Near;
	}

	// Leash: nothing in range -> go after the player's ship wherever it is, so no wave stalls.
	APawn* PlayerShip = UGameplayStatics::GetPlayerPawn(this, 0);
	if (IsValidTarget(PlayerShip))
	{
		return PlayerShip;
	}
	return Registry ? Registry->FindNearestHostile(ETDTeam::Enemy, Ship->GetActorLocation(), 0.f, Ship) : nullptr;
}

float ATDEnemyAIController::TimeToImpact(const AActor& A, const AActor& B)
{
	const FVector RelPos = B.GetActorLocation() - A.GetActorLocation();
	const FVector RelVel = B.GetVelocity() - A.GetVelocity();
	const float Dist = RelPos.Size();
	if (Dist < KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	const float Closing = -FVector::DotProduct(RelPos / Dist, RelVel);
	return Closing > KINDA_SMALL_NUMBER ? Dist / Closing : TNumericLimits<float>::Max();
}

void ATDEnemyAIController::UpdateCatchUp(ATDEnemyShip& Ship)
{
	bool bWant = false;
	if (Ship.GetRole() != ETDEnemyRole::Carrier && !Ship.IsStunned())
	{
		if (ATDEnemyShip* Leader = FormationLeader.Get())
		{
			// Followers must out-run a boosting / catching-up leader to hold the slot.
			const FVector Slot = Leader->GetActorTransform().TransformPositionNoScale(FormationOffset);
			const float Off = FVector::Dist(Slot, Ship.GetActorLocation());
			bWant = bCatchingUp ? Off > 800.f : Off > 2000.f;
		}
		else if (const AActor* T = Target.Get())
		{
			// Hysteresis: start beyond CatchUpDistance, keep going until inside gun range, otherwise a
			// pilot slower than its target hovers at the leash edge forever.
			const FTDEnemySpec& Spec = Ship.GetSpec();
			const float GunRange = Spec.Guns.IsValid() ? Spec.Guns.Range : (Spec.Heavy.IsValid() ? Spec.Heavy.Range : Spec.EngageRange);
			const float Dist = FVector::Dist(T->GetActorLocation(), Ship.GetActorLocation());
			bWant = bCatchingUp ? Dist > GunRange * 0.8f : Dist > CatchUpDistance;
		}
	}
	if (bWant && !bCatchingUp)
	{
		const float Mult = FormationLeader.IsValid() ? CatchUpSpeedMultiplier * 1.3f : CatchUpSpeedMultiplier;
		Ship.SetSpeedOverrideFor(TDEnemyAIOverride, Ship.GetSpec().CruiseSpeed * Mult, 1.f);
		bCatchingUp = true;
	}
	else if (!bWant && bCatchingUp)
	{
		Ship.ClearSpeedOverrideFor(TDEnemyAIOverride);
		bCatchingUp = false;
	}
}

void ATDEnemyAIController::UpdateTarget(float DeltaSeconds)
{
	const ATDEnemyShip* Ship = GetEnemyShip();
	if (!Ship || Ship->IsTrackingLost())
	{
		Target.Reset();
		return;
	}
	if (Target.IsValid() && !IsValidTarget(Target.Get()))
	{
		Target.Reset();
		RetargetTimer = 0.f;	// lost a real target: look again now (once)
	}
	RetargetTimer -= DeltaSeconds;
	if (RetargetTimer <= 0.f)
	{
		RetargetTimer = RetargetInterval * (0.8f + 0.4f * Rng.FRand());	// de-sync packs
		Target = ChooseTarget();
	}
}

// ---------------------------------------------------------------------------------------- steering

FVector2D ATDEnemyAIController::ComputeSteer(const FTransform& Ship, const FVector& WorldPoint, float FullDeflectionDeg)
{
	const FVector Local = Ship.InverseTransformVectorNoScale(WorldPoint - Ship.GetLocation());
	if (Local.IsNearlyZero())
	{
		return FVector2D::ZeroVector;
	}
	const float Full = FMath::Max(1.f, FullDeflectionDeg);
	float YawDeg = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
	float PitchDeg = FMath::RadiansToDegrees(FMath::Atan2(Local.Z, FMath::Max(KINDA_SMALL_NUMBER, FVector2D(Local.X, Local.Y).Size())));
	// Target behind: commit to a full yaw turn rather than dithering around 180.
	if (Local.X < 0.f && FMath::Abs(YawDeg) > 150.f)
	{
		YawDeg = 180.f * (YawDeg >= 0.f ? 1.f : -1.f);
	}
	return FVector2D(FMath::Clamp(YawDeg / Full, -1.f, 1.f), FMath::Clamp(PitchDeg / Full, -1.f, 1.f));
}

void ATDEnemyAIController::SteerToward(ATDEnemyShip& Ship, const FVector& WorldPoint, float Throttle01, bool bBoost)
{
	FVector Goal = WorldPoint;

	// Obstacle avoidance: probe ahead along the velocity (or nose) and slide around asteroids,
	// ruins, planets and the Dominion hull. Targets beyond the obstacle stay the goal once clear.
	bAvoiding = false;
	if (const UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
	{
		if (WorldSys->GetObstacleCount() > 0)
		{
			const FVector Vel = Ship.GetVelocity();
			const FVector Dir = Vel.SizeSquared() > 1.f ? Vel.GetSafeNormal() : Ship.GetActorForwardVector();
			const float Probe = Vel.Size() * AvoidLookAheadSeconds + Ship.GetSpec().Radius * 4.f;
			const float ToGoal = FVector::Dist(WorldPoint, Ship.GetActorLocation());
			FVector Avoid;
			if (WorldSys->FindAvoidance(Ship.GetActorLocation(), Dir, FMath::Min(Probe, ToGoal), Ship.GetSpec().Radius * 1.5f, Avoid))
			{
				Goal = Ship.GetActorLocation() + Avoid * 10000.f;
				bAvoiding = true;
			}
		}
	}

	// Separation: don't stack on other Armada ships (packs spread, no bumping).
	if (!bAvoiding && SeparationRadius > 0.f && !FormationLeader.IsValid())	// packs keep their own spacing
	{
		if (const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			FVector Push = FVector::ZeroVector;
			const FVector Me = Ship.GetActorLocation();
			for (const TWeakObjectPtr<AActor>& Weak : Registry->GetAll())
			{
				const AActor* Other = Weak.Get();
				const ATDEnemyShip* OtherShip = Cast<ATDEnemyShip>(Other);
				if (!OtherShip || OtherShip == &Ship) { continue; }
				const ATDEnemyAIController* OtherAI = Cast<ATDEnemyAIController>(OtherShip->GetController());
				if (OtherAI && OtherAI->GetFormationLeader() == &Ship) { continue; }	// my own wingmen
				const FVector Away = Me - Other->GetActorLocation();
				const float D = Away.Size();
				if (D > KINDA_SMALL_NUMBER && D < SeparationRadius)
				{
					Push += Away / D * (1.f - D / SeparationRadius);
				}
			}
			if (!Push.IsNearlyZero())
			{
				const float ToGoal = FMath::Max(1000.f, FVector::Dist(Goal, Me));
				Goal += Push.GetClampedToMaxSize(1.f) * ToGoal * 0.5f;
			}
		}
	}

	Ship.SetFlightInput(TDEnemyAI_MakeInput(ComputeSteer(Ship.GetActorTransform(), Goal, SteerFullDeflectionDeg), Throttle01, bBoost && !bAvoiding));
}

FVector ATDEnemyAIController::GetAimPoint(const ATDEnemyShip& Ship, const AActor& InTarget) const
{
	const FVector To = InTarget.GetActorLocation() - Ship.GetActorLocation();
	const float Lead = FMath::Clamp(To.Size() / 30000.f, 0.f, 1.5f);
	return InTarget.GetActorLocation() + InTarget.GetVelocity() * Lead;
}

FVector ATDEnemyAIController::GetBomberRoutePoint(const AActor& InTarget) const
{
	// Lay the trail across where the squad is heading.
	const FVector Vel = InTarget.GetVelocity();
	const FVector Heading = Vel.IsNearlyZero() ? InTarget.GetActorForwardVector() : Vel.GetSafeNormal();
	return InTarget.GetActorLocation() + Heading * BomberLeadDistance;
}

void ATDEnemyAIController::TryHeavyAttacks(ATDEnemyShip& Ship, AActor& InTarget)
{
	const FTDEnemySpec& Spec = Ship.GetSpec();
	const FVector To = InTarget.GetActorLocation() - Ship.GetActorLocation();
	const float Dist = To.Size();
	const FVector Dir = Dist > KINDA_SMALL_NUMBER ? To / Dist : Ship.GetActorForwardVector();
	const float Dot = FVector::DotProduct(Dir, Ship.GetActorForwardVector());

	for (const FTDEnemyAttackSpec* A : { &Spec.Heavy, &Spec.Heavy2 })
	{
		if (!A->IsValid() || A->Kind == ETDEnemyAttack::MineTrail || A->Kind == ETDEnemyAttack::LaunchFighters)
		{
			continue;
		}
		if (Dist <= A->Range && Dot >= FMath::Cos(FMath::DegreesToRadians(A->ConeHalfAngleDeg)) && Ship.CanStartAttack(A->Kind))
		{
			if (Ship.TryStartAttack(A->Kind, &InTarget))
			{
				return;	// one big attack at a time
			}
		}
	}
}

// ------------------------------------------------------------------------------------------- states

void ATDEnemyAIController::TickIdle(ATDEnemyShip& Ship, float DeltaSeconds)
{
	Ship.SetGunsFiring(false, nullptr);
	// Cruise in a lazy arc so idle ships don't fly off forever.
	FTDFlightInput In = TDEnemyAI_MakeInput(FVector2D(0.15f, 0.f), 0.4f, false);
	Ship.SetFlightInput(In);
	if (Target.IsValid())
	{
		SetState(FormationLeader.IsValid() ? ETDEnemyAIState::Formation : ETDEnemyAIState::Approach);
	}
}

void ATDEnemyAIController::TickFormation(ATDEnemyShip& Ship, float DeltaSeconds)
{
	ATDEnemyShip* Leader = FormationLeader.Get();
	if (!Leader || Leader->IsDying())
	{
		FormationLeader.Reset();
		SetState(ETDEnemyAIState::Approach);
		return;
	}

	const FTransform LeaderXf = Leader->GetActorTransform();
	const FVector Slot = LeaderXf.TransformPositionNoScale(FormationOffset);
	const FVector Ahead = Slot + LeaderXf.GetUnitAxis(EAxis::X) * 2500.f;
	const FVector ToSlot = Slot - Ship.GetActorLocation();
	const float Along = FVector::DotProduct(ToSlot, LeaderXf.GetUnitAxis(EAxis::X));	// + = slot is ahead of us
	const float Dist = ToSlot.Size();

	// Catch up when behind, ease off when ahead.
	const float Throttle = FMath::Clamp(0.75f + Along / 3000.f, 0.f, 1.f);
	const bool bBoost = Along > 4000.f;
	SteerToward(Ship, Dist > 3000.f ? Slot : Ahead, Throttle, bBoost);

	// Fire with the leader, at our own target (packs strafe together).
	AActor* T = Target.Get();
	const bool bFire = Leader->AreGunsFiring() && T;
	if (bFire)
	{
		const FVector To = (T->GetActorLocation() - Ship.GetActorLocation()).GetSafeNormal();
		const bool bLinedUp = FVector::DotProduct(To, Ship.GetActorForwardVector()) >= FMath::Cos(FMath::DegreesToRadians(Ship.GetSpec().Guns.ConeHalfAngleDeg * 1.5f));
		Ship.SetGunsFiring(bLinedUp, T);
	}
	else
	{
		Ship.SetGunsFiring(false, nullptr);
	}
}

void ATDEnemyAIController::TickApproach(ATDEnemyShip& Ship, float DeltaSeconds)
{
	AActor* T = Target.Get();
	if (!T)
	{
		SetState(ETDEnemyAIState::Idle);
		return;
	}
	const FTDEnemySpec& Spec = Ship.GetSpec();
	const float Dist = FVector::Dist(T->GetActorLocation(), Ship.GetActorLocation());

	Ship.SetGunsFiring(false, nullptr);
	SteerToward(Ship, GetAimPoint(Ship, *T), 1.f, Dist > Spec.EngageRange * 2.f);

	if (Dist <= Spec.EngageRange)
	{
		SetState(ETDEnemyAIState::AttackRun);
	}
}

void ATDEnemyAIController::TickAttackRun(ATDEnemyShip& Ship, float DeltaSeconds)
{
	AActor* T = Target.Get();
	if (!T)
	{
		SetState(ETDEnemyAIState::Idle);
		return;
	}
	const FTDEnemySpec& Spec = Ship.GetSpec();
	const FVector Aim = GetAimPoint(Ship, *T);
	const FVector To = Aim - Ship.GetActorLocation();
	const float Dist = To.Size();

	SteerToward(Ship, Aim, 0.85f, false);

	// Guns: only when lined up and in range, so shots are readable and dodgeable.
	if (Spec.Guns.IsValid())
	{
		const bool bInCone = FVector::DotProduct(To.GetSafeNormal(), Ship.GetActorForwardVector()) >= FMath::Cos(FMath::DegreesToRadians(Spec.Guns.ConeHalfAngleDeg));
		Ship.SetGunsFiring(bInCone && Dist <= Spec.Guns.Range, T);
	}
	TryHeavyAttacks(Ship, *T);

	const float GunRange = Spec.Guns.IsValid() ? Spec.Guns.Range : (Spec.Heavy.IsValid() ? Spec.Heavy.Range : Spec.EngageRange);
	if (!bRunReachedGunRange && Dist <= GunRange)
	{
		bRunReachedGunRange = true;
		StateTime = 0.f;	// the run's clock starts once we're actually shooting
	}
	// Peel away before a collision: time-to-impact, not a fixed distance (head-on closing is ~13000 cm/s).
	const bool bTooClose = Dist < Spec.BreakOffRange || TimeToImpact(Ship, *T) < BreakOffTimeToImpact;
	const bool bTooLong = bRunReachedGunRange && Spec.AttackRunMaxSeconds > 0.f && StateTime > Spec.AttackRunMaxSeconds;
	if ((bTooClose || bTooLong) && !Ship.IsTelegraphing())
	{
		// Peel away: mostly sideways/up, away from the target, so the run reads as a pass.
		const FVector Away = (Ship.GetActorLocation() - T->GetActorLocation()).GetSafeNormal();
		const FVector Side = Rng.FRand() < 0.5f ? Ship.GetActorRightVector() : -Ship.GetActorRightVector();
		BreakOffDirection = (Side + Away * 0.6f + Ship.GetActorUpVector() * (Rng.FRand() - 0.5f)).GetSafeNormal();
		Ship.SetGunsFiring(false, nullptr);
		SetState(ETDEnemyAIState::BreakOff);
	}
}

void ATDEnemyAIController::TickBreakOff(ATDEnemyShip& Ship, float DeltaSeconds)
{
	Ship.SetGunsFiring(false, nullptr);
	SteerToward(Ship, Ship.GetActorLocation() + BreakOffDirection * 20000.f, 1.f, true);
	if (StateTime >= Ship.GetSpec().BreakOffSeconds)
	{
		SetState(Target.IsValid() ? ETDEnemyAIState::Approach : ETDEnemyAIState::Idle);	// re-approach
	}
}

void ATDEnemyAIController::TickGunship(ATDEnemyShip& Ship, AActor& InTarget, float DeltaSeconds)
{
	// Gunships face their target and hold range: the frontal shield is always toward the threat.
	const FTDEnemySpec& Spec = Ship.GetSpec();
	const float Dist = FVector::Dist(InTarget.GetActorLocation(), Ship.GetActorLocation());
	const float Hold = Spec.EngageRange * GunshipHoldRangeFraction;
	const float Throttle = Dist > Spec.EngageRange ? 1.f : (Dist > Hold ? 0.5f : 0.f);
	SteerToward(Ship, GetAimPoint(Ship, InTarget), Throttle, false);
	SetState(Dist <= Spec.EngageRange ? ETDEnemyAIState::AttackRun : ETDEnemyAIState::Approach);
	if (State == ETDEnemyAIState::AttackRun)
	{
		TryHeavyAttacks(Ship, InTarget);
	}
}

void ATDEnemyAIController::TickBomber(ATDEnemyShip& Ship, AActor& InTarget, float DeltaSeconds)
{
	const FTDEnemySpec& Spec = Ship.GetSpec();
	if (State == ETDEnemyAIState::BreakOff)
	{
		TickBreakOff(Ship, DeltaSeconds);
		return;
	}

	const FVector Route = GetBomberRoutePoint(InTarget);
	const float Dist = FVector::Dist(Route, Ship.GetActorLocation());
	SteerToward(Ship, Route, 1.f, Dist > Spec.EngageRange * 2.f);
	SetState(Dist <= Spec.Heavy.Range ? ETDEnemyAIState::AttackRun : ETDEnemyAIState::Approach);

	if (State == ETDEnemyAIState::AttackRun && Ship.TryStartAttack(ETDEnemyAttack::MineTrail, &InTarget))
	{
		// Keep flying through the route while the trail drops, then swing round.
		BreakOffDirection = (Ship.GetActorForwardVector() + Ship.GetActorRightVector() * (Rng.FRand() < 0.5f ? 1.f : -1.f)).GetSafeNormal();
	}
	else if (State == ETDEnemyAIState::AttackRun && !Ship.IsTelegraphing() && Ship.GetAttackCooldownRemaining(ETDEnemyAttack::MineTrail) > 0.f && StateTime > 1.f)
	{
		SetState(ETDEnemyAIState::BreakOff);
	}
}

void ATDEnemyAIController::TickCarrier(ATDEnemyShip& Ship, AActor& InTarget, float DeltaSeconds)
{
	// Hold station at engage range and turn broadside so bays and turrets cover the fight.
	const FTDEnemySpec& Spec = Ship.GetSpec();
	const FVector To = InTarget.GetActorLocation() - Ship.GetActorLocation();
	const float Dist = To.Size();
	if (Dist > Spec.EngageRange)
	{
		SteerToward(Ship, InTarget.GetActorLocation(), 1.f, false);
		SetState(ETDEnemyAIState::Approach);
	}
	else
	{
		const FVector Broadside = FVector::CrossProduct(To.GetSafeNormal(), FVector::UpVector).GetSafeNormal();
		SteerToward(Ship, Ship.GetActorLocation() + Broadside * 10000.f, 0.f, false);
		SetState(ETDEnemyAIState::AttackRun);
	}
}

// --------------------------------------------------------------------------------------------- tick

void ATDEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ATDEnemyShip* Ship = GetEnemyShip();
	if (!Ship || Ship->IsDying())
	{
		return;
	}
	StateTime += DeltaSeconds;

	if (Ship->IsStunned())
	{
		SetState(ETDEnemyAIState::Stunned);
		Ship->SetGunsFiring(false, nullptr);
		UpdateCatchUp(*Ship);	// drops any catch-up override
		return;	// the ship holds a dead stick itself
	}
	if (State == ETDEnemyAIState::Stunned)
	{
		SetState(FormationLeader.IsValid() ? ETDEnemyAIState::Formation : ETDEnemyAIState::Approach);
	}

	UpdateTarget(DeltaSeconds);
	UpdateCatchUp(*Ship);

	AActor* T = Target.Get();
	const ETDEnemyRole ShipRole = Ship->GetRole();

	if (State == ETDEnemyAIState::Formation)
	{
		TickFormation(*Ship, DeltaSeconds);
	}
	else if (!T)
	{
		SetState(ETDEnemyAIState::Idle);
		TickIdle(*Ship, DeltaSeconds);
	}
	else if (ShipRole == ETDEnemyRole::Gunship)
	{
		TickGunship(*Ship, *T, DeltaSeconds);
	}
	else if (ShipRole == ETDEnemyRole::Bomber)
	{
		TickBomber(*Ship, *T, DeltaSeconds);
	}
	else if (ShipRole == ETDEnemyRole::Carrier)
	{
		TickCarrier(*Ship, *T, DeltaSeconds);
	}
	else
	{
		switch (State)
		{
		case ETDEnemyAIState::Idle:
		case ETDEnemyAIState::Approach:  TickApproach(*Ship, DeltaSeconds); break;
		case ETDEnemyAIState::AttackRun: TickAttackRun(*Ship, DeltaSeconds); break;
		case ETDEnemyAIState::BreakOff:  TickBreakOff(*Ship, DeltaSeconds); break;
		default:                         TickApproach(*Ship, DeltaSeconds); break;
		}
	}

#if ENABLE_DRAW_DEBUG
	if ((bDebugDraw || GTDEnemyAIDebug) && GetWorld())
	{
		static const TCHAR* Names[] = { TEXT("Idle"), TEXT("Formation"), TEXT("Approach"), TEXT("AttackRun"), TEXT("BreakOff"), TEXT("Stunned") };
		const FColor Col = Ship->IsTelegraphing() ? FColor::Yellow : FColor::Red;
		DrawDebugString(GetWorld(), Ship->GetActorLocation() + FVector(0, 0, Ship->GetSpec().Radius + 100.f), Names[static_cast<uint8>(State)], nullptr, Col, 0.f, true);
		if (T)
		{
			DrawDebugLine(GetWorld(), Ship->GetActorLocation(), T->GetActorLocation(), Col, false, 0.f, 0, 4.f);
		}
	}
#endif
}

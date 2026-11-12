// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Enemies/TDEnemyAIController.h"

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"

#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	int32 GTDEnemyAIDebug = 0;
	FAutoConsoleVariableRef CVarTDEnemyAIDebug(TEXT("TD.EnemyAIDebug"), GTDEnemyAIDebug, TEXT("Draw enemy AI state and target lines (1 = on)."));

	FTDFlightInput MakeInput(const FVector2D& Aim, float Throttle01, bool bBoost)
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

	// Stick with the current target unless it's gone or drifted far out.
	if (IsValidTarget(Target.Get()) && FVector::DistSquared(Target->GetActorLocation(), Ship->GetActorLocation()) <= FMath::Square(AcquireRange * 1.2f))
	{
		return Target.Get();
	}

	// Any squad ship (wingmen too), nearest first.
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	return Registry ? Registry->FindNearestHostile(ETDTeam::Enemy, Ship->GetActorLocation(), AcquireRange, Ship) : nullptr;
}

void ATDEnemyAIController::UpdateTarget(float DeltaSeconds)
{
	const ATDEnemyShip* Ship = GetEnemyShip();
	if (!Ship || Ship->IsTrackingLost())
	{
		Target.Reset();
		return;
	}
	if (!IsValidTarget(Target.Get()))
	{
		Target.Reset();
		RetargetTimer = 0.f;
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
	Ship.SetFlightInput(MakeInput(ComputeSteer(Ship.GetActorTransform(), WorldPoint, SteerFullDeflectionDeg), Throttle01, bBoost));
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
	FTDFlightInput In = MakeInput(FVector2D(0.15f, 0.f), 0.4f, false);
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

	const bool bTooClose = Dist < Spec.BreakOffRange;
	const bool bTooLong = Spec.AttackRunMaxSeconds > 0.f && StateTime > Spec.AttackRunMaxSeconds;
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
		return;	// the ship holds a dead stick itself
	}
	if (State == ETDEnemyAIState::Stunned)
	{
		SetState(FormationLeader.IsValid() ? ETDEnemyAIState::Formation : ETDEnemyAIState::Approach);
	}

	UpdateTarget(DeltaSeconds);

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

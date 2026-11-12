// Triad Drift - crew AI (ENG-23). Owner: Teammate 5.

#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace TDCrewAI
{
	/** Grunts without an attack token circle at this distance until one frees up. */
	static constexpr float WaitingRingCm = 330.f;
}

ATDCrewAIController::ATDCrewAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = false;
}

void ATDCrewAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Home = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;
	PatrolPoint = Home;
	// De-sync packs: each member flanks to a different side and senses on a different beat.
	FlankSign = (GetUniqueID() & 1) ? 1.f : -1.f;
	StrafeSign = (GetUniqueID() & 2) ? 1.f : -1.f;
	SenseTimer = 0.03f * (GetUniqueID() % 7);
	if (ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(InPawn))
	{
		FinishedHandle = Crew->OnAttackFinished.AddUObject(this, &ATDCrewAIController::HandleAttackFinished);
	}
	State = PatrolRadius > 0.f ? ETDCrewState::Patrol : ETDCrewState::Idle;
}

void ATDCrewAIController::OnUnPossess()
{
	if (ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(GetPawn()))
	{
		Crew->OnAttackFinished.Remove(FinishedHandle);
		if (UTDCrewTokenSubsystem* Tokens = UTDCrewTokenSubsystem::Get(this))
		{
			Tokens->Release(Crew);
		}
	}
	Super::OnUnPossess();
}

void ATDCrewAIController::HandleAttackFinished(ATDCrewCharacter* Crew, ETDCrewAttack Attack)
{
	if (UTDCrewTokenSubsystem* Tokens = UTDCrewTokenSubsystem::Get(this))
	{
		Tokens->Release(Crew);
	}
}

void ATDCrewAIController::SetTarget(AActor* NewTarget)
{
	Target = NewTarget;
	if (NewTarget && (State == ETDCrewState::Idle || State == ETDCrewState::Patrol))
	{
		const ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(GetPawn());
		State = ETDCrewState::Alert;
		AlertLeft = Crew ? Crew->Spec.AlertSeconds : 0.5f;
	}
}

bool ATDCrewAIController::HasLineOfSight(const AActor* To) const
{
	const APawn* P = GetPawn();
	if (!P || !To)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDCrewLOS), false, P);
	Params.AddIgnoredActor(To);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, P->GetActorLocation() + FVector(0, 0, 40), To->GetActorLocation(), ECC_Visibility, Params);
}

void ATDCrewAIController::Sense()
{
	ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(GetPawn());
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Crew || !Registry)
	{
		return;
	}
	AActor* Current = Target.Get();
	const UTDHealthComponent* TH = Current ? UTDStatics::GetHealth(Current) : nullptr;
	if (!Current || !TH || TH->IsDead())
	{
		Current = Registry->FindNearestHostile(ETDTeam::Enemy, Crew->GetActorLocation(), Crew->Spec.SightRange, Crew);
		if (Current && !HasLineOfSight(Current))
		{
			Current = nullptr; // not seen yet
		}
		if (Current)
		{
			SetTarget(Current);
		}
		else
		{
			Target = nullptr;
		}
	}
	bHasLOS = Current && HasLineOfSight(Current);
}

void ATDCrewAIController::Steer(ATDCrewCharacter* Crew, const FVector& WorldDir, float Scale)
{
	if (!WorldDir.IsNearlyZero() && Scale > 0.f)
	{
		Crew->AddMovementInput(WorldDir.GetSafeNormal(), FMath::Clamp(Scale, 0.f, 1.f));
	}
}

void ATDCrewAIController::TickPatrol(ATDCrewCharacter* Crew, float DeltaSeconds)
{
	if (PatrolRadius <= 0.f)
	{
		return;
	}
	PatrolTimer -= DeltaSeconds;
	const FVector To = PatrolPoint - Crew->GetActorLocation();
	if (PatrolTimer <= 0.f || To.Size2D() < 60.f)
	{
		PatrolTimer = 3.f;
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		PatrolPoint = Home + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * FMath::FRandRange(0.3f, 1.f) * PatrolRadius;
	}
	Steer(Crew, FVector(To.X, To.Y, 0.f), 0.35f);
}

void ATDCrewAIController::ChooseAttack(ATDCrewCharacter* Crew, float Distance)
{
	if (Crew->IsAttacking() || Crew->IsStaggered())
	{
		return;
	}
	AActor* T = Target.Get();
	const FTDCrewSpec& S = Crew->Spec;
	const float TargetR = 40.f;
	switch (S.Type)
	{
	case ETDCrewType::Grunt:
		if (Distance <= S.MeleeRange + TargetR && Crew->GetCooldownLeft(ETDCrewAttack::Melee) <= 0.f)
		{
			UTDCrewTokenSubsystem* Tokens = UTDCrewTokenSubsystem::Get(this);
			if (!S.bNeedsAttackToken || (Tokens && Tokens->TryAcquire(Crew)))
			{
				if (!Crew->StartAttack(ETDCrewAttack::Melee, T) && Tokens)
				{
					Tokens->Release(Crew);
				}
			}
		}
		break;
	case ETDCrewType::Trooper:
	case ETDCrewType::Drone:
		if (bHasLOS && Distance <= S.RangedMaxRange)
		{
			Crew->StartAttack(ETDCrewAttack::Burst, T);
		}
		break;
	case ETDCrewType::Heavy:
		if (Distance <= S.MeleeRange + TargetR)
		{
			Crew->StartAttack(ETDCrewAttack::Melee, T);
		}
		else if (bHasLOS && Distance >= S.ShotgunMinRange && Distance <= S.ShotgunMaxRange)
		{
			Crew->StartAttack(ETDCrewAttack::Shotgun, T);
		}
		break;
	case ETDCrewType::Warden:
	{
		// Cycle: fan, charge (or fan up close), summon when there's room.
		const int32 Step = BossCycle % 3;
		bool bStarted = false;
		if (Step == 2 && Crew->GetAliveSummons() < S.SummonMaxAlive)
		{
			bStarted = Crew->StartAttack(ETDCrewAttack::Summon, T);
		}
		if (!bStarted && Step >= 1 && Distance > 450.f)
		{
			bStarted = Crew->StartAttack(ETDCrewAttack::ChargeSlam, T);
		}
		if (!bStarted && bHasLOS)
		{
			bStarted = Crew->StartAttack(ETDCrewAttack::BoltFan, T);
		}
		if (!bStarted && Distance <= S.ChargeSlamRadius)
		{
			bStarted = Crew->StartAttack(ETDCrewAttack::ChargeSlam, T);
		}
		if (bStarted)
		{
			++BossCycle;
		}
		break;
	}
	default:
		break;
	}
}

void ATDCrewAIController::TickEngage(ATDCrewCharacter* Crew, float DeltaSeconds)
{
	AActor* T = Target.Get();
	if (!T)
	{
		State = PatrolRadius > 0.f ? ETDCrewState::Patrol : ETDCrewState::Idle;
		return;
	}
	const FTDCrewSpec& S = Crew->Spec;
	FVector To = T->GetActorLocation() - Crew->GetActorLocation();
	To.Z = 0.f;
	const float Distance = To.Size();
	const FVector Dir = To.GetSafeNormal();
	SetControlRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));

	// Drones hold their hover height above whatever is below them.
	if (S.bFlying)
	{
		FHitResult Down;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TDCrewHover), false, Crew);
		const FVector From = Crew->GetActorLocation();
		const float GroundZ = GetWorld()->LineTraceSingleByChannel(Down, From, From - FVector(0, 0, 5000.f), ECC_Visibility, Params) ? Down.ImpactPoint.Z : From.Z - S.HoverHeight;
		const float Err = (GroundZ + S.HoverHeight) - From.Z;
		Crew->AddMovementInput(FVector(0.f, 0.f, FMath::Sign(Err)), FMath::Clamp(FMath::Abs(Err) / 150.f, 0.f, 1.f));
	}

	if (!Crew->IsAttacking() && !Crew->IsCharging())
	{
		const UTDCrewTokenSubsystem* Tokens = UTDCrewTokenSubsystem::Get(this);
		const bool bWaitingForToken = S.bNeedsAttackToken && Tokens && !Tokens->Holds(Crew) && Tokens->GetActiveCount() >= Tokens->MaxMeleeAttackers;
		const float Min = bWaitingForToken ? TDCrewAI::WaitingRingCm - 40.f : S.PreferredRangeMin;
		const float Max = bWaitingForToken ? TDCrewAI::WaitingRingCm + 40.f : S.PreferredRangeMax;

		StrafeTimer -= DeltaSeconds;
		if (StrafeTimer <= 0.f)
		{
			StrafeTimer = 2.f;
			StrafeSign = -StrafeSign;
		}
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir) * StrafeSign;
		if (Distance > Max)
		{
			// Close in; grunts swing out to the side while still far (flank).
			FVector Approach = Dir;
			if (S.FlankAngle > 0.f && Distance > 350.f)
			{
				Approach = Dir.RotateAngleAxis(S.FlankAngle * FlankSign, FVector::UpVector);
			}
			Steer(Crew, Approach, 1.f);
		}
		else if (Distance < Min)
		{
			Steer(Crew, -Dir + Side * 0.3f, 1.f);
		}
		else if (S.StrafeScale > 0.f || bWaitingForToken)
		{
			Steer(Crew, Side, bWaitingForToken ? 0.5f : S.StrafeScale);
		}
	}
	ChooseAttack(Crew, Distance);
}

void ATDCrewAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(GetPawn());
	if (!Crew || DeltaSeconds <= 0.f)
	{
		return;
	}
	if (Crew->IsDead())
	{
		State = ETDCrewState::Dead;
		return;
	}
	SenseTimer -= DeltaSeconds;
	if (SenseTimer <= 0.f)
	{
		SenseTimer = SenseInterval;
		Sense();
	}
	if (Crew->IsStaggered())
	{
		State = ETDCrewState::Stagger;
		return;
	}
	if (State == ETDCrewState::Stagger)
	{
		State = Target.IsValid() ? ETDCrewState::Engage : ETDCrewState::Idle;
	}

	switch (State)
	{
	case ETDCrewState::Idle:
	case ETDCrewState::Patrol:
		TickPatrol(Crew, DeltaSeconds);
		break;
	case ETDCrewState::Alert:
		if (AActor* T = Target.Get())
		{
			SetControlRotation(FRotator(0.f, (T->GetActorLocation() - Crew->GetActorLocation()).Rotation().Yaw, 0.f));
		}
		AlertLeft -= DeltaSeconds;
		if (AlertLeft <= 0.f)
		{
			State = ETDCrewState::Engage;
		}
		break;
	case ETDCrewState::Engage:
		TickEngage(Crew, DeltaSeconds);
		break;
	default:
		break;
	}
}

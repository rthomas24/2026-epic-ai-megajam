// Triad Drift - crew AI (ENG-23). Owner: Teammate 5.

#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewCover.h"
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
	LeaveCover(false);
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
	if (Current && Current->IsHidden())
	{
		Current = nullptr; // the pilot boarded (stowed, hidden): find someone else or stand down
		Target = nullptr;
	}
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
	if (WorldDir.IsNearlyZero() || Scale <= 0.f)
	{
		return;
	}
	FVector Dir = WorldDir.GetSafeNormal();
	// Never walk off a ledge: try the way we want, then sideways, else hold still.
	if (!Crew->HasGroundAhead(Dir, 80.f))
	{
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir) * StrafeSign;
		if (Crew->HasGroundAhead(Side, 80.f))
		{
			Dir = Side;
		}
		else if (Crew->HasGroundAhead(-Side, 80.f))
		{
			Dir = -Side;
		}
		else
		{
			return;
		}
	}
	Crew->AddMovementInput(Dir, FMath::Clamp(Scale, 0.f, 1.f));
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
	if (Crew->IsAttacking() || Crew->IsStaggered() || Crew->IsReacting())
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

void ATDCrewAIController::LeaveCover(bool bRelocation)
{
	if (UTDCrewCoverSubsystem* Cover = UTDCrewCoverSubsystem::Get(this))
	{
		Cover->Release(GetPawn());
	}
	if (bRelocation && CoverIndex != INDEX_NONE)
	{
		++CoverRelocations;
	}
	CoverIndex = INDEX_NONE;
	CoverPhase = ECoverPhase::None;
	CoverSearchTimer = 0.f;
}

bool ATDCrewAIController::TickCover(ATDCrewCharacter* Crew, float DeltaSeconds, float Distance)
{
	UTDCrewCoverSubsystem* Cover = UTDCrewCoverSubsystem::Get(this);
	AActor* T = Target.Get();
	if (!Cover || !T || Cover->Num() == 0)
	{
		return false; // no cover in this arena: plain distance-keeping
	}
	const FTDCrewSpec& S = Crew->Spec;

	// Give the spot up when it no longer protects us: cover destroyed, flanked, or the player is on top of us.
	if (CoverIndex != INDEX_NONE)
	{
		const FTDCoverPoint* P = Cover->GetPoint(CoverIndex);
		const bool bGone = !P || !P->IsValid();
		const bool bFlanked = !bGone && CoverPhase != ECoverPhase::Moving && !Cover->IsProtected(CoverIndex, T, Crew);
		const bool bApproached = Distance < S.CoverAbandonDistance;
		if ((bGone || bFlanked || bApproached) && !Crew->IsAttacking())
		{
			LeaveCover(true);
			CoverSearchTimer = bApproached ? 0.8f : 0.f; // when rushed, back off for a moment first
		}
	}

	// Find a spot.
	if (CoverIndex == INDEX_NONE)
	{
		CoverSearchTimer -= DeltaSeconds;
		if (CoverSearchTimer > 0.f)
		{
			return false;
		}
		CoverSearchTimer = 0.5f;
		const int32 Found = Cover->FindCover(Crew, T, S.CoverMaxTravel, FMath::Max(S.CoverAbandonDistance + 100.f, S.PreferredRangeMin * 0.6f), S.PreferredRangeMax * 1.5f);
		const bool bAvoid = Found != INDEX_NONE && Found == BlockedCoverIndex && GetWorld()->GetTimeSeconds() - BlockedCoverTime < 6.f;
		if (Found == INDEX_NONE || bAvoid || !Cover->Claim(Found, Crew))
		{
			return false; // (a spot that just stalled us is skipped for 6 s: fight in the open meanwhile)
		}
		CoverIndex = Found;
		CoverPhase = ECoverPhase::Moving;
		MoveBestDist = TNumericLimits<float>::Max();
		MoveStallTime = 0.f;
		MoveTotalTime = 0.f;
	}

	const FTDCoverPoint* P = Cover->GetPoint(CoverIndex);
	if (!P)
	{
		LeaveCover(false);
		return false;
	}
	const FVector Here = Crew->GetActorLocation();
	switch (CoverPhase)
	{
	case ECoverPhase::Moving:
	{
		const FVector ToPoint = FVector(P->Location.X - Here.X, P->Location.Y - Here.Y, 0.f);
		const float Dist = (float)ToPoint.Size();
		if (Dist < 45.f)
		{
			CoverPhase = ECoverPhase::Hidden;
			CoverTimer = S.CoverHideSeconds;
			return true;
		}
		// Stuck against the cover (or anything) on the way: no real progress for 1 s, or 4 s overall, and
		// it gives the spot up and fights from where it stands; it won't look for cover again for 2 s.
		MoveTotalTime += DeltaSeconds;
		if (Dist < MoveBestDist - 20.f)
		{
			MoveBestDist = Dist;
			MoveStallTime = 0.f;
		}
		else
		{
			MoveStallTime += DeltaSeconds;
		}
		if (MoveStallTime > 1.f || MoveTotalTime > 4.f)
		{
			BlockedCoverIndex = CoverIndex;
			BlockedCoverTime = GetWorld()->GetTimeSeconds();
			LeaveCover(false);
			CoverSearchTimer = 2.f;
			return false; // this tick: plain engage (attacks are chosen)
		}
		Steer(Crew, ToPoint, 1.f);
		return true;
	}
	case ECoverPhase::Hidden:
	{
		const FVector ToPoint = FVector(P->Location.X - Here.X, P->Location.Y - Here.Y, 0.f);
		if (ToPoint.Size() > 30.f)
		{
			Steer(Crew, ToPoint, 0.6f);
		}
		CoverTimer -= DeltaSeconds;
		if (CoverTimer <= 0.f && Crew->GetCooldownLeft(ETDCrewAttack::Burst) <= 0.f)
		{
			if (Cover->FindPeekLocation(CoverIndex, T, Crew, PeekLocation))
			{
				CoverPhase = ECoverPhase::Peeking;
				PeekTime = 0.f;
			}
			else
			{
				CoverTimer = 0.5f;
			}
		}
		return true;
	}
	case ECoverPhase::Peeking:
	{
		const FVector ToPeek = FVector(PeekLocation.X - Here.X, PeekLocation.Y - Here.Y, 0.f);
		if (Crew->IsAttacking())
		{
			return true; // stand and shoot
		}
		// Blocked on the way to the peek spot, or there with no line of sight: after 3 s it gives the spot up.
		if ((PeekTime += DeltaSeconds) > 3.f)
		{
			BlockedCoverIndex = CoverIndex;
			BlockedCoverTime = GetWorld()->GetTimeSeconds();
			LeaveCover(false);
			CoverSearchTimer = 2.f;
			return false;
		}
		if (ToPeek.Size() > 35.f && !bHasLOS)
		{
			Steer(Crew, ToPeek, 1.f);
			return true;
		}
		if (bHasLOS && Crew->StartAttack(ETDCrewAttack::Burst, T))
		{
			return true;
		}
		if (Crew->GetCooldownLeft(ETDCrewAttack::Burst) > 0.f)
		{
			CoverPhase = ECoverPhase::Moving; // fired: duck back
			MoveBestDist = TNumericLimits<float>::Max();
			MoveStallTime = 0.f;
			MoveTotalTime = 0.f;
		}
		return true;
	}
	default:
		return false;
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

	if (S.bUsesCover && TickCover(Crew, DeltaSeconds, Distance))
	{
		return;
	}

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
	if (Crew->IsStaggered() || Crew->IsReacting()) // a react stands it still like a stagger
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

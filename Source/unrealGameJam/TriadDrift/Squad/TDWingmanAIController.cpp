// Triad Drift - Squad (Teammate 3). ENG-08: wingman AI (PROTECT).
//
// The wingman is a pilot, not a turret: it only ever moves the stick (SetFlightInput through
// TDSquad::SendFlightInput) and pulls the primary/secondary triggers. It never touches powers.

#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

ATDWingmanAIController::ATDWingmanAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	// Think after the squad manager has settled switches/deaths, before ships integrate movement.
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bAttachToPawn = false;
	bWantsPlayerState = false;
}

double ATDWingmanAIController::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void ATDWingmanAIController::InitWingman(ATDSquadManager* InSquad, ETDShipId InShipId)
{
	Squad = InSquad;
	ShipId = InShipId;
	Random.Initialize(static_cast<int32>(InShipId) * 7919 + 17);
}

ATDShip* ATDWingmanAIController::GetShip() const
{
	return Cast<ATDShip>(GetPawn());
}

void ATDWingmanAIController::BeginPlay()
{
	Super::BeginPlay();
	TDSquad::BindTeleported(this);
}

void ATDWingmanAIController::EndPlay(const EEndPlayReason::Type Reason)
{
	TDSquad::UnbindTeleported(this);
	Super::EndPlay(Reason);
}

void ATDWingmanAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// Taking a ship back from the player: settle into formation, then pick fights from there.
	CurrentTarget.Reset();
	PrevShield01 = TDSquad::GetShield01(InPawn);
	NextTargetEvalTime = Now(); // look for a fight immediately: wingmen keep fighting after a switch
	if (State != ETDWingmanState::Downed)
	{
		SetState(ETDWingmanState::Regroup);
	}
}

void ATDWingmanAIController::OnUnPossess()
{
	// Hand the ship over with its triggers released, its own speeds and no AI aim target (M1): the
	// player's aim assist must snap to what THEY aim at, not what the wingman was shooting.
	SetTriggers(false, false);
	TDSquad::SetCatchUpTopSpeed(GetShip(), 0.f);
	TDSquad::ClearAim(GetShip());
	bHyperdriveFormUp = false;
	CurrentTarget.Reset();
	Super::OnUnPossess();
}

void ATDWingmanAIController::SetState(ETDWingmanState NewState)
{
	if (State == NewState)
	{
		return;
	}
	State = NewState;
	StateEnterTime = Now();
	bRegroupCommitted = false;
	if (NewState != ETDWingmanState::Engage)
	{
		SetTriggers(false, false);
		TDSquad::ClearAim(GetShip());
	}
	if (NewState != ETDWingmanState::Formation && NewState != ETDWingmanState::Regroup)
	{
		TDSquad::SetCatchUpTopSpeed(GetShip(), 0.f); // fight and evade on the ship's own speeds
	}
}

void ATDWingmanAIController::SetFocusTarget(AActor* Target)
{
	if (Target && State != ETDWingmanState::Downed)
	{
		CurrentTarget = Target;
		SetState(ETDWingmanState::Engage);
	}
}

void ATDWingmanAIController::SetPassive(bool bInPassive)
{
	if (bPassive == bInPassive)
	{
		return;
	}
	bPassive = bInPassive;
	if (bPassive)
	{
		SetTriggers(false, false);
		CurrentTarget.Reset();
		TDSquad::SetCatchUpTopSpeed(GetShip(), 0.f);
		TDSquad::ClearAim(GetShip());
		TDSquad::SendNeutralInput(GetShip()); // don't leave the last stick latched
	}
	else if (State != ETDWingmanState::Downed)
	{
		State = ETDWingmanState::Formation; // force the transition
		SetState(ETDWingmanState::Regroup);
		NextTargetEvalTime = Now();
	}
}

void ATDWingmanAIController::NotifyShipDowned()
{
	SetTriggers(false, false);
	TDSquad::ClearAim(GetShip());
	CurrentTarget.Reset();
	SetState(ETDWingmanState::Downed);
}

void ATDWingmanAIController::NotifyShipRespawned()
{
	bHyperdriveFormUp = false;
	State = ETDWingmanState::Formation; // force the transition below even from Formation
	SetState(ETDWingmanState::Regroup);
	PrevShield01 = 1.f;
	NextTargetEvalTime = Now();
}

void ATDWingmanAIController::NotifyLeaderTeleported()
{
	ATDShip* Ship = GetShip();
	ATDSquadManager* S = Squad.Get();
	ATDShip* Leader = S ? S->GetFormationLeader() : nullptr;
	if (!Ship || !Leader || Leader == Ship || State == ETDWingmanState::Downed)
	{
		return;
	}
	const FVector Slot = S->GetFormationSlotLocation(ShipId);
	if (FVector::Dist(Ship->GetActorLocation(), Slot) > TeleportFollowDistance * 0.5f)
	{
		// Arrive IN FRONT of the leader's exit point (a portal or gate plane is behind it): a slot
		// 1600 cm behind would sit inside the disc and send us straight back through (T4 W2).
		const FVector Fwd = Leader->GetActorForwardVector();
		const float Behind = FMath::Max(0.f, FVector::DotProduct(Leader->GetActorLocation() - Slot, Fwd));
		const FVector Arrival = Slot + Fwd * (Behind + PortalExitClearance);
		TDSquad::TeleportShip(Ship, TDSquad::FindSafeLocation(this, Arrival, Ship), Leader->GetActorRotation(), /*bKeepSpeed*/ true);
		TDSquad::SetVelocity(Ship, TDSquad::GetVelocity(Leader));
		CurrentTarget.Reset();
		SetState(ETDWingmanState::Formation);
	}
}

void ATDWingmanAIController::HandleSpacecraftTeleported(AActor* Spacecraft, FVector From, FVector To)
{
	const ATDSquadManager* S = Squad.Get();
	if (S && Spacecraft && Spacecraft == S->GetFormationLeader() && Spacecraft != GetPawn())
	{
		NotifyLeaderTeleported();
	}
}

// ---------------------------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------------------------

void ATDWingmanAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ATDShip* Ship = GetShip();
	ATDSquadManager* S = Squad.Get();
	if (!Ship || !S || bPassive || State == ETDWingmanState::Downed || DeltaSeconds <= 0.f)
	{
		return;
	}
	ATDShip* Leader = S->GetFormationLeader();
	if (Leader == Ship)
	{
		return; // the player flies this one (we should already be unpossessed)
	}
	// Gates are the leader's call: never transit one ahead of the player (T4's gate test).
	TDSquad::HoldOutOfGates(Ship, 0.4f);
	const bool bHaveLeader = Leader && S->IsShipAlive(S->GetActiveShipId());
	const double T = Now();

	// B2: form up for the CURRENT leader's jump, read fresh every tick (never latched), so a switch,
	// a death or a retake can never leave wingmen stuck in formation.
	const bool bWasFormingUp = bHyperdriveFormUp;
	bHyperdriveFormUp = bHaveLeader && TDSquad::GetHyperdriveState(Leader) != ETDHyperdriveState::Idle;
	if (bHyperdriveFormUp && !bWasFormingUp)
	{
		CurrentTarget.Reset();
		SetState(ETDWingmanState::Regroup);
	}
	else if (!bHyperdriveFormUp && bWasFormingUp)
	{
		NextTargetEvalTime = T; // jump over or cancelled: pick a fight right away
	}

	// Aim error: a fresh 2-4 degree offset a few times a second, like a real hand on the stick.
	if (T >= NextAimErrorTime)
	{
		AimError.Pitch = Random.FRandRange(AimErrorMinDeg, AimErrorMaxDeg); // magnitude
		AimError.Yaw = Random.FRandRange(0.f, 360.f);                       // direction around the aim axis
		NextAimErrorTime = T + Random.FRandRange(0.3f, 0.6f);
	}

	// Hyperdrive: the leader is moving faster than any ship can fly. Jump with it, in formation.
	if (bHaveLeader)
	{
		float MinS, CruiseS, BoostS;
		TDSquad::GetSpeedRange(Ship, MinS, CruiseS, BoostS);
		const float LeaderSpeed = TDSquad::GetSpeed(Leader);
		// Ride in the slot for the whole jump and its drop-out (T1: the drop-out brakes the leader far
		// harder than a released wingman could, so a speed threshold alone leaves us 35-70k cm ahead),
		// plus any unannounced super-fast move.
		const ETDHyperdriveState LeaderJump = TDSquad::GetHyperdriveState(Leader);
		const bool bJumping = LeaderJump == ETDHyperdriveState::Jumping || LeaderJump == ETDHyperdriveState::DropOut;
		if (bJumping || LeaderSpeed > FMath::Max(HyperdriveFollowMinSpeed, BoostS * 2.f))
		{
			SetTriggers(false, false);
			const FVector Slot = S->GetFormationSlotLocation(ShipId);
			Ship->SetActorLocationAndRotation(Slot, Leader->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
			TDSquad::SetVelocity(Ship, TDSquad::GetVelocity(Leader));
			TDSquad::SendFlightInput(Ship, 0.f, 0.f, 0.f, CruiseS, false);
			CurrentTarget.Reset();
			SetState(ETDWingmanState::Formation);
			return;
		}

		// Lost the leader entirely (gate jump nobody announced, respawn far away): warp back to the slot.
		if (FVector::Dist(Ship->GetActorLocation(), Leader->GetActorLocation()) > TeleportFollowDistance * 2.f)
		{
			NotifyLeaderTeleported();
		}
	}

	// Threat reactions: shield break or an incoming missile -> evade (rate-limited).
	const float Shield01 = TDSquad::GetShield01(Ship);
	const bool bShieldBroke = PrevShield01 > 0.01f && Shield01 <= 0.01f;
	PrevShield01 = Shield01;
	if (State != ETDWingmanState::Evade && T >= NextEvadeAllowedTime)
	{
		if (bShieldBroke || TDSquad::IsMissileIncoming(Ship))
		{
			BeginEvade(Ship);
		}
	}

	switch (State)
	{
	case ETDWingmanState::Formation:
	case ETDWingmanState::Regroup:
		UpdateFormation(DeltaSeconds, Ship, bHaveLeader ? Leader : nullptr);
		break;
	case ETDWingmanState::Engage:
		UpdateEngage(DeltaSeconds, Ship, bHaveLeader ? Leader : nullptr);
		break;
	case ETDWingmanState::Evade:
		UpdateEvade(DeltaSeconds, Ship, bHaveLeader ? Leader : nullptr);
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------------------------------------
// States
// ---------------------------------------------------------------------------------------------

FVector ATDWingmanAIController::GetHomeLocation(ATDShip* Ship, ATDShip* Leader, bool& bOutIsDome) const
{
	bOutIsDome = false;
	if (TDSquad::GetHealth01(Ship) < HurtThreshold01)
	{
		FVector DomeCenter;
		float DomeRadius = 0.f;
		if (TDSquad::GetNearbyShieldDome(GetWorld(), Ship->GetActorLocation(), DomeSeekDistance, DomeCenter, DomeRadius))
		{
			bOutIsDome = true;
			// Aim for a point well inside the dome, on our side of it, rather than its exact centre.
			const FVector FromCenter = (Ship->GetActorLocation() - DomeCenter).GetSafeNormal();
			return DomeCenter + FromCenter * DomeRadius * 0.4f;
		}
	}
	const ATDSquadManager* S = Squad.Get();
	if (S && Leader)
	{
		return S->GetFormationSlotLocation(ShipId);
	}
	return Ship->GetActorLocation() + Ship->GetActorForwardVector() * 5000.f;
}

void ATDWingmanAIController::UpdateFormation(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader)
{
	const double T = Now();

	// Look for a fight, unless we're forming up for a hyperdrive jump.
	if (T >= NextTargetEvalTime && !bHyperdriveFormUp)
	{
		NextTargetEvalTime = T + TargetReevaluateInterval;
		if (AActor* Target = PickTarget(Ship, Leader))
		{
			const bool bFocus = Squad.IsValid() && Target == Squad->GetFocusTarget();
			const bool bCommitted = State == ETDWingmanState::Regroup && bRegroupCommitted && T - StateEnterTime < RegroupCommitSeconds;
			if (bFocus || !bCommitted)
			{
				CurrentTarget = Target;
				SetState(ETDWingmanState::Engage);
				UpdateEngage(DeltaSeconds, Ship, Leader);
				return;
			}
		}
	}

	bool bDome = false;
	const FVector Home = GetHomeLocation(Ship, Leader, bDome);
	const FVector RefVel = (Leader && !bDome) ? TDSquad::GetVelocity(Leader) : FVector::ZeroVector;
	const FVector RefFwd = (Leader && !bDome) ? Leader->GetActorForwardVector() : Ship->GetActorForwardVector();
	FlyToPoint(Ship, Home, RefVel, RefFwd);

	if (State == ETDWingmanState::Regroup)
	{
		const float Dist = FVector::Dist(Ship->GetActorLocation(), Home);
		if (Dist < FormationHoldRadius * 4.f || T - StateEnterTime > 8.0)
		{
			SetState(ETDWingmanState::Formation);
		}
	}
}

void ATDWingmanAIController::FlyToPoint(ATDShip* Ship, const FVector& Point, const FVector& RefVelocity, const FVector& RefForward)
{
	float MinS, CruiseS, BoostS;
	TDSquad::GetSpeedRange(Ship, MinS, CruiseS, BoostS);

	const FVector Loc = Ship->GetActorLocation();
	const FVector ToPoint = Point - Loc;
	const float Dist = ToPoint.Size();
	const float RefSpeed = RefVelocity.Size();

	// Catch-up allowance: a slow ship (Bastion) must still keep up with a faster leader, so the
	// throttle may reach a little above the leader's speed (normal acceleration, no boost meter).
	TDSquad::SetCatchUpTopSpeed(Ship, FMath::Max(CruiseS, RefSpeed * CatchUpSpeedRatio));

	// Split the slot error into along-track (+ = the slot is ahead, we are behind) and lateral.
	const float Along = FVector::DotProduct(ToPoint, RefForward);
	const FVector Lateral = ToPoint - RefForward * Along;
	const bool bAligned = FVector::DotProduct(Ship->GetActorForwardVector(), RefForward) > 0.2f;
	const bool bLeaderMoving = RefSpeed > MinS * 0.5f;

	FVector Dir;
	float TargetSpeed;
	bool bBoost = false;

	if (bLeaderMoving && (bAligned || Dist < FormationHoldRadius * 3.f))
	{
		// Formation flight: fly parallel to the leader, steer out the lateral error and fix the
		// along-track error with SPEED only. Overshooting the slot means easing off, never a U-turn.
		const float LookAhead = FMath::Max(2500.f, RefSpeed * 0.6f);
		Dir = (RefForward * LookAhead + Lateral).GetSafeNormal();
		TargetSpeed = RefSpeed + FMath::Clamp(Along * FormationSpeedGain, -RefSpeed, 6000.f);
		// A squadmate right ahead or behind can't be steered around at matched speed: open the gap
		// along the track too (ahead of us -> ease off, behind us -> speed up).
		TargetSpeed += FVector::DotProduct(ComputeSeparation(Ship), RefForward) * SeparationSpeedGain;
		bBoost = Along > CatchUpBoostDistance; // short gap-closing burst when well behind
	}
	else
	{
		// Pointing the wrong way (after a fight or a respawn) or the leader is parked: fly to where
		// the slot will be; the branch above takes over once we head the same way.
		const FVector Lead = Point + RefVelocity * FMath::Clamp(Dist / FMath::Max(BoostS, 1.f), 0.2f, 1.5f);
		Dir = (Lead - Loc).GetSafeNormal();
		TargetSpeed = bLeaderMoving ? FMath::Max(RefSpeed, CruiseS) + FMath::Min(Dist, 8000.f)
			: FMath::Clamp(Dist, MinS, CruiseS);
		bBoost = Dist > CatchUpBoostDistance;
	}

	if (Dir.IsNearlyZero())
	{
		Dir = Ship->GetActorForwardVector();
	}
	SteerToward(Ship, ApplyAvoidance(Ship, Dir), FMath::Max(TargetSpeed, MinS), bBoost);
}

void ATDWingmanAIController::UpdateEngage(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader)
{
	const double T = Now();

	// Re-pick periodically: focus orders and threats to the player can pre-empt the current target.
	if (T >= NextTargetEvalTime || !CurrentTarget.IsValid() || TDSquad::IsDead(CurrentTarget.Get()))
	{
		NextTargetEvalTime = T + TargetReevaluateInterval;
		CurrentTarget = PickTarget(Ship, Leader);
	}

	AActor* Target = CurrentTarget.Get();
	if (!Target || TDSquad::IsDead(Target))
	{
		CurrentTarget.Reset();
		SetState(ETDWingmanState::Regroup);
		return;
	}

	// Leash: never get dragged too far from the player.
	if (Leader && FVector::Dist(Ship->GetActorLocation(), Leader->GetActorLocation()) > LeashDistance)
	{
		CurrentTarget.Reset();
		SetState(ETDWingmanState::Regroup);
		bRegroupCommitted = true;
		return;
	}

	float MinS, CruiseS, BoostS;
	TDSquad::GetSpeedRange(Ship, MinS, CruiseS, BoostS);

	const FVector Loc = Ship->GetActorLocation();
	const FVector Fwd = Ship->GetActorForwardVector();
	const FVector TLoc = Target->GetActorLocation();
	const FVector TVel = Target->GetVelocity();
	const float Dist = FVector::Dist(Loc, TLoc);

	// Lead the target by projectile flight time.
	const float ProjSpeed = TDSquad::GetPrimaryProjectileSpeed(Ship);
	const float FlightTime = FMath::Clamp(Dist / ProjSpeed, 0.f, 1.5f);
	const FVector AimPoint = TLoc + TVel * FlightTime;
	const FVector AimDir = (AimPoint - Loc).GetSafeNormal();

	// Steering: pursue the lead point, but break off rather than ram the target.
	FVector SteerDir = AimDir;
	float TargetSpeed;
	bool bBoost = false;
	const bool bInFront = FVector::DotProduct(Fwd, AimDir) > 0.f;
	// M7: break off at a distance that accounts for the target's size (a carrier is ~1800 cm across).
	const float BreakOff = BreakOffDistance + TDSquad::GetBoundsRadius(Target);
	if (Dist < BreakOff && bInFront)
	{
		const FVector Side = FVector::CrossProduct(AimDir, Ship->GetActorUpVector()).GetSafeNormal();
		SteerDir = (Fwd + Side * 0.8f + Ship->GetActorUpVector() * 0.4f).GetSafeNormal();
		TargetSpeed = BoostS;
		bBoost = true;
	}
	else if (Dist > 8000.f)
	{
		TargetSpeed = BoostS;
		bBoost = Dist > 12000.f;
	}
	else
	{
		TargetSpeed = FMath::Clamp(TVel.Size() + 1500.f, MinS, CruiseS);
	}

	// Hurt and a dome is up: drift toward it while fighting.
	bool bDome = false;
	const FVector Home = GetHomeLocation(Ship, Leader, bDome);
	if (bDome)
	{
		SteerDir = (SteerDir + (Home - Loc).GetSafeNormal() * 0.6f).GetSafeNormal();
	}

	// Stay in the shot (live-measured: wingmen fought behind the camera): unless off defending the
	// player, get in front of the chase camera, and keep out of the player's aim line.
	if (Leader && !bDefendingPlayer)
	{
		const FTransform Cam = Leader->GetChaseCameraTransformNow();
		const FVector CamFwd = Cam.GetRotation().GetForwardVector();
		const float Depth = FVector::DotProduct(Loc - Cam.GetLocation(), CamFwd);
		if (Depth < InShotMinDepth)
		{
			SteerDir = (SteerDir + CamFwd * InShotOvertakeWeight).GetSafeNormal();
			TargetSpeed = BoostS;
			bBoost = true;
		}
		const FVector LeaderLoc = Leader->GetActorLocation();
		const FVector AimLine = TDSquad::GetAimDirection(Leader);
		const FVector ToMe = Loc - LeaderLoc;
		const float Along = FVector::DotProduct(ToMe, AimLine);
		if (Along > 0.f)
		{
			const FVector Off = ToMe - AimLine * Along;
			const float MinOff = Along * FMath::Tan(FMath::DegreesToRadians(AimLineClearanceDeg)) + 400.f;
			if (Off.Size() < MinOff)
			{
				const FVector Out = Off.IsNearlyZero() ? Leader->GetActorUpVector() : Off.GetSafeNormal();
				SteerDir = (SteerDir + Out * 1.5f).GetSafeNormal();
			}
		}
	}

	SteerToward(Ship, ApplyAvoidance(Ship, SteerDir), TargetSpeed, bBoost);

	// Weapon aim: the lead direction plus this pilot's aim error.
	FVector U, V;
	AimDir.FindBestAxisVectors(U, V);
	const float Phi = FMath::DegreesToRadians(AimError.Yaw);
	const float Err = FMath::DegreesToRadians(AimError.Pitch);
	const FVector Perp = U * FMath::Cos(Phi) + V * FMath::Sin(Phi);
	const FVector ShotDir = (AimDir * FMath::Cos(Err) + Perp * FMath::Sin(Err)).GetSafeNormal();
	// m7: direction only (no explicit aim target), so the weapon's aim assist can't cancel the 2-4 deg error.
	TDSquad::SetAim(Ship, nullptr, ShotDir);

	// Triggers: only with the nose on target, in range, not overheated, and not through the player.
	const float NoseAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Fwd, AimDir), -1.f, 1.f)));
	bool bClearShot = NoseAngle <= FireConeDeg && Dist <= PrimaryRange;
	if (bClearShot && Leader)
	{
		const FVector LLoc = Leader->GetActorLocation();
		const FVector Closest = FMath::ClosestPointOnSegment(LLoc, Loc, AimPoint);
		if (FVector::DistSquared(Closest, LLoc) < FMath::Square(700.f) && FVector::DistSquared(Loc, LLoc) < FMath::Square(Dist))
		{
			bClearShot = false;
		}
	}

	const bool bPrimary = bClearShot && !TDSquad::IsOverheated(Ship, ETDWeaponSlot::Primary);

	bool bSecondary = false;
	if (T < SecondaryReleaseTime)
	{
		// Finish the hold we started (charge weapons fire on release), unless the shot is no longer
		// clear (m8: e.g. the player flew into the line): then release early and let it go.
		bSecondary = bClearShot;
		if (!bClearShot)
		{
			SecondaryReleaseTime = 0.0;
		}
	}
	else if (bClearShot && T >= NextSecondaryTime && TDSquad::GetAmmo(Ship, ETDWeaponSlot::Secondary) > 0)
	{
		bSecondary = true;
		SecondaryReleaseTime = T + SecondaryHoldSeconds;
		NextSecondaryTime = T + SecondaryInterval + Random.FRandRange(0.f, 1.5f);
	}
	SetTriggers(bPrimary, bSecondary);
}

void ATDWingmanAIController::BeginEvade(ATDShip* Ship)
{
	const FVector Fwd = Ship->GetActorForwardVector();
	FVector U, V;
	Fwd.FindBestAxisVectors(U, V);
	const float Phi = Random.FRandRange(0.f, 2.f * PI);
	const FVector Perp = U * FMath::Cos(Phi) + V * FMath::Sin(Phi);
	EvadeDirection = (Perp * 1.2f + Fwd * 0.5f).GetSafeNormal();
	NextEvadeAllowedTime = Now() + EvadeSeconds + EvadeCooldown;
	SetState(ETDWingmanState::Evade);
}

void ATDWingmanAIController::UpdateEvade(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader)
{
	const double T = Now();
	const double Elapsed = T - StateEnterTime;
	if (Elapsed >= EvadeSeconds)
	{
		SetState(ETDWingmanState::Regroup);
		bRegroupCommitted = true;
		return;
	}

	float MinS, CruiseS, BoostS;
	TDSquad::GetSpeedRange(Ship, MinS, CruiseS, BoostS);

	bool bDome = false;
	const FVector Home = GetHomeLocation(Ship, Leader, bDome);
	FVector Dir = EvadeDirection;
	if (bDome)
	{
		Dir = (Home - Ship->GetActorLocation()).GetSafeNormal();
	}
	else if (Elapsed > EvadeSeconds * 0.5)
	{
		// Jink: reverse the lateral half of the break for the second half.
		const FVector Fwd = Ship->GetActorForwardVector();
		const FVector Lateral = EvadeDirection - Fwd * FVector::DotProduct(EvadeDirection, Fwd);
		Dir = (Fwd * 0.6f - Lateral).GetSafeNormal();
	}
	SteerToward(Ship, ApplyAvoidance(Ship, Dir), BoostS, true);
}

// ---------------------------------------------------------------------------------------------
// Targeting
// ---------------------------------------------------------------------------------------------

AActor* ATDWingmanAIController::PickTarget(ATDShip* Ship, ATDShip* Leader) const
{
	const ATDSquadManager* S = Squad.Get();

	// 1) A focus order beats everything.
	if (S)
	{
		if (AActor* Focus = S->GetFocusTarget())
		{
			// m9: a focus target beyond the leash would flip Engage/Regroup; ignore it until it's in reach.
			if (!Leader || FVector::Dist(Focus->GetActorLocation(), Leader->GetActorLocation()) <= LeashDistance)
			{
				return Focus;
			}
		}
	}

	TDSquad::GetPlayerHostiles(GetWorld(), ScratchHostiles);
	if (ScratchHostiles.Num() == 0)
	{
		return nullptr;
	}

	const FVector SelfLoc = Ship->GetActorLocation();
	const FVector Anchor = Leader ? Leader->GetActorLocation() : SelfLoc;
	// m6: only a RECENT attacker counts as threatening the player.
	const AActor* PlayerAttacker = (Leader && TDSquad::GetTimeSinceLastDamage(Leader) <= ThreatMemorySeconds)
		? TDSquad::GetLastAttacker(Leader) : nullptr;

	// What the other wingman is shooting (spread out unless the player is threatened).
	const AActor* OtherWingTarget = nullptr;
	bool bOtherDefending = false;
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
					bOtherDefending = OtherAI->IsDefendingPlayer();
				}
			}
		}
	}

	AActor* BestThreat = nullptr;
	float BestThreatDistSq = TNumericLimits<float>::Max();
	bool bBestThreatInView = false;
	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	// Live measurement (TD.Squad.Frame): with a bias alone the wing still fought off-screen most of the
	// time. So when anything worth shooting is inside the player's view, pick only among those.
	AActor* BestInView = nullptr;
	float BestInViewScore = TNumericLimits<float>::Max();
	FTransform ViewCam = FTransform::Identity;
	float CosView = 2.f; // > 1: no view cone without a leader
	if (Leader)
	{
		ViewCam = Leader->GetChaseCameraTransformNow();
		CosView = FMath::Cos(FMath::DegreesToRadians(FMath::Max(10.f, Leader->GetChaseCameraFOVNow() * 0.5f * ViewConeScale)));
	}

	for (AActor* H : ScratchHostiles)
	{
		if (!IsValid(H) || TDSquad::IsDead(H))
		{
			continue;
		}
		const FVector HLoc = H->GetActorLocation();
		const float DistToAnchorSq = FVector::DistSquared(HLoc, Anchor);
		if (DistToAnchorSq > FMath::Square(EngageRange))
		{
			continue;
		}

		// 2) Whoever threatens the player: hit them recently, or close and pointed at them.
		if (Leader)
		{
			const bool bAttacker = (H == PlayerAttacker);
			const bool bCloseAndFacing = DistToAnchorSq < FMath::Square(ThreatRadius)
				&& FVector::DotProduct(H->GetActorForwardVector(), (Anchor - HLoc).GetSafeNormal()) > 0.7f;
			// m6: one wingman is enough on a threat; the other keeps its own priorities.
			if ((bAttacker || bCloseAndFacing) && H != OtherWingTarget && DistToAnchorSq < BestThreatDistSq)
			{
				BestThreatDistSq = DistToAnchorSq;
				BestThreat = H;
				bBestThreatInView = Leader && FVector::DotProduct((HLoc - Leader->GetChaseCameraTransformNow().GetLocation()).GetSafeNormal(),
					Leader->GetChaseCameraTransformNow().GetRotation().GetForwardVector()) > FMath::Cos(FMath::DegreesToRadians(Leader->GetChaseCameraFOVNow() * 0.5f * ViewConeScale));
			}
		}

		// 3) Nearest, then 4) lowest health: distance in 2000 cm bands, health breaks ties in a band.
		const float Dist = FVector::Dist(SelfLoc, HLoc);
		float Score = FMath::FloorToFloat(Dist / 2000.f) + TDSquad::GetHealth01(H) * 0.99f;
		if (H == OtherWingTarget)
		{
			Score += 1.5f; // prefer a different target from the other wingman
		}
		// Fight where the player can see it: targets behind or beside the leader cost extra bands, so
		// with a choice the wing's dogfights happen in front of the chase camera (threats to the player
		// above still always win).
		if (Leader)
		{
			const float Facing = FVector::DotProduct(Leader->GetActorForwardVector(), (HLoc - Anchor).GetSafeNormal());
			Score += (1.f - Facing) * 0.5f * OnScreenTargetBias;
		}
		if (H == CurrentTarget.Get())
		{
			Score -= 0.75f; // stickiness: don't flicker between equal targets
		}
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = H;
		}
		const bool bInView = FVector::DotProduct((HLoc - ViewCam.GetLocation()).GetSafeNormal(), ViewCam.GetRotation().GetForwardVector()) > CosView;
		if (bInView && Score < BestInViewScore)
		{
			BestInViewScore = Score;
			BestInView = H;
		}
	}

	// Defending the player comes first, but one wingman off-screen on defence is enough: the other
	// keeps the fight in front of the camera (a threat already on screen is fine for either).
	if (BestThreat && (!bOtherDefending || bBestThreatInView))
	{
		bDefendingPlayer = !bBestThreatInView;
		return BestThreat;
	}
	bDefendingPlayer = false;
	return BestInView ? BestInView : Best;
}

// ---------------------------------------------------------------------------------------------
// Piloting
// ---------------------------------------------------------------------------------------------

void ATDWingmanAIController::SteerToward(ATDShip* Ship, const FVector& DesiredDir, float TargetSpeed, bool bBoost)
{
	const FVector Local = Ship->GetActorTransform().InverseTransformVectorNoScale(DesiredDir.GetSafeNormal());
	const float YawErr = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
	const float PitchErr = FMath::RadiansToDegrees(FMath::Atan2(Local.Z, FVector2D(Local.X, Local.Y).Size()));
	const float Gain = 1.f / FMath::Max(FullStickAngleDeg, 1.f);
	const float Yaw = FMath::Clamp(YawErr * Gain, -1.f, 1.f);
	const float Pitch = FMath::Clamp(PitchErr * Gain, -1.f, 1.f);
	// Roll is left to the flight model's auto-level and visual bank.
	TDSquad::SendFlightInput(Ship, Pitch, Yaw, 0.f, TargetSpeed, bBoost);
}

FVector ATDWingmanAIController::ApplyAvoidance(ATDShip* Ship, const FVector& DesiredDir)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return DesiredDir;
	}
	const FVector Loc = Ship->GetActorLocation();
	const ATDSquadManager* S = Squad.Get();
	const double T = Now();

	// Obstacles: a sphere sweep along our velocity, a few times a second (cached in between).
	if (T >= NextAvoidanceSweepTime)
	{
		NextAvoidanceSweepTime = T + 0.15;
		CachedAvoidance = FVector::ZeroVector;

		FVector Vel = TDSquad::GetVelocity(Ship);
		if (Vel.SizeSquared() < 1.f)
		{
			Vel = Ship->GetActorForwardVector() * 1500.f;
		}
		const FVector End = Loc + Vel * AvoidanceLookAhead;
		const FVector VelDir = Vel.GetSafeNormal();

		// 1) T4's obstacle registry (asteroids, ruins, planets, Dominion hull): cheap and exact.
		FVector RegisteredAvoid;
		const float Probe = Vel.Size() * AvoidanceLookAhead + 4.f * AvoidanceShipRadius;
		if (TDSquad::FindRegisteredAvoidance(World, Loc, VelDir, Probe, AvoidanceShipRadius, RegisteredAvoid))
		{
			CachedAvoidance = RegisteredAvoid.GetSafeNormal() * 3.f;
		}
		else
		{
			// 2) Fallback sweep for unregistered geometry (portal gate frames, pylons, moving ships).
			FCollisionQueryParams Params(SCENE_QUERY_STAT(TDWingmanAvoid), false, Ship);
			if (S)
			{
				for (const FTDSquadSlot& Slot : S->GetSlots())
				{
					Params.AddIgnoredActor(Slot.Ship);
				}
			}
			FCollisionObjectQueryParams Objects;
			Objects.AddObjectTypesToQuery(ECC_WorldStatic);
			Objects.AddObjectTypesToQuery(ECC_WorldDynamic);

			FHitResult Hit;
			if (World->SweepSingleByObjectType(Hit, Loc, End, FQuat::Identity, Objects, FCollisionShape::MakeSphere(AvoidanceShipRadius), Params))
			{
				// Push out along the surface normal, harder the sooner we'd hit.
				const float Urgency = 1.f - Hit.Time;
				FVector Away = Hit.ImpactNormal;
				// Prefer going around rather than straight back.
				Away = (Away - VelDir * FVector::DotProduct(Away, VelDir)).GetSafeNormal();
				if (Away.IsNearlyZero())
				{
					Away = Ship->GetActorUpVector();
				}
				CachedAvoidance = Away * (1.5f + 2.5f * Urgency);
			}
		}
	}

	// Squadmates (and the player): keep TeammateSeparation.
	const FVector Separation = ComputeSeparation(Ship);

	const FVector Result = (DesiredDir + CachedAvoidance + Separation).GetSafeNormal();
	return Result.IsNearlyZero() ? DesiredDir : Result;
}

FVector ATDWingmanAIController::ComputeSeparation(const ATDShip* Ship) const
{
	FVector Separation = FVector::ZeroVector;
	const ATDSquadManager* S = Squad.Get();
	if (!S || !Ship)
	{
		return Separation;
	}
	const FVector Loc = Ship->GetActorLocation();
	for (const FTDSquadSlot& Slot : S->GetSlots())
	{
		const ATDShip* Mate = Slot.Ship;
		if (Slot.State != ETDSquadSlotState::Alive || !Mate || Mate == Ship)
		{
			continue;
		}
		const FVector Delta = Loc - Mate->GetActorLocation();
		const float D = Delta.Size();
		if (D > KINDA_SMALL_NUMBER && D < TeammateSeparation)
		{
			Separation += (Delta / D) * (1.f - D / TeammateSeparation) * TeammateSeparationStrength;
		}
	}
	// A director money shot is running: stay out of its lens (C-03 #2).
	FVector ShotCamera;
	if (TDSquad::GetDirectorShotCamera(Ship, ShotCamera))
	{
		const FVector Delta = Loc - ShotCamera;
		const float D = Delta.Size();
		if (D > KINDA_SMALL_NUMBER && D < ShotCameraClearance)
		{
			Separation += (Delta / D) * (1.f - D / ShotCameraClearance) * TeammateSeparationStrength * 2.f;
		}
	}
	return Separation;
}

void ATDWingmanAIController::SetTriggers(bool bPrimary, bool bSecondary)
{
	ATDShip* Ship = GetShip();
	if (!Ship)
	{
		bPrimaryHeld = bSecondaryHeld = false;
		return;
	}
	if (bPrimary != bPrimaryHeld)
	{
		bPrimaryHeld = bPrimary;
		TDSquad::SetTriggerHeld(Ship, ETDWeaponSlot::Primary, bPrimary);
	}
	if (bSecondary != bSecondaryHeld)
	{
		bSecondaryHeld = bSecondary;
		TDSquad::SetTriggerHeld(Ship, ETDWeaponSlot::Secondary, bSecondary);
	}
}

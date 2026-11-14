// Triad Drift - hyperdrive (ENG-15). Owner: Teammate 1.

#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "EngineUtils.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"

const FName UTDHyperdriveComponent::OverrideOwner(TEXT("Hyperdrive"));

template <typename TFX>
concept TDHyperdrive_HasRefusedCue = requires(TFX* FX) { FX->HyperdriveRefused; };

/** Plays UTDTravelFX::HyperdriveRefused once Core has that slot (compiles to nothing before). */
template <typename TFX>
void TDHyperdrive_PlayRefusedCue(TFX* FX, const ATDSpacecraft* Craft)
{
	if constexpr (TDHyperdrive_HasRefusedCue<TFX>)
	{
		if (FX && Craft)
		{
			UTDStatics::PlayCueAttached(FX->HyperdriveRefused, Craft->GetRootComponent());
		}
	}
}

UTDHyperdriveComponent::UTDHyperdriveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTDHyperdriveComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!TravelFX)
	{
		TravelFX = UTDTravelSubsystem::GetDefaultTravelFX(this);
	}
	if (ATDSpacecraft* Craft = GetCraft())
	{
		if (UTDHealthComponent* Health = Craft->GetHealth())
		{
			Health->OnDamaged.AddDynamic(this, &UTDHyperdriveComponent::HandleDamaged);
			Health->OnDeath.AddDynamic(this, &UTDHyperdriveComponent::HandleDeath);
		}
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->OnFlightImpact.AddDynamic(this, &UTDHyperdriveComponent::HandleImpact);
			Craft->OnTeleported.AddDynamic(this, &UTDHyperdriveComponent::HandleTeleported);
			// Run after the flight model so well checks see this frame's movement.
			AddTickPrerequisiteComponent(Move);
		}
		LastLocation = Craft->GetActorLocation();
	}
}

void UTDHyperdriveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLoopFX();
	Super::EndPlay(EndPlayReason);
}

ATDSpacecraft* UTDHyperdriveComponent::GetCraft() const
{
	return Cast<ATDSpacecraft>(GetOwner());
}

double UTDHyperdriveComponent::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

ETDHyperdriveBlock UTDHyperdriveComponent::GetBlockReason() const
{
	return ComputeBlockReason(false);
}

ETDHyperdriveBlock UTDHyperdriveComponent::ComputeBlockReason(bool bCachedSweep) const
{
	const ATDSpacecraft* Craft = GetCraft();
	if (!bEnabled || !Craft)
	{
		return ETDHyperdriveBlock::Disabled;
	}
	const ATDShip* Ship = Cast<ATDShip>(Craft);
	if (!Ship || !Ship->IsPlayerFlown())
	{
		return ETDHyperdriveBlock::NotPlayerFlown;
	}
	if (const UTDShipMovementComponent* Move = Craft->GetShipMovement())
	{
		if (Move->IsInAtmosphere())
		{
			return ETDHyperdriveBlock::InAtmosphere;
		}
	}
	const double T = Now();
	if (T - LastDamageTime < Tuning.DamageLockoutSeconds)
	{
		return ETDHyperdriveBlock::RecentDamage;
	}
	if (T - LastDropOutTime < Tuning.CooldownSeconds)
	{
		return ETDHyperdriveBlock::Cooldown;
	}
	if (const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		FText LockReason;
		if (Travel->IsHyperdriveLocked(LockReason))
		{
			return ETDHyperdriveBlock::Locked;
		}
		FTDTravelGravityWell Well;
		if (Travel->FindWellContaining(Craft->GetActorLocation(), Well))
		{
			return ETDHyperdriveBlock::NearGravityWell;
		}
	}
	if (Tuning.ObstacleClearanceToSpool > 0.f)
	{
		bool bBlocked;
		if (bCachedSweep && SweepCacheTime >= 0.0 && T - SweepCacheTime < HUDSweepInterval)
		{
			bBlocked = bSweepCacheBlocked; // HUD polling: a 200 m sphere sweep a few times a second, not every frame
		}
		else
		{
			bBlocked = SweepAhead(Tuning.ObstacleClearanceToSpool) >= 0.f;
			SweepCacheTime = T;
			bSweepCacheBlocked = bBlocked;
		}
		if (bBlocked)
		{
			return ETDHyperdriveBlock::ObstacleAhead;
		}
	}
	return ETDHyperdriveBlock::None;
}

float UTDHyperdriveComponent::SweepAhead(float MaxDistance) const
{
	const ATDSpacecraft* Craft = GetCraft();
	const UWorld* World = GetWorld();
	if (!Craft || !World || MaxDistance <= 0.f)
	{
		return -1.f;
	}
	const USphereComponent* Sphere = Craft->GetCollisionSphere();
	const float Radius = (Sphere ? Sphere->GetScaledSphereRadius() : 220.f) * Tuning.LookAheadRadiusScale;
	const FVector Start = Craft->GetActorLocation();
	const FVector End = Start + Craft->GetActorForwardVector() * MaxDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDHyperdriveLookAhead), false, Craft);
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		// Squadmates fly in formation around the leader: they never block the jump.
		TArray<AActor*> Squad;
		Registry->GetActorsOfTeam(Craft->GetTeam(), Squad, true);
		Params.AddIgnoredActors(Squad);
	}
	FCollisionResponseParams Response;
	if (Sphere)
	{
		Response.CollisionResponse = Sphere->GetCollisionResponseToChannels();
	}
	FHitResult Hit;
	if (World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, Sphere ? Sphere->GetCollisionObjectType() : ECC_Pawn,
		FCollisionShape::MakeSphere(Radius), Params, Response) && Hit.bBlockingHit)
	{
		return Hit.Distance;
	}
	return -1.f;
}

FText UTDHyperdriveComponent::GetBlockReasonText() const
{
	const ETDHyperdriveBlock Block = ComputeBlockReason(true);
	// Standing reasons the player can do nothing about (on a planet, landing, not flying this ship) only show
	// briefly after a refused press, not permanently on the HUD (Lead, Nov 12). Reasons that tell the player
	// something useful right now (enemies in range, the finale, taking fire, a well, an obstacle, recharging) stay.
	if (IsStandingBlock(Block) && (LastRefusedTime < 0.0 || Now() - LastRefusedTime > StandingReasonShowSeconds))
	{
		return FText::GetEmpty();
	}
	return TextForBlock(Block);
}

bool UTDHyperdriveComponent::IsStandingBlock(ETDHyperdriveBlock Block)
{
	return Block == ETDHyperdriveBlock::InAtmosphere || Block == ETDHyperdriveBlock::Disabled || Block == ETDHyperdriveBlock::NotPlayerFlown;
}

FText UTDHyperdriveComponent::TextForBlock(ETDHyperdriveBlock Block) const
{
	switch (Block)
	{
	case ETDHyperdriveBlock::Locked:
	{
		FText LockText;
		if (const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
		{
			Travel->IsHyperdriveLocked(LockText);
		}
		return LockText.IsEmpty() ? NSLOCTEXT("TDHyperdrive", "Locked", "HYPERDRIVE LOCKED") : LockText;
	}
	case ETDHyperdriveBlock::RecentDamage: return NSLOCTEXT("TDHyperdrive", "Damage", "HYPERDRIVE DISRUPTED - TAKING FIRE");
	case ETDHyperdriveBlock::InAtmosphere: return NSLOCTEXT("TDHyperdrive", "Atmosphere", "NO HYPERDRIVE IN ATMOSPHERE");
	case ETDHyperdriveBlock::NearGravityWell: return NSLOCTEXT("TDHyperdrive", "Well", "TOO CLOSE TO A GRAVITY WELL");
	case ETDHyperdriveBlock::Cooldown: return NSLOCTEXT("TDHyperdrive", "Cooldown", "HYPERDRIVE RECHARGING");
	case ETDHyperdriveBlock::ObstacleAhead: return NSLOCTEXT("TDHyperdrive", "Obstacle", "OBSTACLE AHEAD");
	case ETDHyperdriveBlock::NotPlayerFlown:
	case ETDHyperdriveBlock::Disabled: return NSLOCTEXT("TDHyperdrive", "Offline", "HYPERDRIVE OFFLINE");
	default: return FText::GetEmpty();
	}
}

bool UTDHyperdriveComponent::RequestSpoolFromPilot()
{
	if (State != ETDHyperdriveState::Idle)
	{
		return false;
	}
	const ETDHyperdriveBlock Block = GetBlockReason();
	if (Block != ETDHyperdriveBlock::None)
	{
		const FText Reason = TextForBlock(Block);
		LastRefusedTime = Now();
		UE_LOG(LogTriadDrift, Log, TEXT("Hyperdrive refused: %s"), *Reason.ToString());
		OnSpoolRefused.Broadcast(this, Block, Reason);
		TDHyperdrive_PlayRefusedCue(TravelFX.Get(), GetCraft());
		return false;
	}
	return StartSpool();
}

bool UTDHyperdriveComponent::StartSpool()
{
	if (State != ETDHyperdriveState::Idle || !CanSpool())
	{
		return false;
	}
	Charge = 0.f;
	SetState(ETDHyperdriveState::Spooling);
	if (TravelFX)
	{
		if (const ATDSpacecraft* Craft = GetCraft())
		{
			StartLoopFX(TravelFX->HyperdriveSpool);
		}
	}
	return true;
}

void UTDHyperdriveComponent::StopSpool()
{
	if (State == ETDHyperdriveState::Spooling)
	{
		CancelSpool();
	}
}

void UTDHyperdriveComponent::CancelSpool()
{
	if (State == ETDHyperdriveState::Spooling)
	{
		Charge = 0.f;
		StopLoopFX();
		SetState(ETDHyperdriveState::Idle);
	}
}

bool UTDHyperdriveComponent::RequestDropOut()
{
	if (State == ETDHyperdriveState::Jumping && StateTime >= Tuning.MinJumpSeconds)
	{
		BeginDropOut(NAME_None, ETDHyperdriveEndCause::Manual);
		return true;
	}
	return false;
}

void UTDHyperdriveComponent::SetHyperdriveEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
	if (!bEnabled)
	{
		CancelSpool();
		if (State == ETDHyperdriveState::Jumping)
		{
			BeginDropOut(NAME_None, ETDHyperdriveEndCause::Manual);
		}
	}
}

float UTDHyperdriveComponent::GetCharge01() const
{
	switch (State)
	{
	case ETDHyperdriveState::Spooling: return FMath::Clamp(Charge, 0.f, 1.f);
	case ETDHyperdriveState::Jumping: return 1.f;
	case ETDHyperdriveState::DropOut: return Tuning.DropOutSeconds > 0.f ? FMath::Clamp(1.f - StateTime / Tuning.DropOutSeconds, 0.f, 1.f) : 0.f;
	default: return 0.f;
	}
}

void UTDHyperdriveComponent::SetState(ETDHyperdriveState NewState)
{
	if (State == NewState)
	{
		return;
	}
	State = NewState;
	StateTime = 0.f;
	OnStateChanged.Broadcast(this, NewState);

	const ATDShip* Ship = Cast<ATDShip>(GetOwner());
	if (Ship && Ship->IsPlayerFlown())
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(this))
		{
			Bus->BroadcastHyperdriveState(NewState);
		}
	}
}

bool UTDHyperdriveComponent::PickDestination(const ATDSpacecraft* Craft)
{
	bHasDestination = false;
	DestinationWell = NAME_None;
	if (!Craft) { return false; }
	const FVector From = Craft->GetActorLocation();
	const FVector Nose = Craft->GetActorForwardVector();
	const float CosCone = FMath::Cos(FMath::DegreesToRadians(Tuning.DestinationConeDeg));
	float BestCos = CosCone;
	// A gravity well ahead (planets, the Dominion): drop out at its edge.
	if (const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		for (const FTDTravelGravityWell& W : Travel->GetGravityWells())
		{
			const FVector Centre = W.Owner.IsValid() ? W.Owner->GetActorLocation() : W.Location;
			const FVector To = Centre - From;
			const float Dist = To.Size();
			if (Dist <= W.Radius + 1000.f) { continue; } // already there
			const float C = FVector::DotProduct(To / Dist, Nose);
			if (C >= BestCos)
			{
				BestCos = C;
				bHasDestination = true;
				DestinationWell = W.Id;
				DestinationPoint = Centre - To / Dist * W.Radius;
			}
		}
	}
	// Else the objective marker ahead: drop out MarkerStandOff short of it.
	if (!bHasDestination)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(TDTags::ObjectiveMarker)) { continue; }
			const FVector To = It->GetActorLocation() - From;
			const float Dist = To.Size();
			if (Dist <= Tuning.MarkerStandOff + 5000.f) { continue; }
			const float C = FVector::DotProduct(To / Dist, Nose);
			if (C >= BestCos)
			{
				BestCos = C;
				bHasDestination = true;
				DestinationPoint = It->GetActorLocation() - To / Dist * Tuning.MarkerStandOff;
			}
		}
	}
	if (!bHasDestination) { return false; }
	const float Distance = FVector::Dist(From, DestinationPoint);
	DestinationJumpSpeed = FMath::Max(Tuning.JumpSpeed, Distance / FMath::Max(Tuning.DestinationMaxTunnelSeconds, 0.5f));
	DestinationTunnelSeconds = Distance / DestinationJumpSpeed;
	return true;
}

void UTDHyperdriveComponent::BeginJump()
{
	ATDSpacecraft* Craft = GetCraft();
	if (!Craft)
	{
		SetState(ETDHyperdriveState::Idle);
		return;
	}
	StopLoopFX();
	LastDropOutWell = NAME_None;
	LastLocation = Craft->GetActorLocation();
	LastEndCause = ETDHyperdriveEndCause::None;
	// One jump to a marked destination (Ryan): the nose is laid on it and the speed set so the tunnel lasts at most
	// DestinationMaxTunnelSeconds from anywhere on the map. Nothing ahead: a free jump, as before.
	const bool bDest = PickDestination(Craft);
	if (bDest)
	{
		Craft->TeleportSpacecraft(Craft->GetActorLocation(), (DestinationPoint - Craft->GetActorLocation()).Rotation(), true);
	}
	if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
	{
		Move->SetSpeedOverrideFor(OverrideOwner, bDest ? DestinationJumpSpeed : Tuning.JumpSpeed, bDest ? 0.f : Tuning.JumpSteeringScale, bDest ? DestinationJumpSpeed * 4.f : Tuning.JumpAcceleration);
	}
	SetState(ETDHyperdriveState::Jumping);
	if (TravelFX)
	{
		StartLoopFX(TravelFX->HyperdriveTunnel);
		// For T7's tunnel: how long and how fast (Niagara user floats; ignored when the system lacks them).
		UAudioComponent* NoAudio = nullptr;
		FTDFXCue Streaks = TravelFX->HyperdriveStreaks;
		Streaks.Sound.Reset(); // the tunnel cue carries the jump's sound
		StreaksFX = UTDStatics::PlayCueAttachedLoop(Streaks, Craft->GetRootComponent(), NAME_None, NoAudio);
		for (UNiagaraComponent* FX : { LoopFX.Get(), StreaksFX.Get() })
		{
			if (!FX) { continue; }
			FX->SetVariableFloat(TEXT("User.TunnelSeconds"), bDest ? DestinationTunnelSeconds : Tuning.MaxJumpSeconds);
			FX->SetVariableFloat(TEXT("User.JumpSpeed"), bDest ? DestinationJumpSpeed : Tuning.JumpSpeed);
		}
	}
}

void UTDHyperdriveComponent::BeginDropOut(FName WellId, ETDHyperdriveEndCause Cause, bool bBurst)
{
	ATDSpacecraft* Craft = GetCraft();
	StopLoopFX();
	LastDropOutWell = WellId;
	LastDropOutTime = Now();
	LastEndCause = Cause;
	if (Craft)
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			// Short of an obstacle there's no burst: snap straight down to cruise.
			// The longer boost out only after reaching the marker's stand-off: a well's edge has a planet or the Dominion
			// behind it, so a well arrival keeps the short burst (it must not carry the ship on into the hull).
			const bool bBoostOut = Cause == ETDHyperdriveEndCause::Arrived;
			const float Speed = bBurst ? FMath::Max(Tuning.DropOutSpeed, bBoostOut ? Move->GetFlightTuning().BoostSpeed * 1.4f : 0.f) : Move->GetFlightTuning().CruiseSpeed;
			Move->SetSpeedOverrideFor(OverrideOwner, Speed, 1.f, Tuning.DropOutDeceleration);
		}
		if (TravelFX)
		{
			UTDStatics::PlayCue(this, TravelFX->HyperdriveDropOut, Craft->GetActorLocation(), Craft->GetActorRotation());
		}
	}
	SetState(ETDHyperdriveState::DropOut);
	if (Tuning.DropOutSeconds <= 0.f)
	{
		FinishDropOut();
	}
}

void UTDHyperdriveComponent::FinishDropOut()
{
	if (ATDSpacecraft* Craft = GetCraft())
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->ClearSpeedOverrideFor(OverrideOwner); // only ours: a ram / entry underneath is restored
		}
	}
	SetState(ETDHyperdriveState::Idle);
}

void UTDHyperdriveComponent::StartLoopFX(const FTDFXCue& Cue)
{
	StopLoopFX();
	const ATDSpacecraft* Craft = GetCraft();
	if (!Craft)
	{
		return;
	}
	UAudioComponent* Audio = nullptr;
	LoopFX = UTDStatics::PlayCueAttachedLoop(Cue, Craft->GetRootComponent(), NAME_None, Audio);
	LoopAudio = Audio;
}

void UTDHyperdriveComponent::StopLoopFX()
{
	if (UNiagaraComponent* S = StreaksFX.Get()) { S->Deactivate(); }
	StreaksFX.Reset();
	if (UNiagaraComponent* FX = LoopFX.Get())
	{
		FX->Deactivate();
	}
	if (UAudioComponent* Audio = LoopAudio.Get())
	{
		Audio->FadeOut(0.2f, 0.f); // the spool whine / tunnel roar must stop with the state
	}
	LoopFX.Reset();
	LoopAudio.Reset();
}

void UTDHyperdriveComponent::HandleDamaged(UTDHealthComponent* HealthComp, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	LastDamageTime = Now();
	CancelSpool(); // a hit cancels the spool
}

void UTDHyperdriveComponent::EndForPilotLoss()
{
	switch (State)
	{
	case ETDHyperdriveState::Spooling:
		CancelSpool();
		break;
	case ETDHyperdriveState::Jumping:
	case ETDHyperdriveState::DropOut:
		StopLoopFX();
		LastDropOutTime = Now();
		LastEndCause = ETDHyperdriveEndCause::PilotLoss;
		FinishDropOut(); // clears the override (the flight model bleeds 80k off at HighOverspeedDeceleration) -> Idle
		break;
	default:
		break;
	}
}

void UTDHyperdriveComponent::ResetHyperdrive()
{
	StopLoopFX();
	if (State != ETDHyperdriveState::Idle)
	{
		if (ATDSpacecraft* Craft = GetCraft())
		{
			if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
			{
				Move->ClearSpeedOverrideFor(OverrideOwner);
			}
		}
	}
	Charge = 0.f;
	bEnabled = true;
	LastDamageTime = -1000.0;
	LastDropOutTime = -1000.0;
	LastDropOutWell = NAME_None;
	SetState(ETDHyperdriveState::Idle);
}

void UTDHyperdriveComponent::HandleTeleported(ATDSpacecraft* Spacecraft, FVector FromLocation, FVector ToLocation, FRotator NewFacing)
{
	LastLocation = ToLocation; // a portal mid-jump isn't a path through every well in between
}

void UTDHyperdriveComponent::HandleDeath(UTDHealthComponent* HealthComp, const FTDDamageInfo& KillingBlow)
{
	EndForPilotLoss();
}

void UTDHyperdriveComponent::HandleImpact(const FTDFlightImpact& Impact)
{
	if (State == ETDHyperdriveState::Jumping)
	{
		BeginDropOut(NAME_None, ETDHyperdriveEndCause::Impact, false); // slammed into something mid-jump
	}
}

void UTDHyperdriveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (DeltaTime <= 0.f)
	{
		return;
	}
	StateTime += DeltaTime;
	ATDSpacecraft* Craft = GetCraft();

	switch (State)
	{
	case ETDHyperdriveState::Spooling:
	{
		// Conditions can change mid-spool (entered atmosphere, flew into a well, got disabled).
		const ETDHyperdriveBlock Block = GetBlockReason();
		if (Block != ETDHyperdriveBlock::None && Block != ETDHyperdriveBlock::Cooldown)
		{
			CancelSpool();
			break;
		}
		Charge += DeltaTime / FMath::Max(Tuning.SpoolSeconds, KINDA_SMALL_NUMBER);
		if (Charge >= 1.f)
		{
			Charge = 1.f;
			BeginJump();
		}
		break;
	}
	case ETDHyperdriveState::Jumping:
	{
		if (!Craft)
		{
			SetState(ETDHyperdriveState::Idle);
			break;
		}
		const FVector Loc = Craft->GetActorLocation();
		// Check the path flown this frame plus the next frame, so 80,000 cm/s can't skip a well.
		const FVector Ahead = Loc + Craft->GetVelocity() * DeltaTime;
		FTDTravelGravityWell Well;
		FText LockReason;
		const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
		// Look far enough ahead to brake from jump speed to cruise before reaching it.
		const float LookAhead = Tuning.LookAheadDistance + Craft->GetVelocity().Size() * DeltaTime;
		// A destination jump brakes inside the tunnel: never faster than what still stops at the drop-out point, so the
		// ship arrives at the well's edge (or the marker's stand-off), not inside it, however far it came.
		if (bHasDestination)
		{
			if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
			{
				const float Left = FMath::Max(0.f, (float)FVector::DotProduct(DestinationPoint - Loc, Craft->GetActorForwardVector()));
				const float Stop = FMath::Sqrt(2.f * Tuning.DropOutDeceleration * Left) + Tuning.DropOutSpeed;
				Move->SetSpeedOverrideFor(OverrideOwner, FMath::Clamp(Stop, Tuning.DropOutSpeed, DestinationJumpSpeed), 0.f, Tuning.DropOutDeceleration);
			}
		}
		const bool bReachedMarker = bHasDestination && DestinationWell.IsNone()
			&& FVector::DotProduct(DestinationPoint - Ahead, Craft->GetActorForwardVector()) <= 0.f;
		if (Travel && Travel->FindWellOnSegment(LastLocation, Ahead, Well))
		{
			BeginDropOut(Well.Id, ETDHyperdriveEndCause::GravityWell);
		}
		else if (bReachedMarker)
		{
			BeginDropOut(NAME_None, ETDHyperdriveEndCause::Arrived);
		}
		else if (SweepAhead(LookAhead) >= 0.f)
		{
			BeginDropOut(NAME_None, ETDHyperdriveEndCause::Obstacle, false);
		}
		else if (Travel && Travel->IsHyperdriveLocked(LockReason))
		{
			BeginDropOut(NAME_None, ETDHyperdriveEndCause::Locked);
		}
		else if (Tuning.bDropOutAtBoundary && Craft->GetShipMovement() && Craft->GetShipMovement()->IsNearBoundary()
			&& FVector::DotProduct(Craft->GetVelocity(), Craft->GetActorLocation() - Craft->GetShipMovement()->GetBoundaryCenter()) > 0.f)
		{
			BeginDropOut(NAME_None, ETDHyperdriveEndCause::Boundary);
		}
		else if (StateTime >= (bHasDestination ? DestinationTunnelSeconds + 1.f : Tuning.MaxJumpSeconds))
		{
			BeginDropOut(NAME_None, ETDHyperdriveEndCause::Timeout);
		}
		LastLocation = Loc;
		break;
	}
	case ETDHyperdriveState::DropOut:
		if (StateTime >= (LastEndCause == ETDHyperdriveEndCause::Arrived ? Tuning.DestinationBoostOutSeconds : Tuning.DropOutSeconds))
		{
			FinishDropOut();
		}
		break;
	default:
		break;
	}
}

// Triad Drift - hyperdrive (ENG-15). Owner: Teammate 1.

#include "TriadDrift/Travel/TDHyperdriveComponent.h"
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
	if (Tuning.ObstacleClearanceToSpool > 0.f && SweepAhead(Tuning.ObstacleClearanceToSpool) >= 0.f)
	{
		return ETDHyperdriveBlock::ObstacleAhead;
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
	switch (GetBlockReason())
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
		OnSpoolRefused.Broadcast(this, Block, GetBlockReasonText());
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
	if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
	{
		Move->SetSpeedOverrideFor(OverrideOwner, Tuning.JumpSpeed, Tuning.JumpSteeringScale, Tuning.JumpAcceleration);
	}
	SetState(ETDHyperdriveState::Jumping);
	if (TravelFX)
	{
		StartLoopFX(TravelFX->HyperdriveTunnel);
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
			const float Speed = bBurst ? Tuning.DropOutSpeed : Move->GetFlightTuning().CruiseSpeed;
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
		if (Travel && Travel->FindWellOnSegment(LastLocation, Ahead, Well))
		{
			BeginDropOut(Well.Id, ETDHyperdriveEndCause::GravityWell);
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
		else if (StateTime >= Tuning.MaxJumpSeconds)
		{
			BeginDropOut(NAME_None, ETDHyperdriveEndCause::Timeout);
		}
		LastLocation = Loc;
		break;
	}
	case ETDHyperdriveState::DropOut:
		if (StateTime >= Tuning.DropOutSeconds)
		{
			FinishDropOut();
		}
		break;
	default:
		break;
	}
}

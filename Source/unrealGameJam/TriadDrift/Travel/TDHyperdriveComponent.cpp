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
#include "Engine/World.h"

UTDHyperdriveComponent::UTDHyperdriveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTDHyperdriveComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ATDSpacecraft* Craft = GetCraft())
	{
		if (UTDHealthComponent* Health = Craft->GetHealth())
		{
			Health->OnDamaged.AddDynamic(this, &UTDHyperdriveComponent::HandleDamaged);
		}
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->OnFlightImpact.AddDynamic(this, &UTDHyperdriveComponent::HandleImpact);
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
		FTDGravityWell Well;
		if (Travel->FindWellContaining(Craft->GetActorLocation(), Well))
		{
			return ETDHyperdriveBlock::NearGravityWell;
		}
	}
	return ETDHyperdriveBlock::None;
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
			LoopFX = UTDStatics::PlayCueAttached(TravelFX->HyperdriveSpool, Craft->GetRootComponent());
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
		BeginDropOut(NAME_None);
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
			BeginDropOut(NAME_None);
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
	Craft->SetSpeedOverride(Tuning.JumpSpeed, Tuning.JumpSteeringScale);
	if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
	{
		Move->SetOverrideAcceleration(Tuning.JumpAcceleration);
	}
	SetState(ETDHyperdriveState::Jumping);
	if (TravelFX)
	{
		LoopFX = UTDStatics::PlayCueAttached(TravelFX->HyperdriveTunnel, Craft->GetRootComponent());
	}
}

void UTDHyperdriveComponent::BeginDropOut(FName WellId)
{
	ATDSpacecraft* Craft = GetCraft();
	StopLoopFX();
	LastDropOutWell = WellId;
	LastDropOutTime = Now();
	if (Craft)
	{
		Craft->SetSpeedOverride(Tuning.DropOutSpeed, 1.f);
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->SetOverrideAcceleration(Tuning.JumpAcceleration);
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
		Craft->ClearSpeedOverride();
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->SetOverrideAcceleration(-1.f); // back to the default
		}
	}
	SetState(ETDHyperdriveState::Idle);
}

void UTDHyperdriveComponent::StopLoopFX()
{
	if (UNiagaraComponent* FX = LoopFX.Get())
	{
		FX->Deactivate();
	}
	LoopFX.Reset();
}

void UTDHyperdriveComponent::HandleDamaged(UTDHealthComponent* HealthComp, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	LastDamageTime = Now();
	CancelSpool(); // a hit cancels the spool
}

void UTDHyperdriveComponent::HandleImpact(const FTDFlightImpact& Impact)
{
	if (State == ETDHyperdriveState::Jumping)
	{
		BeginDropOut(NAME_None); // slammed into something mid-jump
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
		FTDGravityWell Well;
		const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
		if (Travel && Travel->FindWellOnSegment(LastLocation, Ahead, Well))
		{
			BeginDropOut(Well.Id);
		}
		else if (Tuning.bDropOutAtBoundary && Craft->GetShipMovement() && Craft->GetShipMovement()->IsNearBoundary())
		{
			BeginDropOut(NAME_None);
		}
		else if (StateTime >= Tuning.MaxJumpSeconds)
		{
			BeginDropOut(NAME_None);
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

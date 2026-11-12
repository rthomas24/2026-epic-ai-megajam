// Triad Drift - ship movement component (ENG-01). Owner: Teammate 1 (Flight & Input).

#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"

UTDShipMovementComponent::UTDShipMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	bConstrainToPlane = false;
	bUpdateOnlyIfRendered = false;
	bAutoActivate = true;
}

void UTDShipMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	SyncStateFromComponent();
	if (!bStateInitialised)
	{
		bStateInitialised = true;
		SetForwardSpeedImmediate(Tuning.CruiseSpeed * StartSpeedFraction);
	}
}

void UTDShipMovementComponent::SetFlightInput(const FTDFlightInput& In)
{
	Input = TDFlightSim::Sanitize(In);
}

void UTDShipMovementComponent::ApplyFlightTuning(const FTDFlightTuning& NewTuning)
{
	Tuning = NewTuning;
	Tuning.MinSpeed = FMath::Max(0.f, Tuning.MinSpeed);
	Tuning.CruiseSpeed = FMath::Max(Tuning.MinSpeed, Tuning.CruiseSpeed);
	Tuning.BoostSpeed = FMath::Max(Tuning.CruiseSpeed, Tuning.BoostSpeed);

	// Clamp a plain cruise down to the new top speed (only ever down; intentional extra speed is kept).
	const bool bExtraSpeedIntended = State.bBoosting || State.bDrifting || State.bBurstOverspeed
		|| Context.bSpeedOverride || Context.ThrottleTopSpeed > Tuning.CruiseSpeed;
	const float Top = Context.bAtmosphere ? Tuning.CruiseSpeed * Context.AtmosphereSpeedCapFraction : Tuning.CruiseSpeed;
	if (!bExtraSpeedIntended && State.ForwardSpeed > Top)
	{
		const float Speed = State.Velocity.Size();
		State.ForwardSpeed = Top;
		if (Speed > Top && Speed > KINDA_SMALL_NUMBER)
		{
			State.Velocity *= Top / Speed;
			State.StrafeVelocity = FVector2D::ZeroVector;
			Velocity = State.Velocity;
			LastWrittenVelocity = Velocity;
			UpdateComponentVelocity();
		}
	}
}

void UTDShipMovementComponent::ApplyOverrideStack()
{
	Context.bSpeedOverride = OverrideStack.Num() > 0;
	if (Context.bSpeedOverride)
	{
		const FSpeedOverrideRequest& Top = OverrideStack.Last();
		Context.OverrideSpeed = Top.Speed;
		Context.OverrideSteeringScale = Top.Steering;
		Context.OverrideAcceleration = Top.Acceleration >= 0.f ? Top.Acceleration : FTDFlightContext().OverrideAcceleration;
	}
	else
	{
		Context.OverrideSteeringScale = 1.f;
		Context.OverrideAcceleration = FTDFlightContext().OverrideAcceleration;
	}
}

void UTDShipMovementComponent::SetSpeedOverrideFor(FName Owner, float SpeedCmPerSec, float SteeringScale, float AccelerationCmPerSec2)
{
	OverrideStack.RemoveAll([Owner](const FSpeedOverrideRequest& R) { return R.Owner == Owner; });
	FSpeedOverrideRequest Request;
	Request.Owner = Owner;
	Request.Speed = FMath::Max(0.f, SpeedCmPerSec);
	Request.Steering = FMath::Max(0.f, SteeringScale);
	Request.Acceleration = AccelerationCmPerSec2;
	OverrideStack.Add(Request);
	ApplyOverrideStack();
}

void UTDShipMovementComponent::ClearSpeedOverrideFor(FName Owner)
{
	OverrideStack.RemoveAll([Owner](const FSpeedOverrideRequest& R) { return R.Owner == Owner; });
	ApplyOverrideStack();
}

bool UTDShipMovementComponent::HasSpeedOverrideFor(FName Owner) const
{
	return OverrideStack.ContainsByPredicate([Owner](const FSpeedOverrideRequest& R) { return R.Owner == Owner; });
}

void UTDShipMovementComponent::EndBoostAndDrift()
{
	if (State.bBoosting)
	{
		State.bBoosting = false;
		State.TimeSinceBoost = 0.f;
		OnBoostEnded.Broadcast();
	}
	if (State.bDrifting)
	{
		State.bDrifting = false;
		OnDriftReleased.Broadcast(false);
	}
	Input.bBoost = false;
	Input.bDrift = false;
}

void UTDShipMovementComponent::ClearAllSpeedOverrides()
{
	OverrideStack.Reset();
	LegacyOverrideAcceleration = -1.f;
	ApplyOverrideStack();
}

void UTDShipMovementComponent::SetSpeedOverride(float SpeedCmPerSec, float SteeringScale)
{
	SetSpeedOverrideFor(NAME_None, SpeedCmPerSec, SteeringScale, LegacyOverrideAcceleration);
}

void UTDShipMovementComponent::ClearSpeedOverride()
{
	ClearSpeedOverrideFor(NAME_None);
}

void UTDShipMovementComponent::SetOverrideAcceleration(float AccelerationCmPerSec2)
{
	LegacyOverrideAcceleration = AccelerationCmPerSec2;
	for (FSpeedOverrideRequest& R : OverrideStack)
	{
		if (R.Owner == NAME_None)
		{
			R.Acceleration = AccelerationCmPerSec2;
		}
	}
	ApplyOverrideStack();
}

void UTDShipMovementComponent::SetBoundary(const FVector& Center, float Radius, bool bEnabled)
{
	Context.BoundaryCenter = Center;
	Context.bBoundaryEnabled = bEnabled;
	Context.BoundaryRadius = FMath::Max(0.f, Radius);
}

void UTDShipMovementComponent::SetBoxBoundary(const FBox& Box)
{
	Context.bBoxBoundary = Box.IsValid != 0;
	Context.BoundaryBox = Box;
	Context.bBoundaryEnabled = true;
}

void UTDShipMovementComponent::ClearBoxBoundary()
{
	Context.bBoxBoundary = false;
}

void UTDShipMovementComponent::SetAtmosphereMode(bool bEnabled, float Gravity, float SpeedCapFraction)
{
	Context.bAtmosphere = bEnabled;
	Context.AtmosphereGravity = FMath::Max(0.f, Gravity);
	Context.AtmosphereSpeedCapFraction = FMath::Clamp(SpeedCapFraction, 0.05f, 1.f);
}

void UTDShipMovementComponent::ApplyWorldPlayBounds()
{
	const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
	if (!Travel || Travel->GetPlayBoundsVersion() == AppliedPlayBoundsVersion)
	{
		return;
	}
	AppliedPlayBoundsVersion = Travel->GetPlayBoundsVersion();
	FVector Center;
	float Radius;
	if (Travel->GetPlayBounds(Center, Radius))
	{
		Context.BoundaryCenter = Center;
		Context.BoundaryRadius = Radius;
		Context.bBoundaryEnabled = true;
		bWorldBoundsApplied = true;
	}
	else if (bWorldBoundsApplied)
	{
		Context.BoundaryRadius = -1.f; // the world cleared its bounds: back to the tuning default
	}
}

void UTDShipMovementComponent::NotifyTeleported(const FQuat& NewFacing, bool bKeepSpeed)
{
	SyncStateFromComponent();
	State.Rotation = NewFacing.GetNormalized();
	const float Speed = bKeepSpeed ? FMath::Max(Velocity.Size(), State.ForwardSpeed) : Tuning.CruiseSpeed;
	State.ForwardSpeed = Speed;
	State.StrafeVelocity = FVector2D::ZeroVector;
	State.Velocity = State.Rotation.GetForwardVector() * Speed;
	if (State.bDrifting)
	{
		// Keep the drift going through the portal (no silent end / re-start), now locked to the new facing.
		State.DriftDirection = State.Rotation.GetForwardVector();
		State.DriftStartForward = State.DriftDirection;
	}
	Velocity = State.Velocity;
	LastWrittenVelocity = Velocity;
}

void UTDShipMovementComponent::AddImpulseVelocity(const FVector& DeltaVelocity)
{
	if (!DeltaVelocity.ContainsNaN())
	{
		State.Velocity += DeltaVelocity;
		if (State.bDrifting)
		{
			State.DriftDirection = State.Velocity.GetSafeNormal(UE_SMALL_NUMBER, State.DriftDirection);
		}
		Velocity = State.Velocity;
		LastWrittenVelocity = Velocity;
	}
}

void UTDShipMovementComponent::ResetFlightState(bool bKeepVelocity)
{
	SyncStateFromComponent();
	const bool bWasBoosting = State.bBoosting;
	const bool bWasDrifting = State.bDrifting;

	Input = FTDFlightInput();
	State.Boost01 = 1.f;
	State.TimeSinceBoost = 1000.f;
	State.bBoosting = false;
	State.bBoostLockout = false;
	State.bDrifting = false;
	State.bBurstOverspeed = false;
	State.bDriftBurstThisStep = false;
	State.bDriftEndedThisStep = false;
	State.AngularRate = FVector::ZeroVector;
	State.StrafeVelocity = FVector2D::ZeroVector;
	State.VisualBank = 0.f;
	ClearAllSpeedOverrides();
	Context.ThrottleTopSpeed = 0.f;

	if (bKeepVelocity)
	{
		State.ForwardSpeed = State.Velocity.Size();
	}
	else
	{
		SetForwardSpeedImmediate(Tuning.CruiseSpeed * StartSpeedFraction);
	}
	if (bWasBoosting) { OnBoostEnded.Broadcast(); }
	if (bWasDrifting) { OnDriftReleased.Broadcast(false); }
}

void UTDShipMovementComponent::SetVelocityImmediate(const FVector& NewVelocity)
{
	if (NewVelocity.ContainsNaN())
	{
		return;
	}
	bStateInitialised = true;
	State.Velocity = NewVelocity;
	State.ForwardSpeed = NewVelocity.Size();
	State.StrafeVelocity = FVector2D::ZeroVector;
	if (State.bDrifting)
	{
		State.DriftDirection = NewVelocity.GetSafeNormal(UE_SMALL_NUMBER, State.DriftDirection);
	}
	Velocity = NewVelocity;
	LastWrittenVelocity = Velocity;
}

void UTDShipMovementComponent::SetForwardSpeedImmediate(float SpeedCmPerSec)
{
	SyncStateFromComponent();
	bStateInitialised = true;
	State.ForwardSpeed = FMath::Max(0.f, SpeedCmPerSec);
	State.StrafeVelocity = FVector2D::ZeroVector;
	State.Velocity = State.Rotation.GetForwardVector() * State.ForwardSpeed;
	Velocity = State.Velocity;
	LastWrittenVelocity = Velocity;
}

float UTDShipMovementComponent::GetSpeed01() const
{
	const float Range = Tuning.BoostSpeed - Tuning.MinSpeed;
	return Range > KINDA_SMALL_NUMBER ? FMath::Clamp((Velocity.Size() - Tuning.MinSpeed) / Range, 0.f, 1.f) : 0.f;
}

float UTDShipMovementComponent::GetMaxSpeed() const
{
	return FMath::Max(Tuning.BoostSpeed, Context.bSpeedOverride ? Context.OverrideSpeed : 0.f);
}

void UTDShipMovementComponent::StopMovementImmediately()
{
	Super::StopMovementImmediately();
	State.Velocity = FVector::ZeroVector;
	State.ForwardSpeed = 0.f;
	State.StrafeVelocity = FVector2D::ZeroVector;
	State.AngularRate = FVector::ZeroVector;
	LastWrittenVelocity = FVector::ZeroVector;
}

void UTDShipMovementComponent::SyncStateFromComponent()
{
	if (UpdatedComponent)
	{
		State.Location = UpdatedComponent->GetComponentLocation();
		State.Rotation = UpdatedComponent->GetComponentQuat();
	}
	// Someone wrote Velocity directly (knockback etc.): adopt it.
	if (!Velocity.Equals(LastWrittenVelocity, 0.01f))
	{
		State.Velocity = Velocity;
		if (State.bDrifting)
		{
			State.DriftDirection = State.Velocity.GetSafeNormal(UE_SMALL_NUMBER, State.DriftDirection);
		}
		LastWrittenVelocity = Velocity;
	}
}

void UTDShipMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!UpdatedComponent || ShouldSkipUpdate(DeltaTime) || DeltaTime <= 0.f)
	{
		return;
	}

	ApplyWorldPlayBounds();
	SyncStateFromComponent();
	const FTDFlightState Before = State;
	const FVector StartLocation = State.Location;

	TDFlightSim::Advance(State, Input, Tuning, Context, DeltaTime);

	const FVector Delta = State.Location - StartLocation;
	State.Location = StartLocation;
	ResolveMove(Delta, FMath::Min(DeltaTime, 0.25f));

	State.Location = UpdatedComponent->GetComponentLocation();
	Velocity = State.Velocity;
	LastWrittenVelocity = Velocity;
	UpdateComponentVelocity();

	BroadcastStateChanges(Before);
}

void UTDShipMovementComponent::ResolveMove(const FVector& Delta, float DeltaTime)
{
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, State.Rotation, true, Hit);
	if (!Hit.IsValidBlockingHit())
	{
		return;
	}

	HandleShipImpact(Hit, DeltaTime);

	// Spend the rest of the frame moving with the post-impact velocity (one extra iteration).
	const float Remaining = FMath::Clamp(1.f - Hit.Time, 0.f, 1.f);
	if (Remaining > KINDA_SMALL_NUMBER)
	{
		FHitResult Hit2(1.f);
		const FVector Rest = State.Velocity * DeltaTime * Remaining;
		SafeMoveUpdatedComponent(Rest, State.Rotation, true, Hit2);
		if (Hit2.IsValidBlockingHit())
		{
			SlideAlongSurface(Rest, 1.f - Hit2.Time, Hit2.Normal, Hit2, true);
		}
	}
}

void UTDShipMovementComponent::HandleShipImpact(const FHitResult& Hit, float DeltaTime)
{
	const FVector Normal = Hit.Normal.GetSafeNormal();
	const FVector V = State.Velocity;
	const float Speed = V.Size();
	const float Into = -FVector::DotProduct(V, Normal);
	if (Speed < KINDA_SMALL_NUMBER || Into <= 0.f || Normal.IsNearlyZero())
	{
		return;
	}
	if (Into < Tuning.MinImpactSpeed)
	{
		// Resting / pinned contact: just stop pushing into the surface. No event, no damage, no shield-regen reset.
		State.Velocity = V + Normal * Into;
		return;
	}

	const UWorld* ContactWorld = GetWorld();
	const double ContactNow = ContactWorld ? ContactWorld->GetTimeSeconds() : 0.0;
	const bool bResting = Hit.GetActor() && LastImpactActor.Get() == Hit.GetActor() && ContactNow - LastImpactTime < Tuning.RestingContactWindow;
	LastImpactActor = Hit.GetActor();
	LastImpactTime = ContactNow;
	if (bResting)
	{
		// Pinned against the same thing (holding throttle into a wall): slide, hold the engine down, no damage.
		State.Velocity = V + Normal * Into;
		State.ForwardSpeed = FMath::Min(State.ForwardSpeed, Tuning.MinSpeed);
		return;
	}

	FTDFlightImpact Impact;
	Impact.OtherActor = Hit.GetActor();
	Impact.ImpactPoint = Hit.ImpactPoint;
	Impact.ImpactNormal = Normal;
	Impact.ImpactSpeed = Into;
	Impact.HeadOn01 = FMath::Clamp(Into / Speed, 0.f, 1.f);
	Impact.bHeadOn = Impact.HeadOn01 >= Tuning.HeadOnThreshold;

	const float CruiseRef = FMath::Max(Tuning.CruiseSpeed, 1.f);
	float Damage;
	if (Impact.bHeadOn)
	{
		// Bounce straight back off the surface, losing most of the speed.
		State.Velocity = V + Normal * Into * (1.f + Tuning.HeadOnRestitution);
		State.Velocity *= Tuning.HeadOnSpeedRetain;
		// Hold the engine down after a head-on bounce so full throttle doesn't ram it straight back in.
		State.ForwardSpeed = FMath::Min(State.ForwardSpeed, FMath::Min(State.Velocity.Size(), Tuning.MinSpeed));
		Damage = Tuning.HeadOnDamage * (FMath::Min(Into, Tuning.BoostSpeed) / CruiseRef);
	}
	else
	{
		// Scrape: keep most tangential speed, pop slightly off the surface, turn the nose along it.
		const FVector Tangent = V + Normal * Into;
		State.Velocity = Tangent * Tuning.ScrapeFriction + Normal * Tuning.ScrapeBounceSpeed;
		State.ForwardSpeed *= Tuning.ScrapeFriction;

		const FVector TangentDir = Tangent.GetSafeNormal();
		if (!TangentDir.IsNearlyZero() && !State.bDrifting)
		{
			const FQuat Along = FQuat::FindBetweenNormals(State.Rotation.GetForwardVector(), TangentDir);
			State.Rotation = (FQuat::Slerp(FQuat::Identity, Along, Tuning.ScrapeNoseAlign) * State.Rotation).GetNormalized();
			UpdatedComponent->SetWorldRotation(State.Rotation);
		}
		const float Angle01 = Tuning.HeadOnThreshold > 0.f ? Impact.HeadOn01 / Tuning.HeadOnThreshold : 1.f;
		Damage = Tuning.GlancingDamage * Angle01 * (FMath::Min(Speed, Tuning.BoostSpeed) / CruiseRef);
	}

	if (State.bDrifting)
	{
		State.DriftDirection = State.Velocity.GetSafeNormal(UE_SMALL_NUMBER, State.DriftDirection);
	}

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastCollisionDamageTime >= Tuning.CollisionDamageCooldown)
	{
		LastCollisionDamageTime = Now;
		Impact.Damage = FMath::Min(Damage, Tuning.MaxImpactDamage);
	}

	OnFlightImpact.Broadcast(Impact);
}

void UTDShipMovementComponent::BroadcastStateChanges(const FTDFlightState& Before)
{
	if (State.bBoosting != Before.bBoosting)
	{
		State.bBoosting ? OnBoostStarted.Broadcast() : OnBoostEnded.Broadcast();
	}
	if (State.bDrifting && !Before.bDrifting)
	{
		OnDriftStarted.Broadcast();
	}
	if (State.bDriftEndedThisStep && Before.bDrifting)
	{
		OnDriftReleased.Broadcast(State.bDriftBurstThisStep);
	}
}

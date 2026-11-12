// Triad Drift - ship movement component (ENG-01). Owner: Teammate 1 (Flight & Input).

#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

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
}

void UTDShipMovementComponent::SetSpeedOverride(float SpeedCmPerSec, float SteeringScale)
{
	Context.bSpeedOverride = true;
	Context.OverrideSpeed = FMath::Max(0.f, SpeedCmPerSec);
	Context.OverrideSteeringScale = FMath::Max(0.f, SteeringScale);
}

void UTDShipMovementComponent::ClearSpeedOverride()
{
	Context.bSpeedOverride = false;
}

void UTDShipMovementComponent::SetBoundary(const FVector& Center, float Radius, bool bEnabled)
{
	Context.BoundaryCenter = Center;
	Context.bBoundaryEnabled = bEnabled;
	Tuning.BoundaryRadius = FMath::Max(0.f, Radius);
}

void UTDShipMovementComponent::NotifyTeleported(const FQuat& NewFacing, bool bKeepSpeed)
{
	SyncStateFromComponent();
	State.Rotation = NewFacing.GetNormalized();
	const float Speed = bKeepSpeed ? FMath::Max(Velocity.Size(), State.ForwardSpeed) : Tuning.CruiseSpeed;
	State.ForwardSpeed = Speed;
	State.StrafeVelocity = FVector2D::ZeroVector;
	State.bDrifting = false;
	State.Velocity = State.Rotation.GetForwardVector() * Speed;
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
		State.Velocity *= 0.5f;
		State.ForwardSpeed = FMath::Min(State.ForwardSpeed, State.Velocity.Size());
		Damage = Tuning.HeadOnDamage * (Into / CruiseRef);
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
			State.Rotation = (FQuat::Slerp(FQuat::Identity, Along, 0.5f) * State.Rotation).GetNormalized();
			UpdatedComponent->SetWorldRotation(State.Rotation);
		}
		const float Angle01 = Tuning.HeadOnThreshold > 0.f ? Impact.HeadOn01 / Tuning.HeadOnThreshold : 1.f;
		Damage = Tuning.GlancingDamage * Angle01 * (Speed / CruiseRef);
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
		Impact.Damage = Damage;
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
	if (State.bDriftBurstThisStep)
	{
		OnDriftReleased.Broadcast();
	}
}

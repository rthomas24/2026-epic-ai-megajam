// Triad Drift - ship movement component (ENG-01). Owner: Teammate 1 (Flight & Input).

#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TriadDrift/Travel/TDTravelBarks.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipImpactReceiver.h"
#include "TriadDrift/Core/TDTypes.h"
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
	if (UpdatedComponent)
	{
		WatchLastLocation = UpdatedComponent->GetComponentLocation();
		bWatchReady = true;
		UpdatedComponent->TransformUpdated.AddUObject(this, &UTDShipMovementComponent::HandleTransformUpdated);
	}
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
	const float Top = GetCruiseCapSpeed();
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

void UTDShipMovementComponent::SetBoxBoundary(const FBox& Box, float WallWarnDistance)
{
	Context.bBoxBoundary = Box.IsValid != 0;
	Context.BoundaryBox = Box;
	Context.BoxWallWarnDistance = WallWarnDistance;
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

	MeasureGroundClearance();
	if (BoundaryReleases.Num() > 0)
	{
		FTDFlightContext Released = Context; // a scripted corridor past the edge (SetBoundaryReleaseFor): no containment
		Released.bBoundaryEnabled = false;
		TDFlightSim::Advance(State, Input, Tuning, Released, DeltaTime);
	}
	else
	{
		TDFlightSim::Advance(State, Input, Tuning, Context, DeltaTime);
	}
	if (bPitchHold)
	{
		// ENG-38 glide: the nose's pitch is held (the path), yaw and roll stay the pilot's; the next step flies along it.
		FRotator R = State.Rotation.Rotator();
		R.Pitch = FMath::FInterpConstantTo(R.Pitch, PitchHoldDeg, DeltaTime, PitchHoldRate);
		State.Rotation = R.Quaternion();
	}

	ApplyKeepOut(DeltaTime);

	const FVector Delta = State.Location - StartLocation;
	State.Location = StartLocation;
	bInOwnMove = true;
	ResolveMove(Delta, FMath::Min(DeltaTime, 0.25f));
	bInOwnMove = false;
	if (UpdatedComponent)
	{
		WatchLastLocation = UpdatedComponent->GetComponentLocation();
	}

	State.Location = UpdatedComponent->GetComponentLocation();
	Velocity = State.Velocity;
	LastWrittenVelocity = Velocity;
	UpdateComponentVelocity();

	BroadcastStateChanges(Before);
}

void UTDShipMovementComponent::ApplyKeepOut(float DeltaTime)
{
	bKeepOutContact = false;
	if (KeepOutRadius <= 0.f)
	{
		KeepOut01 = 0.f;
		return;
	}
	const FVector Offset = State.Location - KeepOutCenter;
	const float Dist = Offset.Size();
	const FVector Out = Dist > KINDA_SMALL_NUMBER ? Offset / Dist : FVector::UpVector;
	KeepOut01 = FMath::Clamp((KeepOutRadius + KeepOutBand - Dist) / KeepOutBand, 0.f, 1.f);
	if (KeepOut01 <= 0.f)
	{
		return;
	}
	// Turn the nose away: toward the surface normal until the ship is clearly heading out, faster the deeper it is.
	const FVector Forward = State.Rotation.GetForwardVector();
	if (FVector::DotProduct(Forward, Out) < 0.45f)
	{
		FVector Axis = FVector::CrossProduct(Forward, Out);
		if (!Axis.Normalize()) { Axis = State.Rotation.GetUpVector(); }
		const float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, Out), -1.f, 1.f));
		const float Step = FMath::DegreesToRadians(FMath::Lerp(45.f, 220.f, KeepOut01)) * DeltaTime;
		State.Rotation = (FQuat(Axis, FMath::Min(Angle, Step)) * State.Rotation).GetNormalized();
	}
	// Slow toward a firm crawl as it goes deeper (the sim re-accelerates outside the band).
	const float Slow = Tuning.CruiseSpeed * 0.45f;
	if (State.ForwardSpeed > Slow)
	{
		State.ForwardSpeed = FMath::FInterpTo(State.ForwardSpeed, Slow, DeltaTime, 5.f * KeepOut01);
	}
	// Never inside: hold the ship out along the normal and drop the inward part of its travel.
	if (Dist < KeepOutRadius)
	{
		State.Location = KeepOutCenter + Out * KeepOutRadius;
		bKeepOutContact = true;
	}
	const float Inward = FVector::DotProduct(State.Velocity, Out);
	if (Inward < 0.f)
	{
		State.Velocity -= Out * Inward * (Dist < KeepOutRadius ? 1.f : KeepOut01);
	}
}

void UTDShipMovementComponent::HandleTransformUpdated(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport)
{
	if (!bWatchReady || !Component)
	{
		return;
	}
	const FVector Now = Component->GetComponentLocation();
	const float Moved = FVector::Dist(Now, WatchLastLocation);
	const FVector From = WatchLastLocation;
	WatchLastLocation = Now;
	if (bInOwnMove || bMoveAnnounced || Moved <= SilentMoveDistance)
	{
		return;
	}
	++SilentMoveCount;
	LargestSilentMove = FMath::Max(LargestSilentMove, Moved);
#if !UE_BUILD_SHIPPING
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsPlayerControlled())
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("TD silent move: %s moved %.0f m in one step, outside flight and without TeleportSpacecraft (%s -> %s). Mover's callstack:"),
			*GetNameSafe(GetOwner()), Moved / 100.f, *From.ToCompactString(), *Now.ToCompactString());
		if (!GIsAutomationTesting) { FDebug::DumpStackTraceToLog(ELogVerbosity::Warning); } // tests: the line is enough, a stack per move is noise
	}
#endif
}

FCollisionObjectQueryParams UTDShipMovementComponent::TerrainObjectTypes()
{
	FCollisionObjectQueryParams Types;
	Types.AddObjectTypesToQuery(ECC_WorldStatic);
	Types.AddObjectTypesToQuery(ECC_WorldDynamic);
	return Types;
}

bool UTDShipMovementComponent::IsTerrainHit(const FHitResult& Hit)
{
	const UPrimitiveComponent* C = Hit.GetComponent();
	if (!C || Cast<APawn>(Hit.GetActor()))
	{
		return false;
	}
	return C->GetCollisionObjectType() == ECC_WorldStatic || C->Mobility != EComponentMobility::Movable || C->Bounds.SphereRadius >= 1500.f;
}

bool UTDShipMovementComponent::SweepTerrain(const UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, float Radius, const FCollisionQueryParams& Params)
{
	if (!World)
	{
		return false;
	}
	TArray<FHitResult> Hits;
	if (Radius > 0.f)
	{
		World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, TerrainObjectTypes(), FCollisionShape::MakeSphere(Radius), Params);
	}
	else
	{
		World->LineTraceMultiByObjectType(Hits, Start, End, TerrainObjectTypes(), Params);
	}
	for (const FHitResult& H : Hits) // sorted by distance
	{
		if (H.bBlockingHit && !H.bStartPenetrating && IsTerrainHit(H))
		{
			OutHit = H;
			return true;
		}
	}
	return false;
}

void UTDShipMovementComponent::MeasureGroundClearance()
{
	Context.GroundClearance = -1.f;
	const UWorld* World = GetWorld();
	if (!Context.bAtmosphere || Tuning.GroundAvoidHeight <= 0.f || !World || !UpdatedComponent)
	{
		return;
	}
	const float Radius = UpdatedComponent->Bounds.SphereRadius;
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start - FVector(0.f, 0.f, Radius + Tuning.GroundAvoidHeight);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDGroundFloor), false, GetOwner());
	FHitResult Hit;
	// Terrain only (static or large movable geometry): ships and projectiles never count as ground.
	if (SweepTerrain(World, Hit, Start, End, 0.f, Params))
	{
		Context.GroundClearance = FMath::Max(0.f, Hit.Distance - Radius);
	}

	// Terrain ahead: sweep the hull along the current velocity (a straight-down trace can't see a rising slope or a
	// cliff in front of the ship).
	Context.TerrainAheadDistance = -1.f;
	Context.TerrainAheadRange = 0.f;
	const float Speed = State.Velocity.Size();
	if (Tuning.GroundLookAheadSeconds > 0.f && Speed > 100.f)
	{
		const float Range = Speed * Tuning.GroundLookAheadSeconds;
		const FVector Dir = State.Velocity / Speed;
		FHitResult Ahead;
		if (SweepTerrain(World, Ahead, Start, Start + Dir * Range, Radius, Params))
		{
			Context.TerrainAheadDistance = Ahead.Distance;
			Context.TerrainAheadRange = Range;
			Context.TerrainAheadNormal = Ahead.ImpactNormal;
		}
	}
}

void UTDShipMovementComponent::ResolveMove(const FVector& Delta, float DeltaTime)
{
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, State.Rotation, true, Hit);
	if (!Hit.IsValidBlockingHit())
	{
		return;
	}

	if (HandleShipImpact(Hit, DeltaTime))
	{
		// The piece broke (a rammed or tiny rock; its collision is gone): carry on through with the rest of the move.
		const float Left = FMath::Clamp(1.f - Hit.Time, 0.f, 1.f);
		if (Left > KINDA_SMALL_NUMBER)
		{
			FHitResult Through(1.f);
			SafeMoveUpdatedComponent(Delta * Left, State.Rotation, true, Through);
			if (Through.IsValidBlockingHit() && !HandleShipImpact(Through, DeltaTime))
			{
				SlideAlongSurface(Delta * Left, 1.f - Through.Time, Through.Normal, Through, true);
			}
		}
		return;
	}

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

void UTDShipMovementComponent::SlideOffSurface(const FVector& Normal, float DeltaTime)
{
	// Never stuck: turn the nose along the surface and keep moving along it (review / playtest, Nov 12: a ship
	// sat pinned at 0 m/s against a closed planet shell with no message).
	const FVector Nose = State.Rotation.GetForwardVector();
	if (FVector::DotProduct(Nose, Normal) < -0.05f && Tuning.PinnedTurnRate > 0.f && DeltaTime > 0.f && !State.bDrifting)
	{
		FVector Along = FVector::VectorPlaneProject(Nose, Normal);
		if (Along.SizeSquared() < 0.01f)
		{
			Along = FVector::VectorPlaneProject(State.Rotation.GetUpVector(), Normal); // dead head-on: pull up
		}
		Along = Along.GetSafeNormal();
		if (!Along.IsNearlyZero())
		{
			const FQuat Want = FQuat::FindBetweenNormals(Nose, Along) * State.Rotation;
			State.Rotation = FMath::QInterpConstantTo(State.Rotation, Want, DeltaTime, FMath::DegreesToRadians(Tuning.PinnedTurnRate)).GetNormalized();
			if (UpdatedComponent)
			{
				UpdatedComponent->SetWorldRotation(State.Rotation);
			}
		}
	}
	const FVector Tangent = FVector::VectorPlaneProject(State.Velocity, Normal);
	const float MinSlide = FMath::Max(Tuning.MinSpeed, 500.f);
	if (Tangent.Size() < MinSlide)
	{
		const FVector Dir = FVector::VectorPlaneProject(State.Rotation.GetForwardVector(), Normal).GetSafeNormal();
		if (!Dir.IsNearlyZero())
		{
			State.Velocity = Dir * MinSlide;
		}
	}
}

bool UTDShipMovementComponent::HandleShipImpact(const FHitResult& Hit, float DeltaTime)
{
	const FVector Normal = Hit.Normal.GetSafeNormal();
	const FVector V = State.Velocity;
	const float Speed = V.Size();
	const float Into = -FVector::DotProduct(V, Normal);
	if (Speed < KINDA_SMALL_NUMBER || Into <= 0.f || Normal.IsNearlyZero())
	{
		return false;
	}
	// The world (asteroids, wrecks, debris) is told first and may break (T6's field: Lancer's ram, tiny pieces).
	const bool bWorld = !Cast<APawn>(Hit.GetActor());
	ITDShipImpactReceiver* Receiver = bWorld ? Cast<ITDShipImpactReceiver>(Hit.GetActor()) : nullptr;
	UE_LOG(LogTriadDrift, Verbose, TEXT("TD flight: %s hit %s / %s item %d at %.1f m/s into it (receiver %d, scrape below %.0f m/s)"),
		*GetNameSafe(GetOwner()), *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()), Hit.Item, Into / 100.f, Receiver ? 1 : 0, Tuning.WorldScrapeSpeed / 100.f);
	if (bWorld && Into >= Tuning.WorldScrapeSpeed)
	{
		if (Receiver)
		{
			if (Receiver->ReceiveShipImpact(Hit.GetComponent(), Hit.Item, Hit.ImpactPoint, Into, FMath::Clamp(Into / Speed, 0.f, 1.f), Cast<ATDShip>(GetOwner())))
			{
				return true; // broken: no bounce, no damage
			}
		}
	}
	if (Into < Tuning.MinImpactSpeed)
	{
		// Resting / pinned contact: stop pushing into the surface and slide off it. No event, no damage.
		State.Velocity = V + Normal * Into;
		if (!Cast<APawn>(Hit.GetActor())) { SlideOffSurface(Normal, DeltaTime); } // bodies and walls, not other ships
		return false;
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
		if (!Cast<APawn>(Hit.GetActor())) { SlideOffSurface(Normal, DeltaTime); } // bodies and walls, not other ships
		return false;
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

	Impact.bWorld = bWorld;
	float MaxDamage = Tuning.MaxImpactDamage;
	if (bWorld)
	{
		// Ryan (Nov 13): the harder you hit the world, the more it hurts. Nothing below the scrape speed; above it, the
		// square of the speed into the surface, up to WorldImpactHullAtBoost of the max hull at boost speed.
		const UTDHealthComponent* Health = nullptr;
		if (const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner())) { Health = Craft->GetHealth(); }
		const float MaxHull = Health ? Health->MaxHull : 100.f;
		const float S0 = Tuning.WorldScrapeSpeed, S1 = FMath::Max(Tuning.BoostSpeed, S0 + 1.f);
		const float K = Into <= S0 ? 0.f : (FMath::Square(Into) - FMath::Square(S0)) / (FMath::Square(S1) - FMath::Square(S0));
		Damage = MaxHull * Tuning.WorldImpactHullAtBoost * K;
		MaxDamage = MaxHull * Tuning.WorldImpactMaxHull;
		Impact.Strength01 = Into <= S0 ? 0.3f * Into / FMath::Max(S0, 1.f) : FMath::Clamp(0.3f + 0.7f * K, 0.f, 1.f);
	}
	else
	{
		Impact.Strength01 = FMath::Clamp(Into / FMath::Max(Tuning.BoostSpeed, 1.f), 0.f, 1.f);
	}

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Damage > 0.f && Now - LastCollisionDamageTime >= Tuning.CollisionDamageCooldown)
	{
		LastCollisionDamageTime = Now;
		Impact.Damage = FMath::Min(Damage, MaxDamage);
	}

	OnFlightImpact.Broadcast(Impact);
	return false;
}

void UTDShipMovementComponent::BroadcastStateChanges(const FTDFlightState& Before)
{
	if (State.bBoosting != Before.bBoosting)
	{
		State.bBoosting ? OnBoostStarted.Broadcast() : OnBoostEnded.Broadcast();
	}
	// ENG-43 Bark_BoostEmpty: the meter ran dry with boost held (once per lock-out).
	if (State.bBoostLockout && !Before.bBoostLockout)
	{
		FTDCombatBarks::SayAsPilot(GetOwner(), TDBarkIdsT1::BoostEmpty, ETDBarkPriority::Flavour);
	}
	if (State.bDrifting && !Before.bDrifting)
	{
		OnDriftStarted.Broadcast();
	}
	if (State.bDriftEndedThisStep && Before.bDrifting)
	{
		OnDriftReleased.Broadcast(State.bDriftBurstThisStep);
	}
	const ETDBoundaryState WasBoundary = Before.bOutOfBounds ? ETDBoundaryState::Outside : (Before.bNearBoundary ? ETDBoundaryState::Warning : ETDBoundaryState::Inside);
	const bool bHeld = IsBoundaryWarningHeld();
	if (!bHeld && (GetBoundaryState() != WasBoundary || (bBoundaryHeldLastStep && GetBoundaryState() != ETDBoundaryState::Inside)))
	{
		OnBoundaryStateChanged.Broadcast(GetBoundaryState()); // a change, or the hold just ended near / past the edge
	}
	bBoundaryHeldLastStep = bHeld;
}

bool UTDShipMovementComponent::CanDodge() const
{
	return Tuning.DodgeSpeed > 0.f && !Context.bSpeedOverride && IsComponentTickEnabled() && State.DodgeTimeLeft <= 0.f
		&& State.TimeSinceDodge >= Tuning.DodgeCooldown;
}

bool UTDShipMovementComponent::RequestDodge(float Direction)
{
	if (FMath::IsNearlyZero(Direction) || !CanDodge())
	{
		return false;
	}
	State.DodgeSign = FMath::Sign(Direction);
	State.DodgeTimeLeft = Tuning.DodgeSeconds;
	State.TimeSinceDodge = 0.f;
	OnDodge.Broadcast(State.DodgeSign);
	return true;
}

void UTDShipMovementComponent::SetBoundaryWarningHoldFor(FName Owner, bool bHold)
{
	if (bHold) { BoundaryWarningHolds.Add(Owner); }
	else { BoundaryWarningHolds.Remove(Owner); }
}

void UTDShipMovementComponent::SetBoundaryReleaseFor(FName Owner, bool bRelease)
{
	if (bRelease) { BoundaryReleases.Add(Owner); }
	else { BoundaryReleases.Remove(Owner); }
}

bool UTDShipMovementComponent::IsBoundaryWarningHeld() const
{
	if (BoundaryWarningHolds.Num() > 0 || BoundaryReleases.Num() > 0) { return true; }
	static const FName SequenceOwners[] = { FName(TEXT("AtmosphereEntry")), FName(TEXT("TDExitClimb")), FName(TEXT("Hyperdrive")), FName(TEXT("LandingAssist")), FName(TEXT("DescentGlide")) };
	for (const FSpeedOverrideRequest& R : OverrideStack)
	{
		for (const FName& N : SequenceOwners) { if (R.Owner == N) { return true; } }
	}
	return false;
}

FText UTDShipMovementComponent::GetBoundaryWarningText() const
{
	if (IsBoundaryWarningHeld()) { return FText::GetEmpty(); }
	const bool bZone = Context.bBoxBoundary;
	switch (GetBoundaryState())
	{
	case ETDBoundaryState::Warning:
		return bZone ? NSLOCTEXT("TDFlight", "EdgeOfArea", "EDGE OF THE FLIGHT AREA") : NSLOCTEXT("TDFlight", "EdgeOfZone", "EDGE OF THE COMBAT ZONE");
	case ETDBoundaryState::Outside:
		return bZone ? NSLOCTEXT("TDFlight", "LeavingArea", "LEAVING THE FLIGHT AREA - TURN BACK") : NSLOCTEXT("TDFlight", "LeavingZone", "LEAVING THE COMBAT ZONE - TURN BACK");
	default:
		return FText::GetEmpty();
	}
}

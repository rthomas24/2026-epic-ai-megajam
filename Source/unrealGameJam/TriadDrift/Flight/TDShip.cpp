// Triad Drift - a squad ship with the chase camera (ENG-01). Owner: Teammate 1 (Flight & Input).

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"

namespace TDShipCamera
{
	/** Signed angle from A to B around Axis (deg), both assumed perpendicular to Axis. */
	float SignedAngle(const FVector& A, const FVector& B, const FVector& Axis)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(A, B), Axis), FVector::DotProduct(A, B)));
	}

	FVector Rotate(const FVector& V, const FVector& Axis, float Deg)
	{
		return FQuat(Axis, FMath::DegreesToRadians(Deg)).RotateVector(V);
	}

	/** Perpendicular part of V against unit Axis, normalised (zero if V is parallel). */
	FVector Perp(const FVector& V, const FVector& Axis)
	{
		return (V - Axis * FVector::DotProduct(V, Axis)).GetSafeNormal();
	}

	/** Continuous through inverted flight: past 90 deg of bank the follow eases back to 0 at 180. */
	float FoldBank(float Deg)
	{
		Deg = FRotator::NormalizeAxis(Deg);
		return FMath::Abs(Deg) > 90.f ? FMath::Sign(Deg) * (180.f - FMath::Abs(Deg)) : Deg;
	}
}

ATDShip::ATDShip()
{
	Team = ETDTeam::Player;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetUsingAbsoluteRotation(true); // we drive its rotation (partial roll follow, look-ahead)
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bDoCollisionTest = false;       // open space; rocks are handled by the flight model
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraLagMaxDistance = CameraTuning.LagMaxDistance;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	ChaseCamera->bUsePawnControlRotation = false;
	ChaseCamera->SetAutoActivate(false);

	Targeting = CreateDefaultSubobject<UTDTargetingComponent>(TEXT("Targeting"));
	Hyperdrive = CreateDefaultSubobject<UTDHyperdriveComponent>(TEXT("Hyperdrive"));
	Landing = CreateDefaultSubobject<UTDLandingComponent>(TEXT("Landing"));
	AtmosphereEntry = CreateDefaultSubobject<UTDAtmosphereEntryComponent>(TEXT("AtmosphereEntry"));

	CockpitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CockpitCamera"));
	CockpitCamera->SetupAttachment(HullPivot);
	CockpitCamera->SetRelativeLocation(CockpitOffset);
	CockpitCamera->bUsePawnControlRotation = false;
	CockpitCamera->SetAutoActivate(false);

	bFindCameraComponentWhenViewTarget = true;
}

void ATDShip::BeginPlay()
{
	Super::BeginPlay();
	OnTeleported.AddDynamic(this, &ATDShip::HandleTeleported);

	SpringArm->TargetArmLength = CameraTuning.ArmLength;
	SpringArm->SocketOffset = FVector(0.f, 0.f, CameraTuning.Height);
	SpringArm->CameraLagSpeed = CameraTuning.LocationLag;
	SpringArm->CameraRotationLagSpeed = CameraTuning.RotationLag;
	SpringArm->CameraLagMaxDistance = CameraTuning.LagMaxDistance;
	CurrentFOV = CameraTuning.MinFOV;
	ChaseCamera->SetFieldOfView(CurrentFOV);

	SetPlayerControlled(bPlayerControlled);
	UpdateChaseCamera(0.f, true);
}

void ATDShip::SetShipId(ETDShipId NewId)
{
	ShipId = NewId;
}

void ATDShip::SetPlayerControlled(bool bInPlayerControlled)
{
	const bool bWasControlled = bPlayerControlled;
	bPlayerControlled = bInPlayerControlled;
	ApplyCameraActivation();
	if (!bInPlayerControlled && bWasControlled && Hyperdrive)
	{
		Hyperdrive->CancelSpool(); // a switch mid-spool cancels it (a jump in progress carries on)
	}
	if (bInPlayerControlled && !bWasControlled)
	{
		SnapCamera(); // a switch is a hard cut (Director's Cut hook #1): no lag swoop from the old ship
	}
}

void ATDShip::SetCockpitView(bool bCockpit)
{
	bCockpitView = bCockpit;
	ApplyCameraActivation();
	if (!bCockpit)
	{
		SnapCamera();
	}
}

void ATDShip::ApplyCameraActivation()
{
	const bool bChase = bPlayerControlled && !bCockpitView;
	const bool bCockpit = bPlayerControlled && bCockpitView;
	if (ChaseCamera)
	{
		ChaseCamera->SetActive(bChase);
	}
	if (SpringArm)
	{
		SpringArm->SetComponentTickEnabled(bChase);
	}
	if (CockpitCamera)
	{
		CockpitCamera->SetRelativeLocation(CockpitOffset);
		CockpitCamera->SetFieldOfView(CockpitFOV);
		CockpitCamera->SetActive(bCockpit);
	}
	// The pilot doesn't see their own placeholder hull from inside it.
	if (HullMesh) { HullMesh->SetOwnerNoSee(bCockpit); }
	if (PlaceholderWings) { PlaceholderWings->SetOwnerNoSee(bCockpit); }
}

void ATDShip::SnapCamera()
{
	if (!SpringArm)
	{
		return;
	}
	UpdateChaseCamera(0.f, true);
	// Lag stays off until the arm has evaluated once without it (see Tick).
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;
	SnapFramesRemaining = 2;
}

void ATDShip::HandleTeleported(ATDSpacecraft* Spacecraft, FVector FromLocation, FVector ToLocation, FRotator NewFacing)
{
	if (bPlayerControlled)
	{
		SnapCamera();
	}
}

void ATDShip::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bPlayerControlled && !bCockpitView)
	{
		UpdateChaseCamera(DeltaSeconds, false);
	}
}

void ATDShip::UpdateChaseCamera(float DeltaSeconds, bool bSnap)
{
	if (!SpringArm || !ChaseCamera || !ShipMovement)
	{
		return;
	}

	if (!bSnap && SnapFramesRemaining > 0 && --SnapFramesRemaining > 0)
	{
		bSnap = true; // still inside the snap window: keep lag off, hard-set FOV and look-ahead
	}
	if (!bSnap)
	{
		// Restore lag after a snap.
		SpringArm->bEnableCameraLag = CameraTuning.LocationLag > 0.f;
		SpringArm->bEnableCameraRotationLag = CameraTuning.RotationLag > 0.f;
		SpringArm->CameraLagSpeed = CameraTuning.LocationLag;
		SpringArm->CameraRotationLagSpeed = CameraTuning.RotationLag;
	}

	// --- Arm rotation: ship forward, with only RollFollow of its bank -------------------------------
	// Bank is measured against a reference "up" that tracks world up (rate-limited, held while the
	// nose is near vertical), so barrel rolls and loops never make the target jump.
	using namespace TDShipCamera;
	const FQuat ShipQuat = GetActorQuat();
	const FVector Forward = ShipQuat.GetForwardVector();
	const FVector ShipUp = ShipQuat.GetUpVector();

	FVector Current = Perp(CameraRefUp, Forward);
	FVector Target = FMath::Abs(Forward.Z) < CameraTuning.VerticalHoldLimit ? Perp(FVector::UpVector, Forward) : Current;
	if (Target.IsNearlyZero()) { Target = ShipUp; }
	if (bSnap || Current.IsNearlyZero()) { Current = Target; }
	const float ToTarget = SignedAngle(Current, Target, Forward);
	const float MaxStep = CameraTuning.ReferenceUpRate * DeltaSeconds;
	CameraRefUp = Rotate(Current, Forward, bSnap ? ToTarget : FMath::Clamp(ToTarget, -MaxStep, MaxStep));

	const FVector VisualUp = (ShipQuat * FQuat(FRotator(0.f, 0.f, ShipMovement->GetVisualBank()))).GetUpVector();
	const float Bank = SignedAngle(CameraRefUp, ShipUp, Forward) + SignedAngle(ShipUp, VisualUp, Forward);
	CameraRoll = CameraTuning.RollFollow * FoldBank(Bank);
	const FVector CameraUp = Rotate(CameraRefUp, Forward, CameraRoll);
	ChaseTargetQuat = FRotationMatrix::MakeFromXZ(Forward, CameraUp).ToQuat();
	SpringArm->SetWorldRotation(ChaseTargetQuat);

	// --- Look-ahead: the camera leans into the turn --------------------------------------------------
	const FTDFlightTuning& Tuning = ShipMovement->GetFlightTuning();
	const FVector Rates = ShipMovement->GetAngularRate();
	const float TurnRef = FMath::Max(Tuning.TurnRate, 1.f);
	const FVector2D TargetLook(FMath::Clamp(Rates.Z / TurnRef, -1.f, 1.f), FMath::Clamp(Rates.Y / TurnRef, -1.f, 1.f));
	LookAhead = bSnap ? TargetLook : FMath::Vector2DInterpTo(LookAhead, TargetLook, DeltaSeconds, CameraTuning.LookAheadResponse);
	ChaseCamera->SetRelativeRotation(FRotator(LookAhead.Y * CameraTuning.LookAheadAngle, LookAhead.X * CameraTuning.LookAheadAngle, 0.f));

	// --- FOV widens with speed -------------------------------------------------------------------------
	const float TargetFOV = FMath::Lerp(CameraTuning.MinFOV, CameraTuning.MaxFOV, ShipMovement->GetSpeed01());
	CurrentFOV = bSnap ? TargetFOV : FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, CameraTuning.FOVResponse);
	ChaseCamera->SetFieldOfView(CurrentFOV);
}

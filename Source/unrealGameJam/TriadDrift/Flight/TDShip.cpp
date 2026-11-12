// Triad Drift - a squad ship with the chase camera (ENG-01). Owner: Teammate 1 (Flight & Input).

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
	SpringArm->CameraLagMaxDistance = 1800.f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	ChaseCamera->bUsePawnControlRotation = false;
	ChaseCamera->SetAutoActivate(false);

	Targeting = CreateDefaultSubobject<UTDTargetingComponent>(TEXT("Targeting"));

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
	if (ChaseCamera)
	{
		ChaseCamera->SetActive(bInPlayerControlled);
	}
	if (SpringArm)
	{
		SpringArm->SetComponentTickEnabled(bInPlayerControlled);
	}
	if (bInPlayerControlled && !bWasControlled)
	{
		SnapCamera(); // a switch is a hard cut (Director's Cut hook #1): no lag swoop from the old ship
	}
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
	if (bPlayerControlled)
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

	// --- Arm rotation: ship frame with only RollFollow of its roll (and visual bank) ---------------
	const FQuat ShipQuat = GetActorQuat();
	const FVector Forward = ShipQuat.GetForwardVector();
	// Near-vertical flight makes "bank relative to world up" meaningless: fade the correction out.
	const float VerticalFade = 1.f - FMath::SmoothStep(0.8f, 0.95f, (float)FMath::Abs(Forward.Z));
	const float Bank = ShipQuat.Rotator().Roll * VerticalFade;
	const float VisualBank = ShipMovement->GetVisualBank();
	const float DesiredRoll = CameraTuning.RollFollow * (Bank + VisualBank);
	const FQuat ArmQuat = ShipQuat * FQuat(FRotator(0.f, 0.f, DesiredRoll - Bank));
	SpringArm->SetWorldRotation(ArmQuat);

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

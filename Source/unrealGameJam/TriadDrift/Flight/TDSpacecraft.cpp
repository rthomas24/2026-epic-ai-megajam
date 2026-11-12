// Triad Drift - the flyable body shared by squad ships and enemy ships (ENG-01). Owner: Teammate 1.

#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace TDSpacecraftDefaults
{
	constexpr float CollisionRadius = 220.f;
}

ATDSpacecraft::ATDSpacecraft()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionSphere->InitSphereRadius(TDSpacecraftDefaults::CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(TDCollision::Ship);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(TDCollision::Interactable, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(TDCollision::Portal, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->SetSimulatePhysics(false);
	CollisionSphere->SetEnableGravity(false);
	CollisionSphere->CanCharacterStepUpOn = ECB_No;
	RootComponent = CollisionSphere;

	HullPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HullPivot"));
	HullPivot->SetupAttachment(CollisionSphere);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	HullMesh->SetupAttachment(HullPivot);
	HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HullMesh->SetGenerateOverlapEvents(false);
	HullMesh->CanCharacterStepUpOn = ECB_No;
	if (ConeMesh.Succeeded())
	{
		HullMesh->SetStaticMesh(ConeMesh.Object);
		// Cone points +Z; pitch it so the tip points along +X (forward). 100 cm base mesh.
		HullMesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		HullMesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 4.2f));
	}

	PlaceholderWings = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderWings"));
	PlaceholderWings->SetupAttachment(HullPivot);
	PlaceholderWings->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderWings->SetGenerateOverlapEvents(false);
	PlaceholderWings->CanCharacterStepUpOn = ECB_No;
	if (CubeMesh.Succeeded())
	{
		PlaceholderWings->SetStaticMesh(CubeMesh.Object);
		PlaceholderWings->SetRelativeLocation(FVector(-60.f, 0.f, 0.f));
		PlaceholderWings->SetRelativeScale3D(FVector(1.2f, 4.4f, 0.14f));
	}

	ShipMovement = CreateDefaultSubobject<UTDShipMovementComponent>(TEXT("ShipMovement"));
	ShipMovement->UpdatedComponent = CollisionSphere;

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Weapons = CreateDefaultSubobject<UTDWeaponComponent>(TEXT("Weapons"));

	// Ships are flown through SetFlightInput only; no control-rotation coupling.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ATDSpacecraft::BeginPlay()
{
	Super::BeginPlay();
	if (ShipMovement)
	{
		ShipMovement->OnFlightImpact.AddDynamic(this, &ATDSpacecraft::HandleFlightImpact);
		AddTickPrerequisiteComponent(ShipMovement);
	}
}

void ATDSpacecraft::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateHullPresentation(DeltaSeconds);
}

UPawnMovementComponent* ATDSpacecraft::GetMovementComponent() const
{
	return ShipMovement;
}

void ATDSpacecraft::Restart()
{
	// Deliberately not calling Super: APawn::Restart calls StopMovementImmediately, which would
	// zero velocity on every possess and break instant ship switching (ENG-07).
	ConsumeMovementInputVector();
	RecalculateBaseEyeHeight();
}

void ATDSpacecraft::SetFlightInput(const FTDFlightInput& In)
{
	if (ShipMovement)
	{
		ShipMovement->SetFlightInput(In);
	}
}

const FTDFlightInput& ATDSpacecraft::GetFlightInput() const
{
	static const FTDFlightInput Empty;
	return ShipMovement ? ShipMovement->GetFlightInput() : Empty;
}

void ATDSpacecraft::ApplyFlightTuning(const FTDFlightTuning& Tuning)
{
	if (ShipMovement)
	{
		ShipMovement->ApplyFlightTuning(Tuning);
	}
}

float ATDSpacecraft::GetSpeed01() const { return ShipMovement ? ShipMovement->GetSpeed01() : 0.f; }
bool ATDSpacecraft::IsBoosting() const { return ShipMovement && ShipMovement->IsBoosting(); }
bool ATDSpacecraft::IsDrifting() const { return ShipMovement && ShipMovement->IsDrifting(); }
float ATDSpacecraft::GetBoost01() const { return ShipMovement ? ShipMovement->GetBoost01() : 0.f; }
bool ATDSpacecraft::IsOutOfBounds() const { return ShipMovement && ShipMovement->IsOutOfBounds(); }

void ATDSpacecraft::TeleportSpacecraft(const FVector& Location, const FRotator& Facing, bool bKeepSpeed)
{
	const FVector From = GetActorLocation();
	SetActorLocationAndRotation(Location, Facing, false, nullptr, ETeleportType::TeleportPhysics);
	if (ShipMovement)
	{
		ShipMovement->NotifyTeleported(Facing.Quaternion(), bKeepSpeed);
	}
	OnTeleported.Broadcast(this, From, Location, Facing);
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastSpacecraftTeleported(this, From, Location);
	}
}

void ATDSpacecraft::SetSpeedOverride(float SpeedCmPerSec, float SteeringScale)
{
	if (ShipMovement)
	{
		ShipMovement->SetSpeedOverride(SpeedCmPerSec, SteeringScale);
	}
}

void ATDSpacecraft::ClearSpeedOverride()
{
	if (ShipMovement)
	{
		ShipMovement->ClearSpeedOverride();
	}
}

void ATDSpacecraft::SetTeam(ETDTeam NewTeam)
{
	Team = NewTeam;
}

void ATDSpacecraft::HandleFlightImpact(const FTDFlightImpact& Impact)
{
	if (Impact.Damage <= 0.f || !Health || Health->IsDead())
	{
		return;
	}
	AActor* Other = Impact.OtherActor.Get();
	if (Other && Other != this)
	{
		// Friendly-fire rules apply: bumping a squadmate scrapes but doesn't hurt.
		UTDStatics::ApplyTDDamage(this, Impact.Damage, CollisionDamageKind, Other, Other, Impact.ImpactPoint, -Impact.ImpactNormal);
	}
	else
	{
		FTDDamageInfo Info;
		Info.Amount = Impact.Damage;
		Info.Kind = CollisionDamageKind;
		Info.HitLocation = Impact.ImpactPoint;
		Info.HitDirection = -Impact.ImpactNormal;
		Health->ApplyDamage(Info);
	}
}

void ATDSpacecraft::UpdateHullPresentation(float DeltaSeconds)
{
	if (!HullPivot || !ShipMovement)
	{
		return;
	}
	const FTDFlightTuning& Tuning = ShipMovement->GetFlightTuning();
	const FVector Rates = ShipMovement->GetAngularRate();
	const float Pitch01 = Tuning.TurnRate > 0.f ? FMath::Clamp(Rates.Y / Tuning.TurnRate, -1.f, 1.f) : 0.f;
	VisualPitch = FMath::FInterpTo(VisualPitch, Pitch01 * VisualPitchLean, DeltaSeconds, 6.f);
	HullPivot->SetRelativeRotation(FRotator(VisualPitch, 0.f, ShipMovement->GetVisualBank()));
}

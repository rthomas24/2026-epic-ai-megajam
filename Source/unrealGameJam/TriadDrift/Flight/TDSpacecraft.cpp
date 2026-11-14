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
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTDTeleportStack(TEXT("td.Debug.TeleportStack"), 0,
	TEXT("1 = log the caller's stack on long player teleports and silent moves (dev; each stack costs 60-270 ms). Default 0: the line only."));
bool TDTeleportStackEnabled() { return CVarTDTeleportStack.GetValueOnAnyThread() != 0; }

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
		PlaceholderHullMesh = ConeMesh.Object;
		HullMesh->SetStaticMesh(ConeMesh.Object);
		// Cone points +Z; pitch it so the tip points along +X (forward). 100 cm base mesh.
		HullMesh->SetRelativeRotation(PlaceholderHullRotation);
		HullMesh->SetRelativeScale3D(PlaceholderHullScale);
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

	EngineGlow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EngineGlow"));
	EngineGlow->SetupAttachment(HullPivot);
	EngineGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EngineGlow->SetGenerateOverlapEvents(false);
	EngineGlow->CanCharacterStepUpOn = ECB_No;
	EngineGlow->SetCastShadow(false);
	if (CubeMesh.Succeeded())
	{
		EngineGlow->SetStaticMesh(CubeMesh.Object);
		EngineGlow->SetRelativeLocation(FVector(-215.f, 0.f, 0.f));
		EngineGlow->SetRelativeScale3D(FVector(0.35f, 0.9f, 0.35f));
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
	ApplyPlaceholderLook();
}

FLinearColor ATDSpacecraft::GetPlaceholderColor() const
{
	return UTDStatics::TeamColor(Team);
}

void ATDSpacecraft::ApplyPlaceholderLook()
{
	TintPlaceholderParts();
}

void ATDSpacecraft::TintPlaceholderParts()
{
	if (!bPlaceholderParts)
	{
		return;
	}
	const FLinearColor Color = GetPlaceholderColor();
	UTDStatics::TintPlaceholder(HullMesh, Color);
	UTDStatics::TintPlaceholder(PlaceholderWings, Color * 0.45f);
	UTDStatics::TintPlaceholder(EngineGlow, Color, EngineGlowBoost);
}

void ATDSpacecraft::RestorePlaceholderHull()
{
	if (!HullMesh)
	{
		return;
	}
	HullMesh->SetStaticMesh(PlaceholderHullMesh);
	HullMesh->SetRelativeRotation(PlaceholderHullRotation);
	HullMesh->SetRelativeScale3D(PlaceholderHullScale);
	HullMesh->EmptyOverrideMaterials();
	HullMesh->SetVisibility(true);
}

void ATDSpacecraft::SetPlaceholderPartsVisible(bool bVisible)
{
	bPlaceholderParts = bVisible;
	if (PlaceholderWings) { PlaceholderWings->SetVisibility(bVisible); }
	if (EngineGlow) { EngineGlow->SetVisibility(bVisible); }
	if (bVisible)
	{
		TintPlaceholderParts();
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

float ATDSpacecraft::GetAltitudeMeters() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return -1.f;
	}
	// Altitude only means something over a planet's ground (atmosphere flight). In space a straight-down trace
	// beside a planet sphere hits its curved side at wildly changing distances (T4 read 320 m -> 1964 m in 0.5 s
	// while the ship slid along a closed planet at 60 m/s), so space reports none.
	if (ShipMovement && !ShipMovement->IsInAtmosphere())
	{
		return -1.f;
	}
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, 2000000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDAltitude), false, this);
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		const float Radius = CollisionSphere ? CollisionSphere->GetScaledSphereRadius() : 0.f;
		return FMath::Max(0.f, (Hit.Distance - Radius) / 100.f);
	}
	return -1.f;
}

void ATDSpacecraft::TeleportSpacecraft(const FVector& Location, const FRotator& Facing, bool bKeepSpeed)
{
	const FVector From = GetActorLocation();
#if !UE_BUILD_SHIPPING
	// Dev trail for long teleports of the player's ship (portals, return points, squad placement): who moved it.
	if (IsPlayerControlled() && FVector::Dist(From, Location) > 100000.f)
	{
		UE_LOG(LogTriadDrift, Log, TEXT("TD teleport: %s %.0f m (%s -> %s). Caller:"), *GetName(), FVector::Dist(From, Location) / 100.f,
			*From.ToCompactString(), *Location.ToCompactString());
		if (!GIsAutomationTesting && TDTeleportStackEnabled()) { FDebug::DumpStackTraceToLog(ELogVerbosity::Log); } // td.Debug.TeleportStack 1: a stack costs 60-270 ms (T6, #153) // tests: the line is enough (a stack reads like a crash in test logs)
	}
#endif
	if (ShipMovement) { ShipMovement->SetMoveAnnounced(true); }
	SetActorLocationAndRotation(Location, Facing, false, nullptr, ETeleportType::TeleportPhysics);
	if (ShipMovement)
	{
		ShipMovement->SetMoveAnnounced(false);
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

void ATDSpacecraft::SetSpeedOverrideFor(FName OverrideOwner, float SpeedCmPerSec, float SteeringScale, float AccelerationCmPerSec2)
{
	if (ShipMovement)
	{
		ShipMovement->SetSpeedOverrideFor(OverrideOwner, SpeedCmPerSec, SteeringScale, AccelerationCmPerSec2);
	}
}

void ATDSpacecraft::ClearSpeedOverrideFor(FName OverrideOwner)
{
	if (ShipMovement)
	{
		ShipMovement->ClearSpeedOverrideFor(OverrideOwner);
	}
}

void ATDSpacecraft::ClearSpeedOverride()
{
	if (ShipMovement)
	{
		ShipMovement->ClearSpeedOverride();
	}
}

void ATDSpacecraft::ResetFlightState(bool bKeepVelocity)
{
	if (ShipMovement)
	{
		ShipMovement->ResetFlightState(bKeepVelocity);
	}
}

void ATDSpacecraft::SetFlightParked(bool bParked)
{
	if (!ShipMovement)
	{
		return;
	}
	if (bParked)
	{
		ShipMovement->EndBoostAndDrift(); // balanced end events: no boost / drift loop survives a park
		ShipMovement->SetFlightInput(FTDFlightInput());
		ShipMovement->StopMovementImmediately();
		ShipMovement->SetComponentTickEnabled(false);
	}
	else
	{
		ShipMovement->SetComponentTickEnabled(true);
	}
}

bool ATDSpacecraft::IsFlightParked() const
{
	return ShipMovement && !ShipMovement->IsComponentTickEnabled();
}

void ATDSpacecraft::SetTeam(ETDTeam NewTeam)
{
	Team = NewTeam;
	if (HasActorBegunPlay())
	{
		ApplyPlaceholderLook();
	}
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

	// Engine glow stretches with speed above cruise, so boost reads at a glance.
	if (EngineGlow && bPlaceholderParts)
	{
		const float Range = FMath::Max(Tuning.BoostSpeed - Tuning.CruiseSpeed, 1.f);
		const float Target01 = FMath::Clamp(((float)GetVelocity().Size() - Tuning.CruiseSpeed) / Range, 0.f, 1.f);
		GlowLength01 = FMath::FInterpTo(GlowLength01, IsBoosting() ? FMath::Max(Target01, 0.6f) : Target01, DeltaSeconds, 8.f);
		const float LengthX = FMath::Lerp(EngineGlowLength.X, EngineGlowLength.Y, GlowLength01);
		EngineGlow->SetRelativeScale3D(FVector(LengthX, 0.9f, 0.35f));
		EngineGlow->SetRelativeLocation(FVector(-200.f - 50.f * LengthX, 0.f, 0.f));
	}
}

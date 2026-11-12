// Triad Drift - planet objective props and hazards (ENG-25 / ENG-26). Owner: Teammate 5.

#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundScene.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "TDPlanetProps"

namespace TDPlanetPropsImpl
{
	static UStaticMesh* Shape(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path);
	}
	static const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	static const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	static const TCHAR* SpherePath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	static const TCHAR* ConePath = TEXT("/Engine/BasicShapes/Cone.Cone");

	static UStaticMeshComponent* MakeMesh(AActor* Owner, const TCHAR* Name, const TCHAR* Path, USceneComponent* Parent)
	{
		UStaticMeshComponent* M = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		M->SetupAttachment(Parent);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(Path);
		if (Finder.Succeeded())
		{
			M->SetStaticMesh(Finder.Object);
		}
		M->SetMobility(EComponentMobility::Movable);
		return M;
	}
}

// ================================================================================================
// Base
// ================================================================================================

ATDPlanetProp::ATDPlanetProp()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void ATDPlanetProp::BeginPlay()
{
	Super::BeginPlay();
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		ResetHandle = Scene->OnResetUnfinished.AddUObject(this, &ATDPlanetProp::HandleGroundReset);
		CompletedHandle = Scene->OnCompletedOnLoad.AddUObject(this, &ATDPlanetProp::HandleCompletedOnLoad);
	}
}

void ATDPlanetProp::HandleCompletedOnLoad()
{
	if (LinkedObjective && LinkedObjective->IsComplete())
	{
		ApplyCompleted();
	}
}

void ATDPlanetProp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->OnResetUnfinished.Remove(ResetHandle);
		Scene->OnCompletedOnLoad.Remove(CompletedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ATDPlanetProp::HandleGroundReset()
{
	if (!LinkedObjective || !LinkedObjective->IsComplete())
	{
		ResetProp();
	}
}

bool ATDPlanetProp::IsLinkedStepCurrent() const
{
	return LinkedObjective && LinkedStep != INDEX_NONE && LinkedObjective->GetCurrentStepIndex() == LinkedStep;
}

ATDFirstPersonCharacter* ATDPlanetProp::AsPlayer(AActor* Actor)
{
	ATDFirstPersonCharacter* C = Cast<ATDFirstPersonCharacter>(Actor);
	return (C && C->IsPlayerControlled()) ? C : nullptr;
}

// ================================================================================================
// Step trigger
// ================================================================================================

ATDStepTrigger::ATDStepTrigger()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(RootComponent);
	Box->SetBoxExtent(FVector(500.f, 500.f, 300.f));
	Box->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
	Box->SetCollisionProfileName(TEXT("Trigger"));
	Beacon = TDPlanetPropsImpl::MakeMesh(this, TEXT("Beacon"), TDPlanetPropsImpl::CylinderPath, RootComponent);
	Beacon->SetRelativeScale3D(FVector(0.6f, 0.6f, 60.f));
	Beacon->SetRelativeLocation(FVector(0.f, 0.f, 3000.f));
	Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacon->SetCastShadow(false);
}

void ATDStepTrigger::BeginPlay()
{
	Super::BeginPlay();
	Box->OnComponentBeginOverlap.AddUniqueDynamic(this, &ATDStepTrigger::HandleOverlap);
	UTDStatics::TintPlaceholder(Beacon, bRequireCarrying ? FLinearColor(0.3f, 1.f, 0.5f) : FLinearColor(0.4f, 0.9f, 1.f), 3.f);
	Beacon->SetVisibility(false);
}

void ATDStepTrigger::HandleOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	TryComplete(Other);
}

bool ATDStepTrigger::TryComplete(AActor* Other)
{
	ATDFirstPersonCharacter* Player = AsPlayer(Other);
	if (!Player || Player->IsDead() || !IsLinkedStepCurrent() || (bRequireCarrying && !Player->IsCarrying()))
	{
		return false;
	}
	if (bRequireCarrying)
	{
		for (TActorIterator<ATDExtractionSite> It(GetWorld()); It; ++It)
		{
			if (It->IsCarried())
			{
				It->Deliver();
			}
		}
		Player->SetCarrying(false);
	}
	return LinkedObjective->CompleteStep(LinkedStep);
}

void ATDStepTrigger::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bCurrent = IsLinkedStepCurrent();
	Beacon->SetVisibility(bCurrent);
	if (!bCurrent)
	{
		return;
	}
	// Already standing inside when the step starts (or respawned there).
	TArray<AActor*> Inside;
	Box->GetOverlappingActors(Inside, ATDFirstPersonCharacter::StaticClass());
	for (AActor* A : Inside)
	{
		if (TryComplete(A))
		{
			break;
		}
	}
}

// ================================================================================================
// Counter
// ================================================================================================

void ATDStepCounter::BeginPlay()
{
	Super::BeginPlay();
	if (LinkedObjective)
	{
		LinkedObjective->OnStepStarted.AddUniqueDynamic(this, &ATDStepCounter::HandleStepStarted);
	}
}

void ATDStepCounter::WatchInteractable(ATDInteractableActor* Interactable)
{
	if (Interactable)
	{
		Interactable->bCompleteLinkedStep = false;
		Interactable->OnInteracted.AddUniqueDynamic(this, &ATDStepCounter::HandleInteracted);
	}
}

void ATDStepCounter::WatchDestructible(AActor* Actor)
{
	if (UTDHealthComponent* H = Actor ? Actor->FindComponentByClass<UTDHealthComponent>() : nullptr)
	{
		H->OnDeath.AddUniqueDynamic(this, &ATDStepCounter::HandleDeath);
	}
}

void ATDStepCounter::HandleInteracted(AActor*, AActor*) { Add(); }
void ATDStepCounter::HandleDeath(UTDHealthComponent*, const FTDDamageInfo&) { Add(); }

void ATDStepCounter::HandleStepStarted(ATDObjective* Objective, int32 StepIndex)
{
	if (StepIndex == LinkedStep)
	{
		Announce();
	}
}

void ATDStepCounter::Add()
{
	if (Count >= Required)
	{
		return;
	}
	++Count;
	if (Count >= Required)
	{
		if (IsLinkedStepCurrent())
		{
			LinkedObjective->CompleteStep(LinkedStep);
		}
		return;
	}
	Announce();
}

void ATDStepCounter::Announce() const
{
	if (IsLinkedStepCurrent())
	{
		LinkedObjective->UpdateStepText(FText::Format(LOCTEXT("Count", "{0}  {1} / {2}"), BaseText, Count, Required));
	}
}

void ATDStepCounter::ResetProp()
{
	Count = 0;
}

// ================================================================================================
// Extraction site (wreck core)
// ================================================================================================

ATDExtractionSite::ATDExtractionSite()
{
	PrimaryActorTick.bCanEverTick = true;
	HoldTime = 4.f;
	bHoldSurvivesDamage = true;
	Prompt = LOCTEXT("Extract", "Extract the core");
	Core = TDPlanetPropsImpl::MakeMesh(this, TEXT("Core"), TDPlanetPropsImpl::SpherePath, RootComponent);
	Core->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	Core->SetRelativeScale3D(FVector(0.55f));
	Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Core->SetCastShadow(false);
}

void ATDExtractionSite::BeginPlay()
{
	Super::BeginPlay();
	UTDStatics::TintPlaceholder(Core, FLinearColor(0.5f, 1.f, 0.6f), 4.f);
	UTDStatics::TintPlaceholder(Mesh, FLinearColor(0.1f, 0.8f, 1.f), 1.2f); // the console glows: it reads in the dark
	// The glowing core is what the eye goes to: it takes the interact trace too (playtest).
	Core->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Core->SetCollisionResponseToAllChannels(ECR_Ignore);
	Core->SetCollisionResponseToChannel(TDCollision::Interactable, ECR_Block);
	CoreHome = Core->GetRelativeTransform();
	if (LinkedObjective)
	{
		LinkedObjective->OnStepStarted.AddUniqueDynamic(this, &ATDExtractionSite::HandleStepStarted);
	}
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		ResetHandle = Scene->OnResetUnfinished.AddWeakLambda(this, [this]()
		{
			if (!LinkedObjective || !LinkedObjective->IsComplete())
			{
				ResetSite();
			}
		});
	}
}

void ATDExtractionSite::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->OnResetUnfinished.Remove(ResetHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ATDExtractionSite::HandleStepStarted(ATDObjective* Objective, int32 StepIndex)
{
	if (StepIndex == LinkedStep)
	{
		StepElapsed = 0.f;
		bWaveBDropped = false;
		if (WaveA)
		{
			WaveA->Activate();
		}
	}
}

void ATDExtractionSite::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (StepElapsed >= 0.f && LinkedObjective && LinkedObjective->GetCurrentStepIndex() == LinkedStep)
	{
		StepElapsed += DeltaSeconds;
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const ATDFirstPersonCharacter* P = PC ? Cast<ATDFirstPersonCharacter>(PC->GetPawn()) : nullptr;
		const bool bHolding = P && P->GetHoldTarget() == this;
		if (!bWaveBDropped && WaveB && (bHolding || StepElapsed >= WaveBDelay))
		{
			bWaveBDropped = true;
			WaveB->Activate();
		}
		// The core pulses faster while it's being pulled out.
		const float Rate = bHolding ? 12.f : 3.f;
		Core->SetRelativeScale3D(FVector(0.55f + 0.06f * FMath::Sin(GetWorld()->GetTimeSeconds() * Rate)));
	}
	// Dropped if the carrier dies.
	if (ATDFirstPersonCharacter* C = Carrier.Get())
	{
		if (C->IsDead())
		{
			DropCore();
		}
	}
}

void ATDExtractionSite::Interact(AActor* Interactor)
{
	const bool bWasCurrent = CanInteract(Interactor);
	Super::Interact(Interactor); // completes the extraction step
	ATDFirstPersonCharacter* P = Cast<ATDFirstPersonCharacter>(Interactor);
	if (!bWasCurrent || !P)
	{
		return;
	}
	Carrier = P;
	P->SetCarrying(true);
	if (UCameraComponent* Cam = P->FindComponentByClass<UCameraComponent>())
	{
		Core->SetCollisionEnabled(ECollisionEnabled::NoCollision); // in hand: never in the way of a trace
		Core->AttachToComponent(Cam, FAttachmentTransformRules::KeepRelativeTransform);
		Core->SetRelativeLocationAndRotation(FVector(70.f, -18.f, -32.f), FRotator::ZeroRotator);
		Core->SetRelativeScale3D(FVector(0.28f));
	}
}

void ATDExtractionSite::Deliver()
{
	if (ATDFirstPersonCharacter* C = Carrier.Get())
	{
		C->SetCarrying(false);
	}
	Carrier = nullptr;
	Core->SetVisibility(false);
}

void ATDExtractionSite::DropCore()
{
	if (ATDFirstPersonCharacter* C = Carrier.Get())
	{
		C->SetCarrying(false);
	}
	Carrier = nullptr;
	Core->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	Core->SetRelativeTransform(CoreHome);
	Core->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ATDExtractionSite::ResetSite()
{
	DropCore();
	Core->SetVisibility(true);
	ResetUse();
	StepElapsed = -1.f;
	bWaveBDropped = false;
}

// ================================================================================================
// Magma
// ================================================================================================

ATDMagmaVolume::ATDMagmaVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(RootComponent);
	Box->SetCollisionProfileName(TEXT("Trigger"));
	Surface = TDPlanetPropsImpl::MakeMesh(this, TEXT("Surface"), TDPlanetPropsImpl::CubePath, RootComponent);
	Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Surface->SetCastShadow(false);
	SetSize(FVector(1000.f, 1000.f, 200.f));
}

void ATDMagmaVolume::SetSize(const FVector& Size)
{
	Box->SetBoxExtent(FVector(Size.X * 0.5f, Size.Y * 0.5f, Size.Z * 0.5f + 60.f));
	Box->SetRelativeLocation(FVector(0.f, 0.f, -Size.Z * 0.5f + 60.f));
	Surface->SetRelativeScale3D(FVector(Size.X, Size.Y, 10.f) / 100.f);
	Surface->SetRelativeLocation(FVector(0.f, 0.f, -5.f));
}

void ATDMagmaVolume::BeginPlay()
{
	Super::BeginPlay();
	UTDStatics::TintPlaceholder(Surface, FLinearColor(1.f, 0.28f, 0.03f), 3.f);
	Box->OnComponentBeginOverlap.AddUniqueDynamic(this, &ATDMagmaVolume::HandleOverlap);
}

void ATDMagmaVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Anyone still inside (stood up after a respawn on the edge, crew wandering in) keeps burning.
	TArray<AActor*> Inside;
	Box->GetOverlappingActors(Inside, APawn::StaticClass());
	Pulse += DeltaSeconds;
	if (Pulse >= 0.5f)
	{
		Pulse = 0.f;
		for (AActor* A : Inside)
		{
			HandleOverlap(nullptr, A, nullptr, 0, false, FHitResult());
		}
	}
}

void ATDMagmaVolume::HandleOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!Other || Other == this)
	{
		return;
	}
	UTDHealthComponent* H = UTDStatics::GetHealth(Other);
	if (!H || !H->IsAlive())
	{
		return;
	}
	UTDStatics::ApplyTDDamage(Other, Damage, ETDDamageKind::Environmental, this, this);
	if (ATDFirstPersonCharacter* P = AsPlayer(Other))
	{
		if (!P->IsDead())
		{
			P->ReturnToSafeGround();
		}
	}
}

// ================================================================================================
// Geyser
// ================================================================================================

ATDGeyser::ATDGeyser()
{
	PrimaryActorTick.bCanEverTick = true;
	Vent = TDPlanetPropsImpl::MakeMesh(this, TEXT("Vent"), TDPlanetPropsImpl::CylinderPath, RootComponent);
	Vent->SetRelativeScale3D(FVector(2.4f, 2.4f, 0.3f));
	Vent->SetRelativeLocation(FVector(0.f, 0.f, 15.f));
	Plume = TDPlanetPropsImpl::MakeMesh(this, TEXT("Plume"), TDPlanetPropsImpl::CylinderPath, RootComponent);
	Plume->SetRelativeScale3D(FVector(1.6f, 1.6f, 8.f));
	Plume->SetRelativeLocation(FVector(0.f, 0.f, 400.f));
	Plume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plume->SetCastShadow(false);
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(RootComponent);
	Box->SetBoxExtent(FVector(110.f, 110.f, 400.f));
	Box->SetRelativeLocation(FVector(0.f, 0.f, 400.f));
	Box->SetCollisionProfileName(TEXT("Trigger"));
}

void ATDGeyser::BeginPlay()
{
	Super::BeginPlay();
	VentMID = UTDStatics::TintPlaceholder(Vent, FLinearColor(1.f, 0.35f, 0.05f), 0.15f);
	UTDStatics::TintPlaceholder(Plume, FLinearColor(1.f, 0.55f, 0.2f), 2.5f);
	Clock = Phase;
	Plume->SetVisibility(false);
}

bool ATDGeyser::IsErupting() const
{
	const float Cycle = DormantSeconds + WarnSeconds + EruptSeconds;
	const float T = FMath::Fmod(Clock, Cycle);
	return T >= DormantSeconds + WarnSeconds;
}

void ATDGeyser::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	const float Cycle = DormantSeconds + WarnSeconds + EruptSeconds;
	const float T = FMath::Fmod(Clock, Cycle);
	const bool bWarn = T >= DormantSeconds && T < DormantSeconds + WarnSeconds;
	const bool bErupt = IsErupting();
	// Warning: the vent glows and a thin plume flickers; eruption: the full column.
	const float Glow = bWarn ? 1.f + 2.f * FMath::Abs(FMath::Sin(T * 20.f)) : (bErupt ? 3.f : 0.15f);
	if (VentMID)
	{
		const FLinearColor C = FLinearColor(1.f, 0.35f, 0.05f) * Glow;
		VentMID->SetVectorParameterValue(TEXT("Color"), C);
		VentMID->SetVectorParameterValue(TEXT("EmissiveColor"), C);
	}
	if (!bWarn && !bErupt)
	{
		HitThisEruption.Reset();
	}
	Plume->SetVisibility(bWarn || bErupt);
	Plume->SetRelativeScale3D(bErupt ? FVector(1.6f, 1.6f, 8.f) : FVector(0.4f, 0.4f, 2.f));
	Plume->SetRelativeLocation(FVector(0.f, 0.f, bErupt ? 400.f : 100.f));
	if (!bErupt)
	{
		return;
	}
	TArray<AActor*> Inside;
	Box->GetOverlappingActors(Inside, APawn::StaticClass());
	for (AActor* A : Inside)
	{
		if (HitThisEruption.Contains(A))
		{
			continue;
		}
		HitThisEruption.Add(A);
		UTDStatics::ApplyTDDamage(A, Damage, ETDDamageKind::Environmental, this, this);
		if (ACharacter* C = Cast<ACharacter>(A))
		{
			C->LaunchCharacter(FVector(0.f, 0.f, LaunchSpeed), false, true);
		}
	}
}

// ================================================================================================
// Shatter crystal
// ================================================================================================

ATDShatterCrystal::ATDShatterCrystal()
{
	Mesh = TDPlanetPropsImpl::MakeMesh(this, TEXT("Mesh"), TDPlanetPropsImpl::ConePath, RootComponent);
	Mesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 2.6f));
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->MaxShield = 0.f;
	Health->DefaultTeam = ETDTeam::Neutral;
}

void ATDShatterCrystal::BeginPlay()
{
	Super::BeginPlay();
	Health->InitMaxValues(MaxHealth, 0.f, true);
	Health->OnDeath.AddUniqueDynamic(this, &ATDShatterCrystal::HandleDeath);
	UTDStatics::TintPlaceholder(Mesh, Color, 1.5f);
}

void ATDShatterCrystal::HandleDeath(UTDHealthComponent*, const FTDDamageInfo&)
{
	bShattered = true;
	Mesh->SetVisibility(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATDShatterCrystal::ApplyCompleted()
{
	bShattered = true;
	Mesh->SetVisibility(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATDShatterCrystal::ResetProp()
{
	if (!bShattered)
	{
		return;
	}
	bShattered = false;
	Health->Revive();
	Health->InitMaxValues(MaxHealth, 0.f, true);
	Mesh->SetVisibility(true);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

// ================================================================================================
// Collapsing platform
// ================================================================================================

ATDCollapsingPlatform::ATDCollapsingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	Slab = TDPlanetPropsImpl::MakeMesh(this, TEXT("Slab"), TDPlanetPropsImpl::CubePath, RootComponent);
	Slab->SetCollisionProfileName(TEXT("BlockAll"));
	SetSize(FVector(400.f, 300.f, 60.f));
}

void ATDCollapsingPlatform::SetSize(const FVector& Size)
{
	Slab->SetRelativeScale3D(Size / 100.f);
	Slab->SetRelativeLocation(FVector(0.f, 0.f, -Size.Z * 0.5f));
}

void ATDCollapsingPlatform::BeginPlay()
{
	Super::BeginPlay();
	HomeRelative = Slab->GetRelativeLocation();
	Tags.AddUnique(TEXT("TD.Unsafe")); // never a safe-ground respawn spot
	UTDStatics::TintPlaceholder(Slab, FLinearColor(0.6f, 0.3f, 1.f), 1.2f);
}

void ATDCollapsingPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bFallen)
	{
		if (Slab->IsVisible())
		{
			FallSpeed += 2000.f * DeltaSeconds;
			Slab->AddRelativeLocation(FVector(0.f, 0.f, -FallSpeed * DeltaSeconds));
			if (Slab->GetRelativeLocation().Z < HomeRelative.Z - 6000.f)
			{
				Slab->SetVisibility(false);
			}
		}
		return;
	}
	if (StandTime < 0.f)
	{
		// Someone on top? (the player's floor is this slab)
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const ACharacter* C = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		if (C && C->GetCharacterMovement() && C->GetCharacterMovement()->CurrentFloor.HitResult.GetComponent() == Slab)
		{
			StandTime = 0.f;
		}
		return;
	}
	StandTime += DeltaSeconds;
	// Cracking: it shudders and flashes before it goes.
	const float Shake = 4.f * FMath::Sin(StandTime * 60.f);
	Slab->SetRelativeLocation(HomeRelative + FVector(Shake, -Shake, 0.f));
	if (StandTime >= FallDelay)
	{
		bFallen = true;
		FallSpeed = 0.f;
		Slab->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UTDStatics::TintPlaceholder(Slab, FLinearColor(1.f, 0.3f, 0.6f), 2.f);
	}
}

void ATDCollapsingPlatform::ResetProp()
{
	bFallen = false;
	StandTime = -1.f;
	FallSpeed = 0.f;
	Slab->SetRelativeLocation(HomeRelative);
	Slab->SetVisibility(true);
	Slab->SetCollisionProfileName(TEXT("BlockAll"));
	UTDStatics::TintPlaceholder(Slab, FLinearColor(0.6f, 0.3f, 1.f), 1.2f);
}

// ================================================================================================
// Collapse run
// ================================================================================================

ATDCollapseRun::ATDCollapseRun()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATDCollapseRun::BeginPlay()
{
	Super::BeginPlay();
	if (LinkedObjective)
	{
		LinkedObjective->OnStepStarted.AddUniqueDynamic(this, &ATDCollapseRun::HandleStepStarted);
	}
	if (Tower)
	{
		TowerHome = Tower->GetActorLocation();
	}
}

void ATDCollapseRun::HandleStepStarted(ATDObjective* Objective, int32 StepIndex)
{
	if (StepIndex == LinkedStep)
	{
		TimeLeft = Seconds;
		DebrisTimer = 0.f;
		LastShownSecond = -1;
	}
}

void ATDCollapseRun::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (TimeLeft < 0.f)
	{
		return;
	}
	if (!IsLinkedStepCurrent())
	{
		TimeLeft = -1.f; // made it (or the objective reset)
		return;
	}
	TimeLeft -= DeltaSeconds;
	// The tower sinks and leans as it comes down.
	if (Tower)
	{
		const float A = 1.f - FMath::Clamp(TimeLeft / Seconds, 0.f, 1.f);
		Tower->SetActorLocation(TowerHome + FVector(0.f, 0.f, -A * 2500.f));
		Tower->SetActorRotation(FRotator(A * 12.f, 0.f, A * 6.f));
	}
	const int32 Shown = FMath::CeilToInt(FMath::Max(TimeLeft, 0.f));
	if (Shown != LastShownSecond)
	{
		LastShownSecond = Shown;
		LinkedObjective->UpdateStepText(FText::Format(LOCTEXT("Run", "GET CLEAR OF THE TOWER · {0}"), Shown));
	}
	DebrisTimer -= DeltaSeconds;
	if (DebrisTimer <= 0.f)
	{
		DebrisTimer = 0.45f;
		DropDebris();
	}
	if (TimeLeft <= 0.f)
	{
		TimeLeft = -1.f;
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (APawn* P = PC ? PC->GetPawn() : nullptr)
		{
			if (UTDHealthComponent* H = UTDStatics::GetHealth(P))
			{
				H->Kill(this, ETDDamageKind::Environmental); // buried: CUT!
			}
		}
	}
}

void ATDCollapseRun::DropDebris()
{
	const FVector Local(FMath::FRandRange(-DebrisExtent.X, DebrisExtent.X), FMath::FRandRange(-DebrisExtent.Y, DebrisExtent.Y), 2500.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Rock = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(GetActorTransform().TransformPosition(Local)), Params);
	if (!Rock)
	{
		return;
	}
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(Rock, TEXT("Rock"));
	M->SetStaticMesh(TDPlanetPropsImpl::Shape(TDPlanetPropsImpl::CubePath));
	M->SetMobility(EComponentMobility::Movable);
	M->SetWorldScale3D(FVector(FMath::FRandRange(1.f, 2.2f)));
	M->SetCollisionProfileName(TEXT("PhysicsActor"));
	M->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // falling rocks are scenery, never a trap
	Rock->SetRootComponent(M);
	M->RegisterComponent();
	M->SetSimulatePhysics(true);
	M->SetPhysicsLinearVelocity(FVector(0.f, 0.f, -1500.f));
	UTDStatics::TintPlaceholder(M, FLinearColor(0.5f, 0.2f, 0.9f), 0.8f);
	Rock->SetLifeSpan(6.f);
	Debris.Add(Rock);
}

void ATDCollapseRun::ResetProp()
{
	TimeLeft = -1.f;
	if (Tower)
	{
		Tower->SetActorLocation(TowerHome);
		Tower->SetActorRotation(FRotator::ZeroRotator);
	}
	for (const TWeakObjectPtr<AActor>& D : Debris)
	{
		if (D.IsValid())
		{
			D->Destroy();
		}
	}
	Debris.Reset();
}

#undef LOCTEXT_NAMESPACE

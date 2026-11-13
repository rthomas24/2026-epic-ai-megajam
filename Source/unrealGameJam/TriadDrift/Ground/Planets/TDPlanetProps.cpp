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
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Story/TDStorySubsystem.h"

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
		if (LinkedObjective && LinkedStep != INDEX_NONE && LinkedStep < LinkedObjective->GetCheckpointStep())
		{
			return; // a step before the checkpoint: stays done
		}
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
	BeaconMesh = TDPropArt::Default(TEXT("SM_StepBeacon"));
}

void ATDStepTrigger::BeginPlay()
{
	Super::BeginPlay();
	Box->OnComponentBeginOverlap.AddUniqueDynamic(this, &ATDStepTrigger::HandleOverlap);
	UTDStatics::TintPlaceholder(Beacon, bRequireCarrying ? FLinearColor(0.3f, 1.f, 0.5f) : FLinearColor(0.4f, 0.9f, 1.f), 3.f);
	TDPropArt::Dress(Beacon, TDPropArt::Load(BeaconMesh));
	Beacon->SetVisibility(false, true);
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
	Beacon->SetVisibility(bCurrent, true);
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
	Prompt = LOCTEXT("Extract", "Recover the device");
	Core = TDPlanetPropsImpl::MakeMesh(this, TEXT("Core"), TDPlanetPropsImpl::SpherePath, RootComponent);
	Core->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	Core->SetRelativeScale3D(FVector(0.55f));
	Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Core->SetCastShadow(false);
	CoreMesh = TDPropArt::Default(TEXT("SM_ExtractionCore"));
}

void ATDExtractionSite::BeginPlay()
{
	Super::BeginPlay();
	UTDStatics::TintPlaceholder(Core, FLinearColor(0.5f, 1.f, 0.6f), 4.f);
	TDPropArt::Dress(Core, TDPropArt::Load(CoreMesh));
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
	Core->SetVisibility(false, true);
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
	Core->SetVisibility(true, true);
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
	SurfaceMesh = TDPropArt::Default(TEXT("SM_Cind_MagmaSurface"));
}

void ATDMagmaVolume::SetSize(const FVector& Size)
{
	// The trigger ends 10 cm above the surface: standing in the magma burns, standing on a stone or a
	// bridge slab over it (tops 60+ cm) never does.
	Box->SetBoxExtent(FVector(Size.X * 0.5f, Size.Y * 0.5f, Size.Z * 0.5f));
	Box->SetRelativeLocation(FVector(0.f, 0.f, -Size.Z * 0.5f + 10.f));
	Surface->SetRelativeScale3D(FVector(Size.X, Size.Y, 10.f) / 100.f);
	Surface->SetRelativeLocation(FVector(0.f, 0.f, -5.f));
}

void ATDMagmaVolume::BeginPlay()
{
	Super::BeginPlay();
	// Deep red-orange at a modest glow: at 3x the placeholder bloomed to flat over-bright yellow from the arrival
	// altitude (Lead's audit, Nov 13). Until a SM_Cind_MagmaSurface exists (the slot dresses it automatically).
	UTDStatics::TintPlaceholder(Surface, FLinearColor(0.85f, 0.12f, 0.02f), 1.1f);
	if (UStaticMeshComponent* Art = TDPropArt::Dress(Surface, TDPropArt::Load(SurfaceMesh)))
	{
		Art->SetCastShadow(false); // a glowing surface lifted 2 cm by its WPO: its shadow drew a dark line along the edges
	}
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
	VentMesh = TDPropArt::Default(TEXT("SM_Cind_GeyserVent"));
	PlumeMesh = TDPropArt::Default(TEXT("SM_Cind_GeyserPlume"));
}

void ATDGeyser::BeginPlay()
{
	Super::BeginPlay();
	VentMID = UTDStatics::TintPlaceholder(Vent, FLinearColor(1.f, 0.35f, 0.05f), 0.15f);
	UTDStatics::TintPlaceholder(Plume, FLinearColor(1.f, 0.55f, 0.2f), 2.5f);
	TDPropArt::Dress(Vent, TDPropArt::Load(VentMesh));
	TDPropArt::Dress(Plume, TDPropArt::Load(PlumeMesh));
	Clock = Phase;
	Plume->SetVisibility(false, true);
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
	Plume->SetVisibility(bWarn || bErupt, true);
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
	Tags.Add(TEXT("TD.BladeBreakable")); // the blade breaks it (it's on no team)
	CrystalMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_01.SM_Cind_Crystal_01")));
}

void ATDShatterCrystal::BeginPlay()
{
	Super::BeginPlay();
	Health->InitMaxValues(MaxHealth, 0.f, true);
	Health->OnDeath.AddUniqueDynamic(this, &ATDShatterCrystal::HandleDeath);
	UTDStatics::TintPlaceholder(Mesh, Color, 1.5f);
	TDPropArt::Dress(Mesh, TDPropArt::Load(CrystalMesh));
}

void ATDShatterCrystal::HandleDeath(UTDHealthComponent*, const FTDDamageInfo&)
{
	bShattered = true;
	Mesh->SetVisibility(false, true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATDShatterCrystal::ApplyCompleted()
{
	bShattered = true;
	Mesh->SetVisibility(false, true);
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
	Mesh->SetVisibility(true, true);
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
	SlabMesh = TDPropArt::Default(TEXT("SM_Cind_CollapseSlab"));
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
	TDPropArt::Dress(Slab, TDPropArt::Load(SlabMesh));
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
				Slab->SetVisibility(false, true);
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
	Slab->SetVisibility(true, true);
	Slab->SetCollisionProfileName(TEXT("BlockAll"));
	UTDStatics::TintPlaceholder(Slab, FLinearColor(0.6f, 0.3f, 1.f), 1.2f);
}

// ================================================================================================
// Collapse run
// ================================================================================================

ATDCollapseRun::ATDCollapseRun()
{
	PrimaryActorTick.bCanEverTick = true;
	DebrisMesh = TDPropArt::Default(TEXT("SM_Cind_DebrisChunk"));
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
		LinkedObjective->UpdateStepText(FText::Format(RunText.IsEmpty() ? LOCTEXT("Run", "RETURN TO EXTRACTION · {0}") : RunText, Shown));
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
	TDPropArt::Dress(M, TDPropArt::Load(DebrisMesh));
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


// ================================================================================================
// Archive transfer (Verdance objective 1, reduced form)
// ================================================================================================

FTDOnSurveyTerminalActivated ATDArchiveTransfer::OnAnyTerminalActivated;

ATDArchiveTransfer::ATDArchiveTransfer()
{
	PrimaryActorTick.bCanEverTick = true;
	RunningText = LOCTEXT("TransferRun", "PROTECT THE DATA TRANSFER · {0}%");
	PausedText = LOCTEXT("TransferPaused", "TRANSFER PAUSED · CLEAR THE TERMINAL · {0}%");
	WaveAt = { 0.f, 0.25f, 0.5f };
}

void ATDArchiveTransfer::BeginPlay()
{
	Super::BeginPlay();
	Fired.Init(false, Waves.Num());
	if (LinkedObjective)
	{
		LinkedObjective->OnStepStarted.AddUniqueDynamic(this, &ATDArchiveTransfer::HandleStepStarted);
	}
}

bool ATDArchiveTransfer::GetArchiveTransfer(const UObject* WorldContextObject, float& OutProgress01, bool& bOutPaused, FText& OutLabel)
{
	OutProgress01 = 0.f;
	bOutPaused = false;
	OutLabel = FText::GetEmpty();
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	for (TActorIterator<ATDArchiveTransfer> It(World); World && It; ++It)
	{
		if (It->IsRunning())
		{
			OutProgress01 = It->Progress;
			bOutPaused = It->bPaused;
			OutLabel = It->Label;
			return true;
		}
	}
	return false;
}

bool ATDArchiveTransfer::IsHostileAtTerminal() const
{
	const FVector At = GetActorLocation();
	for (TActorIterator<ATDCrewCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist2D(It->GetActorLocation(), At) < PauseRadius)
		{
			return true;
		}
	}
	return false;
}

void ATDArchiveTransfer::HandleStepStarted(ATDObjective* Objective, int32 StepIndex)
{
	if (StepIndex == LinkedStep - 1)
	{
		bRunning = true;
		Progress = 0.f;
		OnTransferStarted.Broadcast();
		OnAnyTerminalActivated.Broadcast(this);
		UpdateLine(true);
	}
	else if (StepIndex == LinkedStep)
	{
		bRunning = true;
		Progress = FMath::Max(Progress, 0.5f); // the midpoint checkpoint: a retake picks up here
		for (int32 i = 0; i < Fired.Num(); ++i)
		{
			Fired[i] = Fired[i] && WaveAt.IsValidIndex(i) && WaveAt[i] < 0.5f; // the midpoint wave drops again on a retake
		}
		UpdateLine(true);
	}
}

void ATDArchiveTransfer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRunning || !LinkedObjective)
	{
		return;
	}
	const int32 Step = LinkedObjective->GetCurrentStepIndex();
	if (Step != LinkedStep - 1 && Step != LinkedStep)
	{
		bRunning = false; // finished, or the scene was cut
		return;
	}
	for (int32 i = 0; i < Waves.Num(); ++i)
	{
		if (!Fired[i] && WaveAt.IsValidIndex(i) && Progress >= WaveAt[i] && Waves[i])
		{
			Fired[i] = true;
			Waves[i]->Activate();
		}
	}
	const bool bWasPaused = bPaused;
	bPaused = IsHostileAtTerminal();
	if (bPaused != bWasPaused)
	{
		OnTransferPaused.Broadcast(bPaused);
		if (bPaused)
		{
			UTDStorySubsystem::Fire(this, TEXT("G3_Moss_NobodyNear")); // the first time one reaches the terminal (once per run)
		}
	}
	// T3's lines over the transfer (each once per run): the second wave, then nearly there.
	if (Progress >= 0.25f) { UTDStorySubsystem::Fire(this, TEXT("G3_Wren_LeftApproach")); }
	if (Progress >= 0.85f) { UTDStorySubsystem::Fire(this, TEXT("G3_Kato_AlmostThere")); }
	if (!bPaused)
	{
		const float Cap = Step == LinkedStep - 1 ? 0.5f : 1.f;
		Progress = FMath::Min(Cap, Progress + DeltaSeconds / FMath::Max(Seconds, 1.f));
	}
	UpdateLine(false);
	if (Step == LinkedStep - 1 && Progress >= 0.5f)
	{
		LinkedObjective->CompleteStep(Step); // the midpoint: the next step is the checkpoint
	}
	else if (Step == LinkedStep && Progress >= 1.f)
	{
		bRunning = false;
		OnTransferCompleted.Broadcast();
		LinkedObjective->CompleteStep(Step);
	}
}

void ATDArchiveTransfer::UpdateLine(bool bForce)
{
	const int32 Percent = FMath::FloorToInt(Progress * 100.f);
	if (!bForce && Percent == ShownPercent && bPaused == bShownPaused)
	{
		return;
	}
	ShownPercent = Percent;
	bShownPaused = bPaused;
	Label = FText::Format(bPaused ? PausedText : RunningText, Percent);
	const int32 Step = LinkedObjective ? LinkedObjective->GetCurrentStepIndex() : INDEX_NONE;
	if (Step == LinkedStep - 1 || Step == LinkedStep)
	{
		LinkedObjective->UpdateStepText(Label);
	}
}

void ATDArchiveTransfer::ResetProp()
{
	// A retake: from the midpoint once it was reached (the checkpoint is the second half), else from zero.
	const bool bPastMid = LinkedObjective && LinkedObjective->GetCheckpointStep() >= LinkedStep;
	Progress = bPastMid ? 0.5f : 0.f;
	bRunning = false;
	bPaused = false;
	ShownPercent = -1;
	for (int32 i = 0; i < Fired.Num(); ++i)
	{
		Fired[i] = WaveAt.IsValidIndex(i) && WaveAt[i] < Progress;
	}
}

#undef LOCTEXT_NAMESPACE

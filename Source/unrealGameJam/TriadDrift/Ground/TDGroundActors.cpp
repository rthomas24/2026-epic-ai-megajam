// Triad Drift - ground gameplay actors (ENG-24). Owner: Teammate 5.

#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Ground/TDGroundBarks.h"
#include "Misc/PackageName.h"
#include "Engine/StaticMesh.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Ground/Crew/TDCrewCover.h"
#include "TriadDrift/Ground/TDGroundScene.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TriadDrift/Ground/Squad/TDGroundSquadSubsystem.h" // #209-allies: the squad-of-three encounter scale
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

namespace TDGroundShapes
{
	static UStaticMesh* Cube()
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
		return Mesh.Object;
	}
	static UStaticMesh* Cylinder()
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		return Mesh.Object;
	}
	static UStaticMesh* Sphere()
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		return Mesh.Object;
	}

	/** Tint a placeholder mesh (Core's helper: works on any engine shape, with an optional glow). */
	static void Tint(UStaticMeshComponent* Mesh, const FLinearColor& Color, float Glow = 0.f)
	{
		UTDStatics::TintPlaceholder(Mesh, Color, Glow);
	}

	static APawn* PlayerPawn(const UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? PC->GetPawn() : nullptr;
	}
}

// ================================================================================================
// Interactable
// ================================================================================================

TSoftObjectPtr<UStaticMesh> TDPropArt::Default(const TCHAR* Name)
{
	return TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/Planet/%s.%s"), Name, Name)));
}

UStaticMesh* TDPropArt::Load(const TSoftObjectPtr<UStaticMesh>& Slot)
{
	if (Slot.IsNull())
	{
		return nullptr;
	}
	if (UStaticMesh* Loaded = Slot.Get())
	{
		return Loaded;
	}
	const FString Package = Slot.ToSoftObjectPath().GetLongPackageName();
	return FPackageName::DoesPackageExist(Package) ? Slot.LoadSynchronous() : nullptr;
}

UStaticMeshComponent* TDPropArt::Dress(UStaticMeshComponent* Shape, UStaticMesh* Art, bool bFit)
{
	if (!Shape || !Art || !Shape->GetStaticMesh() || !Shape->GetOwner())
	{
		return nullptr;
	}
	// Fit in the shape's own (unscaled) space, so the shape's scale, later resizes and moves all carry over.
	const FBox ShapeBox = Shape->GetStaticMesh()->GetBoundingBox();
	const FBox ArtBox = Art->GetBoundingBox();
	const FVector ArtSize = ArtBox.GetSize().ComponentMax(FVector(1.f));
	UStaticMeshComponent* A = NewObject<UStaticMeshComponent>(Shape->GetOwner());
	A->SetStaticMesh(Art);
	A->SetMobility(EComponentMobility::Movable);
	A->SetupAttachment(Shape);
	if (bFit)
	{
		// The shape's size in world-axis terms (its own scale applied), to compare long axes with the art's.
		const FVector Scale = Shape->GetRelativeScale3D();
		const FVector ShapeWorld = ShapeBox.GetSize() * Scale;
		// Turn only when both have a clear long horizontal axis and they disagree (a round or square art never turns).
		const bool bTurn = FMath::Abs(ShapeWorld.X - ShapeWorld.Y) > 1.f && FMath::Abs(ArtSize.X - ArtSize.Y) > 1.f
			&& (ShapeWorld.X > ShapeWorld.Y) != (ArtSize.X > ArtSize.Y);
		const FVector ArtAxes = bTurn ? FVector(ArtSize.Y, ArtSize.X, ArtSize.Z) : ArtSize; // art size along the shape's X/Y after the turn
		const FVector Fit = ShapeBox.GetSize() / ArtAxes;
		const FVector FitArt = bTurn ? FVector(Fit.Y, Fit.X, Fit.Z) : Fit; // the art's own axes
		const FRotator Turn(0.f, bTurn ? 90.f : 0.f, 0.f);
		A->SetRelativeRotation(Turn);
		A->SetRelativeScale3D(FitArt);
		A->SetRelativeLocation(ShapeBox.GetCenter() - Turn.RotateVector(ArtBox.GetCenter() * FitArt));
	}
	else
	{
		const FVector Inv = FVector(1.f) / Shape->GetComponentScale().ComponentMax(FVector(KINDA_SMALL_NUMBER));
		const FVector ShapeBottom(ShapeBox.GetCenter().X, ShapeBox.GetCenter().Y, ShapeBox.Min.Z);
		const FVector ArtBottom(ArtBox.GetCenter().X, ArtBox.GetCenter().Y, ArtBox.Min.Z);
		A->SetRelativeScale3D(Inv); // true size in the world
		A->SetRelativeLocation(ShapeBottom - ArtBottom * Inv);
	}
	A->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName); // the shape stays the gameplay collision
	A->SetCanEverAffectNavigation(false);
	A->SetVisibility(Shape->IsVisible());
	A->RegisterComponent();
	Shape->GetOwner()->AddInstanceComponent(A);
	Shape->SetHiddenInGame(true); // still blocks, still the trace target; visibility switches propagate to the art
	return A;
}

ATDInteractableActor::ATDInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetStaticMesh(TDGroundShapes::Cube());
	Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.2f));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->SetCollisionResponseToChannel(TDCollision::Interactable, ECR_Block);
	// A console is not ground: no stepping up onto it, its top is not walkable, and a steep cap (below) makes a pilot who
	// jumps up slide off (third-person playthrough, Nov 13: a pilot on the wreck console was out of every melee crewman's
	// reach and the site fight never ended).
	Mesh->SetWalkableSlopeOverride(FWalkableSlopeOverride(EWalkableSlopeBehavior::WalkableSlope_Unwalkable, 0.f));
	Mesh->CanCharacterStepUpOn = ECB_No;
	TopCap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopCap"));
	TopCap->SetupAttachment(Mesh);
	TopCap->SetStaticMesh(TDGroundShapes::Cylinder());
	TopCap->SetHiddenInGame(true);
	TopCap->SetCastShadow(false);
	TopCap->SetCanEverAffectNavigation(false);
	TopCap->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TopCap->SetCollisionObjectType(ECC_WorldStatic);
	TopCap->SetCollisionResponseToAllChannels(ECR_Ignore); // traces, the camera, bolts and interaction pass through
	TopCap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	TopCap->SetWalkableSlopeOverride(FWalkableSlopeOverride(EWalkableSlopeBehavior::WalkableSlope_Unwalkable, 0.f));
	TopCap->CanCharacterStepUpOn = ECB_No;
	// In the console's own unit-cube space: a column inside the square top (never wider than the console, so nothing
	// walking past - a charging Warden, a strafing pilot - catches on it), 2.5 console-heights tall: its top is far above
	// the pilot's double-jump reach (~2.2 m), and the corners left beside it are too small to stand on. (A steep cone
	// still let a character rest on its flank: CharacterMovement can sit still on an unwalkable slope while falling.)
	TopCap->SetRelativeScale3D(FVector(1.f, 1.f, 2.5f));
	ConsoleMesh = TDPropArt::Default(TEXT("SM_Console"));
}

void ATDInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	TDGroundShapes::Tint(Mesh, FLinearColor(0.1f, 0.7f, 0.9f)); // readable cyan console, solid
	// A console whose own art is missing (the board point's SM_BoardPoint does not exist yet) wears the kit console,
	// never the bare cyan placeholder box (it filled the bottom of the view at the pads).
	UStaticMesh* Art = TDPropArt::Load(ConsoleMesh);
	TDPropArt::Dress(Mesh, Art ? Art : TDPropArt::Load(TDPropArt::Default(TEXT("SM_Console"))));
	if (TopCap && TopCap->GetStaticMesh() && Mesh->GetStaticMesh())
	{
		// The column's base sits on the console's top face (both in the console mesh's local space).
		const float Top = (float)Mesh->GetStaticMesh()->GetBoundingBox().Max.Z;
		const float ConeBottom = (float)TopCap->GetStaticMesh()->GetBoundingBox().Min.Z * (float)TopCap->GetRelativeScale3D().Z;
		TopCap->SetRelativeLocation(FVector(0.f, 0.f, Top - ConeBottom));
	}
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->RegisterInteractable(this); // a retake makes it usable again while its objective is unfinished
	}
}

void ATDInteractableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool ATDInteractableActor::CanInteract(AActor* Interactor) const
{
	if (!bEnabled || (bSingleUse && bUsed))
	{
		return false;
	}
	if (LinkedObjective && LinkedObjective->GetCurrentStepIndex() != LinkedStep)
	{
		return false; // not this step yet
	}
	return true;
}

void ATDInteractableActor::Interact(AActor* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return;
	}
	bUsed = true;
	OnInteracted.Broadcast(this, Interactor);
	if (LinkedObjective && bCompleteLinkedStep)
	{
		LinkedObjective->CompleteStep(LinkedStep);
	}
}

// ================================================================================================
// Door
// ================================================================================================

ATDDoor::ATDDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(TDGroundShapes::Cube());
	Mesh->SetRelativeScale3D(FVector(0.3f, 4.f, 4.f));
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	Mesh->SetMobility(EComponentMobility::Movable);
	DoorMesh = TDPropArt::Default(TEXT("SM_Door"));
}

void ATDDoor::BeginPlay()
{
	Super::BeginPlay();
	ClosedRelative = Mesh->GetRelativeLocation();
	bWantOpen = bStartOpen;
	Open01 = bStartOpen ? 1.f : 0.f;
	Mesh->SetRelativeLocation(ClosedRelative + FVector(0.f, 0.f, OpenHeight * Open01));
	TDGroundShapes::Tint(Mesh, FLinearColor(0.9f, 0.35f, 0.1f));
	TDPropArt::Dress(Mesh, TDPropArt::Load(DoorMesh));
}

void ATDDoor::Open()
{
	if (!bWantOpen && bRecognisesPilot)
	{
		ShowAccessText();
	}
	bWantOpen = true;
}
void ATDDoor::Close() { bWantOpen = false; }

bool ATDDoor::IsAccessTextShown() const { return AccessTextLeft > 0.f; }

void ATDDoor::ShowStatusText(const FText& Text)
{
	EnsureLabels();
	StatusText = Text;
	for (UTextRenderComponent* Label : AccessLabels)
	{
		Label->SetText(Text);
		Label->SetVisibility(true);
	}
}

void ATDDoor::ClearStatusText()
{
	StatusText = FText::GetEmpty();
	if (AccessTextLeft <= 0.f)
	{
		for (UTextRenderComponent* Label : AccessLabels)
		{
			if (Label) { Label->SetVisibility(false); }
		}
	}
}

void ATDDoor::ShowAccessText()
{
	++AccessGreetings;
	AccessTextLeft = FMath::Max(AccessTextSeconds, 0.1f);
	EnsureLabels();
	for (UTextRenderComponent* Label : AccessLabels)
	{
		Label->SetText(AccessText);
		Label->SetVisibility(true);
	}
	if (!AccessSound.IsNull())
	{
		if (USoundBase* S = AccessSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySoundAtLocation(this, S, GetActorLocation() + FVector(0.f, 0.f, 300.f));
		}
	}
	UE_LOG(LogTemp, Log, TEXT("TDDoor %s: %s"), *GetName(), *AccessText.ToString());
}

void ATDDoor::EnsureLabels()
{
	if (AccessLabels.Num() == 0)
	{
		// Over the opening (the leaf's closed top), readable from both sides.
		const FVector Scale = Mesh->GetRelativeScale3D();
		const float Top = ClosedRelative.Z + 50.f * Scale.Z + 45.f;
		const float Depth = 50.f * Scale.X + 6.f;
		for (const float Side : { -1.f, 1.f })
		{
			UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
			Label->SetupAttachment(RootComponent);
			Label->SetRelativeLocation(FVector(Side * Depth, 0.f, Top));
			Label->SetRelativeRotation(FRotator(0.f, Side < 0.f ? 180.f : 0.f, 0.f));
			Label->SetHorizontalAlignment(EHTA_Center);
			Label->SetVerticalAlignment(EVRTA_TextCenter);
			Label->SetWorldSize(36.f);
			Label->SetTextRenderColor(FColor(150, 255, 230));
			Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Label->SetCastShadow(false);
			Label->RegisterComponent();
			AccessLabels.Add(Label);
		}
	}
}

void ATDDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (AccessTextLeft > 0.f)
	{
		AccessTextLeft -= DeltaSeconds;
		if (AccessTextLeft <= 0.f && StatusText.IsEmpty())
		{
			for (UTextRenderComponent* Label : AccessLabels)
			{
				if (Label) { Label->SetVisibility(false); }
			}
		}
	}
	const float Target = bWantOpen ? 1.f : 0.f;
	if (Open01 == Target)
	{
		return;
	}
	const float Step = DeltaSeconds / FMath::Max(OpenSeconds, 0.01f);
	Open01 = Target > Open01 ? FMath::Min(Target, Open01 + Step) : FMath::Max(Target, Open01 - Step);
	Mesh->SetRelativeLocation(ClosedRelative + FVector(0.f, 0.f, OpenHeight * FMath::SmoothStep(0.f, 1.f, Open01)));
}

// ================================================================================================
// Crew stand-in
// ================================================================================================

ATDCrewDummy::ATDCrewDummy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetStaticMesh(TDGroundShapes::Cube());
	Body->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.8f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->MaxShield = 0.f;
	Health->ShieldRegenFractionPerSecond = 0.f;
	Health->DefaultTeam = ETDTeam::Enemy;
	Health->DestroyOwnerAfterDeathSeconds = 2.f;

	AutoPossessAI = EAutoPossessAI::Disabled;
	GetCharacterMovement()->bRunPhysicsWithNoController = true; // knockback works without AI (ENG-23 adds it)
}

void ATDCrewDummy::BeginPlay()
{
	Super::BeginPlay();
	Health->InitMaxValues(MaxHealth, 0.f, true);
	Health->OnDeath.AddUniqueDynamic(this, &ATDCrewDummy::HandleDeath);
	TDGroundShapes::Tint(Body, FLinearColor(0.6f, 0.06f, 0.04f)); // Armada red, solid (the glow material is translucent)
}

void ATDCrewDummy::ApplyStagger(float Seconds, AActor* Source)
{
	StaggerLeft = FMath::Max(StaggerLeft, Seconds);
	WindupLeft = 0.f; // interrupted
}

float ATDCrewDummy::MeleeAttack(AActor* Target)
{
	if (!Target || IsStaggered() || !Health->IsAlive() || MeleeDamage <= 0.f)
	{
		return 0.f;
	}
	const FVector Dir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	return UTDStatics::ApplyTDDamage(Target, MeleeDamage, ETDDamageKind::Melee, this, this, Target->GetActorLocation() - Dir * 40.f, Dir);
}

void ATDCrewDummy::FellOutOfWorld(const UDamageType& DmgType)
{
	if (Health && Health->IsAlive())
	{
		Health->Kill(nullptr, ETDDamageKind::Environmental);
	}
	Destroy();
}

void ATDCrewDummy::HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow)
{
	WindupLeft = 0.f;
	SetActorEnableCollision(false);
	Body->SetVisibility(false);
}

void ATDCrewDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Health->IsAlive())
	{
		return;
	}
	StaggerLeft = FMath::Max(0.f, StaggerLeft - DeltaSeconds);
	SwingCooldown = FMath::Max(0.f, SwingCooldown - DeltaSeconds);
	if (MeleeInterval <= 0.f || IsStaggered())
	{
		return;
	}
	APawn* Player = TDGroundShapes::PlayerPawn(GetWorld());
	if (!Player)
	{
		return;
	}
	const FVector To = Player->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
	const bool bInRange = To.Size2D() <= MeleeRange + 60.f;
	if (WindupLeft > 0.f)
	{
		WindupLeft -= DeltaSeconds;
		if (WindupLeft <= 0.f && bInRange)
		{
			MeleeAttack(Player);
		}
	}
	else if (bInRange && SwingCooldown <= 0.f)
	{
		WindupLeft = MeleeWindup; // telegraph (Body flashes via IsTelegraphing for presentation)
		SwingCooldown = MeleeInterval;
	}
}

// ================================================================================================
// Turret
// ================================================================================================

ATDDummyTurret::ATDDummyTurret()
{
	PrimaryActorTick.bCanEverTick = true;
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	RootComponent = Base;
	Base->SetStaticMesh(TDGroundShapes::Cylinder());
	Base->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.2f));
	Base->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(Base);
	Head->SetStaticMesh(TDGroundShapes::Sphere());
	Head->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Head->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.6f));
	Head->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->MaxShield = 0.f;
	Health->ShieldRegenFractionPerSecond = 0.f;
	Health->DefaultTeam = ETDTeam::Enemy;
}

void ATDDummyTurret::BeginPlay()
{
	Super::BeginPlay();
	TDGroundShapes::Tint(Base, FLinearColor(0.12f, 0.1f, 0.1f));
	TDGroundShapes::Tint(Head, FLinearColor(1.f, 0.25f, 0.05f), 1.5f); // red-orange glowing eye
}

AActor* ATDDummyTurret::FindTarget() const
{
	APawn* Player = TDGroundShapes::PlayerPawn(GetWorld());
	const UTDHealthComponent* PH = Player ? UTDStatics::GetHealth(Player) : nullptr;
	if (!Player || (PH && PH->IsDead()) || FVector::Dist(Player->GetActorLocation(), GetActorLocation()) > MaxRange)
	{
		return nullptr;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDTurretLOS), false, this);
	Params.AddIgnoredActor(Player);
	FHitResult Hit;
	const FVector Muzzle = Head->GetComponentLocation();
	if (GetWorld()->LineTraceSingleByChannel(Hit, Muzzle, Player->GetActorLocation(), ECC_Visibility, Params))
	{
		return nullptr; // no line of sight
	}
	return Player;
}

AActor* ATDDummyTurret::FireAt(AActor* Target)
{
	UWorld* World = GetWorld();
	if (!World || !Target || !Health->IsAlive())
	{
		return nullptr;
	}
	FTDProjectileSpec Spec;
	Spec.Damage = BoltDamage;
	Spec.DamageKind = ETDDamageKind::Energy;
	Spec.Speed = BoltSpeed;
	Spec.Range = MaxRange * 1.5f;
	Spec.CollisionRadius = 25.f;
	Spec.bInheritShooterVelocity = false;

	const FVector Muzzle = Head->GetComponentLocation();
	const FVector Dir = (Target->GetActorLocation() - Muzzle).GetSafeNormal();
	const FTransform Xf(Dir.Rotation(), Muzzle + Dir * 80.f);
	return ATDProjectile::Launch(World, Spec, Xf, this, ETDTeam::Enemy, nullptr, FVector::ZeroVector, 1.f, false);
}

void ATDDummyTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive || !Health->IsAlive())
	{
		return;
	}
	if (ShotsLeft > 0)
	{
		ShotTimer -= DeltaSeconds;
		if (ShotTimer <= 0.f)
		{
			if (AActor* Target = FindTarget())
			{
				FireAt(Target);
			}
			--ShotsLeft;
			ShotTimer = ShotSpacing;
		}
		return;
	}
	BurstTimer -= DeltaSeconds;
	if (BurstTimer <= 0.f)
	{
		BurstTimer = BurstInterval;
		if (FindTarget())
		{
			ShotsLeft = ShotsPerBurst;
			ShotTimer = 0.f;
		}
	}
}

// ================================================================================================
// Encounter volume
// ================================================================================================

ATDEncounterVolume::ATDEncounterVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;
	Box->SetBoxExtent(FVector(600.f, 600.f, 300.f));
	Box->SetCollisionProfileName(UCollisionProfile::DefaultProjectile_ProfileName);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Box->SetGenerateOverlapEvents(true);
	CrewClass = ATDCrewCharacter::StaticClass();
}

void ATDEncounterVolume::BeginPlay()
{
	Super::BeginPlay();
	Box->OnComponentBeginOverlap.AddUniqueDynamic(this, &ATDEncounterVolume::HandleOverlap);
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->RegisterEncounter(this); // the one reset path on a retake
	}

	ActivateIfPlayerInside();
}

void ATDEncounterVolume::MarkClearedOnLoad()
{
	if (bCleared)
	{
		return;
	}
	bActivated = true;
	bCleared = true;
	for (ATDDoor* D : DoorsToOpen)
	{
		if (D) { D->Open(); }
	}
	for (ATDDoor* D : DoorsToLock)
	{
		if (D) { D->Open(); }
	}
}

void ATDEncounterVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StragglerTimer);
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ATDEncounterVolume::HandleOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	if (bTriggerOnOverlap && IsPlayerPawn(Other))
	{
		Activate();
	}
}

bool ATDEncounterVolume::IsPlayerPawn(const AActor* Actor) const
{
	// The on-foot player: the first-person character (whoever drives it), or any player-controlled pawn.
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && UTDStatics::GetTeam(Pawn) == ETDTeam::Player && (Pawn->IsPlayerControlled() || Pawn->IsA<ATDFirstPersonCharacter>());
}

bool ATDEncounterVolume::ActivateIfPlayerInside()
{
	if (!bTriggerOnOverlap || bActivated || !Box)
	{
		return false;
	}
	TArray<AActor*> Inside;
	Box->GetOverlappingActors(Inside, APawn::StaticClass());
	for (AActor* A : Inside)
	{
		if (IsPlayerPawn(A))
		{
			Activate();
			return bActivated;
		}
	}
	return false;
}

void ATDEncounterVolume::HandleStepStarted(ATDObjective* Objective, int32 StepIndex)
{
	if (StepIndex == LinkedStep)
	{
		ActivateIfPlayerInside(); // the player may already be standing in the room
	}
}

void ATDEncounterVolume::AdoptMember(AActor* Member)
{
	if (!Member || Spawned.Contains(Member))
	{
		return;
	}
	Spawned.Add(Member);
	if (UTDHealthComponent* H = UTDStatics::GetHealth(Member))
	{
		H->OnDeath.AddUniqueDynamic(this, &ATDEncounterVolume::HandleMemberDeath);
	}
	Member->OnDestroyed.AddUniqueDynamic(this, &ATDEncounterVolume::HandleMemberDestroyed);
	if (ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(Member))
	{
		Crew->OwningEncounter = this;
	}
}

bool ATDEncounterVolume::ReturnLostBoss(AActor* Member)
{
	// T7 (Nov 14): after a CUT the Warden's room came back cleared with no Warden, completing the scene. A member that
	// is destroyed while still alive left the world (fell through a gap, kill-Z); for a boss that must never clear the
	// room, so a fresh one is put back where the first was spawned.
	const ATDCrewCharacter* Lost = Cast<ATDCrewCharacter>(Member);
	const UTDHealthComponent* H = Member ? UTDStatics::GetHealth(Member) : nullptr;
	const FTransform* At = SpawnedAt.Find(Member);
	// Never capped: a room whose boss keeps falling out must not count as won (T3's edge case: after 3 returns the room
	// cleared); from the second return it comes back at the room's middle.
	if (!bActivated || bCleared || !Lost || Lost->Spec.Type != ETDCrewType::Warden || !H || !H->IsAlive() || !At || !GetWorld())
	{
		return false;
	}
	++BossReturns;
	// The first time where it was spawned; if it is lost again, the room's middle (its spawn spot may be over a gap).
	FTransform Xf = *At;
	if (BossReturns > 1)
	{
		Xf.SetLocation(FVector(GetActorLocation().X, GetActorLocation().Y, At->GetLocation().Z + 50.f));
	}
	AActor* Fresh = GetWorld()->SpawnActorDeferred<AActor>(Member->GetClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(Fresh);
	if (!Crew)
	{
		return false;
	}
	Crew->CrewType = ETDCrewType::Warden;
	Crew->FinishSpawning(Xf);
	if (const FTDRetakeMercyTier* Mercy = GetActiveMercy()) { Crew->ApplyMercy(Mercy->HealthScale, Mercy->MaxSummons); }
	Crew->SetOpeningGrace(OpeningGraceSeconds);
	UE_LOG(LogTriadDrift, Warning, TEXT("%s: the Warden %s left the world alive (at %s); %s put back at %s (return %d)"), *GetName(), *Member->GetName(),
		*Member->GetActorLocation().ToCompactString(), *Crew->GetName(), *Xf.GetLocation().ToCompactString(), BossReturns);
	SpawnedAt.Add(Crew, Xf);
	AdoptMember(Crew);
	return true;
}

void ATDEncounterVolume::HandleMemberDestroyed(AActor* Member)
{
	ReturnLostBoss(Member);
	Spawned.RemoveAll([Member](const TWeakObjectPtr<AActor>& M) { return !M.IsValid() || M.Get() == Member; });
	// Still "alive" by its health component while it is being destroyed: it counts as gone.
	if (bActivated && !bCleared && GetAliveCount() == 0)
	{
		CheckCleared();
	}
}

void ATDEncounterVolume::Activate()
{
	if (bActivated)
	{
		return;
	}
	// Only fight when it's this encounter's turn in its objective.
	if (LinkedObjective && LinkedStep != INDEX_NONE && LinkedObjective->GetCurrentStepIndex() != LinkedStep)
	{
		return;
	}
	bActivated = true;
	Stragglers.Reset();
	SpawnedAt.Reset();
	BossReturns = 0;
	GetWorldTimerManager().SetTimer(StragglerTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { SweepStragglers(1.f); }), 1.f, true);
	bCleared = false;
	++Take;
	const FTDRetakeMercyTier* Mercy = GetActiveMercy();
	if (Mercy)
	{
		UE_LOG(LogTriadDrift, Log, TEXT("Director's mercy: %s take %d eases the fight (health %.0f%%, summons %d, %d fewer crew)"), *GetName(), Take,
			Mercy->HealthScale * 100.f, Mercy->MaxSummons, Mercy->DropCrew);
	}
	if (UTDCrewCoverSubsystem* Cover = UTDCrewCoverSubsystem::Get(this))
	{
		for (AActor* Block : CoverActors)
		{
			Cover->RegisterCoverActor(Block);
		}
	}
	for (ATDDoor* Door : DoorsToLock)
	{
		if (Door) { Door->Close(); }
	}

	UWorld* World = GetWorld();
	UClass* Class = CrewClass ? CrewClass.Get() : ATDCrewCharacter::StaticClass();
	APawn* Player = TDGroundShapes::PlayerPawn(World);
	TArray<FVector> Points = SpawnPoints;
	if (Points.Num() == 0)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			const float Angle = 2.f * PI * i / FMath::Max(Count, 1);
			Points.Add(FVector(FMath::Cos(Angle) * RingRadius, FMath::Sin(Angle) * RingRadius, 0.f));
		}
	}
	if (Mercy && Mercy->DropCrew > 0 && Points.Num() > 1)
	{
		Points.SetNum(FMath::Max(1, Points.Num() - Mercy->DropCrew));
	}
	// ENG-45: the difficulty's crew count, as the Armada's pack sizes (a lone boss stays alone; named crews, the
	// reactor's future pilots, will be exempt when they land).
	if (Points.Num() > 1 && MemberNames.Num() == 0) // named crews (the reactor's future pilots) are exactly who they are
	{
		const int32 Want = UTDDifficultySettings::ScaleCount(Points.Num(), UTDDifficultySettings::GetScale().WaveSize
			* UTDGroundSquadSubsystem::GetEncounterScale()); // #209-allies: x1.6 (td.Ground.SquadScale) while the two allies are on; 1 otherwise and in tests
		const int32 Base = Points.Num();
		for (int32 i = Base; i < Want; ++i)
		{
			// Inside the fight, never outward: a point pushed out of a room spawned crew behind its wall (Played.Cinder on Ace).
			const FVector& From = Points[i % Base];
			const FVector Inward = From.Size2D() > 200.f ? From * 0.7f : From + FVector(0.f, 150.f, 0.f);
			Points.Add(FVector(Inward.X, Inward.Y, From.Z));
		}
		Points.SetNum(FMath::Min(Points.Num(), Want));
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector Center = GetActorLocation();
	// Never on top of the pilot (T7's Cinder play, Nov 14: crew 1.2 m from the retake spot, 24 CUTs in 8 minutes): a
	// point closer than MinSpawnDistance moves straight away from the pilot, kept inside the volume; if the volume is too
	// small for that, its corner farthest from the pilot.
	if (Player && MinSpawnDistance > 0.f && Box)
	{
		const FVector Ext = Box->GetScaledBoxExtent();
		const FVector PL = GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
		for (FVector& P : Points)
		{
			const FVector2D D(P.X - PL.X, P.Y - PL.Y);
			if (D.Size() >= MinSpawnDistance)
			{
				continue;
			}
			const FVector2D Dir = D.IsNearlyZero() ? FVector2D(1.f, 0.f) : D.GetSafeNormal();
			FVector2D Want(PL.X + Dir.X * MinSpawnDistance, PL.Y + Dir.Y * MinSpawnDistance);
			Want.X = FMath::Clamp(Want.X, -Ext.X + 100.f, Ext.X - 100.f);
			Want.Y = FMath::Clamp(Want.Y, -Ext.Y + 100.f, Ext.Y - 100.f);
			if (FVector2D::Distance(Want, FVector2D(PL.X, PL.Y)) < MinSpawnDistance * 0.7f)
			{
				Want = FVector2D(PL.X > 0.f ? -Ext.X + 100.f : Ext.X - 100.f, PL.Y > 0.f ? -Ext.Y + 100.f : Ext.Y - 100.f);
			}
			P.X = Want.X;
			P.Y = Want.Y;
		}
	}
	// A retake gives the pilot a longer beat before the first shot.
	const float Grace = Take >= 2 ? FMath::Max(OpeningGraceSeconds, RetakeGraceSeconds) : OpeningGraceSeconds;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FVector Loc = GetActorTransform().TransformPosition(Points[i]);
		const FTransform Xf(FRotator(0.f, (Center - Loc).Rotation().Yaw, 0.f), Loc);
		AActor* Member = World->SpawnActorDeferred<AActor>(Class, Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Member)
		{
			continue;
		}
		if (ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(Member))
		{
			Crew->CrewType = CrewMix.Num() > 0 ? CrewMix[i % CrewMix.Num()] : ETDCrewType::Grunt;
			if (MemberNames.Num() > 0)
			{
				Crew->FuturePilot = MemberNames[i % MemberNames.Num()];
			}
		}
		Member->FinishSpawning(Xf);
		if (ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(Member))
		{
			if (Mercy)
			{
				Crew->ApplyMercy(Mercy->HealthScale, Mercy->MaxSummons);
			}
			Crew->SetOpeningGrace(Grace);
		}
		// The fight starts now: everyone already knows where the player is.
		if (ATDCrewAIController* AI = Cast<ATDCrewAIController>(Cast<APawn>(Member) ? Cast<APawn>(Member)->GetController() : nullptr))
		{
			AI->SetTarget(Player);
		}
		SpawnedAt.Add(Member, Xf);
		AdoptMember(Member);
	}
	if (UTDGroundBarks* Barks = UTDGroundBarks::Get(this)) { Barks->NotifyEncounterStarted(this); } // contact, a new crew type (ENG-43)
	CheckCleared(); // nothing spawned (bad class) still opens the way rather than soft-locking
}

const FTDRetakeMercyTier* ATDEncounterVolume::GetActiveMercy() const
{
	const FTDRetakeMercyTier* Best = nullptr;
	for (const FTDRetakeMercyTier& T : MercyTiers)
	{
		if (Take >= T.FromTake && (!Best || T.FromTake > Best->FromTake))
		{
			Best = &T;
		}
	}
	return Best;
}

int32 ATDEncounterVolume::GetAliveCount() const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<AActor>& M : Spawned)
	{
		const AActor* A = M.Get();
		const UTDHealthComponent* H = (A && !A->IsActorBeingDestroyed()) ? UTDStatics::GetHealth(A) : nullptr;
		if (H && H->IsAlive())
		{
			++Alive;
		}
	}
	return Alive;
}

void ATDEncounterVolume::HandleMemberDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow)
{
	CheckCleared();
}

void ATDEncounterVolume::SweepStragglers(float Seconds)
{
	if (!bActivated || bCleared || StragglerSeconds <= 0.f)
	{
		GetWorldTimerManager().ClearTimer(StragglerTimer);
		return;
	}
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* Pilot = PC ? PC->GetPawn() : nullptr;
	for (TActorIterator<ATDFirstPersonCharacter> It(GetWorld()); It && !Pilot; ++It) // tests and played runs drive the pilot by AI
	{
		if (!It->IsDead()) { Pilot = *It; }
	}
	if (!Pilot)
	{
		return;
	}
	const FVector Eye = Pilot->GetPawnViewLocation();
	TArray<AActor*> Remove;
	for (const TWeakObjectPtr<AActor>& M : Spawned)
	{
		AActor* A = M.Get();
		const UTDHealthComponent* H = A && !A->IsActorBeingDestroyed() ? UTDStatics::GetHealth(A) : nullptr;
		const ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(A);
		if (!H || !H->IsAlive() || (Crew && Crew->Spec.Type == ETDCrewType::Warden))
		{
			continue;
		}
		FStraggler& S = Stragglers.FindOrAdd(M);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TDStraggler), false, A);
		Params.AddIgnoredActor(Pilot);
		const bool bSeen = !GetWorld()->LineTraceSingleByChannel(Hit, A->GetActorLocation() + FVector(0.f, 0.f, 60.f), Eye, ECC_Visibility, Params);
		const float Health = H->GetHull() + H->GetShield();
		if (bSeen || (S.LastHealth >= 0.f && Health < S.LastHealth))
		{
			S.Unseen = 0.f;
		}
		else
		{
			S.Unseen += Seconds;
		}
		S.LastHealth = Health;
		if (S.Unseen >= StragglerSeconds)
		{
			Remove.Add(A);
		}
	}
	for (AActor* A : Remove)
	{
		UE_LOG(LogTriadDrift, Display, TEXT("%s: %s removed, out of the pilot's sight and untouched for %.0f s (%.0f m away)"), *GetName(), *A->GetName(),
			StragglerSeconds, FVector::Dist(A->GetActorLocation(), Pilot->GetActorLocation()) / 100.f);
		Stragglers.Remove(A);
		A->Destroy(); // OnDestroyed: counts as gone, and the room clears when it was the last
	}
}

void ATDEncounterVolume::CheckCleared()
{
	if (!bActivated || bCleared || GetAliveCount() > 0)
	{
		return;
	}
	bCleared = true;
	UE_LOG(LogTriadDrift, Display, TEXT("%s cleared (take %d, %d spawned this take)%s"), *GetName(), Take, Spawned.Num(),
		LinkedObjective && LinkedStep != INDEX_NONE ? *FString::Printf(TEXT(": completes %s step %d"), *LinkedObjective->ObjectiveId.ToString(), LinkedStep) : TEXT(""));
	for (ATDDoor* Door : DoorsToOpen)
	{
		if (Door) { Door->Open(); }
	}
	for (ATDDoor* Door : DoorsToLock)
	{
		if (Door) { Door->Open(); }
	}
	OnCleared.Broadcast(this);
	if (UTDGroundBarks* Barks = UTDGroundBarks::Get(this)) { Barks->NotifyEncounterCleared(this); } // "that's the lot"
	if (LinkedObjective && LinkedStep != INDEX_NONE)
	{
		LinkedObjective->CompleteStep(LinkedStep);
	}
}

void ATDEncounterVolume::ResetEncounter()
{
	// Disarm first: destroying members fires OnDestroyed, which must not read as "room cleared".
	bActivated = false;
	bCleared = false;
	const TArray<TWeakObjectPtr<AActor>> Members = MoveTemp(Spawned);
	Spawned.Reset();
	for (const TWeakObjectPtr<AActor>& M : Members)
	{
		if (AActor* A = M.Get())
		{
			A->OnDestroyed.RemoveDynamic(this, &ATDEncounterVolume::HandleMemberDestroyed);
			A->Destroy();
		}
	}
	for (ATDDoor* Door : DoorsToOpen)
	{
		if (Door) { Door->Close(); }
	}
	for (ATDDoor* Door : DoorsToLock)
	{
		if (Door) { Door->Open(); }
	}
}



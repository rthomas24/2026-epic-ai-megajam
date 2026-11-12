// Triad Drift - ground gameplay actors (ENG-24). Owner: Teammate 5.

#include "TriadDrift/Ground/TDGroundActors.h"
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
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
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

ATDInteractableActor::ATDInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetStaticMesh(TDGroundShapes::Cube());
	Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.2f));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->SetCollisionResponseToChannel(TDCollision::Interactable, ECR_Block);
}

void ATDInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	TDGroundShapes::Tint(Mesh, FLinearColor(0.1f, 0.7f, 0.9f)); // readable cyan console, solid
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
}

void ATDDoor::BeginPlay()
{
	Super::BeginPlay();
	ClosedRelative = Mesh->GetRelativeLocation();
	bWantOpen = bStartOpen;
	Open01 = bStartOpen ? 1.f : 0.f;
	Mesh->SetRelativeLocation(ClosedRelative + FVector(0.f, 0.f, OpenHeight * Open01));
	TDGroundShapes::Tint(Mesh, FLinearColor(0.9f, 0.35f, 0.1f));
}

void ATDDoor::Open() { bWantOpen = true; }
void ATDDoor::Close() { bWantOpen = false; }

void ATDDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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

void ATDEncounterVolume::HandleMemberDestroyed(AActor* Member)
{
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
	bCleared = false;
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
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector Center = GetActorLocation();
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
		}
		Member->FinishSpawning(Xf);
		// The fight starts now: everyone already knows where the player is.
		if (ATDCrewAIController* AI = Cast<ATDCrewAIController>(Cast<APawn>(Member) ? Cast<APawn>(Member)->GetController() : nullptr))
		{
			AI->SetTarget(Player);
		}
		AdoptMember(Member);

	}
	CheckCleared(); // nothing spawned (bad class) still opens the way rather than soft-locking
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

void ATDEncounterVolume::CheckCleared()
{
	if (!bActivated || bCleared || GetAliveCount() > 0)
	{
		return;
	}
	bCleared = true;
	for (ATDDoor* Door : DoorsToOpen)
	{
		if (Door) { Door->Open(); }
	}
	for (ATDDoor* Door : DoorsToLock)
	{
		if (Door) { Door->Open(); }
	}
	OnCleared.Broadcast(this);
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



// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDDominion.h"

#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMeshComponent* TDDominion_MakePart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh, bool bCollide)
	{
		UStaticMeshComponent* C = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetStaticMesh(Mesh);
		C->SetCollisionProfileName(bCollide ? TEXT("BlockAllDynamic") : TEXT("NoCollision"));
		return C;
	}

	FTDDominionStage TDDominion_MakeStage(float Launch, float Cooldown, float Telegraph)
	{
		FTDDominionStage S;
		S.LaunchInterval = Launch;
		S.TurretBurstCooldown = Cooldown;
		S.TurretTelegraph = Telegraph;
		return S;
	}

	FTDWaveGroup TDDominion_MakeGroup(FName Id, int32 PerPack)
	{
		FTDWaveGroup G;
		G.EnemyId = Id;
		G.Packs = 1;
		G.PerPack = PerPack;
		return G;
	}
}

ATDDominion::ATDDominion()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* CubeMesh = Cube.Succeeded() ? Cube.Object : nullptr;
	UStaticMesh* CylMesh = Cylinder.Succeeded() ? Cylinder.Object : nullptr;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Placeholder hull: one long slab, a bridge tower and two trench walls (engine cubes are 100 cm).
	HullMesh = TDDominion_MakePart(this, Root, TEXT("Hull"), CubeMesh, true);
	BridgeMesh = TDDominion_MakePart(this, Root, TEXT("Bridge"), CubeMesh, true);
	TrenchWallLeft = TDDominion_MakePart(this, Root, TEXT("TrenchWallL"), CubeMesh, true);
	TrenchWallRight = TDDominion_MakePart(this, Root, TEXT("TrenchWallR"), CubeMesh, true);

	// Shield-section emitters: four glowing towers on the hull, one goes dark per generator kill.
	// (A real shield bubble is Nick's ShieldIdle cue; an opaque placeholder sphere would hide the fight.)
	for (int32 i = 0; i < 4; ++i)
	{
		ShieldSections.Add(TDDominion_MakePart(this, Root, *FString::Printf(TEXT("ShieldEmitter%d"), i), CylMesh, false));
	}

	HangarPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Hangar"));
	HangarPoint->SetupAttachment(Root);

	TurretGuns.Kind = ETDEnemyAttack::Guns;
	TurretGuns.WeaponId = TDEnemyWeaponIds::HeavyBolt;
	TurretGuns.Damage = 20.f;
	TurretGuns.Range = 25000.f;
	TurretGuns.ConeHalfAngleDeg = 5.f;
	TurretGuns.TelegraphSeconds = 1.2f;

	Stages.Add(TDDominion_MakeStage(15.f, 2.5f, 1.2f));	// shields up
	Stages.Add(TDDominion_MakeStage(12.f, 2.0f, 1.0f));	// reactor phase 1
	Stages.Add(TDDominion_MakeStage(9.f, 1.5f, 0.9f));		// phase 2
	Stages.Add(TDDominion_MakeStage(6.f, 1.0f, 0.8f));		// phase 3

	LaunchGroups.Add(TDDominion_MakeGroup(TDEnemyIds::ShardFighter, 4));
	LaunchGroups.Add(TDDominion_MakeGroup(TDEnemyIds::HookInterceptor, 2));
	LaunchGroups.Add(TDDominion_MakeGroup(TDEnemyIds::ShardFighter, 5));
}

ATDDominion* ATDDominion::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ATDDominion> It(World); It; ++It)
	{
		if (IsValid(*It)) { return *It; }
	}
	return nullptr;
}

// --------------------------------------------------------------------------------------- lifecycle

void ATDDominion::BeginPlay()
{
	Super::BeginPlay();
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnTakeStarted.AddUniqueDynamic(this, &ATDDominion::HandleTakeStarted);
	}

	// Nick's asset by convention when none is set (no gameplay BP edit needed).
	if (!Visuals)
	{
		Visuals = UTDStatics::LoadPresentation<UTDDominionVisuals>(TEXT("DA_Dominion_Visuals"));
	}
	if (!TurretVisuals)
	{
		TurretVisuals = UTDStatics::LoadPresentation<UTDEnemyVisuals>(TEXT("DA_DominionTurret_Visuals"));
	}

	LayoutParts();

	LayoutParts();
	ShowHull(false);
	SetState(ETDDominionState::Hidden);
	SyncWithProgress();
}

void ATDDominion::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutParts();	// a placed Dominion shows its real shape in the editor
}

void ATDDominion::LayoutParts()
{
	// Lay the placeholder out from HullExtent so tuning the size keeps everything consistent.
	const FVector E = HullExtent;
	HullMesh->SetRelativeScale3D(E * 2.f / 100.f);
	BridgeMesh->SetRelativeLocation(FVector(-E.X * 0.65f, 0.f, E.Z * 1.6f));
	BridgeMesh->SetRelativeScale3D(FVector(E.X * 0.25f, E.Y * 0.5f, E.Z * 1.2f) / 100.f);
	const float TrenchZ = E.Z + 1500.f;
	TrenchWallLeft->SetRelativeLocation(FVector(E.X * 0.1f, -2500.f, TrenchZ));
	TrenchWallRight->SetRelativeLocation(FVector(E.X * 0.1f, 2500.f, TrenchZ));
	TrenchWallLeft->SetRelativeScale3D(FVector(E.X * 1.4f, 400.f, 3000.f) / 100.f);
	TrenchWallRight->SetRelativeScale3D(FVector(E.X * 1.4f, 400.f, 3000.f) / 100.f);
	for (int32 i = 0; i < ShieldSections.Num(); ++i)
	{
		const float Sx = (i < 2 ? 0.55f : -0.35f) * E.X;
		const float Sy = (i % 2 ? 1.f : -1.f) * E.Y * 0.8f;
		ShieldSections[i]->SetRelativeLocation(FVector(Sx, Sy, E.Z + 2000.f));
		ShieldSections[i]->SetRelativeScale3D(FVector(15.f, 15.f, 40.f));
	}
	HangarPoint->SetRelativeLocation(FVector(E.X + 3000.f, 0.f, -E.Z * 0.4f));

	// Black armour with red trim, glowing shield emitters (Null Armada colour code).
	UTDStatics::TintPlaceholder(HullMesh, FLinearColor(0.05f, 0.04f, 0.045f), 0.f);
	UTDStatics::TintPlaceholder(BridgeMesh, FLinearColor(0.35f, 0.04f, 0.03f), 0.f);
	UTDStatics::TintPlaceholder(TrenchWallLeft, FLinearColor(0.12f, 0.03f, 0.03f), 0.f);
	UTDStatics::TintPlaceholder(TrenchWallRight, FLinearColor(0.12f, 0.03f, 0.03f), 0.f);
	for (UStaticMeshComponent* S : ShieldSections)
	{
		UTDStatics::TintPlaceholder(S, FLinearColor(0.2f, 0.7f, 1.f), 6.f);
	}
	if (Visuals && !Visuals->HullMesh.IsNull())
	{
		if (UStaticMesh* Custom = Visuals->HullMesh.LoadSynchronous())
		{
			HullMesh->SetStaticMesh(Custom);
			HullMesh->SetRelativeScale3D(FVector::OneVector);
			BridgeMesh->SetVisibility(false);
		}
	}

	// Art rules: a real hull mesh hides the placeholder bridge and trench-wall visuals (their collision
	// stays: the trench is gameplay cover); a ShieldIdle effect replaces the emitter towers.
	const bool bArtHull = Visuals && !Visuals->HullMesh.IsNull() && HullMesh->GetStaticMesh() == Visuals->HullMesh.Get();
	BridgeMesh->SetVisibility(!bArtHull);
	TrenchWallLeft->SetVisibility(!bArtHull);
	TrenchWallRight->SetVisibility(!bArtHull);
	const bool bArtShield = Visuals && !Visuals->ShieldIdle.VFX.IsNull();
	for (UStaticMeshComponent* S : ShieldSections)
	{
		if (S && bArtShield) { S->SetVisibility(false); }
	}
}

void ATDDominion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnTakeStarted.RemoveDynamic(this, &ATDDominion::HandleTakeStarted);
	}
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->RemoveHyperdriveLock(TDHyperdriveLocks::Finale);
	}
	for (ATDShieldGenerator* G : Generators)
	{
		if (IsValid(G)) { G->OnGeneratorDestroyed.RemoveDynamic(this, &ATDDominion::HandleGeneratorDestroyed); }
	}
	if (IsValid(Reactor))
	{
		Reactor->OnPhaseChanged.RemoveDynamic(this, &ATDDominion::HandleReactorPhase);
		Reactor->OnReactorDestroyed.RemoveDynamic(this, &ATDDominion::HandleReactorDestroyed);
	}
	Super::EndPlay(EndPlayReason);
}

void ATDDominion::SetState(ETDDominionState NewState)
{
	StateTime = 0.f;
	if (State != NewState)
	{
		State = NewState;
		SyncHyperdriveLock();
		OnStateChanged.Broadcast(this, NewState);
	}
}

void ATDDominion::SyncHyperdriveLock()
{
	// The assault (warp-in to victory) locks the hyperdrive: the finale is fought, not skipped. The lock
	// follows the STATE, not its transitions: it is re-asserted on every state change, on every retake and a
	// few times a second, so nothing that clears locks (a retake, a level arrival, a resume) can open it.
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		const bool bLocked = State == ETDDominionState::WarpingIn || State == ETDDominionState::ShieldsUp
			|| State == ETDDominionState::ReactorExposed || State == ETDDominionState::Breakup;
		if (bLocked) { Travel->AddHyperdriveLock(TDHyperdriveLocks::Finale, NSLOCTEXT("TD", "HyperLockFinale", "HYPERDRIVE LOCKED - DOMINION ASSAULT"), this); }
		else { Travel->RemoveHyperdriveLock(TDHyperdriveLocks::Finale); }
	}
}

void ATDDominion::HandleTakeStarted(FName /*SceneId*/, int32 /*TakeNumber*/)
{
	SyncHyperdriveLock();
}

void ATDDominion::ShowHull(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);
}

// ------------------------------------------------------------------------------------------ arrival

void ATDDominion::SyncWithProgress()
{
	ITDGameFlow* Flow = UTDStatics::GetGameFlow(this);
	if (!Flow || State != ETDDominionState::Hidden)
	{
		return;
	}
	const FTDProgress P = Flow->GetProgress();
	if (!P.bDominionRevealed)
	{
		return;
	}
	if (P.bDominionDefensesDown)
	{
		// Resume after a retake / load: defenses already broken, go straight to the reactor.
		ShowHull(true);
		BuildParts();
		for (ATDShieldGenerator* G : Generators)
		{
			if (G) { G->SetDestroyedSilently(); }
		}
		for (UStaticMeshComponent* S : ShieldSections) { S->SetVisibility(false); }
		OnDefensesDown(/*bFromSave*/ true);
		return;
	}
	WarpIn();
}

void ATDDominion::WarpIn()
{
	if (State != ETDDominionState::Hidden)
	{
		return;
	}
	ShowHull(true);
	if (Visuals)
	{
		UTDStatics::PlayCue(this, Visuals->WarpIn, GetActorLocation(), GetActorRotation());
	}
	SetState(ETDDominionState::WarpingIn);
	if (WarpInSeconds <= 0.f)
	{
		BuildParts();
		SetState(ETDDominionState::ShieldsUp);
		StartShieldIdle();
		ApplyStage(0);
	}
}

void ATDDominion::BuildParts()
{
	UWorld* World = GetWorld();
	if (!World || bPartsBuilt)
	{
		return;
	}
	bPartsBuilt = true;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Snap, then set relative offsets: KeepRelative on a freshly spawned actor would apply our rotation twice.
	const FAttachmentTransformRules Attach(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true);
	const FVector E = HullExtent;

	auto SpawnTurret = [&](const FVector& Local, TArray<TObjectPtr<ATDEnemyTurret>>& Into)
	{
		ATDEnemyTurret* T = World->SpawnActor<ATDEnemyTurret>(ATDEnemyTurret::StaticClass(), FTransform::Identity, Params);
		if (!T) { return; }
		T->AttachToActor(this, Attach);
		T->SetActorRelativeLocation(Local);
		const FTDDifficultyScale Difficulty = UTDDifficultySettings::GetScale();
		T->Init(TurretGuns, TurretHull * Difficulty.EnemyHull);
		T->SetDamageMultiplier(Difficulty.EnemyDamage);
		T->Visuals = TurretVisuals;
		T->SetActorScale3D(FVector(4.f));
		Into.Add(T);
	};

	// Generators on pylons just outside the main shield, one per quadrant, each with its guards.
	const float Ring = ShieldRadius * 1.08f;
	for (int32 i = 0; i < 4; ++i)
	{
		const float Angle = PI * 0.25f + PI * 0.5f * i;
		const FVector Local(FMath::Cos(Angle) * Ring, FMath::Sin(Angle) * Ring, 0.f);
		ATDShieldGenerator* G = World->SpawnActorDeferred<ATDShieldGenerator>(ATDShieldGenerator::StaticClass(), FTransform::Identity, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!G) { continue; }
		G->Visuals = Visuals;	// before BeginPlay
		G->FinishSpawning(FTransform::Identity);
		G->AttachToActor(this, Attach);
		G->SetActorRelativeLocation(Local);
		G->Init(GeneratorHull * UTDDifficultySettings::GetScale().EnemyHull, i);
		G->OnGeneratorDestroyed.AddUniqueDynamic(this, &ATDDominion::HandleGeneratorDestroyed);
		Generators.Add(G);

		const FVector Tangent(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		for (int32 k = 0; k < TurretsPerGenerator; ++k)
		{
			const float Side = (k % 2 ? 1.f : -1.f) * (1 + k / 2) * 3000.f;
			SpawnTurret(Local + Tangent * Side + FVector(0, 0, 1500.f), Turrets);
		}
	}

	// Hull turrets.
	for (int32 i = 0; i < ExtraTurrets; ++i)
	{
		const float X = FMath::Lerp(-0.6f, 0.6f, ExtraTurrets > 1 ? float(i) / (ExtraTurrets - 1) : 0.5f) * E.X;
		SpawnTurret(FVector(X, (i % 2 ? 1.f : -1.f) * E.Y, E.Z + 500.f), Turrets);
	}

	// Reactor at the bottom of the trench, plus dormant trench turrets.
	const float TrenchZ = E.Z + 1500.f;
	Reactor = World->SpawnActorDeferred<ATDDominionReactor>(ATDDominionReactor::StaticClass(), FTransform::Identity, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Reactor)
	{
		Reactor->Visuals = Visuals;	// read in the reactor's BeginPlay, so set it before FinishSpawning
		Reactor->FinishSpawning(FTransform::Identity);
		Reactor->AttachToActor(this, Attach);
		Reactor->SetActorRelativeLocation(FVector(E.X * 0.55f, 0.f, TrenchZ));	// sits clear of the hull top and the trench walls
		Reactor->SetExposed(false);
		Reactor->OnPhaseChanged.AddUniqueDynamic(this, &ATDDominion::HandleReactorPhase);
		Reactor->OnReactorDestroyed.AddUniqueDynamic(this, &ATDDominion::HandleReactorDestroyed);
	}
	for (int32 i = 0; i < TrenchTurrets; ++i)
	{
		const float X = FMath::Lerp(-0.5f, 0.4f, TrenchTurrets > 1 ? float(i) / (TrenchTurrets - 1) : 0.5f) * E.X;
		SpawnTurret(FVector(X, (i % 2 ? 1.f : -1.f) * 1700.f, TrenchZ + 1200.f), TrenchTurretActors);	// inside the walls (|y| < 2300)
	}
	for (ATDEnemyTurret* T : TrenchTurretActors)
	{
		T->SetActorTickEnabled(false);
		T->SetActorHiddenInGame(true);
		if (UTDHealthComponent* H = T->GetHealth()) { H->SetTargetable(false); H->SetInvulnerable(true); }
	}
}

// ------------------------------------------------------------------------------------------- stages

void ATDDominion::StartShieldIdle()
{
	if (Visuals && !ShieldIdleFX)
	{
		ShieldIdleFX = UTDStatics::PlayCueAttached(Visuals->ShieldIdle, Root);
	}
}

void ATDDominion::StopShieldIdle()
{
	if (ShieldIdleFX)
	{
		ShieldIdleFX->Deactivate();
		ShieldIdleFX = nullptr;
	}
}

void ATDDominion::ApplyStage(int32 NewStage)
{
	if (Stages.Num() == 0)
	{
		return;
	}
	Stage = FMath::Clamp(NewStage, 0, Stages.Num() - 1);
	const FTDDominionStage& S = Stages[Stage];
	auto Tune = [&S](ATDEnemyTurret* T)
	{
		if (!T) { return; }
		T->BurstCooldownSeconds = S.TurretBurstCooldown;
		T->Guns.TelegraphSeconds = S.TurretTelegraph;
	};
	for (ATDEnemyTurret* T : Turrets) { Tune(T); }
	for (ATDEnemyTurret* T : TrenchTurretActors) { Tune(T); }
	if (S.LaunchInterval > 0.f)
	{
		LaunchTimer = LaunchTimer > 0.f ? FMath::Min(LaunchTimer, S.LaunchInterval) : S.LaunchInterval * 0.5f;
	}
}

void ATDDominion::TickLaunches(float DeltaSeconds)
{
	if (!Stages.IsValidIndex(Stage) || Stages[Stage].LaunchInterval <= 0.f || LaunchGroups.Num() == 0)
	{
		return;
	}
	LaunchTimer -= DeltaSeconds;
	if (LaunchTimer > 0.f)
	{
		return;
	}
	LaunchTimer += Stages[Stage].LaunchInterval;
	if (LaunchTimer <= 0.f) { LaunchTimer = Stages[Stage].LaunchInterval; }
	if (GetLaunchedAlive() < MaxLaunchedAlive || MaxLaunchedAlive <= 0)
	{
		LaunchGroup();
	}
}

void ATDDominion::LaunchGroup()
{
	UWorld* World = GetWorld();
	if (!World || LaunchGroups.Num() == 0)
	{
		return;
	}
	const FTDWaveGroup& G = LaunchGroups[LaunchCursor++ % LaunchGroups.Num()];
	UClass* Class = LaunchedShipClass ? LaunchedShipClass.Get() : ATDEnemyShip::StaticClass();
	const FVector Origin = HangarPoint->GetComponentLocation();
	const FRotator Facing = GetActorRotation();
	const FVector Right = GetActorRightVector();
	const FVector Back = -GetActorForwardVector();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 PackIdx = 0; PackIdx < G.Packs; ++PackIdx)
	{
		ATDEnemyShip* Leader = nullptr;
		for (int32 Slot = 0; Slot < G.PerPack; ++Slot)
		{
			if (MaxLaunchedAlive > 0 && GetLaunchedAlive() >= MaxLaunchedAlive)
			{
				break;
			}
			const int32 Row = (Slot + 1) / 2;
			const float Side = Slot == 0 ? 0.f : (Slot % 2 ? -1.f : 1.f);
			const FVector Loc = Origin + Right * (PackIdx * 4000.f + Side * Row * 900.f) + Back * Row * 900.f;
			ATDEnemyShip* Ship = World->SpawnActor<ATDEnemyShip>(Class, Loc, Facing, Params);
			if (!Ship) { continue; }
			Ship->InitFromSpec(G.EnemyId);
			if (Leader)
			{
				Ship->JoinFormation(Leader, FVector(-Row * 900.f, Side * Row * 900.f, 0.f));
			}
			else
			{
				Leader = Ship;
			}
			Launched.Add(Ship);
		}
	}
	++LaunchCount;
}

int32 ATDDominion::GetLaunchedAlive() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ATDEnemyShip>& S : Launched)
	{
		if (S.IsValid() && !S->IsDying()) { ++N; }
	}
	return N;
}

// --------------------------------------------------------------------------------------- generators

int32 ATDDominion::GetGeneratorsRemaining() const
{
	int32 N = 0;
	for (const ATDShieldGenerator* G : Generators)
	{
		if (G && G->IsAlive()) { ++N; }
	}
	return N;
}

TArray<ATDShieldGenerator*> ATDDominion::GetGenerators() const
{
	TArray<ATDShieldGenerator*> Out;
	for (ATDShieldGenerator* G : Generators) { if (G) { Out.Add(G); } }
	return Out;
}

TArray<ATDEnemyTurret*> ATDDominion::GetTurrets() const
{
	TArray<ATDEnemyTurret*> Out;
	for (ATDEnemyTurret* T : Turrets) { if (T) { Out.Add(T); } }
	return Out;
}

void ATDDominion::HandleGeneratorDestroyed(ATDShieldGenerator* Generator, AActor* /*Killer*/)
{
	if (State != ETDDominionState::ShieldsUp)
	{
		return;
	}
	if (Generator && ShieldSections.IsValidIndex(Generator->GetSectionIndex()))
	{
		ShieldSections[Generator->GetSectionIndex()]->SetVisibility(false);
	}
	if (Visuals && Generator)
	{
		UTDStatics::PlayCue(this, Visuals->ShieldSectionDown, Generator->GetActorLocation());
	}
	// The GeneratorKill money shot comes from T3's UTDDirectorSubsystem (generator actors are tagged).

	const int32 Remaining = GetGeneratorsRemaining();
	OnShieldSectionDown.Broadcast(this, Remaining);
	if (Remaining == 0)
	{
		OnDefensesDown(/*bFromSave*/ false);
	}
}

void ATDDominion::DebugDropShields()
{
	for (ATDShieldGenerator* G : Generators)
	{
		if (G && G->IsAlive() && G->GetHealth())
		{
			G->GetHealth()->Kill();
		}
	}
}

void ATDDominion::OnDefensesDown(bool bFromSave)
{
	if (!bFromSave)
	{
		if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
		{
			Flow->SetDominionState(true, true);
		}
	}
	StopShieldIdle();
	if (Visuals && !bFromSave)
	{
		UTDStatics::PlayCue(this, Visuals->ShieldCollapse, GetActorLocation(), GetActorRotation());
	}
	if (bDockingFinale)
	{
		BeginDockingFinale();
		return;
	}
	ExposeReactor(/*bPlayCues*/ !bFromSave);
}

void ATDDominion::BeginDockingFinale()
{
	// ENG-29 (stretch) overrides this to open the docking bay and hand off to the boarding flow.
	UE_LOG(LogTriadDrift, Warning, TEXT("ATDDominion: docking finale is not built (ENG-29); using the reactor ending."));
	ExposeReactor(true);
}

void ATDDominion::ExposeReactor(bool bPlayCues)
{
	if (Reactor)
	{
		Reactor->SetExposed(true, bPlayCues);
	}
	for (ATDEnemyTurret* T : TrenchTurretActors)
	{
		if (!T) { continue; }
		T->SetActorTickEnabled(true);
		T->SetActorHiddenInGame(false);
		if (UTDHealthComponent* H = T->GetHealth()) { H->SetInvulnerable(false); H->SetTargetable(true); }
	}
	SetState(ETDDominionState::ReactorExposed);
	ApplyStage(1);
}

// ------------------------------------------------------------------------------------------ reactor

void ATDDominion::HandleReactorPhase(ATDDominionReactor* /*InReactor*/, int32 NewPhase)
{
	ApplyStage(NewPhase);
	OnReactorPhaseChanged.Broadcast(this, NewPhase);
}

void ATDDominion::HandleReactorDestroyed(ATDDominionReactor* InReactor, AActor* /*Killer*/)
{
	if (State == ETDDominionState::Breakup || State == ETDDominionState::Finished)
	{
		return;
	}
	// ReactorKill money shot: UTDDirectorSubsystem (the reactor actor is tagged).
	// The war is won: nothing may take it away during the breakup. Protect the squad, end every enemy.
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Actors;
		Registry->GetActorsOfTeam(ETDTeam::Player, Actors, true);
		for (AActor* A : Actors)
		{
			if (UTDHealthComponent* H = UTDStatics::GetHealth(A)) { H->SetInvulnerable(true); }
		}
		Actors.Reset();
		Registry->GetActorsOfTeam(ETDTeam::Enemy, Actors, true);
		for (AActor* A : Actors)
		{
			if (ATDEnemyShip* Ship = Cast<ATDEnemyShip>(A))
			{
				if (UTDHealthComponent* H = Ship->GetHealth()) { H->Kill(nullptr, ETDDamageKind::Environmental); }
			}
		}
	}
	if (UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this))
	{
		Projectiles->KillAll(true);
	}

	// Defenses go quiet while the ship breaks apart.
	for (ATDEnemyTurret* T : Turrets) { if (T) { T->ApplyStun(BreakupSeconds + 60.f); } }
	for (ATDEnemyTurret* T : TrenchTurretActors) { if (T) { T->ApplyStun(BreakupSeconds + 60.f); } }

	SetState(ETDDominionState::Breakup);
	if (Visuals)
	{
		UTDStatics::PlayCue(this, Visuals->BreakupSequence, GetActorLocation(), GetActorRotation());
	}
	OnBreakupStarted.Broadcast(this, State);
}

// --------------------------------------------------------------------------------------------- tick

void ATDDominion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StateTime += DeltaSeconds;

	LockSyncTimer -= DeltaSeconds;
	if (LockSyncTimer <= 0.f)
	{
		LockSyncTimer = 0.25f;
		SyncHyperdriveLock();
	}

	switch (State)
	{
	case ETDDominionState::Hidden:
		PollTimer -= DeltaSeconds;
		if (PollTimer <= 0.f)
		{
			PollTimer = ProgressPollSeconds;
			SyncWithProgress();
		}
		break;

	case ETDDominionState::WarpingIn:
		if (StateTime >= WarpInSeconds)
		{
			BuildParts();
			SetState(ETDDominionState::ShieldsUp);
			StartShieldIdle();
			ApplyStage(0);
		}
		break;

	case ETDDominionState::ShieldsUp:
	case ETDDominionState::ReactorExposed:
		Launched.RemoveAll([](const TWeakObjectPtr<ATDEnemyShip>& S) { return !S.IsValid(); });
		TickLaunches(DeltaSeconds);
		break;

	case ETDDominionState::Breakup:
		if (StateTime >= BreakupSeconds)
		{
			SetState(ETDDominionState::Finished);
			// Victory is a phase on L_Galaxy (no map): stops the run clock, saves, and drives Nick's
			// victory screen through OnPhaseChanged. Exactly once.
			if (!bVictoryAccepted)
			{
				bVictoryAccepted = true;
				if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
				{
					Flow->SetPhase(ETDGamePhase::Victory);
				}
			}
		}
		break;

	default:
		break;
	}
}

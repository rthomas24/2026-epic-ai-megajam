// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDDominion.h"

#include "Misc/PackageName.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDDockingBay.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Profile/TDProfileSubsystem.h"
#include "TriadDrift/World/TDWorldBarks.h"
#include "TriadDrift/World/TDDistortionField.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/World/TDEscapeDebris.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
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
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"

#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTDFinaleBoarding(
	TEXT("td.Finale.Boarding"), 1,
	TEXT("ENG-29 finale: 1 = board the Dominion through its docking bay when the generators fall (needs L_Dominion_Interior); ")
	TEXT("0 = the space reactor kill (the fallback ending). Read when the defenses go down."));

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
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> DetailSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		auto MakeDetail = [this](const TCHAR* Name, UStaticMesh* Mesh)
		{
			UInstancedStaticMeshComponent* D = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
			D->SetupAttachment(Root);
			D->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			D->SetGenerateOverlapEvents(false);
			D->SetCastShadow(false);
			D->SetStaticMesh(Mesh);
			return D;
		};
		HullSeams = MakeDetail(TEXT("HullSeams"), CubeMesh);
		HullEyes = MakeDetail(TEXT("HullEyes"), DetailSphere.Succeeded() ? DetailSphere.Object : nullptr);
	}
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
	// The trench run is a 25-45 s fight for one flown ship (ENG-47 balance, Nov 14: the old 2.0 / 1.5 / 1.0 s bursts and
	// 12 / 9 / 6 s launches killed a ship parked at the trench start in ~5 s). Still escalating, never a wall of fire.
	Stages.Add(TDDominion_MakeStage(20.f, 3.5f, 1.5f));	// reactor phase 1
	Stages.Add(TDDominion_MakeStage(16.f, 3.0f, 1.3f));	// phase 2
	Stages.Add(TDDominion_MakeStage(12.f, 2.5f, 1.1f));	// phase 3

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
		Bus->OnActorKilled.AddUniqueDynamic(this, &ATDDominion::HandleActorKilled);
		Bus->OnHyperdriveState.AddUniqueDynamic(this, &ATDDominion::HandleHyperdriveForLightRow);
	}
	if (UTDProfileSubsystem* Profile = UTDProfileSubsystem::Get(this))
	{
		Profile->OnAftermathBegins.AddUObject(this, &ATDDominion::HandleAftermathBegins);
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

void ATDDominion::BuildHullDetail(bool bShow)
{
	if (!HullSeams || !HullEyes) { return; }
	HullSeams->ClearInstances();
	HullEyes->ClearInstances();
	HullSeams->SetVisibility(bShow);
	HullEyes->SetVisibility(bShow);
	if (!bShow) { return; }
	const FVector E = HullExtent;
	const float Line = 60.f;	// seam thickness (cm)
	// Long seams down both flanks at two heights, and across the top every 6000 cm: a panel grid.
	for (const float Side : { -1.f, 1.f })
	{
		for (const float Zf : { -0.35f, 0.35f })
		{
			HullSeams->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.f, Side * (E.Y + Line * 0.5f), E.Z * Zf),
				FVector(E.X * 2.f, Line, Line) / 100.f));
		}
	}
	for (float X = -E.X + 3000.f; X < E.X; X += 6000.f)
	{
		HullSeams->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, 0.f, E.Z + Line * 0.5f), FVector(Line, E.Y * 2.f, Line) / 100.f));
		for (const float Side : { -1.f, 1.f })
		{
			HullSeams->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Side * (E.Y + Line * 0.5f), 0.f), FVector(Line, Line, E.Z * 2.f) / 100.f));
		}
	}
	// White eye lights along both flanks, a row just under the top edge.
	for (float X = -E.X + 2500.f; X < E.X; X += 5000.f)
	{
		for (const float Side : { -1.f, 1.f })
		{
			HullEyes->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Side * (E.Y + 150.f), E.Z * 0.6f), FVector(3.f)));
		}
	}
	UTDStatics::TintPlaceholder(HullSeams, FLinearColor(1.f, 0.3f, 0.05f), 4.f);
	UTDStatics::TintPlaceholder(HullEyes, FLinearColor::White, 12.f);
}

bool ATDDominion::SetShowcase(bool bShow, const FTransform* Override)
{
	if (!bShow)
	{
		if (!bShowcase) { return true; }
		for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, bool>& P : ShowcaseComponentVisibility)
		{
			if (P.Key.IsValid()) { P.Key->SetVisibility(P.Value); }
		}
		for (const TPair<TWeakObjectPtr<AActor>, bool>& P : ShowcaseActorHidden)
		{
			if (P.Key.IsValid()) { P.Key->SetActorHiddenInGame(P.Value); }
		}
		ShowcaseComponentVisibility.Reset();
		ShowcaseActorHidden.Reset();
		bShowcase = false;
		SetActorTransform(ShowcasePrevTransform, false, nullptr, ETeleportType::TeleportPhysics);
		SetActorHiddenInGame(bShowcasePrevHidden);
		SetActorEnableCollision(bShowcasePrevCollision);
		return true;
	}
	if (State != ETDDominionState::Hidden && State != ETDDominionState::Finished)
	{
		return false;	// the assault owns the ship
	}
	if (!bShowcase)
	{
		bShowcase = true;
		ShowcasePrevTransform = GetActorTransform();
		bShowcasePrevHidden = IsHidden();
		bShowcasePrevCollision = GetActorEnableCollision();
		// Only the silhouette: everything else on the actor and every attached part actor goes dark.
		const TSet<const UPrimitiveComponent*> Keep = { HullMesh.Get(), BridgeMesh.Get(), TrenchWallLeft.Get(), TrenchWallRight.Get(), HullSeams.Get(), HullEyes.Get() };
		TInlineComponentArray<UPrimitiveComponent*> Prims(this);
		for (UPrimitiveComponent* P : Prims)
		{
			if (P && !Keep.Contains(P))
			{
				ShowcaseComponentVisibility.Emplace(P, P->GetVisibleFlag());
				P->SetVisibility(false);
			}
		}
		TArray<AActor*> Attached;
		GetAttachedActors(Attached, true, true);
		for (AActor* A : Attached)
		{
			if (A)
			{
				ShowcaseActorHidden.Emplace(A, A->IsHidden());
				A->SetActorHiddenInGame(true);
			}
		}
		if (DistortionField)
		{
			// T3's shots show the real hull: the far-off mirage and its field step aside until the showcase ends.
			ShowcaseActorHidden.Emplace(DistortionField.Get(), DistortionField->IsHidden());
			DistortionField->SetActorHiddenInGame(true);
		}
	}
	if (Override)
	{
		SetActorTransform(*Override, false, nullptr, ETeleportType::TeleportPhysics);
	}
	SetActorEnableCollision(false);
	SetActorHiddenInGame(false);
	return true;
}

FBox ATDDominion::GetLocalHullBounds() const
{
	FBox Box(ForceInit);
	const FTransform Xf = GetActorTransform();
	for (const UStaticMeshComponent* C : { HullMesh.Get(), BridgeMesh.Get(), TrenchWallLeft.Get(), TrenchWallRight.Get() })
	{
		if (!C || !C->GetStaticMesh() || (C != HullMesh.Get() && !C->GetVisibleFlag())) { continue; }	// hidden stand-ins are not the hull
		// The mesh's own box in the Dominion's frame (world-axis bounds would grow with the Dominion's yaw).
		Box += C->GetStaticMesh()->GetBoundingBox().TransformBy(C->GetComponentTransform().GetRelativeTransform(Xf));
	}
	if (!Box.IsValid) { Box = FBox(-HullExtent, HullExtent); }
	return Box;
}

FTransform ATDDominion::GetStageStart(bool bTrench) const
{
	const FTransform D = GetActorTransform();
	const FBox B = GetLocalHullBounds();
	if (bTrench)
	{
		// Off the real bow on the trench line, facing aft: the reactor is straight ahead.
		const FVector Loc = D.TransformPosition(FVector(B.Max.X + 20000.f, 0.f, HullExtent.Z + 1500.f));
		return FTransform((-D.GetUnitAxis(EAxis::X)).Rotation(), Loc);
	}
	// Front-right and a little above, 2.5 hull radii out: the hull fills a good part of the frame and there are ~7 s
	// at cruise before anything solid (the old 2.4 x HullExtent.X put the r8 art hull ~5 s away).
	// The larger horizontal half-size, not the box diagonal: hull r2's outriggers (|Y| 302 m) and its 200 m mast made
	// the diagonal put the start beyond 3 shield radii, the hull a sliver in the frame.
	const FVector BE = B.GetExtent();
	const float Radius = FMath::Max(FMath::Max(BE.X, BE.Y), HullExtent.Size());
	const float Dist = FMath::Max(ShieldRadius * 2.f, Radius * 2.5f);
	const FVector Loc = D.TransformPosition(FVector(0.75f, 0.6f, 0.25f).GetSafeNormal() * Dist);
	return FTransform((D.GetLocation() - Loc).Rotation(), Loc);
}

void ATDDominion::TickPlaceholderBreakup(float DeltaSeconds)
{
	const float FinalAt = FMath::Max(1.f, BreakupSeconds - 1.5f);
	if (!bBreakupFinalBlast)
	{
		// Explosions walk from the reactor outward, faster and bigger as the breakup builds.
		BreakupBoomTimer -= DeltaSeconds;
		if (BreakupBoomTimer <= 0.f && StateTime < FinalAt)
		{
			const float T = FMath::Clamp(StateTime / FinalAt, 0.f, 1.f);
			BreakupBoomTimer = FMath::Lerp(0.45f, 0.12f, T);
			PlayHullSectionBurst(T, 7919 + BreakupBooms * 31);
			++BreakupBooms;
		}
		if (StateTime >= FinalAt)
		{
			PlayFinalBlast();
		}
	}
}

void ATDDominion::PlayHullSectionBurst(float T01, int32 Seed)
{
	const FBox B = GetLocalHullBounds();
	const FTransform Xf = GetActorTransform();
	FRandomStream R(Seed);
	const FVector Local(R.FRandRange(B.Min.X, B.Max.X), R.FRandRange(B.Min.Y, B.Max.Y) * 0.8f, R.FRandRange(B.Min.Z, B.Max.Z) * 0.8f);
	const FLinearColor Col = R.FRand() < 0.5f ? FLinearColor(1.f, 0.45f, 0.1f) : FLinearColor(1.f, 0.8f, 0.4f);
	const float Radius = FMath::Lerp(2500.f, 7000.f, FMath::Clamp(T01, 0.f, 1.f)) * R.FRandRange(0.7f, 1.2f);
	const FVector Where = Xf.TransformPosition(Local);
	// ENG-37: the section explosion when it has VFX (authored for a 50 m radius), else the placeholder burst.
	if (Visuals && FTDCombatPlaceholders::HasVisualArt(Visuals->HullSectionExplosion))
	{
		FTDFXCue Cue = Visuals->HullSectionExplosion;
		Cue.VFXScale *= Radius / 5000.f;
		UTDStatics::PlayCue(this, Cue, Where, FRotator(R.FRandRange(-180.f, 180.f), R.FRandRange(-180.f, 180.f), 0.f));
	}
	else if (UTDProjectileSubsystem* Booms = UTDProjectileSubsystem::Get(this))
	{
		Booms->PlayPlaceholderExplosion(Where, Radius, Col, 10);
	}
}

void ATDDominion::PlayFinalBlast()
{
	bBreakupFinalBlast = true;
	const FBox B = GetLocalHullBounds();
	const float Radius = B.GetExtent().Size() * 1.2f;
	const FVector Where = GetActorTransform().TransformPosition(B.GetCenter());
	if (Visuals && FTDCombatPlaceholders::HasVisualArt(Visuals->BreakupFinalBlast))
	{
		FTDFXCue Cue = Visuals->BreakupFinalBlast;
		Cue.VFXScale *= Radius / 50000.f;	// authored for a 500 m radius
		UTDStatics::PlayCue(this, Cue, Where, GetActorRotation());
	}
	else if (UTDProjectileSubsystem* Booms = UTDProjectileSubsystem::Get(this))
	{
		Booms->PlayPlaceholderExplosion(Where, Radius, FLinearColor(1.f, 0.85f, 0.6f), 16);
	}
	if (Reactor) { Reactor->SetActorHiddenInGame(true); }
	if (DockingBay) { DockingBay->SetActorHiddenInGame(true); }
	if (DockingInterior) { DockingInterior->SetActorHiddenInGame(true); DockingInterior->SetActorEnableCollision(false); }
	ShowHull(false);	// the ship is gone in the flash
}

FTransform ATDDominion::GetEscapeStart() const
{
	// Out of the bay mouth (where ATDDockingBay sits), 40 m clear of the bow, facing out.
	const FBox B = GetLocalHullBounds();
	const FTransform D = GetActorTransform();
	const FVector Loc = D.TransformPosition(FVector(B.Max.X + 4000.f, 0.f, FMath::Lerp(B.Min.Z, B.GetCenter().Z, 0.6f)));
	return FTransform(D.GetUnitAxis(EAxis::X).Rotation(), Loc);
}

float ATDDominion::GetEscapeExitRadius() const
{
	return FMath::Max3((float)GetLocalHullBounds().GetExtent().Size() * EscapeExitHullRadii, ShieldRadius * 2.f, EscapeExitDistance);
}

// ------------------------------------------------------------------------------------------ escape flight

void ATDDominion::BeginEscapeFlight()
{
	// Back from the interior: the reactor is dead inside, the ship is coming apart. No defences, no launches.
	ShowHull(true);
	BuildParts();
	for (ATDShieldGenerator* G : Generators) { if (G) { G->SetDestroyedSilently(); } }
	for (UStaticMeshComponent* S : ShieldSections) { S->SetVisibility(false); }
	StopShieldIdle();
	auto Quiet = [](AActor* A)
	{
		if (!A) { return; }
		A->SetActorTickEnabled(false);
		if (UTDHealthComponent* H = UTDStatics::GetHealth(A)) { H->SetTargetable(false); H->SetInvulnerable(true); }
	};
	for (ATDEnemyTurret* T : Turrets) { Quiet(T); }
	for (ATDEnemyTurret* T : TrenchTurretActors) { Quiet(T); if (T) { T->SetActorHiddenInGame(true); } }
	if (Reactor) { Quiet(Reactor); Reactor->SetActorHiddenInGame(true); }
	SetState(ETDDominionState::EscapeFlight);
	ResetEscapeTake();
	// The escape is its own scene, with a checkpoint at the mouth: a fail retakes the flight, never the interior.
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->BeginScene(EscapeSceneId);
		GI->SaveCheckpoint();
	}
	// A5_Escape ("Ninety seconds. Move!") is the ON-FOOT run's line and clock (storyline 29, T5's 90 s): the flight is
	// its own beat with its own clock, so it never repeats that number (T7's playtest: "Ninety seconds" over a 60 s flight).
	UTDStorySubsystem::Fire(this, TEXT("G9_Wren_PullAway"));	// "Triad, pull away. Follow the clear vector."
	UE_LOG(LogTriadDrift, Log, TEXT("ATDDominion: escape flight - %.0f s to get %.0f m clear"), EscapeSeconds, GetEscapeExitRadius() / 100.f);
}

void ATDDominion::ResetEscapeTake()
{
	StateTime = 0.f;
	bEscapeBarkSaid = false;
	EscapeBoomTimer = 0.f;
	EscapeDebrisTimer = 1.5f;	// a breath before the first chunk
	EscapeClearTime = 0.f;
	bEscapeClear = false;
	bEscapeFailReported = false;
	bBreakupFinalBlast = false;
	for (AActor* A : EscapeDebris) { if (IsValid(A)) { A->Destroy(); } }
	EscapeDebris.Reset();
	ShowHull(true);
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Squad;
		Registry->GetActorsOfTeam(ETDTeam::Player, Squad, true);
		for (AActor* A : Squad) { if (UTDHealthComponent* H = UTDStatics::GetHealth(A)) { H->SetInvulnerable(false); } }
	}
}

void ATDDominion::SpawnEscapeDebris()
{
	UWorld* World = GetWorld();
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	const AActor* Ship = Squad ? Squad->GetActiveShip() : nullptr;
	if (!World || !Ship)
	{
		return;
	}
	// Ahead of the ship, off to one side, drifting across its path and falling away from the hull.
	FRandomStream R(104729 + EscapeDebrisSpawned * 7);
	const FVector Fwd = Ship->GetVelocity().SizeSquared() > 1.f ? Ship->GetVelocity().GetSafeNormal() : Ship->GetActorForwardVector();
	const FVector Side = FVector::CrossProduct(Fwd, FVector::UpVector).GetSafeNormal();
	const float SideSign = R.FRand() < 0.5f ? -1.f : 1.f;
	const FVector From = Ship->GetActorLocation() + Fwd * R.FRandRange(25000.f, 45000.f) + Side * SideSign * R.FRandRange(6000.f, 14000.f)
		+ FVector::UpVector * R.FRandRange(-3000.f, 6000.f);
	const FVector Away = (From - GetActorLocation()).GetSafeNormal();
	const FVector Vel = -Side * SideSign * R.FRandRange(2500.f, 4500.f) + Away * 1500.f - FVector::UpVector * R.FRandRange(500.f, 1500.f);
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATDEscapeDebris* D = World->SpawnActor<ATDEscapeDebris>(ATDEscapeDebris::StaticClass(), From, FRotator(R.FRandRange(-180.f, 180.f), R.FRandRange(-180.f, 180.f), 0.f), P))
	{
		D->Damage = EscapeDebrisDamage;
		D->Launch(R.FRandRange(1200.f, 3500.f), Vel, FRotator(R.FRandRange(-60.f, 60.f), R.FRandRange(-90.f, 90.f), R.FRandRange(-60.f, 60.f)));
		EscapeDebris.Add(D);
		++EscapeDebrisSpawned;
	}
	EscapeDebris.RemoveAll([](const TObjectPtr<AActor>& A) { return !IsValid(A); });
}

void ATDDominion::TickEscapeFlight(float DeltaSeconds)
{
	if (bEscapeClear)
	{
		EscapeClearTime += DeltaSeconds;
		if (EscapeClearTime >= EscapeVictoryDelay && !bVictoryAccepted)
		{
			bVictoryAccepted = true;
			SetState(ETDDominionState::Finished);
			if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this)) { Flow->SetPhase(ETDGamePhase::Victory); }
		}
		return;
	}
	const float T = FMath::Clamp(StateTime / FMath::Max(EscapeSeconds, 1.f), 0.f, 1.f);
	if (!bEscapeBarkSaid && StateTime >= 4.5f)
	{
		// dialogs.md Bark_Dom_EscapeFlight ("Punch it!"), after Wren's G9_Wren_PullAway transmission has finished.
		bEscapeBarkSaid = true;
		FTDWorldBarks::Say(this, TDWorldBarkIds::DomEscapeFlight, ETDBarkPriority::Critical, FTDWorldBarks::EWho::Wingman, 0.f);
	}
	// The hull burns harder as the clock runs down.
	EscapeBoomTimer -= DeltaSeconds;
	if (EscapeBoomTimer <= 0.f)
	{
		EscapeBoomTimer = FMath::Lerp(0.9f, 0.2f, T);
		PlayHullSectionBurst(T, 15485863 + FMath::FloorToInt(StateTime * 37.f));
	}
	if (EscapeDebrisInterval > 0.f)
	{
		EscapeDebrisTimer -= DeltaSeconds;
		if (EscapeDebrisTimer <= 0.f)
		{
			EscapeDebrisTimer = EscapeDebrisInterval * FMath::Lerp(1.f, 0.5f, T);
			SpawnEscapeDebris();
		}
	}
	// Clear: the ship the player flies is far enough out. The Dominion goes up behind the squad.
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	const AActor* Ship = Squad ? Squad->GetActiveShip() : nullptr;
	if (Ship && FVector::Dist(Ship->GetActorLocation(), GetActorLocation()) >= GetEscapeExitRadius())
	{
		bEscapeClear = true;
		UTDStorySubsystem::PlaySequence(this, TEXT("ClearSpace"));	// storyline 31: "Lancer clear / Bastion clear / Rift clear" (replaces Bark_Dom_Clear)
		EscapeClearTime = 0.f;
		for (AActor* A : EscapeDebris) { if (IsValid(A)) { A->Destroy(); } }
		EscapeDebris.Reset();
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			TArray<AActor*> Players;
			Registry->GetActorsOfTeam(ETDTeam::Player, Players, true);
			for (AActor* A : Players) { if (UTDHealthComponent* H = UTDStatics::GetHealth(A)) { H->SetInvulnerable(true); } }
		}
		PlayFinalBlast();
		OnBreakupStarted.Broadcast(this, State);	// Nick's / T3's breakup beat (money shot) as the Dominion goes up
		return;
	}
	if (StateTime >= EscapeSeconds && !bEscapeFailReported)
	{
		// Too slow: the Dominion goes up with the squad aboard. CUT, the next take starts at the mouth.
		bEscapeFailReported = true;
		PlayFinalBlast();
		if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this)) { Flow->ReportPlayerDefeated(); }
	}
}

FVector ATDDominion::GetHangarLocation() const
{
	return HangarPoint ? HangarPoint->GetComponentLocation() : GetActorLocation();
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

	// Dark armour #1a1b1e (linear ~0.010/0.011/0.013) with red-orange seams and white eye lights; the builder's panel
	// material replaces the flat tint when it exists.
	UMaterialInterface* Panel = (!HullMaterial.IsNull() && FPackageName::DoesPackageExist(HullMaterial.ToSoftObjectPath().GetLongPackageName()))
		? HullMaterial.LoadSynchronous() : nullptr;
	if (Panel) { HullMesh->SetMaterial(0, Panel); }
	else { UTDStatics::TintPlaceholder(HullMesh, FLinearColor(0.0103f, 0.0109f, 0.0129f), 0.f); }
	BuildHullDetail(true);
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
			// The art hull keeps its authored slots: the placeholder's panel material / tint on slot 0 would otherwise
			// survive the mesh swap (art audit 5: the stern blocks rendered flat grey with the panel material).
			HullMesh->EmptyOverrideMaterials();
			BridgeMesh->SetVisibility(false);
			BuildHullDetail(false);	// the hull mesh brings its own detail
		}
	}

	// Art rules: a real hull mesh hides the placeholder bridge and trench-wall visuals (their collision
	// stays: the trench is gameplay cover); a ShieldIdle effect replaces the emitter towers.
	const bool bArtHull = Visuals && !Visuals->HullMesh.IsNull() && HullMesh->GetStaticMesh() == Visuals->HullMesh.Get();
	// Per-piece art (optional): each replaces its stand-in at scale 1 and stays visible with an art hull.
	auto UsePiece = [](UStaticMeshComponent* C, const TSoftObjectPtr<UStaticMesh>& Soft, const FVector& Scale) -> bool
	{
		if (!C || Soft.IsNull()) { return false; }
		UStaticMesh* Art = Soft.LoadSynchronous();
		if (!Art) { return false; }
		C->SetStaticMesh(Art);
		C->SetRelativeScale3D(Scale);
		C->EmptyOverrideMaterials();
		C->SetVisibility(true);
		return true;
	};
	const bool bArtBridge = Visuals && UsePiece(BridgeMesh, Visuals->BridgeMesh, FVector::OneVector);
	const bool bArtTrench = Visuals && UsePiece(TrenchWallLeft, Visuals->TrenchMesh, FVector::OneVector)
		&& UsePiece(TrenchWallRight, Visuals->TrenchMesh, FVector(1.f, -1.f, 1.f));	// mirrored
	BridgeMesh->SetVisibility(bArtBridge || !bArtHull);
	TrenchWallLeft->SetVisibility(bArtTrench || !bArtHull);
	TrenchWallRight->SetVisibility(bArtTrench || !bArtHull);
	// An art hull carries its own bridge and trench (r2): a hidden stand-in keeps no collision, or ships hit invisible
	// walls about 2 m inside the real ones (import builder, r2: |Y| 2300-2700 against r2's walls).
	for (UStaticMeshComponent* C : { BridgeMesh.Get(), TrenchWallLeft.Get(), TrenchWallRight.Get() })
	{
		if (C && !C->GetVisibleFlag()) { C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	}
	if (Visuals && !Visuals->ShieldEmitterMesh.IsNull())
	{
		for (UStaticMeshComponent* S : ShieldSections) { UsePiece(S, Visuals->ShieldEmitterMesh, FVector::OneVector); }
	}
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
		Bus->OnActorKilled.RemoveDynamic(this, &ATDDominion::HandleActorKilled);
		Bus->OnHyperdriveState.RemoveDynamic(this, &ATDDominion::HandleHyperdriveForLightRow);
	}
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->RemoveHyperdriveLock(TDHyperdriveLocks::Finale);
	}
	if (IsValid(DistortionField)) { DistortionField->Destroy(); }
	DistortionField = nullptr;
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
	if (NewState == ETDDominionState::ShieldsUp && State != ETDDominionState::ShieldsUp)
	{
		UTDStorySubsystem::Fire(this, TEXT("G6_Kato_MarkingTargets"));	// storyline 24: the generator markers first show
	}
	if (bShowcase && NewState != State)
	{
		SetShowcase(false);	// the real Dominion takes over (reveal, retake, victory)
	}
	StateTime = 0.f;
	if (NewState != ETDDominionState::Hidden && DistortionField)
	{
		DistortionField->Collapse();	// the passage is open (or the run moved past it): the field goes, the real ship is here
		DistortionField = nullptr;
	}
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
	// World review MAJOR 2: only once the squad has arrived (a ship inside HyperdriveLockRadius, the gravity
	// well where a jump drops out). Locking at the warp-in, 500 000 cm away, made the approach a sublight crawl.
	// Sticky for the assault: flying back out of the well doesn't unlock the fight.
	const bool bAssault = State == ETDDominionState::WarpingIn || State == ETDDominionState::ShieldsUp
		|| State == ETDDominionState::ReactorExposed || State == ETDDominionState::DockingOpen || State == ETDDominionState::Breakup
		|| State == ETDDominionState::EscapeFlight;
	if (!bAssault)
	{
		bSquadArrived = false;
	}
	else if (!bSquadArrived)
	{
		if (const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			TArray<AActor*> Squad;
			Registry->GetActorsOfTeam(ETDTeam::Player, Squad, true);
			for (const AActor* A : Squad)
			{
				if (A && A->IsA<ATDSpacecraft>() && FVector::DistSquared(A->GetActorLocation(), GetActorLocation()) <= FMath::Square(HyperdriveLockRadius))
				{
					bSquadArrived = true;
					break;
				}
			}
		}
	}
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		const bool bLocked = bAssault && bSquadArrived;
		if (bLocked) { Travel->AddHyperdriveLock(TDHyperdriveLocks::Finale, NSLOCTEXT("TD", "HyperLockFinale", "HYPERDRIVE LOCKED - DOMINION ASSAULT"), this); }
		else { Travel->RemoveHyperdriveLock(TDHyperdriveLocks::Finale); }
	}
}

void ATDDominion::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	SyncHyperdriveLock();
	if (SceneId == EscapeSceneId && TakeNumber > 1 && State == ETDDominionState::EscapeFlight)
	{
		ResetEscapeTake();	// the squad respawns at the mouth (the checkpoint's return point); the clock restarts
		return;
	}
	if (SceneId != DominionSceneId || TakeNumber <= 1)
	{
		return;
	}
	if (State == ETDDominionState::Breakup || State == ETDDominionState::Finished)
	{
		// Victory wins over a pending CUT: keep the squad protected through the breakup.
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			TArray<AActor*> Squad;
			Registry->GetActorsOfTeam(ETDTeam::Player, Squad, true);
			for (AActor* A : Squad)
			{
				if (UTDHealthComponent* H = UTDStatics::GetHealth(A)) { H->SetInvulnerable(true); }
			}
		}
		return;
	}
	if (State == ETDDominionState::ShieldsUp || State == ETDDominionState::ReactorExposed || State == ETDDominionState::DockingOpen)
	{
		ResetForNewTake();
	}
}

void ATDDominion::ResetForNewTake()
{
	// World review MAJOR 1 (decided): destroyed generators stay destroyed, a reactor phase already passed stays
	// passed, the current reactor phase restarts at full, turrets re-arm and launched fighters are cleared.
	for (const TWeakObjectPtr<ATDEnemyShip>& S : Launched)
	{
		if (S.IsValid()) { S->Destroy(); }
	}
	Launched.Reset();
	// Re-arm: the first launch of the new take comes after one full interval, not on the first frame.
	LaunchTimer = Stages.IsValidIndex(Stage) ? Stages[Stage].LaunchInterval : 0.f;
	for (ATDShieldGenerator* G : Generators)
	{
		if (G && G->IsAlive())
		{
			if (UTDHealthComponent* H = G->FindComponentByClass<UTDHealthComponent>()) { H->RepairHull(1e9f); H->RestoreShield(1e9f); }
		}
	}
	auto Rearm = [](ATDEnemyTurret* T)
	{
		if (!T) { return; }
		if (UTDHealthComponent* H = T->GetHealth())
		{
			if (H->IsAlive()) { H->RepairHull(1e9f); H->RestoreShield(1e9f); }
		}
		T->ClearStun();
	};
	for (ATDEnemyTurret* T : Turrets) { Rearm(T); }
	if (State == ETDDominionState::ReactorExposed)
	{
		for (ATDEnemyTurret* T : TrenchTurretActors) { Rearm(T); }
	}
	if (Reactor) { Reactor->RestartCurrentPhase(); }
	if (DockingBay) { DockingBay->ResetForNewTake(); }	// the tractor lets go; the bay stays open
}

void ATDDominion::FinishAfterVictory()
{
	if (State == ETDDominionState::Finished)
	{
		return;
	}
	// The war is won: no hull, no weapons, no launches, no lock. (Reached by loading a won save or TD.Stage
	// Victory; the real kill goes through Breakup.)
	bVictoryAccepted = true;
	for (const TWeakObjectPtr<ATDEnemyShip>& S : Launched)
	{
		if (S.IsValid()) { S->Destroy(); }
	}
	Launched.Reset();
	auto Retire = [](AActor* A)
	{
		if (!A) { return; }
		A->SetActorTickEnabled(false);
		A->SetActorHiddenInGame(true);
		A->SetActorEnableCollision(false);
		if (UTDHealthComponent* H = UTDStatics::GetHealth(A)) { H->SetTargetable(false); H->SetInvulnerable(true); }
	};
	for (ATDEnemyTurret* T : Turrets) { Retire(T); }
	for (ATDEnemyTurret* T : TrenchTurretActors) { Retire(T); }
	for (ATDShieldGenerator* G : Generators) { Retire(G); }
	Retire(Reactor);
	Retire(DockingBay);
	if (DockingInterior) { DockingInterior->SetActorHiddenInGame(true); DockingInterior->SetActorEnableCollision(false); }
	ShowHull(false);
	SetState(ETDDominionState::Finished);
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
	if (P.Phase == ETDGamePhase::Victory)
	{
		FinishAfterVictory();	// a won run: the Dominion is gone, nothing fights
		return;
	}
	if (P.Phase == ETDGamePhase::Escape)
	{
		BeginEscapeFlight();	// back from the interior (T5's escape run): fly clear
		return;
	}
	// A voided wave-5 clear never sets the flag: the flow ignores SetDominionState while a retake is pending.
	if (!P.bDominionRevealed)
	{
		// Story Act 1: seen far off behind its distortion field, unreachable, until the passage opens: Cinder finished
		// AND wave 5 cleared (the wave director sets bDominionRevealed only then). The field is the approach; the four
		// generators hold the Dominion's separate defensive shield.
		if (bShowBehindField && !DistortionField && GetWorld())
		{
			FActorSpawnParameters SP;
			SP.Owner = this;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			DistortionField = GetWorld()->SpawnActor<ATDDistortionField>(ATDDistortionField::StaticClass(), GetActorTransform(), SP);
			if (DistortionField) { DistortionField->Init(this); }
		}
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
		T->Visuals = TurretVisuals;	// before Init: Init applies the art (BeginPlay already ran in SpawnActor)
		T->AttachToActor(this, Attach);
		T->SetActorRelativeLocation(Local);
		const FTDDifficultyScale Difficulty = UTDDifficultySettings::GetScale();
		T->Init(TurretGuns, TurretHull * Difficulty.EnemyHull);
		T->SetDamageMultiplier(Difficulty.EnemyDamage);
		T->SetActorScale3D(FVector(4.f));
		Into.Add(T);
	};

	// The art hull's sockets (r2: Generator_0..3, HullTurret_0..1, TrenchTurret_0..3, Reactor, Bay), in this actor's
	// frame. Without them, the placeholder layout below.
	auto SocketLocal = [&](const FString& Name, FTransform& Out) -> bool
	{
		const FName S(*Name);
		if (!HullMesh || !HullMesh->DoesSocketExist(S)) { return false; }
		Out = HullMesh->GetSocketTransform(S).GetRelativeTransform(GetActorTransform());
		// Position and yaw only: the imported r2 sockets carry a 90 degree roll (their Z axis lies along the ring tangent),
		// while the art contract is "+X out, Z up" (manifest ue_yaw_deg, z_up).
		Out.SetRotation(FRotator(0.f, Out.GetUnitAxis(EAxis::X).Rotation().Yaw, 0.f).Quaternion());
		Out.SetScale3D(FVector::OneVector);
		return true;
	};
	// A point inside the art hull's collision moves out along Out to the surface plus Clearance (a trace from 50 m out
	// meets the hull first: the point is buried). The same rule as the carrier's socket parts.
	auto ClearOfHull = [&](const FVector& Local, const FVector& OutLocal, float Clearance) -> FVector
	{
		if (!HullMesh || OutLocal.IsNearlyZero()) { return Local; }
		const FTransform Xf = GetActorTransform();
		const FVector P = Xf.TransformPosition(Local);
		const FVector Out = Xf.TransformVectorNoScale(OutLocal.GetSafeNormal());
		FHitResult Hit;
		// To Clearance past the point: a mount just under it (r2's guard perches: top Z 1420 under a 1500 turret) counts.
		if (HullMesh->LineTraceComponent(Hit, P + Out * 5000.f, P - Out * Clearance, FCollisionQueryParams(SCENE_QUERY_STAT(TDDominionClear), false)))
		{
			return Xf.InverseTransformPosition(FVector(Hit.ImpactPoint) + Out * Clearance);
		}
		return Local;
	};

	// Generators on pylons just outside the main shield, one per quadrant, each with its guards.
	const float Ring = ShieldRadius * 1.08f;
	for (int32 i = 0; i < 4; ++i)
	{
		const float Angle = PI * 0.25f + PI * 0.5f * i;
		FVector Local(FMath::Cos(Angle) * Ring, FMath::Sin(Angle) * Ring, 0.f);
		FVector Tangent(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		FTransform Sock;
		const bool bGenSocket = SocketLocal(FString::Printf(TEXT("Generator_%d"), i), Sock);
		if (bGenSocket)
		{
			Local = Sock.GetLocation();
			Tangent = Sock.GetUnitAxis(EAxis::Y);
		}
		ATDShieldGenerator* G = World->SpawnActorDeferred<ATDShieldGenerator>(ATDShieldGenerator::StaticClass(), FTransform::Identity, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!G) { continue; }
		G->Visuals = Visuals;	// before BeginPlay
		G->FinishSpawning(FTransform::Identity);
		G->AttachToActor(this, Attach);
		G->SetActorRelativeLocation(Local);
		if (bGenSocket) { G->SetActorRelativeRotation(Sock.GetRotation()); }
		G->Init(GeneratorHull * UTDDifficultySettings::GetScale().EnemyHull, i);
		if (bGenSocket)
		{
			// The art pylon's base on the plinth socket; the actor (its aim point) at the pylon's centre above it.
			G->SetActorRelativeLocation(Local + Sock.GetUnitAxis(EAxis::Z) * G->GetArtCentreHeight());
		}
		G->OnGeneratorDestroyed.AddUniqueDynamic(this, &ATDDominion::HandleGeneratorDestroyed);
		Generators.Add(G);

		for (int32 k = 0; k < TurretsPerGenerator; ++k)
		{
			const float Side = (k % 2 ? 1.f : -1.f) * (1 + k / 2) * 3000.f;
			const FVector Guard = Local + Tangent * Side + FVector(0, 0, 1500.f);
			// r2: the guards stand on the plinth's perches (mount top Z 1420), set just above the mount, not in it.
			SpawnTurret(bGenSocket ? ClearOfHull(Guard, FVector::UpVector, 200.f) : Guard, Turrets);
		}
	}

	// Hull turrets.
	for (int32 i = 0; i < ExtraTurrets; ++i)
	{
		const float X = FMath::Lerp(-0.6f, 0.6f, ExtraTurrets > 1 ? float(i) / (ExtraTurrets - 1) : 0.5f) * E.X;
		FTransform Sock;
		if (SocketLocal(FString::Printf(TEXT("HullTurret_%d"), i), Sock)) { SpawnTurret(Sock.GetLocation(), Turrets); }
		else { SpawnTurret(ClearOfHull(FVector(X, (i % 2 ? 1.f : -1.f) * E.Y, E.Z + 500.f), FVector::UpVector, 500.f), Turrets); }
	}

	// Reactor at the bottom of the trench, plus dormant trench turrets.
	const float TrenchZ = E.Z + 1500.f;
	Reactor = World->SpawnActorDeferred<ATDDominionReactor>(ATDDominionReactor::StaticClass(), FTransform::Identity, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Reactor)
	{
		Reactor->Visuals = Visuals;	// read in the reactor's BeginPlay, so set it before FinishSpawning
		Reactor->FinishSpawning(FTransform::Identity);
		Reactor->AttachToActor(this, Attach);
		FTransform Sock;
		Reactor->SetActorRelativeLocation(SocketLocal(TEXT("Reactor"), Sock) ? Sock.GetLocation() : FVector(E.X * 0.55f, 0.f, TrenchZ));	// clear of the hull top and the trench walls
		Reactor->SetExposed(false);
		Reactor->OnPhaseChanged.AddUniqueDynamic(this, &ATDDominion::HandleReactorPhase);
		Reactor->OnReactorDestroyed.AddUniqueDynamic(this, &ATDDominion::HandleReactorDestroyed);
	}
	for (int32 i = 0; i < TrenchTurrets; ++i)
	{
		const float X = FMath::Lerp(-0.5f, 0.4f, TrenchTurrets > 1 ? float(i) / (TrenchTurrets - 1) : 0.5f) * E.X;
		FTransform Sock;
		SpawnTurret(SocketLocal(FString::Printf(TEXT("TrenchTurret_%d"), i), Sock) ? Sock.GetLocation()
			: FVector(X, (i % 2 ? 1.f : -1.f) * 1700.f, TrenchZ + 1200.f), TrenchTurretActors);	// inside the walls (|y| < 2300)
	}
	for (ATDEnemyTurret* T : TrenchTurretActors)
	{
		T->SetActorTickEnabled(false);
		T->SetActorHiddenInGame(true);
		T->SetActorEnableCollision(false);	// no invisible cylinders in the trench before exposure
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
	if (Remaining == 3)
	{
		UTDStorySubsystem::Fire(this, TEXT("G6_Kato_ShieldWeakening"));	// storyline 24: the first generator down (A4_GeneratorDown1 retired by T3)
	}
	// Story (T3's transmissions): one per generator by count down, not by which one. Once per run, queued, held
	// during money shots; a retake or a resumed save (generators silently destroyed) never replays them.
	const int32 Down = FMath::Clamp(Generators.Num() - Remaining, 1, 4);
	UTDStorySubsystem::Fire(this, FName(*FString::Printf(TEXT("A4_GeneratorDown%d"), Down)));
	// The hostile fragments come through clearer as the shield weakens (T3, story alignment a16 / a19).
	if (Down == 2) { UTDStorySubsystem::Fire(this, TEXT("G6_Clue1")); }
	if (Down == 3) { UTDStorySubsystem::Fire(this, TEXT("G6_Clue2")); }
	// "Shield's down. Docking route marked." belongs to the boarding finale only (played on the space ending, Nov 14:
	// Command marked a docking route the fallback never opens). The space ending has A4_GeneratorDown4's "The shield is gone."
	if (Remaining == 0 && UsesBoardingFinale()) { UTDStorySubsystem::Fire(this, TEXT("G6_Command_ShieldCollapsed")); }
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

void ATDDominion::HandleActorKilled(AActor* Victim, AActor* /*Killer*/, ETDDamageKind /*Kind*/)
{
	// The trench run is the player's own run: losing the flown ship there is "CUT!" and a retake at the trench start,
	// not an auto-switch into a wingman a kilometre away facing out to space (effects builder and T4 live, Nov 14).
	if (State != ETDDominionState::ReactorExposed)
	{
		return;
	}
	const ATDShip* Ship = Cast<ATDShip>(Victim);
	if (!Ship || !Ship->IsPlayerFlown())
	{
		return;
	}
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		UE_LOG(LogTriadDrift, Log, TEXT("ATDDominion: the flown ship died in the trench run - CUT, retake at the trench start"));
		Flow->ReportPlayerDefeated();
	}
}

void ATDDominion::HandleAftermathBegins(bool bSkippedTutorial)
{
	// The escape line's point on the natural path; the player start on a tutorial skip.
	FVector From = GetActorLocation();
	FVector Normal;
	const ATDWaveDirector* Waves = ATDWaveDirector::Get(this);
	if (!bSkippedTutorial && Waves && Waves->GetEscapeBoundary(From, Normal)) {}
	else if (const ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(this)) { From = Layout->GetPlayerStart().GetLocation(); }
	StartAftermathLightRow(From);
}

void ATDDominion::StartAftermathLightRow(const FVector& WorldPoint)
{
	if (!Visuals || Visuals->AftermathLightRow.IsEmpty() || !HullMesh)
	{
		return;
	}
	// The hull point nearest WorldPoint (its box: before the reveal the hull is the hidden real ship, without collision),
	// +X along the outward normal of that face.
	const FBox B = GetLocalHullBounds();
	const FVector L = GetActorTransform().InverseTransformPosition(WorldPoint);
	const FVector OnBox = B.GetClosestPointTo(L);
	const FVector Ext = B.GetExtent(), C = B.GetCenter();
	const FVector Rel = (OnBox - C) / Ext.ComponentMax(FVector(1.f));
	const FVector N = FMath::Abs(Rel.X) >= FMath::Abs(Rel.Y) && FMath::Abs(Rel.X) >= FMath::Abs(Rel.Z) ? FVector(FMath::Sign(Rel.X), 0.f, 0.f)
		: FMath::Abs(Rel.Y) >= FMath::Abs(Rel.Z) ? FVector(0.f, FMath::Sign(Rel.Y), 0.f) : FVector(0.f, 0.f, FMath::Sign(Rel.Z));
	if (!LightRowAnchor)
	{
		LightRowAnchor = NewObject<USceneComponent>(this, TEXT("AftermathLightRow"));
		LightRowAnchor->SetupAttachment(GetRootComponent());
		LightRowAnchor->RegisterComponent();
	}
	LightRowAnchor->SetRelativeLocationAndRotation(OnBox, N.Rotation());
	bLightRowOn = true;
	if (!LightRowFX) { LightRowFX = UTDStatics::PlayCueAttached(Visuals->AftermathLightRow, LightRowAnchor); }
	else if (!LightRowFX->IsActive()) { LightRowFX->Activate(true); }
}

void ATDDominion::StopAftermathLightRow()
{
	bLightRowOn = false;
	if (LightRowFX) { LightRowFX->Deactivate(); }	// the system fades itself out within 1 s
}

bool ATDDominion::IsAftermathLightRowActive() const
{
	return bLightRowOn;	// the cue's state (its Niagara component exists only where something renders)
}

FTransform ATDDominion::GetAftermathLightRowTransform() const
{
	return LightRowAnchor ? LightRowAnchor->GetRelativeTransform() : FTransform::Identity;
}

void ATDDominion::HandleHyperdriveForLightRow(ETDHyperdriveState NewState)
{
	if (NewState == ETDHyperdriveState::Jumping) { StopAftermathLightRow(); }
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
	if (!bFromSave)
	{
		FTDWorldBarks::Say(this, TDWorldBarkIds::DomFieldDown, ETDBarkPriority::Critical, FTDWorldBarks::EWho::Kato, 0.f);	// dialogs.md Bark_Dom_FieldDown
	}
	if (Visuals && !bFromSave)
	{
		UTDStatics::PlayCue(this, Visuals->ShieldCollapse, GetActorLocation(), GetActorRotation());
	}
	if (UsesBoardingFinale())
	{
		BeginDockingFinale(/*bPlayCues*/ !bFromSave);
		return;
	}
	ExposeReactor(/*bPlayCues*/ !bFromSave);
}

bool ATDDominion::UsesBoardingFinale() const
{
	if (BoardingMode != ETDBoardingMode::Auto)
	{
		return BoardingMode == ETDBoardingMode::ForceOn;
	}
	if (CVarTDFinaleBoarding.GetValueOnGameThread() == 0)
	{
		return false;
	}
	// A real level only: test worlds (/Temp/Untitled) keep the space ending every existing test plays.
	const UWorld* World = GetWorld();
	if (!World || World->GetOutermost()->GetName().StartsWith(TEXT("/Temp/")))
	{
		return false;
	}
	return FPackageName::DoesPackageExist(TEXT("/Game/TriadDrift/Maps/L_Dominion_Interior"));
}

FTransform ATDDominion::BaySocketWorld() const
{
	// The socket's position and yaw (+X out of the mouth) with the Dominion's up: the imported sockets carry a roll.
	const FTransform S = HullMesh->GetSocketTransform(BaySocket);
	const FVector Out = FVector::VectorPlaneProject(S.GetUnitAxis(EAxis::X), GetActorUpVector()).GetSafeNormal();
	return FTransform(FRotationMatrix::MakeFromXZ(Out.IsNearlyZero() ? GetActorForwardVector() : Out, GetActorUpVector()).Rotator(), S.GetLocation());
}

void ATDDominion::BeginDockingFinale(bool bPlayCues)
{
	// The reactor stays shielded inside (T5's core fight); the trench turrets stay down. The hull turrets and the
	// launches keep the approach hot (stage 1 pressure).
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const bool bSocket = HullMesh && HullMesh->DoesSocketExist(BaySocket);
	if (!DockingBay)
	{
		// The art hull's Bay socket (+X out of the mouth) when it has one; else the real bow, low on the hull, facing out.
		const FBox B = GetLocalHullBounds();
		const FVector Local(B.Max.X, 0.f, FMath::Lerp(B.Min.Z, B.GetCenter().Z, 0.6f));
		const FTransform At = bSocket ? BaySocketWorld() : FTransform(GetActorRotation(), GetActorTransform().TransformPosition(Local));
		FActorSpawnParameters P;
		P.Owner = this;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		DockingBay = World->SpawnActor<ATDDockingBay>(ATDDockingBay::StaticClass(), At.GetLocation(), At.Rotator(), P);
		if (DockingBay)
		{
			if (bSocket) { DockingBay->AttachToComponent(HullMesh, FAttachmentTransformRules::KeepWorldTransform, BaySocket); }
			else { DockingBay->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform); }
		}
	}
	if (bSocket && !DockingInterior)
	{
		// The pocket behind the mouth (structures builder, Gameplay/Props: always cooked); its pivot is the mouth centre.
		if (UClass* Pocket = LoadClass<AActor>(nullptr, TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Dominion/BP_DockingBay_Interior.BP_DockingBay_Interior_C"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			FActorSpawnParameters P;
			P.Owner = this;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FTransform At = BaySocketWorld();
			DockingInterior = World->SpawnActor<AActor>(Pocket, At.GetLocation(), At.Rotator(), P);
			if (DockingInterior) { DockingInterior->AttachToComponent(HullMesh, FAttachmentTransformRules::KeepWorldTransform, BaySocket); }
		}
	}
	if (DockingBay) { DockingBay->Open(this, bPlayCues); }
	SetState(ETDDominionState::DockingOpen);
	ApplyStage(1);
	UE_LOG(LogTriadDrift, Log, TEXT("ATDDominion: boarding finale - the docking bay is open"));
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
		T->SetActorEnableCollision(true);
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
	// No breakup art yet: a placeholder chain of explosions walks the hull, then one big blast and the hull is gone.
	bPlaceholderBreakup = !Visuals || !FTDCombatPlaceholders::HasVisualArt(Visuals->BreakupSequence);
	BreakupBoomTimer = 0.f;
	BreakupBooms = 0;
	bBreakupFinalBlast = false;
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
	case ETDDominionState::DockingOpen:
		PollTimer -= DeltaSeconds;
		if (PollTimer <= 0.f)
		{
			PollTimer = ProgressPollSeconds;
			const ITDGameFlow* Flow = UTDStatics::GetGameFlow(this);
			if (Flow && Flow->GetProgress().Phase == ETDGamePhase::Victory)
			{
				FinishAfterVictory();	// TD.Stage Victory / a victory save: no fight under the victory phase
				break;
			}
		}
		Launched.RemoveAll([](const TWeakObjectPtr<ATDEnemyShip>& S) { return !S.IsValid(); });
		TickLaunches(DeltaSeconds);
		break;

	case ETDDominionState::Breakup:
		if (bPlaceholderBreakup) { TickPlaceholderBreakup(DeltaSeconds); }
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

	case ETDDominionState::EscapeFlight:
		TickEscapeFlight(DeltaSeconds);
		break;

	default:
		break;
	}
}

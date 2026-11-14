// Triad Drift - World (Teammate 4). ENG-34 planet terrain (see TDPlanetTerrain.h).

#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDCloudDeck.h"
#include "ProceduralMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Async/Async.h"
#include "TriadDrift/World/TDPlanetScatter.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/ScopeExit.h"
#include "Async/ParallelFor.h"

const FName ATDPlanetTerrain::TerrainTag(TEXT("TD.Engineering.Terrain"));

ATDPlanetTerrain::ATDPlanetTerrain()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	Tags.Add(TerrainTag);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	FarMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FarRanges"));
	FarMesh->SetupAttachment(Root);
	FarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FarMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	for (TObjectPtr<UStaticMeshComponent>* Slot : { &Sea, &CraterGlow })
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Slot == &Sea ? TEXT("Sea") : TEXT("CraterGlow"));
		C->SetupAttachment(Root);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetVisibility(false);
		if (Cylinder.Succeeded()) { C->SetStaticMesh(Cylinder.Object); }
		*Slot = C;
	}
}

ATDPlanetTerrain* ATDPlanetTerrain::SpawnForPlanet(UWorld* World, ETDDestination Planet, const FTransform& Origin, UMaterialInterface* Material)
{
	if (!World) { return nullptr; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPlanetTerrain* T = World->SpawnActor<ATDPlanetTerrain>(ATDPlanetTerrain::StaticClass(), FTransform(FRotator::ZeroRotator, Origin.GetLocation()), Params);
	if (!T) { return nullptr; }
	if (!Material)
	{
		// The Lead's slope / height terrain material (Gameplay/Props is always cooked), else the plain ground.
		const bool bV = Planet != ETDDestination::Cinder;
		for (const TCHAR* P : { bV ? TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Terrain_Verdance.MI_Terrain_Verdance") : TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Terrain_Cinder.MI_Terrain_Cinder"),
			bV ? TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Verdance.MI_Ground_Verdance") : TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Cinder.MI_Ground_Cinder") })
		{
			const TSoftObjectPtr<UMaterialInterface> Soft{ FSoftObjectPath(P) };
			if (!Material && FPackageName::DoesPackageExist(Soft.ToSoftObjectPath().GetLongPackageName())) { Material = Soft.LoadSynchronous(); }
		}
	}
	T->Setup(Planet, Material);
	return T;
	return T;
}

void ATDPlanetTerrain::Setup(ETDDestination Planet, UMaterialInterface* Material)
{
	const double SetupStart = FPlatformTime::Seconds();
	RegisterCourseFlats(Planet);	// #194: before the field is made, so it carries the relief
	Field = MakeShared<const FTDTerrainField, ESPMode::ThreadSafe>(FTDTerrainStyle::ForPlanet(Planet));
	// One dynamic instance per terrain: the material's height bands use this planet's real relief.
	TerrainMaterial = Material;
	if (Material)
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, this))
		{
			MID->SetScalarParameterValue(TEXT("HeightRangeM"), Field->ReliefMeters());
			TerrainMaterial = MID;
		}
	}
	const FTDTerrainStyle& S = Field->GetStyle();
	const int32 Half = FMath::CeilToInt(S.CoreHalfSize / ChunkSize);
	Chunks.Reset();
	for (int32 Y = -Half; Y < Half; ++Y)
	{
		for (int32 X = -Half; X < Half; ++X)
		{
			FChunk C;
			C.Cell = FIntPoint(X, Y);
			C.Section = 0;
			Chunks.Add(C);
			UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(this, *FString::Printf(TEXT("Chunk_%d_%d"), X, Y));
			M->SetupAttachment(Root);
			M->bUseAsyncCooking = true;	// refreshed chunks cook off the game thread: no hitch
			M->SetCollisionProfileName(TEXT("BlockAll"));
			M->SetCastShadow(true);
			M->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Static;	// rebuilt in place, never moved
			M->RegisterComponent();
			AddInstanceComponent(M);
			ChunkMeshes.Add(M);
		}
	}
	// The whole world at once, in parallel: final LOD around the course (M2: no pop on arrival), the rest coarse.
	// Collision under the pad cooks at once (ground to stand on from frame one); the rest cooks async (ENG-38: the
	// 256-chunk synchronous cook was the long pole of the planet load). BuildFirstViewAt moves this to an arrival.
	BuildAround(FVector::ZeroVector);	// this actor sits at the course origin

	// The region sea and the volcano's crater glow (placeholder looks until the terrain art covers them).
	{
		const FVector2D SC = Field->SeaCentre();
		const float SR = Field->SeaRadius();
		Sea->SetRelativeLocation(FVector(SC.X, SC.Y, S.SeaLevel));
		Sea->SetRelativeScale3D(FVector(SR * 2.f / 100.f, SR * 2.f / 100.f, 0.02f));
		Sea->SetVisibility(true);
		// The environment builder's sea (Gameplay/Props/Planets, always cooked); the tinted placeholder if it is missing.
		UMaterialInterface* SeaMat = LoadObject<UMaterialInterface>(nullptr, S.bLavaSea
			? TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_PlanetSea_Cinder.MI_PlanetSea_Cinder")
			: TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_PlanetSea_Verdance.MI_PlanetSea_Verdance"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (SeaMat)
		{
			Sea->SetMaterial(0, SeaMat);
		}
		else
		{
			UTDStatics::TintPlaceholder(Sea, S.bLavaSea ? FLinearColor(1.f, 0.35f, 0.06f) : FLinearColor(0.05f, 0.25f, 0.3f), S.bLavaSea ? 4.f : 0.6f);
		}
		if (S.bVolcano)
		{
			const float FloorZ = Field->Height(S.HeroCentre.X, S.HeroCentre.Y) + 800.f;
			CraterGlow->SetRelativeLocation(FVector(S.HeroCentre.X, S.HeroCentre.Y, FloorZ));
			CraterGlow->SetRelativeScale3D(FVector(S.CraterRadius * 1.5f / 100.f, S.CraterRadius * 1.5f / 100.f, 0.05f));
			CraterGlow->SetVisibility(true);
			UTDStatics::TintPlaceholder(CraterGlow, FLinearColor(1.f, 0.45f, 0.08f), 10.f);
		}
	}

	// Far ring: one coarse grid from the core edge to the far edge (a hole in the middle), no collision.
	{
		const float Far = S.FarHalfSize;
		const int32 N = FMath::Max(2, FMath::RoundToInt(2.f * Far / ChunkSize));	// 1 km cells: aligned with the core chunks
		const float Step = 2.f * Far / N;
		TArray<FVector> V; TArray<int32> Tri; TArray<FVector> Nrm; TArray<FVector2D> UV0, UV1; TArray<FColor> Col; TArray<FProcMeshTangent> Tan;
		for (int32 j = 0; j <= N; ++j)
		{
			for (int32 i = 0; i <= N; ++i)
			{
				const float X = -Far + i * Step, Y = -Far + j * Step;
				const FTDTerrainSample Sm = Field->Sample(X, Y, Step * 0.5f);
				// True heights everywhere (the old sunk inner part made slabs at the core edge: pale flat patches);
				// the quads inside the core are skipped below, so the ring is a ring. Real normals.
				// ENG-38: the ring bends down like a sphere's surface from the core's edge out (Z -= d^2 / 2R), so
				// the flat world's far edge drops under the horizon; the collidable core keeps its exact heights.
				V.Add(FVector(X, Y, Sm.Height - CurvatureDrop(X, Y)));
				const float HX = Field->Height(X + Step * 0.5f, Y) - Field->Height(X - Step * 0.5f, Y);
				const float HY = Field->Height(X, Y + Step * 0.5f) - Field->Height(X, Y - Step * 0.5f);
				Nrm.Add(FVector(-HX, -HY, Step).GetSafeNormal());
				UV0.Add(FVector2D(X, Y) / 1000.f);
				UV1.Add(FVector2D(Sm.Height / 100.f, Sm.Slope));
				Col.Add(FLinearColor(Sm.Height01, Sm.Slope, Sm.BroadMask, Sm.FineMask).ToFColor(false));
			}
		}
		for (int32 j = 0; j < N; ++j)
		{
			for (int32 i = 0; i < N; ++i)
			{
				// Skip the quads inside the core: the grid is aligned to the chunks, so the ring's inner edge is the core's edge.
				const float QX = -Far + (i + 0.5f) * Step, QY = -Far + (j + 0.5f) * Step;
				if (FMath::Max(FMath::Abs(QX), FMath::Abs(QY)) < S.CoreHalfSize) { continue; }
				const int32 A = j * (N + 1) + i, B = A + 1, C = A + N + 1, D = C + 1;
				Tri.Append({ A, C, B, B, C, D });
			}
		}
		FarMesh->CreateMeshSection(0, V, Tri, Nrm, UV0, UV1, TArray<FVector2D>(), TArray<FVector2D>(), Col, Tan, false);
		if (TerrainMaterial) { FarMesh->SetMaterial(0, TerrainMaterial); }	// the MID with HeightRangeM (the base material made the far ranges flat white)
	}
	// World density v1: the dressed play zone's trees and boulders (off-thread; td.Scatter 0 = none).
	Scatter = ATDPlanetScatter::SpawnFor(this, Planet);
	SetupMs = (FPlatformTime::Seconds() - SetupStart) * 1000.0;
	UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetTerrain: world built in %.0f ms (%d chunks, far ring, sync collision within %.0f m of the pad)"),
		SetupMs, Chunks.Num(), SyncCollisionRadius / 100.f);
	BuildDetailGround();	// #194: the relief patch (no-op without the course's flats)
}

void ATDPlanetTerrain::BuildAround(const FVector& View)
{
	TArray<int32> Todo;
	for (int32 i = 0; i < Chunks.Num(); ++i)
	{
		if (WantedLod(Chunks[i], View) != Chunks[i].Lod) { Todo.Add(i); }
	}
	TArray<FChunkData> Built;
	Built.SetNum(Todo.Num());
	ParallelFor(Todo.Num(), [this, &Built, &Todo, View](int32 k)
	{
		const int32 i = Todo[k];
		const int32 Lod = WantedLod(Chunks[i], View);
		GenerateChunk(*Field, Chunks[i].Cell, ChunkSize, QuadsFor(Lod), Built[k]);
		Built[k].Index = i;
		Built[k].Lod = Lod;
	});
	const FVector2D At(View.X, View.Y);
	for (FChunkData& D : Built)
	{
		UProceduralMeshComponent* M = ChunkMeshes.IsValidIndex(D.Index) ? ChunkMeshes[D.Index].Get() : nullptr;
		const FChunk& C = Chunks[D.Index];
		const FVector2D Centre((C.Cell.X + 0.5f) * ChunkSize, (C.Cell.Y + 0.5f) * ChunkSize);
		const bool bNear = FVector2D::Distance(Centre, At) - ChunkSize * 0.71f <= SyncCollisionRadius;
		if (M) { M->bUseAsyncCooking = !bNear; }
		Chunks[D.Index].Building = -1;
		ApplyChunk(D);
		if (M) { M->bUseAsyncCooking = true; }
		Pending.Remove(D.Index);
	}
}

float ATDPlanetTerrain::CurvatureDrop(float X, float Y) const
{
	if (CurvatureRadius <= 0.f || !Field.IsValid())
	{
		return 0.f;
	}
	const float Beyond = FMath::Max(0.f, FMath::Max(FMath::Abs(X), FMath::Abs(Y)) - Field->GetStyle().CoreHalfSize);
	return Beyond * Beyond / (2.f * CurvatureRadius);
}

double ATDPlanetTerrain::BuildFirstViewAt(const FVector& CourseLocalPoint)
{
	if (!Field.IsValid())
	{
		return 0.0;
	}
	const double Start = FPlatformTime::Seconds();
	const float Ground = Field->Height(CourseLocalPoint.X, CourseLocalPoint.Y);
	BuildAround(FVector(CourseLocalPoint.X, CourseLocalPoint.Y, FMath::Max(0.f, CourseLocalPoint.Z - Ground)));
	FirstViewMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetTerrain: first view around the arrival (%.1f, %.1f km, %.0f m up) in %.0f ms"),
		CourseLocalPoint.X / 100000.f, CourseLocalPoint.Y / 100000.f, (CourseLocalPoint.Z - Ground) / 100.f, FirstViewMs);
	return FirstViewMs;
}

void ATDPlanetTerrain::GenerateChunk(const FTDTerrainField& InField, FIntPoint Cell, float InChunkSize, int32 Quads, FChunkData& Out)
{
	const int32 N = FMath::Max(1, Quads);
	const float Step = InChunkSize / N;
	const float X0 = Cell.X * InChunkSize, Y0 = Cell.Y * InChunkSize;
	const int32 Count = (N + 1) * (N + 1);
	Out.V.Reset(Count); Out.Nrm.Reset(Count); Out.UV0.Reset(Count); Out.UV1.Reset(Count); Out.Col.Reset(Count); Out.Tri.Reset(N * N * 6 + N * 48);
	const float H = FMath::Max(200.f, Step * 0.5f);
	for (int32 j = 0; j <= N; ++j)
	{
		for (int32 i = 0; i <= N; ++i)
		{
			const float X = X0 + i * Step, Y = Y0 + j * Step;
			const FTDTerrainSample Sm = InField.Sample(X, Y, H);	// height + normal + masks: 5 height evaluations
			// #194: under the relief patch the coarse mesh sinks out of sight (one quad inside its edge, so no triangle
			// that crosses the edge shows a notch beside the patch).
			const FBox2D& Rg = InField.GetStyle().MicroRegion;
			const bool bUnder = InField.GetStyle().HasMicroRelief() && X > Rg.Min.X + Step && X < Rg.Max.X - Step && Y > Rg.Min.Y + Step && Y < Rg.Max.Y - Step;
			Out.V.Add(FVector(X, Y, Sm.Height - (bUnder ? 2000.f : 0.f)));
			Out.Nrm.Add(Sm.Normal);
			Out.UV0.Add(FVector2D(X, Y) / 1000.f);
			Out.UV1.Add(FVector2D(Sm.Height / 100.f, Sm.Slope));
			Out.Col.Add(FLinearColor(Sm.Height01, Sm.Slope, Sm.BroadMask, Sm.FineMask).ToFColor(false));
		}
	}
	for (int32 j = 0; j < N; ++j)
	{
		for (int32 i = 0; i < N; ++i)
		{
			const int32 A = j * (N + 1) + i, B = A + 1, Cc = A + N + 1, D = Cc + 1;
			Out.Tri.Append({ A, Cc, B, B, Cc, D });
		}
	}
	// Skirts: each border dropped 30 m, so LOD seams between neighbours never open a gap.
	auto Border = [N](int32 Edge, int32 K) -> int32
	{
		switch (Edge)
		{
		case 0: return K;
		case 1: return N * (N + 1) + K;
		case 2: return K * (N + 1);
		default: return K * (N + 1) + N;
		}
	};
	for (int32 Edge = 0; Edge < 4; ++Edge)
	{
		const int32 Base = Out.V.Num();
		for (int32 K = 0; K <= N; ++K)
		{
			const int32 Src = Border(Edge, K);
			const FVector P = Out.V[Src] - FVector(0.f, 0.f, 3000.f);
			const FVector Nm = Out.Nrm[Src];
			const FVector2D A0 = Out.UV0[Src], A1 = Out.UV1[Src];
			const FColor Cl = Out.Col[Src];
			Out.V.Add(P); Out.Nrm.Add(Nm); Out.UV0.Add(A0); Out.UV1.Add(A1); Out.Col.Add(Cl);
		}
		for (int32 K = 0; K < N; ++K)
		{
			const int32 TA = Border(Edge, K), TB = Border(Edge, K + 1), BA = Base + K, BB = Base + K + 1;
			// One winding, facing out of the chunk (edges 0 and 3 run the other way round): duplicate opposite-wound
			// triangles make the collision trimesh fail to cook.
			if (Edge == 0 || Edge == 3) { Out.Tri.Append({ TA, BA, TB, TB, BA, BB }); }
			else { Out.Tri.Append({ TA, TB, BA, TB, BB, BA }); }
		}
	}
}

void ATDPlanetTerrain::ApplyChunk(FChunkData& Data)
{
	if (!Chunks.IsValidIndex(Data.Index)) { return; }
	FChunk& C = Chunks[Data.Index];
	if (C.Building >= 0 && C.Building != Data.Lod) { return; }	// a newer LOD was requested meanwhile: drop this one
	UProceduralMeshComponent* M = ChunkMeshes.IsValidIndex(Data.Index) ? ChunkMeshes[Data.Index].Get() : nullptr;
	if (!M) { return; }
	M->CreateMeshSection(0, Data.V, Data.Tri, Data.Nrm, Data.UV0, Data.UV1, TArray<FVector2D>(), TArray<FVector2D>(), Data.Col,
		TArray<FProcMeshTangent>(), /*bCreateCollision*/ true);
	if (TerrainMaterial) { M->SetMaterial(0, TerrainMaterial); }
	C.Lod = Data.Lod;
	C.Building = -1;
}

void ATDPlanetTerrain::OnChunkReady(const TSharedPtr<FChunkData, ESPMode::ThreadSafe>& Data)
{
	--InFlight;
	Ready.Add(Data);
}

void ATDPlanetTerrain::FlushAll()
{
	for (UProceduralMeshComponent* M : ChunkMeshes) { M->bUseAsyncCooking = false; }
	ON_SCOPE_EXIT { for (UProceduralMeshComponent* M : ChunkMeshes) { M->bUseAsyncCooking = true; } };
	const FVector View = ViewLocal();
	for (int32 i = 0; i < Chunks.Num(); ++i)
	{
		const int32 Want = WantedLod(Chunks[i], View);
		if (Want == Chunks[i].Lod) { continue; }
		FChunkData D;
		GenerateChunk(*Field, Chunks[i].Cell, ChunkSize, QuadsFor(Want), D);
		D.Index = i;
		D.Lod = Want;
		ApplyChunk(D);
	}
	Pending.Reset();
}

FVector ATDPlanetTerrain::ViewLocal() const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	FVector P = GetActorLocation();
	if (bViewOverride) { P = ViewOverride; }
	else if (PC && PC->PlayerCameraManager) { P = PC->PlayerCameraManager->GetCameraLocation(); }
	else if (PC && PC->GetPawn()) { P = PC->GetPawn()->GetActorLocation(); }
	const FVector L = GetActorTransform().InverseTransformPosition(P);
	const float Ground = Field.IsValid() ? Field->Height(L.X, L.Y) : 0.f;
	return FVector(L.X, L.Y, FMath::Max(0.f, L.Z - Ground));
}

int32 ATDPlanetTerrain::WantedLod(const FChunk& C, const FVector& View) const
{
	const FVector2D Centre((C.Cell.X + 0.5f) * ChunkSize, (C.Cell.Y + 0.5f) * ChunkSize);
	// Ground distance to the chunk, then the viewer's height above the ground on top: LOD0 only once you are low.
	const float Flat = FMath::Max(0.f, FVector2D::Distance(Centre, FVector2D(View.X, View.Y)) - ChunkSize * 0.7f);
	const float D = FMath::Sqrt(FMath::Square(Flat) + FMath::Square(View.Z));
	const int32 Lod = D < Lod0Distance ? 0 : (D < Lod1Distance ? 1 : 2);
	// Silhouettes the player looks at from the course keep their shape: the hero peak never drops below LOD1.
	const FTDTerrainStyle& S = Field->GetStyle();
	const bool bHero = FVector2D::Distance(Centre, S.HeroCentre) < S.HeroRadius + ChunkSize;
	return bHero ? FMath::Min(Lod, 1) : Lod;
}

ATDPlanetTerrain* ATDPlanetTerrain::EnsureForWorld(UWorld* World, ETDDestination Planet)
{
	if (!World || IConsoleManager::Get().FindConsoleVariable(TEXT("td.PlanetTerrain")) && IConsoleManager::Get().FindConsoleVariable(TEXT("td.PlanetTerrain"))->GetInt() == 0)
	{
		return nullptr;
	}
	for (TActorIterator<ATDPlanetTerrain> It(World); It; ++It) { if (!It->IsActorBeingDestroyed()) { return *It; } }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPlanetTerrain* T = World->SpawnActor<ATDPlanetTerrain>(ATDPlanetTerrain::StaticClass(), FTransform::Identity, Params);
	if (T)
	{
		T->bAwaitingCourse = true;
		T->PendingPlanet = Planet;
	}
	return T;
}

int32 ATDPlanetTerrain::GetChunkLodAt(const FVector& WorldPoint) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(WorldPoint);
	const FIntPoint Cell(FMath::FloorToInt(L.X / ChunkSize), FMath::FloorToInt(L.Y / ChunkSize));
	for (const FChunk& C : Chunks) { if (C.Cell == Cell) { return C.Lod; } }
	return -1;
}

void ATDPlanetTerrain::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAwaitingCourse)
	{
		// The course sits at the landing pad (the planet director spawns it, maybe after us): the world centres on it.
		AwaitSeconds += DeltaSeconds;
		AActor* Course = nullptr;
		for (TActorIterator<ATDPlanetCourse> It(GetWorld()); It && !Course; ++It) { Course = *It; }
		if (Course || AwaitSeconds > 10.f)
		{
			bAwaitingCourse = false;
			if (Course) { SetActorLocation(Course->GetActorLocation()); }
			UMaterialInterface* M = nullptr;
			const bool bV = PendingPlanet != ETDDestination::Cinder;
			for (const TCHAR* P : { bV ? TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Terrain_Verdance.MI_Terrain_Verdance") : TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Terrain_Cinder.MI_Terrain_Cinder"),
				bV ? TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Verdance.MI_Ground_Verdance") : TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Cinder.MI_Ground_Cinder") })
			{
				const TSoftObjectPtr<UMaterialInterface> Soft{ FSoftObjectPath(P) };
				if (!M && FPackageName::DoesPackageExist(Soft.ToSoftObjectPath().GetLongPackageName())) { M = Soft.LoadSynchronous(); }
			}
			Setup(PendingPlanet, M);
		}
		return;
	}
	if (!Field) { return; }
	const FVector View = ViewLocal();
	ReplanTimer -= DeltaSeconds;
	if (ReplanTimer <= 0.f)
	{
		ReplanTimer = 0.4f;
		Pending.Reset();
		for (int32 i = 0; i < Chunks.Num(); ++i)
		{
			const int32 Want = WantedLod(Chunks[i], View);
			if (Want != Chunks[i].Lod && Chunks[i].Building != Want) { Pending.Add(i); }
		}
		Pending.Sort([this, View](int32 A, int32 B)
		{
			const FVector2D CA((Chunks[A].Cell.X + 0.5f) * ChunkSize, (Chunks[A].Cell.Y + 0.5f) * ChunkSize);
			const FVector2D CB((Chunks[B].Cell.X + 0.5f) * ChunkSize, (Chunks[B].Cell.Y + 0.5f) * ChunkSize);
			const FVector2D V2(View.X, View.Y);
			return FVector2D::DistSquared(CA, V2) < FVector2D::DistSquared(CB, V2);
		});
	}
	// Generate on workers (nearest first), at most MaxInFlight at a time.
	while (Pending.Num() > 0 && InFlight < MaxInFlight)
	{
		const int32 Index = Pending[0];
		Pending.RemoveAt(0, EAllowShrinking::No);
		FChunk& C = Chunks[Index];
		const int32 Want = WantedLod(C, View);
		if (Want == C.Lod || C.Building == Want) { continue; }
		C.Building = Want;
		++InFlight;
		TWeakObjectPtr<ATDPlanetTerrain> Weak(this);
		TSharedPtr<const FTDTerrainField, ESPMode::ThreadSafe> F = Field;
		const FIntPoint Cell = C.Cell;
		const float Size = ChunkSize;
		const int32 Quads = QuadsFor(Want);
		Async(EAsyncExecution::ThreadPool, [Weak, F, Cell, Size, Quads, Index, Want]()
		{
			TSharedPtr<FChunkData, ESPMode::ThreadSafe> Data = MakeShared<FChunkData, ESPMode::ThreadSafe>();
			GenerateChunk(*F, Cell, Size, Quads, *Data);
			Data->Index = Index;
			Data->Lod = Want;
			AsyncTask(ENamedThreads::GameThread, [Weak, Data]() { if (Weak.IsValid()) { Weak->OnChunkReady(Data); } });
		});
	}
	// Apply finished chunks within the frame budget (mesh upload; collision cooks async).
	const double Start = FPlatformTime::Seconds();
	int32 Lod0Applied = 0;
	while (Ready.Num() > 0 && (FPlatformTime::Seconds() - Start) * 1000.0 < BudgetMs)
	{
		if (Ready[0]->Lod == 0 && Lod0Applied >= MaxLod0PerFrame) { break; }	// one dense upload per frame
		TSharedPtr<FChunkData, ESPMode::ThreadSafe> D = Ready[0];
		Ready.RemoveAt(0, EAllowShrinking::No);
		Lod0Applied += D->Lod == 0 ? 1 : 0;
		ApplyChunk(*D);
	}
}

float ATDPlanetTerrain::GetCorridorMaxGroundZ(const FVector& A, const FVector& B) const
{
	if (!Field) { return GetActorLocation().Z; }
	// A 100 m grid over the corridor's bounding box, kept to points within the radius of the segment.
	const FVector LA = GetActorTransform().InverseTransformPosition(A);
	const FVector LB = GetActorTransform().InverseTransformPosition(B);
	const FVector2D A2(LA), B2(LB);
	const float R = DeckCorridorRadius, Step = 10000.f;
	const float X0 = FMath::Min(A2.X, B2.X) - R, X1 = FMath::Max(A2.X, B2.X) + R;
	const float Y0 = FMath::Min(A2.Y, B2.Y) - R, Y1 = FMath::Max(A2.Y, B2.Y) + R;
	float Max = -FLT_MAX;
	for (float Y = Y0; Y <= Y1; Y += Step)
	{
		for (float X = X0; X <= X1; X += Step)
		{
			const FVector P(X, Y, 0.f);
			if (FMath::PointDistToSegment(P, FVector(A2, 0.f), FVector(B2, 0.f)) > R) { continue; }
			Max = FMath::Max(Max, Field->Height(X, Y));
		}
	}
	return GetActorLocation().Z + (Max == -FLT_MAX ? 0.f : Max);
}

float ATDPlanetTerrain::GetCloudDeckZFor(const FVector& ArrivalWorld, const FVector& GlideEndWorld) const
{
	const float Pad = GetActorLocation().Z;
	return FMath::Clamp(GetCorridorMaxGroundZ(ArrivalWorld, GlideEndWorld) + DeckClearance, Pad + DeckMinAbovePad, Pad + DeckMaxAbovePad);
}

float ATDPlanetTerrain::GetCloudDeckZ() const
{
	if (CachedZoneMaxZ == TNumericLimits<float>::Lowest() && Field)
	{
		CachedZoneMaxZ = GetCloudDeckZFor(GetActorLocation(), GetActorLocation());	// the pad's corridor
	}
	return Field ? CachedZoneMaxZ : GetActorLocation().Z + DeckMinAbovePad;
}

void ATDPlanetTerrain::PrepareArrival(const FVector& GlideEndWorld)
{
	if (Scatter) { Scatter->SetFocusOverride(GlideEndWorld); }
}

void ATDPlanetTerrain::PrepareArrival(const FVector& ArrivalWorld, const FVector& GlideEndWorld)
{
	PrepareArrival(GlideEndWorld);
	const float Z = GetCloudDeckZFor(ArrivalWorld, GlideEndWorld);
	for (TActorIterator<ATDCloudDeck> It(GetWorld()); It; ++It)
	{
		It->SetActorLocation(FVector(It->GetActorLocation().X, It->GetActorLocation().Y, Z));
	}
}

bool ATDPlanetTerrain::IsArrivalReady(const FVector& GlideEndWorld) const
{
	if (!IsBuilt()) { return false; }
	if (!Scatter) { return true; }
	return Scatter->IsNearApplied() && Scatter->IsReadyAround(GlideEndWorld);
}

float ATDPlanetTerrain::GetHeightAt(const FVector& WorldPoint) const
{
	if (!Field) { return GetActorLocation().Z; }
	const FVector L = GetActorTransform().InverseTransformPosition(WorldPoint);
	return GetActorLocation().Z + Field->Height(L.X, L.Y);
}

// ------------------------------------------------------------------------------------------------ #194 relief patch

void ATDPlanetTerrain::RegisterCourseFlats(ETDDestination Planet)
{
	// The course's walked ground (pad, routes at walking width, sites, structures, spawns), course-local, stays flat.
	// Encounter volumes keep their relief (their spawn points are listed apart).
	const ATDPlanetCourse* Course = ATDPlanetCourse::Get(this);
	if (!Course) { FTDTerrainStyle::ClearCourseFlats(Planet); return; }
	const FTransform Xf = Course->GetActorTransform();
	FTDCourseFlats Flats;
	for (const FTDCourseKeepOut& K : Course->GetKeepOuts())
	{
		if (K.Kind == TEXT("Encounter")) { continue; }
		if (K.Points.Num() > 1)
		{
			for (int32 i = 0; i + 1 < K.Points.Num(); ++i)
			{
				const FVector A = Xf.InverseTransformPosition(K.Points[i]), B = Xf.InverseTransformPosition(K.Points[i + 1]);
				Flats.Segs.Add({ FVector2D(A.X, A.Y), FVector2D(B.X, B.Y), K.HalfWidth });
			}
		}
		else if (K.Points.Num() == 1)
		{
			const FVector A = Xf.InverseTransformPosition(K.Points[0]);
			Flats.Segs.Add({ FVector2D(A.X, A.Y), FVector2D(A.X, A.Y), K.HalfWidth });
		}
		else if (K.Box.IsValid)
		{
			const FVector A = Xf.InverseTransformPosition(K.Box.Min), B = Xf.InverseTransformPosition(K.Box.Max);
			Flats.Boxes.Add(FBox2D(FVector2D(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y)), FVector2D(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y))));
		}
	}
	// The pad is always flat, whatever the course lists.
	Flats.Segs.Add({ FVector2D::ZeroVector, FVector2D::ZeroVector, ATDPlanetCourse::PadClearRadius });
	FTDTerrainStyle::RegisterCourseFlats(Planet, Flats);
	UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetTerrain: course ground registered flat (%d boxes, %d route / circle segments)"), Flats.Boxes.Num(), Flats.Segs.Num());
}

void ATDPlanetTerrain::BuildDetailGround()
{
	if (!Field.IsValid() || !Field->GetStyle().HasMicroRelief() || DetailGround) { return; }
	DetailGround = NewObject<UProceduralMeshComponent>(this, TEXT("DetailGround"));
	DetailGround->SetupAttachment(Root);
	DetailGround->bUseAsyncCooking = false;	// built under the arrival cover; the ground must collide on the first frame
	DetailGround->SetCollisionProfileName(TEXT("BlockAll"));	// the pawn and every Visibility trace (crew, pads)
	// Turn 18: static caching alone left the VSM marking overflow on screen. The patch is one large non-Nanite mesh
	// over ~500 m: it receives every shadow but casts none (its 2-6 m relief reads by its normals; the coarse chunks
	// under it still cast for the land around).
	DetailGround->SetCastShadow(false);
	// Lead / T6 (Nov 14): with the relief on, the screen showed "VSM Non-Nanite Marking Job Queue overflow". The patch
	// never moves once built: the virtual shadow map caches its pages instead of re-marking them every frame.
	DetailGround->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Static;
	DetailGround->RegisterComponent();
	AddInstanceComponent(DetailGround);
	const FBox2D Rg = Field->GetStyle().MicroRegion;
	const float Sp = DetailSpacing;
	const int32 NX = FMath::CeilToInt32((float)Rg.GetSize().X / Sp), NY = FMath::CeilToInt32((float)Rg.GetSize().Y / Sp);
	const int32 Tile = 64;	// quads per section side (128 m at 2 m)
	TWeakObjectPtr<ATDPlanetTerrain> Weak(this);
	const TSharedPtr<const FTDTerrainField, ESPMode::ThreadSafe> F = Field;
	const double Start = FPlatformTime::Seconds();
	Async(EAsyncExecution::ThreadPool, [Weak, F, Rg, Sp, NX, NY, Tile, Start]()
	{
		TSharedPtr<TArray<FChunkData>, ESPMode::ThreadSafe> Tiles = MakeShared<TArray<FChunkData>, ESPMode::ThreadSafe>();
		const int32 TX = FMath::DivideAndRoundUp(NX, Tile), TY = FMath::DivideAndRoundUp(NY, Tile);
		Tiles->SetNum(TX * TY);
		ParallelFor(TX * TY, [&](int32 t)
		{
			FChunkData& D = (*Tiles)[t];
			const int32 I0 = (t % TX) * Tile, J0 = (t / TX) * Tile;
			const int32 QX = FMath::Min(Tile, NX - I0), QY = FMath::Min(Tile, NY - J0);
			for (int32 j = 0; j <= QY; ++j)
			{
				for (int32 i = 0; i <= QX; ++i)
				{
					const float X = FMath::Min((float)Rg.Min.X + (I0 + i) * Sp, (float)Rg.Max.X), Y = FMath::Min((float)Rg.Min.Y + (J0 + j) * Sp, (float)Rg.Max.Y);
					const FTDTerrainSample Sm = F->Sample(X, Y, Sp * 0.5f);
					D.V.Add(FVector(X, Y, Sm.Height));
					D.Nrm.Add(Sm.Normal);
					D.UV0.Add(FVector2D(X, Y) / 1000.f);
					D.UV1.Add(FVector2D(Sm.Height / 100.f, Sm.Slope));
					D.Col.Add(FLinearColor(Sm.Height01, Sm.Slope, Sm.BroadMask, Sm.FineMask).ToFColor(false));
				}
			}
			for (int32 j = 0; j < QY; ++j)
			{
				for (int32 i = 0; i < QX; ++i)
				{
					const int32 A = j * (QX + 1) + i, B = A + 1, Cc = A + QX + 1, Dd = Cc + 1;
					D.Tri.Append({ A, Cc, B, B, Cc, Dd });
				}
			}
		});
		AsyncTask(ENamedThreads::GameThread, [Weak, Tiles, Start]()
		{
			ATDPlanetTerrain* Self = Weak.Get();
			if (!Self || !Self->DetailGround) { return; }
			for (int32 t = 0; t < Tiles->Num(); ++t)
			{
				FChunkData& D = (*Tiles)[t];
				Self->DetailGround->CreateMeshSection(t, D.V, D.Tri, D.Nrm, D.UV0, D.UV1, TArray<FVector2D>(), TArray<FVector2D>(), D.Col,
					TArray<FProcMeshTangent>(), /*bCreateCollision*/ true);
				if (Self->TerrainMaterial) { Self->DetailGround->SetMaterial(t, Self->TerrainMaterial); }
			}
			Self->bDetailReady = true;
			UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetTerrain: relief ground ready (%d sections, %.0f ms from start)"), Tiles->Num(), (FPlatformTime::Seconds() - Start) * 1000.0);
		});
	});
}

bool ATDPlanetTerrain::HasDetailGround(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World) { return false; }
	for (TActorIterator<ATDPlanetTerrain> It(World); It; ++It)
	{
		if (It->bDetailReady) { return true; }
	}
	return false;
}

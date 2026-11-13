// Triad Drift - World (Teammate 4). ENG-34 planet terrain (see TDPlanetTerrain.h).

#include "TriadDrift/World/TDPlanetTerrain.h"
#include "ProceduralMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Async/Async.h"
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
			M->RegisterComponent();
			AddInstanceComponent(M);
			ChunkMeshes.Add(M);
		}
	}
	// Coarse world at once (first view ready before the arrival camera), then refine near the viewer.
	// Coarse LOD of everything right away, in parallel: the first view is complete before the arrival camera.
	TArray<FChunkData> Coarse;
	Coarse.SetNum(Chunks.Num());
	ParallelFor(Chunks.Num(), [this, &Coarse](int32 i)
	{
		GenerateChunk(*Field, Chunks[i].Cell, ChunkSize, QuadsLod2, Coarse[i]);
		Coarse[i].Index = i;
		Coarse[i].Lod = 2;
	});
	// Synchronous collision for the first world: the pad and the arrival have ground to stand on from frame one.
	for (UProceduralMeshComponent* M : ChunkMeshes) { M->bUseAsyncCooking = false; }
	for (FChunkData& D : Coarse) { ApplyChunk(D); }
	for (UProceduralMeshComponent* M : ChunkMeshes) { M->bUseAsyncCooking = true; }

	// The region sea and the volcano's crater glow (placeholder looks until the terrain art covers them).
	{
		const FVector2D SC = Field->SeaCentre();
		const float SR = Field->SeaRadius();
		Sea->SetRelativeLocation(FVector(SC.X, SC.Y, S.SeaLevel));
		Sea->SetRelativeScale3D(FVector(SR * 2.f / 100.f, SR * 2.f / 100.f, 0.02f));
		Sea->SetVisibility(true);
		UTDStatics::TintPlaceholder(Sea, S.bLavaSea ? FLinearColor(1.f, 0.35f, 0.06f) : FLinearColor(0.05f, 0.25f, 0.3f), S.bLavaSea ? 4.f : 0.6f);
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
				V.Add(FVector(X, Y, Sm.Height));
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
		if (Material) { FarMesh->SetMaterial(0, Material); }
	}
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
			const FTDTerrainSample Sm = InField.Sample(X, Y, H);
			Out.V.Add(FVector(X, Y, Sm.Height));
			const float HX = InField.Height(X + H, Y) - InField.Height(X - H, Y);
			const float HY = InField.Height(X, Y + H) - InField.Height(X, Y - H);
			Out.Nrm.Add(FVector(-HX, -HY, 2.f * H).GetSafeNormal());
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
	const FVector2D View = ViewLocal();
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

FVector2D ATDPlanetTerrain::ViewLocal() const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	FVector P = GetActorLocation();
	if (PC && PC->PlayerCameraManager) { P = PC->PlayerCameraManager->GetCameraLocation(); }
	else if (PC && PC->GetPawn()) { P = PC->GetPawn()->GetActorLocation(); }
	const FVector L = GetActorTransform().InverseTransformPosition(P);
	return FVector2D(L.X, L.Y);
}

int32 ATDPlanetTerrain::WantedLod(const FChunk& C, const FVector2D& View) const
{
	const FVector2D Centre((C.Cell.X + 0.5f) * ChunkSize, (C.Cell.Y + 0.5f) * ChunkSize);
	const float D = FMath::Max(0.f, FVector2D::Distance(Centre, View) - ChunkSize * 0.7f);
	const int32 Lod = D < Lod0Distance ? 0 : (D < Lod1Distance ? 1 : 2);
	// Silhouettes the player looks at from the course keep their shape: the hero peak never drops below LOD1.
	const FTDTerrainStyle& S = Field->GetStyle();
	const bool bHero = FVector2D::Distance(Centre, S.HeroCentre) < S.HeroRadius + ChunkSize;
	return bHero ? FMath::Min(Lod, 1) : Lod;
}

void ATDPlanetTerrain::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Field) { return; }
	const FVector2D View = ViewLocal();
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
			return FVector2D::DistSquared(CA, View) < FVector2D::DistSquared(CB, View);
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
	while (Ready.Num() > 0 && (FPlatformTime::Seconds() - Start) * 1000.0 < BudgetMs)
	{
		TSharedPtr<FChunkData, ESPMode::ThreadSafe> D = Ready[0];
		Ready.RemoveAt(0, EAllowShrinking::No);
		ApplyChunk(*D);
	}
}

float ATDPlanetTerrain::GetHeightAt(const FVector& WorldPoint) const
{
	if (!Field) { return GetActorLocation().Z; }
	const FVector L = GetActorTransform().InverseTransformPosition(WorldPoint);
	return GetActorLocation().Z + Field->Height(L.X, L.Y);
}

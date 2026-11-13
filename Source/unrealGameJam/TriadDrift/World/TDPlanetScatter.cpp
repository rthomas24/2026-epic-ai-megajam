// Triad Drift - World (Teammate 4). World density v1: planet scatter driven by the terrain field.
#include "TriadDrift/World/TDPlanetScatter.h"

#include "TriadDrift/World/TDTerrainField.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/Core/TDTypes.h"

#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"

static TAutoConsoleVariable<int32> CVarTDScatter(
	TEXT("td.Scatter"), 1,
	TEXT("World density: 1 = planets get their trees and boulders across the play zone; 0 = bare terrain. Read when the planet loads."));

namespace
{
	// Tree species in Gameplay/Props/Trees (PVE, real size): name and the three growth-stage ids (S100, S070, S030).
	struct FTDScatterSpecies { const TCHAR* Name; int32 Id100; };
	const FTDScatterSpecies GTDScatterTrees[] = {
		{ TEXT("BrazilNut"), 21 }, { TEXT("Cacao"), 61 }, { TEXT("Kapok"), 11 },
		{ TEXT("StiltPalm"), 41 }, { TEXT("TreeFern"), 51 }, { TEXT("TrumpetTree"), 31 } };
	enum ETDScatterTree : int32 { BrazilNut, Cacao, Kapok, StiltPalm, TreeFern, TrumpetTree };

	/** Cheap deterministic hash -> 0..1 (seed, cell, salt). */
	float TDScatter_Hash01(int32 Seed, int32 X, int32 Y, int32 Salt)
	{
		uint32 H = uint32(Seed) * 0x9E3779B1u ^ uint32(X) * 0x85EBCA77u ^ uint32(Y) * 0xC2B2AE3Du ^ uint32(Salt) * 0x27D4EB2Fu;
		H ^= H >> 15; H *= 0x2C1B3C6Du; H ^= H >> 12; H *= 0x297A2D39u; H ^= H >> 15;
		return (H & 0xFFFFFF) / float(0x1000000);
	}

	int32 TDScatter_TreeKind(int32 Species, int32 Stage)	// Stage 0 = S100, 1 = S070, 2 = S030
	{
		return Species * 3 + Stage;
	}
}

int32 FTDScatterResult::Total() const
{
	int32 N = 0;
	for (const TArray<FTransform>& A : Near) { N += A.Num(); }
	for (const TArray<FTransform>& A : Far) { N += A.Num(); }
	return N;
}

FTDScatterPlan FTDScatterPlan::ForPlanet(ETDDestination Planet)
{
	FTDScatterPlan P;
	if (Planet != ETDDestination::Verdance)
	{
		return P;	// Cinder follows with its own lists (tomorrow)
	}
	for (const FTDScatterSpecies& S : GTDScatterTrees)
	{
		for (int32 Stage = 0; Stage < 3; ++Stage)
		{
			const int32 Id = S.Id100 + Stage;
			const int32 Size = Stage == 0 ? 100 : (Stage == 1 ? 70 : 30);
			FTDScatterKind K;
			K.MeshPath = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/Trees/SM_TDVeg_%s_S%03d_%d.SM_TDVeg_%s_S%03d_%d"), S.Name, Size, Id, S.Name, Size, Id);
			K.CullStartCm = Stage == 2 ? 60000.f : 180000.f;	// saplings are detail; grown trees read to ~2 km
			K.CullEndCm = Stage == 2 ? 90000.f : 240000.f;
			K.SinkCm = 150.f;
			K.bBlocksOnFoot = Stage != 2;
			P.Kinds.Add(K);
		}
	}
	P.FirstRock = P.Kinds.Num();
	for (int32 i = 1; i <= 3; ++i)
	{
		FTDScatterKind K;
		K.MeshPath = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Rock_%02d.SM_Verd_Rock_%02d"), i, i);
		K.MinScale = 1.5f;
		K.MaxScale = 4.5f;
		K.SinkCm = 60.f;
		K.CullStartCm = 100000.f;
		K.CullEndCm = 140000.f;
		K.bBlocksOnFoot = true;
		P.Kinds.Add(K);
	}
	return P;
}

bool FTDScatterPlan::IsKeptOut(const FTDTerrainField& Field, float X, float Y) const
{
	const FTDTerrainStyle& S = Field.GetStyle();
	return FMath::Abs(X - S.BasinCentre.X) < S.BasinHalfExtent.X + KeepOutMargin
		&& FMath::Abs(Y - S.BasinCentre.Y) < S.BasinHalfExtent.Y + KeepOutMargin;
}

void FTDScatterPlan::Generate(const FTDTerrainField& Field, FTDScatterResult& Out) const
{
	Out.Near.SetNum(Kinds.Num());
	Out.Far.SetNum(Kinds.Num());
	if (Kinds.Num() == 0)
	{
		return;
	}
	const FTDTerrainStyle& S = Field.GetStyle();
	const int32 Seed = S.Seed * 7919 + 17;
	const float Floor = S.BasinFloor;
	auto ZoneWeight = [this](float X, float Y)
	{
		const float D = FMath::Sqrt(X * X + Y * Y);
		if (D > ZoneRadius) { return 0.f; }
		return FMath::Lerp(1.f, ZoneEdgeDensity, FMath::SmoothStep(ZoneFullRadius, ZoneRadius, D));
	};
	auto NearCourse = [this, &S](float X, float Y)
	{
		return FMath::Abs(X - S.BasinCentre.X) < S.BasinHalfExtent.X + CollisionRadius
			&& FMath::Abs(Y - S.BasinCentre.Y) < S.BasinHalfExtent.Y + CollisionRadius;
	};

	// One pass per layer over a jittered grid; rows in parallel, each row writing its own buckets, merged after.
	struct FRow { TArray<TArray<FTransform>> Near, Far; };
	auto Layer = [&](float Spacing, int32 Salt, TFunctionRef<int32(const FTDTerrainSample&, float, float, float /*Rnd*/)> Pick)
	{
		const int32 N = FMath::CeilToInt(ZoneRadius / Spacing);
		const int32 Rows = 2 * N;
		TArray<FRow> RowsOut;
		RowsOut.SetNum(Rows);
		ParallelFor(Rows, [&](int32 r)
		{
			FRow& R = RowsOut[r];
			R.Near.SetNum(Kinds.Num());
			R.Far.SetNum(Kinds.Num());
			const int32 iy = r - N;
			for (int32 ix = -N; ix < N; ++ix)
			{
				const float X = (ix + TDScatter_Hash01(Seed, ix, iy, Salt)) * Spacing;
				const float Y = (iy + TDScatter_Hash01(Seed, ix, iy, Salt + 1)) * Spacing;
				const float Zone = ZoneWeight(X, Y);
				if (Zone <= 0.f || TDScatter_Hash01(Seed, ix, iy, Salt + 2) > Zone || IsKeptOut(Field, X, Y))
				{
					continue;
				}
				const FTDTerrainSample Sm = Field.Sample(X, Y, Spacing * 0.25f);
				if (Sm.Height < S.SeaLevel + 1000.f)
				{
					continue;	// the sea and its shore
				}
				const int32 Kind = Pick(Sm, (Sm.Height - Floor) / 100.f, Sm.BroadMask, TDScatter_Hash01(Seed, ix, iy, Salt + 3));
				if (!Kinds.IsValidIndex(Kind))
				{
					continue;
				}
				const FTDScatterKind& K = Kinds[Kind];
				const float Scale = FMath::Lerp(K.MinScale, K.MaxScale, TDScatter_Hash01(Seed, ix, iy, Salt + 4));
				const float Yaw = TDScatter_Hash01(Seed, ix, iy, Salt + 5) * 360.f;
				// Trees stand up (a slight lean with the slope); rocks tumble.
				const bool bRock = Kind >= FirstRock;
				const FRotator Rot = bRock ? FRotator(TDScatter_Hash01(Seed, ix, iy, Salt + 6) * 40.f - 20.f, Yaw, TDScatter_Hash01(Seed, ix, iy, Salt + 7) * 40.f - 20.f)
					: FRotator(Sm.Normal.Y * 8.f, Yaw, -Sm.Normal.X * 8.f);
				const FTransform Xf(Rot, FVector(X, Y, Sm.Height - K.SinkCm * Scale), FVector(Scale));
				(NearCourse(X, Y) ? R.Near : R.Far)[Kind].Add(Xf);
			}
		});
		for (FRow& R : RowsOut)
		{
			for (int32 k = 0; k < Kinds.Num(); ++k)
			{
				Out.Near[k].Append(R.Near[k]);
				Out.Far[k].Append(R.Far[k]);
			}
		}
	};

	// Trees: rainforest / mixed / open by the biome mask; thinning up the slopes and above ~500 m, none above 1.3 km.
	Layer(TreeSpacing, 101, [this](const FTDTerrainSample& Sm, float HeightM, float Moist, float Rnd) -> int32
	{
		if (Sm.Slope > 0.55f || HeightM > 1300.f) { return INDEX_NONE; }
		const float Density = (Moist > 0.55f ? 0.9f : (Moist > 0.4f ? 0.5f : 0.2f))
			* FMath::Clamp(1.f - (Sm.Slope - 0.2f) * 2.5f, 0.f, 1.f) * FMath::Clamp(1.f - (HeightM - 500.f) / 800.f, 0.f, 1.f);
		if (Rnd > Density) { return INDEX_NONE; }
		const float Pick = Rnd / FMath::Max(Density, 0.01f);	// re-use the roll, 0..1 within the kept ones
		int32 Species;
		if (Moist > 0.55f) { Species = Pick < 0.15f ? Kapok : (Pick < 0.45f ? BrazilNut : (Pick < 0.75f ? TrumpetTree : Cacao)); }
		else if (Moist > 0.4f) { Species = Pick < 0.35f ? StiltPalm : (Pick < 0.7f ? TreeFern : Cacao); }
		else { Species = Pick < 0.6f ? TreeFern : StiltPalm; }
		const float StageRoll = FMath::Frac(Pick * 7.31f);
		return TDScatter_TreeKind(Species, StageRoll < 0.6f ? 0 : (StageRoll < 0.9f ? 1 : 2));
	});
	// Boulders: sparse everywhere, more on slopes and the rocky heights.
	Layer(RockSpacing, 211, [this](const FTDTerrainSample& Sm, float HeightM, float, float Rnd) -> int32
	{
		const float Density = 0.06f + 0.4f * FMath::Clamp((Sm.Slope - 0.3f) * 2.f, 0.f, 1.f) + 0.15f * FMath::Clamp((HeightM - 700.f) / 600.f, 0.f, 1.f);
		if (Rnd > Density) { return INDEX_NONE; }
		return FirstRock + FMath::Min(2, int32(Rnd / FMath::Max(Density, 0.01f) * 3.f));
	});
}

// ------------------------------------------------------------------------------------------------ actor

ATDPlanetScatter::ATDPlanetScatter()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

ATDPlanetScatter* ATDPlanetScatter::SpawnFor(ATDPlanetTerrain* Terrain, ETDDestination InPlanet)
{
	UWorld* World = Terrain ? Terrain->GetWorld() : nullptr;
	if (!World || CVarTDScatter.GetValueOnGameThread() == 0 || FTDScatterPlan::ForPlanet(InPlanet).Kinds.Num() == 0)
	{
		return nullptr;
	}
	FActorSpawnParameters P;
	P.Owner = Terrain;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPlanetScatter* S = World->SpawnActor<ATDPlanetScatter>(ATDPlanetScatter::StaticClass(), Terrain->GetActorTransform(), P);
	if (S)
	{
		S->AttachToActor(Terrain, FAttachmentTransformRules::KeepWorldTransform);
		S->Planet = InPlanet;
		S->Plan = FTDScatterPlan::ForPlanet(InPlanet);
		S->StartAsync();
	}
	return S;
}

void ATDPlanetScatter::StartAsync()
{
	if (bBuilt || bBuilding)
	{
		return;
	}
	bBuilding = true;
	TWeakObjectPtr<ATDPlanetScatter> Weak(this);
	const ETDDestination P = Planet;
	const FTDScatterPlan PlanCopy = Plan;
	Async(EAsyncExecution::ThreadPool, [Weak, P, PlanCopy]()
	{
		const double Start = FPlatformTime::Seconds();
		const FTDTerrainField Field(FTDTerrainStyle::ForPlanet(P));
		FTDScatterResult R;
		PlanCopy.Generate(Field, R);
		const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
		AsyncTask(ENamedThreads::GameThread, [Weak, R = MoveTemp(R), Ms]() mutable
		{
			if (ATDPlanetScatter* Self = Weak.Get()) { Self->Apply(MoveTemp(R), Ms); }
		});
	});
}

void ATDPlanetScatter::BuildNow()
{
	if (bBuilt)
	{
		return;
	}
	const double Start = FPlatformTime::Seconds();
	const FTDTerrainField Field(FTDTerrainStyle::ForPlanet(Planet));
	FTDScatterResult R;
	Plan.Generate(Field, R);
	Apply(MoveTemp(R), (FPlatformTime::Seconds() - Start) * 1000.0);
}

void ATDPlanetScatter::Apply(FTDScatterResult&& Result, double Ms)
{
	bBuilding = false;
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;
	GenerateMs = Ms;
	const double ApplyStart = FPlatformTime::Seconds();
	for (int32 k = 0; k < Plan.Kinds.Num(); ++k)
	{
		const FTDScatterKind& K = Plan.Kinds[k];
		UStaticMesh* Mesh = nullptr;
		const FSoftObjectPath Path(K.MeshPath);
		if (FPackageName::DoesPackageExist(Path.GetLongPackageName()))
		{
			Mesh = Cast<UStaticMesh>(Path.TryLoad());
		}
		if (!Mesh)
		{
			continue;
		}
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			const TArray<FTransform>& Xfs = Pass == 0 ? Result.Near[k] : Result.Far[k];
			if (Xfs.Num() == 0) { continue; }
			UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			H->SetStaticMesh(Mesh);
			H->SetupAttachment(Root);
			// Movable, explicitly: (H)ISMs default to Static, and a Static component cannot attach to the actor's movable
			// root - Unreal aborts the attach and the forest lands around the world origin instead of the course (live).
			H->SetMobility(EComponentMobility::Movable);
			H->SetCullDistances(int32(K.CullStartCm), int32(K.CullEndCm));
			H->SetCastShadow(Pass == 0 || k >= Plan.FirstRock);	// the far canopy skips dynamic shadow
			const bool bCollide = Pass == 0 && K.bBlocksOnFoot;
			if (bCollide)
			{
				// On foot near the course: trunks and boulders block the pilot. Ships and shots fly through.
				H->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				H->SetCollisionResponseToAllChannels(ECR_Ignore);
				H->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
				H->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			}
			else
			{
				H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			H->RegisterComponent();
			H->AddInstances(Xfs, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false);
			AddInstanceComponent(H);
			Hisms.Add(H);
			InstanceCount += Xfs.Num();
			if (Hisms.Num() <= 2)
			{
				FTransform First;
				H->GetInstanceTransform(0, First, /*bWorldSpace*/ true);
				UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetScatter: %s x%d, mesh radius %.0f cm, first at %s (world), visible %d, cull %d-%d"),
					*Mesh->GetName(), Xfs.Num(), Mesh->GetBounds().SphereRadius, *First.GetLocation().ToCompactString(), H->IsVisible() ? 1 : 0,
					H->InstanceStartCullDistance, H->InstanceEndCullDistance);
			}
		}
	}
	UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetScatter: %d instances in %d HISMs (generate %.0f ms off-thread, apply %.0f ms)"),
		InstanceCount, Hisms.Num(), GenerateMs, (FPlatformTime::Seconds() - ApplyStart) * 1000.0);
}

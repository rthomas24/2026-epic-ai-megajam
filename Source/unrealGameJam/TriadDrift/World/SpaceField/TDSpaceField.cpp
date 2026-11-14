// Triad Drift - the space field, galaxy layer. Owner: Teammate 5 (T4 reviews).

#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"
#include "TriadDrift/Combat/Pickups/TDPickupSubsystem.h"

namespace TDSpaceFieldImpl
{
	static int32 GEnabled = 1;
	static FAutoConsoleVariableRef CVarSpaceField(TEXT("td.SpaceField"), GEnabled,
		TEXT("Space field (belts, clusters, debris fields, wrecks, buoys): 1 on (default), 0 hidden with no collision."));

	static UStaticMesh* Load(const TCHAR* Path)
	{
		const FString P(Path);
		const FString Full = P.Contains(TEXT(".")) ? P : P + TEXT(".") + FPackageName::GetShortName(P);
		return LoadObject<UStaticMesh>(nullptr, *Full, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}

	static float DistToSegment(const FVector& P, const FVector& A, const FVector& B)
	{
		return (float)FVector::Dist(P, FMath::ClosestPointOnSegment(P, A, B));
	}

	/** Sizes: many small, few large (a power law between Min and Max). */
	static float PowerSize(FRandomStream& Rng, float Min, float Max, float Exponent)
	{
		return FMath::Lerp(Min, Max, FMath::Pow(Rng.FRand(), Exponent));
	}
}

ATDSpaceField::ATDSpaceField()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

ATDSpaceField* ATDSpaceField::SpawnFor(ATDGalaxyLayout* InLayout)
{
	UWorld* World = InLayout ? InLayout->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;
	ATDSpaceField* Field = World->SpawnActor<ATDSpaceField>(ATDSpaceField::StaticClass(), FTransform::Identity, Params);
	if (Field)
	{
		Field->Layout = InLayout;
		Field->FinishSpawning(FTransform::Identity);
	}
	return Field;
}

ATDSpaceField* ATDSpaceField::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	for (TActorIterator<ATDSpaceField> It(World); World && It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ATDSpaceField::BeginPlay()
{
	Super::BeginPlay();
	if (!Layout)
	{
		Layout = ATDGalaxyLayout::Get(this);
	}
	Build();
	// Caches: four per debris field, the types in turn (one use each per run).
	Caches.Reset();
	const TArray<FVector> Spots = GetCacheSpots(this);
	for (int32 i = 0; i < Spots.Num(); ++i)
	{
		FCache C;
		C.Spot = Spots[i];
		C.Type = static_cast<uint8>(i % 4);
		Caches.Add(C);
	}
	SetFieldEnabled(TDSpaceFieldImpl::GEnabled != 0);
}

void ATDSpaceField::Build()
{
	using namespace TDSpaceFieldImpl;
	if (!Layout)
	{
		return;
	}
	// The kit: the classic rocks and the space dressing (all real-scale meshes, normalised by their bounds).
	Families.SetNum(6);
	auto Fill = [this](int32 F, std::initializer_list<const TCHAR*> Paths)
	{
		for (const TCHAR* P : Paths)
		{
			if (UStaticMesh* M = Load(P)) { Families[F].Add(M); }
		}
	};
	Fill(0, { TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_01"), TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_03"), TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_04") });
	Fill(1, { TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Ice_01"), TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Ice_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Ice_03") });
	Fill(2, { TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Metal_01"), TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Metal_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Metal_03") });
	Fill(3, { TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Crystal_01"), TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Crystal_02") });
	Fill(4, { TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_SpaceDebris_01"), TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_SpaceDebris_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_SpaceDebris_03"), TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_DebrisCluster_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_DebrisCluster_02") });
	Fill(5, { TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_StationTruss_01"), TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_StationTruss_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_StationTruss_03"), TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Derelict_Satellite_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Derelict_Satellite_02"), TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_CargoPod_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_CargoPod_02"), TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_CargoPod_03") });
	BuoyMesh = Load(TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_BeaconBuoy_01"));
	SphereMesh = Load(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	// A classic rock is the fallback for any family the kit is missing.
	for (int32 F = 1; F < Families.Num(); ++F)
	{
		if (Families[F].Num() == 0) { Families[F] = Families[0]; }
	}
	if (Families[0].Num() == 0 && SphereMesh)
	{
		for (TArray<TObjectPtr<UStaticMesh>>& F : Families) { F.Add(SphereMesh); }
	}

	// The lanes (world space): combat zone to each planet, the Dominion and every gate end.
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);
	Lanes.Reset();
	for (const FTDPlanetSpec& Pl : L.Planets) { Lanes.Add({ Zone, Xf.TransformPosition(Pl.Location) }); }
	Lanes.Add({ Zone, Xf.TransformPosition(L.Dominion.GetLocation()) });
	for (const FTDGatePairSpec& G : L.Gates)
	{
		Lanes.Add({ Zone, Xf.TransformPosition(G.A.GetLocation()) });
		Lanes.Add({ Zone, Xf.TransformPosition(G.B.GetLocation()) });
	}
	BuoyLanes = Lanes.Num();
	// Keep-out only (no buoys): each gate pair's own line, and every return point back to the zone (take-offs).
	for (const FTDGatePairSpec& G : L.Gates) { Lanes.Add({ Xf.TransformPosition(G.A.GetLocation()), Xf.TransformPosition(G.B.GetLocation()) }); }
	for (const TPair<FName, FTransform>& RP : L.ReturnPoints) { Lanes.Add({ Xf.TransformPosition(RP.Value.GetLocation()), Zone }); }
	// The Verdance convoys run from the Dominion's field to Verdance (TDGalaxyLayout::Build, offsets up to 900 m).
	ConvoyLanes.Reset();
	for (const FTDPlanetSpec& Pl : L.Planets)
	{
		if (Pl.Destination == ETDDestination::Verdance) { ConvoyLanes.Add({ Xf.TransformPosition(L.Dominion.GetLocation()), Xf.TransformPosition(Pl.Location) }); }
	}
	TumbleRng.Initialize(Seed * 977 + 5);
	Tumblers.Reset();

	// Hidden collision spheres for the big pieces (the visuals never collide).
	if (SphereMesh)
	{
		Collision = NewObject<UInstancedStaticMeshComponent>(this, TEXT("FieldCollision"));
		Collision->SetupAttachment(RootComponent);
		Collision->SetStaticMesh(SphereMesh);
		Collision->SetHiddenInGame(true);
		Collision->SetCastShadow(false);
		Collision->SetCollisionProfileName(TEXT("BlockAll"));
		Collision->RegisterComponent();
	}

	FRandomStream Rng(Seed * 7919 + Layout->GetLayout().AsteroidFields.Num());
	BuildBelts(Rng);
	BuildClusters(Rng);
	BuildDebrisFields(Rng);
	BuildBuoys();
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: %d pieces, %d components, %d obstacles, %d cache spots"), Pieces.Num(), Components.Num(), ObstaclesAdded,
		GetCacheSpots(this).Num());
}

bool ATDSpaceField::IsPlacementAllowed(const FVector& P, float Size, bool bCollides) const
{
	using namespace TDSpaceFieldImpl;
	if (!Layout)
	{
		return false;
	}
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const float Half = Size * 0.5f;
	// Inside the flyable sphere.
	if (FVector::Dist(P, Xf.TransformPosition(L.BoundaryCenter)) > L.BoundaryRadius * 0.95f - Half)
	{
		return false;
	}
	// The combat zone (the existing belt and flank field are untouched).
	const float ToZone = (float)FVector::Dist(P, Xf.TransformPosition(L.CombatZoneCenter));
	if (ToZone < CombatClearAll + Half || (bCollides && ToZone < CombatClearHittable + Half))
	{
		return false;
	}
	// Lanes: never in the core; nothing hittable in the band.
	for (const TArray<FVector>& Lane : Lanes)
	{
		const float D = DistToSegment(P, Lane[0], Lane[1]);
		if (D < LaneCoreRadius + Half || (bCollides && D < LaneHittableRadius + Half))
		{
			return false;
		}
	}
	// The convoy lanes: visuals may line them, nothing hittable inside the tube.
	for (const TArray<FVector>& Lane : ConvoyLanes)
	{
		const float D = DistToSegment(P, Lane[0], Lane[1]);
		if (D < LaneCoreRadius + Half || (bCollides && D < ConvoyKeepOut + Half))
		{
			return false;
		}
	}
	if (bCollides)
	{
		for (const TPair<FName, FTransform>& RP : L.ReturnPoints)
		{
			if (FVector::Dist(P, Xf.TransformPosition(RP.Value.GetLocation())) < ReturnPointKeepOut + Half) { return false; }
		}
	}
	// Gates, return points, the player start (the layout's own zones).
	if (Layout->IsInKeepOut(Xf.InverseTransformPosition(P), bCollides ? PointKeepOut + Half : Half))
	{
		return false;
	}
	// Planets and their approach, the Dominion and its hangar exit.
	for (const FTDPlanetSpec& Pl : L.Planets)
	{
		if (FVector::Dist(P, Xf.TransformPosition(Pl.Location)) < Pl.AtmosphereRadius * AtmosphereKeepOutScale + Half)
		{
			return false;
		}
	}
	if (FVector::Dist(P, Xf.TransformPosition(L.Dominion.GetLocation())) < DominionKeepOut + Half)
	{
		return false;
	}
	if (const FTransform* Exit = L.ReturnPoints.Find(TDReturnPoints::DominionHangarExit))
	{
		if (FVector::Dist(P, Xf.TransformPosition(Exit->GetLocation())) < DominionKeepOut + Half)
		{
			return false;
		}
	}
	return true;
}

float ATDSpaceField::NormFor(UStaticMesh* Mesh) const
{
	const float R = Mesh ? Mesh->GetBounds().SphereRadius : 50.f;
	return R > 1.f ? 0.5f / R : 1.f; // scale per cm of bounding diameter
}

UHierarchicalInstancedStaticMeshComponent* ATDSpaceField::ComponentFor(UStaticMesh* Mesh, float Size)
{
	const int32 Tier = Size < 1000.f ? 0 : Size < 4000.f ? 1 : 2;
	const FString Key = FString::Printf(TEXT("%s_%d"), *GetNameSafe(Mesh), Tier);
	if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Found = Components.Find(Key))
	{
		return *Found;
	}
	static const int32 CullEnd[] = { 150000, 400000, 900000 }; // by size: small 1.5 km, mid 4 km, large 9 km
	UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("Field_%s"), *Key));
	C->SetupAttachment(RootComponent);
	C->SetStaticMesh(Mesh);
	C->SetMobility(EComponentMobility::Static);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCanEverAffectNavigation(false);
	C->SetCastShadow(Tier == 2);
	C->bAffectDistanceFieldLighting = false;
	C->SetCullDistances(0, CullEnd[Tier]);
	C->RegisterComponent();
	Components.Add(Key, C);
	return C;
}

bool ATDSpaceField::AddPiece(UStaticMesh* Mesh, const FVector& At, const FRotator& Rot, float Size, ETDSpaceFieldKind Kind, bool bCollides, bool bTumble)
{
	if (!Mesh || !IsPlacementAllowed(At, Size, bCollides))
	{
		return false;
	}
	const FTransform PieceXf(Rot, At, FVector(Size * NormFor(Mesh)));
	if (bTumble)
	{
		TObjectPtr<UInstancedStaticMeshComponent>& T = TumbleComponents.FindOrAdd(Mesh);
		if (!T)
		{
			T = NewObject<UInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("Tumble_%s"), *GetNameSafe(Mesh)));
			T->SetupAttachment(RootComponent);
			T->SetStaticMesh(Mesh);
			T->SetMobility(EComponentMobility::Movable);
			T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			T->SetCanEverAffectNavigation(false);
			T->SetCastShadow(true);
			T->bAffectDistanceFieldLighting = false;
			T->SetCullDistances(0, 900000);
			T->RegisterComponent();
		}
		T->AddInstance(PieceXf, true);
		FTumbler Tu;
		Tu.Comp = T;
		Tu.Home = PieceXf;
		Tu.Axis = FVector(TumbleRng.GetUnitVector());
		Tu.RadPerSec = FMath::DegreesToRadians(TumbleRng.FRandRange(1.5f, 5.f)) * (TumbleRng.FRand() < 0.5f ? -1.f : 1.f);
		Tumblers.Add(Tu);
	}
	else
	{
		ComponentFor(Mesh, Size)->AddInstance(PieceXf, true);
	}
	FTDSpaceFieldPiece Piece;
	Piece.Location = At;
	Piece.Size = Size;
	Piece.bCollides = bCollides;
	Piece.Kind = Kind;
	Pieces.Add(Piece);
	if (bCollides && Collision)
	{
		// A sphere a little inside the rock (the silhouette reads bigger than the hit), and an avoidance obstacle.
		Collision->AddInstance(FTransform(FRotator::ZeroRotator, At, FVector(Size * 0.4f / 50.f)), true);
		if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
		{
			WorldSys->AddObstacle(At, Size * 0.5f);
			++ObstaclesAdded;
		}
	}
	return true;
}

bool ATDSpaceField::AddRock(const FVector& At, float Size, ETDSpaceFieldKind Kind, FRandomStream& Rng, int32 Family)
{
	const TArray<TObjectPtr<UStaticMesh>>& Pool = Families[FMath::Clamp(Family, 0, Families.Num() - 1)];
	if (Pool.Num() == 0)
	{
		return false;
	}
	UStaticMesh* Mesh = Pool[Rng.RandRange(0, Pool.Num() - 1)];
	const FRotator Rot(Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f));
	// A separate stream picks the tumblers, so placement never changes with MaxTumblers.
	const bool bTumble = Size >= TumbleFromSize && Tumblers.Num() < MaxTumblers && TumbleRng.FRand() < 0.35f;
	return AddPiece(Mesh, At, Rot, Size, Kind, Size >= CollideFromSize, bTumble);
}

void ATDSpaceField::BuildBelts(FRandomStream& Rng)
{
	using namespace TDSpaceFieldImpl;
	const FTransform Xf = Layout->GetActorTransform();
	const FVector Zone = Xf.TransformPosition(Layout->GetLayout().CombatZoneCenter);
	for (int32 B = 0; B < BeltRadii.Num(); ++B)
	{
		// A tilted ring: tilt about X, then a seeded turn about Z.
		const FQuat Tilt = FQuat(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f)) * FQuat(FRotator(0.f, 0.f, BeltTilts.IsValidIndex(B) ? BeltTilts[B] : 10.f));
		// Families: the inner belt rock and ice, the outer metal and crystal, in long arcs (a band of one kind).
		const int32 FamA = B == 0 ? 0 : 2, FamB = B == 0 ? 1 : 3;
		int32 Placed = 0;
		for (int32 Try = 0; Placed < BeltRocks && Try < BeltRocks * 3; ++Try)
		{
			const float Ang = Rng.FRandRange(0.f, 2.f * PI);
			const float Rad = BeltRadii[B] + Rng.FRandRange(-1.f, 1.f) * Rng.FRand() * BeltHalfWidth;
			const float Up = Rng.FRandRange(-1.f, 1.f) * Rng.FRand() * BeltHalfThickness;
			const FVector Local(FMath::Cos(Ang) * Rad, FMath::Sin(Ang) * Rad, Up);
			const float Size = PowerSize(Rng, 800.f, 15000.f, 8.f); // mostly small: about one in five collides
			const int32 Fam = FMath::Sin(Ang * 3.f + B) > 0.f ? FamA : FamB;
			Placed += AddRock(Zone + Tilt.RotateVector(Local), Size, ETDSpaceFieldKind::Belt, Rng, Fam) ? 1 : 0;
		}
	}
}

void ATDSpaceField::BuildClusters(FRandomStream& Rng)
{
	using namespace TDSpaceFieldImpl;
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);
	// Even directions (a Fibonacci sphere of exactly the count) so every bearing, up and down too, has something; a
	// direction blocked at one distance tries others.
	const int32 N = Clusters;
	for (int32 Shell = 0; Shell < ClusterShells.Num(); ++Shell)
	{
	// The outer shell's directions are turned half a step, so its clusters sit between the inner shell's.
	const FQuat ShellTurn(FVector::UpVector, Shell * PI * (3.f - FMath::Sqrt(5.f)) * 0.5f);
	TArray<FVector> Dirs;
	for (int32 i = 0; i < N; ++i)
	{
		const float Y = 1.f - 2.f * (i + 0.5f) / N;
		const float R = FMath::Sqrt(FMath::Max(0.f, 1.f - Y * Y));
		const float Phi = i * PI * (3.f - FMath::Sqrt(5.f));
		Dirs.Add(ShellTurn.RotateVector(FVector(FMath::Cos(Phi) * R, FMath::Sin(Phi) * R, Y)).GetSafeNormal());
	}
	int32 Made = 0;
	for (const FVector& Dir : Dirs)
	{
		if (Made >= Clusters)
		{
			break;
		}
		FVector C = FVector::ZeroVector;
		bool bFound = false;
		for (int32 Try = 0; Try < 8 && !bFound; ++Try)
		{
			C = Zone + Dir * Rng.FRandRange(ClusterShells[Shell].X, ClusterShells[Shell].Y);
			bFound = IsPlacementAllowed(C, 40000.f, false);
		}
		if (!bFound)
		{
			continue;
		}
		++Made;
		const int32 Fam = Rng.RandRange(0, 3);
		// Landmarks first: a few big rocks near the cluster's heart (they collide and steer the AI).
		for (int32 Lm = 0, Try = 0; Lm < LandmarksPerCluster && Try < LandmarksPerCluster * 4; ++Try)
		{
			const FVector Off = FVector(Rng.GetUnitVector()) * Rng.FRandRange(0.f, 14000.f);
			Lm += AddRock(C + Off, Rng.FRandRange(6000.f, 15000.f), ETDSpaceFieldKind::Cluster, Rng, Fam) ? 1 : 0;
		}
		int32 Placed = 0;
		for (int32 Try = 0; Placed < RocksPerCluster && Try < RocksPerCluster * 3; ++Try)
		{
			const FVector Off = FVector(Rng.GetUnitVector()) * Rng.FRandRange(0.f, 1.f) * Rng.FRandRange(0.3f, 1.f) * 25000.f;
			Placed += AddRock(C + Off, PowerSize(Rng, 500.f, 12000.f, 8.f), ETDSpaceFieldKind::Cluster, Rng, Rng.FRand() < 0.7f ? Fam : Rng.RandRange(0, 3)) ? 1 : 0;
		}
	}
	}
}

void ATDSpaceField::BuildDebrisFields(FRandomStream& Rng)
{
	const FTransform Xf = Layout->GetActorTransform();
	const FVector Zone = Xf.TransformPosition(Layout->GetLayout().CombatZoneCenter);
	DebrisFieldCenters.Reset();
	for (int32 Try = 0; DebrisFieldCenters.Num() < DebrisFields && Try < DebrisFields * 30; ++Try)
	{
		FVector Dir = FVector(Rng.GetUnitVector());
		Dir.Z *= 0.5f;
		const FVector C = Zone + Dir.GetSafeNormal() * Rng.FRandRange(250000.f, 700000.f);
		bool bFar = IsPlacementAllowed(C, 50000.f, true);
		for (const FVector& O : DebrisFieldCenters) { bFar &= FVector::Dist(O, C) > 200000.f; }
		if (!bFar)
		{
			continue;
		}
		DebrisFieldCenters.Add(C);
		// The wreck: a large hull piece that drifts on a small loop and turns (a landmark that moves).
		const TArray<TObjectPtr<UStaticMesh>>& Wreckage = Families[5];
		UStaticMesh* Hull = Wreckage.Num() > 0 ? Wreckage[Rng.RandRange(0, FMath::Min(4, Wreckage.Num() - 1))].Get() : nullptr;
		const float WreckSize = Rng.FRandRange(8000.f, 20000.f);
		if (Hull && IsPlacementAllowed(C, WreckSize + 6000.f, true))
		{
			UInstancedStaticMeshComponent* W = NewObject<UInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("Wreck_%d"), DebrisFieldCenters.Num()));
			W->SetupAttachment(RootComponent);
			W->SetStaticMesh(Hull);
			W->SetMobility(EComponentMobility::Movable);
			W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			W->SetCastShadow(true);
			W->RegisterComponent();
			const FTransform Home(FRotator(Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-30.f, 30.f)), C, FVector(WreckSize * NormFor(Hull)));
			W->AddInstance(Home, true);
			WreckComponents.Add(W);
			WreckHomes.Add(Home);
			// Its collision covers the whole loop it drifts on (3 m/s round a 30 m circle).
			FTDSpaceFieldPiece Piece;
			Piece.Location = C;
			Piece.Size = WreckSize + 6000.f;
			Piece.bCollides = true;
			Piece.Kind = ETDSpaceFieldKind::Wreck;
			Pieces.Add(Piece);
			if (Collision)
			{
				Collision->AddInstance(FTransform(FRotator::ZeroRotator, C, FVector(WreckSize * 0.4f / 50.f)), true);
			}
			if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
			{
				WorldSys->AddObstacle(C, WreckSize * 0.5f + 3000.f);
				++ObstaclesAdded;
			}
		}
		// The battle debris around it: hull plates, struts, pods and small rubble, visual only.
		int32 Placed = 0;
		for (int32 P = 0; Placed < PiecesPerDebrisField && P < PiecesPerDebrisField * 3; ++P)
		{
			const FVector Off = FVector(Rng.GetUnitVector()) * (WreckSize * 0.7f + Rng.FRandRange(0.f, 30000.f) * Rng.FRand());
			const bool bWreckage = Rng.FRand() < 0.55f;
			const TArray<TObjectPtr<UStaticMesh>>& Pool = Families[bWreckage ? 5 : 4];
			if (Pool.Num() == 0) { continue; }
			const float Size = TDSpaceFieldImpl::PowerSize(Rng, 150.f, bWreckage ? 2400.f : 900.f, 2.5f);
			Placed += AddPiece(Pool[Rng.RandRange(0, Pool.Num() - 1)], C + Off,
				FRotator(Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f)), Size, ETDSpaceFieldKind::Debris, false) ? 1 : 0;
		}
	}
}

void ATDSpaceField::BuildBuoys()
{
	if (!BuoyMesh)
	{
		return;
	}
	// A blinking buoy every BuoySpacing beside every lane, alternating sides, outside the hittable band.
	for (int32 LaneIndex = 0; LaneIndex < BuoyLanes && LaneIndex < Lanes.Num(); ++LaneIndex)
	{
		const TArray<FVector>& Lane = Lanes[LaneIndex];
		const FVector Dir = (Lane[1] - Lane[0]).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
		const float Len = (float)FVector::Dist(Lane[0], Lane[1]);
		int32 k = 0;
		for (float D = CombatClearAll + BuoySpacing * 0.5f; D < Len - 20000.f; D += BuoySpacing, ++k)
		{
			const FVector At = Lane[0] + Dir * D + Side * ((k % 2) ? 1.f : -1.f) * (LaneHittableRadius + 3000.f);
			AddPiece(BuoyMesh, At, Dir.Rotation(), 900.f, ETDSpaceFieldKind::Buoy, false);
		}
	}
}

TArray<FVector> ATDSpaceField::GetCacheSpots(const UObject* WorldContextObject)
{
	TArray<FVector> Out;
	const ATDSpaceField* Field = Get(WorldContextObject);
	if (!Field)
	{
		return Out;
	}
	FRandomStream Rng(Field->Seed * 131);
	for (const FVector& C : Field->DebrisFieldCenters)
	{
		int32 Made = 0;
		for (int32 Try = 0; Made < 4 && Try < 40; ++Try)
		{
			const FVector P = C + FVector(Rng.GetUnitVector()) * Rng.FRandRange(12000.f, 26000.f);
			bool bClear = Field->IsPlacementAllowed(P, 3000.f, true);
			for (const FTDSpaceFieldPiece& Piece : Field->Pieces)
			{
				if (Piece.bCollides && FVector::Dist(Piece.Location, P) < Piece.Size * 0.5f + 1500.f) { bClear = false; break; }
			}
			if (bClear)
			{
				Out.Add(P);
				++Made;
			}
		}
	}
	return Out;
}

void ATDSpaceField::SetFieldEnabled(bool bOn)
{
	bEnabled = bOn;
	for (const TPair<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& C : Components)
	{
		if (C.Value) { C.Value->SetVisibility(bOn); }
	}
	for (UInstancedStaticMeshComponent* W : WreckComponents)
	{
		if (W) { W->SetVisibility(bOn); }
	}
	for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& T : TumbleComponents)
	{
		if (T.Value) { T.Value->SetVisibility(bOn); }
	}
	if (!bOn)
	{
		// Off: no cache in space (an uncollected one comes back when the field is on again).
		for (FCache& C : Caches)
		{
			if (AActor* P = C.Pickup.Get()) { P->Destroy(); }
			C.Pickup.Reset();
		}
	}
	if (Collision)
	{
		Collision->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

void ATDSpaceField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if ((TDSpaceFieldImpl::GEnabled != 0) != bEnabled)
	{
		SetFieldEnabled(TDSpaceFieldImpl::GEnabled != 0);
	}
	if (!bEnabled)
	{
		return;
	}
	UpdateTumblers();
	// Every frame (about 20 distance checks): the magnet pulls a taken cache off its spot within a few frames.
	UpdateCaches();
	// The wrecks drift round a 30 m loop (about 1.6 m/s) and turn slowly: they never leave their keep-out-checked spot.
	Clock += DeltaSeconds;
	for (int32 i = 0; i < WreckComponents.Num(); ++i)
	{
		UInstancedStaticMeshComponent* W = WreckComponents[i];
		if (!W || !WreckHomes.IsValidIndex(i)) { continue; }
		const float Phase = Clock * (2.f * PI / 120.f) + i * 1.3f;
		FTransform T = WreckHomes[i];
		T.AddToTranslation(FVector(FMath::Cos(Phase), FMath::Sin(Phase), 0.f) * 3000.f);
		T.SetRotation(T.GetRotation() * FQuat(FVector(0.3f, 0.2f, 1.f).GetSafeNormal(), Clock * FMath::DegreesToRadians(1.5f + i * 0.4f)));
		W->UpdateInstanceTransform(0, T, true, true, true);
	}
}

void ATDSpaceField::UpdateTumblers()
{
	if (Tumblers.Num() == 0)
	{
		return;
	}
	// One batch per component; instances were added in tumbler order, so a component's tumblers are its instances 0..N.
	for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : TumbleComponents)
	{
		UInstancedStaticMeshComponent* Comp = Pair.Value;
		if (!Comp) { continue; }
		TumbleScratch.Reset();
		for (const FTumbler& T : Tumblers)
		{
			if (T.Comp != Comp) { continue; }
			FTransform X = T.Home;
			X.SetRotation(FQuat(T.Axis, Clock * T.RadPerSec) * T.Home.GetRotation());
			TumbleScratch.Add(X);
		}
		if (TumbleScratch.Num() == Comp->GetInstanceCount())
		{
			Comp->BatchUpdateInstancesTransforms(0, TumbleScratch, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
		}
	}
}

void ATDSpaceField::UpdateCaches()
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Pawn)
	{
		UpdateCachesAt(Pawn->GetActorLocation());
	}
}

void ATDSpaceField::UpdateCachesAt(const FVector& Ship)
{
	if (!bEnabled || !UTDPickupSubsystem::IsEnabled())
	{
		return;
	}
	// The magnet pulls a pickup to the flown ship and collects it on contact. A cache that vanished after it was pulled
	// off its spot (or with the ship right on it) was taken: never again this run. One that timed out (20 s) or was
	// cleared by a CUT never moved: it comes back the next time the ship is near.
	for (FCache& C : Caches)
	{
		if (C.bCollected) { continue; }
		if (AActor* P = C.Pickup.Get())
		{
			C.LastPickupLocation = P->GetActorLocation();
			continue;
		}
		if (!C.Pickup.IsExplicitlyNull())
		{
			C.Pickup.Reset();
			if (FVector::Dist(C.LastPickupLocation, C.Spot) > 300.f || FVector::Dist(Ship, C.Spot) < 3000.f)
			{
				C.bCollected = true;
				continue;
			}
		}
		if (FVector::Dist(Ship, C.Spot) < CacheSpawnDistance)
		{
			C.Pickup = UTDPickupSubsystem::SpawnPickup(this, static_cast<ETDPickupType>(C.Type), C.Spot);
			C.LastPickupLocation = C.Spot;
		}
	}
	LastShipLocation = Ship;
}

int32 ATDSpaceField::GetCachesCollected() const
{
	int32 N = 0;
	for (const FCache& C : Caches) { N += C.bCollected ? 1 : 0; }
	return N;
}

int32 ATDSpaceField::GetCachesAlive() const
{
	int32 N = 0;
	for (const FCache& C : Caches) { N += C.Pickup.IsValid() ? 1 : 0; }
	return N;
}

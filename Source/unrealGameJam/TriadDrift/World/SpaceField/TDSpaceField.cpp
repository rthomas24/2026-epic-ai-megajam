// Triad Drift - the space field, galaxy layer. Owner: Teammate 5 (T4 reviews).

#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/SpaceField/TDSpaceFieldFX.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Misc/ScopeExit.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/ConstructorHelpers.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"
#include "TriadDrift/Combat/Pickups/TDPickupSubsystem.h"

namespace TDSpaceFieldImpl
{
	static int32 GEnabled = 1;
	static float GAnimHz = 30.f;
	static FAutoConsoleVariableRef CVarAnimHz(TEXT("td.SpaceField.AnimHz"), GAnimHz,
		TEXT("Tumbling rocks and drifting wrecks update this often (Hz; 0 = every frame)."));
	static float GAnimRange = 400000.f;
	static FAutoConsoleVariableRef CVarAnimRange(TEXT("td.SpaceField.AnimRange"), GAnimRange,
		TEXT("Only tumblers and wrecks within this distance of the player's camera move (cm; 0 = all). Slow turns farther away are invisible."));
	static FAutoConsoleVariableRef CVarSpaceField(TEXT("td.SpaceField"), GEnabled,
		TEXT("Space field (belts, clusters, debris fields, wrecks, buoys, and the layout's rocks made breakable): 1 on (default), 0 = the galaxy exactly as before the field. Read at level load."));

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
	// The effects builder's break systems, hard-referenced from the class defaults so the cook and asset traces see them
	// (they also sit in the always-cooked Gameplay/VFX folder). A DA_SpaceField_FX cue with a VFX still wins.
	static const TCHAR* BreakPaths[] = {
		TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_S.NS_TD_Asteroid_Break_S"),
		TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_M.NS_TD_Asteroid_Break_M"),
		TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_L.NS_TD_Asteroid_Break_L"),
		TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_Crystal.NS_TD_Asteroid_Break_Crystal") };
	for (const TCHAR* Path : BreakPaths)
	{
		ConstructorHelpers::FObjectFinderOptional<UNiagaraSystem> Found(Path);
		DefaultBreakVFX.Add(Found.Get());
	}
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
	// td.SpaceField 0 at load: never take over the layout's rocks, so the galaxy is exactly today's by construction.
	bAdoptAtLoad = bAdoptLayoutRocks && TDSpaceFieldImpl::GEnabled != 0;
	Build();
	FTDFieldDamage::Register(this);	// (DA_SpaceField_FX is warmed by T7's UTDWarmupSubsystem; the defaults register below)
	// Look the effects asset up now (T7's warm-up has streamed it): on the first break the lookup cost 26 ms.
	if (!FX) { FX = UTDStatics::LoadPresentation<UTDSpaceFieldFX>(TEXT("DA_SpaceField_FX")); }
	bFXLoaded = true;	// Lancer's ram and explosions find the field without searching the world
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
	SpecialRng.Initialize(Seed * 613 + 11);
	BreakRng.Initialize(Seed * 389 + 17);
	States.Reset();
	ColliderToPiece.Reset();
	FreeColliders.Reset();
	FreeVisuals.Reset();
	Grid.Reset();
	Broken = 0;

	// Hidden collision spheres for every piece (the visuals never collide): ships, shots and hyperdrive all hit them.
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
	if (UseComposedRegions())
	{
		// The seven regions of docs/eng/SPACE-DESIGN.md.
		BuildShepherdBelt(Rng);
		BuildWatchlineHalo(Rng);
		BuildQuietGraveyard(Rng);
		BuildIceDrift(Rng);
		BuildSurveyScar(Rng);
		BuildShatteredMoons(Rng);
		BuildOuterDark(Rng);
		PlaceRegionCentrepieces(Rng);
		BuildRegionDressing();
	}
	else
	{
		BuildBelts(Rng);	// the Nov 13 composition (td.SpaceField.Compose 0)
		BuildClusters(Rng);
	}
	BuildDebrisFields(Rng);
	BuildBuoys();
	if (bAdoptAtLoad) { AdoptLayoutRocks(); }
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
	if (FVector::Dist(P, Xf.TransformPosition(L.Dominion.GetLocation())) < (bMoonPlacement ? MoonDominionClear : DominionKeepOut) + Half)
	{
		return false;
	}
	if (const FTransform* Exit = L.ReturnPoints.Find(TDReturnPoints::DominionHangarExit))
	{
		if (FVector::Dist(P, Xf.TransformPosition(Exit->GetLocation())) < (bMoonPlacement ? MoonDominionClear : DominionKeepOut) + Half)
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

bool ATDSpaceField::AddPiece(UStaticMesh* Mesh, const FVector& At, const FRotator& Rot, float Size, ETDSpaceFieldKind Kind, bool bCollides, bool bTumble, bool bSpecial)
{
	if (!Mesh || !IsPlacementAllowed(At, Size, bCollides))
	{
		return false;
	}
	const FTransform PieceXf(Rot, At, FVector(Size * NormFor(Mesh)));
	UInstancedStaticMeshComponent* VisualComp = nullptr;
	int32 VisualInstance = INDEX_NONE;
	int32 TumblerIndex = INDEX_NONE;
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
		VisualComp = T;
		VisualInstance = T->AddInstance(PieceXf, true);
		TumblerIndex = Tumblers.Num();
		FTumbler Tu;
		Tu.Instance = VisualInstance;
		Tu.Comp = T;
		Tu.Home = PieceXf;
		Tu.Axis = FVector(TumbleRng.GetUnitVector());
		Tu.RadPerSec = FMath::DegreesToRadians(TumbleRng.FRandRange(1.5f, 5.f)) * (TumbleRng.FRand() < 0.5f ? -1.f : 1.f);
		Tumblers.Add(Tu);
	}
	else
	{
		VisualComp = ComponentFor(Mesh, Size);
		VisualInstance = VisualComp->AddInstance(PieceXf, true);
	}
	FTDSpaceFieldPiece Piece;
	Piece.Location = At;
	Piece.Size = Size;
	Piece.bCollides = bCollides;
	Piece.Kind = Kind;
	Pieces.Add(Piece);
	RegisterPiece(VisualComp, VisualInstance, Mesh, Size, bSpecial, TumblerIndex, INDEX_NONE);	// also its avoidance obstacle (10 m and up)
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
	// Separate streams pick the tumblers and the specials, so placement never changes with either.
	const bool bTumble = Size >= TumbleFromSize && Tumblers.Num() < MaxTumblers && TumbleRng.FRand() < 0.35f;
	// Specials: the crystal-veined, glowing family is kept for them alone (plain rocks are classic, ice or metal).
	const bool bSpecial = (Kind == ETDSpaceFieldKind::Belt || Kind == ETDSpaceFieldKind::Cluster) && Size >= MediumFromSize && SpecialOneIn > 0
		&& SpecialRng.RandRange(1, SpecialOneIn) == 1 && Families.IsValidIndex(3) && Families[3].Num() > 0;
	if (bSpecial)
	{
		Mesh = Families[3][SpecialRng.RandRange(0, Families[3].Num() - 1)];
	}
	return AddPiece(Mesh, At, Rot, Size, Kind, Size >= CollideFromSize, bTumble, bSpecial);
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
		const int32 FamA = B == 0 ? 0 : 2, FamB = B == 0 ? 1 : 0;	// crystal (3) is for the specials
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

void ATDSpaceField::BuildClusters(FRandomStream& Rng, int32 FirstShell)
{
	using namespace TDSpaceFieldImpl;
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);
	// Even directions (a Fibonacci sphere of exactly the count) so every bearing, up and down too, has something; a
	// direction blocked at one distance tries others.
	const int32 N = Clusters;
	for (int32 Shell = FirstShell; Shell < ClusterShells.Num(); ++Shell)
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
		const int32 Fam = Rng.RandRange(0, 2);
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
			Placed += AddRock(C + Off, PowerSize(Rng, 500.f, 12000.f, 8.f), ETDSpaceFieldKind::Cluster, Rng, Rng.FRand() < 0.7f ? Fam : Rng.RandRange(0, 2)) ? 1 : 0;
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
			RegisterPiece(W, 0, Hull, WreckSize, false, INDEX_NONE, WreckComponents.Num() - 1);
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
	if (bOn && bAdoptLayoutRocks && AdoptedLayoutComps.Num() == 0 && States.Num() > 0)
	{
		AdoptLayoutRocks();	// started with td.SpaceField 0, switched on later
	}
	ShowLayoutRocks(!bOn);	// off at runtime: show the layout's own rocks again (best effort; td.SpaceField 0 at load is the exact fallback)
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
	// td.SpaceField is read at level load only (Lead, Nov 14: a provable fallback). A change mid-game is reported, not
	// applied: 0 at load is the galaxy exactly as it was before the field, never a half-restored layout.
	if ((TDSpaceFieldImpl::GEnabled != 0) != bEnabled && !bWarnedToggle)
	{
		bWarnedToggle = true;
		UE_LOG(LogTemp, Warning, TEXT("td.SpaceField %d: applies at the next level load (the field stays %s until then)"), TDSpaceFieldImpl::GEnabled, bEnabled ? TEXT("on") : TEXT("off"));
		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, TEXT("td.SpaceField applies at the next level load")); }
	}
	PerfFrameBreakMsMax = FMath::Max(PerfFrameBreakMsMax, PerfFrameBreakMs);
	PerfFrameBreakMs = 0.0;
	for (UInstancedStaticMeshComponent* C : DirtyVisuals) { if (IsValid(C)) { C->MarkRenderStateDirty(); } }
	DirtyVisuals.Reset();
	if (!bEnabled)
	{
		return;
	}
	const double TickStart = FPlatformTime::Seconds();
	ON_SCOPE_EXIT
	{
		const double Ms = (FPlatformTime::Seconds() - TickStart) * 1000.0;
		PerfTickMsSum += Ms;
		PerfTickMsMax = FMath::Max(PerfTickMsMax, Ms);
		++PerfTickFrames;
	};
	Clock += DeltaSeconds;
	// Every frame (about 20 distance checks): the magnet pulls a taken cache off its spot within a few frames.
	UpdateCaches();
	if (IsDressingWanted() != bDressingShown && (RegionBeacons.Num() > 0 || RegionHazes.Num() > 0)) { SetRegionDressingVisible(IsDressingWanted()); }
	// Tumblers and wrecks: slow motion, so a throttled update near the camera only (nothing per frame for far rocks).
	AnimAccum += DeltaSeconds;
	const float Hz = TDSpaceFieldImpl::GAnimHz;
	if (Hz > 0.f && AnimAccum < 1.0 / Hz)
	{
		return;
	}
	AnimAccum = 0.0;
	UpdateTumblers();
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const bool bHaveEye = PC && PC->PlayerCameraManager;
	const FVector Eye = bHaveEye ? PC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	const float Range = TDSpaceFieldImpl::GAnimRange;
	// The wrecks drift round a 30 m loop (about 1.6 m/s) and turn slowly: they never leave their keep-out-checked spot.
	for (int32 i = 0; i < WreckComponents.Num(); ++i)
	{
		UInstancedStaticMeshComponent* W = WreckComponents[i];
		if (!W || !WreckHomes.IsValidIndex(i)) { continue; }
		if (bHaveEye && Range > 0.f && FVector::Dist(Eye, WreckHomes[i].GetLocation()) > Range) { continue; }
		const float Phase = Clock * (2.f * PI / 120.f) + i * 1.3f;
		FTransform T = WreckHomes[i];
		T.AddToTranslation(FVector(FMath::Cos(Phase), FMath::Sin(Phase), 0.f) * 3000.f);
		T.SetRotation(T.GetRotation() * FQuat(FVector(0.3f, 0.2f, 1.f).GetSafeNormal(), Clock * FMath::DegreesToRadians(1.5f + i * 0.4f)));
		W->UpdateInstanceTransform(0, T, true, true, true);
	}
}

void ATDSpaceField::UpdateTumblers()
{
	PerfAnimated = 0;
	if (Tumblers.Num() == 0)
	{
		return;
	}
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const bool bHaveEye = PC && PC->PlayerCameraManager;
	const FVector Eye = bHaveEye ? PC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	const float Range = TDSpaceFieldImpl::GAnimRange;
	TSet<UInstancedStaticMeshComponent*, DefaultKeyFuncs<UInstancedStaticMeshComponent*>, TInlineSetAllocator<16>> Touched;
	for (const FTumbler& T : Tumblers)
	{
		if (!T.Comp || T.Instance == INDEX_NONE) { continue; }
		if (bHaveEye && Range > 0.f && FVector::DistSquared(Eye, T.Home.GetLocation()) > FMath::Square(Range)) { continue; }
		FTransform X = T.Home;
		X.SetRotation(FQuat(T.Axis, Clock * T.RadPerSec) * T.Home.GetRotation());
		T.Comp->UpdateInstanceTransform(T.Instance, X, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		Touched.Add(T.Comp);
		++PerfAnimated;
	}
	for (UInstancedStaticMeshComponent* C : Touched) { C->MarkRenderStateDirty(); }
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

void ATDSpaceField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTDFieldDamage::Unregister(this);
	Super::EndPlay(EndPlayReason);
}

void ATDSpaceField::PerfReport(bool bReset)
{
	const double TickAvg = PerfTickFrames > 0 ? PerfTickMsSum / PerfTickFrames : 0.0;
	const double BreakAvg = PerfBreaks > 0 ? PerfBreakMsSum / PerfBreaks : 0.0;
	UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.Perf: tick avg %.3f ms max %.3f ms over %lld frames (animated %d of %d tumblers) | breaks %lld, avg %.3f ms, max %.3f ms, worst frame of breaks %.3f ms | pieces %d alive %d"),
		TickAvg, PerfTickMsMax, PerfTickFrames, PerfAnimated, Tumblers.Num(), PerfBreaks, BreakAvg, PerfBreakMsMax, PerfFrameBreakMsMax, Pieces.Num(), GetAliveCount());
	if (bReset)
	{
		PerfTickMsSum = PerfTickMsMax = 0.0; PerfTickFrames = 0;
		PerfBreakMsSum = PerfBreakMsMax = 0.0; PerfBreaks = 0;
		PerfFrameBreakMsMax = 0.0;
	}
}

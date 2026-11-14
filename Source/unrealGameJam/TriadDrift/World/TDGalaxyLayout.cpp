// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Waves/TDWaveDirector.h"

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDVerdanceApproach.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "TriadDrift/World/TDSkySphere.h"

namespace
{
	FTransform TDGalaxy_Facing(const FVector& Location, const FVector& LookAt)
	{
		return FTransform((LookAt - Location).Rotation(), Location);
	}

	FTDAsteroidFieldSpec TDGalaxy_Field(const FVector& Center, float Radius, int32 Count, int32 Seed, float Clear = 0.f, float MinSize = 600.f, float MaxSize = 3500.f)
	{
		FTDAsteroidFieldSpec F;
		F.Center = Center;
		F.Radius = Radius;
		F.Count = Count;
		F.Seed = Seed;
		F.ClearRadius = Clear;
		F.MinSize = MinSize;
		F.MaxSize = MaxSize;
		return F;
	}

	FTDGatePairSpec TDGalaxy_GatePair(FName IdA, const FVector& A, FName IdB, const FVector& B, const FVector& BHeadsToward, TArray<ETDGamePhase> Open = {})
	{
		FTDGatePairSpec G;
		G.IdA = IdA;
		G.IdB = IdB;
		G.A = TDGalaxy_Facing(A, B);					// fly into A heading toward B's side of the galaxy
		G.B = TDGalaxy_Facing(B, BHeadsToward);		// come out of B heading somewhere useful
		G.OpenPhases = MoveTemp(Open);
		return G;
	}
}

ATDGalaxyLayout::ATDGalaxyLayout()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Asteroids = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Asteroids"));
	Asteroids->SetupAttachment(GetRootComponent());
	Asteroids->SetCollisionProfileName(TEXT("BlockAll"));
	Asteroids->SetMobility(EComponentMobility::Movable);
	if (Sphere.Succeeded()) { Asteroids->SetStaticMesh(Sphere.Object); }

	// Engineering asteroid and debris meshes (Gameplay/Props/Space, always cooked), hard-referenced here.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Rock1(TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_01.SM_Asteroid_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Rock2(TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_02.SM_Asteroid_02"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Rock3(TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_03.SM_Asteroid_03"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Rock4(TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_04.SM_Asteroid_04"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Debris1(TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_SpaceDebris_01.SM_SpaceDebris_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Debris2(TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_SpaceDebris_02.SM_SpaceDebris_02"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Debris3(TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_SpaceDebris_03.SM_SpaceDebris_03"));
	// Rendering headroom (T1's 1080p profile): per-instance cull distances; debris skips shadows and ray tracing.
	// Movable like the root: a Static component under the movable root aborts its attach (32 "cannot attach ... which
	// is static" warnings) and only lined up with the collision because the layout sits at the origin. Nothing moves
	// these, so Movable costs no per-frame updates.
	auto MakeLook = [this](const FName Name, UStaticMesh* Mesh, float CullStart, float CullEnd, bool bDetail) -> UInstancedStaticMeshComponent*
	{
		// Plain instancing: the belt is ~155 rocks + debris in 7 components, too small for HISM's clusters to pay
		// off (measured: HISM raised draw calls 57 -> 73 for no frame-time gain).
		UInstancedStaticMeshComponent* Look = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Look->SetupAttachment(GetRootComponent());
		Look->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Look->SetGenerateOverlapEvents(false);
		Look->SetMobility(EComponentMobility::Movable);
		Look->SetStaticMesh(Mesh);
		Look->InstanceStartCullDistance = static_cast<int32>(CullStart);
		Look->InstanceEndCullDistance = static_cast<int32>(CullEnd);
		if (bDetail)
		{
			Look->SetCastShadow(false);
			Look->SetVisibleInRayTracing(false);
		}
		return Look;
	};
	{
		UStaticMesh* Rocks[] = { Rock1.Object, Rock2.Object, Rock3.Object, Rock4.Object };
		for (UStaticMesh* M : Rocks)
		{
			if (M)
			{
				AsteroidLooks.Add(MakeLook(*FString::Printf(TEXT("AsteroidLook%d"), AsteroidLooks.Num() + 1), M, 250000.f, 320000.f, false));
				AsteroidLookFamily.Add(0);
				AsteroidLookNorm.Add(1.f);	// authored around a 100 cm sphere
			}
		}
		// Space dressing kit rocks (real scale): normalised so their bounding sphere is 100 cm, like the classic rocks.
		struct FKitRock { const TCHAR* Path; uint8 Family; };
		static const FKitRock KitRocks[] = {
			{ TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Ice_01.SM_Ast_Ice_01"), 1 },
			{ TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Ice_02.SM_Ast_Ice_02"), 1 },
			{ TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Ice_03.SM_Ast_Ice_03"), 1 },
			{ TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Metal_01.SM_Ast_Metal_01"), 2 },
			{ TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Metal_02.SM_Ast_Metal_02"), 2 },
			{ TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Metal_03.SM_Ast_Metal_03"), 2 },
			{ TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Crystal_01.SM_Ast_Crystal_01"), 3 },
			{ TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Crystal_02.SM_Ast_Crystal_02"), 4 },
		};
		for (const FKitRock& K : KitRocks)
		{
			ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Found(K.Path);
			UStaticMesh* M = Found.Get();
			const float R = M ? M->GetBounds().SphereRadius : 0.f;
			if (M && R > 1.f)
			{
				AsteroidLooks.Add(MakeLook(*FString::Printf(TEXT("AsteroidLook%d"), AsteroidLooks.Num() + 1), M, 250000.f, 320000.f, false));
				AsteroidLookFamily.Add(K.Family);
				AsteroidLookNorm.Add(50.f / R);
			}
		}
		// Ruin art, debris clusters and landmarks: visual only, no collision.
		auto Kit = [&MakeLook](TArray<TObjectPtr<UInstancedStaticMeshComponent>>& Into, const TCHAR* Prefix, std::initializer_list<const TCHAR*> Paths, float CullStart, float CullEnd)
		{
			for (const TCHAR* P : Paths)
			{
				ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Found(P);
				if (UStaticMesh* M = Found.Get())
				{
					Into.Add(MakeLook(*FString::Printf(TEXT("%s%d"), Prefix, Into.Num() + 1), M, CullStart, CullEnd, true));
				}
			}
		};
		Kit(RuinArtLooks, TEXT("RuinArt"), {
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_StationTruss_01.SM_StationTruss_01"),	// girders
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_StationTruss_03.SM_StationTruss_03"),	// modules (hub)
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_StationTruss_02.SM_StationTruss_02") }, 250000.f, 320000.f);
		Kit(DebrisClusterLooks, TEXT("DebrisCluster"), {
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_DebrisCluster_01.SM_DebrisCluster_01"),
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_DebrisCluster_02.SM_DebrisCluster_02") }, 120000.f, 160000.f);
		Kit(SatelliteLooks, TEXT("Satellite"), {
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Derelict_Satellite_01.SM_Derelict_Satellite_01"),
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Derelict_Satellite_02.SM_Derelict_Satellite_02") }, 100000.f, 140000.f);
		Kit(CargoLooks, TEXT("CargoPod"), {
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_CargoPod_01.SM_CargoPod_01"),
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_CargoPod_02.SM_CargoPod_02"),
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_CargoPod_03.SM_CargoPod_03") }, 80000.f, 110000.f);
		Kit(LaneRockLooks, TEXT("LaneRock"), {
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Ice_03.SM_Ast_Ice_03"),
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Metal_02.SM_Ast_Metal_02"),
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Metal_03.SM_Ast_Metal_03"),
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_Ast_Crystal_02.SM_Ast_Crystal_02"),
			TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_02.SM_Asteroid_02"),
			TEXT("/Game/TriadDrift/Gameplay/Props/Space/SM_Asteroid_04.SM_Asteroid_04") }, 150000.f, 200000.f);
		Kit(BuoyLooks, TEXT("Buoy"), {
			TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_BeaconBuoy_01.SM_BeaconBuoy_01") }, 120000.f, 160000.f);
		UStaticMesh* Bits[] = { Debris1.Object, Debris2.Object, Debris3.Object };
		for (UStaticMesh* M : Bits)
		{
			if (M) { DebrisLooks.Add(MakeLook(*FString::Printf(TEXT("DebrisLook%d"), DebrisLooks.Num() + 1), M, 40000.f, 60000.f, true)); }
		}
	}

	RuinModules = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RuinModules"));
	RuinModules->SetupAttachment(GetRootComponent());
	RuinModules->SetCollisionProfileName(TEXT("BlockAll"));
	RuinModules->SetMobility(EComponentMobility::Movable);

	RuinBlocks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ruins"));
	RuinBlocks->SetupAttachment(GetRootComponent());
	RuinBlocks->SetCollisionProfileName(TEXT("BlockAll"));
	RuinBlocks->SetMobility(EComponentMobility::Movable);
	if (Cube.Succeeded()) { RuinBlocks->SetStaticMesh(Cube.Object); }

	Layout = MakeDefaultLayout();
}

// ------------------------------------------------------------------------------------ default data

FTDGalaxyLayoutData ATDGalaxyLayout::MakeDefaultLayout()
{
	FTDGalaxyLayoutData L;
	L.CombatZoneCenter = FVector::ZeroVector;
	L.CombatZoneRadius = 45000.f;
	L.BoundaryCenter = FVector(0.f, -50000.f, 0.f);
	L.BoundaryRadius = 900000.f;
	// 250 m out (clear of the ruin ring), turned to yaw 10: with the nebula sky unrotated the opening frame holds the
	// baked sun (yaw 35, pitch 18) on the right and the magenta nebula (yaw -18) on the left, the zone just left of centre.
	L.PlayerStart = FTransform(FRotator(0.f, 10.f, 0.f), FVector(-25000.f, 0.f, 0.f));

	// Combat zone: a belt with a clear dogfighting pocket, plus a denser flank field for cover.
	L.AsteroidFields.Add(TDGalaxy_Field(FVector::ZeroVector, 42000.f, 60, 11, /*Clear*/ 14000.f));
	L.AsteroidFields.Last().MetalShare = 0.35f;	// combat belt: dense metal cover rocks with a few crystal accents
	L.AsteroidFields.Last().CrystalShare = 0.08f;
	L.AsteroidFields.Add(TDGalaxy_Field(FVector(30000.f, -60000.f, 5000.f), 22000.f, 35, 23, 0.f, 800.f, 4500.f));
	L.AsteroidFields.Last().MetalShare = 0.3f;
	L.AsteroidFields.Add(TDGalaxy_Field(FVector(220000.f, 130000.f, 0.f), 30000.f, 30, 37));	// Verdance approach
	L.AsteroidFields.Last().IceShare = 0.45f;	// the cold side: ice with teal crystals
	L.AsteroidFields.Last().CrystalShare = 0.15f;
	L.AsteroidFields.Last().bTealCrystal = true;
	L.AsteroidFields.Add(TDGalaxy_Field(FVector(-210000.f, 170000.f, 20000.f), 30000.f, 30, 41, 0.f, 1000.f, 5000.f));	// Cinder approach
	L.AsteroidFields.Last().MetalShare = 0.3f;	// near Cinder: metal and violet crystal
	L.AsteroidFields.Last().CrystalShare = 0.3f;

	// Station ruins around the pocket: long girders and broken modules as cover.
	const float RuinRing = 22000.f;
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = (2.f * PI / 6.f) * i + 0.3f;
		FTDRuinSpec R;
		R.Location = FVector(FMath::Cos(A) * RuinRing, FMath::Sin(A) * RuinRing, (i % 2 ? 1.f : -1.f) * 2500.f);
		R.Rotation = FRotator(i * 7.f, FMath::RadiansToDegrees(A) + 90.f, i * 11.f);
		R.Extent = (i % 2) ? FVector(5000.f, 500.f, 500.f) : FVector(1800.f, 1500.f, 1200.f);
		R.bModule = (i % 2) == 0;	// girders on odd slots, modules on even
		L.Ruins.Add(R);
	}

	// Planets far out; their orbit return points sit between the planet and the combat zone.
	FTDPlanetSpec Verdance;
	Verdance.Destination = ETDDestination::Verdance;
	Verdance.Location = FVector(450000.f, 260000.f, 0.f);
	Verdance.PlanetRadius = 60000.f;
	Verdance.AtmosphereRadius = 72000.f;
	Verdance.ReturnPoint = TDReturnPoints::VerdanceOrbit;
	Verdance.Color = FLinearColor(0.1f, 0.9f, 0.55f);
	{
		const FString Dir = TEXT("/Game/TriadDrift/Gameplay/Props/Planets/");
		Verdance.Look.Surface = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Dir + TEXT("SM_Planet_Sphere.SM_Planet_Sphere")));
		Verdance.Look.SurfaceMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Dir + TEXT("MI_Planet_Verdance.MI_Planet_Verdance")));
		Verdance.Look.Atmosphere = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Dir + TEXT("SM_Planet_Atmosphere.SM_Planet_Atmosphere")));
		Verdance.Look.AtmosphereMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Dir + TEXT("MI_PlanetAtmo_Verdance.MI_PlanetAtmo_Verdance")));
		Verdance.Look.Ring = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Dir + TEXT("SM_Planet_Ring.SM_Planet_Ring")));
		Verdance.Look.RingMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Dir + TEXT("MI_PlanetRing_Verdance.MI_PlanetRing_Verdance")));
	}
	L.Planets.Add(Verdance);

	FTDPlanetSpec Cinder;
	Cinder.Destination = ETDDestination::Cinder;
	Cinder.Location = FVector(-420000.f, 330000.f, 40000.f);
	Cinder.PlanetRadius = 55000.f;
	Cinder.AtmosphereRadius = 66000.f;
	Cinder.ReturnPoint = TDReturnPoints::CinderOrbit;
	Cinder.Color = FLinearColor(0.9f, 0.3f, 0.05f);
	{
		const FString Dir = TEXT("/Game/TriadDrift/Gameplay/Props/Planets/");
		Cinder.Look.Surface = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Dir + TEXT("SM_Planet_Sphere.SM_Planet_Sphere")));
		Cinder.Look.SurfaceMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Dir + TEXT("MI_Planet_Cinder.MI_Planet_Cinder")));
		Cinder.Look.Atmosphere = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Dir + TEXT("SM_Planet_Atmosphere.SM_Planet_Atmosphere")));
		Cinder.Look.AtmosphereMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Dir + TEXT("MI_PlanetAtmo_Cinder.MI_PlanetAtmo_Cinder")));
	}
	L.Planets.Add(Cinder);

	for (const FTDPlanetSpec& P : L.Planets)
	{
		const FVector ToZone = (L.CombatZoneCenter - P.Location).GetSafeNormal();
		const FVector Orbit = P.Location + ToZone * (P.AtmosphereRadius + 20000.f);
		L.ReturnPoints.Add(P.ReturnPoint, TDGalaxy_Facing(Orbit, L.CombatZoneCenter));	// face home after take-off
	}

	// Dominion approach, far "south" of the combat zone, nose toward it.
	const FVector DominionLoc(0.f, -520000.f, 20000.f);
	L.Dominion = TDGalaxy_Facing(DominionLoc, L.CombatZoneCenter);
	const FVector DominionFwd = (L.CombatZoneCenter - DominionLoc).GetSafeNormal();
	L.ReturnPoints.Add(TDReturnPoints::DominionHangarExit, FTransform(DominionFwd.Rotation(), DominionLoc + DominionFwd * 45000.f));

	// Gates: combat zone <-> each orbit, and a flank pair that opens for waves 3-5.
	const FVector VerdanceOrbit = L.ReturnPoints[TDReturnPoints::VerdanceOrbit].GetLocation();
	const FVector CinderOrbit = L.ReturnPoints[TDReturnPoints::CinderOrbit].GetLocation();
	// Orbit gates sit OFF the line from the orbit return point to the combat zone (70 000 cm to the side): a ship
	// leaving orbit flies straight home and never takes the gate by accident (T1 / T5 playtests: it did, 3 s
	// after every return, and popped out near the centre). Taking the gate is a choice the HUD marker offers.
	auto OrbitGateAt = [&L](const FVector& Orbit)
	{
		const FVector Home = (L.CombatZoneCenter - Orbit).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Home).GetSafeNormal();
		return Orbit + Side * 70000.f + Home * 5000.f;	// clear of 10 s of cruise (60 000 cm) plus the disc
	};
	L.Gates.Add(TDGalaxy_GatePair(TEXT("Gate_Verdance_Zone"), FVector(30000.f, 18000.f, 0.f), TEXT("Gate_Verdance_Orbit"), OrbitGateAt(VerdanceOrbit), Verdance.Location));
	L.Gates.Add(TDGalaxy_GatePair(TEXT("Gate_Cinder_Zone"), FVector(-30000.f, 18000.f, 0.f), TEXT("Gate_Cinder_Orbit"), OrbitGateAt(CinderOrbit), Cinder.Location));
	L.Gates.Add(TDGalaxy_GatePair(TEXT("Gate_Flank_West"), FVector(-35000.f, -38000.f, 0.f), TEXT("Gate_Flank_East"), FVector(38000.f, 36000.f, 3000.f), L.CombatZoneCenter,
		{ ETDGamePhase::Waves3to4, ETDGamePhase::Wave5, ETDGamePhase::DominionAssault }));

	return L;
}

// ---------------------------------------------------------------------------------------- access

ATDGalaxyLayout* ATDGalaxyLayout::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ATDGalaxyLayout> It(World); It; ++It)
	{
		if (IsValid(*It)) { return *It; }
	}
	return nullptr;
}

ATDGalaxyLayout* ATDGalaxyLayout::SpawnDefaultGalaxy(const UObject* WorldContext)
{
	if (ATDGalaxyLayout* Existing = Get(WorldContext))
	{
		return Existing;
	}
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ATDGalaxyLayout>(ATDGalaxyLayout::StaticClass(), FTransform::Identity, Params);
}

bool ATDGalaxyLayout::FindReturnPoint(FName Name, FTransform& OutTransform) const
{
	if (Name.IsNone())
	{
		return false;
	}
	if (const FTransform* Found = Layout.ReturnPoints.Find(Name))
	{
		OutTransform = *Found * GetActorTransform();
		return true;
	}
	return false;
}

FTransform ATDGalaxyLayout::GetPlayerStart() const
{
	return Layout.PlayerStart * GetActorTransform();
}

TArray<ATDPlanetAtmosphere*> ATDGalaxyLayout::GetPlanets() const
{
	TArray<ATDPlanetAtmosphere*> Out;
	for (ATDPlanetAtmosphere* P : Planets) { if (P) { Out.Add(P); } }
	return Out;
}

TArray<AActor*> ATDGalaxyLayout::GetActiveMarkers() const
{
	TArray<AActor*> Out;
	for (ATDPlanetAtmosphere* P : Planets) { if (P && P->IsOpen()) { Out.Add(P); } }
	return Out;
}

ATDPortalGate* ATDGalaxyLayout::FindGate(FName GateId) const
{
	for (ATDPortalGate* G : GateActors)
	{
		if (G && G->GateId == GateId) { return G; }
	}
	return nullptr;
}

TArray<FTransform> ATDGalaxyLayout::GetSpawnPoints() const
{
	TArray<FTransform> Out;
	Out.Add(GetPlayerStart());
	for (const TPair<FName, FTransform>& KV : Layout.ReturnPoints) { Out.Add(KV.Value * GetActorTransform()); }
	return Out;
}

int32 ATDGalaxyLayout::GetAsteroidLookCount() const
{
	int32 N = 0;
	for (const UInstancedStaticMeshComponent* Look : AsteroidLooks) { N += Look ? Look->GetInstanceCount() : 0; }
	return N;
}

int32 ATDGalaxyLayout::GetAsteroidCount() const
{
	return Asteroids ? Asteroids->GetInstanceCount() : 0;
}

// ----------------------------------------------------------------------------------------- build

void ATDGalaxyLayout::BeginPlay()
{
	Super::BeginPlay();
	if (bBuildOnBeginPlay)
	{
		Build();
	}
}

void ATDGalaxyLayout::Build()
{
	if (bBuilt || !GetWorld())
	{
		return;
	}
	bBuilt = true;

	// One call: T1's travel subsystem hands the bounds to every ship movement component (new spawns
	// and respawns included). The world subsystem keeps a copy for AI and queries.
	const FVector BoundsCenter = GetActorTransform().TransformPosition(Layout.BoundaryCenter);
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->SetPlayBounds(BoundsCenter, Layout.BoundaryRadius);
	}
	if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
	{
		WorldSys->SetPlayBounds(BoundsCenter, Layout.BoundaryRadius);
	}

	if (!TravelFX)
	{
		TravelFX = UTDStatics::LoadPresentation<UTDTravelFX>(TEXT("DA_Travel_FX"));
	}
	BuildInstances(/*bRegisterObstacles*/ true);
	if (bSpawnSky)
	{
		SkySphere = ATDSkySphere::SpawnIfAvailable(GetWorld());
	}
	BuildPlanets();
	BuildGates();

	if (bSpawnDominion && !ATDDominion::Get(this))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		UClass* Cls = DominionClass ? DominionClass.Get() : ATDDominion::StaticClass();
		Dominion = GetWorld()->SpawnActor<ATDDominion>(Cls, Layout.Dominion * GetActorTransform(), Params);
	}
	else
	{
		Dominion = ATDDominion::Get(this);
	}
	if (Dominion)
	{
		// The hull is an obstacle for avoidance (string of spheres along its length).
		if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
		{
			const FVector E = Dominion->HullExtent;
			for (int32 i = -2; i <= 2; ++i)
			{
				WorldSys->AddObstacle(Dominion->GetActorTransform().TransformPosition(FVector(i * E.X * 0.4f, 0.f, 0.f)), FMath::Max(E.Y, E.Z) * 1.3f);
			}
		}
	}

	// Story (storyline 14-18, reduced): on the way to Verdance, Armada convoys ferry scanning gear from the Dominion's
	// field toward Verdance (one hauler scannable), and a transponder flickers off Verdance's orbit. Shown only in
	// Phase Verdance; spawned once here so they cost nothing the rest of the run.
	if (bSpawnVerdanceApproach && Dominion)
	{
		const FTDPlanetSpec* V = Layout.Planets.FindByPredicate([](const FTDPlanetSpec& P) { return P.Destination == ETDDestination::Verdance; });
		if (V)
		{
			const FTransform Xf = GetActorTransform();
			const FVector VLoc = Xf.TransformPosition(V->Location);
			const FVector From = Dominion->GetActorLocation();
			const FVector Dir = (VLoc - From).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
			const FVector A = From + Dir * 150000.f;							// out of the field
			const FVector B = VLoc - Dir * (V->AtmosphereRadius + 50000.f);		// short of the atmosphere
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector Offsets[] = { Side * 60000.f + FVector(0.f, 0.f, 8000.f), -Side * 90000.f - FVector(0.f, 0.f, 6000.f) };
			for (int32 i = 0; i < 2; ++i)
			{
				if (ATDArmadaConvoy* C = GetWorld()->SpawnActor<ATDArmadaConvoy>(ATDArmadaConvoy::StaticClass(), FTransform(A + Offsets[i]), P))
				{
					C->SetupLane(A + Offsets[i], B + Offsets[i], i == 0 ? 4 : 3, /*bScannable*/ i == 0);
					VerdanceApproach.Add(C);
				}
			}
			FTransform Orbit;
			const FVector BeaconAt = (FindReturnPoint(V->ReturnPoint, Orbit) ? Orbit.GetLocation() : VLoc) + Side * 30000.f + FVector(0.f, 0.f, 12000.f);
			if (ATDTransponderBeacon* T = GetWorld()->SpawnActor<ATDTransponderBeacon>(ATDTransponderBeacon::StaticClass(), FTransform(BeaconAt), P))
			{
				VerdanceApproach.Add(T);
			}
		}
	}

	// Hyperdrive drop-out at the Dominion approach (planets register their own wells).
	if (Dominion)
	{
		if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
		{
			Travel->RegisterGravityWell(TDGalaxyWells::Dominion, Dominion->GetActorLocation(), DominionWellRadius, Dominion);
		}
		Dominion->HyperdriveLockRadius = DominionWellRadius;	// the finale locks where jumps drop out
		{
		}
	}

	SetHidePlaceholderDressing(bHidePlaceholderDressing);
	UpdateGatePhases();
}

void ATDGalaxyLayout::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bBuilt)
	{
		if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
		{
			Travel->ClearPlayBounds();
			Travel->UnregisterGravityWell(TDGalaxyWells::Dominion);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ATDGalaxyLayout::SetHidePlaceholderDressing(bool bHide)
{
	bHidePlaceholderDressing = bHide;
	// Visibility only: collision, obstacles and gameplay stay exactly the same.
	const bool bRockLooks = AsteroidMesh.IsNull() && GetAsteroidLookCount() > 0;
	if (Asteroids) { Asteroids->SetVisibility(!bHide && !bRockLooks); }
	for (UInstancedStaticMeshComponent* Look : AsteroidLooks) { if (Look) { Look->SetVisibility(!bHide); } }
	for (UInstancedStaticMeshComponent* Look : DebrisLooks) { if (Look) { Look->SetVisibility(!bHide); } }
	if (RuinBlocks) { RuinBlocks->SetVisibility(!bHide && !UsesRuinArt()); }
	for (UInstancedStaticMeshComponent* C : GetDressingComponents()) { C->SetVisibility(!bHide); }
	for (ATDPlanetAtmosphere* P : Planets)
	{
		if (P) { P->SetPlaceholderVisible(!bHide); }
	}
}

float FTDJourneyKeyPoint::DistanceTo(const FVector& P) const
{
	if (Kind == ETDJourneyPointKind::Route && Length > 0.f && !Direction.IsNearlyZero())
	{
		return FMath::PointDistToSegment(P, Location, Location + Direction * Length) - Radius;
	}
	return FVector::Dist(P, Location) - Radius;
}

void ATDGalaxyLayout::GetJourneyKeyPoints(TArray<FTDJourneyKeyPoint>& Out) const
{
	Out.Reset();
	const FTransform Xf = GetActorTransform();
	auto Add = [&Out](FName Id, ETDJourneyPointKind Kind, const FVector& At, float Radius, const FVector& Dir = FVector::ZeroVector, float Length = 0.f)
	{
		FTDJourneyKeyPoint K;
		K.Id = Id; K.Kind = Kind; K.Location = At; K.Radius = Radius; K.Direction = Dir.GetSafeNormal(); K.Length = Length;
		Out.Add(K);
	};
	const FVector Zone = Xf.TransformPosition(Layout.CombatZoneCenter);
	Add(TEXT("CombatZone"), ETDJourneyPointKind::Pocket, Zone, Layout.CombatZoneRadius);
	// The player start is also the tutorial-skip start (T7's playtest: the squad landed in station dressing there).
	Add(TEXT("PlayerStart"), ETDJourneyPointKind::KeepOut, Xf.TransformPosition(Layout.PlayerStart.GetLocation()), AsteroidKeepOut * 2.f);
	for (const TPair<FName, FTransform>& RP : Layout.ReturnPoints)
	{
		Add(RP.Key, ETDJourneyPointKind::KeepOut, Xf.TransformPosition(RP.Value.GetLocation()), AsteroidKeepOut);
	}
	for (const FTDGatePairSpec& G : Layout.Gates)
	{
		Add(G.IdA, ETDJourneyPointKind::KeepOut, Xf.TransformPosition(G.A.GetLocation()), G.Radius + 5000.f);
		Add(G.IdB, ETDJourneyPointKind::KeepOut, Xf.TransformPosition(G.B.GetLocation()), G.Radius + 5000.f);
	}
	for (const ATDPlanetAtmosphere* P : GetPlanets())
	{
		if (!P) { continue; }
		const FName Name = P->GetDestination() == ETDDestination::Verdance ? FName(TEXT("Verdance")) : FName(TEXT("Cinder"));
		Add(Name, ETDJourneyPointKind::KeepOut, P->GetPlanetCenter(), P->GetAtmosphereRadius() + 30000.f);
		// The approach from the combat zone to the atmosphere, 400 m wide.
		const FVector To = P->GetPlanetCenter() - Zone;
		const float Len = FMath::Max(0.f, (float)To.Size() - P->GetAtmosphereRadius());
		Add(FName(*(Name.ToString() + TEXT("_Approach"))), ETDJourneyPointKind::Route, Zone, 20000.f, To, Len);
		if (P->GetDestination() == ETDDestination::Verdance)
		{
			// Wave 2's ordered withdrawal flies this way (ATDWaveDirector::BeginWithdrawal, up to 2.5 km).
			Add(TEXT("TutorialEscape"), ETDJourneyPointKind::Route, Zone, 30000.f, To, FMath::Min(Len, 300000.f));
		}
	}
	for (const AActor* A : VerdanceApproach)
	{
		if (A) { Add(FName(*A->GetName()), ETDJourneyPointKind::KeepOut, A->GetActorLocation(), 15000.f); }	// convoy lanes and the transponder
	}
	if (const ATDDominion* D = Dominion.Get())
	{
		const FVector DL = D->GetActorLocation();
		Add(TEXT("Dominion"), ETDJourneyPointKind::KeepOut, DL, D->GetLocalHullBounds().GetExtent().Size() + 10000.f);
		Add(TEXT("DominionKillZone"), ETDJourneyPointKind::Pocket, DL, D->ShieldRadius * 3.f);
		const FVector ToDom = DL - Zone;
		Add(TEXT("DominionApproach"), ETDJourneyPointKind::Route, Zone, 30000.f, ToDom, FMath::Max(0.f, (float)ToDom.Size() - D->ShieldRadius * 3.f));
		// The escape flight leaves the bow (the bay mouth) and flies out to the clear line.
		const FTransform Esc = D->GetEscapeStart();
		Add(TEXT("EscapeCorridor"), ETDJourneyPointKind::Route, Esc.GetLocation(), 30000.f, Esc.GetRotation().GetForwardVector(), D->GetEscapeExitRadius());
	}
}

TArray<FTDRuinSpec> ATDGalaxyLayout::GetPlacedRuins() const
{
	// A ruin may never sit on the player start (also the tutorial-skip start: T7's playtest, Nov 14, the squad landed in a
	// girder 80 m away). One that does turns about the combat zone's up axis, 30 degrees at a time, until it is clear.
	TArray<FTDRuinSpec> Out = Layout.Ruins;
	const FVector Start = Layout.PlayerStart.GetLocation();
	for (FTDRuinSpec& R : Out)
	{
		for (int32 Step = 0; Step < 11; ++Step)
		{
			const float Need = R.Extent.Size() * 2.5f + AsteroidKeepOut * 2.f;	// the art and its panels reach past the box
			if (FVector::Dist(R.Location, Start) >= Need) { break; }
			const FVector Rel = R.Location - Layout.CombatZoneCenter;
			R.Location = Layout.CombatZoneCenter + FRotator(0.f, 30.f, 0.f).RotateVector(Rel);
			R.Rotation.Yaw += 30.f;
		}
	}
	return Out;
}

bool ATDGalaxyLayout::IsClearOfJourney(const FVector& WorldPoint, float Margin) const
{
	TArray<FTDJourneyKeyPoint> Points;
	GetJourneyKeyPoints(Points);
	for (const FTDJourneyKeyPoint& K : Points)
	{
		if (K.Kind != ETDJourneyPointKind::Pocket && K.DistanceTo(WorldPoint) < Margin) { return false; }
	}
	return true;
}

bool ATDGalaxyLayout::IsInKeepOut(const FVector& LocalPoint, float Margin) const
{
	const float R = AsteroidKeepOut + Margin;
	auto Near = [&](const FVector& P) { return FVector::DistSquared(P, LocalPoint) <= FMath::Square(R); };
	if (Near(Layout.PlayerStart.GetLocation())) { return true; }
	for (const TPair<FName, FTransform>& RP : Layout.ReturnPoints)
	{
		if (Near(RP.Value.GetLocation())) { return true; }
	}
	for (const FTDGatePairSpec& G : Layout.Gates)
	{
		if (Near(G.A.GetLocation()) || Near(G.B.GetLocation())) { return true; }
	}
	return false;
}

void ATDGalaxyLayout::BuildInstances(bool bRegisterObstacles)
{
	// Idempotent: the editor preview and BeginPlay both build; never stack instances.
	Asteroids->ClearInstances();
	RuinBlocks->ClearInstances();
	if (RuinModules) { RuinModules->ClearInstances(); }
	UStaticMesh* ModuleArt = !RuinModuleMesh.IsNull() ? RuinModuleMesh.LoadSynchronous() : nullptr;
	if (ModuleArt && RuinModules) { RuinModules->SetStaticMesh(ModuleArt); }
	for (UInstancedStaticMeshComponent* Look : AsteroidLooks) { if (Look) { Look->ClearInstances(); } }
	for (UInstancedStaticMeshComponent* Look : DebrisLooks) { if (Look) { Look->ClearInstances(); } }
	bool bNickRock = false;
	if (!AsteroidMesh.IsNull()) { if (UStaticMesh* Art = AsteroidMesh.LoadSynchronous()) { Asteroids->SetStaticMesh(Art); bNickRock = true; } }
	// Nick's presentation mesh wins; otherwise the engineering rocks carry the look and the spheres only collide.
	const bool bRockLooks = !bNickRock && AsteroidLooks.Num() > 0;
	if (!RuinMesh.IsNull()) { if (UStaticMesh* Art = RuinMesh.LoadSynchronous()) { RuinBlocks->SetStaticMesh(Art); } }

	UTDWorldSubsystem* WorldSys = bRegisterObstacles ? UTDWorldSubsystem::Get(this) : nullptr;
	const FTransform Xf = GetActorTransform();
	TArray<FSphere> Placed;
	for (const FTDAsteroidFieldSpec& F : Layout.AsteroidFields)
	{
		FRandomStream Rng(F.Seed);
		FRandomStream LookRng(F.Seed * 31 + 7);	// separate stream: placement (and the count) never change
		int32 Count = 0;
		for (int32 Attempt = 0; Count < F.Count && Attempt < F.Count * 8; ++Attempt)
		{
			FVector Offset = Rng.GetUnitVector() * F.Radius * FMath::Pow(Rng.FRand(), 0.6f);
			Offset.Z *= F.Flatten;
			if (Offset.Size() < F.ClearRadius)
			{
				continue;
			}
			const float Size = FMath::Lerp(F.MinSize, F.MaxSize, FMath::Square(Rng.FRand()));	// mostly small, a few big
			const float Radius = Size * 0.5f;
			const FVector Local = F.Center + Offset;
			if (IsInKeepOut(Local, Radius))
			{
				continue;	// gates, return points and the player start stay clear
			}
			bool bOverlap = false;
			for (const FSphere& S : Placed) { if (FVector::DistSquared(S.Center, Local) < FMath::Square(S.W + Radius)) { bOverlap = true; break; } }
			if (bOverlap) { continue; }
			// Uniform scale: the visible rock matches its sphere collision exactly.
			const FTransform RockXf(FRotator(Rng.FRandRange(-180, 180), Rng.FRandRange(-180, 180), Rng.FRandRange(-180, 180)), Local, FVector(Size / 100.f));
			Asteroids->AddInstance(RockXf);
			if (bRockLooks)
			{
				// Same 100 cm bounding sphere as the collision sphere: the rock never pokes out of its collision.
				const int32 L = PickRockLook(F, LookRng);
				AsteroidLooks[L]->AddInstance(
					FTransform(FRotator(LookRng.FRandRange(-180, 180), LookRng.FRandRange(-180, 180), LookRng.FRandRange(-180, 180)), Local,
						RockXf.GetScale3D() * (AsteroidLookNorm.IsValidIndex(L) ? AsteroidLookNorm[L] : 1.f)));
			}
			Placed.Add(FSphere(Local, Radius));
			if (WorldSys) { WorldSys->AddObstacle(Xf.TransformPosition(Local), Radius); }
			++Count;
		}
	}
	// Debris: sparse, small, between the rocks; visual only.
	if (bRockLooks && DebrisLooks.Num() > 0)
	{
		for (const FTDAsteroidFieldSpec& F : Layout.AsteroidFields)
		{
			FRandomStream DRng(F.Seed * 131 + 3);
			const int32 Want = FMath::RoundToInt(F.Count * DebrisPerAsteroid);
			int32 Count = 0;
			for (int32 Attempt = 0; Count < Want && Attempt < Want * 6; ++Attempt)
			{
				FVector Offset = DRng.GetUnitVector() * F.Radius * FMath::Pow(DRng.FRand(), 0.5f);
				Offset.Z *= F.Flatten;
				const FVector Local = F.Center + Offset;
				const float Size = DRng.FRandRange(150.f, 600.f);
				bool bInside = IsInKeepOut(Local, Size);
				for (const FSphere& S : Placed)
				{
					if (bInside) { break; }
					bInside = FVector::DistSquared(S.Center, Local) < FMath::Square(S.W + Size);
				}
				if (bInside) { continue; }
				DebrisLooks[DRng.RandHelper(DebrisLooks.Num())]->AddInstance(
					FTransform(FRotator(DRng.FRandRange(-180, 180), DRng.FRandRange(-180, 180), DRng.FRandRange(-180, 180)), Local, FVector(Size / 100.f)));
				++Count;
			}
		}
	}
	// The spheres are pure collision when the rocks carry the look.
	Asteroids->SetVisibility(!bRockLooks && !bHidePlaceholderDressing);
	if (AsteroidMesh.IsNull() && !bRockLooks)
	{
		UTDStatics::TintPlaceholder(Asteroids, FLinearColor(0.25f, 0.2f, 0.3f), 0.f);
	}

	for (const FTDRuinSpec& R : GetPlacedRuins())
	{
		if (R.bModule && ModuleArt && RuinModules)
		{
			RuinModules->AddInstance(FTransform(R.Rotation, R.Location, R.Extent * 2.f / 100.f));	// same convention as RuinMesh
		}
		else
		{
			RuinBlocks->AddInstance(FTransform(R.Rotation, R.Location, R.Extent * 2.f / 100.f));
		}
		if (WorldSys) { WorldSys->AddObstacle(Xf.TransformPosition(R.Location), R.Extent.GetMax()); }
	}
	if (RuinMesh.IsNull())
	{
		UTDStatics::TintPlaceholder(RuinBlocks, FLinearColor(0.35f, 0.35f, 0.4f), 0.f);
	}
	BuildDressing();
}

bool ATDGalaxyLayout::UsesRuinArt() const
{
	return bSpaceDressing && RuinMesh.IsNull() && RuinArtLooks.Num() > 0;
}

TArray<UInstancedStaticMeshComponent*> ATDGalaxyLayout::GetDressingComponents() const
{
	TArray<UInstancedStaticMeshComponent*> Out;
	for (const TArray<TObjectPtr<UInstancedStaticMeshComponent>>* Group : { &RuinArtLooks, &DebrisClusterLooks, &SatelliteLooks, &CargoLooks, &BuoyLooks, &LaneRockLooks })
	{
		for (UInstancedStaticMeshComponent* C : *Group) { if (C) { Out.Add(C); } }
	}
	return Out;
}

void ATDGalaxyLayout::BuildDressing()
{
	for (UInstancedStaticMeshComponent* C : GetDressingComponents()) { C->ClearInstances(); }
	const bool bRuinArt = UsesRuinArt();
	// The cube ruins stay as collision; with art over them they stop drawing.
	if (RuinBlocks) { RuinBlocks->SetVisibility(!bRuinArt && !bHidePlaceholderDressing); }
	if (!bSpaceDressing)
	{
		return;
	}
	auto Bounds = [](const UInstancedStaticMeshComponent* C) { return C && C->GetStaticMesh() ? C->GetStaticMesh()->GetBounds().BoxExtent : FVector(100.f); };
	auto RandRot = [](FRandomStream& R) { return FRotator(R.FRandRange(-180, 180), R.FRandRange(-180, 180), R.FRandRange(-180, 180)); };

	// 1) Ruins: truss art over each collision box, its long axis on the box's long axis, uniformly scaled to the box's
	//    length; 2-3 debris clusters drifting around it.
	FRandomStream RuinRng(9137);
	for (const FTDRuinSpec& R : GetPlacedRuins())
	{
		if (bRuinArt)
		{
			UInstancedStaticMeshComponent* Art = RuinArtLooks[R.bModule && RuinArtLooks.Num() > 1 ? 1 : 0];
			const FVector B = Bounds(Art);
			// The hub (module) is authored longest along Y: turn it so its long axis lies on the box's X.
			const bool bTurn = B.Y > B.X;
			const float Long = FMath::Max(B.X, B.Y);
			const float S = FMath::Max(0.05f, R.Extent.X / FMath::Max(1.f, Long));
			const FQuat Rot = R.Rotation.Quaternion() * (bTurn ? FRotator(0.f, 90.f, 0.f).Quaternion() : FQuat::Identity);
			Art->AddInstance(FTransform(Rot, R.Location, FVector(S)));
			if (!R.bModule && RuinArtLooks.Num() > 2 && RuinRng.FRand() < 0.5f)
			{
				// A torn hull panel hanging off some girders.
				UInstancedStaticMeshComponent* Panel = RuinArtLooks[2];
				const FVector Off = R.Rotation.RotateVector(FVector(RuinRng.FRandRange(-0.6f, 0.6f) * R.Extent.X, R.Extent.Y + 600.f, 0.f));
				Panel->AddInstance(FTransform(RandRot(RuinRng), R.Location + Off, FVector(RuinRng.FRandRange(0.8f, 1.2f))));
			}
		}
		if (DebrisClusterLooks.Num() > 0)
		{
			const int32 N = 2 + RuinRng.RandHelper(2);
			for (int32 i = 0; i < N; ++i)
			{
				const FVector Off = RuinRng.GetUnitVector() * (R.Extent.GetMax() + RuinRng.FRandRange(1500.f, 4000.f));
				const FVector P = R.Location + Off;
				if (IsInKeepOut(P, 2500.f)) { continue; }
				DebrisClusterLooks[RuinRng.RandHelper(DebrisClusterLooks.Num())]->AddInstance(FTransform(RandRot(RuinRng), P, FVector(RuinRng.FRandRange(0.7f, 1.4f))));
			}
		}
	}

	// 2) Landmarks, sparse, never in the dogfight pocket.
	const float Pocket = Layout.CombatZoneRadius;
	auto Clear = [this, Pocket](const FVector& P, float R) { return !IsInKeepOut(P, R) && FVector::Dist(P, Layout.CombatZoneCenter) > Pocket; };
	FRandomStream MarkRng(5521);
	// Satellites: a pair in high orbit around each planet, on the far side from the orbit return point's lane.
	if (SatelliteLooks.Num() > 0)
	{
		for (const FTDPlanetSpec& Pl : Layout.Planets)
		{
			const FVector ToZone = (Layout.CombatZoneCenter - Pl.Location).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(ToZone, FVector::UpVector).GetSafeNormal();
			for (int32 i = 0; i < 2; ++i)
			{
				const FVector Dir = (ToZone * 0.5f + Side * (i == 0 ? 1.f : -1.f) + FVector(0, 0, 0.35f)).GetSafeNormal();
				const FVector P = Pl.Location + Dir * Pl.AtmosphereRadius * 1.25f;
				if (!Clear(P, 2000.f)) { continue; }
				SatelliteLooks[i % SatelliteLooks.Num()]->AddInstance(FTransform(RandRot(MarkRng), P, FVector(MarkRng.FRandRange(0.9f, 1.2f))));
			}
		}
	}
	// Cargo pods: a spilled cluster at a third and two thirds of each lane from the zone to a planet / the Dominion.
	if (CargoLooks.Num() > 0)
	{
		TArray<FVector> LaneEnds;
		for (const FTDPlanetSpec& Pl : Layout.Planets) { LaneEnds.Add(Pl.Location); }
		LaneEnds.Add(Layout.Dominion.GetLocation());
		for (const FVector& End : LaneEnds)
		{
			const FVector Dir = (End - Layout.CombatZoneCenter).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
			for (const float T : { 0.33f, 0.66f })
			{
				const FVector Centre = FMath::Lerp(Layout.CombatZoneCenter, End, T) + Side * 6000.f;
				const int32 N = 4 + MarkRng.RandHelper(5);
				for (int32 i = 0; i < N; ++i)
				{
					const FVector P = Centre + MarkRng.GetUnitVector() * MarkRng.FRandRange(300.f, 2500.f);
					if (!Clear(P, 1000.f)) { continue; }
					CargoLooks[MarkRng.RandHelper(CargoLooks.Num())]->AddInstance(FTransform(RandRot(MarkRng), P, FVector(MarkRng.FRandRange(0.8f, 1.2f))));
				}
			}
		}
	}
	// Lane rocks: a hollow tube of small rocks and debris along every lane out of the combat zone, so travel never
	// crosses empty space; the lane's centre line, the pocket, gates and spawn points stay clear.
	if (LaneRockLooks.Num() > 0 && LaneRocksPerLane > 0)
	{
		TArray<FVector> LaneEnds;
		for (const FTDPlanetSpec& Pl : Layout.Planets) { LaneEnds.Add(Pl.Location - (Pl.Location - Layout.CombatZoneCenter).GetSafeNormal() * Pl.AtmosphereRadius * 1.6f); }
		LaneEnds.Add(Layout.Dominion.GetLocation() - (Layout.Dominion.GetLocation() - Layout.CombatZoneCenter).GetSafeNormal() * 80000.f);
		for (const FTDGatePairSpec& G : Layout.Gates)
		{
			LaneEnds.Add(G.A.GetLocation());
			LaneEnds.Add(G.B.GetLocation());
		}
		FRandomStream LaneRng(4271);
		for (const FVector& End : LaneEnds)
		{
			const FVector Dir = (End - Layout.CombatZoneCenter).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
			const FVector Up = FVector::CrossProduct(Side, Dir).GetSafeNormal();
			int32 Placed = 0;
			for (int32 Attempt = 0; Placed < LaneRocksPerLane && Attempt < LaneRocksPerLane * 4; ++Attempt)
			{
				const float T = LaneRng.FRandRange(0.f, 1.f);
				const float Ang = LaneRng.FRandRange(0.f, 2.f * PI);
				const float Rad = LaneClearRadius + LaneRng.FRandRange(0.f, 22000.f) * FMath::Sqrt(LaneRng.FRand());
				const FVector P = FMath::Lerp(Layout.CombatZoneCenter, End, T) + (Side * FMath::Cos(Ang) + Up * FMath::Sin(Ang) * 0.5f) * Rad;
				if (!Clear(P, 1500.f)) { continue; }
				// Mostly small (3-12 m), a few bigger (to 25 m); the kit rocks are real scale, so size by bounds.
				UInstancedStaticMeshComponent* Look = LaneRockLooks[LaneRng.RandHelper(LaneRockLooks.Num())];
				const float Want = FMath::Lerp(300.f, 2500.f, FMath::Pow(LaneRng.FRand(), 3.f));
				const float R = FMath::Max(1.f, Look->GetStaticMesh() ? Look->GetStaticMesh()->GetBounds().SphereRadius : 50.f);
				Look->AddInstance(FTransform(RandRot(LaneRng), P, FVector(Want * 0.5f / R)));
				++Placed;
			}
		}
	}
	// Buoys: lining both sides of every gate approach, 300 m apart, out to 900 m.
	if (BuoyLooks.Num() > 0)
	{
		for (const FTDGatePairSpec& G : Layout.Gates)
		{
			for (const FTransform& End : { G.A, G.B })
			{
				for (const float Along : { -90000.f, -60000.f, -30000.f, 30000.f, 60000.f, 90000.f })
				{
					for (const float Side : { -1.f, 1.f })
					{
						const FVector P = End.TransformPosition(FVector(Along, Side * (G.Radius + 4000.f), 0.f));
						if (!Clear(P, 500.f)) { continue; }
						BuoyLooks[0]->AddInstance(FTransform(End.GetRotation(), P, FVector(1.f)));
					}
				}
			}
		}
	}
}

int32 ATDGalaxyLayout::PickRockLook(const FTDAsteroidFieldSpec& F, FRandomStream& Rng) const
{
	// Family by the field's shares, then any look of that family (classic when the family has none).
	const float Roll = Rng.FRand();
	uint8 Family = 0;
	if (bSpaceDressing)
	{
		if (Roll < F.IceShare) { Family = 1; }
		else if (Roll < F.IceShare + F.MetalShare) { Family = 2; }
		else if (Roll < F.IceShare + F.MetalShare + F.CrystalShare) { Family = F.bTealCrystal ? 4 : 3; }
	}
	TArray<int32, TInlineAllocator<8>> Pool;
	for (int32 i = 0; i < AsteroidLooks.Num(); ++i)
	{
		if (AsteroidLookFamily.IsValidIndex(i) && AsteroidLookFamily[i] == Family) { Pool.Add(i); }
	}
	if (Pool.Num() == 0)
	{
		for (int32 i = 0; i < AsteroidLooks.Num(); ++i) { if (!AsteroidLookFamily.IsValidIndex(i) || AsteroidLookFamily[i] == 0) { Pool.Add(i); } }
	}
	return Pool.Num() > 0 ? Pool[Rng.RandHelper(Pool.Num())] : Rng.RandHelper(AsteroidLooks.Num());
}

void ATDGalaxyLayout::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;	// at runtime BeginPlay builds everything (with obstacles and actors)
	}
	// Editor: show the belt and ruins for real, and sketch the rest, so the map can be dressed around it.
	BuildInstances(/*bRegisterObstacles*/ false);
	DrawEditorPreview();
}

void ATDGalaxyLayout::DrawEditorPreview() const
{
#if WITH_EDITOR && ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld() || !bEditorPreview)
	{
		return;
	}
	FlushPersistentDebugLines(World);
	const FTransform Xf = GetActorTransform();
	for (const FTDPlanetSpec& P : Layout.Planets)
	{
		const FVector C = Xf.TransformPosition(P.Location);
		DrawDebugSphere(World, C, P.PlanetRadius, 24, P.Color.ToFColor(true), true, -1.f, 0, 200.f);
		DrawDebugSphere(World, C, P.AtmosphereRadius, 24, FColor::Cyan, true, -1.f, 0, 60.f);
	}
	for (const FTDGatePairSpec& G : Layout.Gates)
	{
		const FTransform A = G.A * Xf;
		const FTransform B = G.B * Xf;
		DrawDebugCircle(World, A.GetLocation(), G.Radius, 32, FColor::Magenta, true, -1.f, 0, 120.f, A.GetUnitAxis(EAxis::Y), A.GetUnitAxis(EAxis::Z), false);
		DrawDebugCircle(World, B.GetLocation(), G.Radius, 32, FColor::Magenta, true, -1.f, 0, 120.f, B.GetUnitAxis(EAxis::Y), B.GetUnitAxis(EAxis::Z), false);
		DrawDebugLine(World, A.GetLocation(), B.GetLocation(), FColor(255, 0, 255, 60), true, -1.f, 0, 20.f);
	}
	const FTransform D = Layout.Dominion * Xf;
	DrawDebugBox(World, D.GetLocation(), FVector(30000.f, 8000.f, 5000.f), D.GetRotation(), FColor::Red, true, -1.f, 0, 300.f);
	for (const TPair<FName, FTransform>& RP : Layout.ReturnPoints)
	{
		const FTransform T = RP.Value * Xf;
		DrawDebugDirectionalArrow(World, T.GetLocation(), T.GetLocation() + T.GetUnitAxis(EAxis::X) * 8000.f, 2000.f, FColor::Green, true, -1.f, 0, 150.f);
		DrawDebugString(World, T.GetLocation() + FVector(0, 0, 2000.f), RP.Key.ToString(), nullptr, FColor::Green, -1.f);
	}
	const FTransform PS = Layout.PlayerStart * Xf;
	DrawDebugDirectionalArrow(World, PS.GetLocation(), PS.GetLocation() + PS.GetUnitAxis(EAxis::X) * 8000.f, 2000.f, FColor::White, true, -1.f, 0, 150.f);
	DrawDebugSphere(World, Xf.TransformPosition(Layout.BoundaryCenter), Layout.BoundaryRadius, 48, FColor(255, 255, 255, 40), true, -1.f, 0, 400.f);
#endif
}

void ATDGalaxyLayout::BuildPlanets()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FTDPlanetSpec& P : Layout.Planets)
	{
		const FTransform T(FRotator::ZeroRotator, GetActorTransform().TransformPosition(P.Location));
		UClass* Cls = PlanetClass ? PlanetClass.Get() : ATDPlanetAtmosphere::StaticClass();
		ATDPlanetAtmosphere* Planet = GetWorld()->SpawnActorDeferred<ATDPlanetAtmosphere>(Cls, T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Planet) { continue; }
		Planet->Configure(P.Destination, P.ReturnPoint, P.PlanetRadius, P.AtmosphereRadius, P.Color);
		Planet->Look = P.Look;	// optional art; collision stays the sphere
		Planet->FinishSpawning(T);
		Planets.Add(Planet);
	}
}

void ATDGalaxyLayout::BuildGates()
{
	for (int32 i = 0; i < Layout.Gates.Num(); ++i)
	{
		const FTDGatePairSpec& G = Layout.Gates[i];
		const FTransform TA = G.A * GetActorTransform();
		const FTransform TB = G.B * GetActorTransform();
		UClass* Cls = GateClass ? GateClass.Get() : ATDPortalGate::StaticClass();
		ATDPortalGate* A = GetWorld()->SpawnActorDeferred<ATDPortalGate>(Cls, TA, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		ATDPortalGate* B = GetWorld()->SpawnActorDeferred<ATDPortalGate>(Cls, TB, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!A || !B) { continue; }
		for (ATDPortalGate* Gate : { A, B })
		{
			if (TravelFX) { Gate->TravelFX = TravelFX; }
			if (!GateFrameMesh.IsNull()) { Gate->FrameMeshOverride = GateFrameMesh; }
		}
		A->GateId = G.IdA;
		B->GateId = G.IdB;
		// Enemies may use only the in-zone flank pair; planet gates would carry them out of the fight.
		const bool bFlank = G.IdA.ToString().StartsWith(TEXT("Gate_Flank"));
		A->bTransitEnemyShips = bFlank;
		B->bTransitEnemyShips = bFlank;
		A->Radius = B->Radius = G.Radius;
		A->FinishSpawning(TA);
		B->FinishSpawning(TB);
		ATDPortalGate::LinkGates(A, B);
		GateActors.Add(A); GatePairIndex.Add(i);
		GateActors.Add(B); GatePairIndex.Add(i);
	}
}

// ------------------------------------------------------------------------------------------ tick

void ATDGalaxyLayout::UpdateGatePhases()
{
	ITDGameFlow* Flow = UTDStatics::GetGameFlow(this);
	const ETDGamePhase Phase = Flow ? Flow->GetProgress().Phase : ETDGamePhase::Waves1to2;
	// Second review MAJOR B: no leaving a fight through a planet gate the enemies can't follow (the flank pair stays).
	const ATDWaveDirector* WaveDirector = ATDWaveDirector::Get(this);
	const bool bWaveRunning = WaveDirector && WaveDirector->IsWaveRunning();
	for (int32 i = 0; i < GateActors.Num(); ++i)
	{
		ATDPortalGate* G = GateActors[i];
		if (!G || !Layout.Gates.IsValidIndex(GatePairIndex[i])) { continue; }
		const TArray<ETDGamePhase>& Open = Layout.Gates[GatePairIndex[i]].OpenPhases;
		const FString PairId = Layout.Gates[GatePairIndex[i]].IdA.ToString();
		const bool bPlanetGate = PairId.Contains(TEXT("Verdance")) || PairId.Contains(TEXT("Cinder"));
		const bool bShouldOpen = (Open.Num() == 0 || Open.Contains(Phase)) && !(bPlanetGate && bWaveRunning);
		if (G->bActive != bShouldOpen)
		{
			G->SetGateActive(bShouldOpen);
		}
	}
}

void ATDGalaxyLayout::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBuilt)
	{
		return;
	}
	PhaseTimer -= DeltaSeconds;
	if (PhaseTimer <= 0.f)
	{
		PhaseTimer = 1.f;
		UpdateGatePhases();
	}
}

// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDGalaxyLayout.h"

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FTransform Facing(const FVector& Location, const FVector& LookAt)
	{
		return FTransform((LookAt - Location).Rotation(), Location);
	}

	FTDAsteroidFieldSpec Field(const FVector& Center, float Radius, int32 Count, int32 Seed, float Clear = 0.f, float MinSize = 600.f, float MaxSize = 3500.f)
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

	FTDGatePairSpec GatePair(FName IdA, const FVector& A, FName IdB, const FVector& B, const FVector& BHeadsToward, TArray<ETDGamePhase> Open = {})
	{
		FTDGatePairSpec G;
		G.IdA = IdA;
		G.IdB = IdB;
		G.A = Facing(A, B);					// fly into A heading toward B's side of the galaxy
		G.B = Facing(B, BHeadsToward);		// come out of B heading somewhere useful
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
	L.PlayerStart = FTransform(FRotator::ZeroRotator, FVector(-25000.f, 0.f, 0.f));

	// Combat zone: a belt with a clear dogfighting pocket, plus a denser flank field for cover.
	L.AsteroidFields.Add(Field(FVector::ZeroVector, 42000.f, 60, 11, /*Clear*/ 14000.f));
	L.AsteroidFields.Add(Field(FVector(30000.f, -60000.f, 5000.f), 22000.f, 35, 23, 0.f, 800.f, 4500.f));
	L.AsteroidFields.Add(Field(FVector(220000.f, 130000.f, 0.f), 30000.f, 30, 37));	// Verdance approach
	L.AsteroidFields.Add(Field(FVector(-210000.f, 170000.f, 20000.f), 30000.f, 30, 41, 0.f, 1000.f, 5000.f));	// Cinder approach

	// Station ruins around the pocket: long girders and broken modules as cover.
	const float RuinRing = 22000.f;
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = (2.f * PI / 6.f) * i + 0.3f;
		FTDRuinSpec R;
		R.Location = FVector(FMath::Cos(A) * RuinRing, FMath::Sin(A) * RuinRing, (i % 2 ? 1.f : -1.f) * 2500.f);
		R.Rotation = FRotator(i * 7.f, FMath::RadiansToDegrees(A) + 90.f, i * 11.f);
		R.Extent = (i % 2) ? FVector(5000.f, 500.f, 500.f) : FVector(1800.f, 1500.f, 1200.f);
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
	L.Planets.Add(Verdance);

	FTDPlanetSpec Cinder;
	Cinder.Destination = ETDDestination::Cinder;
	Cinder.Location = FVector(-420000.f, 330000.f, 40000.f);
	Cinder.PlanetRadius = 55000.f;
	Cinder.AtmosphereRadius = 66000.f;
	Cinder.ReturnPoint = TDReturnPoints::CinderOrbit;
	Cinder.Color = FLinearColor(0.9f, 0.3f, 0.05f);
	L.Planets.Add(Cinder);

	for (const FTDPlanetSpec& P : L.Planets)
	{
		const FVector ToZone = (L.CombatZoneCenter - P.Location).GetSafeNormal();
		const FVector Orbit = P.Location + ToZone * (P.AtmosphereRadius + 20000.f);
		L.ReturnPoints.Add(P.ReturnPoint, Facing(Orbit, L.CombatZoneCenter));	// face home after take-off
	}

	// Dominion approach, far "south" of the combat zone, nose toward it.
	const FVector DominionLoc(0.f, -520000.f, 20000.f);
	L.Dominion = Facing(DominionLoc, L.CombatZoneCenter);
	const FVector DominionFwd = (L.CombatZoneCenter - DominionLoc).GetSafeNormal();
	L.ReturnPoints.Add(TDReturnPoints::DominionHangarExit, FTransform(DominionFwd.Rotation(), DominionLoc + DominionFwd * 45000.f));

	// Gates: combat zone <-> each orbit, and a flank pair that opens for waves 3-5.
	const FVector VerdanceOrbit = L.ReturnPoints[TDReturnPoints::VerdanceOrbit].GetLocation();
	const FVector CinderOrbit = L.ReturnPoints[TDReturnPoints::CinderOrbit].GetLocation();
	L.Gates.Add(GatePair(TEXT("Gate_Verdance_Zone"), FVector(30000.f, 18000.f, 0.f), TEXT("Gate_Verdance_Orbit"), VerdanceOrbit - (VerdanceOrbit - L.CombatZoneCenter).GetSafeNormal() * 15000.f, Verdance.Location));
	L.Gates.Add(GatePair(TEXT("Gate_Cinder_Zone"), FVector(-30000.f, 18000.f, 0.f), TEXT("Gate_Cinder_Orbit"), CinderOrbit - (CinderOrbit - L.CombatZoneCenter).GetSafeNormal() * 15000.f, Cinder.Location));
	L.Gates.Add(GatePair(TEXT("Gate_Flank_West"), FVector(-35000.f, -38000.f, 0.f), TEXT("Gate_Flank_East"), FVector(38000.f, 36000.f, 3000.f), L.CombatZoneCenter,
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

	if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
	{
		WorldSys->SetPlayBounds(GetActorTransform().TransformPosition(Layout.BoundaryCenter), Layout.BoundaryRadius);
	}

	BuildAsteroids();
	BuildRuins();
	BuildPlanets();
	BuildGates();

	if (bSpawnDominion && !ATDDominion::Get(this))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Dominion = GetWorld()->SpawnActor<ATDDominion>(ATDDominion::StaticClass(), Layout.Dominion * GetActorTransform(), Params);
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

	ApplyBoundsToShips();
	UpdateGatePhases();
}

void ATDGalaxyLayout::BuildAsteroids()
{
	UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this);
	const FTransform Xf = GetActorTransform();
	for (const FTDAsteroidFieldSpec& F : Layout.AsteroidFields)
	{
		FRandomStream Rng(F.Seed);
		int32 Placed = 0;
		for (int32 Attempt = 0; Placed < F.Count && Attempt < F.Count * 6; ++Attempt)
		{
			FVector Offset = Rng.GetUnitVector() * F.Radius * FMath::Pow(Rng.FRand(), 0.6f);
			Offset.Z *= F.Flatten;
			if (Offset.Size() < F.ClearRadius)
			{
				continue;
			}
			const float Size = FMath::Lerp(F.MinSize, F.MaxSize, FMath::Square(Rng.FRand()));	// mostly small, a few big
			const FVector World = Xf.TransformPosition(F.Center + Offset);
			if (WorldSys && WorldSys->IsInsideObstacle(World, Size))
			{
				continue;	// no overlaps
			}
			const FVector Scale = FVector(Size / 100.f) * FVector(1.f + Rng.FRand() * 0.6f, 1.f, 0.7f + Rng.FRand() * 0.4f);
			Asteroids->AddInstance(FTransform(FRotator(Rng.FRandRange(-180, 180), Rng.FRandRange(-180, 180), Rng.FRandRange(-180, 180)), F.Center + Offset, Scale));
			if (WorldSys) { WorldSys->AddObstacle(World, 50.f * Scale.GetMax()); }	// basic sphere = 100 cm across
			++Placed;
		}
	}
	if (UMaterialInstanceDynamic* MID = Asteroids->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.25f, 0.2f, 0.3f));
	}
}

void ATDGalaxyLayout::BuildRuins()
{
	UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this);
	for (const FTDRuinSpec& R : Layout.Ruins)
	{
		RuinBlocks->AddInstance(FTransform(R.Rotation, R.Location, R.Extent * 2.f / 100.f));
		if (WorldSys) { WorldSys->AddObstacle(GetActorTransform().TransformPosition(R.Location), R.Extent.GetMax()); }
	}
	if (UMaterialInstanceDynamic* MID = RuinBlocks->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.35f, 0.4f));
	}
}

void ATDGalaxyLayout::BuildPlanets()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FTDPlanetSpec& P : Layout.Planets)
	{
		const FTransform T(FRotator::ZeroRotator, GetActorTransform().TransformPosition(P.Location));
		ATDPlanetAtmosphere* Planet = GetWorld()->SpawnActorDeferred<ATDPlanetAtmosphere>(ATDPlanetAtmosphere::StaticClass(), T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Planet) { continue; }
		Planet->Configure(P.Destination, P.ReturnPoint, P.PlanetRadius, P.AtmosphereRadius, P.Color);
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
		ATDPortalGate* A = GetWorld()->SpawnActorDeferred<ATDPortalGate>(ATDPortalGate::StaticClass(), TA, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		ATDPortalGate* B = GetWorld()->SpawnActorDeferred<ATDPortalGate>(ATDPortalGate::StaticClass(), TB, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!A || !B) { continue; }
		A->GateId = G.IdA;
		B->GateId = G.IdB;
		A->Radius = B->Radius = G.Radius;
		A->FinishSpawning(TA);
		B->FinishSpawning(TB);
		ATDPortalGate::LinkGates(A, B);
		GateActors.Add(A); GatePairIndex.Add(i);
		GateActors.Add(B); GatePairIndex.Add(i);
	}
}

// ------------------------------------------------------------------------------------------ tick

void ATDGalaxyLayout::ApplyBoundsToShips()
{
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Registry)
	{
		return;
	}
	const FVector Center = GetActorTransform().TransformPosition(Layout.BoundaryCenter);
	for (const TWeakObjectPtr<AActor>& Weak : Registry->GetAll())
	{
		if (const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Weak.Get()))
		{
			if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
			{
				Move->SetBoundary(Center, Layout.BoundaryRadius, true);
			}
		}
	}
}

void ATDGalaxyLayout::UpdateGatePhases()
{
	ITDGameFlow* Flow = UTDStatics::GetGameFlow(this);
	const ETDGamePhase Phase = Flow ? Flow->GetProgress().Phase : ETDGamePhase::Waves1to2;
	for (int32 i = 0; i < GateActors.Num(); ++i)
	{
		ATDPortalGate* G = GateActors[i];
		if (!G || !Layout.Gates.IsValidIndex(GatePairIndex[i])) { continue; }
		const TArray<ETDGamePhase>& Open = Layout.Gates[GatePairIndex[i]].OpenPhases;
		const bool bShouldOpen = Open.Num() == 0 || Open.Contains(Phase);
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
	BoundsTimer -= DeltaSeconds;
	if (BoundsTimer <= 0.f)
	{
		BoundsTimer = BoundsRefreshSeconds;
		ApplyBoundsToShips();
	}
	PhaseTimer -= DeltaSeconds;
	if (PhaseTimer <= 0.f)
	{
		PhaseTimer = 1.f;
		UpdateGatePhases();
	}
}

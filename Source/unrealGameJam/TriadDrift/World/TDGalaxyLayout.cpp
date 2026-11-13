// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDGalaxyLayout.h"

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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
	auto MakeLook = [this](const FName Name, UStaticMesh* Mesh) -> UInstancedStaticMeshComponent*
	{
		UInstancedStaticMeshComponent* Look = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Look->SetupAttachment(GetRootComponent());
		Look->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Look->SetGenerateOverlapEvents(false);
		Look->SetMobility(EComponentMobility::Movable);
		Look->SetStaticMesh(Mesh);
		return Look;
	};
	{
		UStaticMesh* Rocks[] = { Rock1.Object, Rock2.Object, Rock3.Object, Rock4.Object };
		for (UStaticMesh* M : Rocks)
		{
			if (M) { AsteroidLooks.Add(MakeLook(*FString::Printf(TEXT("AsteroidLook%d"), AsteroidLooks.Num() + 1), M)); }
		}
		UStaticMesh* Bits[] = { Debris1.Object, Debris2.Object, Debris3.Object };
		for (UStaticMesh* M : Bits)
		{
			if (M) { DebrisLooks.Add(MakeLook(*FString::Printf(TEXT("DebrisLook%d"), DebrisLooks.Num() + 1), M)); }
		}
	}

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
	L.AsteroidFields.Add(TDGalaxy_Field(FVector::ZeroVector, 42000.f, 60, 11, /*Clear*/ 14000.f));
	L.AsteroidFields.Add(TDGalaxy_Field(FVector(30000.f, -60000.f, 5000.f), 22000.f, 35, 23, 0.f, 800.f, 4500.f));
	L.AsteroidFields.Add(TDGalaxy_Field(FVector(220000.f, 130000.f, 0.f), 30000.f, 30, 37));	// Verdance approach
	L.AsteroidFields.Add(TDGalaxy_Field(FVector(-210000.f, 170000.f, 20000.f), 30000.f, 30, 41, 0.f, 1000.f, 5000.f));	// Cinder approach

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
	// Orbit gates sit OFF the line from the orbit return point to the combat zone (25 000 cm to the side): a ship
	// leaving orbit flies straight home and never takes the gate by accident (T1 / T5 playtests: it did, 3 s
	// after every return, and popped out near the centre). Taking the gate is a choice the HUD marker offers.
	auto OrbitGateAt = [&L](const FVector& Orbit)
	{
		const FVector Home = (L.CombatZoneCenter - Orbit).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Home).GetSafeNormal();
		return Orbit + Side * 25000.f + Home * 5000.f;
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
	if (RuinBlocks) { RuinBlocks->SetVisibility(!bHide); }
	for (ATDPlanetAtmosphere* P : Planets)
	{
		if (P) { P->SetPlaceholderVisible(!bHide); }
	}
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
				AsteroidLooks[LookRng.RandHelper(AsteroidLooks.Num())]->AddInstance(
					FTransform(FRotator(LookRng.FRandRange(-180, 180), LookRng.FRandRange(-180, 180), LookRng.FRandRange(-180, 180)), Local, RockXf.GetScale3D()));
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

	for (const FTDRuinSpec& R : Layout.Ruins)
	{
		RuinBlocks->AddInstance(FTransform(R.Rotation, R.Location, R.Extent * 2.f / 100.f));
		if (WorldSys) { WorldSys->AddObstacle(Xf.TransformPosition(R.Location), R.Extent.GetMax()); }
	}
	if (RuinMesh.IsNull())
	{
		UTDStatics::TintPlaceholder(RuinBlocks, FLinearColor(0.35f, 0.35f, 0.4f), 0.f);
	}
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
	PhaseTimer -= DeltaSeconds;
	if (PhaseTimer <= 0.f)
	{
		PhaseTimer = 1.f;
		UpdateGatePhases();
	}
}

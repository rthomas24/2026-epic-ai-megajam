// Triad Drift - planet layouts (ENG-25 Verdance, ENG-26 Cinder). Owner: Teammate 5.

#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/TDGroundScene.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "TDPlanetCourse"

namespace TDCourseColors
{
	// Verdance: deep teal ground, bioluminescent cyan / magenta / lime.
	static const FLinearColor VerdanceGround(0.012f, 0.05f, 0.045f);
	static const FLinearColor VerdanceRock(0.08f, 0.16f, 0.14f);
	static const FLinearColor Trunk(0.06f, 0.05f, 0.08f);
	static const FLinearColor Cyan(0.2f, 0.95f, 1.f);
	static const FLinearColor Magenta(1.f, 0.25f, 0.85f);
	static const FLinearColor Lime(0.55f, 1.f, 0.3f);
	static const FLinearColor WreckMetal(0.18f, 0.18f, 0.22f);
	static const FLinearColor Ember(1.f, 0.45f, 0.1f);
	static const FLinearColor RelayStone(0.6f, 0.5f, 1.f);
	// Cinder: basalt, magma orange, crystal violet.
	static const FLinearColor CinderGround(0.05f, 0.035f, 0.035f);
	static const FLinearColor Basalt(0.12f, 0.1f, 0.1f);
	static const FLinearColor MagmaGlow(1.f, 0.3f, 0.04f);
	static const FLinearColor Crystal(0.7f, 0.3f, 1.f);
	static const FLinearColor ArmadaRed(0.55f, 0.06f, 0.04f);
	static const FLinearColor Steel(0.2f, 0.2f, 0.23f);
}

ATDPlanetCourse::ATDPlanetCourse()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Lit(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	// Engineering's dressing meshes (always cooked: hard-referenced from this CDO).
	static const TCHAR* const PropPaths[] = {
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Mushroom_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Mushroom_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Mushroom_03"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Fern_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Fern_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_GlowPlant_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_GlowPlant_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Rock_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Rock_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Verdance/SM_Verd_Rock_03"),
		TEXT("/Game/TriadDrift/Gameplay/Vegetation/Verdance/Kapok/SM_TDVeg_Kapok_S100_11"),
		TEXT("/Game/TriadDrift/Gameplay/Vegetation/Verdance/Kapok/SM_TDVeg_Kapok_S070_12"),
		TEXT("/Game/TriadDrift/Gameplay/Vegetation/Verdance/BrazilNut/SM_TDVeg_BrazilNut_S100_21"),
		TEXT("/Game/TriadDrift/Gameplay/Vegetation/Verdance/TrumpetTree/SM_TDVeg_TrumpetTree_S100_31"),
		TEXT("/Game/TriadDrift/Gameplay/Vegetation/Verdance/StiltPalm/SM_TDVeg_StiltPalm_S100_41"),
		TEXT("/Game/TriadDrift/Gameplay/Vegetation/Verdance/TreeFern/SM_TDVeg_TreeFern_S100_51"),
		TEXT("/Game/TriadDrift/Gameplay/Vegetation/Verdance/Cacao/SM_TDVeg_Cacao_S100_61"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_03"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_04"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Basalt_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Basalt_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Basalt_03"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_BasaltColumn_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_BasaltColumn_02")
	};
	for (const TCHAR* Path : PropPaths)
	{
		const FString Short = FPackageName::GetShortName(Path);
		PropRefs.Add(FName(*Short), TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("%s.%s"), Path, *Short))));
	}
	CubeMesh = Cube.Object;
	CylinderMesh = Cyl.Object;
	SphereMesh = Sph.Object;
	ConeMesh = Cone.Object;
	GlowMaterial = nullptr; // glow comes from UTDStatics::TintPlaceholder (Core's always-cooked glow material)
	LitMaterial = Lit.Object;
}

ATDPlanetCourse* ATDPlanetCourse::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	for (TActorIterator<ATDPlanetCourse> It(World); World && It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ATDPlanetCourse::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Dressing = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), GetActorTransform(), Params);
	if (Dressing)
	{
		USceneComponent* Root = NewObject<USceneComponent>(Dressing, TEXT("Root"));
		Dressing->SetRootComponent(Root);
		Root->RegisterComponent();
		Dressing->SetActorTransform(GetActorTransform());
		Spawned.Add(Dressing);
	}
	for (const TPair<FName, TSoftObjectPtr<UStaticMesh>>& Ref : PropRefs)
	{
		if (UStaticMesh* M = Ref.Value.LoadSynchronous())
		{
			PropMeshes.Add(Ref.Key, M);
		}
	}
	if (PlanetId == TEXT("Cinder"))
	{
		BuildCinder();
	}
	else
	{
		BuildVerdance();
	}
	MarkCompletedFromProgress();
}

// ------------------------------------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------------------------------------

UStaticMeshComponent* ATDPlanetCourse::Block(EShape Shape, const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, float Glow, const FRotator& Rot, bool bCollide)
{
	if (!Dressing)
	{
		return nullptr;
	}
	UStaticMesh* Mesh = Shape == EShape::Cube ? CubeMesh : Shape == EShape::Cylinder ? CylinderMesh : Shape == EShape::Sphere ? SphereMesh : ConeMesh;
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Dressing);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetupAttachment(Dressing->GetRootComponent());
	C->SetRelativeLocationAndRotation(Center, Rot);
	C->SetRelativeScale3D(SizeCm / 100.f);
	C->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
	C->SetCanEverAffectNavigation(bCollide);
	C->SetCastShadow(Glow <= 0.f);
	C->RegisterComponent();
	Dressing->AddInstanceComponent(C);
	UTDStatics::TintPlaceholder(C, Color, Glow);
	return C;
}

UStaticMeshComponent* ATDPlanetCourse::Platform(const FVector& XY, float Top, const FVector& SizeCm, const FLinearColor& Color, float Glow)
{
	return Block(EShape::Cube, FVector(XY.X, XY.Y, Top - SizeCm.Z * 0.5f), SizeCm, Color, Glow);
}

void ATDPlanetCourse::Route(const TArray<FVector>& Points, const FLinearColor& Color, float Spacing)
{
	Routes.Add(Points);
	// Glowing posts on both sides of the route: the way reads from anywhere, day or night.
	for (int32 i = 0; i + 1 < Points.Num(); ++i)
	{
		const FVector A = Points[i];
		const FVector B = Points[i + 1];
		const FVector Dir = (B - A).GetSafeNormal2D();
		const FVector Side(-Dir.Y, Dir.X, 0.f);
		const int32 N = FMath::Max(1, FMath::RoundToInt((B - A).Size2D() / Spacing));
		for (int32 k = 0; k < N; ++k)
		{
			const FVector P = FMath::Lerp(A, B, (k + 0.5f) / N);
			for (const float S : { -1.f, 1.f })
			{
				const FVector Base = FVector(P.X, P.Y, 0.f) + Side * S * 450.f;
				Block(EShape::Cylinder, Base + FVector(0.f, 0.f, 60.f), FVector(18.f, 18.f, 120.f), FLinearColor(0.05f, 0.05f, 0.06f), 0.f, FRotator::ZeroRotator, false);
				Block(EShape::Sphere, Base + FVector(0.f, 0.f, 130.f), FVector(34.f), Color, 3.f, FRotator::ZeroRotator, false);
			}
		}
	}
}

bool ATDPlanetCourse::IsOnRoute(const FVector& Local, float Clearance) const
{
	for (const TArray<FVector>& R : Routes)
	{
		for (int32 i = 0; i + 1 < R.Num(); ++i)
		{
			const FVector P = FMath::ClosestPointOnSegment(FVector(Local.X, Local.Y, 0.f), FVector(R[i].X, R[i].Y, 0.f), FVector(R[i + 1].X, R[i + 1].Y, 0.f));
			if (FVector::Dist2D(P, Local) < Clearance)
			{
				return true;
			}
		}
	}
	return false;
}

void ATDPlanetCourse::ScatterFlora(int32 Count, float Radius, uint32 Seed)
{
	using namespace TDCourseColors;
	FRandomStream Rng(Seed);
	const bool bCinder = PlanetId == TEXT("Cinder");
	int32 Placed = 0;
	for (int32 Tries = 0; Tries < Count * 6 && Placed < Count; ++Tries)
	{
		const FVector P(Rng.FRandRange(-Radius * 0.35f, Radius), Rng.FRandRange(-Radius, Radius), 0.f);
		if (P.Size2D() < 3500.f || IsOnRoute(P, 1100.f))
		{
			continue; // keep the pads, the routes and the arenas clear
		}
		++Placed;
		if (bCinder)
		{
			// Basalt spires and glowing crystal clusters.
			const float H = Rng.FRandRange(400.f, 1600.f);
			Block(EShape::Cone, P + FVector(0.f, 0.f, H * 0.5f), FVector(Rng.FRandRange(250.f, 600.f), Rng.FRandRange(250.f, 600.f), H), Basalt);
			if (Rng.FRand() < 0.5f)
			{
				const FVector C = P + FVector(Rng.FRandRange(-300.f, 300.f), Rng.FRandRange(-300.f, 300.f), 0.f);
				for (int32 k = 0; k < 3; ++k)
				{
					const float CH = Rng.FRandRange(120.f, 320.f);
					Block(EShape::Cone, C + FVector(k * 60.f, k * -40.f, CH * 0.5f), FVector(80.f, 80.f, CH), Rng.FRand() < 0.5f ? Crystal : Ember, 2.f,
						FRotator(Rng.FRandRange(-20.f, 20.f), 0.f, Rng.FRandRange(-20.f, 20.f)), false);
				}
			}
		}
		else
		{
			// A tall dark trunk with a glowing cap, sometimes a ring of glowing mushrooms.
			const float H = Rng.FRandRange(600.f, 1500.f);
			const FLinearColor Cap = Rng.FRand() < 0.4f ? Cyan : (Rng.FRand() < 0.5f ? Magenta : Lime);
			Block(EShape::Cylinder, P + FVector(0.f, 0.f, H * 0.5f), FVector(90.f, 90.f, H), Trunk);
			Block(EShape::Sphere, P + FVector(0.f, 0.f, H + 120.f), FVector(Rng.FRandRange(380.f, 700.f), Rng.FRandRange(380.f, 700.f), 260.f), Cap, 1.8f, FRotator::ZeroRotator, false);
			if (Rng.FRand() < 0.4f)
			{
				for (int32 k = 0; k < 4; ++k)
				{
					const float A = k * 1.57f + Rng.FRand();
					const FVector M = P + FVector(FMath::Cos(A) * 260.f, FMath::Sin(A) * 260.f, 0.f);
					Block(EShape::Cylinder, M + FVector(0.f, 0.f, 40.f), FVector(16.f, 16.f, 80.f), Trunk, 0.f, FRotator::ZeroRotator, false);
					Block(EShape::Sphere, M + FVector(0.f, 0.f, 90.f), FVector(70.f, 70.f, 36.f), Cap, 2.5f, FRotator::ZeroRotator, false);
				}
			}
		}
	}
}

bool ATDPlanetCourse::IsKeptClear(const FVector& Local, float Clearance) const
{
	if (IsOnRoute(Local, Clearance))
	{
		return true;
	}
	for (const FKeepOut& K : KeepOuts)
	{
		if (FMath::Abs(Local.X - K.Center.X) < K.Extent.X && FMath::Abs(Local.Y - K.Center.Y) < K.Extent.Y)
		{
			return true;
		}
	}
	return false;
}

bool ATDPlanetCourse::ScatterMeshes(const TArray<FScatter>& Kinds, uint32 Seed)
{
	if (!Dressing)
	{
		return false;
	}
	bool bAll = true;
	FRandomStream Rng(Seed);
	for (const FScatter& K : Kinds)
	{
		UStaticMesh* Mesh = Prop(K.Mesh);
		if (!Mesh)
		{
			bAll = false;
			continue;
		}
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(Dressing);
		ISM->SetMobility(EComponentMobility::Movable);
		ISM->SetStaticMesh(Mesh);
		ISM->SetupAttachment(Dressing->GetRootComponent());
		ISM->SetCollisionProfileName(K.bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		ISM->SetCanEverAffectNavigation(false);
		ISM->RegisterComponent();
		Dressing->AddInstanceComponent(ISM);
		TArray<FTransform> Xfs;
		for (int32 Tries = 0; Tries < K.Count * 8 && Xfs.Num() < K.Count; ++Tries)
		{
			const float A = Rng.FRandRange(0.f, 2.f * PI);
			const float R = FMath::Sqrt(Rng.FRandRange(FMath::Square(K.Radius.X), FMath::Square(K.Radius.Y)));
			const FVector P = K.Center + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f);
			if (P.Size2D() < 3500.f || IsKeptClear(P, K.Clearance))
			{
				continue; // the pad, the routes and every objective area stay clear
			}
			const float S = Rng.FRandRange(K.Scale.X, K.Scale.Y);
			Xfs.Emplace(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), P - FVector(0.f, 0.f, K.Sink * S), FVector(S));
		}
		ISM->AddInstances(Xfs, false, false);
	}
	return bAll;
}

void ATDPlanetCourse::MarkRoutes(FName MeshName, float Spacing, float Offset, const FVector2D& Scale)
{
	UStaticMesh* Mesh = Prop(MeshName);
	if (!Mesh || !Dressing)
	{
		return;
	}
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(Dressing);
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetStaticMesh(Mesh);
	ISM->SetupAttachment(Dressing->GetRootComponent());
	ISM->SetCollisionProfileName(TEXT("NoCollision")); // never something to get stuck on beside a route
	ISM->RegisterComponent();
	Dressing->AddInstanceComponent(ISM);
	TArray<FTransform> Xfs;
	FRandomStream Rng(0x7EA1);
	int32 Side = 0;
	for (const TArray<FVector>& R : Routes)
	{
		for (int32 i = 0; i + 1 < R.Num(); ++i)
		{
			const FVector Dir = (R[i + 1] - R[i]).GetSafeNormal2D();
			const FVector Perp(-Dir.Y, Dir.X, 0.f);
			const int32 N = FMath::Max(1, FMath::RoundToInt((R[i + 1] - R[i]).Size2D() / Spacing));
			for (int32 k = 0; k < N; ++k)
			{
				const FVector P = FMath::Lerp(R[i], R[i + 1], (k + 0.5f) / N) + Perp * Offset * ((++Side % 2) ? 1.f : -1.f);
				Xfs.Emplace(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(P.X, P.Y, 0.f), FVector(Rng.FRandRange(Scale.X, Scale.Y)));
			}
		}
	}
	ISM->AddInstances(Xfs, false, false);
}

template <class T>
T* ATDPlanetCourse::SpawnProp(const FVector& Location, const FRotator& Rotation)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;
	T* A = GetWorld()->SpawnActor<T>(T::StaticClass(), FTransform(GetActorRotation() + Rotation, W(Location)), Params);
	if (A)
	{
		Spawned.Add(A);
	}
	return A;
}

ATDObjective* ATDPlanetCourse::MakeObjective(FName Id, const FText& Title, ETDAbilityId Reward, ETDShipId Ship, int32 StepCount)
{
	ATDObjective* Obj = SpawnProp<ATDObjective>(FVector::ZeroVector);
	if (!Obj)
	{
		return nullptr;
	}
	Obj->ObjectiveId = Id;
	Obj->Title = Title;
	Obj->RewardAbility = Reward;
	Obj->RewardShip = Ship;
	Obj->bAutoStart = false;
	Obj->Steps.SetNum(StepCount);
	Objectives.Add(Obj);
	return Obj;
}

void ATDPlanetCourse::FinishObjective(ATDObjective* Obj, const TArray<FText>& StepTexts, const TArray<AActor*>& Waypoints)
{
	if (!Obj)
	{
		return;
	}
	for (int32 i = 0; i < Obj->Steps.Num(); ++i)
	{
		Obj->Steps[i].Text = StepTexts.IsValidIndex(i) ? StepTexts[i] : FText::GetEmpty();
		Obj->Steps[i].Waypoint = Waypoints.IsValidIndex(i) ? Waypoints[i] : nullptr;
	}
	Obj->FinishSpawning(Obj->GetActorTransform());
	Obj->OnStepStarted.AddUniqueDynamic(this, &ATDPlanetCourse::HandleStepStarted);
	Obj->OnCompleted.AddUniqueDynamic(this, &ATDPlanetCourse::HandleObjectiveCompleted);
}

ATDEncounterVolume* ATDPlanetCourse::Encounter(ATDObjective* Obj, int32 LinkedStep, const FVector& Center, const FVector& Extent, const TArray<ETDCrewType>& Mix, bool bOnOverlap, float RingRadius, const TArray<FVector>& Points)
{
	ATDEncounterVolume* E = SpawnProp<ATDEncounterVolume>(Center);
	if (!E)
	{
		return nullptr;
	}
	E->CrewMix = Mix;
	E->Count = Mix.Num();
	E->RingRadius = RingRadius;
	E->SpawnPoints = Points;
	E->LinkedObjective = Obj;
	E->LinkedStep = LinkedStep;
	E->bTriggerOnOverlap = bOnOverlap;
	E->FinishSpawning(E->GetActorTransform());
	if (E->Box)
	{
		E->Box->SetBoxExtent(Extent);
	}
	return E;
}

AActor* ATDPlanetCourse::Marker(const FVector& At, float Yaw)
{
	AActor* M = SpawnProp<AActor>(At, FRotator(0.f, Yaw, 0.f));
	if (M)
	{
		USceneComponent* Root = NewObject<USceneComponent>(M, TEXT("Root"));
		M->SetRootComponent(Root);
		M->FinishSpawning(M->GetActorTransform());
		Root->RegisterComponent();
		M->SetActorTransform(FTransform(GetActorRotation() + FRotator(0.f, Yaw, 0.f), W(At)));
	}
	return M;
}

void ATDPlanetCourse::Checkpoint(ATDObjective* Obj, int32 Step, const FVector& At, float Yaw)
{
	if (Obj && Obj->Steps.IsValidIndex(Step))
	{
		Obj->Steps[Step].bCheckpoint = true;
		Obj->Steps[Step].RespawnAt = Marker(At + FVector(0.f, 0.f, 100.f), Yaw);
	}
}

void ATDPlanetCourse::AddTrailhead(ATDObjective* Obj, const FVector& At, const FLinearColor& Color, const FText& Name)
{
	// A glowing totem: a dark plinth, a tall light column and a floating crown.
	Platform(At, 40.f, FVector(500.f, 500.f, 40.f), FLinearColor(0.05f, 0.05f, 0.06f));
	Block(EShape::Cylinder, At + FVector(0.f, 0.f, 260.f), FVector(22.f, 22.f, 440.f), Color, 2.f, FRotator::ZeroRotator, false); // thin: readable without filling the view up close
	Block(EShape::Cone, At + FVector(0.f, 0.f, 600.f), FVector(160.f, 160.f, 200.f), Color, 3.f, FRotator(180.f, 0.f, 0.f), false);
	AActor* Marker = SpawnProp<AActor>(At + FVector(0.f, 0.f, 300.f));
	if (Marker)
	{
		USceneComponent* Root = NewObject<USceneComponent>(Marker, TEXT("Root"));
		Marker->SetRootComponent(Root);
		Marker->FinishSpawning(Marker->GetActorTransform());
		Root->RegisterComponent();
		Marker->SetActorLocation(W(At + FVector(0.f, 0.f, 300.f)));
	}
	FTDTrailhead T;
	T.Objective = Obj;
	T.Marker = Marker;
	T.Name = Name;
	Trailheads.Add(T);
}

// ------------------------------------------------------------------------------------------------
// Running
// ------------------------------------------------------------------------------------------------

ATDObjective* ATDPlanetCourse::FindObjective(FName ObjectiveId) const
{
	for (ATDObjective* O : Objectives)
	{
		if (O && O->ObjectiveId == ObjectiveId)
		{
			return O;
		}
	}
	return nullptr;
}

bool ATDPlanetCourse::IsAnyObjectiveRunning() const
{
	for (const ATDObjective* O : Objectives)
	{
		if (O && O->IsActive())
		{
			return true;
		}
	}
	return false;
}

bool ATDPlanetCourse::StartObjectiveAt(FName ObjectiveId)
{
	ATDObjective* O = FindObjective(ObjectiveId);
	if (!O || O->IsActive() || O->IsComplete() || IsAnyObjectiveRunning())
	{
		return false; // one scene at a time
	}
	O->StartObjective();
	return O->IsActive();
}

bool ATDPlanetCourse::GetIdleLine(FText& OutText, TArray<AActor*>& OutMarkers) const
{
	OutMarkers.Reset();
	if (IsAnyObjectiveRunning())
	{
		return false;
	}
	TArray<FText> Names;
	for (const FTDTrailhead& T : Trailheads)
	{
		if (T.Objective && !T.Objective->IsComplete())
		{
			Names.Add(T.Name);
			if (T.Marker)
			{
				OutMarkers.Add(T.Marker);
			}
		}
	}
	if (Names.Num() == 0)
	{
		return false;
	}
	OutText = Names.Num() == 1
		? FText::Format(LOCTEXT("IdleOne", "HEAD TO {0}"), Names[0])
		: FText::Format(LOCTEXT("IdleTwo", "HEAD TO A SIGNAL · {0} OR {1}"), Names[0], Names[1]);
	return true;
}

void ATDPlanetCourse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const ATDFirstPersonCharacter* P = PC ? Cast<ATDFirstPersonCharacter>(PC->GetPawn()) : nullptr;
	if (!P || P->IsDead() || IsAnyObjectiveRunning())
	{
		return;
	}
	if (const UTDGameInstance* GI = UTDGameInstance::Get(this); GI && GI->IsRetakePending())
	{
		return;
	}
	for (const FTDTrailhead& T : Trailheads)
	{
		if (T.Objective && T.Marker && !T.Objective->IsComplete()
			&& FVector::Dist2D(T.Marker->GetActorLocation(), P->GetActorLocation()) < TrailheadRadius)
		{
			StartObjectiveAt(T.Objective->ObjectiveId);
			return;
		}
	}
}

void ATDPlanetCourse::HandleStepStarted(ATDObjective* Objective, int32 StepIndex)
{
	for (const FTDStepEncounter& S : StepEncounters)
	{
		if (S.Objective == Objective && S.Step == StepIndex && S.Encounter && !S.Encounter->IsActive() && !S.Encounter->IsCleared())
		{
			S.Encounter->Activate();
		}
	}
	for (const FTDStepDoor& D : StepDoors)
	{
		if (D.Objective == Objective && D.Step == StepIndex && D.Door)
		{
			D.Door->Open();
		}
	}
}

void ATDPlanetCourse::HandleObjectiveCompleted(ATDObjective* Objective)
{
	// The relay wakes: a pulse ring grows over the plateau (the reward screen is Nick's).
	if (Objective && Objective->ObjectiveId == TEXT("Obj_Relay"))
	{
		for (int32 i = 0; i < 3; ++i)
		{
			Block(EShape::Sphere, FVector(9200.f, -8200.f, 900.f), FVector(1200.f + i * 900.f, 1200.f + i * 900.f, 40.f), TDCourseColors::RelayStone, 2.f - i * 0.5f, FRotator::ZeroRotator, false);
		}
	}
}

void ATDPlanetCourse::MarkCompletedFromProgress()
{
	const UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (!GI)
	{
		return;
	}
	for (ATDObjective* O : Objectives)
	{
		if (O && GI->GetProgressRef().CompletedObjectives.Contains(O->ObjectiveId))
		{
			O->MarkAlreadyComplete(); // loaded mid-planet: done stays done
			for (const FTDStepDoor& D : StepDoors)
			{
				if (D.Objective == O && D.Door) { D.Door->Open(); } // the breached gate stays breached
			}
		}
	}
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->ApplyCompletedState(); // rooms cleared, doors open, crystals gone
	}
}

// ------------------------------------------------------------------------------------------------
// Verdance (ENG-25)
// ------------------------------------------------------------------------------------------------

void ATDPlanetCourse::BuildVerdance()
{
	using namespace TDCourseColors;
	// Ground: one lit slab, 4 km, top at the pad (z = 0). The rig's grey slab is switched off.
	Block(EShape::Cube, FVector(0.f, 0.f, -100.f), FVector(600000.f, 600000.f, 200.f), VerdanceGround);

	// ---- Obj 1: the wreck core (-> Gravity Well on Bastion). North-east. ----
	const FVector WreckHead(2600.f, 2600.f, 0.f);
	const FVector Ambush(6200.f, 4600.f, 0.f);
	const FVector WreckSite(11500.f, 7200.f, 0.f);
	ATDObjective* Wreck = MakeObjective(TEXT("Obj_WreckCore"), LOCTEXT("WreckTitle", "The Wreck Core"), ETDAbilityId::GravityWell, ETDShipId::Bastion, 4);
	AddTrailhead(Wreck, WreckHead, Cyan, LOCTEXT("TheWreck", "THE WRECK"));
	Route({ FVector(900.f, 900.f, 0.f), WreckHead, Ambush, FVector(9300.f, 6300.f, 0.f), WreckSite }, Cyan);

	// Ambush: fallen logs for cover, two troopers and two grunts waiting in the glow.
	TArray<AActor*> AmbushCover;
	for (const FVector& L : { FVector(5600.f, 4100.f, 0.f), FVector(6700.f, 5200.f, 0.f), FVector(6900.f, 4000.f, 0.f), FVector(5700.f, 5300.f, 0.f) })
	{
		Block(EShape::Cube, L + FVector(0.f, 0.f, 70.f), FVector(500.f, 110.f, 140.f), Trunk, 0.f, FRotator(0.f, 35.f, 0.f));
	}
	Encounter(Wreck, INDEX_NONE, Ambush + FVector(0.f, 0.f, 200.f), FVector(1500.f, 1500.f, 600.f),
		{ ETDCrewType::Trooper, ETDCrewType::Grunt, ETDCrewType::Grunt }, true, 900.f); // the first fight: one shooter (playtest: 4 crew killed a non-blocker in ~4 s)

	// The wreck: a broken hull nose-down in the ground, glowing breaks, debris to fight around.
	// Collidable pieces turn in yaw only (a tilted box left an overhang the pilot got wedged under in playtest);
	// the broken, tilted look comes from the non-colliding glow pieces.
	Block(EShape::Cube, WreckSite + FVector(-600.f, 900.f, 350.f), FVector(2600.f, 900.f, 700.f), WreckMetal, 0.f, FRotator(0.f, 25.f, 0.f));
	Block(EShape::Cube, WreckSite + FVector(-700.f, 900.f, 780.f), FVector(1800.f, 700.f, 160.f), WreckMetal, 0.f, FRotator(-12.f, 25.f, 8.f), false);
	Block(EShape::Cylinder, WreckSite + FVector(900.f, -700.f, 350.f), FVector(700.f, 700.f, 700.f), WreckMetal, 0.f, FRotator(0.f, -30.f, 0.f));
	Block(EShape::Cylinder, WreckSite + FVector(1100.f, -900.f, 700.f), FVector(500.f, 500.f, 900.f), WreckMetal, 0.f, FRotator(70.f, -30.f, 0.f), false);
	Block(EShape::Cube, WreckSite + FVector(-900.f, -500.f, 120.f), FVector(600.f, 160.f, 240.f), WreckMetal, 0.f, FRotator(0.f, 70.f, 0.f));
	Block(EShape::Cube, WreckSite + FVector(500.f, 800.f, 110.f), FVector(500.f, 160.f, 220.f), WreckMetal, 0.f, FRotator(0.f, -20.f, 0.f));
	Block(EShape::Sphere, WreckSite + FVector(-500.f, 700.f, 500.f), FVector(500.f, 260.f, 200.f), Ember, 3.f, FRotator(0.f, 25.f, 0.f), false);
	Block(EShape::Sphere, WreckSite + FVector(1100.f, -850.f, 300.f), FVector(300.f), Ember, 3.f, FRotator::ZeroRotator, false);

	ATDStepTrigger* Beacon = SpawnProp<ATDStepTrigger>(FVector(9300.f, 6300.f, 0.f));
	Beacon->LinkedObjective = Wreck;
	Beacon->LinkedStep = 0;
	Beacon->FinishSpawning(Beacon->GetActorTransform());

	// Explicit spawn points clear of the hull (a ring point landed inside it: an unreachable, unkillable grunt).
	ATDEncounterVolume* SiteFight = Encounter(Wreck, 1, WreckSite + FVector(0.f, 0.f, 200.f), FVector(2600.f, 2600.f, 700.f),
		{ ETDCrewType::Grunt, ETDCrewType::Grunt, ETDCrewType::Heavy, ETDCrewType::Grunt }, false, 1000.f,
		{ FVector(1300.f, 200.f, 0.f), FVector(-200.f, -1300.f, 0.f), FVector(1100.f, 1100.f, 0.f), FVector(-1500.f, -300.f, 0.f) });
	StepEncounters.Add({ Wreck, 1, SiteFight });

	// Two waves drop in on the extraction (spawned high: they fall in).
	ATDEncounterVolume* WaveA = Encounter(Wreck, INDEX_NONE, WreckSite + FVector(0.f, 0.f, 900.f), FVector(10.f), { ETDCrewType::Grunt, ETDCrewType::Grunt }, false, 1100.f); // (machine playtest: 3+3 during a 4 s hold killed the pilot 20 times)
	ATDEncounterVolume* WaveB = Encounter(Wreck, INDEX_NONE, WreckSite + FVector(0.f, 0.f, 900.f), FVector(10.f), { ETDCrewType::Trooper, ETDCrewType::Drone }, false, 1300.f);
	ATDExtractionSite* Core = SpawnProp<ATDExtractionSite>(WreckSite + FVector(0.f, 0.f, 70.f));
	Core->LinkedObjective = Wreck;
	Core->LinkedStep = 2;
	Core->WaveA = WaveA;
	Core->WaveB = WaveB;
	Core->FinishSpawning(Core->GetActorTransform());

	ATDStepTrigger* Ship = SpawnProp<ATDStepTrigger>(FVector(300.f, 0.f, 0.f));
	Ship->LinkedObjective = Wreck;
	Ship->LinkedStep = 3;
	Ship->bRequireCarrying = true;
	Ship->FinishSpawning(Ship->GetActorTransform());
	Ship->Box->SetBoxExtent(FVector(1400.f, 1400.f, 400.f));

	Checkpoint(Wreck, 2, WreckSite + FVector(-900.f, -300.f, 0.f), 15.f); // dying in the extraction: back at the wreck
	FinishObjective(Wreck,
		{ LOCTEXT("W0", "FOLLOW THE BEACON TO THE WRECK"), LOCTEXT("W1", "CLEAR THE WRECK SITE"),
		  LOCTEXT("W2", "EXTRACT THE CORE · HOLD [E]"), LOCTEXT("W3", "CARRY THE CORE TO YOUR SHIP") },
		{ Beacon, SiteFight, Core, Ship });

	// ---- Obj 2: the ancient relay (-> Cloak on Rift). South-east, up a ruined climb. ----
	const FVector RelayHead(2600.f, -2600.f, 0.f);
	ATDObjective* Relay = MakeObjective(TEXT("Obj_Relay"), LOCTEXT("RelayTitle", "The Ancient Relay"), ETDAbilityId::Cloak, ETDShipId::Rift, 3);
	AddTrailhead(Relay, RelayHead, Magenta, LOCTEXT("TheRelay", "THE RELAY"));
	Route({ FVector(900.f, -900.f, 0.f), RelayHead, FVector(5000.f, -5000.f, 0.f) }, Magenta);

	// The climb (jump 134 cm, double jump ~214, mantle 180 above the feet, dash 12 m), sized from playtest:
	// A +110 jump -> B +180 double jump / mantle -> a 7 m gap: jump + dash onto C, a deep 10 m landing (a 12 m
	// dash must not overshoot it) -> plateau +270 jump + mantle. C (330) can't be climbed from the ground, so a
	// miss means back to A: platforming, not a wall walk.
	Platform(FVector(5700.f, -5500.f, 0.f), 110.f, FVector(600.f, 600.f, 110.f), RelayStone, 0.4f);
	Platform(FVector(6400.f, -6050.f, 0.f), 290.f, FVector(600.f, 600.f, 290.f), RelayStone, 0.4f);
	Platform(FVector(7550.f, -7000.f, 0.f), 330.f, FVector(1000.f, 1000.f, 330.f), RelayStone, 0.4f);
	Block(EShape::Cube, FVector(6950.f, -6500.f, -40.f), FVector(700.f, 300.f, 20.f), Magenta, 2.f, FRotator(0.f, -40.f, 0.f), false); // glowing hint under the gap
	const FVector Plateau(9200.f, -8200.f, 0.f);
	const float PlateauTop = 600.f;
	Platform(Plateau, PlateauTop, FVector(2600.f, 2600.f, PlateauTop), VerdanceRock);
	// Ruined pillars around the plateau edge.
	for (int32 i = 0; i < 8; ++i)
	{
		const float A = i * PI / 4.f;
		Block(EShape::Cylinder, Plateau + FVector(FMath::Cos(A) * 1200.f, FMath::Sin(A) * 1200.f, PlateauTop + 300.f), FVector(120.f, 120.f, 600.f), VerdanceRock);
	}

	ATDStepTrigger* Climb = SpawnProp<ATDStepTrigger>(Plateau + FVector(0.f, 0.f, PlateauTop));
	Climb->LinkedObjective = Relay;
	Climb->LinkedStep = 0;
	Climb->FinishSpawning(Climb->GetActorTransform());
	Climb->Box->SetBoxExtent(FVector(1300.f, 1300.f, 300.f));

	// Three resonance nodes on stone stubs around the plateau (+180: double jump or mantle), drones guarding.
	ATDStepCounter* Nodes = SpawnProp<ATDStepCounter>(Plateau + FVector(0.f, 0.f, PlateauTop));
	Nodes->LinkedObjective = Relay;
	Nodes->LinkedStep = 1;
	Nodes->Required = 3;
	Nodes->BaseText = LOCTEXT("Nodes", "WAKE THE RESONANCE NODES");
	Nodes->FinishSpawning(Nodes->GetActorTransform());
	for (int32 i = 0; i < 3; ++i)
	{
		const float A = PI * 0.3f + i * PI * 2.f / 3.f;
		const FVector Stub = Plateau + FVector(FMath::Cos(A) * 800.f, FMath::Sin(A) * 800.f, 0.f);
		Platform(Stub, PlateauTop + 180.f, FVector(320.f, 320.f, 180.f), RelayStone, 0.6f);
		ATDInteractableActor* Node = SpawnProp<ATDInteractableActor>(Stub + FVector(0.f, 0.f, PlateauTop + 180.f + 70.f));
		Node->Prompt = LOCTEXT("NodePrompt", "Wake the node");
		Node->LinkedObjective = Relay;
		Node->LinkedStep = 1;
		Node->bCompleteLinkedStep = false;
		Node->FinishSpawning(Node->GetActorTransform());
		UTDStatics::TintPlaceholder(Node->Mesh, Magenta, 2.f);
		Nodes->WatchInteractable(Node);
	}
	ATDEncounterVolume* Drones = Encounter(Relay, INDEX_NONE, Plateau + FVector(0.f, 0.f, PlateauTop + 400.f), FVector(1400.f, 1400.f, 600.f),
		{ ETDCrewType::Drone, ETDCrewType::Drone, ETDCrewType::Drone }, false, 900.f);
	StepEncounters.Add({ Relay, 1, Drones });

	ATDInteractableActor* RelayCore = SpawnProp<ATDInteractableActor>(Plateau + FVector(0.f, 0.f, PlateauTop + 90.f));
	RelayCore->Prompt = LOCTEXT("RelayPrompt", "Awaken the relay");
	RelayCore->HoldTime = 1.5f;
	RelayCore->LinkedObjective = Relay;
	RelayCore->LinkedStep = 2;
	RelayCore->FinishSpawning(RelayCore->GetActorTransform());
	RelayCore->SetActorScale3D(FVector(2.f, 2.f, 1.5f));
	UTDStatics::TintPlaceholder(RelayCore->Mesh, RelayStone, 2.5f);
	Block(EShape::Cylinder, Plateau + FVector(0.f, 0.f, PlateauTop + 1400.f), FVector(80.f, 80.f, 1800.f), RelayStone, 2.f, FRotator::ZeroRotator, false);

	Checkpoint(Relay, 1, Plateau + FVector(0.f, 0.f, PlateauTop), 0.f); // no re-climb for a death on the plateau
	FinishObjective(Relay,
		{ LOCTEXT("R0", "CLIMB TO THE ANCIENT RELAY"), LOCTEXT("R1", "WAKE THE RESONANCE NODES  0 / 3"), LOCTEXT("R2", "AWAKEN THE RELAY · HOLD [E]") },
		{ Climb, Nodes, RelayCore });

	// Dressing: engineering's props and trees (the placeholder flora only if they are missing).
	KeepOuts = {
		{ WreckSite, FVector(2800.f, 2800.f, 0.f) },
		{ FVector(7400.f, -6700.f, 0.f), FVector(2400.f, 1600.f, 0.f) },  // the climb: jump and dash lines
		{ FVector(9200.f, -8200.f, 0.f), FVector(2000.f, 2000.f, 0.f) },  // the plateau
		{ Ambush, FVector(1700.f, 1700.f, 0.f) },
	};
	MarkRoutes(TEXT("SM_Verd_Mushroom_03"), 1600.f, 700.f, FVector2D(0.8f, 1.1f)); // tall glowing trail markers
	const FVector Mid(5000.f, 0.f, 0.f);
	const bool bProps = ScatterMeshes({
		// A canopy ring around the play space; a few giants as landmarks.
		{ TEXT("SM_TDVeg_Kapok_S100_11"), 10, FVector2D(0.9f, 1.1f), false, 1500.f, Mid, FVector2D(17000.f, 24000.f), 20.f },
		{ TEXT("SM_TDVeg_BrazilNut_S100_21"), 18, FVector2D(0.9f, 1.15f), false, 1500.f, Mid, FVector2D(15000.f, 24000.f), 20.f },
		{ TEXT("SM_TDVeg_TrumpetTree_S100_31"), 30, FVector2D(0.85f, 1.2f), false, 1200.f, Mid, FVector2D(12000.f, 22000.f), 10.f },
		{ TEXT("SM_TDVeg_StiltPalm_S100_41"), 30, FVector2D(0.8f, 1.2f), false, 1000.f, Mid, FVector2D(9000.f, 20000.f), 10.f },
		{ TEXT("SM_TDVeg_Kapok_S070_12"), 3, FVector2D(1.f, 1.f), false, 2500.f, FVector(6000.f, 0.f, 0.f), FVector2D(5000.f, 9000.f), 20.f },
		// The mid layer inside the play space.
		{ TEXT("SM_TDVeg_TreeFern_S100_51"), 40, FVector2D(0.8f, 1.3f), false, 900.f, Mid, FVector2D(3800.f, 12000.f), 5.f },
		{ TEXT("SM_TDVeg_Cacao_S100_61"), 30, FVector2D(0.8f, 1.2f), false, 900.f, Mid, FVector2D(3800.f, 13000.f), 5.f },
		{ TEXT("SM_Verd_Mushroom_02"), 40, FVector2D(0.7f, 1.3f), false, 500.f, Mid, FVector2D(3800.f, 12000.f), 5.f },
		{ TEXT("SM_Verd_Mushroom_01"), 70, FVector2D(0.7f, 1.4f), false, 350.f, Mid, FVector2D(3800.f, 12000.f), 3.f },
		{ TEXT("SM_Verd_Fern_02"), 120, FVector2D(0.8f, 1.4f), false, 300.f, Mid, FVector2D(3800.f, 13000.f), 3.f },
		{ TEXT("SM_Verd_Fern_01"), 140, FVector2D(0.8f, 1.5f), false, 250.f, Mid, FVector2D(3800.f, 13000.f), 3.f },
		{ TEXT("SM_Verd_GlowPlant_01"), 160, FVector2D(0.8f, 1.6f), false, 200.f, Mid, FVector2D(3800.f, 12000.f), 2.f },
		{ TEXT("SM_Verd_GlowPlant_02"), 160, FVector2D(0.8f, 1.6f), false, 200.f, Mid, FVector2D(3800.f, 12000.f), 2.f },
		// Rock clusters: the big ones collide, well off the routes.
		{ TEXT("SM_Verd_Rock_03"), 18, FVector2D(0.8f, 1.4f), true, 1400.f, Mid, FVector2D(4000.f, 15000.f), 30.f },
		{ TEXT("SM_Verd_Rock_02"), 30, FVector2D(0.8f, 1.3f), true, 1100.f, Mid, FVector2D(4000.f, 15000.f), 20.f },
		{ TEXT("SM_Verd_Rock_01"), 60, FVector2D(0.8f, 1.5f), false, 400.f, Mid, FVector2D(3800.f, 14000.f), 10.f },
	}, 0x5EED25);
	if (!bProps)
	{
		ScatterFlora(140, 16000.f, 0x5EED25);
	}
}

// ------------------------------------------------------------------------------------------------
// Cinder (ENG-26)
// ------------------------------------------------------------------------------------------------

void ATDPlanetCourse::BuildCinder()
{
	using namespace TDCourseColors;
	Block(EShape::Cube, FVector(0.f, 0.f, -100.f), FVector(600000.f, 600000.f, 200.f), CinderGround);

	auto Magma = [this](const FVector& Center, const FVector& Size)
	{
		ATDMagmaVolume* M = SpawnProp<ATDMagmaVolume>(Center);
		if (M)
		{
			M->FinishSpawning(M->GetActorTransform());
			M->SetSize(Size);
		}
		return M;
	};
	auto Geyser = [this](const FVector& At, float Phase)
	{
		ATDGeyser* G = SpawnProp<ATDGeyser>(At);
		if (G)
		{
			G->Phase = Phase;
			G->FinishSpawning(G->GetActorTransform());
		}
		return G;
	};

	// ---- Obj 3: the outpost (-> EMP Pulse on Lancer). North-east, over a magma river. ----
	const FVector OutHead(2600.f, 2600.f, 0.f);
	ATDObjective* Outpost = MakeObjective(TEXT("Obj_Outpost"), LOCTEXT("OutpostTitle", "The Outpost"), ETDAbilityId::EMPPulse, ETDShipId::Lancer, 6);
	AddTrailhead(Outpost, OutHead, Ember, LOCTEXT("TheOutpost", "THE OUTPOST"));
	Route({ FVector(900.f, 900.f, 0.f), OutHead, FVector(5000.f, 4200.f, 0.f), FVector(7600.f, 6000.f, 0.f), FVector(9200.f, 6000.f, 0.f) }, Ember);

	// The magma river with basalt stepping stones (3 m gaps: a jump).
	Magma(FVector(5600.f, 4800.f, 0.f), FVector(1600.f, 7000.f, 300.f));
	for (int32 i = 0; i < 3; ++i)
	{
		Platform(FVector(5050.f + i * 550.f, 4400.f + i * 300.f, 0.f), 60.f, FVector(280.f, 280.f, 160.f), Basalt);
	}
	// A geyser row on the far bank, rippling.
	for (int32 i = 0; i < 3; ++i)
	{
		Geyser(FVector(7000.f + i * 450.f, 5600.f + i * 200.f, 0.f), i * 1.2f);
	}

	// The compound along +X: gate wall, yard, barracks, command deck, Warden arena.
	const float Y0 = 6000.f;
	auto Wall = [this, Y0](float X, float GapHalf)
	{
		// A wall across the compound with a 6 m doorway in the middle (GapHalf = 0: solid).
		const float Half = 1800.f;
		if (GapHalf <= 0.f)
		{
			Block(EShape::Cube, FVector(X, Y0, 300.f), FVector(150.f, Half * 2.f, 600.f), TDCourseColors::Steel);
			return;
		}
		const float Len = Half - GapHalf;
		Block(EShape::Cube, FVector(X, Y0 + GapHalf + Len * 0.5f, 300.f), FVector(150.f, Len, 600.f), TDCourseColors::Steel);
		Block(EShape::Cube, FVector(X, Y0 - GapHalf - Len * 0.5f, 300.f), FVector(150.f, Len, 600.f), TDCourseColors::Steel);
		Block(EShape::Cube, FVector(X, Y0, 580.f), FVector(160.f, GapHalf * 2.f, 40.f), TDCourseColors::ArmadaRed, 1.5f, FRotator::ZeroRotator, false);
	};
	const float XGate = 9600.f, XBarracks = 11800.f, XCommand = 13800.f, XArena = 15800.f, XEnd = 18200.f;
	for (const float X : { XGate, XBarracks, XCommand, XArena })
	{
		Wall(X, 300.f);
	}
	Wall(XEnd, 0.f);
	// Side walls.
	for (const float S : { -1.f, 1.f })
	{
		Block(EShape::Cube, FVector((XGate + XEnd) * 0.5f, Y0 + S * 1800.f, 300.f), FVector(XEnd - XGate, 150.f, 600.f), Steel);
	}
	auto Door = [this, Y0](float X)
	{
		ATDDoor* D = SpawnProp<ATDDoor>(FVector(X, Y0, 0.f));
		if (D)
		{
			D->OpenHeight = 650.f;
			D->FinishSpawning(D->GetActorTransform());
			D->Mesh->SetRelativeScale3D(FVector(0.3f, 6.2f, 6.f));
			D->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
			UTDStatics::TintPlaceholder(D->Mesh, TDCourseColors::ArmadaRed, 0.4f);
		}
		return D;
	};
	ATDDoor* Gate = Door(XGate);
	ATDDoor* DoorB = Door(XBarracks);
	ATDDoor* DoorC = Door(XCommand);
	ATDDoor* DoorD = Door(XArena);

	// Gate conduits: two glowing red pylons beside the gate, slash them down.
	ATDStepCounter* Conduits = SpawnProp<ATDStepCounter>(FVector(XGate - 200.f, Y0, 0.f));
	Conduits->LinkedObjective = Outpost;
	Conduits->LinkedStep = 0;
	Conduits->Required = 2;
	Conduits->BaseText = LOCTEXT("Conduits", "BREACH THE GATE · DESTROY THE CONDUITS");
	Conduits->FinishSpawning(Conduits->GetActorTransform());
	for (const float S : { -1.f, 1.f })
	{
		ATDShatterCrystal* C = SpawnProp<ATDShatterCrystal>(FVector(XGate - 300.f, Y0 + S * 650.f, 0.f));
		C->LinkedObjective = Outpost;
		C->LinkedStep = 0;
		C->Color = FLinearColor(1.f, 0.1f, 0.05f);
		C->MaxHealth = 60.f;
		C->FinishSpawning(C->GetActorTransform());
		C->Mesh->SetStaticMesh(CylinderMesh);
		C->Mesh->SetRelativeScale3D(FVector(1.f, 1.f, 3.f));
		C->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
		Conduits->WatchDestructible(C);
	}
	ATDEncounterVolume* GateGuards = Encounter(Outpost, INDEX_NONE, FVector(XGate - 1400.f, Y0, 200.f), FVector(1600.f, 1800.f, 600.f),
		{ ETDCrewType::Grunt, ETDCrewType::Trooper, ETDCrewType::Grunt }, true, 700.f);
	StepDoors.Add({ Outpost, 1, Gate });

	// Cover crates inside each room.
	auto Crates = [this, Y0](float X0, float X1, int32 Seed)
	{
		FRandomStream R(Seed);
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector At(FMath::Lerp(X0 + 400.f, X1 - 400.f, R.FRand()), Y0 + R.FRandRange(-1300.f, 1300.f), 0.f);
			Block(EShape::Cube, At + FVector(0.f, 0.f, 70.f), FVector(R.FRandRange(220.f, 360.f), R.FRandRange(150.f, 300.f), 140.f), TDCourseColors::Steel);
		}
	};
	Crates(XGate, XBarracks, 31);
	Crates(XBarracks, XCommand, 32);
	Crates(XCommand, XArena, 33);

	auto RoomFight = [&](float X0, float X1, int32 Step, const TArray<ETDCrewType>& Mix, ATDDoor* OpensDoor)
	{
		ATDEncounterVolume* E = Encounter(Outpost, Step, FVector((X0 + X1) * 0.5f, Y0, 200.f), FVector((X1 - X0) * 0.5f, 1700.f, 600.f), Mix, true, 800.f);
		if (E && OpensDoor)
		{
			E->DoorsToOpen.Add(OpensDoor);
		}
		return E;
	};
	// Escalating, one new threat per room (machine playtest: two troopers plus a heavy beat the pilot five times):
	// grunts + a shooter, then the heavy, then shooters with a drone, then the Warden.
	ATDEncounterVolume* Yard = RoomFight(XGate, XBarracks, 1, { ETDCrewType::Grunt, ETDCrewType::Grunt, ETDCrewType::Trooper }, DoorB);
	ATDEncounterVolume* Barracks = RoomFight(XBarracks, XCommand, 2, { ETDCrewType::Heavy, ETDCrewType::Grunt, ETDCrewType::Grunt }, DoorC);
	ATDEncounterVolume* Command = RoomFight(XCommand, XArena, 3, { ETDCrewType::Trooper, ETDCrewType::Drone, ETDCrewType::Trooper }, DoorD);
	ATDEncounterVolume* Arena = RoomFight(XArena, XEnd, 4, { ETDCrewType::Warden }, nullptr);

	ATDInteractableActor* Prototype = SpawnProp<ATDInteractableActor>(FVector(XEnd - 700.f, Y0, 80.f));
	Prototype->Prompt = LOCTEXT("ClaimEmp", "Claim the EMP prototype");
	Prototype->HoldTime = 1.5f;
	Prototype->LinkedObjective = Outpost;
	Prototype->LinkedStep = 5;
	Prototype->FinishSpawning(Prototype->GetActorTransform());
	Prototype->SetActorScale3D(FVector(1.6f, 1.6f, 1.4f));
	UTDStatics::TintPlaceholder(Prototype->Mesh, FLinearColor(0.3f, 0.8f, 1.f), 2.5f);

	// Every room is a checkpoint: a death costs that room, not the whole compound (machine playtest: the
	// barracks death sent the pilot back to the gate conduits every time). The Warden too (review N6).
	Checkpoint(Outpost, 1, FVector(XGate - 500.f, Y0, 0.f), 0.f);
	Checkpoint(Outpost, 2, FVector(XBarracks - 500.f, Y0, 0.f), 0.f);
	Checkpoint(Outpost, 3, FVector(XCommand - 500.f, Y0, 0.f), 0.f);
	Checkpoint(Outpost, 4, FVector(XArena - 600.f, Y0, 0.f), 0.f);
	FinishObjective(Outpost,
		{ LOCTEXT("O0", "BREACH THE GATE · DESTROY THE CONDUITS  0 / 2"), LOCTEXT("O1", "CLEAR THE YARD"), LOCTEXT("O2", "CLEAR THE BARRACKS"),
		  LOCTEXT("O3", "CLEAR THE COMMAND DECK"), LOCTEXT("O4", "DEFEAT THE WARDEN"), LOCTEXT("O5", "CLAIM THE EMP PROTOTYPE · HOLD [E]") },
		{ Conduits, Yard, Barracks, Command, Arena, Prototype });
	(void)GateGuards;

	// ---- Obj 4: the jammer (-> reveals the Dominion). South-east, across a magma lake. ----
	const FVector JamHead(2600.f, -2600.f, 0.f);
	ATDObjective* Jammer = MakeObjective(TEXT("Obj_Jammer"), LOCTEXT("JammerTitle", "The Signal Jammer"), ETDAbilityId::None, ETDShipId::Rift, 3);
	AddTrailhead(Jammer, JamHead, Crystal, LOCTEXT("TheJammer", "THE JAMMER"));
	Route({ FVector(900.f, -900.f, 0.f), JamHead, FVector(5200.f, -4600.f, 0.f), FVector(6800.f, -5000.f, 0.f) }, Crystal);

	// A crystal wall across the trail: slash through it.
	for (int32 i = 0; i < 3; ++i)
	{
		ATDShatterCrystal* C = SpawnProp<ATDShatterCrystal>(FVector(4700.f + i * 120.f, -4100.f - i * 300.f, 0.f));
		C->LinkedObjective = Jammer;
		C->LinkedStep = 0;
		C->Color = Crystal;
		C->FinishSpawning(C->GetActorTransform());
	}
	// Rock walls either side of the crystal choke so it can't be walked round.
	Block(EShape::Cube, FVector(4600.f, -3200.f, 300.f), FVector(400.f, 1200.f, 600.f), Basalt, 0.f, FRotator(0.f, -30.f, 0.f));
	Block(EShape::Cube, FVector(5100.f, -5400.f, 300.f), FVector(400.f, 1400.f, 600.f), Basalt, 0.f, FRotator(0.f, -30.f, 0.f));

	// The magma lake and the collapsing crystal bridge: one continuous walkway of slabs (no jumps), each
	// falling 1.1 s after you step on it: keep running.
	const float LakeX0 = 7000.f, LakeX1 = 10400.f, LakeY = -5000.f;
	Magma(FVector((LakeX0 + LakeX1) * 0.5f, LakeY, 0.f), FVector(LakeX1 - LakeX0, 5000.f, 300.f));
	const int32 Segments = 6;
	for (int32 i = 0; i < Segments; ++i)
	{
		const float X = LakeX0 + 250.f + i * ((LakeX1 - LakeX0 - 500.f) / (Segments - 1));
		ATDCollapsingPlatform* P = SpawnProp<ATDCollapsingPlatform>(FVector(X, LakeY + (i % 2 ? 60.f : -60.f), 80.f));
		P->LinkedObjective = Jammer;
		P->FinishSpawning(P->GetActorTransform());
		P->SetSize(FVector((LakeX1 - LakeX0 - 500.f) / (Segments - 1) + 30.f, 320.f, 60.f)); // overlapping: no gaps
	}
	ATDStepTrigger* FarBank = SpawnProp<ATDStepTrigger>(FVector(LakeX1 + 600.f, LakeY, 0.f));
	FarBank->LinkedObjective = Jammer;
	FarBank->LinkedStep = 0;
	FarBank->FinishSpawning(FarBank->GetActorTransform());

	// The jammer tower and its three charge sites, defended.
	const FVector Tower(12600.f, LakeY, 0.f);
	AActor* TowerActor = SpawnProp<AActor>(Tower);
	if (TowerActor)
	{
		USceneComponent* Root = NewObject<USceneComponent>(TowerActor, TEXT("Root"));
		TowerActor->SetRootComponent(Root);
		TowerActor->FinishSpawning(TowerActor->GetActorTransform());
		Root->SetMobility(EComponentMobility::Movable);
		Root->RegisterComponent();
		TowerActor->SetActorLocation(W(Tower));
		auto Part = [this, TowerActor](EShape Shape, const FVector& Rel, const FVector& Size, const FLinearColor& Color, float Glow)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(TowerActor);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(Shape == EShape::Cylinder ? CylinderMesh : Shape == EShape::Cone ? ConeMesh : CubeMesh);
			C->SetupAttachment(TowerActor->GetRootComponent());
			C->SetRelativeLocation(Rel);
			C->SetRelativeScale3D(Size / 100.f);
			C->SetCollisionProfileName(Glow > 0.f ? TEXT("NoCollision") : TEXT("BlockAll"));
			C->RegisterComponent();
			TowerActor->AddInstanceComponent(C);
			UTDStatics::TintPlaceholder(C, Color, Glow);
		};
		Part(EShape::Cylinder, FVector(0.f, 0.f, 1500.f), FVector(500.f, 500.f, 3000.f), TDCourseColors::Steel, 0.f);
		Part(EShape::Cone, FVector(0.f, 0.f, 3400.f), FVector(400.f, 400.f, 800.f), TDCourseColors::ArmadaRed, 1.5f);
		Part(EShape::Cube, FVector(0.f, 0.f, 2600.f), FVector(1600.f, 80.f, 80.f), TDCourseColors::ArmadaRed, 2.5f);
		Part(EShape::Cube, FVector(0.f, 0.f, 2200.f), FVector(80.f, 1400.f, 80.f), TDCourseColors::ArmadaRed, 2.5f);
	}
	ATDStepCounter* Charges = SpawnProp<ATDStepCounter>(Tower);
	Charges->LinkedObjective = Jammer;
	Charges->LinkedStep = 1;
	Charges->Required = 3;
	Charges->BaseText = LOCTEXT("Charges", "PLANT THE CHARGES");
	Charges->FinishSpawning(Charges->GetActorTransform());
	for (int32 i = 0; i < 3; ++i)
	{
		const float A = PI + (i - 1) * 0.9f;
		ATDInteractableActor* Site = SpawnProp<ATDInteractableActor>(Tower + FVector(FMath::Cos(A) * 520.f, FMath::Sin(A) * 520.f, 60.f));
		Site->Prompt = LOCTEXT("Plant", "Plant a charge");
		Site->HoldTime = 1.5f;
		Site->LinkedObjective = Jammer;
		Site->LinkedStep = 1;
		Site->bCompleteLinkedStep = false;
		Site->FinishSpawning(Site->GetActorTransform());
		UTDStatics::TintPlaceholder(Site->Mesh, FLinearColor(1.f, 0.85f, 0.1f), 2.f);
		Charges->WatchInteractable(Site);
	}
	ATDEncounterVolume* Defence = Encounter(Jammer, INDEX_NONE, Tower + FVector(0.f, 0.f, 200.f), FVector(2000.f, 2000.f, 600.f),
		{ ETDCrewType::Trooper, ETDCrewType::Grunt, ETDCrewType::Grunt, ETDCrewType::Drone }, false, 900.f); // (machine playtest: 5 on a 15 m ring spawned on top of the retake point)
	StepEncounters.Add({ Jammer, 1, Defence });

	// The run: back along the lake's south shore (solid ground), debris falling, out of the tower's reach.
	const FVector Exit(LakeX0 - 600.f, LakeY - 3400.f, 0.f);
	Platform(FVector((LakeX0 + LakeX1 + 1600.f) * 0.5f, LakeY - 3300.f, 0.f), 30.f, FVector(LakeX1 - LakeX0 + 2400.f, 1200.f, 30.f), Basalt);
	ATDStepTrigger* Clear = SpawnProp<ATDStepTrigger>(Exit);
	Clear->LinkedObjective = Jammer;
	Clear->LinkedStep = 2;
	Clear->FinishSpawning(Clear->GetActorTransform());
	Clear->Box->SetBoxExtent(FVector(900.f, 900.f, 400.f));
	ATDCollapseRun* Run = SpawnProp<ATDCollapseRun>(FVector((LakeX0 + LakeX1) * 0.5f + 1500.f, LakeY - 2400.f, 0.f));
	Run->LinkedObjective = Jammer;
	Run->LinkedStep = 2;
	Run->Seconds = 22.f;
	Run->Tower = TowerActor;
	Run->DebrisExtent = FVector(3500.f, 1200.f, 0.f);
	Run->FinishSpawning(Run->GetActorTransform());
	Routes.Add({ Tower, FVector(LakeX1 + 800.f, LakeY - 3300.f, 0.f), Exit });

	Checkpoint(Jammer, 1, FVector(LakeX1 + 250.f, LakeY, 0.f), 0.f); // the charges: from the far bank, out of the defence's drop
	FinishObjective(Jammer,
		{ LOCTEXT("J0", "CROSS THE CRYSTAL BRIDGE"), LOCTEXT("J1", "PLANT THE CHARGES  0 / 3"), LOCTEXT("J2", "GET CLEAR OF THE TOWER") },
		{ FarBank, Charges, Clear });

	KeepOuts = {
		{ FVector(5600.f, 4800.f, 0.f), FVector(1100.f, 3700.f, 0.f) },   // the magma river and stones
		{ FVector(7400.f, 5800.f, 0.f), FVector(900.f, 800.f, 0.f) },     // the geysers
		{ FVector(13900.f, 6000.f, 0.f), FVector(4800.f, 2300.f, 0.f) },  // the outpost compound and its approach
		{ FVector(8700.f, -5000.f, 0.f), FVector(2600.f, 3000.f, 0.f) },  // the magma lake and bridge
		{ FVector(12600.f, -5000.f, 0.f), FVector(2400.f, 2400.f, 0.f) }, // the jammer tower fight
		{ FVector(9000.f, -8300.f, 0.f), FVector(4500.f, 900.f, 0.f) },   // the escape run
		{ FVector(4900.f, -4300.f, 0.f), FVector(900.f, 1500.f, 0.f) },   // the crystal wall
	};
	const FVector Mid(6000.f, 0.f, 0.f);
	const bool bProps = ScatterMeshes({
		// Colour tells the way: amber crystals toward the outpost (north-east), cyan toward the jammer (south-east).
		{ TEXT("SM_Cind_Crystal_04"), 10, FVector2D(0.8f, 1.2f), false, 900.f, FVector(8000.f, 5500.f, 0.f), FVector2D(1500.f, 6000.f), 20.f },
		{ TEXT("SM_Cind_Crystal_02"), 70, FVector2D(0.8f, 1.6f), false, 450.f, FVector(5500.f, 4000.f, 0.f), FVector2D(500.f, 5000.f), 10.f },
		{ TEXT("SM_Cind_Crystal_04"), 8, FVector2D(0.9f, 1.3f), false, 700.f, FVector(3500.f, 3000.f, 0.f), FVector2D(800.f, 2500.f), 20.f }, // at the outpost trailhead
		{ TEXT("SM_Cind_Crystal_03"), 10, FVector2D(0.8f, 1.2f), false, 900.f, FVector(8000.f, -5500.f, 0.f), FVector2D(1500.f, 6000.f), 20.f },
		{ TEXT("SM_Cind_Crystal_01"), 70, FVector2D(0.8f, 1.6f), false, 450.f, FVector(5500.f, -4000.f, 0.f), FVector2D(500.f, 5000.f), 10.f },
		{ TEXT("SM_Cind_Crystal_03"), 8, FVector2D(0.9f, 1.3f), false, 700.f, FVector(3500.f, -3000.f, 0.f), FVector2D(800.f, 2500.f), 20.f }, // at the jammer trailhead
		// Basalt columns as canyon walls beside the routes, and a tall outer ring.
		{ TEXT("SM_Cind_BasaltColumn_02"), 40, FVector2D(0.9f, 1.6f), true, 1300.f, Mid, FVector2D(3800.f, 12000.f), 30.f },
		{ TEXT("SM_Cind_BasaltColumn_01"), 60, FVector2D(0.8f, 1.5f), true, 1100.f, Mid, FVector2D(3800.f, 12000.f), 20.f },
		{ TEXT("SM_Cind_BasaltColumn_02"), 60, FVector2D(1.5f, 3.f), true, 1500.f, Mid, FVector2D(14000.f, 24000.f), 30.f },
		{ TEXT("SM_Cind_Basalt_03"), 30, FVector2D(0.8f, 1.4f), true, 1100.f, Mid, FVector2D(3800.f, 14000.f), 30.f },
		{ TEXT("SM_Cind_Basalt_02"), 50, FVector2D(0.8f, 1.3f), false, 700.f, Mid, FVector2D(3800.f, 14000.f), 20.f },
		{ TEXT("SM_Cind_Basalt_01"), 120, FVector2D(0.8f, 1.6f), false, 300.f, Mid, FVector2D(3800.f, 14000.f), 10.f },
	}, 0xC1DE26);
	if (!bProps)
	{
		ScatterFlora(120, 16000.f, 0xC1DE26);
	}
}

#undef LOCTEXT_NAMESPACE

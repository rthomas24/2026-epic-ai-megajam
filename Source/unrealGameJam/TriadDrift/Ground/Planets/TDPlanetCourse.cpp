// Triad Drift - planet layouts (ENG-25 Verdance, ENG-26 Cinder). Owner: Teammate 5.

#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "Components/DecalComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/MeshComponent.h"
#include "TimerManager.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "Engine/PostProcessVolume.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "Components/PointLightComponent.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/Crew/TDFutureCrew.h"
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
#include "Engine/StaticMeshSocket.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Misc/PackageName.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Ground/TDGroundBarks.h"
#include "TriadDrift/Ground/Interior/TDReactorCoreFX.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

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
	static constexpr float OutpostClearRadius = 1200.f; // the 800 x 400 kit room: half diagonal 450, plus the door apron
	static const FVector VerdanceRelaySite(8000.f, -6500.f, 0.f); // not a colour: the relay deck (decision 2), ground level
	// Cinder: basalt, magma orange, crystal violet.
	static const FLinearColor CinderGround(0.05f, 0.035f, 0.035f);
	static const FLinearColor Basalt(0.12f, 0.1f, 0.1f);
	static const FLinearColor MagmaGlow(1.f, 0.3f, 0.04f);
	static const FLinearColor Crystal(0.7f, 0.3f, 1.f);
	static const FLinearColor ArmadaRed(0.55f, 0.06f, 0.04f);
	static const FLinearColor Steel(0.2f, 0.2f, 0.23f);
}

/** A socket of a Grok Bot prop (anchor tower, crystal cradle) in the mesh's own space (Interchange may keep or strip
 *  the SOCKET_ prefix). */
/** A structures-builder piece in Props/Kit/Story under either of its names, or null when it is not in the build. */
static UStaticMesh* TDPlanetCourse_KitStory(const TCHAR* Name, const TCHAR* AltName = nullptr)
{
	for (const TCHAR* N : { Name, AltName })
	{
		if (!N) { continue; }
		const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Story/%s"), N);
		if (FindPackage(nullptr, *Package) || FPackageName::DoesPackageExist(Package))
		{
			if (UStaticMesh* M = TDPropArt::Load(TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("%s.%s"), *Package, N))))) { return M; }
		}
	}
	return nullptr;
}

static bool TDPlanetCourse_AnchorSocket(const UStaticMesh* Mesh, const TCHAR* Name, FVector& OutLocal)
{
	if (!Mesh) { return false; }
	for (const FString& Candidate : { FString(Name), FString(TEXT("SOCKET_")) + Name })
	{
		if (const UStaticMeshSocket* S = Mesh->FindSocket(FName(*Candidate)))
		{
			OutLocal = S->RelativeLocation;
			return true;
		}
	}
	return false;
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
		TEXT("/Game/TriadDrift/Gameplay/Props/Trees/SM_TDVeg_Kapok_S100_11"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Trees/SM_TDVeg_Kapok_S070_12"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Trees/SM_TDVeg_BrazilNut_S100_21"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Trees/SM_TDVeg_TrumpetTree_S100_31"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Trees/SM_TDVeg_StiltPalm_S100_41"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Trees/SM_TDVeg_TreeFern_S100_51"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Trees/SM_TDVeg_Cacao_S100_61"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_03"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Crystal_04"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Basalt_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Basalt_02"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_Basalt_03"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_BasaltColumn_01"),
		TEXT("/Game/TriadDrift/Gameplay/Props/Cinder/SM_Cind_BasaltColumn_02"),
		// Grok Bot's Cinder field anchor (PR #73), imported by ArtSource/EngImport/scripts/ue_import_engai.py
		TEXT("/Game/TriadDrift/Gameplay/Props/CinderAnchor/SM_CinderAnchorTower"),
		TEXT("/Game/TriadDrift/Gameplay/Props/CinderAnchor/SM_CinderAnchorEmitter"),
		// Grok Bot's crystal cache (PR #75), the EMP Pulse objective's last step
		TEXT("/Game/TriadDrift/Gameplay/Props/CrystalCache/SM_CrystalCacheCradle"),
		TEXT("/Game/TriadDrift/Gameplay/Props/CrystalCache/SM_CrystalCacheCrystal")
	};
	for (const TCHAR* Path : PropPaths)
	{
		const FString Short = FPackageName::GetShortName(Path);
		PropRefs.Add(FName(*Short), TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("%s.%s"), Path, *Short))));
	}
	GroundMaterialVerdance = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Verdance.MI_Ground_Verdance")));
	GroundMaterialCinder = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Cinder.MI_Ground_Cinder")));
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
	SiteClears.Reset();
	bKeepOutBuilt = false;
	if (PlanetId == TEXT("Cinder"))
	{
		BuildCinder();
	}
	else if (IsInterior())
	{
		BuildInterior();
	}
	else
	{
		BuildVerdance();
	}
	MarkCompletedFromProgress();
	AdoptTerrainMaterial();
}

void ATDPlanetCourse::AdoptTerrainMaterial()
{
	UWorld* World = GetWorld();
	if (!GroundSlab || !World)
	{
		return;
	}
	// With the procedural terrain (ENG-34), its mesh draws the floor (its basin is flat at exactly z = 0, T4) and the
	// slab only stands under it: hidden, collision kept, so what you see is what you stand on with no z-fight. The
	// terrain material would read the slab's missing vertex colour as white (high, steep rock: review m7), so the slab
	// never wears it. With no terrain the slab keeps MI_Ground_<Planet>.
	bool bTerrain = false;
	for (TActorIterator<ATDPlanetTerrain> It(World); It; ++It)
	{
		GroundSlab->SetHiddenInGame(true);
		bTerrain = true;
		break;
	}
	// #194: T4's relief patch (craters, mounds, gullies) is the ground once it has collision: the flat slab at z 0 goes
	// entirely, or the pilot would stand on an invisible floor over every divot.
	if (bTerrain && ATDPlanetTerrain::HasDetailGround(this))
	{
		GroundSlab->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogTriadDrift, Display, TEXT("#194: relief ground has collision: the course's flat slab is off"));
		return;
	}
	// The terrain and its patch may come a moment after the course: look again for ~30 s.
	if (++TerrainMaterialTries < 120)
	{
		World->GetTimerManager().SetTimer(TerrainMaterialTimer, this, &ATDPlanetCourse::AdoptTerrainMaterial, 0.25f, false);
	}
}


// ------------------------------------------------------------------------------------------------
// Keep-out areas for the world scatter (T4, docs/eng/WORLD-DENSITY-PLAN.md)
// ------------------------------------------------------------------------------------------------

void ATDPlanetCourse::BuildKeepOuts() const
{
	KeepOutCache.Reset();
	bKeepOutBuilt = true;
	const FTransform Xf = GetActorTransform();
	auto AddBox = [this](FName Kind, const FBox& B)
	{
		if (B.IsValid)
		{
			FTDCourseKeepOut K;
			K.Kind = Kind;
			K.Box = B;
			KeepOutCache.Add(K);
		}
	};
	// The pad and the parked squad.
	{
		const FVector C = Xf.GetLocation();
		AddBox(TEXT("Pad"), FBox(C - FVector(PadClearRadius, PadClearRadius, 1000.f), C + FVector(PadClearRadius, PadClearRadius, 1000.f)));
	}
	// The routes between objectives: polylines with the walking width.
	for (const TArray<FVector>& R : Routes)
	{
		FTDCourseKeepOut K;
		K.Kind = TEXT("Route");
		K.HalfWidth = RouteHalfWidth;
		for (const FVector& L : R) { K.Points.Add(Xf.TransformPosition(L)); }
		KeepOutCache.Add(K);
	}
	// The course's own hand-made keep-outs (magma, geysers, the compound, the lake...).
	for (const FKeepOut& O : KeepOuts)
	{
		const FVector C = Xf.TransformPosition(O.Center);
		AddBox(TEXT("Site"), FBox(C - FVector(O.Extent.X, O.Extent.Y, 1000.f), C + FVector(O.Extent.X, O.Extent.Y, 1000.f)));
	}
	// Each walkable kit site's scatter-clear circle (as its bounding square).
	for (const FSiteClear& S : SiteClears)
	{
		const FVector C = Xf.TransformPosition(S.Center);
		AddBox(TEXT("Site"), FBox(C - FVector(S.Radius, S.Radius, 1000.f), C + FVector(S.Radius, S.Radius, 1000.f)));
	}
	// Everything the course spawned that play touches: objective props, interactables, triggers, doors, encounter
	// volumes (and their crew spawn points), with a margin.
	for (const TObjectPtr<AActor>& A : Spawned)
	{
		if (!A || A == Dressing)
		{
			continue;
		}
		if (const ATDEncounterVolume* E = Cast<ATDEncounterVolume>(A))
		{
			if (E->Box)
			{
				AddBox(TEXT("Encounter"), E->Box->Bounds.GetBox());
			}
			const FVector C = E->GetActorLocation();
			TArray<FVector> Points = E->SpawnPoints;
			if (Points.Num() == 0)
			{
				for (int32 i = 0; i < FMath::Max(E->Count, 1); ++i)
				{
					const float Angle = 2.f * PI * i / FMath::Max(E->Count, 1);
					Points.Add(FVector(FMath::Cos(Angle) * E->RingRadius, FMath::Sin(Angle) * E->RingRadius, 0.f));
				}
			}
			for (const FVector& P : Points)
			{
				AddBox(TEXT("Spawn"), FBox(C + P - FVector(250.f, 250.f, 1000.f), C + P + FVector(250.f, 250.f, 1000.f)));
			}
			continue;
		}
		if (A->IsA<ATDObjective>())
		{
			continue; // no shape of its own
		}
		FVector Origin, Extent;
		A->GetActorBounds(false, Origin, Extent);
		if (Extent.IsNearlyZero())
		{
			continue; // markers
		}
		AddBox(TEXT("Site"), FBox(Origin - Extent - FVector(300.f, 300.f, 0.f), Origin + Extent + FVector(300.f, 300.f, 0.f)));
	}
	// The course's own colliding structures (platforms, walls, the station, the relay climb...).
	if (Dressing)
	{
		TInlineComponentArray<UStaticMeshComponent*> Meshes(Dressing);
		for (const UStaticMeshComponent* M : Meshes)
		{
			if (M && M != GroundSlab && M->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				FBox B = M->Bounds.GetBox();
				if (B.GetSize().X > 20000.f || B.GetSize().Y > 20000.f)
				{
					continue; // ground-sized: the slab and the like
				}
				AddBox(TEXT("Structure"), B.ExpandBy(FVector(200.f, 200.f, 0.f)));
			}
		}
	}
}

const TArray<FTDCourseKeepOut>& ATDPlanetCourse::GetKeepOuts() const
{
	if (!bKeepOutBuilt)
	{
		BuildKeepOuts();
	}
	return KeepOutCache;
}

bool ATDPlanetCourse::IsScatterAllowed(const FVector& WorldPoint, float Radius) const
{
	const FVector2D P(WorldPoint.X, WorldPoint.Y);
	for (const FTDCourseKeepOut& K : GetKeepOuts())
	{
		if (K.Points.Num() > 1)
		{
			for (int32 i = 0; i + 1 < K.Points.Num(); ++i)
			{
				const FVector C = FMath::ClosestPointOnSegment(FVector(P, 0.f), FVector(K.Points[i].X, K.Points[i].Y, 0.f), FVector(K.Points[i + 1].X, K.Points[i + 1].Y, 0.f));
				if (FVector2D::Distance(FVector2D(C.X, C.Y), P) < K.HalfWidth + Radius)
				{
					return false;
				}
			}
		}
		else if (K.Box.IsValid)
		{
			if (P.X > K.Box.Min.X - Radius && P.X < K.Box.Max.X + Radius && P.Y > K.Box.Min.Y - Radius && P.Y < K.Box.Max.Y + Radius)
			{
				return false;
			}
		}
	}
	return true;
}

bool ATDPlanetCourse::IsScatterAllowedAt(const UObject* WorldContextObject, const FVector& WorldPoint, float Radius)
{
	const ATDPlanetCourse* Course = Get(WorldContextObject);
	return !Course || Course->IsScatterAllowed(WorldPoint, Radius);
}


// ------------------------------------------------------------------------------------------------
// Grok Bot's survey outpost (the Verdance research site, the Cinder survey station)
// ------------------------------------------------------------------------------------------------

float ATDPlanetCourse::GroundZAt(const FVector& Local) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	FCollisionQueryParams Q(SCENE_QUERY_STAT(TDCourseGround), true);
	for (const TObjectPtr<AActor>& A : Spawned) { if (A) { Q.AddIgnoredActor(A.Get()); } } // the course's own slab and props
	Q.AddIgnoredActor(this);
	FHitResult Hit;
	const FVector P = W(FVector(Local.X, Local.Y, 0.f));
	if (World->LineTraceSingleByChannel(Hit, P + FVector(0.f, 0.f, 20000.f), P - FVector(0.f, 0.f, 5000.f), ECC_WorldStatic, Q))
	{
		return (float)GetActorTransform().InverseTransformPosition(Hit.ImpactPoint).Z; // #194: relief can sit below the old flat
	}
	return 0.f;
}

AActor* ATDPlanetCourse::SpawnKitBP(const TCHAR* Folder, const TCHAR* Name, const FVector& Local, float Yaw, float Scale)
{
	const FString Path = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/%s/%s.%s_C"), Folder, Name, Name);
	UClass* Cls = LoadClass<AActor>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Cls || !GetWorld())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* A = GetWorld()->SpawnActor<AActor>(Cls, FTransform(GetActorRotation() + FRotator(0.f, Yaw, 0.f), W(Local), FVector(Scale)), Params);
	if (A)
	{
		Spawned.Add(A);
	}
	return A;
}

void ATDPlanetCourse::RetintDressing(const FVector& Local, float RadiusCm, const TCHAR* MeshName, const TCHAR* MaterialPath)
{
	// Environment builder (Nov 14): give the dressing's copies of one mesh within a radius another material (the Fab
	// intake's tiling sets on site debris), at runtime: no asset is changed.
	UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!M || !Dressing) { return; }
	const FVector At = W(Local);
	TInlineComponentArray<UStaticMeshComponent*> Parts(Dressing.Get());
	for (UStaticMeshComponent* C : Parts)
	{
		if (C && C->GetStaticMesh() && C->GetStaticMesh()->GetName() == MeshName && FVector::Dist2D(C->GetComponentLocation(), At) <= RadiusCm) { C->SetMaterial(0, M); }
	}
}

// #194 ambient life (environment builder): the effects builder's ambient systems and Fab scorch decals, spawned from the
// dressing. The systems do not cull themselves: every component gets a 60 m draw distance and is deactivated beyond it
// by its own significance (LD max draw distance). One component per spot; a coarse grid so the count stays ~100.
static UNiagaraSystem* TDAmb_Load(const TCHAR* Name)
{
	const FString Path = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/Ambient/%s.%s"), Name, Name);
	return FPackageName::DoesPackageExist(FString::Printf(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/Ambient/%s"), Name))
		? LoadObject<UNiagaraSystem>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn) : nullptr;
}

int32 ATDPlanetCourse::SpawnAmbient(const TCHAR* Name, const FVector& Local, float ZOffset)
{
	UNiagaraSystem* Sys = TDAmb_Load(Name);
	if (!Sys || !Dressing) { return 0; }
	const FVector At = W(FVector(Local.X, Local.Y, GroundZAt(Local) + ZOffset));
	UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAttached(Sys, Dressing->GetRootComponent(), NAME_None, At, FRotator::ZeroRotator,
		EAttachLocation::KeepWorldPosition, false, true, ENCPoolMethod::None, true);
	if (!N) { return 0; }
	N->SetCachedMaxDrawDistance(6000.f);
	N->LDMaxDrawDistance = 6000.f;
	return 1;
}

int32 ATDPlanetCourse::SpawnScorch(const FVector& Local, float SizeCm, float Yaw, bool bCinder)
{
	const TCHAR* Path = (bCinder && int32(Yaw) % 2 == 1) ? TEXT("/Game/TriadDrift/Gameplay/Props/Fab/AshDrift/MI_Fab_AshDrift_Cinder.MI_Fab_AshDrift_Cinder")	// every other Cinder mark: an ash drift
		: bCinder ? TEXT("/Game/TriadDrift/Gameplay/Props/Fab/ScorchMark/MI_Fab_ScorchMark_Cinder.MI_Fab_ScorchMark_Cinder")
		: TEXT("/Game/TriadDrift/Gameplay/Props/Fab/ScorchMark/MI_Fab_ScorchMark_Verdance.MI_Fab_ScorchMark_Verdance");
	UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!M || !Dressing) { return 0; }
	UDecalComponent* D = NewObject<UDecalComponent>(Dressing);
	D->SetupAttachment(Dressing->GetRootComponent());
	D->SetDecalMaterial(M);
	D->DecalSize = FVector(300.f, SizeCm * 0.5f, SizeCm * 0.5f);	// projected along X: pitch -90 turns it straight down
	D->SetWorldLocationAndRotation(W(FVector(Local.X, Local.Y, GroundZAt(Local) + 50.f)), FRotator(-90.f, Yaw, 0.f));
	D->SetFadeScreenSize(0.002f);
	D->RegisterComponent();
	Dressing->AddInstanceComponent(D);
	return 1;
}

void ATDPlanetCourse::DressAmbient(bool bCinder)
{
	int32 n = 0, k = 0;
	// the pad
	n += SpawnAmbient(TEXT("NS_TD_Amb_Shared_PadDust"), FVector(0.f, 0.f, 0.f), 10.f);
	// a 22 m grid over the course box, each spot one effect by planet, kept off the paths and sites
	for (float X = -2600.f; X <= 14000.f; X += 2200.f)
	{
		for (float Y = -10000.f; Y <= 10000.f; Y += 2200.f, ++k)
		{
			const uint32 H = GetTypeHash(FIntPoint(int32(X), int32(Y))) ^ 0x9E3779B9u;
			const float Roll = (H % 1000) / 1000.f;
			const FVector L(X + float((H >> 10) % 1200) - 600.f, Y + float((H >> 20) % 1200) - 600.f, 0.f);
			if (!IsScatterAllowed(W(FVector(L.X, L.Y, GroundZAt(L))), 300.f)) { continue; }
			const TCHAR* Fx = nullptr;
			float Z = 30.f;
			if (!bCinder)
			{
				Fx = Roll < 0.28f ? TEXT("NS_TD_Amb_Verdance_Fireflies") : Roll < 0.48f ? TEXT("NS_TD_Amb_Verdance_Spores") : Roll < 0.6f ? TEXT("NS_TD_Amb_Verdance_Insects")
					: Roll < 0.72f ? TEXT("NS_TD_Amb_Verdance_MushroomPulse") : Roll < 0.78f ? TEXT("NS_TD_Amb_Verdance_GroundMist")
					: Roll < 0.9f ? TEXT("NS_TD_Amb_Verdance_FallingLeaves") : TEXT("NS_TD_Amb_Verdance_GlowPollen");
				if (FCString::Strstr(Fx, TEXT("Leaves"))) { Z = 900.f; }
			}
			else
			{
				Fx = Roll < 0.14f ? TEXT("NS_TD_Amb_Cinder_Vent_S") : Roll < 0.24f ? TEXT("NS_TD_Amb_Cinder_Vent_M") : Roll < 0.29f ? TEXT("NS_TD_Amb_Cinder_Vent_L")
					: Roll < 0.47f ? TEXT("NS_TD_Amb_Cinder_EmberDrift") : Roll < 0.57f ? TEXT("NS_TD_Amb_Cinder_HeatShimmer") : Roll < 0.67f ? TEXT("NS_TD_Amb_Cinder_LavaPop")
					: Roll < 0.84f ? TEXT("NS_TD_Amb_Cinder_FallingAsh") : TEXT("NS_TD_Amb_Cinder_SulfurFumes");
				if (FCString::Strstr(Fx, TEXT("Ash"))) { Z = 1200.f; }
			}
			n += SpawnAmbient(Fx, L, Z);
		}
	}
	// huts: a chimney on every shack / prefab / workshop, a beacon on every lab / mast / flood mast
	if (Dressing)
	{
		TInlineComponentArray<UStaticMeshComponent*> Parts(Dressing.Get());
		for (UStaticMeshComponent* C : Parts)
		{
			if (!C || !C->GetStaticMesh()) { continue; }
			const FString N = C->GetStaticMesh()->GetName();
			if (!N.StartsWith(TEXT("SM_Hut_"))) { continue; }
			const bool bChimney = N.Contains(TEXT("Prefab")) || N.Contains(TEXT("Shack")) || N.Contains(TEXT("Workshop"));
			const bool bBeacon = N.Contains(TEXT("Lab")) || N.Contains(TEXT("Mast"));
			if (!bChimney && !bBeacon) { continue; }
			const FBoxSphereBounds B = C->Bounds;
			const FVector Top = GetActorTransform().InverseTransformPosition(B.Origin + FVector(0.f, 0.f, B.BoxExtent.Z));
			UNiagaraSystem* Sys = TDAmb_Load(bChimney ? TEXT("NS_TD_Amb_Shared_HutChimney") : TEXT("NS_TD_Amb_Shared_HutBeacon"));
			if (!Sys) { continue; }
			if (UNiagaraComponent* Fx = UNiagaraFunctionLibrary::SpawnSystemAttached(Sys, C, NAME_None, W(Top), FRotator::ZeroRotator, EAttachLocation::KeepWorldPosition, false, true, ENCPoolMethod::None, true))
			{
				Fx->SetCachedMaxDrawDistance(6000.f);
				Fx->LDMaxDrawDistance = 6000.f;
				++n;
			}
		}
	}
	// scorch decals: round the outposts and the camps, and at the crash site (Verdance) / the dig (Cinder)
	int32 d = 0;
	const TArray<FVector> Centres = bCinder ? TArray<FVector>{ FVector(3700.f, 0.f, 0.f), FVector(11800.f, 2200.f, 0.f), FVector(6000.f, -4300.f, 0.f) }
		: TArray<FVector>{ FVector(11500.f, 7200.f, 0.f), FVector(12400.f, 5500.f, 0.f), FVector(10800.f, 9500.f, 0.f) };
	for (int32 c = 0; c < Centres.Num(); ++c)
	{
		for (int32 i = 0; i < 5; ++i)
		{
			const float A = (i * 72.f + c * 31.f) * PI / 180.f;
			const FVector L = Centres[c] + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * (900.f + 250.f * (i % 3));
			// decals are flat and walkable: only kept off the route lines (not the site boxes they are meant to mark)
			bool bOnRoute = false;
			for (const TArray<FVector>& Rt : Routes) { for (int32 q = 0; q + 1 < Rt.Num(); ++q) { if (FMath::PointDistToSegment(FVector(L.X, L.Y, 0.f), FVector(Rt[q].X, Rt[q].Y, 0.f), FVector(Rt[q + 1].X, Rt[q + 1].Y, 0.f)) < 400.f) { bOnRoute = true; } } }
			if (bOnRoute) { continue; }
			d += SpawnScorch(L, 400.f + 100.f * ((i + c) % 5), i * 67.f, bCinder);
		}
	}
	UE_LOG(LogTriadDrift, Display, TEXT("Course: ambient %d effects, %d scorch decals (%s)"), n, d, bCinder ? TEXT("Cinder") : TEXT("Verdance"));
}

void ATDPlanetCourse::DressRouteBreaks(bool bCinder)
{
	// #194 (environment builder): ground-break pieces from the structures builder's site set. A walk-under arch or fallen
	// trunk straddling each route at about 40 % of its length (yawed along the path, so the passage lines up), and a
	// landmark (stone ring / overgrown pylon, or gantry / vent chimney / slag heap) every ~40 m across the course box,
	// each at the first spot that clears every keep-out (DressCamp).
	const TCHAR* Folder = bCinder ? TEXT("Huts/Cinder") : TEXT("Huts/Verdance");
	int32 r = 0;
	for (const TArray<FVector>& R : Routes)
	{
		if (R.Num() < 2) { continue; }
		float Total = 0.f;
		for (int32 i = 0; i + 1 < R.Num(); ++i) { Total += FVector::Dist2D(R[i], R[i + 1]); }
		float Want = Total * (0.38f + 0.1f * (r % 3)), Run = 0.f;
		for (int32 i = 0; i + 1 < R.Num(); ++i)
		{
			const float Len = FVector::Dist2D(R[i], R[i + 1]);
			if (Run + Len >= Want)
			{
				const FVector P = FMath::Lerp(R[i], R[i + 1], (Want - Run) / FMath::Max(Len, 1.f));
				const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(R[i + 1].Y - R[i].Y, R[i + 1].X - R[i].X));
				const TCHAR* Piece = bCinder ? TEXT("SM_Site_Cinder_BasaltArch") : (r % 2 == 0 ? TEXT("SM_Site_Verdance_RootArch") : TEXT("SM_Site_Verdance_FallenTrunk"));
				// T7 (relief1 walk): an arch's feet on the walk line jam the pilot: set it 8 m beside the route instead, a framing
				// piece the player passes, not walks into
				const FVector Side = FVector(-(R[i + 1].Y - R[i].Y), R[i + 1].X - R[i].X, 0.f).GetSafeNormal2D() * ((r % 2) ? 800.f : -800.f);
				DressCamp(FVector(P.X, P.Y, 0.f) + Side, 0.f, 1200.f, { { Folder, Piece, FVector::ZeroVector, Yaw, 1.f, true } }, FName(*FString::Printf(TEXT("RouteBreak%d"), r)));
				break;
			}
			Run += Len;
		}
		++r;
	}
	const TCHAR* Marks[] = { TEXT("SM_Site_Verdance_StoneRing"), TEXT("SM_Site_Verdance_OvergrownPylon") };
	const TCHAR* MarksC[] = { TEXT("SM_Site_Cinder_CollapsedGantry"), TEXT("SM_Site_Cinder_VentChimney"), TEXT("SM_Site_Cinder_SlagHeap") };
	int32 m = 0;
	for (float X = -2000.f; X <= 14000.f; X += 4000.f)
	{
		for (float Y = -9000.f; Y <= 9000.f; Y += 4000.f, ++m)
		{
			const TCHAR* Piece = bCinder ? MarksC[m % 3] : Marks[m % 2];
			DressCamp(FVector(X + (m % 3) * 700.f, Y + (m % 2) * 900.f, 0.f), 0.f, 1800.f, { { Folder, Piece, FVector::ZeroVector, float((m * 47) % 360), 1.f, true } },
				FName(*FString::Printf(TEXT("Landmark%d"), m)));
		}
	}
}

int32 ATDPlanetCourse::DressCamp(const FVector& Around, float MinR, float MaxR, const TArray<FDressItem>& Items, FName Tag)
{
	// #194 (environment builder): a group of huts and its yard, placed at the first spot round Around (rings MinR..MaxR,
	// 16 bearings) where every piece's footprint clears every course keep-out (routes, sites, encounters, the pad) with
	// a margin, so nothing stands in a 4 m path corridor. Seated on the ground by Dress(). Returns the pieces placed.
	auto Radius = [](const FDressItem& It)
	{
		const FString N(It.Name);
		if (N.Contains(TEXT("Lab")) || N.Contains(TEXT("Workshop")) || N.Contains(TEXT("DrillRig")) || N.Contains(TEXT("Gantry"))) { return 520.f; }
		if (N.Contains(TEXT("Prefab")) || N.Contains(TEXT("Tank")) || N.Contains(TEXT("Shack")) || N.Contains(TEXT("PumpHouse")) || N.Contains(TEXT("Hopper")) || N.Contains(TEXT("StoneRing")) || N.Contains(TEXT("SlagHeap"))) { return 420.f; }
		return N.Contains(TEXT("Dome")) || N.Contains(TEXT("Wall")) || N.Contains(TEXT("Gate")) ? 300.f : 220.f;
	};
	for (float R = MinR; R <= MaxR + 1.f; R += 600.f)
	{
		for (int32 b = 0; b < 16; ++b)
		{
			const float A = FMath::DegreesToRadians(b * 22.5f + R * 0.01f);
			const FVector C = Around + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * R;
			bool bOk = true;
			for (const FDressItem& It : Items)
			{
				const FVector L = C + FVector(It.Local.X, It.Local.Y, 0.f);
				if (!IsScatterAllowed(W(FVector(L.X, L.Y, GroundZAt(L))), Radius(It) + 60.f)) { bOk = false; break; }
				// Lead (#194): the player walks straight at the marker, so nothing of a camp within 6 m of the line from the pad
				// to any objective (each route's end)
				if (It.bCollide)
				{
					FVector Prev = FVector::ZeroVector;
					for (const TArray<FVector>& Rt : Routes)
					{
						if (!Rt.Num()) { continue; }
						const FVector End(Rt.Last().X, Rt.Last().Y, 0.f);
						if (FMath::PointDistToSegment(FVector(L.X, L.Y, 0.f), FVector::ZeroVector, End) < 600.f + Radius(It)
							|| FMath::PointDistToSegment(FVector(L.X, L.Y, 0.f), Prev, End) < 400.f + Radius(It)) { bOk = false; break; }
						Prev = End;
					}
					// ...and 4 m + body off every route's own line (lamp-lit paths), checked directly
					for (const TArray<FVector>& Rt : Routes)
					{
						for (int32 q = 0; q + 1 < Rt.Num() && bOk; ++q)
						{
							if (FMath::PointDistToSegment(FVector(L.X, L.Y, 0.f), FVector(Rt[q].X, Rt[q].Y, 0.f), FVector(Rt[q + 1].X, Rt[q + 1].Y, 0.f)) < 400.f + Radius(It)) { bOk = false; }
						}
						if (!bOk) { break; }
					}
					if (!bOk) { break; }
					// T7: huts 5 m back from any interact point (the camera boom ended in a hut wall at the device)
					for (const TObjectPtr<AActor>& Sp : Spawned)
					{
						if (Sp && Sp->IsA<ATDInteractableActor>() && FVector::Dist2D(GetActorTransform().InverseTransformPosition(Sp->GetActorLocation()), L) < 500.f + Radius(It)) { bOk = false; break; }
					}
					if (!bOk) { break; }
				}
				// T4's relief (Nov 14): a hut needs a flat-enough spot: its footprint's corners within 70 cm of each other
				if (It.bCollide && Radius(It) >= 300.f)
				{
					const float Rr = Radius(It) * 0.7f;
					float Lo = 1e9f, Hi = -1e9f;
					for (const FVector& D : { FVector(Rr, Rr, 0.f), FVector(-Rr, Rr, 0.f), FVector(Rr, -Rr, 0.f), FVector(-Rr, -Rr, 0.f), FVector::ZeroVector })
					{
						const float Z = GroundZAt(L + D);
						Lo = FMath::Min(Lo, Z); Hi = FMath::Max(Hi, Z);
					}
					if (Hi - Lo > 70.f) { bOk = false; break; }
				}
			}
			if (!bOk) { continue; }
			const int32 Placed = Dress(Items, C, 0.f, Tag);
			UE_LOG(LogTriadDrift, Display, TEXT("Course: camp %s at %s (%d of %d pieces)"), *Tag.ToString(), *C.ToCompactString(), Placed, Items.Num());
			return Placed;
		}
	}
	UE_LOG(LogTriadDrift, Warning, TEXT("Course: camp %s found no clear spot round %s"), *Tag.ToString(), *Around.ToCompactString());
	return 0;
}

int32 ATDPlanetCourse::Dress(const TArray<FDressItem>& Items, const FVector& Origin, float Yaw, FName Site)
{
	int32 Placed = 0;
	const FRotator R(0.f, Yaw, 0.f);
	for (const FDressItem& It : Items)
	{
		FVector L = Origin + R.RotateVector(FVector(It.Local.X, It.Local.Y, 0.f));
		L.Z = GroundZAt(L) + It.Local.Z + 3.f; // on the ground (2-3 cm over it; decals carry their own small z)
		const FName Tag(*FString::Printf(TEXT("%s:%s"), *Site.ToString(), It.Name));
		if (FCString::Strncmp(It.Name, TEXT("BP_"), 3) == 0)
		{
			if (AActor* A = SpawnKitBP(It.Folder, It.Name, L, Yaw + It.Yaw, It.Scale))
			{
				FVector O, E;
				A->GetActorBounds(true, O, E);
				ClearSite(GetActorTransform().InverseTransformPosition(O), FMath::Min(FMath::Max(E.X, E.Y), 3000.f) + 200.f);
				DressLog.Add(Tag);
				++Placed;
			}
			continue;
		}
		const FString Path = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/%s/%s"), It.Folder, It.Name);
		UStaticMesh* M = FPackageName::DoesPackageExist(Path) ? LoadObject<UStaticMesh>(nullptr, *(Path + TEXT(".") + It.Name), nullptr, LOAD_Quiet | LOAD_NoWarn) : nullptr;
		if (!M || !Dressing)
		{
			continue;
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Dressing);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(M);
		C->SetupAttachment(Dressing->GetRootComponent());
		C->SetRelativeTransform(FTransform(FRotator(0.f, Yaw + It.Yaw, 0.f), L, FVector(It.Scale)));
		if (It.bCollide) { C->SetCollisionProfileName(TEXT("BlockAll")); C->CanCharacterStepUpOn = ECB_No; }
		else { C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
		C->SetCanEverAffectNavigation(false);
		C->RegisterComponent();
		Dressing->AddInstanceComponent(C);
		ClearSite(L, FMath::Min(150.f + (float)M->GetBounds().SphereRadius * It.Scale, 1500.f));
		DressLog.Add(Tag);
		++Placed;
	}
	return Placed;
}

void ATDPlanetCourse::DressOutpost(const FVector& Site, float Yaw, FName Tag)
{
	const TCHAR* Out = TEXT("OutpostDressing");
	Dress({
		{ Out, TEXT("SM_Outpost_CargoCrate"), FVector(520.f, 320.f, 0.f), 12.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_CargoCrate"), FVector(560.f, 395.f, 0.f), -8.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_SupplyCrate"), FVector(470.f, 410.f, 0.f), 30.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_SupplyCrate"), FVector(610.f, 300.f, 0.f), 75.f, 0.9f, true },
		{ Out, TEXT("SM_Outpost_FieldTerminal"), FVector(640.f, -330.f, 0.f), 90.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_Floodlight"), FVector(450.f, -440.f, 0.f), 150.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_CommAntenna"), FVector(-150.f, 470.f, 0.f), 0.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_CommsDish"), FVector(-320.f, 500.f, 0.f), 200.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_Generator"), FVector(-520.f, -260.f, 0.f), 90.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_PowerJunction"), FVector(-470.f, -400.f, 0.f), 0.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_BollardLight"), FVector(760.f, 260.f, 0.f), 0.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_BollardLight"), FVector(760.f, -260.f, 0.f), 0.f, 1.f, true },
	}, Site, Yaw, Tag);
	// Nicholas's recoloured Fab props (batch BJ; environment builder): dressing at 1.0 against the outpost's sides, away from
	// the ramp (+X between the bollards) and the door; one blocking hull each, never stood on.
	const TCHAR* Crate = TEXT("Fab/CrateStack");
	const TCHAR* Barrel = TEXT("Fab/OilBarrel");
	const TCHAR* Core = TEXT("Fab/PowerCore");
	if (Tag == TEXT("CinderOutpost"))
	{
		Dress({
			{ Crate, TEXT("SM_Fab_CrateStack"), FVector(300.f, 600.f, 0.f), 8.f, 1.f, true },
			{ Crate, TEXT("SM_Fab_CrateStack"), FVector(110.f, 650.f, 0.f), -12.f, 1.f, true },
			{ Barrel, TEXT("SM_Fab_OilBarrel"), FVector(-660.f, -170.f, 0.f), 0.f, 1.f, true },	// by the generator and the junction
			{ Barrel, TEXT("SM_Fab_OilBarrel"), FVector(-720.f, -250.f, 0.f), 40.f, 1.f, true },
			{ Barrel, TEXT("SM_Fab_OilBarrel"), FVector(-650.f, -320.f, 0.f), 75.f, 1.f, true },
			{ Barrel, TEXT("SM_Fab_OilBarrel"), FVector(-610.f, -540.f, 0.f), 15.f, 1.f, true },
			{ Barrel, TEXT("SM_Fab_OilBarrel"), FVector(-530.f, -600.f, 0.f), 60.f, 1.f, true },
			{ Core, TEXT("SM_Fab_PowerCore"), FVector(-330.f, -540.f, 0.f), 90.f, 1.f, true },
			// intake (Nov 14): a Mechanic Base generator beyond the barrels, a pillar console by the floodlight, a spare truss
			// segment lying on the +Y side, and concrete rubble round the generators (retinted below)
			{ TEXT("Fab/MechanicBase/Mesh/SM/PROP/MACHINE"), TEXT("SM_POWER_GENERATOR_01"), FVector(-950.f, -380.f, 0.f), 90.f, 1.f, true },
			{ TEXT("Fab/MechanicBase/Mesh/SM/PROP/MACHINE"), TEXT("SM_ELEVATOR_CONSOLE_01"), FVector(560.f, -600.f, 87.f), 200.f, 1.f, true },
			{ TEXT("Fab/PipeBeam"), TEXT("SM_Fab_PipeBeam"), FVector(-300.f, 950.f, 0.f), 90.f, 1.f, true },
			{ TEXT("Planet"), TEXT("SM_Cind_DebrisChunk"), FVector(-820.f, -120.f, -10.f), 30.f, 0.9f, false },
			{ TEXT("Planet"), TEXT("SM_Cind_DebrisChunk"), FVector(-760.f, -660.f, -10.f), 140.f, 0.8f, false },
			{ TEXT("Planet"), TEXT("SM_Cind_DebrisChunk"), FVector(-430.f, -700.f, -10.f), 250.f, 0.7f, false },
		}, Site, Yaw, Tag);
		RetintDressing(Site, 1600.f, TEXT("SM_Cind_DebrisChunk"), TEXT("/Game/TriadDrift/Gameplay/Props/Fab/Textures/ConcreteRubble/MI_Fab_ConcreteRubble.MI_Fab_ConcreteRubble"));
	}
	else if (Tag == TEXT("VerdanceOutpost"))
	{
		Dress({
			{ Crate, TEXT("SM_Fab_CrateStack"), FVector(300.f, 600.f, 0.f), 8.f, 1.f, true },
			{ Core, TEXT("SM_Fab_PowerCore"), FVector(-660.f, -330.f, 0.f), 90.f, 1.f, true },
			{ TEXT("Fab/MechanicBase/Mesh/SM/PROP/MACHINE"), TEXT("SM_ELEVATOR_CONSOLE_01"), FVector(-500.f, -540.f, 87.f), 160.f, 1.f, true },	// intake
		}, Site, Yaw, Tag);
	}
}

void ATDPlanetCourse::DressPad(FName Tag)
{
	const TCHAR* Out = TEXT("OutpostDressing");
	TArray<FDressItem> Items;
	for (int32 i = 0; i < 8; ++i)
	{
		const float A = 22.5f + i * 45.f;
		Items.Add({ Out, TEXT("SM_Outpost_BollardLight"), FVector(FMath::Cos(FMath::DegreesToRadians(A)), FMath::Sin(FMath::DegreesToRadians(A)), 0.f) * 1550.f, A + 180.f, 1.f, true });
	}
	Items.Append({
		{ Out, TEXT("SM_Outpost_Floodlight"), FVector(-1900.f, 1200.f, 0.f), -35.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_Floodlight"), FVector(-1900.f, -1200.f, 0.f), 35.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_CargoCrate"), FVector(300.f, -1900.f, 0.f), 15.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_SupplyCrate"), FVector(420.f, -2020.f, 0.f), -20.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_Generator"), FVector(-200.f, -2000.f, 0.f), 0.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_PowerJunction"), FVector(-380.f, -1880.f, 0.f), 90.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_CommsDish"), FVector(-1700.f, 1900.f, 0.f), 225.f, 1.f, true },
		{ Out, TEXT("SM_Outpost_CommAntenna"), FVector(-1500.f, 2050.f, 0.f), 0.f, 1.f, true },
	});
	Dress(Items, FVector::ZeroVector, 0.f, Tag);
}

AActor* ATDPlanetCourse::PlaceOutpost(FVector& Site, float Yaw, bool bCinder, UStaticMeshComponent*& OutConsole)
{
	OutConsole = nullptr;
	Site.Z = GroundZAt(Site) > 0.f ? GroundZAt(Site) + 3.f : Site.Z; // over T4's terrain, never under it (#42)
	ClearSite(Site, TDCourseColors::OutpostClearRadius); // the room (or its placeholder) and its door, whichever is built
	const TCHAR* Path = bCinder ? TEXT("/Game/TriadDrift/Gameplay/Props/Outpost/BP_Outpost_Site_Cinder.BP_Outpost_Site_Cinder_C")
		: TEXT("/Game/TriadDrift/Gameplay/Props/Outpost/BP_Outpost_Site_Verdance.BP_Outpost_Site_Verdance_C");
	UClass* Cls = LoadClass<AActor>(nullptr, Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Cls)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Not sunk, not scaled (the manifest): its pivot is on the ground under the deck centre.
	AActor* A = GetWorld()->SpawnActor<AActor>(Cls, FTransform(GetActorRotation() + FRotator(0.f, Yaw, 0.f), W(Site)), Params);
	if (!A)
	{
		return nullptr;
	}
	Spawned.Add(A);
	TInlineComponentArray<UStaticMeshComponent*> Meshes(A);
	for (UStaticMeshComponent* M : Meshes)
	{
		if (M && M->GetStaticMesh() && M->GetStaticMesh()->GetName().Contains(TEXT("Console")))
		{
			OutConsole = M;
			M->CanCharacterStepUpOn = ECB_No; // no stepping or mantling onto a console (Lead, Nov 13)
		}
	}
	SiteOutpost = A;
	return A;
}

static const FVector OutpostConsoleCentre(-353.f, 0.f, 53.3f);
static const FVector OutpostConsoleScale(74.f / 60.f, 166.f / 60.f, 106.6f / 120.f);

template <class T>
T* ATDPlanetCourse::OutpostTerminal(const FVector& Site, float Yaw)
{
	// The kit outpost (structures builder, 22f47909; Outpost_Site_*.manifest.json): SM_Kit_Console at site (-360, 0, 0),
	// its box X -30..44, Y -83..83, Z 0..106.6 (centre (-353, 0, 53.3)); the pilot stands at (-250, 0, 0) facing yaw 180.
	const FVector At = Site + FRotator(0.f, Yaw, 0.f).RotateVector(OutpostConsoleCentre);
	T* C = SpawnProp<T>(At, FRotator(0.f, Yaw, 0.f));
	return C;
}

void ATDPlanetCourse::HandleTerminalScreen(AActor* Interactable, AActor* Interactor)
{
	const TObjectPtr<UStaticMeshComponent>* Screen = TerminalScreens.Find(Interactable);
	if (!Screen || !*Screen)
	{
		return;
	}
	// Nicholas's scan on the screen (T2, issue #62); else the screens brighten (the manifest's ScreenPower 6-10).
	const bool bCinder = PlanetId == TEXT("Cinder");
	if (!UTDUIHostSubsystem::ShowScanOnScreen(*Screen, NAME_None, bCinder, 8.f))
	{
		if (UMaterialInstanceDynamic* MID = (*Screen)->CreateDynamicMaterialInstance(0))
		{
			MID->SetScalarParameterValue(TEXT("ScreenPower"), 8.f);
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------------------------------------

UStaticMesh* ATDPlanetCourse::ArtFor(FName Name)
{
	if (Name.IsNone())
	{
		return nullptr;
	}
	if (const TSoftObjectPtr<UStaticMesh>* Set = PropRefs.Find(Name))
	{
		return TDPropArt::Load(*Set);
	}
	return TDPropArt::Load(TDPropArt::Default(*Name.ToString()));
}

UStaticMeshComponent* ATDPlanetCourse::Block(EShape Shape, const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, float Glow, const FRotator& Rot, bool bCollide, FName Art)
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
	TDPropArt::Dress(C, ArtFor(Art));
	return C;
}

UStaticMeshComponent* ATDPlanetCourse::Platform(const FVector& XY, float Top, const FVector& SizeCm, const FLinearColor& Color, float Glow, FName Art)
{
	return Block(EShape::Cube, FVector(XY.X, XY.Y, Top - SizeCm.Z * 0.5f), SizeCm, Color, Glow, FRotator::ZeroRotator, true, Art);
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
				UStaticMeshComponent* Post = Block(EShape::Cylinder, Base + FVector(0.f, 0.f, 60.f), FVector(18.f, 18.f, 120.f), FLinearColor(0.05f, 0.05f, 0.06f), 0.f, FRotator::ZeroRotator, false);
				UStaticMeshComponent* Bulb = Block(EShape::Sphere, Base + FVector(0.f, 0.f, 130.f), FVector(34.f), Color, 3.f, FRotator::ZeroRotator, false);
				if (TDPropArt::Dress(Post, ArtFor(TEXT("SM_RoutePost")), false) && Bulb) { Bulb->SetHiddenInGame(true); } // one mesh: post and bulb
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
		if (P.Size2D() < 3500.f || IsOnRoute(P, 1100.f) || !IsOnGround(P, 600.f))
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
	// The prop's own clearance counts against the keep-outs too (Lead, Nov 13: a kapok placed at a box's edge put its
	// canopy through the outpost room).
	for (const FKeepOut& K : KeepOuts)
	{
		if (FMath::Abs(Local.X - K.Center.X) < K.Extent.X + Clearance && FMath::Abs(Local.Y - K.Center.Y) < K.Extent.Y + Clearance)
		{
			return true;
		}
	}
	for (const FSiteClear& S : SiteClears)
	{
		if (FVector::Dist2D(Local, S.Center) < S.Radius + Clearance)
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
			if (P.Size2D() < 3500.f || IsKeptClear(P, K.Clearance) || !IsOnGround(P, 300.f))
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

namespace TDCourseImpl
{
	/**
	 * Finish a deferred spawn without applying the class's root scale twice. FinishSpawning composes the transform it is
	 * given with the root's template transform; passing GetActorTransform() (which already has that scale) squared it:
	 * the 60 x 60 x 120 console came out 36 x 36 x 144, and the kit console was squeezed to fit it.
	 */
	static void FinishProp(AActor* A)
	{
		if (!A)
		{
			return;
		}
		const AActor* Template = A->GetClass()->GetDefaultObject<AActor>();
		const USceneComponent* TemplateRoot = Template ? Template->GetRootComponent() : nullptr;
		const FVector TemplateScale = TemplateRoot ? TemplateRoot->GetRelativeScale3D() : FVector::OneVector;
		const FVector Scale = A->GetActorScale3D() / TemplateScale.ComponentMax(FVector(KINDA_SMALL_NUMBER));
		A->FinishSpawning(FTransform(A->GetActorQuat(), A->GetActorLocation(), Scale));
	}
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
	TDCourseImpl::FinishProp(Obj);
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
	// Director's mercy on every course fight unless the build sets its own (T7's Cinder play, Nov 14: mercy never
	// logged there): take 2 at 85 % health, take 3 one fewer at 75 %, take 5 two fewer at 60 %.
	{
		FTDRetakeMercyTier M2; M2.FromTake = 2; M2.HealthScale = 0.85f;
		FTDRetakeMercyTier M3; M3.FromTake = 3; M3.HealthScale = 0.75f; M3.DropCrew = 1;
		FTDRetakeMercyTier M5; M5.FromTake = 5; M5.HealthScale = 0.6f; M5.DropCrew = 2;
		E->MercyTiers = { M2, M3, M5 };
	}
	TDCourseImpl::FinishProp(E);
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
		TDCourseImpl::FinishProp(M);
		Root->RegisterComponent();
		M->SetActorTransform(FTransform(GetActorRotation() + FRotator(0.f, Yaw, 0.f), W(At)));
	}
	return M;
}

void ATDPlanetCourse::AddMagmaCrossing(const AActor* MagmaActor, ATDObjective* Objective, int32 Step, const TArray<FVector>& Chain, const FVector& RescueAt, float RescueYaw)
{
	const ATDMagmaVolume* M = Cast<ATDMagmaVolume>(MagmaActor);
	UTDObjectiveSubsystem* Markers = UTDObjectiveSubsystem::Get(this);
	if (!M || !M->Box || !Markers)
	{
		return;
	}
	TArray<AActor*> Waypoints;
	for (const FVector& L : Chain) { Waypoints.Add(Marker(L + FVector(0.f, 0.f, 100.f))); }
	Markers->AddCrossing(M->Box->Bounds.GetBox(), Objective, Step, Waypoints);
	if (!MagmaRescues.ContainsByPredicate([M](const FMagmaRescue& R) { return R.Magma.Get() == M; }))
	{
		MagmaRescues.Add({ M, FTransform(GetActorRotation() + FRotator(0.f, RescueYaw, 0.f), W(RescueAt + FVector(0.f, 0.f, 100.f))) });
	}
}

void ATDPlanetCourse::NoteMagmaDeath(const AActor* Magma)
{
	PendingMagmaDeath = Magma;
}

bool ATDPlanetCourse::GetMagmaRescue(const AActor* Magma, FTransform& Out) const
{
	for (const FMagmaRescue& R : MagmaRescues)
	{
		if (R.Magma.Get() == Magma) { Out = R.Far; return true; }
	}
	return false;
}

bool ATDPlanetCourse::ResolveRespawn(FTransform& InOutWhere)
{
	// Count magma deaths in a row in the same magma; any other death starts over.
	const AActor* Killer = PendingMagmaDeath.Get();
	PendingMagmaDeath.Reset();
	MagmaDeathsInRow = (Killer && Killer == LastMagmaDeath.Get()) ? MagmaDeathsInRow + 1 : (Killer ? 1 : 0);
	LastMagmaDeath = Killer;
	FTransform Far;
	if (MagmaDeathsInRow >= MagmaRescueAfterDeaths && GetMagmaRescue(Killer, Far))
	{
		UE_LOG(LogTriadDrift, Display, TEXT("%s: %d magma deaths in a row in %s, respawning on the far side"), *GetName(), MagmaDeathsInRow, *GetNameSafe(Killer));
		MagmaDeathsInRow = 0;
		LastMagmaDeath.Reset();
		InOutWhere = Far;
		return true;
	}
	return false;
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
	if (IsInterior())
	{
		// Inside the Dominion the doors show the way: the trailhead is only its marker (no totem on the deck).
		AActor* Spot = Marker(At + FVector(0.f, 0.f, 300.f));
		FTDTrailhead T;
		T.Objective = Obj;
		T.Marker = Spot;
		T.Name = Name;
		Trailheads.Add(T);
		return;
	}
	// A glowing totem: a dark plinth, a tall light column and a floating crown.
	UStaticMeshComponent* Plinth = Platform(At, 40.f, FVector(500.f, 500.f, 40.f), FLinearColor(0.05f, 0.05f, 0.06f));
	UStaticMeshComponent* Column = Block(EShape::Cylinder, At + FVector(0.f, 0.f, 260.f), FVector(22.f, 22.f, 440.f), Color, 2.f, FRotator::ZeroRotator, false); // thin: readable without filling the view up close
	UStaticMeshComponent* Crown = Block(EShape::Cone, At + FVector(0.f, 0.f, 600.f), FVector(160.f, 160.f, 200.f), Color, 3.f, FRotator(180.f, 0.f, 0.f), false);
	if (TDPropArt::Dress(Plinth, ArtFor(TEXT("SM_Trailhead")), false)) // one mesh: plinth, column and crown on the plinth's base
	{
		if (Column) { Column->SetHiddenInGame(true); }
		if (Crown) { Crown->SetHiddenInGame(true); }
	}
	AActor* Marker = SpawnProp<AActor>(At + FVector(0.f, 0.f, 300.f));
	if (Marker)
	{
		USceneComponent* Root = NewObject<USceneComponent>(Marker, TEXT("Root"));
		Marker->SetRootComponent(Root);
		TDCourseImpl::FinishProp(Marker);
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
	for (const FTDTrailhead& T : Trailheads)
	{
		if (T.Objective == O && T.After && !T.After->IsComplete())
		{
			return false; // an ordered course (the interior): the rooms come one after the other
		}
		if (T.Objective == O && T.After && T.After == WardenGateObjective.Get() && !IsFinalRecordPlayed())
		{
			return false; // #96: the reactor waits for the final record, an old checkpoint's Warden completion included
		}
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
	if (IsInterior() && WardenGateObjective.IsValid() && WardenGateObjective->IsComplete() && !IsFinalRecordPlayed() && FinalRecordConsole)
	{
		// #96, a checkpoint from before the gate: the Warden is done but the record was never played.
		OutText = LOCTEXT("IdleRecord", "PLAY THE FINAL RECORD");
		OutMarkers.Add(FinalRecordConsole);
		return true;
	}
	TArray<FText> Names;
	for (const FTDTrailhead& T : Trailheads)
	{
		if (T.Objective && !T.Objective->IsComplete() && (!T.After || T.After->IsComplete()))
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

void ATDPlanetCourse::HandleStoryLine(const FTDTransmissionView& Line)
{
	// The drone answers on T3's pulse line (the "three pulses" sound plays from DA_UI_Style there; Nick's slot).
	if (Line.bActive && Line.Id == TEXT("C3_Wren_Answered") && DormantDrone && DroneWakeStart < 0.0)
	{
		DroneWakeStart = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
		UE_LOG(LogTriadDrift, Display, TEXT("Cinematic 03: the relay drone wakes (three pulses, then steady blue)"));
	}
}

void ATDPlanetCourse::TickDroneWake()
{
	if (DroneWakeStart < 0.0 || !DormantDrone || !GetWorld())
	{
		return;
	}
	// T3's light timing: three 0.25 s pulses 0.6 s apart, then steady.
	const float T = (float)(GetWorld()->GetRealTimeSeconds() - DroneWakeStart);
	const float Cycle = 0.6f;
	const bool bSteady = T >= 3.f * Cycle;
	const float Glow = bSteady ? 6.f : (FMath::Fmod(T, Cycle) < 0.25f ? 14.f : 0.4f);
	if (UMaterialInstanceDynamic* MID = DroneOptic ? Cast<UMaterialInstanceDynamic>(DroneOptic->GetMaterial(0)) : nullptr)
	{
		const FLinearColor Blue(0.2f, 0.55f, 1.f);
		MID->SetVectorParameterValue(TEXT("Color"), Blue * Glow);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Blue * Glow);
	}
	// Up off the deck, 40 cm over 2 s, then a slow bob.
	const float Lift = 40.f * FMath::Clamp(T / 2.f, 0.f, 1.f) + (bSteady ? 6.f * FMath::Sin(T * 1.6f) : 0.f);
	DormantDrone->SetWorldLocation(DroneHome + FVector(0.f, 0.f, Lift));
}

void ATDPlanetCourse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickDroneWake();
	InteriorTime += DeltaSeconds;
	TickFinalRecord(DeltaSeconds);
	TickCoreChamber();
	if (UTDGroundBarks* Barks = UTDGroundBarks::Get(this)) { Barks->TickBarks(DeltaSeconds); } // ENG-43's polled barks
	for (const FCoreRing& R : CoreRings) // the reactor's rings turn (and rock a little, under 20 degrees: inside the shield)
	{
		if (UStaticMeshComponent* Ring = R.Ring.Get())
		{
			Ring->SetRelativeRotation(FRotator(R.Tilt * FMath::Sin(InteriorTime * 0.35f), InteriorTime * R.YawSpeed, R.Tilt * 0.5f * FMath::Cos(InteriorTime * 0.27f)));
		}
	}
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
		if (T.Objective && T.Marker && !T.Objective->IsComplete() && (!T.After || T.After->IsComplete())
			&& FVector::Dist2D(T.Marker->GetActorLocation(), P->GetActorLocation()) < TrailheadRadius)
		{
			StartObjectiveAt(T.Objective->ObjectiveId);
			return;
		}
	}
}

void ATDPlanetCourse::HandleStepStarted(ATDObjective* Objective, int32 StepIndex)
{
	FireStepBeats(Objective, StepIndex);
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
	for (const FTDStepDoor& D : CompletionDoors)
	{
		if (D.Objective == Objective && D.Door) { D.Door->Open(); }
	}
	FireCompletionBeats(Objective);
	if (UTDGroundBarks* Barks = UTDGroundBarks::Get(this)) { Barks->NotifyObjectiveDone(); } // ENG-43
	if (IsInterior())
	{
		HandleInteriorObjectiveCompleted(Objective);
		return;
	}
	// The relay wakes: the kit relay's power (its Relay component's slot 0: RelayPower 0.15 -> 1), and a pulse ring over its deck
	if (Objective && Objective->ObjectiveId == TEXT("Obj_Relay") && RelayWake)
	{
		TInlineComponentArray<UMeshComponent*> Meshes(RelayWake.Get());
		for (UMeshComponent* M : Meshes)
		{
			if (M && M->GetName().Contains(TEXT("Relay")))
			{
				if (UMaterialInstanceDynamic* MID = M->CreateDynamicMaterialInstance(0)) { MID->SetScalarParameterValue(TEXT("RelayPower"), 1.f); }
			}
		}
	}
	// (pulse ring) (the reward screen is Nick's).
	if (Objective && Objective->ObjectiveId == TEXT("Obj_Relay"))
	{
		for (int32 i = 0; i < 3; ++i)
		{
			Block(EShape::Sphere, TDCourseColors::VerdanceRelaySite + FVector(0.f, 0.f, 900.f), FVector(1200.f + i * 900.f, 1200.f + i * 900.f, 40.f), TDCourseColors::RelayStone, 2.f - i * 0.5f, FRotator::ZeroRotator, false);
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
			for (const FTDStepDoor& D : CompletionDoors)
			{
				if (D.Objective == O && D.Door && !(D.Door == WardenExitDoor && !IsFinalRecordPlayed())) { D.Door->Open(); }
			}
		}
	}
	if (IsInterior() && WardenExitDoor && WardenGateObjective.IsValid() && WardenGateObjective->IsComplete() && !IsFinalRecordPlayed())
	{
		WardenExitDoor->ShowStatusText(LOCTEXT("RecordFirstLoad", "PLAY THE FINAL RECORD FIRST"));
		UE_LOG(LogTriadDrift, Display, TEXT("#96: checkpoint has the Warden done but no final record: exit shut, the record first"));
	}
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->ApplyCompletedState(); // rooms cleared, doors open, crystals gone
	}
	// #96: a checkpoint taken after the final record (T3's saved story state) keeps the reactor door open.
	if (IsInterior() && UTDStorySubsystem::IsSequenceDoneOrSkipped(this, TEXT("Cinematic08")))
	{
		OpenReactorDoor();
	}
}

// ------------------------------------------------------------------------------------------------
// Verdance (ENG-25)
// ------------------------------------------------------------------------------------------------

void ATDPlanetCourse::BuildVerdance()
{
	using namespace TDCourseColors;
	// Ground: one lit slab, top at the pad (z = 0), the gameplay box plus ~600 m (ENG-34): T4's terrain takes over
	// from its edge. The rig's grey slab is switched off.
	GroundBox = FBox2D(FVector2D(-3100.f, -10600.f), FVector2D(14100.f, 10100.f));
	if (UStaticMeshComponent* Ground = Block(EShape::Cube, FVector(GroundBox.GetCenter(), -100.f), FVector(GroundBox.GetSize(), 200.f), VerdanceGround))
	{
		GroundSlab = Ground;
		if (UMaterialInterface* GroundArt = GroundMaterialVerdance.IsNull() ? nullptr : (FPackageName::DoesPackageExist(GroundMaterialVerdance.ToSoftObjectPath().GetLongPackageName()) ? GroundMaterialVerdance.LoadSynchronous() : nullptr))
		{
			Ground->SetMaterial(0, GroundArt); // world-aligned tiling ground (Lead's MI_Ground_*)
		}
	}

	// ---- Obj 1: the wreck core (-> Gravity Well on Bastion). North-east. ----
	const FVector WreckHead(2600.f, 2600.f, 0.f);
	const FVector Ambush(6200.f, 4600.f, 0.f);
	const FVector WreckSite(11500.f, 7200.f, 0.f);
	ATDObjective* Wreck = MakeObjective(TEXT("Obj_WreckCore"), LOCTEXT("WreckTitle", "Recover the Signal-Analysis Device"), ETDAbilityId::GravityWell, ETDShipId::Bastion, 3);
	AddTrailhead(Wreck, WreckHead, Cyan, LOCTEXT("TheWreck", "THE RESEARCH SITE"));
	Route({ FVector(900.f, 900.f, 0.f), WreckHead, Ambush, FVector(9300.f, 6300.f, 0.f), WreckSite }, Cyan);

	// Ambush: fallen logs for cover, two troopers and two grunts waiting in the glow.
	TArray<AActor*> AmbushCover;
	for (const FVector& L : { FVector(5600.f, 4100.f, 0.f), FVector(6700.f, 5200.f, 0.f), FVector(6900.f, 4000.f, 0.f), FVector(5700.f, 5300.f, 0.f) })
	{
		Block(EShape::Cube, L + FVector(0.f, 0.f, 70.f), FVector(500.f, 110.f, 140.f), Trunk, 0.f, FRotator(0.f, 35.f, 0.f), true, TEXT("SM_Verd_Log"));
	}
	ATDEncounterVolume* AmbushFight = Encounter(Wreck, INDEX_NONE, Ambush + FVector(0.f, 0.f, 200.f), FVector(1500.f, 1500.f, 600.f),
		{ ETDCrewType::Grunt, ETDCrewType::Grunt }, true, 900.f); // the first fight, melee only: the blade before the bolts
	// (T7's Standard play, Nov 14: 5-7 CUTs at the wreck core with a shooter in each of its two fights; Lead's GO.)
	// Director's mercy: take 2 one grunt, take 3 that grunt at 60 % health.
	FTDRetakeMercyTier AmbushMercy2; AmbushMercy2.FromTake = 2; AmbushMercy2.DropCrew = 1;
	FTDRetakeMercyTier AmbushMercy3; AmbushMercy3.FromTake = 3; AmbushMercy3.DropCrew = 1; AmbushMercy3.HealthScale = 0.6f;
	AmbushFight->MercyTiers = { AmbushMercy2, AmbushMercy3 };

	// The survey station (storyline.md section 18, reduced form): one open-sided station, intact and lit, nobody home. A
	// roofed deck on four posts with two low equipment walls, the survey terminal in the middle and a dormant drone in a
	// corner (Cinematic 03 wakes it). Placeholders by name until the builder's pieces land (SM_Verd_Station_*).
	// Grok Bot's outpost when it exists (its bay and ramp turned to the beacon the pilot comes from); else the placeholder.
	// At the fight's edge, not its middle (Played.Verdance: a solid cabin in the middle of the station fight walled the
	// pilot off from the crew), clear of the spawn points and of the line from the checkpoint.
	FVector OutpostSite = WreckSite + FVector(900.f, -1700.f, 0.f);
	const float SiteYaw = FMath::RadiansToDegrees(FMath::Atan2(6300.f - OutpostSite.Y, 9300.f - OutpostSite.X));
	UStaticMeshComponent* VerdConsole = nullptr;
	AActor* VerdOutpost = PlaceOutpost(OutpostSite, SiteYaw, false, VerdConsole);
	const FLinearColor StationMetal(0.32f, 0.36f, 0.4f);
	if (!VerdOutpost)
	{
	Platform(FVector(WreckSite.X, WreckSite.Y, 0.f), 30.f, FVector(1600.f, 1600.f, 30.f), StationMetal, 0.f, TEXT("SM_Verd_Station_Deck"));
	for (const FVector2D& C : { FVector2D(-700.f, -700.f), FVector2D(700.f, -700.f), FVector2D(-700.f, 700.f), FVector2D(700.f, 700.f) })
	{
		Block(EShape::Cube, WreckSite + FVector(C.X, C.Y, 260.f), FVector(60.f, 60.f, 460.f), StationMetal, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Verd_Station_Post"));
	}
	Block(EShape::Cube, WreckSite + FVector(0.f, 0.f, 520.f), FVector(1700.f, 1700.f, 40.f), StationMetal, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Verd_Station_Roof"));
	Block(EShape::Cube, WreckSite + FVector(0.f, -760.f, 90.f), FVector(900.f, 60.f, 120.f), StationMetal, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Verd_Station_Rack"));
	Block(EShape::Cube, WreckSite + FVector(-760.f, 200.f, 90.f), FVector(60.f, 700.f, 120.f), StationMetal, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Verd_Station_Rack"));
	Block(EShape::Cube, WreckSite + FVector(0.f, 0.f, 505.f), FVector(1200.f, 60.f, 8.f), Cyan, 3.f, FRotator::ZeroRotator, false); // the station's work light
	}
	// (The dormant drone moved to the relay deck with the transfer: Cinematic 03 fires there.)

	ATDStepTrigger* Beacon = SpawnProp<ATDStepTrigger>(FVector(9300.f, 6300.f, 0.f));
	Beacon->LinkedObjective = Wreck;
	Beacon->LinkedStep = 0;
	TDCourseImpl::FinishProp(Beacon);

	// Explicit spawn points clear of the hull (a ring point landed inside it: an unreachable, unkillable grunt).
	ATDEncounterVolume* SiteFight = Encounter(Wreck, 1, WreckSite + FVector(0.f, 0.f, 200.f), FVector(2600.f, 2600.f, 700.f),
		{ ETDCrewType::Grunt, ETDCrewType::Trooper }, false, 1000.f, // the first shooter; the Heavy waits for the relay's transfer (T7's play, Nov 14)
		{ FVector(1300.f, 200.f, 0.f), FVector(-200.f, -1300.f, 0.f), FVector(1100.f, 1100.f, 0.f) });
	StepEncounters.Add({ Wreck, 1, SiteFight });
	// Director's mercy: from take 2 the trooper sits it out, from take 3 the grunt at 60 % health.
	FTDRetakeMercyTier SiteMercy2; SiteMercy2.FromTake = 2; SiteMercy2.DropCrew = 1;
	FTDRetakeMercyTier SiteMercy3; SiteMercy3.FromTake = 3; SiteMercy3.DropCrew = 1; SiteMercy3.HealthScale = 0.6f;
	SiteFight->MercyTiers = { SiteMercy2, SiteMercy3 };

	// The survey console: the signal-analysis device is recovered here (a tap; Gravity Well). Decision 2 (Ryan, Nov 13): the
	// transfer defence moved to the relay, so the station is the pick-up and nothing is carried.
	// The console's origin is its box centre: 60 x 120 x 130 cm standing on the 30 cm deck (T2's live check: at deck height
	// it sat half sunk and only took the interact trace looking 40 degrees down).
	ATDInteractableActor* Terminal = VerdOutpost ? OutpostTerminal<ATDInteractableActor>(OutpostSite, SiteYaw)
		: SpawnProp<ATDInteractableActor>(WreckSite + FVector(-250.f, 0.f, 30.f + 65.f), FRotator(0.f, 180.f, 0.f));
	Terminal->Prompt = LOCTEXT("SurveyTerminal", "Recover the device");
	Terminal->HoldTime = 0.f;
	Terminal->LinkedObjective = Wreck;
	Terminal->LinkedStep = 2;
	Terminal->ConsoleMesh = TDPropArt::Default(TEXT("SM_Verd_Station_Terminal"));
	TDCourseImpl::FinishProp(Terminal);
	if (VerdOutpost)
	{
		Terminal->SetActorScale3D(OutpostConsoleScale); // the kit console's box (the cube is 60 x 60 x 120)
		Terminal->Mesh->SetHiddenInGame(true); // Grok Bot's console is what the player sees; this is its interact box
		TerminalScreens.Add(Terminal, VerdConsole);
		Terminal->OnInteracted.AddUniqueDynamic(this, &ATDPlanetCourse::HandleTerminalScreen);
	}
	else
	{
		Terminal->SetActorScale3D(FVector(1.f, 2.f, 130.f / 120.f)); // the cube is 60 x 60 x 120 at scale 1
		UTDStatics::TintPlaceholder(Terminal->Mesh, FLinearColor(0.1f, 0.8f, 1.f), 1.5f);
	}

	Checkpoint(Wreck, 1, FVector(8300.f, 5800.f, 0.f), 30.f); // dying in the station fight: back at its edge, not at the pad
	FinishObjective(Wreck,
		{ LOCTEXT("W0", "FOLLOW THE BEACON TO THE RESEARCH SITE"), LOCTEXT("W1", "CLEAR THE RESEARCH SITE"),
		  LOCTEXT("W2", "RECOVER THE SIGNAL-ANALYSIS DEVICE · [E]") },
		{ Beacon, SiteFight, Terminal });

	// ---- Obj 2: the relay (-> Cloak on Rift). South-east, at ground level; after the device (decision 2, Ryan Nov 13). ----
	// The device is connected to the relay and the archive transfers while the squad holds the deck: the transfer defence
	// (75 s in two halves, a midpoint checkpoint, three waves, the Heavy at 50%, paused while a crewman is at the console).
	// The old climb (platforms, resonance nodes, altar) is gone. The arena brief for the structures builder: a flat 28 x 28 m
	// deck 30-60 cm high with ramps on three sides, 4-6 waist-high cover pieces, the console 4 m in from the west edge with
	// 3 m clear in front, the relay itself at the far (east) side and not walkable, 14 m of open ground around the deck, and
	// no perch over 2.5 m within 10 m of the console. Until BP_Relay lands, this is the placeholder by those numbers.
	const FVector RelayHead(2600.f, -2600.f, 0.f);
	const FVector RelaySite = VerdanceRelaySite;
	ATDObjective* Relay = MakeObjective(TEXT("Obj_Relay"), LOCTEXT("RelayTitle", "Wake the Relay"), ETDAbilityId::Cloak, ETDShipId::Rift, 5);
	AddTrailhead(Relay, RelayHead, Magenta, LOCTEXT("TheRelay", "THE RELAY"));
	Trailheads.Last().After = Wreck; // the fixed order: the relay waits for the device
	Route({ FVector(900.f, -900.f, 0.f), RelayHead, FVector(5200.f, -5000.f, 0.f), RelaySite + FVector(-1900.f, 0.f, 0.f) }, Magenta);

	// The structures builder's BP_Relay built to the brief (eng/struct 8ce5c3fb, Relay_Site.manifest.json): a 2800 x 2800
	// deck at 50, ramps W (2) / N / S, five 110 cm covers, the relay on a 2.5 m plinth (X 200..1400, Y -800..800) with a
	// 16 m mast, the console at (-1000, 0, 50) with the pilot at (-890, 0, 50) facing 180. The earlier 1600 deck
	// (22f47909) doesn't fit the defence: only a BP at least 26 m across is used, else the placeholder below.
	AActor* RelayArt = nullptr;
	if (UClass* RelayClass = LoadClass<AActor>(nullptr, TEXT("/Game/TriadDrift/Gameplay/Props/Relay/BP_Relay.BP_Relay_C"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		FActorSpawnParameters RelayParams;
		RelayParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		RelayArt = GetWorld()->SpawnActor<AActor>(RelayClass, FTransform(GetActorRotation(), W(RelaySite)), RelayParams);
		FVector O = FVector::ZeroVector, E = FVector::ZeroVector;
		if (RelayArt) { RelayArt->GetActorBounds(true, O, E); }
		if (RelayArt)
		{
			// intake (Nov 14): the relay plaza in stone tile: the BP's largest flat mesh (its deck), at runtime only
			UStaticMeshComponent* Best = nullptr;
			float BestArea = 0.f;
			TInlineComponentArray<UStaticMeshComponent*> Parts(RelayArt);
			for (UStaticMeshComponent* C : Parts)
			{
				const FBoxSphereBounds B = C->Bounds;
				const float Area = B.BoxExtent.X * B.BoxExtent.Y;
				if (B.BoxExtent.Z < 120.f && Area > BestArea) { Best = C; BestArea = Area; }
			}
			UMaterialInterface* Tile = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TriadDrift/Gameplay/Props/Fab/Textures/StoneTileFloor/MI_Fab_StoneTileFloor.MI_Fab_StoneTileFloor"), nullptr, LOAD_Quiet | LOAD_NoWarn);
			if (Best && Tile && BestArea > 1000.f * 1000.f) { Best->SetMaterial(0, Tile); UE_LOG(LogTriadDrift, Display, TEXT("Verdance: relay plaza %s in stone tile"), *Best->GetName()); }
		}
		if (RelayArt && FMath::Min(E.X, E.Y) < 1300.f)
		{
			UE_LOG(LogTriadDrift, Log, TEXT("Verdance: BP_Relay is %.0f x %.0f cm, not the 28 m defence deck yet; the placeholder stands in."), E.X * 2.f, E.Y * 2.f);
			RelayArt->Destroy();
			RelayArt = nullptr;
		}
		if (RelayArt) { Spawned.Add(RelayArt); RelayWake = RelayArt; }
	}
	const float DeckTop = RelayArt ? 50.f : 40.f;
	if (!RelayArt)
	{
	if (UStaticMeshComponent* Deck = Platform(RelaySite, DeckTop, FVector(2800.f, 2800.f, DeckTop), RelayStone, 0.2f, TEXT("SM_Verd_RelayDeck")))
	{
		// intake (Nov 14): the relay plaza in stone tile (placeholder deck)
		if (UMaterialInterface* Tile = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TriadDrift/Gameplay/Props/Fab/Textures/StoneTileFloor/MI_Fab_StoneTileFloor.MI_Fab_StoneTileFloor"), nullptr, LOAD_Quiet | LOAD_NoWarn)) { Deck->SetMaterial(0, Tile); }
	}
	for (const FVector& R : { FVector(-1550.f, 0.f, 0.f), FVector(0.f, 1550.f, 0.f), FVector(0.f, -1550.f, 0.f) }) // ramps on the west, north and south sides
	{
		const bool bX = FMath::Abs(R.X) > 1.f;
		Block(EShape::Cube, RelaySite + R + FVector(0.f, 0.f, DeckTop * 0.25f), bX ? FVector(300.f, 800.f, DeckTop * 0.5f) : FVector(800.f, 300.f, DeckTop * 0.5f),
			RelayStone, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Verd_RelayRamp"));
	}
	// Waist-high cover (100 cm), clear of the console's 3 m.
	for (const FVector& C : { FVector(-300.f, 700.f, 0.f), FVector(-300.f, -700.f, 0.f), FVector(500.f, 0.f, 0.f), FVector(400.f, 900.f, 0.f), FVector(400.f, -900.f, 0.f) })
	{
		Block(EShape::Cube, RelaySite + C + FVector(0.f, 0.f, DeckTop + 50.f), FVector(300.f, 80.f, 100.f), RelayStone, 0.f, FRotator(0.f, C.Y == 0.f ? 90.f : 0.f, 0.f), true, TEXT("SM_Verd_RelayCover"));
	}
	// The relay: a tall stone spire at the east side with a glowing core (collides; not a perch).
	Block(EShape::Cylinder, RelaySite + FVector(1150.f, 0.f, DeckTop + 900.f), FVector(220.f, 220.f, 1800.f), RelayStone, 0.6f, FRotator::ZeroRotator, true, TEXT("SM_Verd_RelaySpire"));
	Block(EShape::Cylinder, RelaySite + FVector(1150.f, 0.f, DeckTop + 1400.f), FVector(90.f, 90.f, 1800.f), RelayStone, 2.f, FRotator::ZeroRotator, false);
	}

	// The dormant drone (Cinematic 03 wakes it, T3): on the deck north of the plinth, in view from the console.
	DormantDrone = Block(EShape::Sphere, RelaySite + FVector(700.f, 1100.f, DeckTop + 90.f), FVector(120.f, 120.f, 90.f), FLinearColor(0.32f, 0.36f, 0.4f), 0.f,
		FRotator(0.f, -135.f, 0.f), false, TEXT("SM_Verd_Station_DormantDrone"));
	if (DormantDrone)
	{
		// Its eye: dark until it wakes (TickDroneWake), facing the console.
		const FVector Front = FRotator(0.f, -135.f, 0.f).Vector();
		DroneOptic = Block(EShape::Sphere, RelaySite + FVector(700.f, 1100.f, DeckTop + 100.f) + Front * 62.f, FVector(26.f), FLinearColor(0.02f, 0.03f, 0.05f), 0.05f,
			FRotator::ZeroRotator, false);
		if (DroneOptic) { DroneOptic->AttachToComponent(DormantDrone, FAttachmentTransformRules::KeepWorldTransform); }
		DroneHome = DormantDrone->GetComponentLocation();
		DroneWakeStart = -1.0;
		if (UTDStorySubsystem* Story = UTDStorySubsystem::Get(this))
		{
			Story->OnTransmission.AddUniqueDynamic(this, &ATDPlanetCourse::HandleStoryLine);
		}
	}

	ClearSite(RelaySite, 2400.f); // the deck's corners plus a margin; the 29 m keep-out below keeps the open ground
	ClearSite(WreckSite, 1500.f); // the research site's hull and walkway
	ATDStepTrigger* Reach = SpawnProp<ATDStepTrigger>(RelaySite);
	Reach->LinkedObjective = Relay;
	Reach->LinkedStep = 0;
	TDCourseImpl::FinishProp(Reach);
	Reach->Box->SetBoxExtent(FVector(1800.f, 1800.f, 400.f));

	// The relay console: 4 m in from the west edge, facing the pad; the tap connects the device (Cinematic 03, T3's hook,
	// fires on the transfer's start).
	const FVector ConsoleAt = RelaySite + FVector(-1000.f, 0.f, 0.f);
	ATDInteractableActor* RelayConsole = RelayArt
		? SpawnProp<ATDInteractableActor>(ConsoleAt + OutpostConsoleCentre + FVector(360.f, 0.f, DeckTop), FRotator(0.f, 180.f, 0.f)) // the kit console's box, as in the outposts
		: SpawnProp<ATDInteractableActor>(ConsoleAt + FVector(0.f, 0.f, DeckTop + 65.f), FRotator(0.f, 180.f, 0.f));
	RelayConsole->Prompt = LOCTEXT("RelayConsole", "Connect the device");
	RelayConsole->HoldTime = 0.f;
	RelayConsole->LinkedObjective = Relay;
	RelayConsole->LinkedStep = 1;
	RelayConsole->ConsoleMesh = TDPropArt::Default(TEXT("SM_Verd_RelayConsole"));
	TDCourseImpl::FinishProp(RelayConsole);
	if (RelayArt)
	{
		RelayConsole->SetActorScale3D(OutpostConsoleScale);
		RelayConsole->Mesh->SetHiddenInGame(true); // the kit console is what the player sees; this is its interact box
	}
	else
	{
		RelayConsole->SetActorScale3D(FVector(1.f, 2.f, 130.f / 120.f));
		UTDStatics::TintPlaceholder(RelayConsole->Mesh, Magenta, 1.5f);
	}

	// Three waves dropping in at the deck's north, south and east edges (never the west, the pilot's side), the third with
	// the Heavy (the disruption). Spawn points are relative to the volume, which sits 9 m up: they fall in.
	auto Wave = [&](const TArray<ETDCrewType>& Mix, const TArray<FVector>& Points)
	{
		return Encounter(Relay, INDEX_NONE, RelaySite + FVector(0.f, 0.f, 900.f), FVector(10.f), Mix, false, 1700.f, Points);
	};
	// Into the deck's edge band (13-14 m from its centre, the manifest's drop ring), off the plinth (X 200..1400, Y +-800).
	// The first wave lands ~18 m from the console (at -300 it was 15 m: grunts paused the transfer within 3 s of the start).
	ATDEncounterVolume* Wave1 = Wave({ ETDCrewType::Grunt, ETDCrewType::Grunt }, { FVector(300.f, 1300.f, 0.f), FVector(300.f, -1300.f, 0.f) });
	ATDEncounterVolume* Wave2 = Wave({ ETDCrewType::Trooper, ETDCrewType::Drone }, { FVector(900.f, 1200.f, 0.f), FVector(900.f, -1200.f, 0.f) });
	ATDEncounterVolume* Wave3 = Wave({ ETDCrewType::Heavy, ETDCrewType::Grunt }, { FVector(500.f, 1250.f, 0.f), FVector(-800.f, -1250.f, 0.f) });
	FTDRetakeMercyTier TransferMercy3; TransferMercy3.FromTake = 3; TransferMercy3.DropCrew = 1;
	Wave3->MercyTiers = { TransferMercy3 };
	// The pause zone is centred on the console.
	ATDArchiveTransfer* Transfer = SpawnProp<ATDArchiveTransfer>(ConsoleAt + FVector(0.f, 0.f, DeckTop));
	Transfer->LinkedObjective = Relay;
	Transfer->LinkedStep = 3; // steps 2 (first half) and 3 (second half, the midpoint checkpoint)
	Transfer->Waves = { Wave1, Wave2, Wave3 };
	Transfer->WaveAt = { 0.f, 0.25f, 0.5f };
	TDCourseImpl::FinishProp(Transfer);
	ArchiveTransfer = Transfer;

	ATDStepTrigger* Ship = SpawnProp<ATDStepTrigger>(FVector(300.f, 0.f, 0.f));
	Ship->LinkedObjective = Relay;
	Ship->LinkedStep = 4;
	TDCourseImpl::FinishProp(Ship);
	Ship->Box->SetBoxExtent(FVector(1400.f, 1400.f, 400.f));

	Checkpoint(Relay, 2, RelaySite + FVector(-500.f, 300.f, DeckTop), 180.f); // the transfer's start: on the deck behind the console point
	Checkpoint(Relay, 3, RelaySite + FVector(-500.f, 300.f, DeckTop), 180.f); // the midpoint
	FinishObjective(Relay,
		{ LOCTEXT("R0", "FOLLOW THE SIGNAL TO THE RELAY"), LOCTEXT("R1", "CONNECT THE DEVICE TO THE RELAY · [E]"),
		  LOCTEXT("R2", "PROTECT THE DATA TRANSFER"), LOCTEXT("R3", "PROTECT THE DATA TRANSFER"), LOCTEXT("R4", "GET BACK TO YOUR SHIP") },
		{ Reach, RelayConsole, Transfer, Transfer, Ship });

	// Story dressing (the environment builder's table, Nov 14; Kit/Story meshes, Kit/Sites sets, OutpostDressing props):
	// placed on the ground, each clearing its own footprint of flora.
	{
		const TCHAR* Story = TEXT("Kit/Story"); const TCHAR* Sites = TEXT("Kit/Sites"); const TCHAR* Out = TEXT("OutpostDressing");
		Dress({
			// A: the research site clearing, an abandoned field camp, and the relay mast pulling the eye from the pad.
			{ Sites, TEXT("BP_StorySet_VerdanceCamp"), FVector(9400.f, 9000.f, 0.f), 200.f, 1.f, true },
			{ Story, TEXT("SM_Kit_DragMark"), FVector(10800.f, 9500.f, 2.f), 60.f, 1.5f, false },
			{ Story, TEXT("SM_Kit_LampPostDead"), FVector(9700.f, 7050.f, 0.f), 0.f, 1.f, true },
			{ Story, TEXT("SM_Kit_LampPostDead"), FVector(10600.f, 6250.f, 0.f), 180.f, 1.f, true },
			{ Sites, TEXT("BP_Landmark_RelayMast"), FVector(13200.f, 6800.f, 0.f), 215.f, 1.f, true },
			// B: the wreck scar's head, and the kit wreck (the walkway through the hull) at the research site.
			{ Story, TEXT("SM_Kit_AntennaToppled"), FVector(13300.f, 8600.f, 0.f), 35.f, 1.f, true },
			{ Story, TEXT("SM_Kit_SampleCases"), FVector(12900.f, 9200.f, 0.f), 80.f, 1.f, true },
			// The kit wreck at the scar's end, just outside the site fight's box: on its 2 m walkway a pilot would be out of the
			// melee crew's reach mid-fight (Course.CrewAttackAPilotStandingAtTheWreckCore).
			{ TEXT("Wreck"), TEXT("BP_Wreck"), WreckSite + FVector(2900.f, 1700.f, 0.f), 36.f, 1.f, true },
			// ART-18's clue, nothing points at it: a scorched Triad panel in the grass past the wreck's far ramp toe.
			{ Story, TEXT("SM_Kit_InsigniaPanel"), WreckSite + FVector(2900.f, 1700.f, 0.f) + FRotator(0.f, 36.f, 0.f).RotateVector(FVector(-420.f, 1950.f, 0.f)), 196.f, 1.f, false },
			// C: the relay's approach.
			{ Story, TEXT("SM_Kit_SurveyStakeFlag"), FVector(4800.f, -6000.f, 0.f), 120.f, 1.f, false },
			{ Story, TEXT("SM_Kit_SurveyStakeFlag"), FVector(5300.f, -6800.f, 0.f), 120.f, 1.f, false },
			{ Story, TEXT("SM_Kit_SurveyStakeFlag"), FVector(5800.f, -7500.f, 0.f), 120.f, 1.f, false },
			{ Story, TEXT("SM_Kit_ShelterFrame"), FVector(9900.f, -8600.f, 0.f), 300.f, 1.f, true },
			{ Story, TEXT("SM_Kit_SampleCases"), FVector(9700.f, -4700.f, 0.f), 20.f, 1.f, true },
			// The survey line beside the pad-to-research-site route.
			{ Sites, TEXT("BP_SurveyLine_Verdance"), FVector(1500.f, 1800.f, 0.f), 0.f, 1.f, true },
		}, FVector::ZeroVector, 0.f, TEXT("Verdance"));
		// The relay deck's service corner (relay frame: +X is the plinth side).
		Dress({
			{ Out, TEXT("SM_Outpost_Generator"), FVector(3200.f, 600.f, 0.f), 180.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_PowerJunction"), FVector(3150.f, 250.f, 0.f), 180.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_DataKiosk"), FVector(3300.f, -400.f, 0.f), 180.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_Floodlight"), FVector(3000.f, 1300.f, 0.f), 200.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_Floodlight"), FVector(3000.f, -1300.f, 0.f), 160.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_SupplyCrate"), FVector(3350.f, -900.f, 0.f), 40.f, 1.f, true },
		}, RelaySite, 0.f, TEXT("Relay"));
		DressOutpost(OutpostSite, SiteYaw, TEXT("VerdanceOutpost"));
		// Route dressing (environment builder, Nov 14): the relay path's barest 30 m (bare mossy ground between the route posts
		// past RelayHead) gets an outcrop, a mushroom stand and ferns on both sides, 6-13 m off the line (local X along the
		// path, Y across it); nothing with collision within 6 m of the line.
		{
			const TCHAR* V = TEXT("Verdance");
			Dress({
				{ V, TEXT("SM_Verd_Rock_01"), FVector(-1100.f, 900.f, -40.f), 20.f, 2.6f, true },
				{ V, TEXT("SM_Verd_Rock_03"), FVector(-700.f, 1150.f, -30.f), 75.f, 1.8f, true },
				{ V, TEXT("SM_Verd_Rock_02"), FVector(900.f, -1000.f, -40.f), 140.f, 3.2f, true },
				{ V, TEXT("SM_Verd_Mushroom_02"), FVector(200.f, -1200.f, 0.f), 30.f, 5.0f, true },
				{ V, TEXT("SM_Verd_Mushroom_03"), FVector(500.f, -1300.f, 0.f), 200.f, 3.6f, true },
				{ V, TEXT("SM_Verd_Mushroom_01"), FVector(-200.f, -1050.f, 0.f), 110.f, 2.8f, true },
				{ V, TEXT("SM_Verd_GlowPlant_01"), FVector(-900.f, 700.f, 0.f), 0.f, 2.4f, false },
				{ V, TEXT("SM_Verd_GlowPlant_02"), FVector(1200.f, -700.f, 0.f), 60.f, 2.2f, false },
				{ V, TEXT("SM_Verd_Fern_01"), FVector(-400.f, 750.f, 0.f), 10.f, 1.6f, false },
				{ V, TEXT("SM_Verd_Fern_02"), FVector(0.f, 820.f, 0.f), 150.f, 1.4f, false },
				{ V, TEXT("SM_Verd_Fern_01"), FVector(700.f, -720.f, 0.f), 260.f, 1.5f, false },
				{ V, TEXT("SM_Verd_Fern_02"), FVector(1400.f, 800.f, 0.f), 90.f, 1.6f, false },
			}, FVector(5060.f, -5380.f, 0.f), -47.f, TEXT("RelayRouteDressing"));
		}
		DressPad(TEXT("VerdancePad"));
		// #194: the Verdance camp set (structures builder, batch BV): a research camp by the research site (doors on a shared
		// yard, walkways between them, tank, mast and fences on the outside), a smaller camp by the relay path and lone huts
		// along both routes; each placed where it clears every keep-out (DressCamp). Missing meshes are skipped by Dress().
		{
			const TCHAR* Hut = TEXT("Huts/Verdance");
			DressCamp(OutpostSite, 1500.f, 9600.f, {
				{ Hut, TEXT("SM_Hut_Verdance_Lab"), FVector(-950.f, 0.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Prefab"), FVector(900.f, 0.f, 0.f), 180.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Prefab"), FVector(0.f, 950.f, 0.f), 270.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Dome"), FVector(0.f, -800.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_WaterTank"), FVector(-1000.f, 1000.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Mast"), FVector(950.f, -950.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Walkway"), FVector(-450.f, 0.f, 0.f), 0.f, 1.f, false },
				{ Hut, TEXT("SM_Hut_Verdance_Walkway"), FVector(420.f, 0.f, 0.f), 0.f, 1.f, false },
				{ Hut, TEXT("SM_Hut_Verdance_Walkway"), FVector(0.f, 440.f, 0.f), 90.f, 1.f, false },
				{ Hut, TEXT("SM_Hut_Verdance_Fence"), FVector(-1550.f, -300.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Fence"), FVector(-1550.f, -700.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Fence"), FVector(400.f, -1500.f, 0.f), 0.f, 1.f, true },
				{ TEXT("Fab/CrateStack"), TEXT("SM_Fab_CrateStack"), FVector(-600.f, -1000.f, 0.f), 12.f, 1.f, true },
				{ TEXT("Fab/OilBarrel"), TEXT("SM_Fab_OilBarrel"), FVector(-300.f, -1100.f, 0.f), 0.f, 1.f, true },
				{ TEXT("Fab/MechanicBase/Mesh/SM/PROP/MACHINE"), TEXT("SM_ELEVATOR_CONSOLE_01"), FVector(500.f, 700.f, 87.f), 225.f, 1.f, true },
				// perimeter (structures builder's set): low walls on two sides, the gate facing the yard's open side, lamps
				{ Hut, TEXT("SM_Hut_Verdance_WallLow"), FVector(-1800.f, -1000.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_WallLow"), FVector(-1800.f, 1000.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_WallLow"), FVector(-200.f, 1800.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Gate"), FVector(1800.f, 0.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_LampPost"), FVector(1700.f, -500.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_LampPost"), FVector(1700.f, 500.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_LampPost"), FVector(-500.f, -1200.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Supplies"), FVector(600.f, -1150.f, 0.f), 20.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Generator"), FVector(-1250.f, -1100.f, 0.f), 90.f, 1.f, true },
			}, TEXT("VerdanceResearchCamp"));
			DressCamp(VerdanceRelaySite + FVector(-3000.f, 1200.f, 0.f), 1200.f, 4200.f, {
				{ Hut, TEXT("SM_Hut_Verdance_Prefab"), FVector(-700.f, 0.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Dome"), FVector(650.f, 0.f, 0.f), 180.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Prefab"), FVector(0.f, 800.f, 0.f), 270.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Mast"), FVector(0.f, -750.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Verdance_Walkway"), FVector(-150.f, 0.f, 0.f), 0.f, 1.f, false },
				{ TEXT("Fab/PowerCore"), TEXT("SM_Fab_PowerCore"), FVector(350.f, 450.f, 0.f), 30.f, 1.f, true },
			}, TEXT("VerdanceRelayCamp"));
			// lone huts: one beside each route, about halfway
			DressCamp(FVector(5200.f, 3700.f, 0.f), 1000.f, 3400.f, { { Hut, TEXT("SM_Hut_Verdance_Prefab"), FVector::ZeroVector, 210.f, 1.f, true },
				{ TEXT("Fab/OilBarrel"), TEXT("SM_Fab_OilBarrel"), FVector(0.f, 450.f, 0.f), 0.f, 1.f, true } }, TEXT("VerdanceSiteRouteHut"));
			DressCamp(FVector(3300.f, -3300.f, 0.f), 1000.f, 3400.f, { { Hut, TEXT("SM_Hut_Verdance_Dome"), FVector::ZeroVector, 135.f, 1.f, true } }, TEXT("VerdanceRelayRouteHut"));
		}
		DressRouteBreaks(false);
		DressAmbient(false);
	}

	// Dressing: engineering's props and trees (the placeholder flora only if they are missing).
	KeepOuts = {
		{ WreckSite, FVector(2800.f, 2800.f, 0.f) },
		{ RelaySite, FVector(2900.f, 2900.f, 0.f) },  // the relay deck and its 14 m of open ground
		{ Ambush, FVector(1700.f, 1700.f, 0.f) },
	};
	MarkRoutes(TEXT("SM_Verd_Mushroom_03"), 1600.f, 700.f, FVector2D(0.8f, 1.1f)); // tall glowing trail markers
	const FVector Mid(5000.f, 0.f, 0.f);
	const bool bProps = ScatterMeshes({
		// A canopy ring around the play space; a few giants as landmarks.
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
	GroundBox = FBox2D(FVector2D(-3100.f, -10600.f), FVector2D(20100.f, 9600.f));
	if (UStaticMeshComponent* Ground = Block(EShape::Cube, FVector(GroundBox.GetCenter(), -100.f), FVector(GroundBox.GetSize(), 200.f), CinderGround))
	{
		GroundSlab = Ground;
		if (UMaterialInterface* GroundArt = GroundMaterialCinder.IsNull() ? nullptr : (FPackageName::DoesPackageExist(GroundMaterialCinder.ToSoftObjectPath().GetLongPackageName()) ? GroundMaterialCinder.LoadSynchronous() : nullptr))
		{
			Ground->SetMaterial(0, GroundArt); // world-aligned tiling ground (Lead's MI_Ground_*)
		}
	}

	auto Magma = [this](const FVector& Center, const FVector& Size)
	{
		ATDMagmaVolume* M = SpawnProp<ATDMagmaVolume>(Center);
		if (M)
		{
			TDCourseImpl::FinishProp(M);
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
			TDCourseImpl::FinishProp(G);
		}
		return G;
	};

	// ---- Obj 0: the survey records (storyline.md sections 22 and 25: Cinder in the reduced form). A short marked route
	// from the pad past one patrol to an open-sided survey station at the dig's rim (the Verdance station kit), whose
	// terminal works once the patrol is dead or left behind; it is Cinematic 05's hook (T3). Then the excavation, then
	// the anchor, in that order. ----
	FVector StationAt(3700.f, 0.f, 0.f);
	ATDObjective* Survey = MakeObjective(TEXT("Obj_Survey"), LOCTEXT("SurveyTitle", "Access the Survey Records"), ETDAbilityId::None, ETDShipId::Rift, 2);
	AddTrailhead(Survey, FVector(1300.f, 0.f, 0.f), Crystal, LOCTEXT("TheSurvey", "THE SURVEY STATION"));
	Route({ FVector(700.f, 0.f, 0.f), FVector(2300.f, 0.f, 0.f), StationAt - FVector(900.f, 0.f, 0.f) }, Crystal);
	UStaticMeshComponent* CindConsole = nullptr;
	const AActor* CindOutpost = PlaceOutpost(StationAt, 180.f, true, CindConsole); // bay and ramp to the pad (-X)
	if (!CindOutpost)
	{
		const FLinearColor StationMetal(0.3f, 0.3f, 0.32f);
		Platform(FVector(StationAt.X, StationAt.Y, 0.f), 30.f, FVector(1600.f, 1600.f, 30.f), StationMetal, 0.f, TEXT("SM_Verd_Station_Deck"));
		for (const FVector2D& C : { FVector2D(-700.f, -700.f), FVector2D(700.f, -700.f), FVector2D(-700.f, 700.f), FVector2D(700.f, 700.f) })
		{
			Block(EShape::Cube, StationAt + FVector(C.X, C.Y, 260.f), FVector(60.f, 60.f, 460.f), StationMetal, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Verd_Station_Post"));
		}
		Block(EShape::Cube, StationAt + FVector(0.f, 0.f, 520.f), FVector(1700.f, 1700.f, 40.f), StationMetal, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Verd_Station_Roof"));
		Block(EShape::Cube, StationAt + FVector(0.f, 760.f, 90.f), FVector(900.f, 60.f, 120.f), StationMetal, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Verd_Station_Rack"));
		Block(EShape::Cube, StationAt + FVector(0.f, 0.f, 505.f), FVector(1200.f, 60.f, 8.f), Ember, 3.f, FRotator::ZeroRotator, false); // the work light
	}
	// The patrol on the way (overlap: it starts when the pilot walks in; leaving it behind is allowed).
	Encounter(Survey, INDEX_NONE, FVector(2000.f, 0.f, 200.f), FVector(700.f, 1100.f, 600.f), { ETDCrewType::Grunt, ETDCrewType::Trooper }, true, 650.f);
	ATDStepTrigger* AtStation = SpawnProp<ATDStepTrigger>(StationAt - FVector(900.f, 0.f, 0.f));
	AtStation->LinkedObjective = Survey;
	AtStation->LinkedStep = 0;
	TDCourseImpl::FinishProp(AtStation);
	ATDSurveyTerminal* Records = CindOutpost ? OutpostTerminal<ATDSurveyTerminal>(StationAt, 180.f)
		: SpawnProp<ATDSurveyTerminal>(StationAt + FVector(0.f, 0.f, 30.f + 65.f), FRotator(0.f, 180.f, 0.f));
	Records->Prompt = LOCTEXT("SurveyRecords", "Access the survey records");
	Records->HoldTime = 0.f;
	Records->LinkedObjective = Survey;
	Records->LinkedStep = 1;
	Records->ConsoleMesh = TDPropArt::Default(TEXT("SM_Verd_Station_Terminal"));
	TDCourseImpl::FinishProp(Records);
	if (CindOutpost)
	{
		Records->SetActorScale3D(OutpostConsoleScale);
		Records->Mesh->SetHiddenInGame(true);
		TerminalScreens.Add(Records, CindConsole);
		Records->OnInteracted.AddUniqueDynamic(this, &ATDPlanetCourse::HandleTerminalScreen);
	}
	else
	{
		Records->SetActorScale3D(FVector(1.f, 2.f, 130.f / 120.f));
		UTDStatics::TintPlaceholder(Records->Mesh, FLinearColor(1.f, 0.55f, 0.15f), 1.5f);
	}
	FinishObjective(Survey, { LOCTEXT("S0", "INVESTIGATE THE MINING COMPLEX"), LOCTEXT("S1", "ACCESS THE SURVEY RECORDS · [E]") }, { AtStation, Records });
	SurveyTerminal = Records;

	// ---- Obj 3: the outpost (-> EMP Pulse on Lancer). North-east, over a magma river. ----
	const FVector OutHead(2600.f, 2600.f, 0.f);
	ATDObjective* Outpost = MakeObjective(TEXT("Obj_Outpost"), LOCTEXT("OutpostTitle", "Secure the Excavation"), ETDAbilityId::EMPPulse, ETDShipId::Lancer, 6);
	AddTrailhead(Outpost, OutHead, Ember, LOCTEXT("TheOutpost", "THE EXCAVATION"));
	Trailheads.Last().After = Survey; // the fixed order: the records first
	Route({ FVector(900.f, 900.f, 0.f), OutHead, FVector(5000.f, 4200.f, 0.f), FVector(7600.f, 6000.f, 0.f), FVector(9200.f, 6000.f, 0.f) }, Ember);

	// The magma river with basalt stepping stones (3 m gaps: a jump).
	const ATDMagmaVolume* River = Magma(FVector(5600.f, 4800.f, 0.f), FVector(1600.f, 7000.f, 300.f));
	TArray<FVector> RiverChain = { FVector(4450.f, 4250.f, 0.f) }; // the near bank, each stone, the far bank
	for (int32 i = 0; i < 3; ++i)
	{
		Platform(FVector(5050.f + i * 550.f, 4400.f + i * 300.f, 0.f), 60.f, FVector(280.f, 280.f, 160.f), Basalt, 0.f, TEXT("SM_Cind_SteppingStone"));
		RiverChain.Add(FVector(5050.f + i * 550.f, 4400.f + i * 300.f, 220.f));
	}
	RiverChain.Add(FVector(6750.f, 5100.f, 0.f));
	// The structures builder's walk-tested magma bridge (eng/struct e76ce3bb): pivot on the bank at the span centre, +X
	// along the span, ramps to the ground at x +-1400, deck top 50 cm (the burn trigger ends 10 cm over the surface).
	// On the route (5000, 4200) -> (7600, 6000); both ramp feet on dry ground (x 4449 and 6751), the channel under the
	// deck. It is the marker's way across when it is in the build; the stones otherwise.
	{
		const float BridgeYaw = 34.7f;
		const FVector BridgeAt(5600.f, 4620.f, 0.f);
		const FVector Along = FRotator(0.f, BridgeYaw, 0.f).Vector();
		const FVector FootA = BridgeAt - Along * 1400.f, FootB = BridgeAt + Along * 1400.f;
		const float BankZ = FMath::Max(GroundZAt(FootA), GroundZAt(FootB));
		if (SpawnKitBP(TEXT("Kit/Sites"), TEXT("BP_MagmaBridge"), FVector(BridgeAt.X, BridgeAt.Y, BankZ), BridgeYaw, 1.f))
		{
			SiteClears.Add({ BridgeAt, 1500.f });
			RiverChain = { FootA - Along * 150.f, FVector(BridgeAt.X, BridgeAt.Y, BankZ + 50.f), FootB + Along * 100.f };
		}
	}
	// A 60 cm basalt lip along both banks (T7, Nov 14: two deaths stepping into the river's east edge at y 6000), open
	// where the bridge and the stepping stones meet the banks (east y 4850..5500, west y 3750..4600). Jumpable on purpose.
	for (const FVector4f& Seg : { FVector4f(6430.f, 1300.f, 4850.f, 0.f), FVector4f(6430.f, 5500.f, 8300.f, 0.f),
		FVector4f(4770.f, 1300.f, 3750.f, 0.f), FVector4f(4770.f, 4600.f, 8300.f, 0.f) })
	{
		Block(EShape::Cube, FVector(Seg.X, (Seg.Y + Seg.Z) * 0.5f, 30.f), FVector(50.f, Seg.Z - Seg.Y, 60.f), Basalt, 0.f, FRotator::ZeroRotator, true);
	}
	// The marker crosses from wherever the pilot is (T7: the straight line from the respawn ran over the river); two
	// magma deaths in a row put the pilot on the far bank (clear of the geyser row), facing the gate.
	AddMagmaCrossing(River, Outpost, INDEX_NONE, RiverChain, FVector(6900.f, 4900.f, 0.f), 25.f);
	// A geyser row on the far bank, rippling, at least 8 m clear of the bridge's ramp feet (the structures builder's
	// walk, Nov 14: a geyser 3.1 m past the far foot threw the pilot 5-6 m; Course.CinderMarkers... checks it).
	for (int32 i = 0; i < 3; ++i)
	{
		Geyser(FVector(7750.f + i * 450.f, 5600.f + i * 200.f, 0.f), i * 1.2f);
	}

	// The compound along +X: gate wall, yard, barracks, command deck, Warden arena.
	const float Y0 = 6000.f;
	auto Wall = [this, Y0](float X, float GapHalf)
	{
		// A wall across the compound with a 6 m doorway in the middle (GapHalf = 0: solid).
		const float Half = 1800.f;
		if (GapHalf <= 0.f)
		{
			Block(EShape::Cube, FVector(X, Y0, 300.f), FVector(150.f, Half * 2.f, 600.f), TDCourseColors::Steel, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Cind_OutpostWall"));
			return;
		}
		const float Len = Half - GapHalf;
		Block(EShape::Cube, FVector(X, Y0 + GapHalf + Len * 0.5f, 300.f), FVector(150.f, Len, 600.f), TDCourseColors::Steel, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Cind_OutpostWall"));
		Block(EShape::Cube, FVector(X, Y0 - GapHalf - Len * 0.5f, 300.f), FVector(150.f, Len, 600.f), TDCourseColors::Steel, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Cind_OutpostWall"));
		UStaticMeshComponent* Lintel = Block(EShape::Cube, FVector(X, Y0, 580.f), FVector(160.f, GapHalf * 2.f, 40.f), TDCourseColors::ArmadaRed, 1.5f, FRotator::ZeroRotator, false);
		if (UStaticMesh* Frame = ArtFor(TEXT("SM_Cind_OutpostWall_Doorway")))
		{
			// The doorway frame fills the whole gap (150 x 600 x 600, lintel built in): carried by a no-collision box there.
			UStaticMeshComponent* Gap = Block(EShape::Cube, FVector(X, Y0, 300.f), FVector(150.f, GapHalf * 2.f, 600.f), TDCourseColors::Steel, 0.f, FRotator::ZeroRotator, false);
			if (TDPropArt::Dress(Gap, Frame) && Lintel)
			{
				Lintel->SetHiddenInGame(true);
			}
		}
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
		Block(EShape::Cube, FVector((XGate + XEnd) * 0.5f, Y0 + S * 1800.f, 300.f), FVector(XEnd - XGate, 150.f, 600.f), Steel, 0.f, FRotator::ZeroRotator, true, TEXT("SM_Cind_OutpostWall"));
	}
	auto Door = [this, Y0](float X)
	{
		ATDDoor* D = SpawnProp<ATDDoor>(FVector(X, Y0, 0.f));
		if (D)
		{
			D->OpenHeight = 650.f;
			D->Mesh->SetRelativeScale3D(FVector(0.3f, 6.2f, 6.f)); // before BeginPlay: it records the closed spot
			D->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
			TDCourseImpl::FinishProp(D);
			UTDStatics::TintPlaceholder(D->Mesh, TDCourseColors::ArmadaRed, 0.4f);
		}
		return D;
	};
	ATDDoor* Gate = Door(XGate);
	ATDDoor* DoorB = Door(XBarracks);
	ATDDoor* DoorC = Door(XCommand);
	if (DoorC) { DoorC->bRecognisesPilot = true; } // the dig's inner door knows the pilot (storyline.md §6: a clue, never explained)
	ATDDoor* DoorD = Door(XArena);

	// Gate conduits: two glowing red pylons beside the gate, slash them down.
	ATDStepCounter* Conduits = SpawnProp<ATDStepCounter>(FVector(XGate - 200.f, Y0, 0.f));
	Conduits->LinkedObjective = Outpost;
	Conduits->LinkedStep = 0;
	Conduits->Required = 2;
	Conduits->BaseText = LOCTEXT("Conduits", "BREACH THE GATE · DESTROY THE CONDUITS");
	TDCourseImpl::FinishProp(Conduits);
	for (const float S : { -1.f, 1.f })
	{
		ATDShatterCrystal* C = SpawnProp<ATDShatterCrystal>(FVector(XGate - 300.f, Y0 + S * 650.f, 0.f));
		C->LinkedObjective = Outpost;
		C->LinkedStep = 0;
		C->Color = FLinearColor(1.f, 0.1f, 0.05f);
		C->MaxHealth = 60.f;
		// The structures builder's conduit (100 x 100 x 300 cm, pivot at the footing on the ground; it fits the placeholder
		// cylinder at scale 1) and its broken stump in its place once it falls (eng/struct 4a5f5060). The old path
		// (Props/Planet/SM_Cind_Conduit) never existed.
		C->CrystalMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Story/SM_Kit_NullConduit.SM_Kit_NullConduit")));
		C->BrokenMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Story/SM_Kit_NullConduitBroken.SM_Kit_NullConduitBroken")));
		C->Mesh->SetStaticMesh(CylinderMesh); // before FinishSpawning: the art is fitted to this shape at BeginPlay
		C->Mesh->SetRelativeScale3D(FVector(1.f, 1.f, 3.f));
		C->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
		TDCourseImpl::FinishProp(C);
		Conduits->WatchDestructible(C);
		if (MarkerGroups.Num() == 0 || MarkerGroups.Last().Objective.Get() != Outpost) { FMarkerGroup& G = MarkerGroups.AddDefaulted_GetRef(); G.Objective = Outpost; G.Step = 0; G.Counter = Conduits; }
		MarkerGroups.Last().Targets.Add(C);
	}
	// The gate's guards start 12 m+ past the bridge's far ramp foot (6751, 5417): the structures builder's walk (Nov 14)
	// had crew 2-3 m from the bridge end killing the pilot stepping off. Trigger box x 8200..9600, crew on a 5 m ring.
	ATDEncounterVolume* GateGuards = Encounter(Outpost, INDEX_NONE, FVector(XGate - 700.f, Y0, 200.f), FVector(700.f, 1800.f, 600.f),
		{ ETDCrewType::Grunt, ETDCrewType::Trooper, ETDCrewType::Grunt }, true, 500.f);
	// They belong to the gate step for retakes: past the gate a CUT does not bring them back (T5's replay, Nov 14: the
	// yard's retake spot is inside their trigger box and they re-armed on every yard retake).
	if (GateGuards) { GateGuards->RetakeStep = 0; }
	StepDoors.Add({ Outpost, 1, Gate });

	// Cover crates inside each room.
	auto Crates = [this, Y0](float X0, float X1, int32 Seed)
	{
		FRandomStream R(Seed);
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector At(FMath::Lerp(X0 + 400.f, X1 - 400.f, R.FRand()), Y0 + R.FRandRange(-1300.f, 1300.f), 0.f);
			Block(EShape::Cube, At + FVector(0.f, 0.f, 70.f), FVector(R.FRandRange(220.f, 360.f), R.FRandRange(150.f, 300.f), 140.f), TDCourseColors::Steel, 0.f, FRotator::ZeroRotator, true, R.FRand() < 0.5f ? FName(TEXT("SM_Cind_Crate_A")) : FName(TEXT("SM_Cind_Crate_B")));
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
	// The Heavy listed last: mercy's "one fewer" (take 3) takes it first (T5's replay: 8 CUTs to its bash while it stayed).
	ATDEncounterVolume* Barracks = RoomFight(XBarracks, XCommand, 2, { ETDCrewType::Grunt, ETDCrewType::Grunt, ETDCrewType::Heavy }, DoorC);
	ATDEncounterVolume* Command = RoomFight(XCommand, XArena, 3, { ETDCrewType::Trooper, ETDCrewType::Drone, ETDCrewType::Trooper }, DoorD);
	ATDEncounterVolume* Arena = RoomFight(XArena, XEnd, 4, { ETDCrewType::Warden }, nullptr);
	// Director's mercy (machine playtests lost the Warden up to 9 takes running with him nearly dead): from take 3
	// he starts at 75% and summons once, from take 5 at 50% and never summons.
	FTDRetakeMercyTier Mercy3; Mercy3.FromTake = 3; Mercy3.HealthScale = 0.75f; Mercy3.MaxSummons = 1;
	FTDRetakeMercyTier Mercy5; Mercy5.FromTake = 5; Mercy5.HealthScale = 0.5f; Mercy5.MaxSummons = 0;
	Arena->MercyTiers = { Mercy3, Mercy5 };

	// Screen toward the way the pilot comes in (the kit console's screen faces its +X).
	// Grok Bot's crystal cache (PR #75: a 200 x 360 x 180 cm cradle, the hero crystal on its Crystal socket) when
	// imported, at true size (the cube is its box, base on the ground); else the kit's prototype console.
	UStaticMesh* CacheArt = ArtFor(TEXT("SM_CrystalCacheCradle"));
	const FVector CacheSize = CacheArt ? FVector(200.f, 360.f, 180.f) : FVector(160.f, 160.f, 140.f);
	ATDInteractableActor* Prototype = SpawnProp<ATDInteractableActor>(FVector(XEnd - 700.f, Y0 + 700.f, CacheSize.Z * 0.5f), FRotator(0.f, 180.f, 0.f)); // off the arena line (the Warden fight crosses it)
	Prototype->ConsoleMesh = CacheArt ? TSoftObjectPtr<UStaticMesh>(CacheArt) : TDPropArt::Default(TEXT("SM_Cind_PrototypeConsole")); // fitted to the cube at the scale below
	Prototype->Prompt = LOCTEXT("ClaimEmp", "Recover the crystals and the EMP prototype");
	Prototype->HoldTime = 1.5f;
	Prototype->LinkedObjective = Outpost;
	Prototype->LinkedStep = 5;
	TDCourseImpl::FinishProp(Prototype);
	Prototype->SetActorScale3D(CacheSize / 100.f);
	UTDStatics::TintPlaceholder(Prototype->Mesh, FLinearColor(0.3f, 0.8f, 1.f), 2.5f);
	UStaticMesh* CrystalArt = CacheArt ? ArtFor(TEXT("SM_CrystalCacheCrystal")) : nullptr;
	TArray<USceneComponent*> CacheParts;
	if (CrystalArt) { Prototype->Mesh->GetChildrenComponents(false, CacheParts); }
	for (USceneComponent* Child : CacheParts)
	{
		UStaticMeshComponent* Cradle = Cast<UStaticMeshComponent>(Child);
		if (!Cradle || Cradle->GetStaticMesh() != CacheArt) { continue; }
		FVector CrystalAt(0.f, 0.f, 30.05f);	// the cradle floor
		TDPlanetCourse_AnchorSocket(CacheArt, TEXT("Crystal"), CrystalAt);
		UStaticMeshComponent* CrystalPart = NewObject<UStaticMeshComponent>(Prototype);
		CrystalPart->SetMobility(EComponentMobility::Movable);
		CrystalPart->SetStaticMesh(CrystalArt);
		CrystalPart->SetupAttachment(Cradle);
		CrystalPart->SetRelativeLocation(CrystalAt);
		CrystalPart->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName); // the cube is the hold-E target
		CrystalPart->SetCanEverAffectNavigation(false);
		CrystalPart->RegisterComponent();
		Prototype->AddInstanceComponent(CrystalPart);
		break;
	}

	// Every room is a checkpoint: a death costs that room, not the whole compound (machine playtest: the
	// barracks death sent the pilot back to the gate conduits every time). The Warden too (review N6).
	// The gate: a CUT there comes back on the far bank, past the river, not at the pad (T5's play, Nov 14: every retake at
	// the conduits re-walked the whole way over the bridge).
	Checkpoint(Outpost, 0, FVector(7300.f, 4900.f, 0.f), 25.f);
	Checkpoint(Outpost, 1, FVector(XGate - 500.f, Y0, 0.f), 0.f);
	Checkpoint(Outpost, 2, FVector(XBarracks - 500.f, Y0, 0.f), 0.f);
	Checkpoint(Outpost, 3, FVector(XCommand - 500.f, Y0, 0.f), 0.f);
	Checkpoint(Outpost, 4, FVector(XArena - 600.f, Y0, 0.f), 0.f);
	FinishObjective(Outpost,
		{ LOCTEXT("O0", "BREACH THE GATE · DESTROY THE CONDUITS  0 / 2"), LOCTEXT("O1", "CLEAR THE YARD"), LOCTEXT("O2", "CLEAR THE BARRACKS"),
		  LOCTEXT("O3", "CLEAR THE DIG"), LOCTEXT("O4", "DEFEAT THE WARDEN"), LOCTEXT("O5", "RECOVER THE STABILISING CRYSTALS · HOLD [E]") },
		{ Conduits, Yard, Barracks, Command, Arena, Prototype });
	(void)GateGuards;

	// ---- Obj 4: the jammer (-> reveals the Dominion). South-east, across a magma lake. ----
	const FVector JamHead(2600.f, -2600.f, 0.f);
	ATDObjective* Jammer = MakeObjective(TEXT("Obj_Jammer"), LOCTEXT("JammerTitle", "Disable the Field Anchor"), ETDAbilityId::None, ETDShipId::Rift, 3);
	AddTrailhead(Jammer, JamHead, Crystal, LOCTEXT("TheJammer", "THE FIELD ANCHOR"));
	Trailheads.Last().After = Outpost; // then the anchor (the crystals before the anchor: the story needs them)
	Route({ FVector(900.f, -900.f, 0.f), JamHead, FVector(5200.f, -4600.f, 0.f), FVector(6800.f, -5000.f, 0.f) }, Crystal);

	// A crystal wall across the trail: slash through it.
	for (int32 i = 0; i < 3; ++i)
	{
		ATDShatterCrystal* C = SpawnProp<ATDShatterCrystal>(FVector(4700.f + i * 120.f, -4100.f - i * 300.f, 0.f));
		C->LinkedObjective = Jammer;
		C->LinkedStep = 0;
		C->Color = Crystal;
		TDCourseImpl::FinishProp(C);
	}
	// Rock walls either side of the crystal choke so it can't be walked round.
	Block(EShape::Cube, FVector(4600.f, -3200.f, 300.f), FVector(400.f, 1200.f, 600.f), Basalt, 0.f, FRotator(0.f, -30.f, 0.f), true, TEXT("SM_Cind_ChokeRock_A"));
	Block(EShape::Cube, FVector(5100.f, -5400.f, 300.f), FVector(400.f, 1400.f, 600.f), Basalt, 0.f, FRotator(0.f, -30.f, 0.f), true, TEXT("SM_Cind_ChokeRock_B"));

	// #140: from the approach the bridge marker pointed straight through the rock wall. The wall band is a hazard to
	// route round, its safe chain the crystal choke (registered before the lake's crossing: the first match wins).
	if (UTDObjectiveSubsystem* Routes140 = UTDObjectiveSubsystem::Get(this))
	{
		FBox ChokeWall(ForceInit);
		for (const FVector& Corner : { FVector(4350.f, -7200.f, -500.f), FVector(5350.f, -7200.f, -500.f), FVector(4350.f, -1800.f, 1500.f), FVector(5350.f, -1800.f, 1500.f) })
		{
			ChokeWall += W(Corner);
		}
		TArray<AActor*> Choke;
		for (const FVector& L : { FVector(4150.f, -4250.f, 100.f), FVector(4820.f, -4400.f, 100.f), FVector(5550.f, -4550.f, 100.f) }) { Choke.Add(Marker(L)); }
		Routes140->AddCrossing(ChokeWall, Jammer, 0, Choke);
	}

	// The magma lake and the collapsing crystal bridge: one continuous walkway of slabs (no jumps), each
	// falling 1.1 s after you step on it: keep running.
	const float LakeX0 = 7000.f, LakeX1 = 10400.f, LakeY = -5000.f;
	const ATDMagmaVolume* Lake = Magma(FVector((LakeX0 + LakeX1) * 0.5f, LakeY, 0.f), FVector(LakeX1 - LakeX0, 5000.f, 300.f));
	// Out over the bridge (the near and far bank), back by the south shore once it has fallen (the return route).
	AddMagmaCrossing(Lake, Jammer, 0, { FVector(LakeX0 - 350.f, LakeY, 0.f), FVector(LakeX1 + 350.f, LakeY, 0.f) }, FVector(LakeX1 + 250.f, LakeY, 0.f), 0.f);
	AddMagmaCrossing(Lake, Jammer, 2, { FVector(LakeX1 + 800.f, LakeY - 3300.f, 0.f) }, FVector(LakeX1 + 250.f, LakeY, 0.f), 0.f);
	const int32 Segments = 6;
	for (int32 i = 0; i < Segments; ++i)
	{
		const float X = LakeX0 + 250.f + i * ((LakeX1 - LakeX0 - 500.f) / (Segments - 1));
		ATDCollapsingPlatform* P = SpawnProp<ATDCollapsingPlatform>(FVector(X, LakeY + (i % 2 ? 60.f : -60.f), 80.f));
		P->LinkedObjective = Jammer;
		TDCourseImpl::FinishProp(P);
		P->SetSize(FVector((LakeX1 - LakeX0 - 500.f) / (Segments - 1) + 30.f, 320.f, 60.f)); // overlapping: no gaps
	}
	ATDStepTrigger* FarBank = SpawnProp<ATDStepTrigger>(FVector(LakeX1 + 600.f, LakeY, 0.f));
	FarBank->LinkedObjective = Jammer;
	FarBank->LinkedStep = 0;
	TDCourseImpl::FinishProp(FarBank);

	// The jammer tower and its three charge sites, defended.
	const FVector Tower(12600.f, LakeY, 0.f);
	AActor* TowerActor = SpawnProp<AActor>(Tower);
	UStaticMeshComponent* AnchorBody = nullptr; // Grok Bot's tower, when it wears it: its collision places the charge points
	if (TowerActor)
	{
		USceneComponent* Root = NewObject<USceneComponent>(TowerActor, TEXT("Root"));
		TowerActor->SetRootComponent(Root);
		TDCourseImpl::FinishProp(TowerActor);
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
			return C;
		};
		UStaticMeshComponent* Shaft = Part(EShape::Cylinder, FVector(0.f, 0.f, 1500.f), FVector(500.f, 500.f, 3000.f), TDCourseColors::Steel, 0.f);
		UStaticMeshComponent* Tip = Part(EShape::Cone, FVector(0.f, 0.f, 3400.f), FVector(400.f, 400.f, 800.f), TDCourseColors::ArmadaRed, 1.5f);
		UStaticMeshComponent* BarX = Part(EShape::Cube, FVector(0.f, 0.f, 2600.f), FVector(1600.f, 80.f, 80.f), TDCourseColors::ArmadaRed, 2.5f);
		UStaticMeshComponent* BarY = Part(EShape::Cube, FVector(0.f, 0.f, 2200.f), FVector(80.f, 1400.f, 80.f), TDCourseColors::ArmadaRed, 2.5f);
		// Grok Bot's anchor tower (PR #73, 35.8 m, plinth r 7.5 m) when imported, else the planet kit's jammer tower.
		UStaticMesh* AnchorArt = ArtFor(TEXT("SM_CinderAnchorTower"));
		if (UStaticMeshComponent* Dressed = TDPropArt::Dress(Shaft, AnchorArt ? AnchorArt : ArtFor(TEXT("SM_Cind_JammerTower")), false)) // one rigid mesh on the shaft's base; it sinks with the tower actor
		{
			for (UStaticMeshComponent* Other : { Tip, BarX, BarY }) { if (Other) { Other->SetHiddenInGame(true); } }
			if (AnchorArt)
			{
				AnchorBody = Dressed;
				// Its own UCX boxes (plinth, shaft, collar) are the collision now: the 5 m placeholder shaft stood inside
				// the 15 m plinth, so the pilot walked into the visible base.
				Dressed->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
				Shaft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				// The emitter crown seats in the top well.
				FVector EmitterAt(0.f, 0.f, 3402.8f);
				TDPlanetCourse_AnchorSocket(AnchorArt, TEXT("TopEmitter"), EmitterAt);
				if (UStaticMesh* EmitterArt = ArtFor(TEXT("SM_CinderAnchorEmitter")))
				{
					UStaticMeshComponent* Emitter = NewObject<UStaticMeshComponent>(TowerActor);
					Emitter->SetMobility(EComponentMobility::Movable);
					Emitter->SetStaticMesh(EmitterArt);
					Emitter->SetupAttachment(Dressed);
					Emitter->SetRelativeLocation(EmitterAt);
					Emitter->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
					Emitter->SetCanEverAffectNavigation(false);
					Emitter->RegisterComponent();
					TowerActor->AddInstanceComponent(Emitter);
				}
			}
		}
	}
	UStaticMesh* const AnchorTowerArt = ArtFor(TEXT("SM_CinderAnchorTower"));
	ATDStepCounter* Charges = SpawnProp<ATDStepCounter>(Tower);
	Charges->LinkedObjective = Jammer;
	Charges->LinkedStep = 1;
	Charges->Required = 3;
	Charges->BaseText = LOCTEXT("Charges", "PLANT THE CHARGES");
	TDCourseImpl::FinishProp(Charges);
	for (int32 i = 0; i < 3; ++i)
	{
		const float A = PI + (i - 1) * 0.9f;
		// On the tower's charge ports (r 587-601 cm, 2.6-2.75 m up the plinth faces) when the anchor art is in; the
		// old 520 cm ring at 60 cm sat inside its plinth.
		FVector SiteAt(FMath::Cos(A) * 520.f, FMath::Sin(A) * 520.f, 60.f);
		if (AnchorTowerArt) { TDPlanetCourse_AnchorSocket(AnchorTowerArt, *FString::Printf(TEXT("Charge_%d"), i), SiteAt); }
		float FaceGap = 45.f; // how far the charge point stands out from the tower's face
		// The sockets (r 587-601) sit inside the tower's plinth collision (UCX_00, a 13.8 m square box 5.7 m tall: Played.Cinder
		// could never reach one, and the interact trace hits the box first). Each charge point goes just outside whatever
		// collision the tower has (its lowest hull), along its port's heading, at a standing pilot's reach; it follows a better
		// hull on its own.
		if (AnchorBody && AnchorTowerArt && AnchorTowerArt->GetBodySetup())
		{
			// The lowest collision hull is the plinth: leave it along the port's heading (mesh space = course space here:
			// the tower stands unrotated and unscaled at Tower).
			FBox Plinth(ForceInit);
			const FKConvexElem* PlinthHull = nullptr;
			for (const FKConvexElem& Hull : AnchorTowerArt->GetBodySetup()->AggGeom.ConvexElems)
			{
				if (Hull.ElemBox.IsValid && Hull.ElemBox.Min.Z < 100.f && (!Plinth.IsValid || Hull.ElemBox.GetVolume() > Plinth.GetVolume())) { Plinth = Hull.ElemBox; PlinthHull = &Hull; }
			}
			for (const FKBoxElem& Box : AnchorTowerArt->GetBodySetup()->AggGeom.BoxElems)
			{
				const FBox B = FBox::BuildAABB(Box.Center, FVector(Box.X, Box.Y, Box.Z) * 0.5f);
				if (B.Min.Z < 100.f && (!Plinth.IsValid || B.GetVolume() > Plinth.GetVolume())) { Plinth = B; PlinthHull = nullptr; }
			}
			// The refitted octagon (import a78930a4) leaves each socket 10 cm outside: then the socket is the charge point.
			FVector Closest, Normal;
			const bool bSocketOutside = PlinthHull && PlinthHull->GetClosestPointAndNormal(SiteAt, FTransform::Identity, Closest, Normal) > 1.f;
			if (bSocketOutside) { Plinth = FBox(ForceInit); FaceGap = 10.f; }
			const FVector D = FVector(SiteAt.X, SiteAt.Y, 0.f).GetSafeNormal();
			if (Plinth.IsValid && !D.IsNearlyZero())
			{
				const float Tx = FMath::Abs(D.X) > KINDA_SMALL_NUMBER ? (D.X > 0.f ? Plinth.Max.X : Plinth.Min.X) / D.X : BIG_NUMBER;
				const float Ty = FMath::Abs(D.Y) > KINDA_SMALL_NUMBER ? (D.Y > 0.f ? Plinth.Max.Y : Plinth.Min.Y) / D.Y : BIG_NUMBER;
				SiteAt = D * (FMath::Min(Tx, Ty) + 45.f);
				SiteAt.Z = 110.f;
			}
		}
		ATDInteractableActor* Site = SpawnProp<ATDInteractableActor>(Tower + SiteAt);
		Site->Prompt = LOCTEXT("Plant", "Plant a charge");
		Site->HoldTime = 1.5f;
		Site->LinkedObjective = Jammer;
		Site->LinkedStep = 1;
		Site->bCompleteLinkedStep = false;
		TDCourseImpl::FinishProp(Site);
		UTDStatics::TintPlaceholder(Site->Mesh, FLinearColor(1.f, 0.85f, 0.1f), 2.f);
		// The structures builder's charge (Kit/Story/SM_Kit_AnchorCharge, 30 x 60 x 54 cm) at scale 1: its back plate on the
		// tower face at the charge point, +X away from the tower. The 4.2 m box stays as the interact volume only (the
		// builder's check, Nov 14: fitted to the box it stood as a 4 m post 2.1 m up).
		if (UStaticMesh* ChargeArt = TDPlanetCourse_KitStory(TEXT("SM_Kit_AnchorCharge")))
		{
			const FVector Away = FVector(SiteAt.X, SiteAt.Y, 0.f).GetSafeNormal();
			UStaticMeshComponent* Charge = NewObject<UStaticMeshComponent>(Site, TEXT("ChargeArt"));
			Charge->SetStaticMesh(ChargeArt);
			Charge->SetMobility(EComponentMobility::Movable);
			Charge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Charge->SetupAttachment(Site->GetRootComponent());
			Charge->SetUsingAbsoluteScale(true);
			Charge->SetUsingAbsoluteRotation(true);
			Charge->RegisterComponent();
			Site->AddInstanceComponent(Charge);
			Charge->SetWorldScale3D(FVector(1.f));
			Charge->SetWorldRotation(GetActorRotation() + FRotator(0.f, Away.Rotation().Yaw, 0.f));
			Charge->SetWorldLocation(W(Tower + SiteAt - Away * FaceGap)); // on the face
			// The box is the interact volume only. Its BeginPlay dressed it with the kit console and a top-cap column (the
			// "box on a thin stalk" the structures builder saw): hide every mesh on the site but the charge.
			auto HideAllButCharge = [](ATDInteractableActor* On, UStaticMeshComponent* Keep)
			{
				TInlineComponentArray<UStaticMeshComponent*> SiteMeshes(On);
				for (UStaticMeshComponent* M : SiteMeshes)
				{
					if (M && M != Keep) { M->SetHiddenInGame(true); }
				}
			};
			HideAllButCharge(Site, Charge);
			// Again next tick, in case its BeginPlay (which dresses it) runs after this.
			GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Site, [Site, Charge, HideAllButCharge]() { HideAllButCharge(Site, Charge); }));
		}
		Charges->WatchInteractable(Site);
		UE_LOG(LogTriadDrift, Display, TEXT("Cinder anchor: charge point %d at %s (stand about 1.4 m out from the tower face and look up at it)"), i,
			*Site->GetActorLocation().ToCompactString());
		// Each unplanted charge point carries the marker, nearest first (T7, Nov 14: only the counter at the tower had one).
		if (MarkerGroups.Num() == 0 || MarkerGroups.Last().Objective.Get() != Jammer) { FMarkerGroup& G = MarkerGroups.AddDefaulted_GetRef(); G.Objective = Jammer; G.Step = 1; G.Counter = Charges; }
		MarkerGroups.Last().Targets.Add(Site);
	}
	ATDEncounterVolume* Defence = Encounter(Jammer, INDEX_NONE, Tower + FVector(0.f, 0.f, 200.f), FVector(2000.f, 2000.f, 600.f),
		{ ETDCrewType::Trooper, ETDCrewType::Grunt, ETDCrewType::Grunt }, false, 900.f,
		// On the charge side (-X), clear of the tower's 13.8 m plinth box: a 9 m ring put crew inside its corners and behind
		// the tower, where neither they nor the pilot could get round it (Played.Cinder, 7a958ecf's collision).
		{ FVector(-1500.f, 0.f, 0.f), FVector(-1200.f, 1150.f, 0.f), FVector(-1200.f, -1150.f, 0.f) }); // take 1 about 25 dps standing still like every other room (the drone made it about 29); (machine playtest: 5 on a 15 m ring spawned on top of the retake point)
	StepEncounters.Add({ Jammer, 1, Defence });
	FTDRetakeMercyTier DefenceMercy3; DefenceMercy3.FromTake = 3; DefenceMercy3.DropCrew = 1;
	FTDRetakeMercyTier DefenceMercy5; DefenceMercy5.FromTake = 5; DefenceMercy5.DropCrew = 2;
	Defence->MercyTiers = { DefenceMercy3, DefenceMercy5 };

	// The run: back along the lake's south shore (solid ground), debris falling, out of the tower's reach.
	const FVector Exit(LakeX0 - 600.f, LakeY - 3400.f, 0.f);
	Platform(FVector((LakeX0 + LakeX1 + 1600.f) * 0.5f, LakeY - 3300.f, 0.f), 30.f, FVector(LakeX1 - LakeX0 + 2400.f, 1200.f, 30.f), Basalt, 0.f, TEXT("SM_Cind_EscapeShore"));
	ATDStepTrigger* Clear = SpawnProp<ATDStepTrigger>(Exit);
	Clear->LinkedObjective = Jammer;
	Clear->LinkedStep = 2;
	TDCourseImpl::FinishProp(Clear);
	Clear->Box->SetBoxExtent(FVector(900.f, 900.f, 400.f));
	ATDCollapseRun* Run = SpawnProp<ATDCollapseRun>(FVector((LakeX0 + LakeX1) * 0.5f + 1500.f, LakeY - 2400.f, 0.f));
	Run->LinkedObjective = Jammer;
	Run->LinkedStep = 2;
	Run->Seconds = 25.f; // T7 (Nov 14): 20 s was tight from the north charge (the straight line clips the plinth)
	Run->Tower = TowerActor;
	Run->DebrisExtent = FVector(3500.f, 1200.f, 0.f);
	// The anchor's own collapse pieces (structures builder, Kit/Story), each optional.
	for (const TCHAR* L : { TEXT("A"), TEXT("B"), TEXT("C") })
	{
		if (TDPlanetCourse_KitStory(*FString::Printf(TEXT("SM_Kit_AnchorDebris_%s"), L), *FString::Printf(TEXT("AnchorDebris_%s"), L)))
		{
			Run->DebrisMeshes.Add(TSoftObjectPtr<UStaticMesh>(TDPlanetCourse_KitStory(*FString::Printf(TEXT("SM_Kit_AnchorDebris_%s"), L), *FString::Printf(TEXT("AnchorDebris_%s"), L))));
		}
		if (UStaticMesh* Section = TDPlanetCourse_KitStory(*FString::Printf(TEXT("SM_Kit_AnchorSection_%s"), L), *FString::Printf(TEXT("AnchorSection_%s"), L)))
		{
			Run->SectionMeshes.Add(TSoftObjectPtr<UStaticMesh>(Section));
		}
	}
	TDCourseImpl::FinishProp(Run);
	Routes.Add({ Tower, FVector(LakeX1 + 800.f, LakeY - 3300.f, 0.f), Exit });

	Checkpoint(Jammer, 1, FVector(LakeX1 + 250.f, LakeY, 0.f), 0.f); // the charges: from the far bank, out of the defence's drop
	// The collapse run: a CUT keeps the charges planted and the defence down, and the run restarts south of the tower,
	// on the way out (T7: a CUT reset the charges to 0 / 3).
	Checkpoint(Jammer, 2, FVector(LakeX1 + 1000.f, LakeY - 1600.f, 0.f), 200.f);
	if (Defence) { Defence->RetakeStep = 1; }
	FinishObjective(Jammer,
		{ LOCTEXT("J0", "CROSS THE CRYSTAL BRIDGE"), LOCTEXT("J1", "PLANT THE CHARGES  0 / 3"), LOCTEXT("J2", "RETURN TO EXTRACTION") },
		{ FarBank, Charges, Clear });

	// Story dressing (the environment builder's table, Nov 14).
	{
		const TCHAR* Story = TEXT("Kit/Story"); const TCHAR* Sites = TEXT("Kit/Sites");
		Dress({
			// A: the excavation (the mine set, the derrick beyond it on the walking line).
			{ Sites, TEXT("BP_StorySet_CinderMine"), FVector(13500.f, 2400.f, 0.f), 0.f, 1.f, true },
			{ Sites, TEXT("BP_Landmark_NullDerrick"), FVector(19300.f, 6000.f, 0.f), 270.f, 1.f, true },
			// ART-18's clue: a destroyed hull plate with Triad's chevrons, behind the mine set, off every route.
			{ Story, TEXT("SM_Kit_InsigniaHull"), FVector(15300.f, 1200.f, 0.f), 115.f, 1.f, true },
			{ Story, TEXT("SM_Kit_ScorchLines"), FVector(8600.f, 8400.f, 2.f), 30.f, 1.3f, false },
			{ Story, TEXT("SM_Kit_WarningBeacon"), FVector(4300.f, 2400.f, 0.f), 0.f, 1.f, true },
			{ Story, TEXT("SM_Kit_WarningBeacon"), FVector(4400.f, 5800.f, 0.f), 0.f, 1.f, true },
			// B: the anchor tower's approach.
			{ Story, TEXT("SM_Kit_WarningBeacon"), FVector(6600.f, -2800.f, 0.f), 0.f, 1.f, true },
			{ Story, TEXT("SM_Kit_WarningBeacon"), FVector(6600.f, -7200.f, 0.f), 0.f, 1.f, true },
			{ Story, TEXT("SM_Kit_ScorchLines"), FVector(13800.f, -3600.f, 2.f), 50.f, 1.4f, false },
			{ Story, TEXT("SM_Kit_ScorchLines"), FVector(11600.f, -6600.f, 2.f), 230.f, 1.2f, false },
			{ Story, TEXT("SM_Kit_CableReel"), FVector(14200.f, -6200.f, 0.f), 10.f, 1.f, true },
			// C: the survey station.
			{ Story, TEXT("SM_Kit_HeatShieldCracked"), FVector(3700.f, 1500.f, 0.f), 0.f, 1.f, true },
			{ Story, TEXT("SM_Kit_CableReel"), FVector(4900.f, -1300.f, 0.f), 60.f, 1.f, true },
			{ Story, TEXT("SM_Kit_SurveyStakeFlag"), FVector(5400.f, -300.f, 0.f), 90.f, 1.f, false },
			{ Story, TEXT("SM_Kit_SurveyStakeFlag"), FVector(6200.f, -200.f, 0.f), 90.f, 1.f, false },
			{ Story, TEXT("SM_Kit_SurveyStakeFlag"), FVector(7000.f, -100.f, 0.f), 90.f, 1.f, false },
		}, FVector::ZeroVector, 0.f, TEXT("Cinder"));
		// The dig-site cabin (ART-17): beside the mine set on cleared ground, its stairs to the pad side (-X), off the
		// excavation compound's keep-out; its own dressing in its frame.
		const FVector CabinAt(11800.f, 2200.f, 0.f);
		Dress({ { Sites, TEXT("BP_DigSite_Cabin"), FVector::ZeroVector, 0.f, 1.f, true } }, CabinAt, 180.f, TEXT("DigCabin"));
		const TCHAR* Out = TEXT("OutpostDressing");
		Dress({
			{ Out, TEXT("SM_Outpost_CargoCrate"), FVector(520.f, 320.f, 0.f), 12.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_SupplyCrate"), FVector(480.f, 410.f, 0.f), 30.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_Floodlight"), FVector(450.f, -440.f, 0.f), 150.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_Generator"), FVector(-520.f, -260.f, 0.f), 90.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_PowerJunction"), FVector(-470.f, -400.f, 0.f), 0.f, 1.f, true },
			{ Out, TEXT("SM_Outpost_CommAntenna"), FVector(-200.f, 470.f, 0.f), 0.f, 1.f, true },
		}, CabinAt, 180.f, TEXT("DigCabin"));
		// intake (Nov 14): two spare truss segments on the ground beside the dig cabin, concrete rubble round them
		Dress({
			{ TEXT("Fab/PipeBeam"), TEXT("SM_Fab_PipeBeam"), FVector(-700.f, 950.f, 0.f), 0.f, 1.f, true },
			{ TEXT("Fab/PipeBeam"), TEXT("SM_Fab_PipeBeam"), FVector(-860.f, 1000.f, 0.f), 4.f, 1.f, true },
			{ TEXT("Planet"), TEXT("SM_Cind_DebrisChunk"), FVector(-560.f, 600.f, -10.f), 60.f, 0.9f, false },
			{ TEXT("Planet"), TEXT("SM_Cind_DebrisChunk"), FVector(-980.f, 1450.f, -10.f), 200.f, 0.8f, false },
			{ TEXT("Planet"), TEXT("SM_Cind_DebrisChunk"), FVector(-620.f, 1500.f, -10.f), 320.f, 0.7f, false },
		}, CabinAt, 180.f, TEXT("DigCabinIntake"));
		RetintDressing(CabinAt, 1800.f, TEXT("SM_Cind_DebrisChunk"), TEXT("/Game/TriadDrift/Gameplay/Props/Fab/Textures/ConcreteRubble/MI_Fab_ConcreteRubble.MI_Fab_ConcreteRubble"));
		DressOutpost(StationAt, 180.f, TEXT("CinderOutpost"));
		// Route dressing (environment builder, Nov 14): the field anchor path's barest 30 m (an empty basalt flat before the
		// anchor) gets a crystal outcrop, choke rocks, a burnt tree, debris and an abandoned survey cache (Fab barrels and a
		// crate, sparingly), 6-14 m off the line; nothing with collision within 6 m of it.
		{
			const TCHAR* C = TEXT("Cinder");
			const TCHAR* Pl = TEXT("Planet");
			Dress({
				{ C, TEXT("SM_Cind_Crystal_04"), FVector(-900.f, 1100.f, -40.f), 25.f, 4.5f, true },
				{ C, TEXT("SM_Cind_Crystal_03"), FVector(-500.f, 1300.f, -30.f), 160.f, 3.5f, true },
				{ C, TEXT("SM_Cind_Basalt_03"), FVector(1000.f, 950.f, -30.f), 80.f, 2.4f, true },
				{ Pl, TEXT("SM_Cind_ChokeRock_A"), FVector(-1200.f, -1000.f, -50.f), 300.f, 1.0f, true },
				{ Pl, TEXT("SM_Cind_ChokeRock_B"), FVector(600.f, -1250.f, -50.f), 40.f, 1.1f, true },
				{ TEXT("Scatter"), TEXT("SM_Scatter_Cind_CharTree_01"), FVector(1300.f, -900.f, -30.f), 120.f, 1.4f, true },
				{ Pl, TEXT("SM_Cind_DebrisChunk"), FVector(-200.f, -700.f, -10.f), 70.f, 1.0f, false },
				{ Pl, TEXT("SM_Cind_DebrisChunk"), FVector(300.f, 680.f, -10.f), 210.f, 0.8f, false },
				{ TEXT("Fab/OilBarrel"), TEXT("SM_Fab_OilBarrel"), FVector(-100.f, -900.f, 0.f), 0.f, 1.f, true },
				{ TEXT("Fab/OilBarrel"), TEXT("SM_Fab_OilBarrel"), FVector(-30.f, -980.f, 0.f), 50.f, 1.f, true },
				{ TEXT("Fab/CrateStack"), TEXT("SM_Fab_CrateStack"), FVector(150.f, -1000.f, 0.f), 15.f, 1.f, true },
			}, FVector(5620.f, -4180.f, 0.f), -35.f, TEXT("AnchorRouteDressing"));
		}
		DressPad(TEXT("CinderPad"));
		// #194: the Cinder mining camp by the survey outpost (workshop, two shacks, pump house, ore hopper, drill rig, pipe
		// runs, flood masts, walls, gate and lamps), a smaller one by the jammer path, lone shacks by the routes, then the
		// ground-break arches and landmarks. Each where it clears every keep-out (DressCamp).
		{
			const TCHAR* Hut = TEXT("Huts/Cinder");
			DressCamp(StationAt, 2000.f, 9600.f, {
				{ Hut, TEXT("SM_Hut_Cinder_Workshop"), FVector(-950.f, 0.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_Shack"), FVector(900.f, 0.f, 0.f), 180.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_Shack"), FVector(0.f, 950.f, 0.f), 270.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_PumpHouse"), FVector(0.f, -950.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_OreHopper"), FVector(-1050.f, 1050.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_DrillRig"), FVector(1150.f, -1150.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_PipeRun"), FVector(-450.f, -1050.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_PipeRun"), FVector(450.f, -1050.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_FloodMast"), FVector(-1400.f, -1400.f, 0.f), 45.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_FloodMast"), FVector(1400.f, 1400.f, 0.f), 225.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_Supplies"), FVector(550.f, 650.f, 0.f), 15.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_Generator"), FVector(-550.f, 650.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_WallLow"), FVector(-1900.f, -800.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_WallLow"), FVector(-1900.f, 800.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_Gate"), FVector(1900.f, 0.f, 0.f), 90.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_LampPost"), FVector(1800.f, -500.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_LampPost"), FVector(1800.f, 500.f, 0.f), 0.f, 1.f, true },
				{ TEXT("Fab/OilBarrel"), TEXT("SM_Fab_OilBarrel"), FVector(-300.f, 1250.f, 0.f), 0.f, 1.f, true },
				{ TEXT("Fab/OilBarrel"), TEXT("SM_Fab_OilBarrel"), FVector(-220.f, 1330.f, 0.f), 40.f, 1.f, true },
			}, TEXT("CinderMiningCamp"));
			DressCamp(FVector(6000.f, -4300.f, 0.f), 1200.f, 4200.f, {
				{ Hut, TEXT("SM_Hut_Cinder_Shack"), FVector(-700.f, 0.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_Shack"), FVector(700.f, 0.f, 0.f), 180.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_Generator"), FVector(0.f, 750.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_FloodMast"), FVector(0.f, -800.f, 0.f), 0.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_Supplies"), FVector(-300.f, 700.f, 0.f), 0.f, 1.f, true },
			}, TEXT("CinderJammerCamp"));
			DressCamp(FVector(5000.f, 4200.f, 0.f), 1000.f, 3400.f, { { Hut, TEXT("SM_Hut_Cinder_Shack"), FVector::ZeroVector, 200.f, 1.f, true },
				{ Hut, TEXT("SM_Hut_Cinder_LampPost"), FVector(450.f, 0.f, 0.f), 0.f, 1.f, true } }, TEXT("CinderOutRouteShack"));
			DressCamp(FVector(3500.f, -2800.f, 0.f), 1000.f, 3400.f, { { Hut, TEXT("SM_Hut_Cinder_Shack"), FVector::ZeroVector, 145.f, 1.f, true } }, TEXT("CinderJamRouteShack"));
		}
		DressRouteBreaks(true);
		DressAmbient(true);
	}

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

#include "TriadDrift/Ground/Interior/TDDominionInteriorCourse.inl"

#undef LOCTEXT_NAMESPACE

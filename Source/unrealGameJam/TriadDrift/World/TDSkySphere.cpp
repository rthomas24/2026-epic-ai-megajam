// Triad Drift - World (Teammate 4). Engineering nebula sky (see TDSkySphere.h).

#include "TriadDrift/World/TDSkySphere.h"
#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"

const FName ATDSkySphere::EngineeringSkyTag(TEXT("TD.Engineering.Sky"));

namespace TDSkySphereDefaults
{
	const TCHAR* MeshPath = TEXT("/Game/TriadDrift/Gameplay/Props/Sky/SM_TD_SkySphere.SM_TD_SkySphere");
	const TCHAR* MaterialPath = TEXT("/Game/TriadDrift/Gameplay/Props/Sky/MI_TD_NebulaSky.MI_TD_NebulaSky");

	bool Exists(const FSoftObjectPath& P)
	{
		return !P.IsNull() && FPackageName::DoesPackageExist(P.GetLongPackageName());
	}
}

ATDSkySphere::ATDSkySphere()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;	// after the camera moved this frame: no lag at speed
	Tags.Add(EngineeringSkyTag);

	Sky = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sky"));
	RootComponent = Sky;
	Sky->SetMobility(EComponentMobility::Movable);
	Sky->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sky->SetGenerateOverlapEvents(false);
	Sky->SetCastShadow(false);
	Sky->bAffectDistanceFieldLighting = false;
	Sky->bVisibleInRayTracing = false;
	Sky->bReceivesDecals = false;
	Sky->bNeverDistanceCull = true;
	Sky->SetCanEverAffectNavigation(false);
	Sky->CanCharacterStepUpOn = ECB_No;

	SkyMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TDSkySphereDefaults::MeshPath));
	SkyMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TDSkySphereDefaults::MaterialPath));
}

bool ATDSkySphere::IsArtAvailable()
{
	return TDSkySphereDefaults::Exists(FSoftObjectPath(TDSkySphereDefaults::MeshPath))
		&& TDSkySphereDefaults::Exists(FSoftObjectPath(TDSkySphereDefaults::MaterialPath));
}

ATDSkySphere* ATDSkySphere::SpawnIfAvailable(UWorld* World)
{
	if (!World || !IsArtAvailable())
	{
		return nullptr;
	}
	for (TActorIterator<ATDSkySphere> It(World); It; ++It)
	{
		return *It;	// one per world
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ATDSkySphere>(ATDSkySphere::StaticClass(), FTransform::Identity, Params);
}

void ATDSkySphere::BeginPlay()
{
	Super::BeginPlay();
	UStaticMesh* Mesh = TDSkySphereDefaults::Exists(SkyMesh.ToSoftObjectPath()) ? SkyMesh.LoadSynchronous() : nullptr;
	UMaterialInterface* Mat = TDSkySphereDefaults::Exists(SkyMaterial.ToSoftObjectPath()) ? SkyMaterial.LoadSynchronous() : nullptr;
	if (!Mesh)
	{
		Destroy();
		return;
	}
	Sky->SetStaticMesh(Mesh);
	Sky->SetWorldRotation(FRotator::ZeroRotator);	// the baked sun sits at SunYawDeg / SunPitchDeg only unrotated
	Sky->SetWorldScale3D(FVector(RadiusCm / 100.f));	// the mesh is authored at 100 cm radius
	if (Mat)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, this);
		MID->SetScalarParameterValue(TEXT("Brightness"), Brightness);
		Sky->SetMaterial(0, MID);
	}
	RefreshYield();
}

void ATDSkySphere::RefreshYield()
{
	bool bPresentation = false;
	for (TActorIterator<AActor> It(GetWorld()); It && !bPresentation; ++It)
	{
		bPresentation = *It != this && !It->IsActorBeingDestroyed() && It->ActorHasTag(ATDPlaceholderSky::PresentationSkyTag);
	}
	if (bPresentation != bYielding)
	{
		bYielding = bPresentation;
		SetActorHiddenInGame(bYielding);
	}
}

void ATDSkySphere::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RescanTimer -= DeltaSeconds;
	if (RescanTimer <= 0.f)
	{
		RescanTimer = 1.f;
		RefreshYield();
	}
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (PC && PC->PlayerCameraManager)
	{
		SetActorLocation(PC->PlayerCameraManager->GetCameraLocation());
	}
	if (!bRecaptured && !bYielding)
	{
		// The placeholder sky light captured an empty void at spawn: recapture once the nebula is in place so the
		// ambient light picks up its colours.
		bRecaptured = true;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<USkyLightComponent*> Lights(*It);
			for (USkyLightComponent* L : Lights)
			{
				if (L) { L->RecaptureSky(); }
			}
		}
	}
}

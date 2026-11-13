// Triad Drift - placeholder lighting and exposure (N-004). Owner: Teammate 5.

#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TriadDrift/World/TDSkySphere.h"

const FName ATDPlaceholderSky::PresentationSkyTag(TEXT("TD.Presentation.Sky"));
const FName ATDPlaceholderSky::PlaceholderTag(TEXT("TD.Placeholder.Sky"));

ATDPlaceholderSky::ATDPlaceholderSky()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	Tags.Add(PlaceholderTag);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetForwardShadingPriority(1); // the sun owns forward shading

	Fill = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Fill"));
	Fill->SetupAttachment(RootComponent);
	Fill->SetMobility(EComponentMobility::Movable);
	Fill->SetForwardShadingPriority(0); // never competes with the sun
	Fill->SetCastShadows(false);
	Fill->SetAtmosphereSunLight(false);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	FarGround = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FarGround"));
	FarGround->SetupAttachment(RootComponent);
	FarGround->SetMobility(EComponentMobility::Movable);	// placed when the style is applied
	FarGround->SetCollisionProfileName(TEXT("BlockAll"));	// nothing falls into a void past the course slab
	FarGround->SetCastShadow(false);
	FarGround->bAffectDistanceFieldLighting = false;
	FarGround->SetVisibility(false);
	if (PlaneMesh.Succeeded()) { FarGround->SetStaticMesh(PlaneMesh.Object); }

	HorizonFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HorizonFog"));
	HorizonFog->SetupAttachment(RootComponent);
	HorizonFog->SetVisibility(false);

	Exposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Exposure"));
	Exposure->SetupAttachment(RootComponent);
	Exposure->bUnbound = true;
	Exposure->Priority = -100.f; // anything the map adds wins (Nick's skybox uses -10)
}

ATDPlaceholderSky::FWorldSky ATDPlaceholderSky::Scan(const UWorld* World)
{
	FWorldSky Out;
	if (!World)
	{
		return Out;
	}
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const AActor* A = *It;
		if (!A || A->IsActorBeingDestroyed() || A->ActorHasTag(PlaceholderTag))
		{
			continue;
		}
		if (A->ActorHasTag(PresentationSkyTag))
		{
			Out.bTaggedSky = true;
		}
		if (!Out.bDirectionalLight && A->FindComponentByClass<UDirectionalLightComponent>())
		{
			Out.bDirectionalLight = true;
		}
		if (!Out.bPostProcess)
		{
			if (const APostProcessVolume* Vol = Cast<APostProcessVolume>(A); Vol && Vol->bUnbound)
			{
				Out.bPostProcess = true;
			}
			TInlineComponentArray<UPostProcessComponent*> PPs(A);
			for (const UPostProcessComponent* PP : PPs)
			{
				if (PP && PP->bUnbound)
				{
					Out.bPostProcess = true;
				}
			}
		}
	}
	return Out;
}

ATDPlaceholderSky* ATDPlaceholderSky::SpawnIfNeeded(UWorld* World, ETDPlaceholderSkyStyle Style)
{
	if (!World)
	{
		return nullptr;
	}
	const FWorldSky Sky = Scan(World);
	if (Sky.ProvidesLighting() && Sky.ProvidesExposure())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPlaceholderSky* P = World->SpawnActor<ATDPlaceholderSky>(ATDPlaceholderSky::StaticClass(), FTransform::Identity, Params);
	if (P)
	{
		P->ApplyStyle(Style);
		P->Refresh();
	}
	return P;
}

void ATDPlaceholderSky::ApplyStyle(ETDPlaceholderSkyStyle Style)
{
	ApplyHorizon(Style);
	if (Style == ETDPlaceholderSkyStyle::Space)
	{
		// The sun shines from the one baked into the nebula sky (ATDSkySphere::SunYawDeg / SunPitchDeg); planets read
		// this light (atmosphere sun) for their night side.
		Sun->SetWorldRotation(ATDSkySphere::SunLightRotation());
		Sun->SetIntensity(8.f);
		Sun->SetLightColor(FLinearColor(1.f, 0.93f, 0.82f));
		Sun->SetAtmosphereSunLight(true);
		// Soft shadowless fill from below and behind the sun, nebula violet: the shade side reads as shape, not black.
		Fill->SetWorldRotation(FRotator(20.f, ATDSkySphere::SunYawDeg, 0.f));
		Fill->SetIntensity(1.2f);
		Fill->SetLightColor(FLinearColor(0.5f, 0.4f, 1.f));
		SkyLight->SetIntensity(1.f);
		SkyLight->bLowerHemisphereIsBlack = false;
		SkyLight->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
		Atmosphere->DestroyComponent();
		Atmosphere = nullptr;
		// Fixed exposure: auto exposure in a black void pumps badly when explosions appear.
		FPostProcessSettings& S = Exposure->Settings;
		S.bOverride_AutoExposureMethod = true;
		S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		S.AutoExposureApplyPhysicalCameraExposure = false;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = 0.f;
		// Bloom only from real emitters (engines, eyes, bolts, explosions) and capped, so a mass explosion never
		// whites out the screen (playtest: TD.KillAll in wave 5).
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 0.8f;
		S.bOverride_BloomThreshold = true;
		S.BloomThreshold = 1.5f;
		// A touch of motion blur sells speed without smearing the targets.
		S.bOverride_MotionBlurAmount = true;
		S.MotionBlurAmount = 0.2f;
		S.bOverride_MotionBlurMax = true;
		S.MotionBlurMax = 2.f;
	}
	else
	{
		Sun->SetWorldRotation(FRotator(-50.f, 30.f, 0.f));
		Sun->SetIntensity(10.f);
		Sun->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
		Sun->SetAtmosphereSunLight(true);
		// Soft shadowless fill from the opposite side: shadows read as shade, never pure black, even if the
		// sky light's real-time capture contributes little (it does in a packaged / -game run).
		Fill->SetWorldRotation(FRotator(-25.f, 210.f, 0.f));
		Fill->SetIntensity(2.f);
		Fill->SetLightColor(FLinearColor(0.55f, 0.7f, 1.f));
		Fill->SetCastShadows(false);
		Fill->SetAtmosphereSunLight(false);
		SkyLight->bRealTimeCapture = true;
		SkyLight->bLowerHemisphereIsBlack = false;
		SkyLight->SetIntensity(1.5f);
		// Daylight: auto exposure is fine; the post-process only exists so a map without one is consistent.
	}
	if (Style == ETDPlaceholderSkyStyle::Verdance && Atmosphere)
	{
		Sun->SetWorldRotation(FRotator(-9.f, 200.f, 0.f));
		Sun->SetIntensity(4.f);
		Sun->SetLightColor(FLinearColor(1.f, 0.78f, 0.62f));
		Atmosphere->SetRayleighScattering(FLinearColor(0.0035f, 0.02f, 0.03f));
		if (Fill) { Fill->SetLightColor(FLinearColor(0.35f, 0.9f, 0.85f)); Fill->SetIntensity(1.2f); }
		SkyLight->SetIntensity(1.f);
		FPostProcessSettings& S = Exposure->Settings;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = -0.7f;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 1.4f;
	}
	else if (Style == ETDPlaceholderSkyStyle::Cinder && Atmosphere)
	{
		Sun->SetWorldRotation(FRotator(-22.f, 130.f, 0.f));
		Sun->SetIntensity(6.f);
		Sun->SetLightColor(FLinearColor(1.f, 0.55f, 0.35f));
		Atmosphere->SetRayleighScattering(FLinearColor(0.03f, 0.011f, 0.005f));
		Atmosphere->SetMieScatteringScale(0.012f);
		if (Fill) { Fill->SetLightColor(FLinearColor(1.f, 0.4f, 0.2f)); Fill->SetIntensity(1.2f); }
		FPostProcessSettings& S = Exposure->Settings;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = -0.4f;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 1.3f;
	}
	SkyLight->RecaptureSky();
}

void ATDPlaceholderSky::ApplyHorizon(ETDPlaceholderSkyStyle Style)
{
	const bool bPlanet = Style == ETDPlaceholderSkyStyle::Verdance || Style == ETDPlaceholderSkyStyle::Cinder;
	if (!bPlanet)
	{
		if (FarGround) { FarGround->DestroyComponent(); FarGround = nullptr; }
		if (HorizonFog) { HorizonFog->DestroyComponent(); HorizonFog = nullptr; }
		return;
	}
	const bool bVerdance = Style == ETDPlaceholderSkyStyle::Verdance;
	if (FarGround)
	{
		// 200 km, 0.5 m under the course slab's top (z = 0): invisible under the slab, the ground beyond it.
		FarGround->SetWorldLocationAndRotation(FVector(0.f, 0.f, -50.f), FRotator::ZeroRotator);
		FarGround->SetWorldScale3D(FVector(200000.f, 200000.f, 1.f));
		const FSoftObjectPath Path(bVerdance ? TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Verdance.MI_Ground_Verdance")
			: TEXT("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Cinder.MI_Ground_Cinder"));
		if (FPackageName::DoesPackageExist(Path.GetLongPackageName()))
		{
			if (UMaterialInterface* M = Cast<UMaterialInterface>(Path.TryLoad())) { FarGround->SetMaterial(0, M); }
		}
		FarGround->SetVisibility(true);
	}
	if (HorizonFog)
	{
		// Clear near the player, thick at the horizon: the edge and the distance melt into a planet-tinted haze.
		HorizonFog->SetWorldLocation(FVector::ZeroVector);
		HorizonFog->SetFogDensity(bVerdance ? 0.018f : 0.022f);
		HorizonFog->SetFogHeightFalloff(0.12f);
		HorizonFog->SetStartDistance(6000.f);
		HorizonFog->SetFogMaxOpacity(1.f);
		HorizonFog->SetFogInscatteringColor(bVerdance ? FLinearColor(0.10f, 0.32f, 0.28f) : FLinearColor(0.42f, 0.16f, 0.05f));
		HorizonFog->SetDirectionalInscatteringColor(bVerdance ? FLinearColor(0.35f, 0.45f, 0.35f) : FLinearColor(0.6f, 0.3f, 0.12f));
		HorizonFog->SetVisibility(true);
	}
}

bool ATDPlaceholderSky::HasHorizon() const
{
	return FarGround && FarGround->IsVisible() && HorizonFog && HorizonFog->IsVisible();
}

bool ATDPlaceholderSky::HasLights() const
{
	return (Sun && Sun->IsRegistered()) || (Fill && Fill->IsRegistered()) || (SkyLight && SkyLight->IsRegistered());
}

bool ATDPlaceholderSky::HasExposure() const
{
	return Exposure && Exposure->IsRegistered();
}

void ATDPlaceholderSky::Refresh()
{
	const FWorldSky Sky = Scan(GetWorld());
	if (Sky.bTaggedSky)
	{
		// Nick's environment brings its own ground and atmosphere.
		if (FarGround) { FarGround->DestroyComponent(); FarGround = nullptr; }
		if (HorizonFog) { HorizonFog->DestroyComponent(); HorizonFog = nullptr; }
	}
	if (Sky.ProvidesLighting())
	{
		if (Sun) { Sun->DestroyComponent(); Sun = nullptr; }
		if (Fill) { Fill->DestroyComponent(); Fill = nullptr; }
		if (SkyLight) { SkyLight->DestroyComponent(); SkyLight = nullptr; }
		if (Atmosphere) { Atmosphere->DestroyComponent(); Atmosphere = nullptr; }
	}
	if (Sky.ProvidesExposure() && Exposure)
	{
		Exposure->DestroyComponent();
		Exposure = nullptr;
	}
	if (!HasLights() && !HasExposure() && !HasHorizon())
	{
		Destroy();
	}
}

void ATDPlaceholderSky::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RescanTimer -= DeltaSeconds;
	if (RescanTimer <= 0.f)
	{
		RescanTimer = RescanInterval;
		Refresh(); // a sky streamed in or spawned late takes over
	}
}

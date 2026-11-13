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
#include "HAL/IConsoleManager.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"
#include "Components/ExponentialHeightFogComponent.h"
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
	if (Style == ETDPlaceholderSkyStyle::Verdance || Style == ETDPlaceholderSkyStyle::Cinder)
	{
		ATDPlanetTerrain::EnsureForWorld(World, Style == ETDPlaceholderSkyStyle::Verdance ? ETDDestination::Verdance : ETDDestination::Cinder);
	}
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
		// Environment pass (Nov 13): a late-afternoon sun high enough to light the valleys, a teal fill from the far side
		// (the old fill came from the sun's side: every shadow was black), fixed exposure, squad colours stay readable.
		Sun->SetWorldRotation(FRotator(-24.f, 200.f, 0.f));
		Sun->SetIntensity(9.f);
		Sun->SetLightColor(FLinearColor(1.f, 0.86f, 0.70f));
		Atmosphere->SetRayleighScattering(FLinearColor(0.0035f, 0.02f, 0.03f));
		Atmosphere->SetAerialPespectiveViewDistanceScale(1.f);
		Atmosphere->SetMieScatteringScale(0.0015f);
		if (Fill) { Fill->SetWorldRotation(FRotator(-35.f, 20.f, 0.f)); Fill->SetLightColor(FLinearColor(0.30f, 0.80f, 0.90f)); Fill->SetIntensity(3.f); }
		SkyLight->SetIntensity(2.f);
		FPostProcessSettings& S = Exposure->Settings;
		S.bOverride_AutoExposureMethod = true;
		S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		S.AutoExposureApplyPhysicalCameraExposure = false;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = 0.f;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 0.9f;
		S.bOverride_BloomThreshold = true;
		S.BloomThreshold = 1.2f;
	}
	else if (Style == ETDPlaceholderSkyStyle::Cinder && Atmosphere)
	{
		// Environment pass (Nov 13): a red sun through ember haze, an ember fill from the far side, fixed exposure.
		Sun->SetWorldRotation(FRotator(-22.f, 130.f, 0.f));
		Sun->SetIntensity(8.f);
		Sun->SetLightColor(FLinearColor(1.f, 0.62f, 0.42f));
		Atmosphere->SetRayleighScattering(FLinearColor(0.03f, 0.011f, 0.005f));
		Atmosphere->SetMieScatteringScale(0.004f);
		if (Fill) { Fill->SetWorldRotation(FRotator(-30.f, 310.f, 0.f)); Fill->SetLightColor(FLinearColor(0.8f, 0.45f, 0.35f)); Fill->SetIntensity(1.2f); }
		SkyLight->SetIntensity(2.f);
		FPostProcessSettings& S = Exposure->Settings;
		S.bOverride_AutoExposureMethod = true;
		S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		S.AutoExposureApplyPhysicalCameraExposure = false;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = 0.f;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 1.f;
		S.bOverride_BloomThreshold = true;
		S.BloomThreshold = 1.f;
	}
	SkyLight->RecaptureSky();
}

// Ryan's standing rule (keep the old look recoverable): td.PlanetTerrain 0 = today's flat slab + far ground, no terrain.
static TAutoConsoleVariable<int32> CVarTDPlanetTerrain(TEXT("td.PlanetTerrain"), 1,
	TEXT("ENG-34 planet worlds: 1 = procedural terrain and mountains (default), 0 = the earlier flat ground and far plane (applies on the next planet load)."));

void ATDPlaceholderSky::TrySpawnTerrain()
{
	const bool bPlanet = AppliedStyle == ETDPlaceholderSkyStyle::Verdance || AppliedStyle == ETDPlaceholderSkyStyle::Cinder;
	if (!bPlanet || !bSpawnTerrain || Terrain || !GetWorld() || IsActorBeingDestroyed() || CVarTDPlanetTerrain.GetValueOnGameThread() == 0) { return; }
	// The world is owned by the terrain itself (once per planet map, never destroyed by a sky rescan): just make sure
	// it exists and drop the flat far ground once it does.
	Terrain = ATDPlanetTerrain::EnsureForWorld(GetWorld(), AppliedStyle == ETDPlaceholderSkyStyle::Verdance ? ETDDestination::Verdance : ETDDestination::Cinder);
	if (Terrain && FarGround)
	{
		FarGround->DestroyComponent();
		FarGround = nullptr;
	}
}

void ATDPlaceholderSky::ApplyHorizon(ETDPlaceholderSkyStyle Style)
{
	AppliedStyle = Style;
	TrySpawnTerrain();
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
		// Thick only near the ground and far away: a steep falloff (density halves every ~0.4 km of height) keeps the
		// view down from the arrival altitude (~220 m) clear, and the start distance keeps the arrival's ground
		// (lava cracks, spires, the outpost) out of the haze; the horizon still melts into the tint.
		BaseFogDensity = bVerdance ? 0.006f : 0.008f;
		HorizonFog->SetFogDensity(BaseFogDensity);
		HorizonFog->SetFogHeightFalloff(bVerdance ? 0.08f : 0.06f);
		HorizonFog->SetStartDistance(bVerdance ? 150000.f : 120000.f);
		HorizonFog->SetFogMaxOpacity(1.f);
		HorizonFog->SetFogInscatteringColor(bVerdance ? FLinearColor(0.04f, 0.20f, 0.22f) : FLinearColor(0.16f, 0.05f, 0.025f));
		HorizonFog->SetDirectionalInscatteringColor(bVerdance ? FLinearColor(0.10f, 0.20f, 0.18f) : FLinearColor(0.35f, 0.13f, 0.05f));
		{
			// Environment pass: the first layer is the valley mist (thins out ~125 m up); the second is the aerial haze
			// that reaches flight altitude (thins out ~3 km up), so from 5 km the horizon fades instead of ending.
			HorizonFog->SetSecondFogDensity(bVerdance ? 0.0016f : 0.0012f);
			HorizonFog->SetSecondFogHeightFalloff(0.003f);
			HorizonFog->SetSecondFogHeightOffset(0.f);
		}
		HorizonFog->SetVisibility(true);
	}
}

bool ATDPlaceholderSky::HasHorizon() const
{
	return ((FarGround && FarGround->IsVisible()) || Terrain) && HorizonFog && HorizonFog->IsVisible();
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
	TickDescentFog();
}

void ATDPlaceholderSky::TickDescentFog()
{
	// ENG-38 focus-in: high above the ground the haze is thick (the world below is still a soft shape); it thins to
	// the planet's normal horizon fog between 6 km and 2 km up, so detail resolves on the way down without a pop.
	if (!GetWorld())
	{
		return;
	}
	// The fog to drive: ours when it is up, else the level's own height fog (a tagged environment sky brings its own,
	// and the placeholder's is removed). Its authored density is the "normal" one.
	UExponentialHeightFogComponent* Fog = (HorizonFog && HorizonFog->IsVisible()) ? HorizonFog.Get() : DescentFog.Get();
	if (!Fog)
	{
		DescentFogSearch -= GetWorld()->GetDeltaSeconds();
		if (DescentFogSearch > 0.f || !FogTerrain.IsValid() && !TActorIterator<ATDPlanetTerrain>(GetWorld()))
		{
			return;
		}
		DescentFogSearch = 2.f;
		for (TObjectIterator<UExponentialHeightFogComponent> It; It; ++It)
		{
			if (It->GetWorld() == GetWorld() && It->IsRegistered() && It->IsVisible() && *It != HorizonFog)
			{
				DescentFog = *It;
				DescentFogBase = It->FogDensity;
				break;
			}
		}
		Fog = DescentFog.Get();
		if (!Fog)
		{
			if (!bDescentFogLogged) { bDescentFogLogged = true; UE_LOG(LogTriadDrift, Log, TEXT("ATDPlaceholderSky: descent fog - no height fog in this level")); }
			return;
		}
	}
	const float Base = Fog == HorizonFog ? BaseFogDensity : DescentFogBase;
	if (Base <= 0.f)
	{
		return;
	}
	if (!FogTerrain.IsValid())
	{
		for (TActorIterator<ATDPlanetTerrain> It(GetWorld()); It; ++It) { FogTerrain = *It; break; }
		if (!FogTerrain.IsValid()) { return; }
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector Cam = PC->PlayerCameraManager->GetCameraLocation();
	const float Alt = Cam.Z - FogTerrain->GetHeightAt(Cam);
	DescentFogScale = 1.f + (DescentFogMaxScale - 1.f) * FMath::SmoothStep(DescentFogLowCm, DescentFogHighCm, Alt);
	Fog->SetFogDensity(Base * DescentFogScale);
	if (!bDescentFogLogged)
	{
		bDescentFogLogged = true;
		UE_LOG(LogTriadDrift, Log, TEXT("ATDPlaceholderSky: descent fog drives %s (base density %.4f, x%.1f at %.0f m up)"), *Fog->GetPathName(), Base, DescentFogScale, Alt / 100.f);
	}
}

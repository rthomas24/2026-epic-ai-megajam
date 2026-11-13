// Triad Drift - placeholder lighting and exposure until Nick's sky exists (N-004 contract). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDPlaceholderSky.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UPostProcessComponent;
class UExponentialHeightFogComponent;
class UStaticMeshComponent;
class USkyAtmosphereComponent;

UENUM()
enum class ETDPlaceholderSkyStyle : uint8
{
	Space,   // warm sun + violet fill + sky light, fixed exposure, bloom
	Planet,  // daylight sun + real-time sky light + sky atmosphere
	Verdance, // planet, at dusk: a low cool sun and a teal sky so the bioluminescence glows
	Cinder    // planet, in ember haze: a red sun through a rust-red sky
};

/**
 * One actor holding every placeholder light and the placeholder post-process, tagged
 * "TD.Placeholder.Sky". The game modes spawn it only when the map provides no sky of its own; it
 * removes itself as soon as one appears (streamed in or spawned late).
 *
 * The map "provides a sky" when ANY of these is in the world (agreed with Nick's side, N-004):
 * - an actor tagged "TD.Presentation.Sky" (switches ALL placeholder lighting and exposure off)
 * - any actor (other than this one) with a UDirectionalLightComponent: we drop our lights
 * - any unbound UPostProcessComponent or unbound APostProcessVolume: we drop our exposure / bloom
 */
UCLASS(NotPlaceable)
class UNREALGAMEJAM_API ATDPlaceholderSky : public AActor
{
	GENERATED_BODY()

public:
	ATDPlaceholderSky();

	static const FName PresentationSkyTag;   // "TD.Presentation.Sky"
	static const FName PlaceholderTag;        // "TD.Placeholder.Sky"

	/** What the world already provides (ignores placeholder actors). */
	struct FWorldSky
	{
		bool bTaggedSky = false;
		bool bDirectionalLight = false;
		bool bPostProcess = false;
		bool ProvidesLighting() const { return bTaggedSky || bDirectionalLight; }
		bool ProvidesExposure() const { return bTaggedSky || bPostProcess; }
	};
	static FWorldSky Scan(const UWorld* World);

	/** Spawns the placeholder if the world needs any of it; null when the map already has a full sky. */
	static ATDPlaceholderSky* SpawnIfNeeded(UWorld* World, ETDPlaceholderSkyStyle Style);

	void ApplyStyle(ETDPlaceholderSkyStyle Style);

	/** Planet styles: far ground to the horizon + tinted height fog; other styles remove both. */
	void ApplyHorizon(ETDPlaceholderSkyStyle Style);

	/** Re-scan: drop the parts the world now provides; destroy self when nothing is left. */
	void Refresh();

	bool HasLights() const;
	bool HasExposure() const;

	/** Seconds between re-scans for a late sky. */
	UPROPERTY(EditAnywhere, Category = "Placeholder")
	float RescanInterval = 0.5f;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Placeholder") TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere, Category = "Placeholder") TObjectPtr<UDirectionalLightComponent> Fill;
	UPROPERTY(VisibleAnywhere, Category = "Placeholder") TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(VisibleAnywhere, Category = "Placeholder") TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(VisibleAnywhere, Category = "Placeholder") TObjectPtr<UPostProcessComponent> Exposure;
	/** Planet styles: the ground carried to the horizon (a 200 km plane just under the course slab, same material). */
	UPROPERTY(VisibleAnywhere, Category = "Placeholder") TObjectPtr<UStaticMeshComponent> FarGround;
	/** Planet styles: planet-tinted height fog so the distance blends into the sky. */
	UPROPERTY(VisibleAnywhere, Category = "Placeholder") TObjectPtr<UExponentialHeightFogComponent> HorizonFog;

	/** True once a planet style built its far ground and haze (tests). */
	bool HasHorizon() const;

	/** ENG-34: the planet world (procedural terrain), spawned at the course origin once the course exists. */
	UPROPERTY(Transient) TObjectPtr<class ATDPlanetTerrain> Terrain;

public:
	/** ENG-38 focus-in: horizon fog density multiplier high up (DescentFogHighCm and above), easing to 1 at DescentFogLowCm. */
	UPROPERTY(EditAnywhere, Category = "Placeholder") float DescentFogMaxScale = 12.f;
	UPROPERTY(EditAnywhere, Category = "Placeholder") float DescentFogHighCm = 500000.f;
	UPROPERTY(EditAnywhere, Category = "Placeholder") float DescentFogLowCm = 150000.f;
	/** The multiplier applied last frame (tests). */
	float GetDescentFogScale() const { return DescentFogScale; }
	float GetBaseFogDensity() const { return BaseFogDensity; }
	void TickDescentFog();

protected:
	float BaseFogDensity = 0.f;
	float DescentFogScale = 1.f;
	TWeakObjectPtr<class ATDPlanetTerrain> FogTerrain;
	TWeakObjectPtr<class UExponentialHeightFogComponent> DescentFog;
	float DescentFogBase = 0.f;
	float DescentFogSearch = 0.f;
	bool bDescentFogLogged = false;
	/** Planet styles spawn the terrain (off in tests that only check lighting). */
	UPROPERTY(EditAnywhere, Category = "Placeholder") bool bSpawnTerrain = true;
	void TrySpawnTerrain();
	ETDPlaceholderSkyStyle AppliedStyle = ETDPlaceholderSkyStyle::Space;

private:
	float RescanTimer = 0.f;
};

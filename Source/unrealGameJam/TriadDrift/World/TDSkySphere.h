// Triad Drift - World (Teammate 4). The engineering nebula sky for space (art audit, Nov 13): the builder's
// SM_TD_SkySphere + MI_TD_NebulaSky (ArtSource/EngProps/Sky/README.md), centred on the camera every frame.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDSkySphere.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * An inverted, unlit "Is Sky" sphere 100 km in radius that follows the camera, so it is always the background and
 * never occludes anything in the galaxy (the whole play space is well inside it). No collision, no shadows.
 * Tagged "TD.Engineering.Sky". Nick's sky (an actor tagged "TD.Presentation.Sky") wins: this one hides itself
 * while such an actor exists. The sun baked into the texture is the one tunable the space lighting reads.
 */
UCLASS(NotPlaceable)
class UNREALGAMEJAM_API ATDSkySphere : public AActor
{
	GENERATED_BODY()

public:
	ATDSkySphere();

	static const FName EngineeringSkyTag;	// "TD.Engineering.Sky"

	/** Where the baked sun sits (README: Yaw 35, Pitch +18 with the sphere unrotated). The single sun tunable. */
	static constexpr float SunYawDeg = 35.f;
	static constexpr float SunPitchDeg = 18.f;

	/** Rotation for a directional light that shines FROM the baked sun (Pitch -18, Yaw 215). */
	static FRotator SunLightRotation() { return FRotator(-SunPitchDeg, SunYawDeg + 180.f, 0.f); }

	/** True when the sky mesh and material exist (they are optional; the star backdrop stays without them). */
	static bool IsArtAvailable();

	/** Spawns the sky unless the art is missing or a presentation sky is already there. */
	static ATDSkySphere* SpawnIfAvailable(UWorld* World);

	/** Sky radius (cm). Far beyond the play space; the far plane is infinite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Sky", meta = (ClampMin = "100000"))
	float RadiusCm = 10000000.f;

	/** MI parameter "Brightness" (README: raise to taste against the exposure). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Sky", meta = (ClampMin = "0"))
	float Brightness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Sky")
	TSoftObjectPtr<UStaticMesh> SkyMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Sky")
	TSoftObjectPtr<UMaterialInterface> SkyMaterial;

	/** True while a presentation sky has taken over (this one is hidden). */
	bool IsYielding() const { return bYielding; }

	/** Hide when a TD.Presentation.Sky actor exists, show again when it is gone. */
	void RefreshYield();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Sky")
	TObjectPtr<UStaticMeshComponent> Sky;

	float RescanTimer = 0.f;
	bool bYielding = false;
	bool bRecaptured = false;
};

// Triad Drift - Powers (Teammate 2). Presentation for powers: Nick's data assets by path convention, and
// engine-shape placeholders that exist in Shipping until art replaces them (B2 of the Powers review).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDPowerPresentation.generated.h"

class UTDShipVisuals;
class UTDAbilityFX;
class UTDTravelFX;
class UTDWeaponFX;
class UStaticMeshComponent;
class UStaticMesh;
class UInstancedStaticMeshComponent;
class UNiagaraComponent;
class USceneComponent;
struct FTDFXCue;

/**
 * Data assets live in one flat folder (docs/visual/Presentation-Asset-Paths.md):
 *   /Game/TriadDrift/Presentation/DA_<Rift|Lancer|Bastion>_Visuals   (UTDShipVisuals: power wind-up / active / end cues)
 *   /Game/TriadDrift/Presentation/DA_<WeaponId>_FX                    (UTDWeaponFX)
 *   /Game/TriadDrift/Presentation/DA_Ability_<GravityWell|Cloak|EMPPulse>_FX (UTDAbilityFX, proposed)
 *   /Game/TriadDrift/Presentation/DA_Travel_FX                        (UTDTravelFX: portals, proposed)
 * All optional: a missing asset returns null and is remembered (no repeated disk probes).
 */
struct UNREALGAMEJAM_API FTDPowerPresentation
{
	static UTDShipVisuals* LoadShipVisuals(ETDShipId Ship);
	static UTDAbilityFX* LoadAbilityFX(ETDAbilityId Ability);
	static UTDTravelFX* LoadTravelFX();
	static UTDWeaponFX* LoadWeaponFX(FName WeaponId);

	/**
	 * Play a cue through a named request (e.g. "Ship.PowerActive", "Travel.PortalOpen", "Ability.Loop").
	 * Every request is counted, art or not, so tests can assert each moment asks for its cue before art
	 * exists. A map increment per request: cheap enough for all builds.
	 */
	static UNiagaraComponent* RequestCue(const UObject* WorldContext, FName Moment, const FTDFXCue& Cue, const FVector& Location,
		const FRotator& Rotation = FRotator::ZeroRotator);
	static UNiagaraComponent* RequestCueAttached(FName Moment, const FTDFXCue& Cue, USceneComponent* AttachTo);
	static int32 GetCueRequests(FName Moment);
	static void ResetCueRequests();

	/** True when the cue has art (so the placeholder should stay hidden). */
	static bool HasArt(const FTDFXCue& Cue);

	enum class EShape : uint8 { Sphere, Cylinder, Cone, Cube };

	/** Engine basic shape, no collision or shadows, tinted (emissive) in Color. Shipping-safe. */
	static UStaticMeshComponent* AddShape(AActor* Owner, USceneComponent* Parent, EShape Shape, const FVector& Scale,
		const FRotator& RelativeRotation, FLinearColor Color, float EmissiveBoost);

	/**
	 * A wire-like sphere: Rings great circles (1 = horizontal ring, 3 = XY/XZ/YZ) of thin cube segments in ONE
	 * instanced component. Reads as a shield shell without a translucent material.
	 */
	static UInstancedStaticMeshComponent* AddRingLattice(AActor* Owner, USceneComponent* Parent, float Radius, int32 Rings,
		int32 SegmentsPerRing, float Thickness, FLinearColor Color, float EmissiveBoost);

	/** Debug/inspection: placeholder components this module created on Owner. */
	static int32 CountPlaceholders(const AActor* Owner);

	static const FName PlaceholderTag;
};

/** A short-lived placeholder burst (EMP pulse) so one-shot effects are visible in Shipping without art. */
UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDPlaceholderFlash : public AActor
{
	GENERATED_BODY()

public:
	ATDPlaceholderFlash();

	static ATDPlaceholderFlash* SpawnRingFlash(UWorld* World, const FVector& Location, float Radius, FLinearColor Color, float Seconds);

	virtual void Tick(float DeltaSeconds) override;

	/** Engine basic shapes, hard-referenced by this class's default object so they are always cooked. */
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

private:
	float Life = 0.4f;
	float Age = 0.f;
};

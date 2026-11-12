// Triad Drift - Combat (Teammate 2). Placeholder looks that read before art exists: projectile streaks,
// muzzle flashes and impact flashes. Each hides as soon as the weapon's DA_<Id>_FX supplies its slot.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDCombatPlaceholders.generated.h"

struct FTDProjectileSpec;
struct FTDFXCue;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** How a projectile's placeholder is drawn: a glowing streak trailing the shot, plus a head on missiles. */
struct FTDStreakLook
{
	/** False for slow things (mines): they keep their PlaceholderScale sphere. */
	bool bStreak = false;
	/** Rockets, missiles, shells: fatter and shorter. */
	bool bHeavy = false;
	/** A bright head sphere at the front (homing missiles). */
	bool bHead = false;
	float Length = 0.f;
	float Thickness = 0.f;
	float HeadDiameter = 0.f;
};

struct UNREALGAMEJAM_API FTDCombatPlaceholders
{
	/** Placeholder glow strength (Color * this) for streaks and flashes. */
	static constexpr float StreakGlow = 10.f;
	static constexpr float FlashGlow = 14.f;

	/**
	 * Lasers and needles: 300-800 cm long (0.015 s of flight), 35-60 cm thick. Rockets, missiles and shells:
	 * 250-500 cm, 70-140 cm thick (slim missiles from 40). Heavy shells get a head 1.8x the thickness, missiles 1.4x.
	 */
	static FTDStreakLook ComputeStreak(const FTDProjectileSpec& Spec);

	/**
	 * Seen from the chase camera a shot flies away end-on, so its length doesn't show: its thickness is what
	 * reads. Thickness grows with distance from the camera to keep about MinStreakPixels (11) at 1280 wide and
	 * 90 degrees FOV, up to MaxDistanceGrowth times the base. Returns the scale factor (>= 1).
	 */
	static float DistanceGrowth(const FTDStreakLook& Look, float DistanceToCamera);
	/** Same, with an explicit on-screen floor (incoming hostile fire uses MinIncomingStreakPixels). */
	static float DistanceGrowth(const FTDStreakLook& Look, float DistanceToCamera, float MinPixels);

	/**
	 * Hostile fire flying TOWARD the camera is seen head-on: its streak is a dot. It gets a hot head and this
	 * on-screen floor so incoming fire reads before it lands (T1's feel pass).
	 */
	static constexpr float MinIncomingStreakPixels = 18.f;
	static constexpr float IncomingHeadScale = 2.2f;
	static bool IsIncoming(const FVector& Velocity, const FVector& ShotLocation, const FVector& CameraLocation)
	{
		return FVector::DotProduct(Velocity, ShotLocation - CameraLocation) < 0.f;
	}
	static constexpr float MinStreakPixels = 11.f;
	/** Heavy shells read bigger than needles: the heavy ship's gun should look heavy. */
	static constexpr float MinHeavyStreakPixels = 18.f;
	static constexpr float MaxDistanceGrowth = 8.f;

	/**
	 * True when Cue draws something (a VFX). A sound- or shake-only cue does not replace a placeholder
	 * flash: hiding on FTDFXCue::IsEmpty would make every flash vanish the moment audio lands.
	 */
	static bool HasVisualArt(const FTDFXCue& Cue);

	/**
	 * Caps a muzzle flash to about a third of the hull across at its biggest (it swells to 1.2x): HullRadius is the
	 * shooter's collision radius. 0 or less = no cap.
	 */
	static float CapMuzzleFlashRadius(float Radius, float HullRadius);

	/** Muzzle flash radius for a shot of Spec. */
	static float MuzzleFlashRadius(const FTDProjectileSpec& Spec);

	/** Impact flash radius (splash weapons scale with their splash). */
	static float ImpactFlashRadius(const FTDProjectileSpec& Spec);
};

/**
 * A short glowing sphere that swells and fades: the muzzle / impact flash placeholder. Pooled by
 * UTDProjectileSubsystem::PlayPlaceholderFlash; never spawn it directly. The engine sphere is hard-referenced by the
 * class default object; the glow is Core's always-cooked M_TD_PlaceholderGlow.
 */
UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDCombatFlash : public AActor
{
	GENERATED_BODY()

public:
	ATDCombatFlash();

	void Play(const FVector& Location, float Radius, const FLinearColor& Color, float Seconds, USceneComponent* AttachTo);
	bool IsPlaying() const { return bPlaying; }
	float GetRadius() const { return Radius; }
	void Stop();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;

private:
	void Apply(float Alpha);

	UPROPERTY(VisibleAnywhere, Category = "Flash")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID;

	FLinearColor Color = FLinearColor::White;
	float Radius = 100.f;
	float Life = 0.1f;
	float Age = 0.f;
	bool bPlaying = false;
};

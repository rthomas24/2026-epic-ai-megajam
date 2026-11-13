// Triad Drift - Combat (Teammate 2). Placeholder looks that read before art exists: projectile streaks,
// muzzle flashes and impact flashes. Each hides as soon as the weapon's DA_<Id>_FX supplies its slot.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDCombatPlaceholders.generated.h"

struct FTDProjectileSpec;
enum class ETDTeam : uint8;
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
	 * On foot the camera is a metre or two from the shot, so the space streak (35+ cm thick, 2.2x incoming head, glow 10)
	 * reads as a glowing ball (T5's journey check; the Lead's Heavy pellets at 5 m). On-foot bolts: 40-80 cm long
	 * (0.02 s of flight), 5-9 cm thick, a head 1.4x the thickness, glow OnFootGlow so red-orange does not bloom to white;
	 * they grow at most OnFootMaxGrowth with distance and get no incoming boost.
	 */
	static FTDStreakLook ComputeOnFootStreak(const FTDProjectileSpec& Spec);

	/**
	 * Squad shots in space ("bright weapon trails"): seen from the chase camera a shot flies away end-on, so a short
	 * streak reads as a dot. Light shots become long tracers, SquadTracerSeconds of flight (up to MaxSquadTracerCm), that
	 * grow out of the muzzle as they fly (never reaching back past the ship or the camera) and carry a near-white tip so
	 * they never vanish against a sky of their own colour. The squad's heavy shots keep their length with a smaller
	 * head (stacked 1.8x heads made a pale ball over the reticle).
	 */
	static FTDStreakLook ComputeSquadStreak(const FTDProjectileSpec& Spec);
	static constexpr float SquadTracerSeconds = 0.04f;
	static constexpr float MaxSquadTracerCm = 2500.f;
	/** The part of a streak drawn so far: the distance flown, between the thickness and the full length. */
	static float VisibleLength(const FTDStreakLook& Look, float FlownCm) { return FMath::Clamp(FlownCm, Look.Thickness, FMath::Max(Look.Length, Look.Thickness)); }
	/** The near-white tracer tip: a fifth of the visible streak, 1.5x the thickness to 400 cm. */
	static float TipLength(const FTDStreakLook& Look, float Visible) { return FMath::Clamp(Visible * 0.2f, Look.Thickness * 1.5f, 400.f); }
	static constexpr float TipWhite = 0.45f;
	static constexpr float TipThickness = 0.85f;

	/** Muzzle, impact and explosion flashes keep at least this radius per cm of camera distance (~20 px at 300 m, 1080p). */
	static constexpr float MinFlashRadiusPerCameraDistance = 0.007f;

	/**
	 * A projectile head (missiles, heavy shells, incoming hostile fire) is never wider than this fraction of its distance to
	 * the camera (~50 px at 1080p): live in a wave-5 furball an incoming bolt passing a few metres from the camera drew its
	 * 2.2x head as a pale ball over the player's ship.
	 */
	static constexpr float MaxHeadPerCameraDistance = 0.045f;
	/** Enemy heads in space: pure Armada colour at this glow (x StreakGlow), not pushed toward white. */
	static constexpr float EnemyHeadGlow = 0.7f;
	static constexpr float MinOnFootStreakPixels = 5.f;
	/** Live on Cinder, glow 3 tonemapped to (255, 231, 225), a pale pink-white dot; the lit floor is about 0.18. 1.2 stays
	 *  saturated red-orange and still stands well clear of the ground. */
	static constexpr float OnFootGlow = 1.2f;
	static constexpr float OnFootMaxGrowth = 3.f;
	static float OnFootDistanceGrowth(const FTDStreakLook& Look, float DistanceToCamera);
	/** On-foot impact spark radius: 3x the bolt's thickness, 15-30 cm (space impacts are 140-200 cm). */
	static float OnFootImpactRadius(const FTDStreakLook& Look) { return FMath::Clamp(Look.Thickness * 3.f, 15.f, 30.f); }

	/**
	 * Colour code (AGENTS.md "Look and feel"): squad shots in their ship's colour, every enemy shot in Armada
	 * red-orange, a deflected shot in the colour of whoever turned it (the blade's colour on foot). An FX colour
	 * from Nick's DA_<Id>_FX wins, except after a deflection.
	 *   Shooter: the current instigator. BladeColor: used when a non-ship deflected it.
	 */
	static FLinearColor ProjectileColor(const FTDProjectileSpec& Spec, ETDTeam Team, const AActor* Shooter, bool bDeflected);
	/** Armada hues: red to orange (hue 0-45 degrees, saturated). Anything else on an enemy shot is replaced. */
	static bool IsArmadaHue(const FLinearColor& Color);
	/** The blade's colour on Shooter (its UTDBladeFX), else the placeholder blade's default. */
	static FLinearColor BladeColorOf(const AActor* Shooter);

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
	/** A debris streak: flies at Velocity, stretched along it (Length x Thickness), shrinking as it fades. */
	void PlayDebris(const FVector& Location, const FVector& Velocity, float Length, float Thickness, const FLinearColor& Color, float Seconds);
	/** A flash sphere's radius never exceeds this fraction of its distance to the player's camera. */
	static constexpr float MaxRadiusPerCameraDistance = 0.3f;
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
	FVector DebrisVelocity = FVector::ZeroVector;
	float DebrisLength = 0.f;
	bool bDebris = false;
	float Radius = 100.f;
	float Life = 0.1f;
	float Age = 0.f;
	bool bPlaying = false;
};

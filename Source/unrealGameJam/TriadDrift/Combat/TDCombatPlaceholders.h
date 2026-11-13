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
 * A styled flash (explosion parts): drawn with the additive M_TD_FlashGlow (soft-edged, fades to transparent) when the
 * project has it, its colour running Hot -> Mid -> Cool over its life (40% / 60%) and its glow falling off, or rising
 * and falling for an ember afterglow. Size eases out from GrowFrom to GrowTo x the radius. Delay hides it first.
 */
struct FTDFlashLook
{
	FLinearColor Hot = FLinearColor::White;
	FLinearColor Mid = FLinearColor(1.f, 0.6f, 0.15f);
	FLinearColor Cool = FLinearColor(0.5f, 0.05f, 0.02f);
	float Glow = 8.f;
	/** Glow x (1 - t)^GlowExponent (ignored for an ember). */
	float GlowExponent = 1.5f;
	/** Glow x sin(pi t): an afterglow that swells in and dies away. */
	bool bEmber = false;
	float GrowFrom = 0.4f;
	float GrowTo = 1.f;
	float Delay = 0.f;
	/** M_TD_FlashGlow edge falloff (higher = softer edge, smaller bright centre). */
	float EdgePower = 1.5f;
	/** Brightest at the rim instead of the centre (the shockwave ring). */
	bool bRim = false;
	/** Per-axis scale on the sphere (a flat disc for the shockwave: (1, 1, 0.04)). */
	FVector Squash = FVector::OneVector;
	/** Turn the squashed axis toward the player's camera at the start (a ring seen face-on). */
	bool bFaceCamera = false;
	/** Dark translucent smoke (M_TD_SmokePuff): Cool..Hot is its colour, Opacity its peak, swelling in and fading out. */
	bool bSmoke = false;
	float Opacity = 0.55f;
};

/**
 * A short glowing sphere that swells and fades: the muzzle / impact flash placeholder, and the parts of a placeholder
 * explosion (PlayLook). Pooled by UTDProjectileSubsystem; never spawn it directly. The engine sphere is hard-referenced
 * by the class default object; the glows are M_TD_PlaceholderGlow (Core) and M_TD_FlashGlow, both always cooked.
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
	/** A styled sphere (explosion core, shell, pop, ember). */
	void PlayLook(const FVector& Location, float Radius, float Seconds, const FTDFlashLook& Look);
	/** A styled debris spark. */
	void PlayDebrisLook(const FVector& Location, const FVector& Velocity, float Length, float Thickness, float Seconds, const FTDFlashLook& Look);
	/**
	 * A hull chunk: a small cube in HullMaterial (the dying ship's own material; dark grey when null) flung at Velocity,
	 * tumbling at AngularDegPerSec and shrinking away over Seconds.
	 */
	void PlayChunk(const FVector& Location, const FVector& Velocity, float Size, UMaterialInterface* HullMaterial, float Seconds,
		const FVector& AngularDegPerSec);
	bool IsChunk() const { return bPlaying && bChunk; }
	bool IsDebris() const { return bPlaying && bDebris; }
	/** The translucent smoke material (/Game/TriadDrift/Gameplay/Materials/M_TD_SmokePuff), or null if missing. */
	static UMaterialInterface* GetSmokeMaterial();
	bool IsSmoke() const { return bPlaying && bStyled && StyledLook.bSmoke; }
	/** The additive soft glow (/Game/TriadDrift/Gameplay/Materials/M_TD_FlashGlow, always cooked), or null if missing. */
	static UMaterialInterface* GetFlashGlowMaterial();
	bool IsStyled() const { return bStyled; }
	bool IsDelayed() const { return bPlaying && DelayLeft > 0.f; }
	/** Emissive written last (tests). */
	FLinearColor GetLastColor() const { return LastColor; }
	bool UsesAdditiveGlow() const { return bStyled && AdditiveMID && Mesh->GetMaterial(0) == AdditiveMID; }
	/** A flash sphere's radius never exceeds this fraction of its distance to the player's camera (default cap). */
	static constexpr float MaxRadiusPerCameraDistance = 0.3f;
	/**
	 * Explosion shells: radius at most this fraction of the camera distance, about 25% of the screen height at 16:9 and
	 * 90 degrees horizontal FOV (tan(7.3 deg)). The Lead: a near death must not fill more than ~25% of the screen height.
	 */
	static constexpr float MaxShellRadiusPerCameraDistance = 0.128f;
	/** Debris streaks near the camera: thickness and length at most these fractions of the camera distance. */
	static constexpr float MaxDebrisThicknessPerCameraDistance = 0.02f;
	static constexpr float MaxDebrisLengthPerCameraDistance = 0.25f;
	/** This play's camera cap (Play resets it to MaxRadiusPerCameraDistance). */
	void SetCameraCap(float RadiusPerCameraDistance)
	{
		CameraCap = FMath::Max(RadiusPerCameraDistance, 0.01f);
		if (bPlaying)
		{
			Apply(FMath::Clamp(Age / Life, 0.f, 1.f)); // at once, also for a delayed part still waiting its turn
		}
	}
	float GetCameraCap() const { return CameraCap; }
	bool IsPlaying() const { return bPlaying; }
	float GetRadius() const { return Radius; }
	void Stop();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;

private:
	void Apply(float Alpha);

	UPROPERTY(VisibleAnywhere, Category = "Flash")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AdditiveMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SmokeMID;

	bool bChunk = false;
	float ChunkSize = 50.f;
	/** A plate, not a cube: per-axis proportions picked per chunk. */
	FVector ChunkAspect = FVector::OneVector;
	FRotator ChunkSpin = FRotator::ZeroRotator;

	void UseStyledMaterial();
	bool bStyled = false;
	FTDFlashLook StyledLook;
	float DelayLeft = 0.f;
	FLinearColor LastColor = FLinearColor::Black;

	FLinearColor Color = FLinearColor::White;
	float CameraCap = MaxRadiusPerCameraDistance;
	FVector DebrisVelocity = FVector::ZeroVector;
	float DebrisLength = 0.f;
	bool bDebris = false;
	float Radius = 100.f;
	float Life = 0.1f;
	float Age = 0.f;
	bool bPlaying = false;
};

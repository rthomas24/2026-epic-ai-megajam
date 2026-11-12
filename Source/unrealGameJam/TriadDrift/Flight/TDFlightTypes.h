// Triad Drift - flight tuning and simulation state (ENG-01). Owner: Teammate 1 (Flight & Input).

#pragma once

#include "CoreMinimal.h"
#include "TDFlightTypes.generated.h"

/**
 * Every number the arcade flight model uses. Defaults are the features.md ENG-01 start values;
 * per-ship values (ENG-04) are applied on top through ATDSpacecraft::ApplyFlightTuning.
 * Speeds in cm/s, rates in degrees/s, times in seconds.
 */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDFlightTuning
{
	GENERATED_BODY()

	// ---- Speed ---------------------------------------------------------------------------------

	/** Slowest the ship flies (throttle fully back or braking). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float MinSpeed = 1500.f;

	/** Speed at Throttle +1. FTDFlightInput::Throttle maps -1..+1 linearly to MinSpeed..CruiseSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float CruiseSpeed = 6000.f;

	/** Speed while boosting. Default is cruise +80% (ENG-01); ENG-04 sets it per ship. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float BoostSpeed = 10800.f;

	/** Engine acceleration toward a higher target speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float Acceleration = 9000.f;

	/** Acceleration while boosting (boost kicks in hard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float BoostAcceleration = 24000.f;

	/** Deceleration toward a lower target speed (throttle back, boost ended, drift burst bleeding off). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float Deceleration = 6000.f;

	/** Deceleration while the brake is held: hard stop down to MinSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float BrakeDeceleration = 18000.f;

	/** How quickly velocity re-aligns with the nose (1/s). Higher = grippier, lower = slidier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float VelocityGrip = 10.f;

	// ---- Rotation ------------------------------------------------------------------------------

	/** Max pitch and yaw rate at full aim deflection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation", meta = (ClampMin = "0"))
	float TurnRate = 110.f;

	/** Max manual roll rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation", meta = (ClampMin = "0"))
	float RollRate = 180.f;

	/** How fast the angular rate follows the stick (1/s). Higher = snappier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation", meta = (ClampMin = "0"))
	float AngularResponse = 12.f;

	/** Auto-level proportional gain: deg/s of roll per degree of bank. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation", meta = (ClampMin = "0"))
	float AutoLevelGain = 2.5f;

	/** Above this |forward.Z| the nose counts as vertical and auto-level is suspended. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation", meta = (ClampMin = "0", ClampMax = "1"))
	float VerticalNoseLimit = 0.9f;

	/** Gentle roll back toward world up when there is no roll input (deg/s). 0 = off (pure 6DOF). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation", meta = (ClampMin = "0"))
	float AutoLevelRate = 40.f;

	/** Visual-only bank of the hull into turns at full yaw (degrees). Does not change the flight frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation", meta = (ClampMin = "0", ClampMax = "90"))
	float AutoBankAngle = 40.f;

	/** How quickly the visual bank follows (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation", meta = (ClampMin = "0"))
	float AutoBankResponse = 6.f;

	// ---- Strafe --------------------------------------------------------------------------------

	/** Lateral / vertical thruster speed at full strafe input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Strafe", meta = (ClampMin = "0"))
	float StrafeSpeed = 2500.f;

	/** How quickly strafe velocity follows input (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Strafe", meta = (ClampMin = "0"))
	float StrafeAcceleration = 10000.f;

	// ---- Boost ---------------------------------------------------------------------------------

	/** Seconds of continuous boost from a full meter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boost", meta = (ClampMin = "0.1"))
	float BoostDuration = 3.f;

	/** Seconds after boost ends before the meter starts refilling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boost", meta = (ClampMin = "0"))
	float BoostRegenDelay = 1.2f;

	/** Seconds to refill an empty meter once regen has started (lower = faster regen, e.g. Lancer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boost", meta = (ClampMin = "0.1"))
	float BoostRegenTime = 2.5f;

	/** Meter fraction needed to START a boost (prevents stutter-boosting on an empty meter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boost", meta = (ClampMin = "0", ClampMax = "1"))
	float BoostMinToStart = 0.1f;

	// ---- Drift ---------------------------------------------------------------------------------

	/** Turn rate multiplier while drifting (the nose swings freely). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Drift", meta = (ClampMin = "0"))
	float DriftTurnMultiplier = 1.5f;

	/** Speed burst on drift release, as a fraction of the drift speed (0.2 = +20%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Drift", meta = (ClampMin = "0"))
	float DriftReleaseBurst = 0.2f;

	/** The release burst is only granted after drifting this long... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Drift", meta = (ClampMin = "0"))
	float DriftBurstMinSeconds = 0.25f;

	/** ...or after the nose has swung at least this far from where the drift started (deg). Stops tap-spam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Drift", meta = (ClampMin = "0"))
	float DriftBurstMinAngle = 20.f;

	/** Seconds after a release burst before another drift can earn one (stops drift-pumping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Drift", meta = (ClampMin = "0"))
	float DriftBurstCooldown = 1.5f;

	/** Speed lost per second while drifting (cm/s^2). 0 = drift keeps all speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Drift", meta = (ClampMin = "0"))
	float DriftDrag = 300.f;

	/** Bleed-off rate of speed above the current target after a drift burst (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Drift", meta = (ClampMin = "0"))
	float OverspeedDeceleration = 2500.f;

	/** Deceleration when far above boost speed (dropping out of hyperdrive, end of a ram). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Speed", meta = (ClampMin = "0"))
	float HighOverspeedDeceleration = 30000.f;

	// ---- Atmosphere ground floor ----------------------------------------------------------------
	/**
	 * In atmosphere, ground below the hull closer than this (cm) starts a soft floor: the sink is cancelled and
	 * the nose is pulled toward level, so a hands-off or diving ship skims the ground instead of grinding into it.
	 * 0 = off.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Atmosphere", meta = (ClampMin = "0"))
	float GroundAvoidHeight = 3000.f;

	/** Nose pull-up toward level at the ground (deg/s), scaled by how deep into GroundAvoidHeight the ship is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Atmosphere", meta = (ClampMin = "0"))
	float GroundPullUpRate = 120.f;

	/**
	 * Pinned against something (holding into a wall, a planet shell, the Dominion hull): the nose swings toward the
	 * surface at this rate (deg/s) and the ship keeps at least MinSpeed along it, so it slides off instead of
	 * sitting at 0 m/s. Exactly head-on, it pulls up (the ship's own up, projected on the surface).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0"))
	float PinnedTurnRate = 120.f;

	// ---- Collision -----------------------------------------------------------------------------

	/** Impact "head-on-ness" (0 = parallel graze, 1 = straight into the surface) above which a hit counts as head-on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0", ClampMax = "1"))
	float HeadOnThreshold = 0.7f;

	/** Damage for a head-on hit at cruise speed; scales linearly with impact speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0"))
	float HeadOnDamage = 60.f;

	/** Damage for a glancing hit at cruise speed and HeadOnThreshold angle; scales with angle and speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0"))
	float GlancingDamage = 12.f;

	/** Impacts slower than this into the surface (cm/s) are resolved silently: no event, no damage (resting contact). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0"))
	float MinImpactSpeed = 300.f;

	/** Fraction of the bounced velocity kept after a head-on hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0", ClampMax = "1"))
	float HeadOnSpeedRetain = 0.5f;

	/** How far the nose turns toward the surface tangent on a scrape (0 = not at all, 1 = fully). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0", ClampMax = "1"))
	float ScrapeNoseAlign = 0.5f;

	/** Fraction of the into-surface speed bounced back on a head-on hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0", ClampMax = "1"))
	float HeadOnRestitution = 0.35f;

	/** Fraction of tangential speed kept when scraping along a surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0", ClampMax = "1"))
	float ScrapeFriction = 0.85f;

	/** Small outward pop on a glancing hit so the ship doesn't stick to the surface (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0"))
	float ScrapeBounceSpeed = 400.f;

	/** No single collision deals more than this (cm/s above BoostSpeed count as BoostSpeed too). Never one-shots a ship. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0"))
	float MaxImpactDamage = 60.f;

	/** Touching the same actor again within this long is resting contact: resolved, but no event or damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0"))
	float RestingContactWindow = 0.5f;

	/** Minimum seconds between two collision damage events (no damage machine-gunning while scraping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision", meta = (ClampMin = "0"))
	float CollisionDamageCooldown = 0.3f;

	// ---- Play-space boundary -------------------------------------------------------------------

	/** Default radius when no world play bounds are set (UTDTravelSubsystem::SetPlayBounds / SetBoundary win). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boundary", meta = (ClampMin = "0"))
	float BoundaryRadius = 1500000.f;

	/** Warning band inside the radius (IsNearBoundary). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boundary", meta = (ClampMin = "0"))
	float BoundaryWarnDistance = 50000.f;

	/** Depth past the radius at which steering and push-back reach full strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boundary", meta = (ClampMin = "1"))
	float BoundaryFullStrengthDepth = 20000.f;

	/** Max rate the nose is turned back toward the centre (deg/s, at full strength). Above any ship's turn rate
	 *  (incl. drift) so the pilot can't out-steer it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boundary", meta = (ClampMin = "0"))
	float BoundarySteerRate = 220.f;

	/** Inward push acceleration at full strength (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boundary", meta = (ClampMin = "0"))
	float BoundaryPushAcceleration = 8000.f;

	// ---- Integration ---------------------------------------------------------------------------

	/** Longest single simulation step. Frames are split into equal sub-steps no longer than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Integration", meta = (ClampMin = "0.001", ClampMax = "0.05"))
	float MaxSubstepSeconds = 1.f / 120.f;
};

/** One collision the movement component resolved this frame. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDFlightImpact
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flight")
	TWeakObjectPtr<AActor> OtherActor;

	UPROPERTY(BlueprintReadOnly, Category = "Flight")
	FVector ImpactPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Flight")
	FVector ImpactNormal = FVector::ZeroVector;

	/** Speed into the surface at impact (cm/s). */
	UPROPERTY(BlueprintReadOnly, Category = "Flight")
	float ImpactSpeed = 0.f;

	/** 0 = parallel graze, 1 = straight into the surface. */
	UPROPERTY(BlueprintReadOnly, Category = "Flight")
	float HeadOn01 = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Flight")
	bool bHeadOn = false;

	/** Damage the flight model wants applied to its own ship (0 while on cooldown). */
	UPROPERTY(BlueprintReadOnly, Category = "Flight")
	float Damage = 0.f;
};

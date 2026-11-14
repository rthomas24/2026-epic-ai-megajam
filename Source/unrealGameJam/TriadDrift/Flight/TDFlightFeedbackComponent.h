// Triad Drift - flight feel that reads with NO art (ENG-01 / ENG-15 / ENG-20). Owner: Teammate 1 (Flight & Input).
//
// Every flight and travel moment has a presentation cue slot (UTDShipVisuals / UTDTravelFX) that Nick's side fills.
// Until a slot is filled the player would see and feel nothing, so this component adds cheap placeholders:
//   - camera kicks (roll / pitch / FOV punch / procedural shake), only while the cue has no Shake
//   - short glow bursts (Core placeholder glow, squad colour), only while the cue has no VFX
//   - near-field speed dust that streaks with velocity (sense of speed against a far star field)
// The table of what each moment does lives in docs/eng/flight.md ("Feedback with no art").

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TDFlightFeedbackComponent.generated.h"

class ATDShip;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
struct FTDFXCue;
enum class ETDHyperdriveState : uint8;
enum class ETDLandingState : uint8;

/** Tuning for the placeholder feedback. Values are small on purpose: readable, never nauseating. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDFlightFeedbackTuning
{
	GENERATED_BODY()

	/** Master scale for every placeholder camera kick and shake (0 turns them off). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0"))
	float CameraScale = 1.f;

	/** Dodge: camera roll into the move (deg) and FOV punch (deg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	float DodgeRoll = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	float DodgeFOV = 4.f;

	/** Seconds a dodge or drift burst glow stays visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0.05"))
	float BurstSeconds = 0.35f;

	/** Glow brightness of the bursts (emissive multiplier of the squad colour). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0"))
	float BurstGlow = 12.f;

	// ---- Speed dust ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust")
	bool bSpeedDust = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "0", ClampMax = "1000"))
	int32 DustCount = 120;

	/** Half size of the box of dust that travels with the ship (cm). Motes wrap around inside it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "500"))
	float DustHalfExtent = 4500.f;

	/** The box is centred this far ahead of the ship, so most motes are in front of the camera (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust")
	float DustAhead = 2500.f;

	/** Streak length = speed * this (s), clamped to DustStreakMax (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "0"))
	float DustStreakSeconds = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "1"))
	float DustStreakMax = 2200.f;

	/** Mote thickness (cm) at DustFullSpeed and above; motes shrink to nothing at DustMinSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "0"))
	float DustThickness = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "0"))
	float DustMinSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "1"))
	float DustFullSpeed = 20000.f;

	/** Motes closer than this to the camera shrink away, so none smears across the view (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "0"))
	float DustCameraClear = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust", meta = (ClampMin = "0"))
	float DustGlow = 1.2f;

	// ---- Engine trail placeholder (every squad ship, while UTDShipVisuals::EngineTrail has no VFX) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
	bool bEngineTrail = true;

	/** Glow segments in the ribbon, each one sample of the exhaust's recent path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "2", ClampMax = "32"))
	int32 TrailSegments = 10;

	/** Seconds between path samples: the ribbon is TrailSegments x this long in time (1.5 s = 90 m at cruise, longer at boost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0.005"))
	float TrailSampleSeconds = 0.15f;

	/** Thickness at the nozzle (cm, no wider than a nozzle); it tapers (quadratically) to nothing at the tail. Boost: +25%. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "1"))
	float TrailThickness = 18.f;

	/** Emissive strength of the ribbon: kept low so it reads as a thin light streak, not a solid bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0"))
	float TrailGlow = 1.6f;

	/** The flown ship's flame thickness at the nozzle (cm) and its glow (a short hot flame). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "1"))
	float PlayerFlameThickness = 22.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0"))
	float PlayerFlameGlow = 1.2f;

	/** The flown ship's own ribbon in the chase view stops this far behind the nozzle (cm); wingmen keep the full length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0"))
	float PlayerTrailMax = 300.f;

	/** ...and never reaches lower on screen than this fraction of the viewport height (0 top, 1 bottom), so the
	 *  flame stays clear of the HUD band whatever the ship's size, nozzle layout or camera distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0.5", ClampMax = "1"))
	float PlayerFlameScreenLimit = 0.70f;

	/** Ships with more than two engines (the Bastion's four) get this share of the flown flame length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0.1", ClampMax = "1"))
	float PlayerFlameManyEnginesScale = 0.6f;

	/** Wingman and other non-flown ribbons fade to nothing within this distance of the camera (cm)... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0"))
	float TrailCameraFadeStart = 1500.f;

	/** ...and are fully drawn again from this distance (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0"))
	float TrailCameraFadeEnd = 2500.f;

	/** Non-flown ribbons fade out over the screen band from this height (0 top, 1 bottom)... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0", ClampMax = "1"))
	float TrailHudBandFadeStart = 0.66f;

	/** ...and are gone below this one (the HUD band), so a wingman passing close never paints across the HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0", ClampMax = "1"))
	float TrailHudBandLimit = 0.72f;

	/** Taking damage on the flown ship: procedural shake (deg) at a full-strength hit (HitFullDamage or more), 0.3 s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float HitShake = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1"))
	float HitFullDamage = 25.f;

	/** A ship destroyed near the flown ship: shake (deg) right next to it, fading to 0 at ExplosionShakeRadius (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float ExplosionShake = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float ExplosionShakeRadius = 9000.f;

	// ---- Audition profile (issue #23, opt-in: TD.Flight.CamProfile Audition). Nick's UI proposal as the start.
	/** Extra camera roll into a turn at full yaw rate (deg), and pitch at full pitch rate (deg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionTurnRoll = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionTurnPitch = 0.6f;
	/** How fast the tilt follows and settles back when steering stops (1/s). No idle bob. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionSettleRate = 5.f;
	/** Sustained shake (deg): cruising, boosting, in the hyperdrive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionShakeCruise = 0.04f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionShakeBoost = 0.08f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionShakeHyper = 0.15f;
	/** Motion blur amount boosting / in the hyperdrive (0 cruising), and its max length (% of screen width). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionBlurBoost = 0.04f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionBlurHyper = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Audition") float AuditionBlurMax = 0.35f;
	/** ENG-38 burn feel at full intensity (entry / exit): sustained shake (deg), FOV push (deg), motion blur amount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Descent") float DescentShake = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Descent") float DescentFOV = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Descent") float DescentBlur = 0.35f;
	/** Caps on the composed camera motion (every kick, shake and tilt together), any profile: deg. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Caps") float ComposedRollCap = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Caps") float ComposedPitchCap = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Caps") float ComposedShakeCap = 1.5f;
};

UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDFlightFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDFlightFeedbackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** True when the cue has no VFX, so a placeholder glow should stand in. */
	static bool WantsVisualPlaceholder(const FTDFXCue* Cue);

	/** Visibility (0..1) of a non-flown ribbon segment from its distance to the camera (cm) and its projected
	 *  height on screen (0 top, 1 bottom; any value above 1 = off the bottom or behind the eye; negative = unknown,
	 *  e.g. no viewport). */
	static float TrailFadeAt(const FTDFlightFeedbackTuning& Tuning, float DistToEye, float ScreenY);
	/** True when the cue has no camera shake, so a placeholder camera kick should stand in. */
	static bool WantsCameraPlaceholder(const FTDFXCue* Cue);

	/** Called by ATDShip::TryDodge after the dodge starts (Direction -1 left / +1 right). */
	void PlayDodge(float Direction);

	/**
	 * A one-off camera kick that decays over Seconds: roll and pitch in degrees, FOV in degrees (+ widens),
	 * Shake = procedural jitter amplitude in degrees.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Feedback")
	void AddCameraKick(float Roll, float Pitch, float FOV, float Shake, float Seconds);

	/** Placeholder glow burst on the ship: local offset and scale on the hull pivot (cm, BasicShapes sphere = 100 cm). */
	void SpawnBurst(const FVector& LocalOffset, const FVector& Scale, const FLinearColor& Color, float Seconds);

	// ---- Read by the chase camera (and by tests) ----
	FRotator GetCameraOffset() const { return CameraOffset; }
	float GetFOVOffset() const { return FOVOffset; }

	/**
	 * The player's comfort options (Settings page): every camera shake and kick (these procedural ones and the
	 * presentation cues' camera shakes, through the space controller) is multiplied by UserShakeScale (0 = none);
	 * the speed FOV widening and the FOV punches by UserSpeedFOVScale (0 = a fixed FOV).
	 */
	static float UserShakeScale;
	static float UserSpeedFOVScale;

	/**
	 * Issue #23 camera profiles (TD.Flight.CamProfile). Nicholas chose AUDITION as the player default (Nov 13, from the
	 * comparison video); Default and Tuned stay selectable for comparison. Reduced motion (the Camera shake and Speed
	 * effects sliders) scales Audition exactly as before:
	 *  Default  = the camera as on main before the fixes (FOV punches never reach the camera after BeginPlay, blur as
	 *             set in BeginPlay, no composed caps);
	 *  Tuned    = both fixes: the feedback FOV punches and the live blur reach the chase camera every frame, with the
	 *             composed caps; engineering's kicks and hit / explosion shakes at their tuned values;
	 *  Audition = Tuned + Nicholas's restrained profile (turn tilt that settles, light sustained shake, speed blur).
	 */
	enum class ECamProfile : uint8 { Default, Tuned, Audition };
	/** What a new game starts with (Nicholas's pick, issue #23). */
	static constexpr ECamProfile PlayerDefaultProfile = ECamProfile::Audition;
	static ECamProfile CamProfile;
	/** ENG-38: the burn's 0..1 (UTDDescentSubsystem sets it each frame from the heat curve). */
	static float DescentIntensity;
	static bool IsFixedCameraPath() { return CamProfile != ECamProfile::Default; }
	static const TCHAR* CamProfileName() { return CamProfile == ECamProfile::Audition ? TEXT("Audition") : CamProfile == ECamProfile::Tuned ? TEXT("Tuned") : TEXT("Default"); }

	/** Motion blur the chase camera should use now (Audition: by boost / hyperdrive, scaled by the Speed slider). */
	float GetMotionBlurAmount() const { return BlurAmount; }
	float GetMotionBlurMax() const { return BlurAmount > 0.f ? Tuning.AuditionBlurMax : 0.f; }

	/** Peaks of the composed camera motion since the last reset (deg), for the audition report. */
	struct FComposedPeaks { float Roll = 0.f; float Pitch = 0.f; float Shake = 0.f; float FOV = 0.f; float Blur = 0.f; };
	static FComposedPeaks ComposedPeaks;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Feedback")
	int32 GetActiveBurstCount() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Feedback")
	int32 GetVisibleDustCount() const { return bDustVisible ? DustPositions.Num() : 0; }

	/** Placeholder engine trail: segments drawn now over every engine (0 = hidden), and the first ribbon's length (cm). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Feedback")
	int32 GetVisibleTrailSegments() const { return bTrailVisible ? TrailTransforms.Num() : 0; }
	float GetTrailLength() const { return TrailLength; }

	/** Average game-thread cost of this component's tick over the last ~2 s, in milliseconds. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Feedback")
	float GetAverageTickMs() const { return AverageTickMs; }

	/** Placeholder moments fired so far, by name ("Dodge", "BoostStart", "PortalTransit"...): tests and agent playtests. */
	int32 GetPlaceholderCount(FName Moment) const { return PlaceholderCounts.FindRef(Moment); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Feedback")
	FTDFlightFeedbackTuning Tuning;

private:
	struct FKick
	{
		float Roll = 0.f;
		float Pitch = 0.f;
		float FOV = 0.f;
		float Shake = 0.f;
		float Seconds = 0.3f;
		float Age = 0.f;
	};
	struct FBurst
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		TObjectPtr<UMaterialInstanceDynamic> MID;
		FLinearColor Color = FLinearColor::White;
		FVector Scale = FVector::OneVector;
		float Seconds = 0.3f;
		float Age = 1000.f;
	};

	ATDShip* GetShip() const;
	void Note(FName Moment) { PlaceholderCounts.FindOrAdd(Moment)++; }
	void PollMoments(float DeltaTime);
	void UpdateCamera(float DeltaTime);
	void UpdateBursts(float DeltaTime);
	void UpdateDust(float DeltaTime);
	void EnsureDust();
	void UpdateTrail(float DeltaTime);
	FBurst* AcquireBurst();

	UFUNCTION()
	void HandleDriftReleased(bool bBurst);

	UFUNCTION()
	void HandleTeleported(class ATDSpacecraft* Spacecraft, FVector FromLocation, FVector ToLocation, FRotator NewFacing);

	TArray<FKick> Kicks;
	TArray<FBurst> Bursts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BurstMeshes; // keeps the pooled meshes referenced
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BurstMIDs;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Dust;
	TArray<FVector> DustPositions;
	TArray<FTransform> DustTransforms;
	bool bDustVisible = false;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Trail;
	TArray<TArray<FVector>> TrailHistory; // per engine, newest first; [0] is the live nozzle position
	TArray<FVector> TrailNozzles;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> TrailMID;
	float TrailMIDGlow = -1.f;
	bool bTrailTranslucent = false; // M_TD_TrailGlow present: per-segment fade through instance custom data
	TArray<FTransform> TrailTransforms;
	float TrailClock = 0.f;
	float TrailLength = 0.f;
	bool bTrailVisible = false;

	FRotator CameraOffset = FRotator::ZeroRotator;
	float FOVOffset = 0.f;
	FVector2D AuditionTilt = FVector2D::ZeroVector; // roll, pitch (deg)
	float BlurAmount = 0.f;
	UFUNCTION() void HandleDamaged(class UTDHealthComponent* Health, const struct FTDDamageInfo& Info, float ShieldDamage, float HullDamage);
	UFUNCTION() void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);
	float SustainedFOV = 0.f;
	float SustainedShake = 0.f;
	float ShakeTime = 0.f;

	// Edge detection of the moments that have no event of their own.
	bool bWasBoosting = false;
	bool bWasBraking = false;
	ETDHyperdriveState LastHyperState;
	ETDLandingState LastLandingState;

	TMap<FName, int32> PlaceholderCounts;
	float AverageTickMs = 0.f;
};

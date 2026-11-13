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

	/** Seconds between path samples: the ribbon is TrailSegments x this long in time (0.3 s = 18 m at cruise). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0.005"))
	float TrailSampleSeconds = 0.03f;

	/** Thickness at the nozzle (cm); it tapers to nothing at the tail. Boost makes it 40% thicker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "1"))
	float TrailThickness = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0"))
	float TrailGlow = 5.f;

	/** The flown ship's own ribbon in the chase view stops this far behind the nozzle (cm); wingmen keep the full length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail", meta = (ClampMin = "0"))
	float PlayerTrailMax = 900.f;
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
	TArray<FTransform> TrailTransforms;
	float TrailClock = 0.f;
	float TrailLength = 0.f;
	bool bTrailVisible = false;

	FRotator CameraOffset = FRotator::ZeroRotator;
	float FOVOffset = 0.f;
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

// Triad Drift - Director (Teammate 3). C-03 #2: the director camera that actually cuts.
//
// On an accepted money shot the player's view blends (real time, slow-mo doesn't stretch it) to a
// cinematic camera placed from the FTDDirectorShot hint, holds framed on subject + secondary for the
// shot, and blends back onto the ship's live chase camera (T1's GetChaseCameraTransformNow, so the
// return never pops). The player keeps flying and aiming the whole time: input is never touched.
// Placement is swept against geometry, ships and T4's obstacle registry. Modes (accessibility):
// Full, Reduced (slow-mo + a short FOV punch, no takeover) and Off, from UTDDirectorSettings or the
// td.DirectorCam console variable.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraModifier.h"
#include "Engine/DeveloperSettings.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TDDirectorCamera.generated.h"

class APlayerController;
class ATDShip;

UENUM(BlueprintType)
enum class ETDDirectorCameraMode : uint8
{
	/** No camera effect at all (slow-mo and events still happen). */
	Off,
	/** Slow-mo + a short FOV punch on the player's own camera; the view never leaves the ship. */
	Reduced,
	/** The full director cut. */
	Full
};

UENUM(BlueprintType)
enum class ETDDirectorCameraPhase : uint8
{
	Idle,
	BlendIn,
	Hold,
	BlendOut
};

/** Why a director shot ended early. */
UENUM(BlueprintType)
enum class ETDDirectorCameraCancel : uint8
{
	SceneCut,
	Travel,
	ShipSwitch,
	PlayerDown,
	SubjectGone,
	Disabled,
	ViewTakenOver
};

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDDirectorBlend
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Director", meta = (ClampMin = "0", Units = "s"))
	float BlendIn = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Director", meta = (ClampMin = "0", Units = "s"))
	float BlendOut = 0.2f;
};

/** Project Settings > Game > Triad Drift Director. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Triad Drift Director"))
class UNREALGAMEJAM_API UTDDirectorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTDDirectorSettings();

	static const UTDDirectorSettings* Get() { return GetDefault<UTDDirectorSettings>(); }

	/** Settings mode, unless td.DirectorCam overrides it (0 off, 1 reduced, 2 full, -1 = use this). */
	static ETDDirectorCameraMode GetEffectiveMode();

	/** Dev builds only: td.Director.DebugHoldSeconds extends shots and cards so a slow screenshot tool
	 *  can catch them. Always 0 in Shipping and by default. */
	static float GetDebugHoldSeconds();

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	ETDDirectorCameraMode CameraMode = ETDDirectorCameraMode::Full;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FTDDirectorBlend DefaultBlend;

	/** Per-kind blend overrides (story beats can ease in slower). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	TMap<ETDHighlightKind, FTDDirectorBlend> BlendByKind;

	/** Director camera field of view. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "20", ClampMax = "120"))
	float ShotFOV = 65.f;

	/** The camera never sits closer than this to the player's ship (never shows its inside). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (Units = "cm"))
	float MinDistanceFromPlayerShip = 900.f;

	/** A placement pulled in closer than this to the focus is rejected. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (Units = "cm"))
	float MinDistanceFromFocus = 900.f;

	/** Camera collision radius for the placement sweep. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (Units = "cm"))
	float CameraProbeRadius = 80.f;

	/** Reduced mode: FOV punch size and length (real seconds). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Reduced", meta = (Units = "deg"))
	float FOVPunchDegrees = 9.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Reduced", meta = (Units = "s"))
	float FOVPunchSeconds = 0.35f;

	const FTDDirectorBlend& GetBlend(ETDHighlightKind Kind) const;
};

/** Reduced mode: a quick FOV kick on the player's own view (real time). */
UCLASS()
class UNREALGAMEJAM_API UTDDirectorFOVPunch : public UCameraModifier
{
	GENERATED_BODY()

public:
	void Punch(float Degrees, float RealSeconds);

	/** Current added FOV (0 when idle). */
	float GetCurrentPunch() const;

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
	double StartRealTime = -1000.0;
	float PunchDegrees = 0.f;
	float PunchSeconds = 0.f;
};

/** The view target during a shot. Its pose is driven by UTDDirectorCameraRig. */
UCLASS(NotPlaceable, Transient)
class UNREALGAMEJAM_API ATDDirectorCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:
	ATDDirectorCameraActor(const FObjectInitializer& ObjectInitializer);
};

/**
 * Owns the director camera for one world: placement, blends, hold, return and every cancel path.
 * Ticked by UTDDirectorSubsystem with real (undilated) seconds.
 */
UCLASS()
class UNREALGAMEJAM_API UTDDirectorCameraRig : public UObject
{
	GENERATED_BODY()

public:
	/** Start (or retarget to) a shot. Returns true if the view is (being) taken. */
	bool BeginShot(const FTDDirectorShot& Shot, double ShotEndRealTime);

	/** End the shot immediately and give the view back to the player's pawn (no blend). */
	void Cancel(ETDDirectorCameraCancel Reason);

	/** Real-time update; also checks the cancel conditions. */
	void Tick(double RealNow, float RealDelta);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Director")
	ETDDirectorCameraPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Director")
	bool IsActive() const { return Phase != ETDDirectorCameraPhase::Idle; }

	ATDDirectorCameraActor* GetCameraActor() const { return CameraActor; }
	ETDDirectorCameraCancel GetLastCancel() const { return LastCancel; }
	int32 GetCancelCount() const { return CancelCount; }
	const FVector& GetShotLocation() const { return ShotLocation; }
	const FVector& GetTrackedLocation() const { return TrackedLocation; }
	const FVector& GetLastFocus() const { return LastFocus; }
	UTDDirectorFOVPunch* GetFOVPunch() const { return FOVPunch.Get(); }

	/**
	 * Where to put the camera for a shot framing Focus: Preferred if clear, else the other side,
	 * a quarter turn, or above, pulled in where blocked. False = no safe spot (then no takeover).
	 * Ignore = actors the probe passes through (the shot's subjects). Public for tests.
	 */
	static bool ResolveCameraLocation(const UWorld* World, const FVector& Focus, const FVector& Preferred,
		const TArray<const AActor*>& Ignore, const AActor* PlayerShip, FVector& OutLocation);

	/** Called by the subsystem when a shot is accepted in Reduced mode. */
	void PunchFOV();

	void SetWorld(UWorld* InWorld) { OwnerWorld = InWorld; }

private:
	APlayerController* GetPlayerController() const;
	ATDShip* GetPlayerShip() const;
	FVector CurrentFocus() const;
	void ApplyPose(const FVector& Location, const FRotator& Rotation, float FOV);
	void FinishReturn();

	TWeakObjectPtr<UWorld> OwnerWorld;

	UPROPERTY(Transient)
	TObjectPtr<ATDDirectorCameraActor> CameraActor;

	TWeakObjectPtr<UTDDirectorFOVPunch> FOVPunch;
	TWeakObjectPtr<APlayerController> PC;
	TWeakObjectPtr<APawn> PawnAtStart;
	TWeakObjectPtr<AActor> Subject;
	TWeakObjectPtr<AActor> Secondary;
	bool bSubjectAliveAtStart = false;

	ETDDirectorCameraPhase Phase = ETDDirectorCameraPhase::Idle;
	double PhaseStart = 0.0;
	double HoldEnd = 0.0;
	float BlendIn = 0.12f;
	float BlendOut = 0.2f;

	FVector FromLocation = FVector::ZeroVector;
	FRotator FromRotation = FRotator::ZeroRotator;
	float FromFOV = 90.f;
	FVector ShotLocation = FVector::ZeroVector;
	/** Camera offset from the focus at placement: the camera tracks the action (a fast ram would
	 *  otherwise leave a fixed camera in a second). */
	FVector ShotOffset = FVector::ZeroVector;
	/** Where the camera currently is in its tracking hold (after the per-frame clearance check). */
	FVector TrackedLocation = FVector::ZeroVector;
	FVector TrackedLocationFor(const FVector& Focus) const;
	FVector LastFocus = FVector::ZeroVector;
	/** The live subject died / vanished mid-shot: frame LastFocus until the hold ends. */
	bool bSubjectGone = false;
	float ShotFOV = 65.f;

	ETDDirectorCameraCancel LastCancel = ETDDirectorCameraCancel::SceneCut;
	int32 CancelCount = 0;
};

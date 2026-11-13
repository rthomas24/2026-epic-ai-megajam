// Triad Drift - a squad ship with the chase camera (ENG-01). Owner: Teammate 1 (Flight & Input).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TDShip.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UTDTargetingComponent;
class UTDHyperdriveComponent;
class UTDLandingComponent;
class UTDAtmosphereEntryComponent;
class UTDFlightFeedbackComponent;
class UTDShipVisuals;
class UNiagaraComponent;
class UAudioComponent;
struct FTDFXCue;
struct FTDFlightImpact;

/** Chase-camera feel. Defaults are features.md ENG-01. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDChaseCameraTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ArmLength = 1400.f;

	/** Camera height above the ship's centre line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float Height = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float RotationLag = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float LocationLag = 18.f; // flight-feel pass Nov 13 (was 12): see docs/eng/flight.md

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float MinFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float MaxFOV = 110.f;

	/** How quickly the FOV follows speed (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float FOVResponse = 4.f;

	/** Fraction of the ship's roll (and visual bank) the camera follows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float RollFollow = 0.3f;

	/** Camera leads into turns by up to this many degrees at full turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float LookAheadAngle = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float LookAheadResponse = 5.f;

	/** Share of LookAheadAngle used for pitch. Leading a pull-up drops the ship down the screen into the HUD band
	 *  (the flight-feel pass measured the ship's centre at 72% of screen height in a 120 deg/s loop at full lead). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float LookAheadPitchScale = 0.2f;

	/** While drifting above cruise speed the camera's yaw follows the ship's velocity by this fraction of the slip
	 *  angle, so a sliding spin keeps the ship on screen. The reticle and the shots stay together (both use this
	 *  camera); the nose keeps the rest of the slip, so you still fly one way and shoot another. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float DriftYawFollow = 0.4f;

	/** Ease in at drift start and out at release (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float DriftBlendSeconds = 0.3f;

	/** The drift camera works above this fraction of cruise speed (1 = only above cruise). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float DriftMinSpeedFraction = 1.f;

	/** While drifting (at any speed) the chase camera holds the distance it had when the drift began: a drift sheds
	 *  speed, the lagged camera would close in and the ship would sink into the HUD band. 0 = off, 1 = full hold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float DriftHoldDistance = 1.f;

	/** Max distance the location lag may trail the ship (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float LagMaxDistance = 1000.f;

	/** Above this |forward.Z| the camera holds its reference "up" instead of tracking world up (loops). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float VerticalHoldLimit = 0.95f;

	/** How fast the camera's reference "up" may swing back toward world up (deg/s): smooths loops and dives. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float ReferenceUpRate = 120.f;

	/**
	 * Camera motion blur in space (0..1, engine default 0.5). Off by default: any camera-rotation blur turns the
	 * small bright stars into dashes (0.1 with a 1% cap still smeared in the Lead's playtest, Nov 12); speed
	 * reads through FOV, speed lines and trails instead.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float MotionBlurAmount = 0.f;

	/** Cap on motion-blur length as a percentage of screen width (engine default 5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "100"))
	float MotionBlurMax = 0.f;
};

/** A squad ship (Rift, Lancer, Bastion). Squad stats come from T3's UTDShipLibrary through ApplyFlightTuning. */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDShip : public ATDSpacecraft
{
	GENERATED_BODY()

public:
	ATDShip();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	ETDShipId GetShipId() const { return ShipId; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ship")
	void SetShipId(ETDShipId NewId);

	/** Presentation (Nick, ART-03). With a mesh set it replaces the placeholder hull; otherwise the
	 *  placeholder is tinted in the squad colour. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ship")
	void SetShipVisuals(UTDShipVisuals* InVisuals);

	/**
	 * Frozen audio-parameter contract (docs/eng/flight.md): every frame, both engine loops (UTDShipVisuals::EngineLoop
	 * and the EngineTrail cue's sound) receive these float parameters, for the player's ship and wingmen alike.
	 */
	static const FName AudioParamSpeed01; // 0..1 of top speed (GetSpeed01)
	static const FName AudioParamBoost01; // eased to 1 while boosting
	static const FName AudioParamDrift01; // eased to 1 while drifting

	UAudioComponent* GetEngineLoopAudio() const { return EngineLoopAudio.Get(); }
	UAudioComponent* GetEngineTrailAudio() const { return EngineTrailAudio.Get(); }

	/** Tests only (null otherwise): replaces UGameplayStatics::SpawnSoundAttached for the engine loop (headless runs have no audio device). */
	static TFunction<UAudioComponent*(USoundBase*, USceneComponent*, FName)> TestSpawnEngineLoop;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	UTDShipVisuals* GetShipVisuals() const { return Visuals; }

	/** A modular part built from UTDShipVisuals::Parts, by PartName (engine flares, pods...). Null if absent. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	UStaticMeshComponent* FindVisualPart(FName PartName) const;

	/** Every part component currently built (visual only: no collision, no tick). */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetVisualParts() const { return VisualParts; }

	/**
	 * How many times this ship requested a presentation cue by slot name ("EngineTrail", "BoostStart",
	 * "BoostLoop", "DriftLoop", "DriftRelease", "CollisionScrape"). Requests happen whenever a visuals
	 * asset is assigned, even if the slot is empty (then nothing plays). For tests and debugging.
	 */
	/**
	 * Low-hull alarm (ENG-35): UTDShipVisuals::LowHullLoop runs on the ship the PLAYER flies while its hull is below
	 * this fraction; it stops above it (+ 0.02), on death and when the player switches away. Wingmen never sound it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ship", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHullAlarmFraction = 0.35f;

	int32 GetCueRequestCount(FName Slot) const { const int32* N = CueRequests.Find(Slot); return N ? *N : 0; }

	/** True while a looping cue slot ("EngineTrail", "EngineLoop", "BoostLoop", "DriftLoop") is running on this ship. */
	bool IsCueLoopActive(FName Slot) const { return ActiveLoops.Contains(Slot); }

	/** True while real art (a Mesh or any Part) replaces the placeholder. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	bool HasRealArt() const { return bHasRealArt; }

	virtual void ApplyPlaceholderLook() override;

	/**
	 * Only toggles the chase camera (active + ticking). Never touches velocity, input latching or
	 * possession: the squad manager possesses / unpossesses around this call.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ship")
	void SetPlayerControlled(bool bInPlayerControlled);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	bool IsPlayerFlown() const { return bPlayerControlled; }

	/** World position of the engine exhaust (art socket / Engine part, else the rear of the hull or the placeholder glow). */
	FVector GetEngineExhaustLocation() const;

	/** Every exhaust: the art's Engine_0..Engine_3 sockets when it has them, else the single exhaust above. */
	void GetEngineExhaustLocations(TArray<FVector>& Out) const;

	/**
	 * ENG-37: every engine nozzle the exhaust effect attaches to: the hull's "Engine" and "Engine_0".."Engine_7" /
	 * "Engine_1".."Engine_8" sockets that exist, in that order; else the single engine attach (visual part, socket or
	 * placeholder glow). One EngineTrail effect runs per nozzle; the sound runs once, on the first.
	 */
	void GetEngineNozzles(TArray<TPair<USceneComponent*, FName>>& Out) const;

	/** The nozzle sockets a mesh has, in order: "Engine", then "Engine_0".."Engine_8" that exist (empty when none).
	 *  Shared with T4's enemy engines (ENG-37). */
	static void GetNozzleSockets(const UStaticMeshComponent* Mesh, TArray<FName>& Out);

	/** What the exhaust effects are fed each frame as Niagara user parameters "Throttle" (0..1) and "Boost" (0/1). */
	float GetExhaustThrottle01() const { return ExhaustThrottle01; }
	bool GetExhaustBoost() const { return bExhaustBoost; }
	/** Exhaust effect components running now (0 without art or with no EngineTrail VFX). */
	int32 GetExhaustEffectCount() const;

	/**
	 * Ship-feel A/B (T1's review, Nov 13; OFF by default until Ryan decides): when true, the squad's per-ship handling
	 * weight (FTDShipSpec Feel* fields: acceleration, nose response, drift turn, dodge cooldown) replaces the shared
	 * flight defaults. TD.Flight.ShipFeel 0|1 flips it live on every squad ship.
	 */
	static bool bPerShipFeel;

	/** Sets a Niagara user float only when the system exposes it (a system without the parameter is left alone). */
	static bool SetExhaustParamIfExposed(UNiagaraComponent* FX, FName Param, float Value);

	/** Engines are lit (alive, not parked or hidden, not landed with engines off). */
	bool AreEnginesRunning() const { return ShouldEnginesRun(); }

	/**
	 * True while the player must not switch away from this ship: hyperdrive spooling / jumping /
	 * dropping out, a landing or take-off in progress, landed (on foot), or atmospheric entry.
	 * T3's squad manager refuses manual switches while this is true. OutReason is HUD-ready.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	bool IsSwitchLocked(FText& OutReason) const;

	/**
	 * The player's dodge (Direction -1 left / +1 right). Player-flown only (wingmen and enemies never dodge); refused
	 * during hyperdrive spool / jump, landing, atmosphere entry, a director slow-motion shot, the ram (speed override)
	 * and the cooldown. Requests the "Dodge" cue.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	bool TryDodge(float Direction);

	/** T2's target selection / lock-on for this ship. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	UTDTargetingComponent* GetTargeting() const { return Targeting; }

	/** ENG-15 hyperdrive. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	UTDHyperdriveComponent* GetHyperdrive() const { return Hyperdrive; }

	/** ENG-20 auto-landing / take-off (gear, canopy, Landing01 state). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	UTDLandingComponent* GetLanding() const { return Landing; }

	/** ENG-20 atmospheric entry (T4's planet atmosphere calls BeginEntry). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	UTDAtmosphereEntryComponent* GetAtmosphereEntry() const { return AtmosphereEntry; }

	/** No-art flight feel: placeholder camera kicks, glow bursts and speed dust (docs/eng/flight.md). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	UTDFlightFeedbackComponent* GetFlightFeedback() const { return FlightFeedback; }

	/** Cockpit view (optional camera, ENG-01). The chase camera is the default. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Camera")
	void SetCockpitView(bool bCockpit);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Camera")
	void ToggleCockpitView() { SetCockpitView(!bCockpitView); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	bool IsCockpitView() const { return bCockpitView; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	UCameraComponent* GetCockpitCamera() const { return CockpitCamera; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	USpringArmComponent* GetSpringArm() const { return SpringArm; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	UCameraComponent* GetChaseCamera() const { return ChaseCamera; }

	/** Snaps camera lag and FOV to their targets (switch cuts, teleports). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Camera")
	void SnapCamera();

	/**
	 * Where the chase camera would be this frame with no lag: valid for AI-flown ships whose arm
	 * doesn't tick (T3's switch-cut "to" camera). For the player-flown ship it's the live camera.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	FTransform GetChaseCameraTransformNow() const;

	/** The chase camera FOV this ship would use at its current speed (no smoothing). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	float GetChaseCameraFOVNow() const;

	/** The chase arm's target rotation this frame (before the spring arm's own rotation lag). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	FQuat GetChaseTargetQuat() const { return ChaseTargetQuat; }

	/** Camera roll relative to its reference up (deg): RollFollow x the ship's bank, folded past 90 deg. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	float GetCameraRoll() const { return CameraRoll; }

	/** Extra arm length the drift distance hold adds right now (cm). */
	float GetDriftHoldExtra() const { return DriftHoldExtra; }

	/** How far the drift camera blend is in (0 none .. 1 full DriftYawFollow), before easing. */
	float GetDriftCameraBlend() const { return DriftCamBlend; }

	const FTDChaseCameraTuning& GetCameraTuning() const { return CameraTuning; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	void UpdateChaseCamera(float DeltaSeconds, bool bSnap);

	/** Arm rotation for the current ship pose. RefUp is the camera's reference up (updated in place). */
	FQuat ComputeChaseQuat(FVector& InOutRefUp, bool bSnap, float DeltaSeconds, float& OutRoll) const;

	UFUNCTION()
	void HandleTeleported(ATDSpacecraft* Spacecraft, FVector FromLocation, FVector ToLocation, FRotator NewFacing);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Ship")
	ETDShipId ShipId = ETDShipId::Rift;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Ship")
	TObjectPtr<UTDShipVisuals> Visuals;

	virtual FLinearColor GetPlaceholderColor() const override;

	UFUNCTION() void HandleBoostStarted();
	UFUNCTION() void HandleBoostEnded();
	UFUNCTION() void HandleDriftStarted();
	UFUNCTION() void HandleDriftEnded(bool bBurst);
	UFUNCTION() void HandleFlightImpactCue(const FTDFlightImpact& Impact);
	UFUNCTION() void HandleShieldBrokenCue(UTDHealthComponent* BrokenHealth, const FTDDamageInfo& Info);
	void UpdateLowHullAlarm();

	/** Where engine cues attach: a part named "Engine", else the primary mesh's "Engine" socket, else the hull. */
	USceneComponent* GetEngineAttach(FName& OutSocket) const;
	void StartEngineTrail();
	void StartLoop(FName Slot, const FTDFXCue& Cue, TWeakObjectPtr<UNiagaraComponent>& OutFX, TWeakObjectPtr<UAudioComponent>& OutAudio);
	void StopLoop(FName Slot, TWeakObjectPtr<UNiagaraComponent>& FX, TWeakObjectPtr<UAudioComponent>& Audio);
	void StopEngine();
	/** Engines run while the ship is alive, not parked, and (during landing) the engines are on. */
	bool ShouldEnginesRun() const;
	/** Shakes only for the ship the player flies: a wingman's boost must not shake the player's camera. */
	FTDFXCue ForThisShip(const FTDFXCue& Cue) const;
	TSet<FName> ActiveLoops;
	TWeakObjectPtr<UAudioComponent> EngineLoopAudio;
	/** Eased 0..1 boost / drift for the engine audio parameters. */
	float AudioBoost01 = 0.f;
	float AudioDrift01 = 0.f;
	/** Sends Speed01 / Boost01 / Drift01 (and, for non-MetaSound sounds, the pitch) to one engine loop. */
	void ApplyEngineAudioParams(UAudioComponent* Audio) const;
	double LastScrapeCueTime = -1000.0;

	/** Minimum time between two scrape cues (a long scrape is one sound, not a machine gun). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Presentation", meta = (ClampMin = "0"))
	float ScrapeCueInterval = 0.25f;
	void NoteCue(FName Slot) { CueRequests.FindOrAdd(Slot)++; }

	TMap<FName, int32> CueRequests;
	TWeakObjectPtr<UNiagaraComponent> EngineTrailFX, BoostLoopFX, DriftLoopFX;
	TArray<TWeakObjectPtr<UNiagaraComponent>> ExtraExhaustFX; // nozzles 2..N (VFX only)
	float ExhaustThrottle01 = 0.f;
	bool bExhaustBoost = false;
	void UpdateExhaustParams();
	TWeakObjectPtr<UAudioComponent> EngineTrailAudio, BoostLoopAudio, DriftLoopAudio, LowHullAudio;
	TWeakObjectPtr<UNiagaraComponent> LowHullFX;

	/** Tears down parts, restores placeholders, then builds Mesh / Parts from Visuals. */
	void RebuildVisuals();
	void ClearVisualParts();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> VisualParts;

	TArray<FName> VisualPartNames;
	bool bHasRealArt = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera")
	FTDChaseCameraTuning CameraTuning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera")
	TObjectPtr<UCameraComponent> ChaseCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Ship")
	TObjectPtr<UTDTargetingComponent> Targeting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Ship")
	TObjectPtr<UTDHyperdriveComponent> Hyperdrive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Ship")
	TObjectPtr<UTDLandingComponent> Landing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Ship")
	TObjectPtr<UTDAtmosphereEntryComponent> AtmosphereEntry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Ship")
	TObjectPtr<UTDFlightFeedbackComponent> FlightFeedback;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera")
	TObjectPtr<UCameraComponent> CockpitCamera;

	/** Cockpit eye point relative to the hull pivot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera")
	FVector CockpitOffset = FVector(140.f, 0.f, 70.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float CockpitFOV = 95.f;

private:
	bool bPlayerControlled = false;
	bool bCockpitView = false;
	void ApplyCameraActivation();
	float CurrentFOV = 90.f;
	int32 SnapFramesRemaining = 0;
	FVector CameraRefUp = FVector::ZeroVector;
	FQuat ChaseTargetQuat = FQuat::Identity;
	float CameraRoll = 0.f;
	float DriftCamBlend = 0.f;
	float DriftHoldBlend = 0.f;
	float DriftHoldStartDist = 0.f;
	float DriftHoldExtra = 0.f;
	bool bDriftHoldWasDrifting = false;
	FVector2D LookAhead = FVector2D::ZeroVector;
};

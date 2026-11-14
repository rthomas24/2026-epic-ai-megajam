// Triad Drift - planet objective props and hazards (ENG-25 Verdance, ENG-26 Cinder). Owner: Teammate 5.
//
// Zero-asset gameplay pieces the planet courses are built from. Each one resets itself on a "CUT!"
// (UTDGroundSceneSubsystem::OnResetUnfinished) unless its objective is already complete.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TDPlanetProps.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTDHealthComponent;
class UMaterialInstanceDynamic;
class ATDObjective;
class ATDFirstPersonCharacter;
class ATDEncounterVolume;

/** Base: registers for the ground reset and knows whether its objective is done. */
UCLASS(Abstract)
class UNREALGAMEJAM_API ATDPlanetProp : public AActor
{
	GENERATED_BODY()

public:
	ATDPlanetProp();

	/** The objective this prop belongs to (resets are skipped once it is complete). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	TObjectPtr<ATDObjective> LinkedObjective;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	int32 LinkedStep = INDEX_NONE;

	/** Back to the starting state (called on a retake while the objective is incomplete). */
	virtual void ResetProp() {}

	/** The objective was done before this level loaded: show the done state (crystals gone, ...). */
	virtual void ApplyCompleted() {}

	bool IsLinkedStepCurrent() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	static ATDFirstPersonCharacter* AsPlayer(AActor* Actor);

private:
	void HandleGroundReset();
	void HandleCompletedOnLoad();
	FDelegateHandle ResetHandle;
	FDelegateHandle CompletedHandle;
};

// ================================================================================================
/** Walk into it during LinkedStep and the step completes ("follow the beacon", "bring the core back"). */
UCLASS()
class UNREALGAMEJAM_API ATDStepTrigger : public ATDPlanetProp
{
	GENERATED_BODY()

public:
	ATDStepTrigger();

	/** Only counts while the player carries the objective item; it is delivered (dropped) here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	bool bRequireCarrying = false;

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	TObjectPtr<UBoxComponent> Box;

	/** A tall glowing column so the destination reads from far away (hidden when not the current step). */
	UPROPERTY(VisibleAnywhere, Category = "Planet")
	TObjectPtr<UStaticMeshComponent> Beacon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art") TSoftObjectPtr<UStaticMesh> BeaconMesh;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	bool TryComplete(AActor* Other);

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
};

// ================================================================================================
/**
 * Counts things toward one objective step: interactables used, actors destroyed. Shows "TEXT n / N" on the
 * objective line and completes the step at Required.
 */
UCLASS()
class UNREALGAMEJAM_API ATDStepCounter : public ATDPlanetProp
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	int32 Required = 3;

	/** "WAKE THE RESONANCE NODES" (the count is appended). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FText BaseText;

	/** Count this interactable's use (it must not complete the step itself). */
	void WatchInteractable(ATDInteractableActor* Interactable);

	/** Count this actor's death (its UTDHealthComponent). */
	void WatchDestructible(AActor* Actor);

	int32 GetCount() const { return Count; }
	void Add();

	virtual void ResetProp() override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION() void HandleInteracted(AActor* Interactable, AActor* Interactor);
	UFUNCTION() void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);
	UFUNCTION() void HandleStepStarted(ATDObjective* Objective, int32 StepIndex);
	void Announce() const;

	int32 Count = 0;
};

// ================================================================================================
/**
 * The wreck core: hold to extract (survives damage), during which two waves drop in; then the player
 * carries it (no sprint, block or attacks) until a bRequireCarrying trigger takes it. Dropped on death.
 */
UCLASS()
class UNREALGAMEJAM_API ATDExtractionSite : public ATDInteractableActor
{
	GENERATED_BODY()

public:
	ATDExtractionSite();

	/** Drops in when the extraction step starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	TObjectPtr<ATDEncounterVolume> WaveA;

	/** Drops in once the player starts holding (or WaveBDelay after the step starts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	TObjectPtr<ATDEncounterVolume> WaveB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (Units = "s"))
	float WaveBDelay = 8.f;

	/** The core the player carries (attached in front of the camera while carrying). */
	UPROPERTY(VisibleAnywhere, Category = "Planet")
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art") TSoftObjectPtr<UStaticMesh> CoreMesh;

	virtual void Interact(AActor* Interactor) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Delivered at the trigger: the core is gone, the carrier is free. */
	void Deliver();
	void ResetSite();
	bool IsCarried() const { return Carrier.IsValid(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION() void HandleStepStarted(ATDObjective* Objective, int32 StepIndex);
	void DropCore();

	TWeakObjectPtr<ATDFirstPersonCharacter> Carrier;
	FTransform CoreHome;
	float StepElapsed = -1.f;
	bool bWaveBDropped = false;
	FDelegateHandle ResetHandle;
};

// ================================================================================================
/** Magma: heavy damage, then the player is put back on the last safe ground. Enemies just burn. */
UCLASS()
class UNREALGAMEJAM_API ATDMagmaVolume : public ATDPlanetProp
{
	GENERATED_BODY()

public:
	ATDMagmaVolume();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	float Damage = 35.f;

	UPROPERTY(VisibleAnywhere, Category = "Hazard")
	TObjectPtr<UStaticMeshComponent> Surface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art") TSoftObjectPtr<UStaticMesh> SurfaceMesh;

	UPROPERTY(VisibleAnywhere, Category = "Hazard")
	TObjectPtr<UBoxComponent> Box;

	/** Size in cm (the surface sits at the top of the box). */
	void SetSize(const FVector& Size);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
	float Pulse = 0.f;
};

// ================================================================================================
/** A geyser: dormant, then a 1 s rumble (glow), then a 1.5 s eruption that launches and burns. */
UCLASS()
class UNREALGAMEJAM_API ATDGeyser : public ATDPlanetProp
{
	GENERATED_BODY()

public:
	ATDGeyser();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (Units = "s")) float DormantSeconds = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (Units = "s")) float WarnSeconds = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (Units = "s")) float EruptSeconds = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard") float Damage = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard") float LaunchSpeed = 1400.f;
	/** Start offset into the cycle, so a row of geysers ripples. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (Units = "s")) float Phase = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Hazard") TObjectPtr<UStaticMeshComponent> Vent;
	UPROPERTY(VisibleAnywhere, Category = "Hazard") TObjectPtr<UStaticMeshComponent> Plume;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art") TSoftObjectPtr<UStaticMesh> VentMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art") TSoftObjectPtr<UStaticMesh> PlumeMesh;
	UPROPERTY(VisibleAnywhere, Category = "Hazard") TObjectPtr<UBoxComponent> Box;

	bool IsErupting() const;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> VentMID;
	float Clock = 0.f;
	TSet<TWeakObjectPtr<AActor>> HitThisEruption;
};

// ================================================================================================
/** A crystal that shatters when slashed (Neutral team, any damage). Can block a path or be a target. */
UCLASS()
class UNREALGAMEJAM_API ATDShatterCrystal : public ATDPlanetProp
{
	GENERATED_BODY()

public:
	ATDShatterCrystal();

	UPROPERTY(VisibleAnywhere, Category = "Planet") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art") TSoftObjectPtr<UStaticMesh> CrystalMesh;
	UPROPERTY(VisibleAnywhere, Category = "Planet") TObjectPtr<UTDHealthComponent> Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") FLinearColor Color = FLinearColor(1.f, 0.45f, 0.1f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") float MaxHealth = 30.f;

	bool IsShattered() const { return bShattered; }
	virtual void ResetProp() override;
	virtual void ApplyCompleted() override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION() void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);
	bool bShattered = false;
};

// ================================================================================================
/** A crystal bridge segment: stand on it and it cracks, then falls after FallDelay. Back on a retake. */
UCLASS()
class UNREALGAMEJAM_API ATDCollapsingPlatform : public ATDPlanetProp
{
	GENERATED_BODY()

public:
	ATDCollapsingPlatform();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (Units = "s")) float FallDelay = 1.1f;
	UPROPERTY(VisibleAnywhere, Category = "Planet") TObjectPtr<UStaticMeshComponent> Slab;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art") TSoftObjectPtr<UStaticMesh> SlabMesh;

	void SetSize(const FVector& Size);
	bool HasFallen() const { return bFallen; }
	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetProp() override;

protected:
	virtual void BeginPlay() override;

private:
	FVector HomeRelative = FVector::ZeroVector;
	float StandTime = -1.f;
	float FallSpeed = 0.f;
	bool bFallen = false;
};

// ================================================================================================
/**
 * The jammer tower coming down: when LinkedStep starts, a countdown runs and debris falls around the route;
 * reaching the exit trigger in time completes the step; running out of time kills the player (CUT!).
 */
UCLASS()
class UNREALGAMEJAM_API ATDCollapseRun : public ATDPlanetProp
{
	GENERATED_BODY()

public:
	/** Art for the falling debris (fitted to each 100-220 cm rock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art") TSoftObjectPtr<UStaticMesh> DebrisMesh;

	ATDCollapseRun();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (Units = "s")) float Seconds = 25.f;
	/** Debris lands at random points inside this box (relative to the actor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") FVector DebrisExtent = FVector(4000.f, 1500.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TObjectPtr<AActor> Tower;
	/** The objective line while it runs; {0} = whole seconds left. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") FText RunText;

	bool IsRunning() const { return TimeLeft >= 0.f; }
	float GetTimeLeft() const { return TimeLeft; }
	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetProp() override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION() void HandleStepStarted(ATDObjective* Objective, int32 StepIndex);
	void DropDebris();

	float TimeLeft = -1.f;
	float DebrisTimer = 0.f;
	int32 LastShownSecond = -1;
	FVector TowerHome = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AActor>> Debris;
};

// ================================================================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnArchiveTransfer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnArchiveTransferPaused, bool, bPaused);
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnSurveyTerminalActivated, AActor*);

/**
 * Verdance objective 1, reduced form (storyline.md sections 16 and 18, Ryan Nov 13): the survey archive transfer. It
 * runs over two objective steps (LinkedStep - 1 = the first half, LinkedStep = the second half, a checkpoint at the
 * midpoint), Seconds in all, and pauses while any living crewman stands within PauseRadius of the terminal (no new
 * enemy AI; nothing damages the terminal). Three fixed waves drop in at WaveAt (fractions of the transfer); the third
 * brings the Heavy. T2 shows the meter through GetArchiveTransfer; T3 hangs Cinematic 03 on OnAnyTerminalActivated.
 */
UCLASS()
class UNREALGAMEJAM_API ATDArchiveTransfer : public ATDPlanetProp
{
	GENERATED_BODY()

public:
	ATDArchiveTransfer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (Units = "s")) float Seconds = 75.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (Units = "cm")) float PauseRadius = 500.f;
	/** The waves and the transfer fraction each drops in at (same order). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TArray<TObjectPtr<ATDEncounterVolume>> Waves;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TArray<float> WaveAt;
	/** T3's live walk (Nov 13): Cinematic 03 talks for ~45 s over the fight. The first half waits (no waves, no progress)
	 *  until this line starts or has already played this run, the scene isn't playing (skipped, or none), or HoldMaxSeconds
	 *  pass. None = no hold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") FName HoldUntilLine = TEXT("C3_Moss_Company");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (Units = "s")) float HoldMaxSeconds = 60.f;
	/** Held for the cinematic (the meter shows, nothing moves). */
	bool IsHolding() const { return bHolding; }
	/** The objective line; {0} = the whole percent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") FText RunningText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") FText PausedText;

	UPROPERTY(BlueprintAssignable, Category = "Planet") FTDOnArchiveTransfer OnTransferStarted;
	UPROPERTY(BlueprintAssignable, Category = "Planet") FTDOnArchiveTransferPaused OnTransferPaused;
	UPROPERTY(BlueprintAssignable, Category = "Planet") FTDOnArchiveTransfer OnTransferCompleted;
	/** The survey terminal was activated (the transfer's first half starts): Cinematic 03's hook (T3). */
	static FTDOnSurveyTerminalActivated OnAnyTerminalActivated;

	/** T2's meter: false when no transfer is running in this world. Label = the objective line shown. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet", meta = (WorldContext = "WorldContextObject"))
	static bool GetArchiveTransfer(const UObject* WorldContextObject, float& OutProgress01, bool& bOutPaused, FText& OutLabel);

	float GetProgress01() const { return Progress; }
	bool IsPaused() const { return bPaused; }
	bool IsRunning() const { return bRunning; }
	/** A crewman within PauseRadius of the terminal (2D), alive. */
	bool IsHostileAtTerminal() const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetProp() override;
	virtual void ApplyCompleted() override { Progress = 1.f; bRunning = false; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION() void HandleStepStarted(ATDObjective* Objective, int32 StepIndex);
	void UpdateLine(bool bForce);
	float Progress = 0.f;
	bool bPaused = false;
	bool bRunning = false;
	bool bHolding = false;
	float HoldElapsed = 0.f;
	bool ShouldRelease() const;
	int32 ShownPercent = -1;
	bool bShownPaused = false;
	TArray<bool> Fired;
	FText Label;
};

// ================================================================================================
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnSurveyRecordsAccessed, AActor*);

/**
 * Cinder's survey terminal (storyline.md sections 22 and 25, reduced form): usable only out of combat (no living crewman
 * within ClearRadius), it completes its step and fires OnAnySurveyRecordsAccessed, T3's hook for Cinematic 05.
 */
UCLASS()
class UNREALGAMEJAM_API ATDSurveyTerminal : public ATDInteractableActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact", meta = (Units = "cm")) float ClearRadius = 1500.f;
	/** False for story consoles that only share the out-of-combat rule (the Dominion arena's recordings). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") bool bAnnounceRecords = true;

	/** The survey records were accessed (T3: Cinematic 05 at the terminal). */
	static FTDOnSurveyRecordsAccessed OnAnySurveyRecordsAccessed;

	/** A living crewman within ClearRadius: the terminal waits (the patrol must be dead or left behind). */
	bool IsHostileNear() const;

	virtual bool CanInteract(AActor* Interactor) const override;
	virtual void Interact(AActor* Interactor) override;
};

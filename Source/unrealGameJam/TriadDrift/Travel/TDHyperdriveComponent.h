// Triad Drift - hyperdrive (ENG-15). Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDHyperdriveComponent.generated.h"

class ATDSpacecraft;
class UTDHealthComponent;
class UTDTravelFX;
class UNiagaraComponent;
struct FTDFlightImpact;
struct FTDDamageInfo;

/** Why the hyperdrive can't spool right now (HUD tip / sound). */
UENUM(BlueprintType)
enum class ETDHyperdriveBlock : uint8
{
	None,
	RecentDamage,
	InAtmosphere,
	NearGravityWell,
	Cooldown,
	Disabled,
	NotPlayerFlown, // wingmen / AI never jump on their own (T3 follows the leader instead)
	ObstacleAhead,  // something solid within ObstacleClearanceToSpool along the nose
	Locked          // an active wave / finale / cutscene holds a UTDTravelSubsystem hyperdrive lock
};

/** Why the last jump ended. */
UENUM(BlueprintType)
enum class ETDHyperdriveEndCause : uint8
{
	None,
	GravityWell,
	Boundary,
	Obstacle,  // look-ahead saw something solid: dropped to cruise short of it
	Impact,
	Timeout,
	Manual,
	Locked,
	PilotLoss,
	Arrived    // a destination jump reached its drop-out point (the marker's stand-off)
};

/** Defaults are features.md ENG-15. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDHyperdriveTuning
{
	GENERATED_BODY()

	/** Hold time before the jump. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0.05"))
	float SpoolSeconds = 1.2f;

	/** Can't spool within this long of taking damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float DamageLockoutSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float JumpSpeed = 80000.f;

	/** Turn/roll rate multiplier while jumping (0.1 = 10% steering). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0", ClampMax = "1"))
	float JumpSteeringScale = 0.1f;

	/** Acceleration into the jump (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float JumpAcceleration = 120000.f;

	/** A manual drop-out is only accepted after this long in the jump. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float MinJumpSeconds = 0.5f;

	/** The jump ends by itself after this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0.1"))
	float MaxJumpSeconds = 12.f;

	/** Speed burst carried out of the jump (cm/s), held for DropOutSeconds then bled off by the flight model. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float DropOutSpeed = 15000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float DropOutSeconds = 0.6f;

	/** How hard the ship brakes from jump speed to DropOutSpeed (cm/s^2): a snap, not a coast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float DropOutDeceleration = 400000.f;

	/** Can't spool again for this long after a drop-out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float CooldownSeconds = 1.f;

	/** During a jump, the path this far ahead is swept every tick; anything solid ends the jump short of it (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float LookAheadDistance = 30000.f;

	/** A spool is refused when something solid is closer than this along the nose (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "0"))
	float ObstacleClearanceToSpool = 20000.f;

	/** Look-ahead sweep radius as a multiple of the ship's collision radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive", meta = (ClampMin = "1"))
	float LookAheadRadiusScale = 1.5f;

	/** Drop out automatically on reaching the play-space boundary's warning band. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive")
	bool bDropOutAtBoundary = true;

	// ---- One jump to a marked destination (Ryan, Nov 13: "gets you to a place really quickly")
	/** A gravity well (planet, Dominion) or the objective marker within this cone of the nose is the jump's destination. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive|Destination", meta = (ClampMin = "0", ClampMax = "60"))
	float DestinationConeDeg = 20.f;
	/** The tunnel to a destination lasts at most this long: the jump speed rises with the distance (never below JumpSpeed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive|Destination", meta = (ClampMin = "0.5"))
	float DestinationMaxTunnelSeconds = 3.5f;
	/** A jump to the objective marker drops out this far short of it (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive|Destination", meta = (ClampMin = "0"))
	float MarkerStandOff = 150000.f;
	/** After a destination jump the drop-out burst lasts this long (the boost out the far side). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hyperdrive|Destination", meta = (ClampMin = "0"))
	float DestinationBoostOutSeconds = 1.5f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnHyperdriveStateChanged, UTDHyperdriveComponent*, Hyperdrive, ETDHyperdriveState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDOnHyperdriveRefused, UTDHyperdriveComponent*, Hyperdrive, ETDHyperdriveBlock, Reason, FText, ReasonText);

/**
 * Hold to spool (1.2 s), jump at 80,000 cm/s with 10% steering (ATDSpacecraft::SetSpeedOverride),
 * drop out automatically inside a gravity well (UTDTravelSubsystem), at the boundary, on impact, after
 * MaxJumpSeconds or on request; the drop-out carries a short speed burst. Damage within 2 s blocks the
 * spool, and a hit cancels it. The player-flown ship's state also goes out on UTDEventBus::OnHyperdriveState.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDHyperdriveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDHyperdriveComponent();

	/** Begin spooling (input pressed). Returns false and leaves the state alone if blocked. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Hyperdrive")
	bool StartSpool();

	/** Input released: cancels a spool that hasn't finished. Does nothing once jumping. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Hyperdrive")
	void StopSpool();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Hyperdrive")
	void CancelSpool();

	/** Leave the jump now (pilot pressed hyperdrive again, or a scripted stop). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Hyperdrive")
	bool RequestDropOut();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Hyperdrive")
	void SetHyperdriveEnabled(bool bEnabled);

	/**
	 * The pilot is leaving this ship (switch, death): cancel a spool, or end a jump / drop-out at once
	 * (override cleared, flight model bleeds the speed off), and broadcast Idle while the ship still
	 * counts as player-flown so wingmen, HUD and FX listeners see the end. ATDShip calls this.
	 */
	void EndForPilotLoss();

	/**
	 * Hard reset for a retake (Director's Cut "CUT!"): instantly Idle, charge 0, speed override cleared,
	 * no drop-out burst or FX, damage lockout and cooldown cleared, enabled. Broadcasts Idle if it changed.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Hyperdrive")
	void ResetHyperdrive();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hyperdrive")
	ETDHyperdriveState GetState() const { return State; }

	/** HUD charge: spool progress 0..1, 1 while jumping, falls back to 0 through the drop-out. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hyperdrive")
	float GetCharge01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hyperdrive")
	ETDHyperdriveBlock GetBlockReason() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hyperdrive")
	bool CanSpool() const { return GetBlockReason() == ETDHyperdriveBlock::None; }

	/**
	 * Player-facing text for the current block (HUD), e.g. "HYPERDRIVE LOCKED - ENEMIES IN RANGE". Empty for None.
	 * Cheap enough to call every frame: the obstacle sweep behind it is refreshed every HUDSweepInterval seconds
	 * (gameplay checks such as a pilot's press always sweep live).
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hyperdrive")
	FText GetBlockReasonText() const;

	/** How often the HUD text refreshes the obstacle sweep (s). */
	static constexpr double HUDSweepInterval = 0.2;

	/** Standing reasons (in atmosphere, landing / offline) appear in GetBlockReasonText this long after a refused press. */
	static constexpr double StandingReasonShowSeconds = 2.0;

	/** In atmosphere / disabled / not player-flown: true for reasons the player can't act on right now. */
	static bool IsStandingBlock(ETDHyperdriveBlock Block);

	/** The pilot pressed hyperdrive but it couldn't spool: HUD message + refused sound (Nick). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Hyperdrive")
	FTDOnHyperdriveRefused OnSpoolRefused;

	/** Pilot input (controller): spool, or say why not (fires OnSpoolRefused + the refused cue). */
	bool RequestSpoolFromPilot();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hyperdrive")
	bool IsJumping() const { return State == ETDHyperdriveState::Jumping; }

	/** Id of the gravity well that ended the last jump (NAME_None if it ended another way). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hyperdrive")
	FName GetLastDropOutWell() const { return LastDropOutWell; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hyperdrive")
	ETDHyperdriveEndCause GetLastEndCause() const { return LastEndCause; }

	/** Speed-override owner key used on the movement component. */
	static const FName OverrideOwner;

	/** Distance to the first solid thing along the nose within MaxDistance (squadmates ignored); -1 if clear. */
	float SweepAhead(float MaxDistance) const;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Hyperdrive")
	FTDOnHyperdriveStateChanged OnStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hyperdrive")
	FTDHyperdriveTuning Tuning;

	/** Spool / tunnel / drop-out cues (Nick). Empty slots do nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hyperdrive")
	TObjectPtr<UTDTravelFX> TravelFX;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleDamaged(UTDHealthComponent* HealthComp, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);

	UFUNCTION()
	void HandleImpact(const FTDFlightImpact& Impact);

	UFUNCTION()
	void HandleDeath(UTDHealthComponent* HealthComp, const FTDDamageInfo& KillingBlow);

private:
	ATDSpacecraft* GetCraft() const;
	void SetState(ETDHyperdriveState NewState);
	void BeginJump();
	void BeginDropOut(FName WellId, ETDHyperdriveEndCause Cause, bool bBurst = true);
	void FinishDropOut();
	void StopLoopFX();
	double Now() const;

	ETDHyperdriveState State = ETDHyperdriveState::Idle;
	float Charge = 0.f;
	float StateTime = 0.f;
	double LastDamageTime = -1000.0;
	double LastDropOutTime = -1000.0;
	bool bEnabled = true;
	FName LastDropOutWell = NAME_None;
	ETDHyperdriveEndCause LastEndCause = ETDHyperdriveEndCause::None;
	FVector LastLocation = FVector::ZeroVector;
	TWeakObjectPtr<UNiagaraComponent> LoopFX;
	TWeakObjectPtr<UNiagaraComponent> StreaksFX; // HyperdriveStreaks, for the jump

	// ---- The current jump's destination (none: a free jump, as before).
	bool bHasDestination = false;
	FVector DestinationPoint = FVector::ZeroVector; // the drop-out point (well edge or marker stand-off)
	FName DestinationWell = NAME_None;
	float DestinationJumpSpeed = 0.f;
	float DestinationTunnelSeconds = 0.f;
	/** Chooses this jump's destination from the nose (a well, else the objective marker); false = a free jump. */
	bool PickDestination(const class ATDSpacecraft* Craft);
public:
	/** The jump in progress (or the last) had a destination: tests, HUD. */
	bool HasDestination() const { return bHasDestination; }
	float GetDestinationJumpSpeed() const { return DestinationJumpSpeed; }
	float GetDestinationTunnelSeconds() const { return DestinationTunnelSeconds; }
private:
	TWeakObjectPtr<class UAudioComponent> LoopAudio;
	void StartLoopFX(const struct FTDFXCue& Cue);

	UFUNCTION()
	void HandleTeleported(class ATDSpacecraft* Spacecraft, FVector FromLocation, FVector ToLocation, FRotator NewFacing);

private:
	ETDHyperdriveBlock ComputeBlockReason(bool bCachedSweep) const;
	FText TextForBlock(ETDHyperdriveBlock Block) const;
	mutable double SweepCacheTime = -1.0;
	double LastRefusedTime = -1.0;
	mutable bool bSweepCacheBlocked = false;
};

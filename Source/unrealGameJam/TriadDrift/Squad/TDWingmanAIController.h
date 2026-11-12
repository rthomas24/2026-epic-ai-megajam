// Triad Drift - Squad (Teammate 3). ENG-08: wingman AI (PROTECT).

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDWingmanAIController.generated.h"

class ATDShip;
class ATDSquadManager;

UENUM(BlueprintType)
enum class ETDWingmanState : uint8
{
	/** Hold a loose V slot on the player's ship; boost to catch up when far behind. */
	Formation,
	/** Attack a target with primary/secondary weapons only. */
	Engage,
	/** Break off: shield broke or a missile is incoming. */
	Evade,
	/** Return to formation after an evade or when the leader moved away. */
	Regroup,
	/** Ship is down; the controller idles until it respawns. */
	Downed
};

/**
 * Flies an ATDShip like a pilot: it only ever calls ATDSpacecraft::SetFlightInput and
 * UTDWeaponComponent (primary/secondary). It NEVER uses signature powers or abilities.
 * While the player flies a ship, that ship's wingman controller is unpossessed and dormant.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDWingmanAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATDWingmanAIController();

	/** Bind to its squad manager and ship slot. Called by ATDSquadManager after spawning. */
	void InitWingman(ATDSquadManager* InSquad, ETDShipId InShipId);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Wingman")
	ETDWingmanState GetWingmanState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Wingman")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Wingman")
	ETDShipId GetShipId() const { return ShipId; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Wingman")
	ATDShip* GetShip() const;

	/** Prioritise Target (from the squad's focus command) until it dies or the command expires. */
	void SetFocusTarget(AActor* Target);

	/** Squad manager hooks. */
	void NotifyShipDowned();
	void NotifyShipRespawned();

	/** Leader teleported (portal, gate, hyperdrive drop-out): warp-follow if far. */
	void NotifyLeaderTeleported();

	// ---------------- Tuning (features.md ENG-08 defaults) ----------------

	/** Within this distance of the slot we hold formation at matched speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (Units = "cm"))
	float FormationHoldRadius = 600.f;

	/** Beyond this distance to the slot we boost to catch up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (Units = "cm"))
	float CatchUpBoostDistance = 8000.f;

	/** Formation throttle may reach the leader's speed x this (T1 SetThrottleTopSpeed), so slow ships keep up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (ClampMin = "1"))
	float CatchUpSpeedRatio = 1.25f;

	/** cm/s of extra speed per cm the wingman is behind its slot (negative when ahead). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (ClampMin = "0"))
	float FormationSpeedGain = 1.2f;

	/** Beyond this distance after a leader teleport, warp-follow instead of flying. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (Units = "cm"))
	float TeleportFollowDistance = 30000.f;

	/** Look for targets within this range of the leader. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "cm"))
	float EngageRange = 25000.f;

	/** Don't stray further than this from the leader while engaging. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "cm"))
	float LeashDistance = 35000.f;

	/** An enemy within this distance of the player's ship counts as "threatening the player". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "cm"))
	float ThreatRadius = 12000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "deg"))
	float FireConeDeg = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "cm"))
	float PrimaryRange = 20000.f;

	/** Fire the secondary in bursts no more often than this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "s"))
	float SecondaryInterval = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "deg"))
	float AimErrorMinDeg = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "deg"))
	float AimErrorMaxDeg = 4.f;

	/** Re-evaluate the target this often. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "s"))
	float TargetReevaluateInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Evade", meta = (Units = "s"))
	float EvadeSeconds = 1.8f;

	/** Health (hull+shield) fraction below which a hurt wingman drifts toward an active shield dome. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Evade")
	float HurtThreshold01 = 0.4f;

	/** Look-ahead time for obstacle avoidance sweeps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (Units = "s"))
	float AvoidanceLookAhead = 1.2f;

	/** Keep at least this far from squadmates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (Units = "cm"))
	float TeammateSeparation = 900.f;

	/** Steering gain: degrees of error that map to full stick deflection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Steering", meta = (Units = "deg"))
	float FullStickAngleDeg = 25.f;

	/** Seconds before a wingman may evade again (no endless evading under fire). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Evade", meta = (Units = "s"))
	float EvadeCooldown = 3.f;

	/** Seconds the secondary trigger is held per shot (Splitter Bolt needs >= 0.8 s charge). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "s"))
	float SecondaryHoldSeconds = 1.f;

	/** Closer than this to the target, break off and overshoot instead of ramming it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "cm"))
	float BreakOffDistance = 1800.f;

	/**
	 * Leader faster than this = hyperdrive (ENG-15 jumps at 80 000 cm/s): ride along in the slot.
	 * Must stay above every non-jump speed, including Lancer's ram (3x boost = 45 000 cm/s).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (Units = "cm/s"))
	float HyperdriveFollowMinSpeed = 60000.f;

	/** After a leash break or an evade, wait this long before picking a new fight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "s"))
	float RegroupCommitSeconds = 2.f;

	/** Bus handler: a spacecraft was moved discontinuously (portal, gate, hyperdrive drop-out). */
	UFUNCTION()
	void HandleSpacecraftTeleported(AActor* Spacecraft, FVector From, FVector To);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetState(ETDWingmanState NewState);
	void UpdateFormation(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader);
	void UpdateEngage(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader);
	void UpdateEvade(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader);

	/** Fly to a moving point (formation slot or dome), matching the reference velocity when close. */
	void FlyToPoint(ATDShip* Ship, const FVector& Point, const FVector& RefVelocity, const FVector& RefForward);

	AActor* PickTarget(ATDShip* Ship, ATDShip* Leader) const;

	/** Formation slot, or the shield dome when hurt and a dome is up. */
	FVector GetHomeLocation(ATDShip* Ship, ATDShip* Leader, bool& bOutIsDome) const;

	/** Turn the desired world direction + speed into FTDFlightInput and send it to the ship. */
	void SteerToward(ATDShip* Ship, const FVector& DesiredDir, float TargetSpeed, bool bBoost);

	/** Bend DesiredDir away from obstacles and squadmates. */
	FVector ApplyAvoidance(ATDShip* Ship, const FVector& DesiredDir);

	void SetTriggers(bool bPrimary, bool bSecondary);
	void BeginEvade(ATDShip* Ship);
	double Now() const;

	TWeakObjectPtr<ATDSquadManager> Squad;
	ETDShipId ShipId = ETDShipId::Rift;
	ETDWingmanState State = ETDWingmanState::Formation;
	TWeakObjectPtr<AActor> CurrentTarget;
	TWeakObjectPtr<AActor> FocusTarget;

	double StateEnterTime = 0.0;
	double NextTargetEvalTime = 0.0;
	double NextSecondaryTime = 0.0;
	double SecondaryReleaseTime = 0.0;
	double NextEvadeAllowedTime = 0.0;
	double NextAvoidanceSweepTime = 0.0;
	double NextAimErrorTime = 0.0;
	FVector EvadeDirection = FVector::ZeroVector;
	FVector CachedAvoidance = FVector::ZeroVector;
	FRotator AimError = FRotator::ZeroRotator;
	float PrevShield01 = 1.f;
	/** Regroup was forced (leash/evade): don't re-engage for RegroupCommitSeconds. */
	bool bRegroupCommitted = false;
	bool bPrimaryHeld = false;
	bool bSecondaryHeld = false;
	FRandomStream Random;

	/** Reused every target evaluation (no per-frame allocation). */
	mutable TArray<AActor*> ScratchHostiles;
};

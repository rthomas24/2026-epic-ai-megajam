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
 * Flies an ATDShip like a pilot: ATDSpacecraft::SetFlightInput, UTDWeaponComponent (primary/secondary) and,
 * since #201, the ship's own signature powers and unlocked ability when they are ready (TDWingmanShadow.cpp).
 * Shadow mode (ATDSquadManager::IsShadowMode) decides how far it ranges and whose targets it takes.
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

	/** True while this wingman is off defending the player from a threat outside the view. */
	bool IsDefendingPlayer() const { return bDefendingPlayer && CurrentTarget.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Wingman")
	ETDShipId GetShipId() const { return ShipId; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Wingman")
	ATDShip* GetShip() const;

	/** Prioritise Target (from the squad's focus command) until it dies or the command expires. */
	void SetFocusTarget(AActor* Target);

	/** #201: shadow mode flipped: look for a target under the new rules now. */
	void NotifyShadowModeChanged();

	/** #201: the squad's shadow mode (true without a squad). */
	bool IsShadowMode() const;

	/** #201: the leash in force (cm): shadow = min(LeashDistance, td.Squad.Shadow.Leash); solo = the capped solo leash. */
	float GetLeash() const;

	/** #201: powers this wingman has used since it was spawned, by slot (0 = signature, 1 = second signature, 2 = ability). */
	int32 GetPowerUseCount(int32 Slot) const { return (Slot >= 0 && Slot < 3) ? PowerUseCount[Slot] : 0; }

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

	/** #201: further than this from its slot a wingman flies straight at it, boosting (no parallel formation flight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (Units = "cm"))
	float FarReturnDistance = 9000.f;

	/** Formation throttle may reach the leader's speed x this (T1 SetThrottleTopSpeed), so slow ships keep up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (ClampMin = "1"))
	float CatchUpSpeedRatio = 1.25f;

	/** cm/s of extra speed per cm the wingman is behind its slot (negative when ahead). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (ClampMin = "0"))
	float FormationSpeedGain = 1.2f;

	/** Beyond this distance after a leader teleport, warp-follow instead of flying. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (Units = "cm"))
	float TeleportFollowDistance = 30000.f;

	/** After a portal/gate jump, wingmen arrive this far AHEAD of the leader's exit point (then ease back into the V). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Formation", meta = (Units = "cm"))
	float PortalExitClearance = 1200.f;

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

	/** Distance bands (2000 cm each) a target directly behind the leader costs vs one dead ahead, so
	 *  with a choice wingmen fight in front of the player's camera. 0 = ignore. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (ClampMin = "0"))
	float OnScreenTargetBias = 2.5f;

	/** Targets inside this fraction of the player's half field of view count as "on screen": when any
	 *  are in range, a wingman not defending the player picks only among them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (ClampMin = "0.1", ClampMax = "1.5"))
	float ViewConeScale = 0.9f;

	/** An on-screen target is preferred only if it scores within this many 2000 cm bands of the best. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (ClampMin = "0"))
	float InViewMaxExtraBands = 4.f;

	/** Engaging (not defending): get at least this far in front of the player's chase camera (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "cm"))
	float InShotMinDepth = 2500.f;

	/** How hard a wingman behind the camera turns to overtake into the shot (vs its pursuit direction). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (ClampMin = "0"))
	float InShotOvertakeWeight = 0.f;

	/** Never fly inside this cone around the player's aim line (deg; plus 4 m). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "deg"))
	float AimLineClearanceDeg = 8.f;

	/** The player's last attacker counts as a threat for this long after its last hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Engage", meta = (Units = "s"))
	float ThreatMemorySeconds = 4.f;

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

	/** Survival (td.Wingmen.Survival): below this hull, stay out of fights (focus orders excepted)... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Evade", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHullStayOut01 = 0.3f;
	/** ...until the shield is back above this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Evade", meta = (ClampMin = "0", ClampMax = "1"))
	float ShieldBackIn01 = 0.8f;
	/** Losing this much hull within HullBurstWindowSeconds also triggers an evade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Evade", meta = (ClampMin = "0", ClampMax = "1"))
	float HullBurst01 = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Evade", meta = (Units = "s"))
	float HullBurstWindowSeconds = 1.f;

	/** A hurt wingman only seeks an active shield dome within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Evade", meta = (Units = "cm"))
	float DomeSeekDistance = 15000.f;

	/** Look-ahead time for obstacle avoidance sweeps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (Units = "s"))
	float AvoidanceLookAhead = 1.2f;

	/** How hard squadmates push apart inside TeammateSeparation (relative to the desired direction). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (ClampMin = "0"))
	float TeammateSeparationStrength = 5.f;

	/** cm/s of speed change per unit of along-track separation push. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (ClampMin = "0"))
	float SeparationSpeedGain = 600.f;

	/** Ship radius used for obstacle probes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (Units = "cm"))
	float AvoidanceShipRadius = 500.f;

	/** Obstacle probes are never shorter than this (cm), so a slow, turning wingman still sees a rock in time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (Units = "cm", ClampMin = "0"))
	float MinAvoidanceProbe = 6000.f;

	/** A rock surface nearer than this (cm, past the ship's radius) pushes the wingman straight away from it, every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (Units = "cm", ClampMin = "0"))
	float CloseAvoidanceRange = 1500.f;

	/** Set by ApplyAvoidance each frame: 1 in open space, down to 0.45 against a rock surface (SteerToward applies it). */
	float AvoidSpeedScale = 1.f;

	/** Keep at least this far from squadmates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wingman|Avoidance", meta = (Units = "cm"))
	float TeammateSeparation = 1500.f;

	/** While a director shot runs, keep at least this far from its camera (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wingman|Avoidance", meta = (ClampMin = "0"))
	float ShotCameraClearance = 2500.f;

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

	/** Passive: no input, no triggers, no targets (landing, cinematics). Leaving passive regroups. */
	void SetPassive(bool bInPassive);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Wingman")
	bool IsPassive() const { return bPassive; }

	/** True while forming up for the leader's hyperdrive jump (its hyperdrive is not Idle). Re-read every tick. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Wingman")
	bool IsFormingUpForJump() const { return bHyperdriveFormUp; }

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

	// ---- #201 (TDWingmanShadow.cpp) ----
	/** Shadow OFF: the wingman's own choice: nearest, weakest, and whoever is hunting IT, inside the solo leash. */
	AActor* PickTargetSolo(ATDShip* Ship, ATDShip* Leader) const;
	/** Use the ship's powers when they are ready and worth it (a few times a second). */
	void UpdatePowers(ATDShip* Ship, ATDShip* Leader);
	void ThinkLancerPowers(ATDShip* Ship, ATDShip* Leader);
	void ThinkBastionPowers(ATDShip* Ship, ATDShip* Leader);
	void ThinkRiftPowers(ATDShip* Ship, ATDShip* Leader);
	/** Shadow ON: Enemy is the player's business: the player's target, a focus order, or someone attacking the player. */
	bool IsPlayersFight(const AActor* Enemy, const ATDShip* Leader) const;
	/** May this wingman use an OFFENSIVE power on Enemy right now? Always when hunting alone. */
	bool MayAttackWithPower(const AActor* Enemy, const ATDShip* Leader) const;
	/** td.Squad.LogTargets: why the last PickTarget chose what it chose. */
	mutable const TCHAR* LastPickReason = TEXT("");
	mutable TWeakObjectPtr<AActor> LastLoggedTarget;
	void LogPick(const AActor* Picked, const TCHAR* Reason) const;
	/** Ready, and the padded AI cooldown has passed. */
	bool IsPowerReadyForAI(const ATDShip* Ship, int32 Slot) const;
	/** Start the power; logs "wingman <ship> used <power> on <target> because <reason>" and asks for the pilot's line. */
	bool UsePower(ATDShip* Ship, int32 Slot, const AActor* Target, const FString& Reason, FName BarkId);
	/** "Your fight": the target has been held at the player's floor since this time (-1 = it is not). */
	double HeldAtFloorSince = -1.0;
	/** ...and after td.Squad.Shadow.YourFightRetarget it is left alone until this time. */
	TWeakObjectPtr<AActor> LeftForPlayer;
	double LeftForPlayerUntil = -1.0;
	double NextPowerThinkTime = 0.0;
	double LastAnyPowerTime = -1000.0;
	double LastPowerUseTime[3] = { -1000.0, -1000.0, -1000.0 };
	int32 PowerUseCount[3] = { 0, 0, 0 };
	/** An AI ram is running: the dash is ended once past the target or over the leash. */
	bool bAIRamming = false;
	TWeakObjectPtr<AActor> RamTarget;
	/** Rift: portal A is placed for a missile, B follows on the next think. */
	bool bPortalBPending = false;

	/** Formation slot, or the shield dome when hurt and a dome is up. */
	FVector GetHomeLocation(ATDShip* Ship, ATDShip* Leader, bool& bOutIsDome) const;
	bool ShouldStayOutOfFights(const ATDShip* Ship) const;
	double HullWindowStart = 0.0;
	float HullAtWindowStart = 1.f;

	/** Turn the desired world direction + speed into FTDFlightInput and send it to the ship. */
	void SteerToward(ATDShip* Ship, const FVector& DesiredDir, float TargetSpeed, bool bBoost);

	/** Bend DesiredDir away from obstacles and squadmates. */
	FVector ApplyAvoidance(ATDShip* Ship, const FVector& DesiredDir);

	/** Push away from squadmates closer than TeammateSeparation (unscaled direction * strength). */
	FVector ComputeSeparation(const ATDShip* Ship) const;

	void SetTriggers(bool bPrimary, bool bSecondary);
	void BeginEvade(ATDShip* Ship);
	double Now() const;

	TWeakObjectPtr<ATDSquadManager> Squad;
	ETDShipId ShipId = ETDShipId::Rift;
	ETDWingmanState State = ETDWingmanState::Formation;
	TWeakObjectPtr<AActor> CurrentTarget;

	double StateEnterTime = 0.0;
	double NextTargetEvalTime = 0.0;
	mutable bool bDefendingPlayer = false;
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
	bool bPassive = false;
	bool bHyperdriveFormUp = false;
	bool bPrimaryHeld = false;
	bool bSecondaryHeld = false;
	FRandomStream Random;

	/** Reused every target evaluation (no per-frame allocation). */
	mutable TArray<AActor*> ScratchHostiles;
};

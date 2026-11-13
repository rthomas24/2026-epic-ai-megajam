// Triad Drift - Enemies & Waves (Teammate 4). ENG-13 enemy pilot AI.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TDEnemyAIController.generated.h"

class ATDEnemyShip;

UENUM(BlueprintType)
enum class ETDEnemyAIState : uint8
{
	Idle,			// no target (or tracking lost): cruise / loiter
	Formation,		// pack follower holding its slot on the leader
	Approach,		// closing to engage range
	AttackRun,		// guns hot, heavy attacks when lined up
	BreakOff,		// peel away after the run
	Stunned			// EMP: dead stick
};

/**
 * Flies an ATDEnemyShip through SetFlightInput (never moves it directly) and drives its attacks.
 * Cycle: Approach -> AttackRun -> BreakOff -> Approach (re-approach). Role tweaks:
 *  - Fighter packs: one leader runs the cycle; followers hold formation and fire when the leader does.
 *  - Interceptor: always hunts the player's active (possessed) ship.
 *  - Gunship: slow, faces its target and holds range instead of breaking off.
 *  - Bomber: flies along the squad's route ahead of the ships and lays mine trails.
 *  - Carrier: holds station, turns broadside; bays and turrets do the fighting.
 * Targets ANY squad ship via UTDCombatRegistry.
 */
UCLASS()
class UNREALGAMEJAM_API ATDEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATDEnemyAIController();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	ETDEnemyAIState GetAIState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	AActor* GetTarget() const { return Target.Get(); }

	/** Force a target (squad focus tests, scripted moments). Ignored while tracking is lost. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|EnemyAI")
	void SetTarget(AActor* NewTarget);

	/** Make this pilot a follower of Leader at a local-space offset (cm). Null leader = fly solo. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|EnemyAI")
	void SetFormationLeader(ATDEnemyShip* Leader, FVector LocalOffset);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	ATDEnemyShip* GetFormationLeader() const { return FormationLeader.Get(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	ATDEnemyShip* GetEnemyShip() const;

	/** Override the per-pilot "prefers the player's ship" roll (tests, scripted pilots). */
	void SetPrefersPlayerShip(bool bPrefer) { bPrefersPlayer = bPrefer; }

	/** Re-seed this pilot's random choices (ATDEnemyShip::SetSpawnSeed). */
	void ReseedRandom(int32 Seed) { Rng.Initialize(Seed); }

	/** Forget the target immediately (called by the ship's LoseTracking). */
	void ForgetTarget();

	/** Seconds between target re-evaluations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0.05"))
	float RetargetInterval = 1.0f;

	/** Max distance to acquire a target (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float AcquireRange = 60000.f;

	/** Steering gain: input = clamp(angle_error / SteerFullDeflectionDeg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "1"))
	float SteerFullDeflectionDeg = 25.f;

	/** How far ahead of the squad a bomber lays its trail (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float BomberLeadDistance = 9000.f;

	/** Gunships hold this fraction of their engage range instead of breaking off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0", ClampMax = "1"))
	float GunshipHoldRangeFraction = 0.6f;

	/** Gunships steer at their target plus its velocity x this (s): covers the heavy hull's turning lag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float GunshipTurnLeadSeconds = 0.6f;

	/**
	 * Leash: beyond this distance from its target (or with no target in range) a pilot flies at
	 * CatchUpSpeedMultiplier x its cruise so a wave can always be finished, even vs a fleeing Lancer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float CatchUpDistance = 22000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "1"))
	float CatchUpSpeedMultiplier = 1.7f;

	/** Break off when time-to-impact with the target drops below this (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float BreakOffTimeToImpact = 0.6f;

	/** The interceptor's telegraphed homing missile never starts closer than this (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float TelegraphMinDistance = 6000.f;

	/** A gunship's heavy-bolt volley fires at a ship charging it down to this distance (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float GunshipBoltMinDistance = 1500.f;

	/** A gunship's missile salvo needs this much room (cm) and a time-to-impact longer than its wind-up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float GunshipSalvoMinDistance = 4000.f;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	bool IsCatchingUp() const { return bCatchingUp; }

	/** Attack runs on the player's ship steer for a point this far in front of its nose (readability). 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float CrossInFrontDistance = 5000.f;

	/**
	 * Opening pass (readability): waves warp in behind or beside the player, out of view. A fast pilot's
	 * first move is to overtake and cross the player's nose OpeningPassAhead in front (OpeningPassSide to one
	 * side) at catch-up speed, before its first attack run. Only for pilots whose catch-up speed is at
	 * least OpeningPassMinSpeed (fighters, interceptors); heavies keep their normal approach.
	 * CVar TD.EnemyOpeningPass 0 turns it off.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float OpeningPassAhead = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float OpeningPassSide = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float OpeningPassMaxSeconds = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float OpeningPassMinSpeed = 9000.f;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	bool IsOnOpeningPass() const { return bOpeningPass; }

	/**
	 * Attack slots (ENG-27): at most this many pilots may be on an attack run against the same squad ship at
	 * once. The rest hold a visible orbit at AttackSlotHoldDistance around it and take a slot when one frees.
	 * The wave director sets it per wave (UTDWaveDirector::AttackSlotsPerTarget). 0 = unlimited.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	int32 AttackSlotsPerTarget = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float AttackSlotHoldDistance = 9000.f;

	/** True while this pilot waits for an attack slot (holding its orbit). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	bool IsHoldingForSlot() const { return bHoldingForSlot; }

	/** Keep at least this far from other Armada ships (cm). 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float SeparationRadius = 1400.f;

	/** Chance a non-interceptor pilot prefers the player's ship over the nearest squad ship. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0", ClampMax = "1"))
	float PreferPlayerShipChance = 0.5f;

	/** Obstacle look-ahead (seconds of travel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI", meta = (ClampMin = "0"))
	float AvoidLookAheadSeconds = 1.2f;

	/** True while steering around an obstacle this frame. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	bool IsAvoiding() const { return bAvoiding; }

	/** Enable on-screen debug draws (state, target line). Console: TD.EnemyAIDebug 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EnemyAI")
	bool bDebugDraw = false;

	/**
	 * Pure steering maths (tests call it without a world): Aim input (X yaw, Y pitch, each -1..1)
	 * that turns a ship with transform Ship toward WorldPoint. Full deflection at FullDeflectionDeg.
	 */
	static FVector2D ComputeSteer(const FTransform& Ship, const FVector& WorldPoint, float FullDeflectionDeg);

	/** Seconds in the current state. */
	float GetStateTime() const { return StateTime; }

	/** Deterministic seed for aim error / break-off direction (tests). */
	void SetRandomSeed(int32 Seed) { Rng.Initialize(Seed); }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void SetState(ETDEnemyAIState NewState);
	void UpdateTarget(float DeltaSeconds);
	AActor* ChooseTarget() const;

	void TickIdle(ATDEnemyShip& Ship, float DeltaSeconds);
	void TickFormation(ATDEnemyShip& Ship, float DeltaSeconds);
	void TickApproach(ATDEnemyShip& Ship, float DeltaSeconds);
	void TickAttackRun(ATDEnemyShip& Ship, float DeltaSeconds);
	void TickBreakOff(ATDEnemyShip& Ship, float DeltaSeconds);
	void TickBomber(ATDEnemyShip& Ship, AActor& InTarget, float DeltaSeconds);
	void TickCarrier(ATDEnemyShip& Ship, AActor& InTarget, float DeltaSeconds);
	void TickGunship(ATDEnemyShip& Ship, AActor& InTarget, float DeltaSeconds);
	bool IsValidTarget(const AActor* Candidate) const;

	/** Steer toward a world point with a throttle 0..1. Writes SetFlightInput. */
	void SteerToward(ATDEnemyShip& Ship, const FVector& WorldPoint, float Throttle01, bool bBoost = false);
	void TryHeavyAttacks(ATDEnemyShip& Ship, AActor& InTarget);
	FVector GetAimPoint(const ATDEnemyShip& Ship, const AActor& InTarget) const;
	FVector GetBomberRoutePoint(const AActor& InTarget) const;

	ETDEnemyAIState State = ETDEnemyAIState::Idle;
	bool bAvoiding = false;
	bool bPrefersPlayer = false;
	bool bCatchingUp = false;
	bool bRunReachedGunRange = false;
	bool bOpeningPass = false;
	bool bHoldingForSlot = false;
	float HoldAngle = 0.f;
	/** How many other pilots are on an attack run against Target right now. */
	int32 CountAttackersOn(const AActor* InTarget) const;
	bool bOpeningPassChecked = false;
	float OpeningPassTime = 0.f;
	float OpeningPassSideSign = 1.f;
	/** Starts (first tick after possession, once the spec is known) and ends the opening pass. */
	void UpdateOpeningPass(ATDEnemyShip& Ship, float DeltaSeconds);
	FVector GetOpeningPassPoint() const;
	void UpdateCatchUp(ATDEnemyShip& Ship);
	static float TimeToImpact(const AActor& A, const AActor& B);
	float StateTime = 0.f;
	float RetargetTimer = 0.f;
	FVector BreakOffDirection = FVector::ForwardVector;

	TWeakObjectPtr<AActor> Target;
	TWeakObjectPtr<ATDEnemyShip> FormationLeader;
	FVector FormationOffset = FVector::ZeroVector;

	FRandomStream Rng;
};

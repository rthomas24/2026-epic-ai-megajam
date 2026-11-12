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
	float BreakOffTimeToImpact = 0.8f;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|EnemyAI")
	bool IsCatchingUp() const { return bCatchingUp; }

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
	bool bCatchingUp = false;
	bool bRunReachedGunRange = false;
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

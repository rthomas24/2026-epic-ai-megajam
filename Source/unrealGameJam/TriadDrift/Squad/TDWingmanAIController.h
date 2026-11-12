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

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetState(ETDWingmanState NewState);
	void UpdateFormation(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader);
	void UpdateEngage(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader);
	void UpdateEvade(float DeltaSeconds, ATDShip* Ship, ATDShip* Leader);

	AActor* PickTarget(ATDShip* Ship, ATDShip* Leader) const;

	/** Turn the desired world direction + throttle into FTDFlightInput and send it to the ship. */
	void SteerToward(ATDShip* Ship, const FVector& DesiredDir, float Throttle01, bool bBoost);

	/** Bend DesiredDir away from obstacles and squadmates. */
	FVector ApplyAvoidance(ATDShip* Ship, const FVector& DesiredDir) const;

	void SetTriggers(bool bPrimary, bool bSecondary);

	UFUNCTION()
	void HandleShieldBroken();

	TWeakObjectPtr<ATDSquadManager> Squad;
	ETDShipId ShipId = ETDShipId::Rift;
	ETDWingmanState State = ETDWingmanState::Formation;
	TWeakObjectPtr<AActor> CurrentTarget;
	TWeakObjectPtr<AActor> FocusTarget;

	double StateEnterTime = 0.0;
	double NextTargetEvalTime = 0.0;
	double NextSecondaryTime = 0.0;
	FVector EvadeDirection = FVector::ZeroVector;
	FRotator AimError = FRotator::ZeroRotator;
	double NextAimErrorTime = 0.0;
	bool bPrimaryHeld = false;
	bool bSecondaryHeld = false;
};

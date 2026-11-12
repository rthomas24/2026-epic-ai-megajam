// Triad Drift - ground gameplay actors: interactables, doors, encounter volumes, stand-in crew and a
// practice turret (ENG-24; crew AI proper is ENG-23). Owner: Teammate 5. Zero content assets.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Ground/TDGroundTypes.h"
#include "TriadDrift/Ground/Crew/TDCrewTypes.h"
#include "TDGroundActors.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UTDHealthComponent;
class ATDObjective;
class ATDDoor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnInteracted, AActor*, Interactable, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnEncounterCleared, AActor*, Encounter);

// ================================================================================================
/** Tap or hold to use (console, relay node, charge site). Can complete a step of an objective. */
UCLASS()
class UNREALGAMEJAM_API ATDInteractableActor : public AActor, public ITDInteractable
{
	GENERATED_BODY()

public:
	ATDInteractableActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	FText Prompt = NSLOCTEXT("TriadDrift", "InteractDefault", "Interact");

	/** 0 = tap. Otherwise hold for this long (damage interrupts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact", meta = (ClampMin = "0", Units = "s"))
	float HoldTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	bool bSingleUse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	bool bEnabled = true;

	/** Optional: interacting completes this step of this objective (only while it is the current step). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	TObjectPtr<ATDObjective> LinkedObjective;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	int32 LinkedStep = 0;

	UPROPERTY(BlueprintAssignable, Category = "Interact")
	FTDOnInteracted OnInteracted;

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void ResetUse() { bUsed = false; }

	UFUNCTION(BlueprintPure, Category = "Interact")
	bool WasUsed() const { return bUsed; }

	// ITDInteractable
	virtual FText GetPrompt() const override { return Prompt; }
	virtual bool CanInteract(AActor* Interactor) const override;
	virtual void Interact(AActor* Interactor) override;
	virtual float GetHoldTime() const override { return HoldTime; }

	UPROPERTY(VisibleAnywhere, Category = "Interact")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	bool bUsed = false;
};

// ================================================================================================
/** A door that slides up out of the way. Zero assets: an engine cube. */
UCLASS()
class UNREALGAMEJAM_API ATDDoor : public AActor
{
	GENERATED_BODY()

public:
	ATDDoor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (Units = "cm"))
	float OpenHeight = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (Units = "s"))
	float OpenSeconds = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bStartOpen = false;

	UFUNCTION(BlueprintCallable, Category = "Door") void Open();
	UFUNCTION(BlueprintCallable, Category = "Door") void Close();
	UFUNCTION(BlueprintPure, Category = "Door") bool IsOpen() const { return bWantOpen; }
	UFUNCTION(BlueprintPure, Category = "Door") float GetOpen01() const { return Open01; }

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<UStaticMeshComponent> Mesh;

protected:
	virtual void BeginPlay() override;

private:
	bool bWantOpen = false;
	float Open01 = 0.f;
	FVector ClosedRelative = FVector::ZeroVector;
};

// ================================================================================================
/**
 * Stand-in crew enemy until ENG-23: a capsule with a cube body, Enemy team, health, staggerable,
 * and a simple telegraphed melee swing so blocks and parries can be tested by hand.
 */
UCLASS()
class UNREALGAMEJAM_API ATDCrewDummy : public ACharacter, public ITDTeam, public ITDStaggerable
{
	GENERATED_BODY()

public:
	ATDCrewDummy(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew")
	float MaxHealth = 60.f;

	/** Melee swing damage (grunt baton: 15). 0 = passive target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew")
	float MeleeDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew", meta = (Units = "cm"))
	float MeleeRange = 160.f;

	/** Seconds between swings when the player is in range; 0 = never swings on its own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew", meta = (Units = "s"))
	float MeleeInterval = 0.f;

	/** Telegraph before the hit lands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew", meta = (Units = "s"))
	float MeleeWindup = 0.5f;

	/** Swing at Target right now (no windup). Returns the damage dealt. */
	UFUNCTION(BlueprintCallable, Category = "Crew")
	float MeleeAttack(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Crew") bool IsStaggered() const { return StaggerLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "Crew") bool IsTelegraphing() const { return WindupLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "Crew") bool IsGuardBroken() const { return bGuardBroken; }

	UTDHealthComponent* GetHealth() const { return Health; }

	// ITDTeam
	virtual ETDTeam GetTeam() const override { return ETDTeam::Enemy; }
	// ITDStaggerable
	virtual void ApplyStagger(float Seconds, AActor* Source) override;
	virtual void BreakGuard(AActor* Source) override { bGuardBroken = true; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Crew")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Crew")
	TObjectPtr<UTDHealthComponent> Health;

private:
	UFUNCTION()
	void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	float StaggerLeft = 0.f;
	float WindupLeft = 0.f;
	float SwingCooldown = 0.f;
	bool bGuardBroken = false;
};

// ================================================================================================
/** Practice turret: fires slow, bright, deflectable 3-round bursts at the player (trooper bolts). */
UCLASS()
class UNREALGAMEJAM_API ATDDummyTurret : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDDummyTurret();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (Units = "s"))
	float BurstInterval = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "1"))
	int32 ShotsPerBurst = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (Units = "s"))
	float ShotSpacing = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float BoltDamage = 10.f;

	/** Slow and readable so it can be blocked by eye. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (Units = "cm/s"))
	float BoltSpeed = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (Units = "cm"))
	float MaxRange = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float MaxHealth = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	bool bActive = true;

	/** Fire one bolt at Target now (tests). Returns the projectile. */
	AActor* FireAt(AActor* Target);

	UTDHealthComponent* GetHealth() const { return Health; }

	virtual ETDTeam GetTeam() const override { return ETDTeam::Enemy; }
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UTDHealthComponent> Health;

private:
	AActor* FindTarget() const;
	float BurstTimer = 1.f;
	float ShotTimer = 0.f;
	int32 ShotsLeft = 0;
};

// ================================================================================================
/**
 * Encounter volume: when the player walks in (or Activate is called) it closes its doors, spawns a crew
 * group, and when every member is dead it opens the doors, fires OnCleared and optionally completes an
 * objective step. Resets with its objective on a retake.
 */
UCLASS()
class UNREALGAMEJAM_API ATDEncounterVolume : public AActor
{
	GENERATED_BODY()

public:
	ATDEncounterVolume();

	/** What to spawn (default: ATDCrewCharacter, the ENG-23 crew). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TSubclassOf<AActor> CrewClass;

	/** Crew types, cycled across the spawn points (only for ATDCrewCharacter classes). Empty = all grunts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<ETDCrewType> CrewMix;

	/** Spawn points relative to the volume. If empty, Count crew are placed in a ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (MakeEditWidget))
	TArray<FVector> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (ClampMin = "1"))
	int32 Count = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (Units = "cm"))
	float RingRadius = 500.f;

	/** Doors opened when the room is clear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<TObjectPtr<ATDDoor>> DoorsToOpen;

	/** Doors shut behind the player when the fight starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<TObjectPtr<ATDDoor>> DoorsToLock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TObjectPtr<ATDObjective> LinkedObjective;

	/** Step completed on clear (INDEX_NONE = none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	int32 LinkedStep = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	bool bTriggerOnOverlap = true;

	UPROPERTY(BlueprintAssignable, Category = "Encounter")
	FTDOnEncounterCleared OnCleared;

	UFUNCTION(BlueprintCallable, Category = "Encounter") void Activate();
	UFUNCTION(BlueprintCallable, Category = "Encounter") void ResetEncounter();
	UFUNCTION(BlueprintPure, Category = "Encounter") bool IsActive() const { return bActivated && !bCleared; }
	UFUNCTION(BlueprintPure, Category = "Encounter") bool IsCleared() const { return bCleared; }
	UFUNCTION(BlueprintPure, Category = "Encounter") int32 GetAliveCount() const;

	const TArray<TWeakObjectPtr<AActor>>& GetSpawned() const { return Spawned; }

	UPROPERTY(VisibleAnywhere, Category = "Encounter")
	TObjectPtr<UBoxComponent> Box;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void HandleMemberDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	void HandleObjectiveReset(ATDObjective* Objective);
	void CheckCleared();

	TArray<TWeakObjectPtr<AActor>> Spawned;
	bool bActivated = false;
	bool bCleared = false;
	FDelegateHandle ResetHandle;
};

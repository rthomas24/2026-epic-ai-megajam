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
/**
 * Art for the code-built shapes (Ryan: no basic shapes visible). Every shape keeps its footprint and its collision;
 * when its art mesh exists (Gameplay/Props/Planet/<Name>, or a slot set per spawn) the art is shown in its place,
 * fitted to the shape's exact box, with no collision, and follows the shape's visibility switches.
 */
namespace TDPropArt
{
	/** /Game/TriadDrift/Gameplay/Props/Planet/<Name>.<Name> */
	UNREALGAMEJAM_API TSoftObjectPtr<UStaticMesh> Default(const TCHAR* Name);
	/** The slot's mesh if its package exists (quiet when it doesn't yet). */
	UNREALGAMEJAM_API UStaticMesh* Load(const TSoftObjectPtr<UStaticMesh>& Slot);
	/** Show Art in Shape's place; Shape is hidden in game but keeps its collision and visibility switches reach the art.
	 *  Fitted (default): the art is stretched to the shape's exact box (turned 90 deg when its long horizontal axis
	 *  runs the other way). Placed: true scale, its bottom-centre on the shape's bottom-centre (combined pieces such as
	 *  the trailhead or the jammer tower that cover several shapes). */
	UNREALGAMEJAM_API UStaticMeshComponent* Dress(UStaticMeshComponent* Shape, UStaticMesh* Art, bool bFit = true);
}

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

	/** False: usable only during LinkedStep but doesn't complete it (a counter does: resonance nodes, charges). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	bool bCompleteLinkedStep = true;

	/** A hold that taking damage doesn't break (the wreck-core extraction under fire). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	bool bHoldSurvivesDamage = false;

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

	/**
	 * Invisible column on the console's top (pawns only), taller than any jump: nobody perches on a console out of every
	 * melee crewman's reach (Lead, Nov 13). Covers the whole top at any console size.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Interact")
	TObjectPtr<UStaticMeshComponent> TopCap;

	/** Art in place of the cube (fitted to its box; the cube stays the collision and the interact target). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art")
	TSoftObjectPtr<UStaticMesh> ConsoleMesh;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/** Art for the door leaf (fitted to its box; the box stays the collision and slides as before). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art")
	TSoftObjectPtr<UStaticMesh> DoorMesh;

	/**
	 * Story clue (storyline.md §6): the door recognises the pilot. When it opens it lights AccessText over the opening,
	 * on both faces, for AccessTextSeconds. No explanation, ever (the Cinder dig door; every Dominion interior door).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Story")
	bool bRecognisesPilot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Story")
	FText AccessText = NSLOCTEXT("TriadDrift", "DoorAccessRecognised", "ACCESS GRANTED - ID RECOGNISED");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Story", meta = (Units = "s"))
	float AccessTextSeconds = 4.f;

	/** Optional: played at the door when it recognises the pilot (the Warden door's three-pulse authentication). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Story")
	TSoftObjectPtr<class USoundBase> AccessSound;

	/** A standing line over the door (both faces) until cleared: the reactor door's "ARCHIVE PLAYBACK · n s". */
	void ShowStatusText(const FText& Text);
	void ClearStatusText();
	FText GetStatusText() const { return StatusText; }

	/** True while the access line is shown (tests). */
	UFUNCTION(BlueprintPure, Category = "Door|Story") bool IsAccessTextShown() const;
	/** How many times this door has greeted the pilot. */
	int32 GetAccessGreetings() const { return AccessGreetings; }

protected:
	virtual void BeginPlay() override;

private:
	bool bWantOpen = false;
	float Open01 = 0.f;
	FVector ClosedRelative = FVector::ZeroVector;
	float AccessTextLeft = 0.f;
	int32 AccessGreetings = 0;
	UPROPERTY(Transient) TArray<TObjectPtr<class UTextRenderComponent>> AccessLabels;
	void ShowAccessText();
	void EnsureLabels();
	FText StatusText;
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
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

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
	virtual void BeginPlay() override;

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

	/** Optional names cycled across the spawned crew like CrewMix (the reactor room's future pilots: FuturePilot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<FName> MemberNames;

	/** Spawn points relative to the volume. If empty, Count crew are placed in a ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (MakeEditWidget))
	TArray<FVector> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (ClampMin = "1"))
	int32 Count = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (Units = "cm"))
	float RingRadius = 500.f;

	/** Set-dressing blocks crew can hide behind (registered with UTDCrewCoverSubsystem on Activate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<TObjectPtr<AActor>> CoverActors;

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

	/** Spawned crew hold fire this long: the fight shows itself before it hurts (playtest: an ambush trooper's first
	 *  burst landed 0.5 s after the crew appeared, and a pilot who didn't react was dead in 5 s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (Units = "s", ClampMin = "0"))
	float OpeningGraceSeconds = 1.2f;

	/** A member (never the Warden) that has had no line of sight to the pilot and lost no health for this long is
	 *  removed, so a fight cannot stall on crew stuck behind a wall (T7's play, Nov 14: a Trooper and a Heavy stood
	 *  23 m away behind the relay's terminal wall for 2 minutes). 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (Units = "s", ClampMin = "0"))
	float StragglerSeconds = 30.f;

	/** One straggler pass (the encounter runs it every second while the fight is on); public for tests. */
	void SweepStragglers(float Seconds);

	/** Director's mercy: easing per take of this fight (empty = always the same fight). Sorted by FromTake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<FTDRetakeMercyTier> MercyTiers;

	UPROPERTY(BlueprintAssignable, Category = "Encounter")
	FTDOnEncounterCleared OnCleared;

	/** This fight's current take (0 before it first starts, 1 on the first attempt). */
	UFUNCTION(BlueprintPure, Category = "Encounter") int32 GetTake() const { return Take; }
	/** The mercy tier in force for the current take, or null. */
	const FTDRetakeMercyTier* GetActiveMercy() const;

	UFUNCTION(BlueprintCallable, Category = "Encounter") void Activate();
	UFUNCTION(BlueprintCallable, Category = "Encounter") void ResetEncounter();

	/** Starts the fight if the player is already inside (walked in early, respawned inside, step just began). */
	UFUNCTION(BlueprintCallable, Category = "Encounter") bool ActivateIfPlayerInside();

	/** Its objective was already done when the level loaded (Continue): cleared, doors open, never arms. */
	void MarkClearedOnLoad();

	/** Add an actor spawned mid-fight (a Warden's summons) so it counts toward the clear and is reset with the room. */
	void AdoptMember(AActor* Member);
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

	/** A member that vanished without dying (fell out of the world, destroyed) still counts as gone. */
	UFUNCTION()
	void HandleMemberDestroyed(AActor* Member);

public:
	/** The scene registry calls this when an objective step begins (the player may already be inside). */
	void HandleStepStarted(ATDObjective* Objective, int32 StepIndex);
private:

	void CheckCleared();
	bool IsPlayerPawn(const AActor* Actor) const;

	TArray<TWeakObjectPtr<AActor>> Spawned;
public:
	/** A boss that leaves the world without dying (kill-Z, a fall) is put back this many times before the room may clear. */
	static constexpr int32 MaxBossReturns = 3;
private:
	struct FStraggler { float Unseen = 0.f; float LastHealth = -1.f; };
	/** Where each member was spawned (a boss that leaves the world without dying is put back there). */
	TMap<TWeakObjectPtr<AActor>, FTransform> SpawnedAt;
	int32 BossReturns = 0;
	bool ReturnLostBoss(AActor* Member);
	TMap<TWeakObjectPtr<AActor>, FStraggler> Stragglers;
	FTimerHandle StragglerTimer;
	bool bActivated = false;
	bool bCleared = false;
	int32 Take = 0;
};

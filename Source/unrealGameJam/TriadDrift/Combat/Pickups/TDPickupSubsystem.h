// Triad Drift - space pickups (Teammate 2). docs/eng/WORLD-DENSITY-PLAN.md section 3.5; the Lead's spec, Nov 13.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"
#include "TDPickupSubsystem.generated.h"

class ATDShip;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnPickupCollected, ETDPickupType, Type, AActor*, Ship);

/** Values and limits (modest on purpose: pickups help, they don't carry a wave). */
USTRUCT(BlueprintType)
struct FTDPickupTuning
{
	GENERATED_BODY()

	/** Hull repair: this fraction of max hull on the flown ship. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float HullRepair01 = .30f;
	/** Shield cell: the flown ship's shield refilled by this fraction of max; each living wingman by WingmanShield01. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float Shield01 = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float WingmanShield01 = .5f;
	/** Overcharge: outgoing weapon damage x this for OverchargeSeconds (a second one refreshes the timer, never stacks). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float OverchargeMultiplier = 1.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float OverchargeSeconds = 8.f;

	/** Drop chance per kill by type; carriers drop CarrierDrops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float FighterChance = .12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float MidChance = .25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") int32 CarrierDrops = 2;
	/** Drops of each type per wave (caches don't count); at most MaxAlive drops in the world at once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") int32 CapPerTypePerWave = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") int32 MaxAlive = 3;
	/** Safety: under this hull the next kill drops a repair, once per scene, if no repair dropped for SafetyGapSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float SafetyHull01 = .35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float SafetyGapSeconds = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float LifetimeSeconds = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float BlinkSeconds = 4.f;
	/** Generous: inside MagnetRadius the pickup flies to the flown ship; inside CollectRadius it is taken. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float MagnetRadius = 6000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float CollectRadius = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups") float MagnetSpeed = 9000.f;
};

/**
 * Space pickups. Off with td.Pickups 0 (then nothing spawns, nothing applies: today's game exactly).
 * - Drops: the bus's OnActorKilled for an Armada ship killed by the squad (chance by enemy type, caps per wave and
 *   alive), plus the low-hull safety repair. Nothing drops during a money shot's slow motion.
 * - Caches (T1's debris fields, T4): SpawnPickup(World, Type, Location) — no caps.
 * - Collection: only the ship the player flies, by distance (pickups have no collision).
 * - A CUT clears every pickup and the overcharge; a new take resets the safety drop. Not saved: they never persist.
 * HUD: GetPickups, GetOverchargeRemaining, OnPickupCollected (BlueprintAssignable: Nick's HUD plugin can bind it too).
 */
UCLASS()
class UNREALGAMEJAM_API UTDPickupSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDPickupSubsystem* Get(const UObject* WorldContext);
	static bool IsEnabled();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** A pickup at Location (caches, dev, tests). Null when pickups are off. Caches skip the caps; drops use them. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Pickups", meta = (WorldContext = "WorldContext"))
	static ATDPickup* SpawnPickup(const UObject* WorldContext, ETDPickupType Type, FVector Location);

	/** What a kill of Victim (an ATDEnemyShip) would do now: the drops it makes (tests call it; the bus path does too). */
	int32 HandleKill(AActor* Victim, AActor* Killer);

	/** Apply Type to Ship as if collected (tests; the collect path). False if Ship can't take it. */
	bool Collect(ATDPickup* Pickup, ATDShip* Ship);
	bool ApplyEffect(ETDPickupType Type, ATDShip* Ship);

	/** Clear every pickup and the overcharge (a CUT, the switch going off). */
	void ClearAll();

	const TArray<TObjectPtr<ATDPickup>>& GetPickups() const { return Pickups; }
	float GetOverchargeRemaining() const { return OverchargeLeft; }
	float GetOverchargeSeconds() const { return Tuning.OverchargeSeconds; }
	int32 GetDropsThisWave(ETDPickupType Type) const { return DropsThisWave[static_cast<int32>(Type)]; }
	int32 GetCollected(ETDPickupType Type) const { return Collected[static_cast<int32>(Type)]; }

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Pickups")
	FTDOnPickupCollected OnPickupCollected;

	UPROPERTY(EditAnywhere, Category = "TriadDrift|Pickups")
	FTDPickupTuning Tuning;

	/** Tests: a fixed seed for the drop rolls. */
	FRandomStream Rng;

	/** Bark ids for T3 (one per type; optional: no line, no subtitle). */
	static FName BarkFor(ETDPickupType Type);

private:
	UFUNCTION() void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);
	UFUNCTION() void HandleWaveStarted(int32 WaveNumber, float Intensity01);
	UFUNCTION() void HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds);
	UFUNCTION() void HandleTakeStarted(FName SceneId, int32 TakeNumber);

	ATDPickup* SpawnInternal(ETDPickupType Type, const FVector& Location);
	bool PickDropType(ETDPickupType& Out);
	ATDShip* GetFlownShip() const;
	void EndOvercharge();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDPickup>> Pickups;
	TWeakObjectPtr<ATDShip> OverchargedShip;
	TWeakObjectPtr<class UTDEventBus> Bus;
	float OverchargeLeft = 0.f;
	int32 DropsThisWave[4] = { 0, 0, 0, 0 };
	int32 Collected[4] = { 0, 0, 0, 0 };
	int32 CollectedThisWave[4] = { 0, 0, 0, 0 };
	int32 CurrentWave = 0;
	void LogWaveSummary() const;
	bool bSafetyUsedThisScene = false;
	bool bSafetyArmed = false;
	double LastRepairDropTime = -1e9;
	bool bWasEnabled = true;
};

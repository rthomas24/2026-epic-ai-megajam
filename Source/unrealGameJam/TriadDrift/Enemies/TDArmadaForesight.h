// Triad Drift - Enemies (Teammate 4). Story clue: now and then the Armada seems to know what the player will do.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDArmadaForesight.generated.h"

class ATDEnemyShip;
class ATDShip;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnArmadaAnticipated, ATDEnemyShip*, Enemy, FName, Kind);

/**
 * The paradox clue (storyline, the Lead's item 3c): from wave 3 on, rarely, an enemy pre-empts the player.
 * - "Switch": the moment the player switches ships, the nearest enemy is already on the new ship (nose and guns),
 *   no reaction time.
 * - "Dodge": the moment the player dodges, an enemy in range is already firing where the ship is going.
 * Rare and deniable: at most one tell per CooldownSeconds, Chance per opportunity. OnAnticipated fires each time
 * (T3's bark can notice it: "...it knew."). Nothing is said by this system itself.
 */
UCLASS()
class UNREALGAMEJAM_API UTDArmadaForesight : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDArmadaForesight* Get(const UObject* WorldContext);

	static const FName KindSwitch;
	static const FName KindDodge;

	/** An enemy pre-empted the player: Kind is "Switch" or "Dodge". */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Armada")
	FTDOnArmadaAnticipated OnAnticipated;

	/** First wave in which tells can happen. */
	int32 MinWave = 3;
	/** Chance per opportunity (a switch or a dodge). */
	float Chance = 0.25f;
	/** At least this long between two tells (s). */
	float CooldownSeconds = 40.f;
	/** Only enemies this close to the player's ship can pre-empt (cm). */
	float Range = 300000.f;
	/** At most one tell in a wave (the clue stays rare and deniable). */
	bool bOncePerWave = true;
	/** The visible tell: a small pale flash at the enemy's nose as it turns (cm; 0 = none). */
	float TellFlashRadius = 900.f;
	/** The flash is held this long (s). */
	float TellFlashSeconds = 0.4f;
	/** An enemy within this angle of the camera's forward counts as in view (deg). */
	float ViewConeHalfAngleDeg = 30.f;
	int32 GetTellsThisWave() const { return TellsThisWave; }

	int32 GetTellCount() const { return Tells; }
	int32 GetCurrentWave() const { return Wave; }

	/** Tests / dev: the wave number, as the wave director would announce it. */
	void SetWave(int32 InWave) { Wave = InWave; }

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTDArmadaForesight, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber, float Intensity01);
	UFUNCTION()
	void HandleShipSwitched(ETDShipId NewShip, ETDShipId OldShip);
	UFUNCTION()
	void HandleDodge(float Direction);

protected:
	bool Roll();
	ATDShip* GetFlownShip() const;
	ATDEnemyShip* PickEnemy(const FVector& Near, const FVector* Facing) const;
	void Tell(ATDEnemyShip* Enemy, const FVector& AimPoint, AActor* Target, FName Kind);

	int32 Wave = 0;
	int32 Tells = 0;
	int32 TellsThisWave = 0;
	TWeakObjectPtr<ATDEnemyShip> FlashEnemy;
	float FlashLeft = 0.f;
	float FlashTimer = 0.f;
	double LastTellTime = -1e9;
	float BindTimer = 0.f;
	FRandomStream Rng{ 0x5EED };
	TArray<TWeakObjectPtr<class UTDShipMovementComponent>> Bound;
};

// Triad Drift - Enemies & Waves (Teammate 4). ENG-27 balance run: event recorder + scripted pilot.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDBalanceRecorder.generated.h"

class ATDSquadManager;
class UTDHealthComponent;

/** Numbers for one wave of a balance run. */
USTRUCT()
struct UNREALGAMEJAM_API FTDBalanceWaveStats
{
	GENERATED_BODY()

	int32 WaveNumber = 0;
	float Seconds = 0.f;
	int32 Takes = 1;
	bool bCleared = false;
	int32 Enemies = 0;
	int32 Kills = 0;
	float DamageTaken = 0.f;		// shield + hull, all squad ships
	int32 ShipDowns = 0;
	TMap<ETDShipId, int32> KillsByShip;
	TMap<ETDDamageKind, int32> KillsByKind;
};

/**
 * Listens to the event bus and the squad's health while a balance run plays: kills (by ship and by
 * damage kind), damage taken, ship downs and squad wipes. Pure observer; changes nothing.
 */
UCLASS(Transient)
class UNREALGAMEJAM_API UTDBalanceRecorder : public UObject
{
	GENERATED_BODY()

public:
	void Begin(UObject* WorldContext);
	void End();

	/** (Re)bind health listeners on the squad's ships (call after SpawnSquad / respawns). */
	void WatchSquad(ATDSquadManager* Squad);

	FTDBalanceWaveStats Current;
	int32 Wipes = 0;

	UFUNCTION() void HandleKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);
	UFUNCTION() void HandleShipDowned(ETDShipId Ship);
	UFUNCTION() void HandleSquadWiped();
	UFUNCTION() void HandleDamaged(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);

private:
	TWeakObjectPtr<UObject> Context;
	TArray<TWeakObjectPtr<UTDHealthComponent>> Watched;
};

/**
 * A simple aim-and-fire bot that flies the player's ship through the same gameplay calls the
 * player controller makes (SetFlightInput on the possessed ship, the weapon component's triggers,
 * squad switching). Wingmen are T3's real AI. No cheats, no powers.
 */
struct UNREALGAMEJAM_API FTDBalancePilot
{
	/** Seconds between ship switches (exercises the switching loop). */
	float SwitchEverySeconds = 25.f;
	/** Fires the primary when the target is within this cone (deg) and range (cm). */
	float FireConeDeg = 8.f;
	float FireRange = 25000.f;
	/** Secondary FireOnce every this many seconds when lined up. */
	float SecondaryEverySeconds = 2.5f;

	void Tick(UWorld* World, ATDSquadManager* Squad, float DeltaSeconds);

private:
	float SwitchTimer = 0.f;
	float SecondaryTimer = 0.f;
	TWeakObjectPtr<AActor> LastShip;
};

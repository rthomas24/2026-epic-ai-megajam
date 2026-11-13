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
	/** Downs of the ship the player was flying (the rest are wingmen). */
	int32 ActiveShipDowns = 0;
	TMap<ETDShipId, int32> KillsByShip;
	TMap<ETDDamageKind, int32> KillsByKind;

	// --- diagnostics (why is a wave slow / lost?) ---
	/** Damage taken by the squad, by what dealt it: enemy id, "Turret", "Collision/Other". */
	TMap<FName, float> DamageBySource;
	int32 BaysKilled = 0;
	int32 TurretsKilled = 0;
	int32 CarriersKilled = 0;
	/** Seconds the player's pilot spent on each kind of target. */
	float SecondsOnBays = 0.f;
	float SecondsOnShips = 0.f;
	float SecondsNoTarget = 0.f;
	int32 PowersUsed = 0;
	/** Seconds with at least one enemy ship inside the player's view (chase-camera frustum: 45 deg either side,
	 *  28 deg up/down, within 30 000 cm). */
	float SecondsEnemyOnScreen = 0.f;
	/** Integral of (enemies on screen) over time: / SecondsTotal = average count on screen. */
	float EnemyOnScreenCountSeconds = 0.f;
	/** Longest stretch with no enemy on screen while enemies were alive (s), and the current one. */
	float LongestEmptySeconds = 0.f;
	float CurrentEmptySeconds = 0.f;
	/** Seconds from the wave start to the first enemy on screen (-1 = never). */
	float FirstContactSeconds = -1.f;
	float SecondsTotal = 0.f;
	/** Hull lost by the ship the player was flying when hit, as a fraction of that ship's max hull (sum). */
	float ActiveHullLost01 = 0.f;
	int32 Switches = 0;
};

/** How well the scripted player flies. */
USTRUCT()
struct UNREALGAMEJAM_API FTDPilotSkill
{
	GENERATED_BODY()

	FName Name = TEXT("Average");
	/** Aim error cone (deg) re-rolled every half second. */
	float AimErrorDeg = 3.f;
	/** Seconds before the pilot reacts to a new best target. */
	float ReactionSeconds = 0.4f;
	/** Fires the primary inside this cone (deg). */
	float FireConeDeg = 8.f;
	/** Ship switch every N s (0 = never). */
	float SwitchEverySeconds = 25.f;
	/** Signature power every N s when a target is close (0 = never). */
	float PowerEverySeconds = 12.f;
	/** Whether the pilot goes for carrier bays from stand-off range (a player who has learned the carrier). */
	bool bHuntsBays = true;
	/** Passive: never steers or fires (the worst case for readability: a player who isn't turning). */
	bool bPassive = false;
	/** Drifter (T2's scripted run): cruises, fire held in bursts (2 s on, 1 s off), a 35 degree turn every 11 s,
	 *  no aiming, no evasion. The lower bound: a player who is often not where the fight is. */
	bool bDrifter = false;
	/** Evader (the reference player): aims and fights like Average, but weaves, and on a fresh hit (or an enemy
	 *  closing head-on inside 3000 cm) breaks hard to one side with a boost for 1 s. */
	bool bEvader = false;

	static FTDPilotSkill Weak();
	static FTDPilotSkill Average() { return FTDPilotSkill(); }
	static FTDPilotSkill Strong();
	static FTDPilotSkill Passive();
	static FTDPilotSkill Drifter();
	static FTDPilotSkill Evader();
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
	/** The ship the player flew last frame (set by the run loop): a down event fires after the auto-switch. */
	ETDShipId LastActiveShip = ETDShipId::Rift;

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
	FTDPilotSkill Skill;

	/** Distribution runs: a different (still deterministic) pilot per seed. */
	void Reseed(int32 Seed) { Rng.Initialize(Seed); }

	// Drifter / Evader state.
	float StyleTimer = 0.f;
	float TurnRemaining = 0.f;
	float TurnSign = 1.f;
	float DodgeTimer = -1.f;
	float DodgeSign = 1.f;
	float WeavePhase = 0.f;
	float LastHealthSum = -1.f;
	TWeakObjectPtr<AActor> LastShip;
	/** Fires the primary when the target is within the skill's cone and this range (cm). */
	float FireRange = 25000.f;
	/** Secondary FireOnce every this many seconds when lined up. */
	float SecondaryEverySeconds = 2.5f;
	/** Stay this far from carrier bays (outside the 8000 cm point defense). */
	float StandOffDistance = 11000.f;

	/** Stats are written into Stats (the recorder's current wave). */
	void Tick(UWorld* World, ATDSquadManager* Squad, float DeltaSeconds, FTDBalanceWaveStats& Stats);

private:
	AActor* PickTarget(UWorld* World, const AActor* Ship) const;

	float SwitchTimer = 0.f;
	float SecondaryTimer = 0.f;
	float PowerTimer = 0.f;
	float ReactionTimer = 0.f;
	float AimRerollTimer = 0.f;
	FVector AimJitter = FVector::ZeroVector;
	TWeakObjectPtr<AActor> Target;
	FRandomStream Rng{ 2026 };
};

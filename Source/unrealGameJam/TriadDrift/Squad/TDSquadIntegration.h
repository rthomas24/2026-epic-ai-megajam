// Triad Drift - Squad (Teammate 3).
// The ONLY place Squad code calls into other areas (Core, T1 Flight, T2 Combat/Powers, T4 Waves).
// Squad logic is written against these functions so a cross-area signature change is a one-file fix.
// Every function is null-safe and degrades to a harmless default.

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"

class AActor;
class APawn;
class ATDShip;
class ATDSquadManager;
class ATDWingmanAIController;
class UWorld;
struct FTDShipSpec;
struct FTDFXCue;
class UTDShipVisuals;

namespace TDSquad
{
	// ---- Ship setup (T1 ATDShip / Core health / T2 weapons) ----
	void ApplySpec(ATDShip* Ship, const FTDShipSpec& Spec);
	void SetPlayerControlled(ATDShip* Ship, bool bPlayerControlled);

	// ---- Flight (T1) ----
	FVector GetVelocity(const ATDShip* Ship);
	void SetVelocity(ATDShip* Ship, const FVector& Velocity);
	float GetSpeed(const ATDShip* Ship);
	/**
	 * Pitch/Yaw/Roll in [-1,1] (positive = nose up / nose right / roll right). TargetSpeed in cm/s is
	 * turned into throttle (and boost when above the throttle-up speed) from the ship's own tuning.
	 */
	void SendFlightInput(ATDShip* Ship, float Pitch, float Yaw, float Roll, float TargetSpeed, bool bForceBoost);
	void GetSpeedRange(const ATDShip* Ship, float& OutMin, float& OutCruise, float& OutBoost);
	/** T1 throttle top speed above cruise (formation catch-up). <= cruise (or 0) clears it. */
	void SetCatchUpTopSpeed(ATDShip* Ship, float TopSpeed);
	void TeleportShip(ATDShip* Ship, const FVector& Location, const FRotator& Facing, bool bKeepSpeed);
	/** M7: these two ships never block each other when moving. */
	void IgnoreEachOther(ATDShip* A, ATDShip* B);
	/** M7: Desired if it's clear of registered obstacles and blocking geometry, else the nearest clear
	 *  point found by stepping up and back (respawns, warps). */
	FVector FindSafeLocation(const UObject* WorldContext, const FVector& Desired, const AActor* Ignore);
	/** Radius of an actor's collision bounds (break-off distance against big targets). */
	float GetBoundsRadius(const AActor* Actor);
	/** T1: stop / resume the flight simulation (downed ships, landing). Parked = zero velocity, no sim. */
	void SetFlightParked(ATDShip* Ship, bool bParked);
	bool IsFlightParked(const ATDShip* Ship);
	/** T1: clear latched input, boost, drift, overrides, angular rates (respawn, retake). */
	void ResetFlightState(ATDShip* Ship, bool bKeepVelocity);
	/** Neutral stick, no boost (latched until replaced). */
	void SendNeutralInput(ATDShip* Ship);

	// ---- Travel (T1 hyperdrive, landing) ----
	ETDHyperdriveState GetHyperdriveState(const ATDShip* Ship);
	bool IsLandingBusy(const ATDShip* Ship);
	/** T1: is a manual switch away from this ship locked (hyperdrive, landing, atmospheric entry)? */
	bool IsSwitchLocked(const ATDShip* Ship, FText& OutReason);
	/** T1: retake: hyperdrive instantly Idle, landing instantly back to flying. */
	void ResetTravelForRetake(ATDShip* Ship);
	/** T1: landing instantly back to flying (no-op when already flying). */
	void AbortLanding(ATDShip* Ship);
	float GetHyperdriveCharge01(const ATDShip* Ship);
	/** Why the hyperdrive can't spool right now (ship block reason, else a world travel lock), or empty. */
	FText GetHyperdriveBlockText(const ATDShip* Ship);
	float GetAltitudeMeters(const ATDShip* Ship);
	/** T2: carry a manually cycled target from one ship to the next on a switch. */
	void CarryManualTarget(const ATDShip* From, ATDShip* To);

	/** The ship is going down: no spool, and a jump in progress drops out. */
	void StopHyperdrive(ATDShip* Ship);
	/** Back in the fight (respawn, retake): hyperdrive usable again. */
	void EnableHyperdrive(ATDShip* Ship);

	// ---- Health (Core UTDHealthComponent) ----
	bool IsDead(const AActor* Actor);
	/** (hull + shield) / (max hull + max shield), 0 when dead or no health component. */
	float GetHealth01(const AActor* Actor);
	float GetHull01(const AActor* Actor);
	float GetShield01(const AActor* Actor);
	void SetInvulnerable(AActor* Actor, float Seconds);
	void RepairHull(AActor* Actor, float Amount);
	/** Full hull and shield, alive again. */
	void RestoreFullHealth(AActor* Actor);

	// ---- Weapons (T2 UTDWeaponComponent) ----
	void SetTriggerHeld(ATDShip* Ship, ETDWeaponSlot Slot, bool bHeld);
	void SetAim(ATDShip* Ship, AActor* Target, const FVector& WorldDir);
	/** Drop any explicit aim target / direction (hand-over to the player, downed, passive). */
	void ClearAim(ATDShip* Ship);
	/** T2: keyed weapon block (a downed ship can't fire). */
	void SetWeaponsBlocked(ATDShip* Ship, FName SourceKey, bool bBlocked);
	bool IsOverheated(const ATDShip* Ship, ETDWeaponSlot Slot);
	float GetHeat01(const ATDShip* Ship, ETDWeaponSlot Slot);
	int32 GetAmmo(const ATDShip* Ship, ETDWeaponSlot Slot);
	/** Typical projectile speed of the primary, for lead prediction. */
	float GetPrimaryProjectileSpeed(const ATDShip* Ship);
	/** T2: UTDWeaponComponent::SetOutgoingDamageMultiplier / Clear (keyed, multiplicative). */
	void SetOutgoingDamageMultiplier(ATDShip* Ship, FName SourceKey, float Multiplier);
	void ClearOutgoingDamageMultiplier(ATDShip* Ship, FName SourceKey);

	// ---- Awareness (Core combat registry, T2 projectiles/powers) ----
	/** Live hostiles of the player team. OutHostiles is reset (keeps its allocation). */
	void GetPlayerHostiles(const UWorld* World, TArray<AActor*>& OutHostiles);
	/** The last actor that damaged Actor (Core UTDHealthComponent::GetLastAttacker). */
	AActor* GetLastAttacker(const AActor* Actor);
	float GetTimeSinceLastDamage(const AActor* Actor);
	/**
	 * T4 obstacle registry (asteroids, ruins, planets, Dominion hull): if something registered lies
	 * within ProbeDistance along Direction, returns true and a unit direction that slides around it.
	 */
	/** Wingmen never transit a permanent gate on their own (they lead the player in the forward wedge
	 *  and would arrive alone): if Ship's next LookAheadSeconds of flight crosses an active gate disc,
	 *  hold it in the transit cooldown so it flies through the frame; the leader's transit follow-warps
	 *  it across. Returns true while held. */
	bool HoldOutOfGates(const AActor* Ship, float LookAheadSeconds);
	/** HUD: T1's landing prompt for this ship, empty when none. */
	FText GetLandPrompt(const ATDShip* Ship);
	/** Where the director's money-shot camera is, while one is running. */
	bool GetDirectorShotCamera(const AActor* WorldContext, FVector& OutLocation);
	bool FindRegisteredAvoidance(const UWorld* World, const FVector& Origin, const FVector& Direction, float ProbeDistance, float ShipRadius, FVector& OutAvoidDirection);
	/** True if a hostile homing projectile is tracking Ship. */
	bool IsMissileIncoming(const ATDShip* Ship);
	/** Nearest active player shield dome (Bastion, ENG-11) within MaxDistance of Location. */
	bool GetNearbyShieldDome(const UWorld* World, const FVector& Location, float MaxDistance, FVector& OutCenter, float& OutRadius);

	/** HUD: T2 signature power cooldowns, 1 = ready (1 when the ship has no such power). */
	void GetPowerCooldowns(const ATDShip* Ship, float& OutPower1, float& OutPower2);
	/** HUD (N-004 contract): the ability the ship has EARNED (None while locked) and its readiness
	 *  (0 while locked, never 1 for an absent power; else the ability power's cooldown, 1 = ready). */
	void GetEarnedAbility(const ATDShip* Ship, ETDAbilityId& OutAbility, float& OutReadiness01);

	/** T2 powers: install the spec's signature powers and ability slot once (no-op for slots already filled). */
	void InstallPowers(ATDShip* Ship, const FTDShipSpec& Spec);
	/** T2 powers: every power on the ship back to ready (respawn, retake). Never re-creates them. */
	void ResetPowers(ATDShip* Ship);
	/** T2: refill ammo, clear heat and charge (respawn, retake). */
	void ResetWeapons(ATDShip* Ship);
	/** A ship leaves the player's hands: release any held power input and drop a charging shot (m15). */
	void ReleasePlayerHolds(ATDShip* Ship);

	// ---- HUD (T1 flight state, T2 targeting, T4 waves) ----
	float GetBoost01(const ATDShip* Ship);
	float GetSpeed01(const ATDShip* Ship);
	bool IsOutOfBounds(const ATDShip* Ship);
	AActor* GetCurrentTarget(const ATDShip* Ship);
	void GetLockedTargets(const ATDShip* Ship, TArray<AActor*>& Out);
	void GetTargetMarkers(const ATDShip* Ship, TArray<FTDTargetMarker>& Out);
	ETDMissileWarning GetMissileWarning(const ATDShip* Ship);
	bool GetLeadIndicator(const ATDShip* Ship, FVector& OutWorldLocation);
	/** Cached wave-director lookup: ATDWaveDirector::Get iterates actors, so the HUD path must not call it every frame. */
	struct FWaveDirectorCache
	{
		TWeakObjectPtr<AActor> Director;
		double NextLookupTime = -1.0;
	};
	/** Misses are retried at most every RetrySeconds (world real time). */
	FTDWaveInfo GetWaveInfo(const UObject* WorldContext, FWaveDirectorCache& Cache, double RetrySeconds = 1.0);

	// ---- Events & flow (Core event bus, T5 game flow) ----
	void BroadcastShipSwitched(const UObject* WorldContext, ETDShipId NewShip, ETDShipId OldShip);
	void BroadcastShipDowned(const UObject* WorldContext, ETDShipId Ship);
	void BroadcastShipRespawned(const UObject* WorldContext, ETDShipId Ship);
	void BroadcastSquadWiped(const UObject* WorldContext);
	/** True if a game flow received the defeat (it then owns the restart). */
	bool ReportPlayerDefeated(const UObject* WorldContext);
	void AddShipSwitchStat(const UObject* WorldContext);
	/** A voice line: ITDGameFlow::RequestBark, or straight to UTDEventBus::OnBarkRequested with no flow. */
	void RequestBark(const UObject* WorldContext, FName BarkId, FName Speaker);
	/** Subscribe / unsubscribe Squad->HandleActorKilled to UTDEventBus::OnActorKilled (wingman kill barks). */
	void BindActorKilled(ATDSquadManager* Squad);
	void UnbindActorKilled(ATDSquadManager* Squad);

	/** Subscribe / unsubscribe Squad->HandlePowerUsed to UTDEventBus::OnPowerUsed (T2 broadcasts it). */
	void BindPowerUsed(ATDSquadManager* Squad);
	void UnbindPowerUsed(ATDSquadManager* Squad);

	/** Subscribe / unsubscribe Wingman->HandleSpacecraftTeleported to UTDEventBus::OnSpacecraftTeleported. */
	void BindTeleported(ATDWingmanAIController* Wingman);
	void UnbindTeleported(ATDWingmanAIController* Wingman);

	/** Subscribe / unsubscribe Squad->HandleWaveCleared / HandleWaveStarted (between-wave hull repair). */
	void BindWaveEvents(ATDSquadManager* Squad);
	void UnbindWaveEvents(ATDSquadManager* Squad);

	/** Presentation (Nick's UTDShipVisuals): null-safe; an empty cue does nothing. */
	void PlayShipCue(const ATDShip* Ship, const FTDFXCue& Cue, bool bAttached);
	/** The ship's own visuals asset (T1 ATDShip::GetShipVisuals), or null. */
	const UTDShipVisuals* GetShipVisuals(const ATDShip* Ship);


	/** World transform of a ship's chase camera (actor transform if it has none). */
	FTransform GetChaseCameraTransform(const ATDShip* Ship);
}

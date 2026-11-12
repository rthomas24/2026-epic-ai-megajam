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

	// ---- Health (Core UTDHealthComponent) ----
	bool IsDead(const AActor* Actor);
	/** (hull + shield) / (max hull + max shield), 0 when dead or no health component. */
	float GetHealth01(const AActor* Actor);
	float GetHull01(const AActor* Actor);
	float GetShield01(const AActor* Actor);
	void SetInvulnerable(AActor* Actor, float Seconds);
	/** Full hull and shield, alive again. */
	void RestoreFullHealth(AActor* Actor);

	// ---- Weapons (T2 UTDWeaponComponent) ----
	void SetTriggerHeld(ATDShip* Ship, ETDWeaponSlot Slot, bool bHeld);
	void SetAim(ATDShip* Ship, AActor* Target, const FVector& WorldDir);
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
	/** True if a hostile homing projectile is tracking Ship. */
	bool IsMissileIncoming(const ATDShip* Ship);
	/** Nearest active player shield dome (Bastion, ENG-11) within MaxDistance of Location. */
	bool GetNearbyShieldDome(const UWorld* World, const FVector& Location, float MaxDistance, FVector& OutCenter, float& OutRadius);

	/** HUD: T2 power cooldowns, 1 = ready (also 1 when the ship has no such power). */
	void GetPowerCooldowns(const ATDShip* Ship, float& OutPower1, float& OutPower2, float& OutAbility);

	// ---- HUD (T1 flight state, T2 targeting, T4 waves) ----
	float GetBoost01(const ATDShip* Ship);
	float GetSpeed01(const ATDShip* Ship);
	bool IsOutOfBounds(const ATDShip* Ship);
	AActor* GetCurrentTarget(const ATDShip* Ship);
	void GetLockedTargets(const ATDShip* Ship, TArray<AActor*>& Out);
	FTDWaveInfo GetWaveInfo(const UObject* WorldContext);

	// ---- Events & flow (Core event bus, T5 game flow) ----
	void BroadcastShipSwitched(const UObject* WorldContext, ETDShipId NewShip, ETDShipId OldShip);
	void BroadcastShipDowned(const UObject* WorldContext, ETDShipId Ship);
	void BroadcastShipRespawned(const UObject* WorldContext, ETDShipId Ship);
	void BroadcastSquadWiped(const UObject* WorldContext);
	/** True if a game flow received the defeat (it then owns the restart). */
	bool ReportPlayerDefeated(const UObject* WorldContext);
	void AddShipSwitchStat(const UObject* WorldContext);

	/** Subscribe / unsubscribe Squad->HandlePowerUsed to UTDEventBus::OnPowerUsed (T2 broadcasts it). */
	void BindPowerUsed(ATDSquadManager* Squad);
	void UnbindPowerUsed(ATDSquadManager* Squad);

	/** Subscribe / unsubscribe Wingman->HandleSpacecraftTeleported to UTDEventBus::OnSpacecraftTeleported. */
	void BindTeleported(ATDWingmanAIController* Wingman);
	void UnbindTeleported(ATDWingmanAIController* Wingman);

	/** Subscribe / unsubscribe Wingman->HandleHyperdriveState to UTDEventBus::OnHyperdriveState. */
	void BindHyperdriveState(ATDWingmanAIController* Wingman);
	void UnbindHyperdriveState(ATDWingmanAIController* Wingman);

	/** World transform of a ship's chase camera (actor transform if it has none). */
	FTransform GetChaseCameraTransform(const ATDShip* Ship);
}

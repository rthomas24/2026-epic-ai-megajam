// Triad Drift - player aim assist (issues #192 / #201; Teammate 2).
//
// One place for the player's aim-assist tuning: the soft lock (UTDTargetingComponent), muzzle convergence on the lock's
// lead point (UTDWeaponComponent, all three ships) and gentle in-flight homing on the lock (ATDProjectile, the Rift's and
// the Lancer's regular shots only). Wingmen and powers are untouched. Every number is a td.Aim.* cvar; the strength scales
// up on a pad and on easier difficulties (Story strongest). Never a full lock: the cone, the convergence and the bend are
// all capped. T1 owns the ship's turning (the soft nose pull reads the same lock).

#pragma once

#include "CoreMinimal.h"

class AActor;

struct UNREALGAMEJAM_API FTDAimAssist
{
	/** td.Aim.Enabled and TD.Weapons.AimAssist both on. */
	static bool IsEnabled();
	/** The owner is a squad ship flown by the player (assist applies only then). */
	static bool IsPlayerShip(const AActor* Ship);
	/** Pad / mouse scale times the difficulty scale (>= 0). */
	static float Strength(const AActor* Ship);
	/** Soft-lock cone (half-angle, degrees) for the player's ship, capped. */
	static float LockConeDeg(const AActor* Ship);
	/** Extra degrees the lock stays inside before it starts to age out. */
	static float LockKeepMarginDeg();
	/** Seconds the lock holds after the reticle leaves its keep cone. */
	static float StickySeconds();
	/** Muzzle convergence cone (degrees): a shot whose lead point is inside it leaves toward the lead. */
	static float ConvergeDeg(const AActor* Ship);
	/** Multiplier on a weapon's homing turn rate, and the cap on a shot's total bend (degrees). */
	static float HomingTurnScale(const AActor* Ship);
	static float HomingMaxTurnDeg();
};

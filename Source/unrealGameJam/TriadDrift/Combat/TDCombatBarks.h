// Triad Drift - Combat (Teammate 2). The combat and power bark triggers T2 owns in dialogs.md (sections 3 and 4).
//
// Every trigger asks once per condition (an edge: the missile launched at you, the heat reaching full, ammo crossing
// 25%, a cooldown ending...), never every frame. The line itself goes through T3's ATDSquadManager::RequestSquadBark,
// whose limiter decides whether it plays (global gap, per-id cooldown, priority). The words are dialogs.md's, from
// T3's FTDBarkLines (Nicholas's DT_Barks wins row by row).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Squad/TDSquadBarks.h"

/** The ids T2 fires (dialogs.md; change an id only with T3). */
namespace TDBarkIdsT2
{
	inline const FName MissileIncoming(TEXT("Bark_MissileIncoming"));	// a wingman, Critical
	inline const FName TargetShielded(TEXT("Bark_TargetShielded"));	// Kato or a wingman, Normal
	inline const FName AmmoLow(TEXT("Bark_AmmoLow"));					// flown pilot, Normal
	inline const FName AmmoOut(TEXT("Bark_AmmoOut"));					// flown pilot, High
	inline const FName WeaponHot(TEXT("Bark_WeaponHot"));				// flown pilot, Normal
	inline const FName PowerReady(TEXT("Bark_PowerReady"));			// flown pilot, Flavour, 1 in 2
	inline const FName PowerNotReady(TEXT("Bark_PowerNotReady"));		// flown pilot, Normal
	inline const FName AbilityLocked(TEXT("Bark_AbilityLocked"));		// flown pilot, Normal
	inline const FName PortalOpen(TEXT("Bark_Portal_Open"));			// Wren, Normal
	inline const FName PortalThrough(TEXT("Bark_Portal_Through"));		// Wren, Flavour
	inline const FName PortalMissileReturn(TEXT("Bark_Portal_MissileReturn")); // Wren, High
	inline const FName RamGo(TEXT("Bark_Ram_Go"));						// Kato, Normal
	inline const FName RamKill(TEXT("Bark_Ram_Kill"));					// Kato, High
	inline const FName SwarmFire(TEXT("Bark_Swarm_Fire"));				// Moss, Normal
	inline const FName DomeUp(TEXT("Bark_Dome_Up"));					// Moss, Normal
	inline const FName DomeSaved(TEXT("Bark_Dome_Saved"));				// Moss, Normal
	inline const FName CloakOn(TEXT("Bark_Cloak_On"));					// Wren, Normal
	inline const FName CloakAmbush(TEXT("Bark_Cloak_Ambush"));			// Wren, High
	inline const FName EMPFire(TEXT("Bark_EMP_Fire"));					// Kato, Normal
	inline const FName EMPHit(TEXT("Bark_EMP_Hit"));					// Kato, High (3+ stunned)
	inline const FName GravityFire(TEXT("Bark_Gravity_Fire"));			// Moss, Normal
	inline const FName GravityCaught(TEXT("Bark_Gravity_Caught"));		// Moss, High (3+ caught)
}

struct UNREALGAMEJAM_API FTDCombatBarks
{
	/** Speaker says BarkId through the squad's limiter. True if it played. Always counted (GetRequestCount). */
	static bool Say(const UObject* WorldContext, ETDShipId Speaker, FName BarkId, ETDBarkPriority Priority);

	/** Ship's own pilot says it, only while the player flies Ship (the player's own actions). */
	static bool SayAsPilot(const AActor* Ship, FName BarkId, ETDBarkPriority Priority);

	/** A living wingman (not the flown ship) says it; Preferred first when it is one. False when nobody can. */
	static bool SayAsWingman(const UObject* WorldContext, FName BarkId, ETDBarkPriority Priority, int32 Preferred = INDEX_NONE);

	/** True while the player flies Actor (an ATDShip the player controls). */
	static bool IsPlayerFlown(const AActor* Actor);

	/** The ship id of a squad ship (false for anything else). */
	static bool GetShipId(const AActor* Actor, ETDShipId& OutShip);

	/** Tests and diagnostics: how many times BarkId was asked for (played or held), since the last reset. */
	static int32 GetRequestCount(FName BarkId);
	static void ResetRequestCounts();
};

// Triad Drift - Squad. Shadow mode (issue #201): every tunable in one place, as td.Squad.* console variables.
//
// Shadow ON  (default): the two wingmen shadow the player's ship. They fight inside a leash round the player and
//                       take the player's attackers and the player's own target first.
// Shadow OFF          : they hunt on their own, about three times further out (hard-capped), pick their own
//                       targets, hit harder and aim tighter. The price is that nobody covers the player.
//
// The state lives on ATDSquadManager (IsShadowMode / SetShadowMode / ToggleShadowMode / OnShadowModeChanged).

#pragma once

#include "CoreMinimal.h"

namespace TDShadow
{
	/** Shadow ON: a wingman further than this from the player's ship breaks off and comes back (cm). */
	UNREALGAMEJAM_API float ShadowLeash();
	/** Shadow ON: only enemies within this distance of the player's ship are picked (cm). */
	UNREALGAMEJAM_API float ShadowEngageRange();
	/** Shadow ON, "your fight": wingmen never finish an enemy the player hit, targeted or locked in the last YourFightSeconds;
	 *  their damage leaves it YourFightFloor01 of its max hull, and after YourFightRetargetSeconds at that floor they pick another. */
	UNREALGAMEJAM_API bool YourFightEnabled();
	UNREALGAMEJAM_API float YourFightSeconds();
	UNREALGAMEJAM_API float YourFightFloor01();
	UNREALGAMEJAM_API float YourFightRetargetSeconds();
	/** Shadow ON: through wave EarlyWaves the wingmen do EarlyDamageMultiplier damage and prefer enemies the player is not on. */
	UNREALGAMEJAM_API int32 EarlyWaves();
	UNREALGAMEJAM_API float EarlyDamageMultiplier();
	/** Early waves: the wingmen's kills never exceed the player's, unless the player has hit nothing for this long (s). */
	UNREALGAMEJAM_API float EarlyIdleSeconds();
	/** Shadow OFF: a wingman prefers targets at least this far from the player's ship (cm). */
	UNREALGAMEJAM_API float SoloMinSeparation();
	/** Shadow ON: a wingman in a chase drops its target and returns at ShadowLeash x this. */
	UNREALGAMEJAM_API float ShadowBreakOffScale();
	/** Shadow OFF: the leash (cm): ShadowLeash x td.Squad.Solo.LeashScale, never above the hard cap or the radar's edge. */
	UNREALGAMEJAM_API float SoloLeash();
	/** Shadow OFF: a wingman looks for its own targets within this distance of ITSELF (cm), inside the solo leash. */
	UNREALGAMEJAM_API float SoloHuntRange();
	/** Shadow OFF: outgoing weapon damage multiplier for the two wingmen. */
	UNREALGAMEJAM_API float SoloDamageMultiplier();
	/** Shadow OFF: the wingman's aim error is multiplied by this (below 1 = tighter). */
	UNREALGAMEJAM_API float SoloAimErrorScale();
	/** Shadow OFF: seconds between secondary shots are multiplied by this (below 1 = more often). */
	UNREALGAMEJAM_API float SoloSecondaryIntervalScale();

	/** Wingmen use their signature powers and unlocked abilities (td.Squad.Powers). */
	UNREALGAMEJAM_API bool PowersEnabled();
	/** A wingman waits cooldown x this before using the same power again. */
	UNREALGAMEJAM_API float PowerCooldownPad();
	/** Seconds between any two wingman power uses, across the squad. */
	UNREALGAMEJAM_API float PowerGlobalGap();
	/** Enemies needed in reach for an EMP, a missile swarm at a cluster, or a gravity well. */
	UNREALGAMEJAM_API int32 ClusterCount();
	/** A single target with at least this much max hull is "tough": worth a missile swarm on its own. */
	UNREALGAMEJAM_API float ToughHull();
	/** Shadow ON: false (default) = offensive wingman powers only on the player's target or the player's attackers. */
	UNREALGAMEJAM_API bool ShadowOffensivePowersAnywhere();
	/** td.Squad.LogTargets: log wingman target changes with their reason, and leash breaks. */
	UNREALGAMEJAM_API bool LogTargets();
	/** Rift's wingman may place a portal pair in front of a missile that is homing on it (td.Squad.Powers.Portals). */
	UNREALGAMEJAM_API bool PortalsEnabled();
}

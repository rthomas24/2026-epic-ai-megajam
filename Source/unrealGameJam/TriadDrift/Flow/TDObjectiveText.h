// Triad Drift - the objective line for scenes the flow owns (waves, travel beats, the Dominion, victory).
// Owner: Teammate 5. On-foot objective steps are broadcast by ATDObjective; this never talks over them.

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"

class UWorld;

/** One objective line as the HUD shows it (bus OnObjectiveUpdated). */
struct UNREALGAMEJAM_API FTDObjectiveLine
{
	FName Id;
	int32 Step = 0;
	FText Text;

	bool IsSet() const { return !Id.IsNone(); }
	bool operator==(const FTDObjectiveLine& O) const { return Id == O.Id && Step == O.Step && Text.EqualTo(O.Text); }
	bool operator!=(const FTDObjectiveLine& O) const { return !(*this == O); }
};

/**
 * Computes the current objective line from progress and the live world:
 *  - waves: "WAVE 3 · CLEAR THE NULL ARMADA", between waves "WAVE 4 INCOMING"
 *  - travel: "JUMP TO VERDANCE", on the planet "LAND AT THE BEACON", then "RETURN TO YOUR SHIP" and "TAKE OFF · CLIMB TO ORBIT"
 *  - Dominion: "DESTROY THE SHIELD GENERATORS 2 / 4" (Step = generators down), "FLY THE TRENCH · DESTROY THE REACTOR"
 *  - victory: "VICTORY · THAT'S A WRAP"
 * Returns false when the flow should stay silent: menus, and on foot while a planet objective is running.
 */
struct UNREALGAMEJAM_API FTDObjectiveText
{
	static bool Compute(UWorld* World, const FTDProgress& Progress, ETDDestination Destination, FTDObjectiveLine& Out);

	/**
	 * The actors the flow tags TDTags::ObjectiveMarker right now (HUD direction markers):
	 *  - shields up: every live generator; reactor exposed: the reactor
	 *  - on foot: the running objective's waypoints; both objectives done: the board-ship point (take-off)
	 *  - waves: none (T3 falls back to the nearest enemy). Planet entries / gates are tagged by T4's hook and
	 *    the landing pad by T1, not here.
	 */
	static void ComputeMarkers(UWorld* World, const FTDProgress& Progress, ETDDestination Destination, TArray<AActor*>& Out);
};

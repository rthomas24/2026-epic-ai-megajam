// Triad Drift - journey stages for cheats, playtests and the F10 menu. Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDJourney.generated.h"

/** Every stop on the player journey (features.md §0), in order. */
UENUM(BlueprintType)
enum class ETDJourneyStage : uint8
{
	Waves1to2,
	VerdanceArrival,   // descending onto Verdance
	VerdanceOnFoot,    // landed at the pad, pilot on foot
	Waves3to4,         // back in the galaxy from Verdance orbit
	CinderArrival,
	CinderOnFoot,
	Wave5,             // back from Cinder: the Dominion's guard
	DominionAssault,   // Dominion revealed, shields up, squad at the approach
	ReactorExposed,    // all four generators down
	Victory
};

/**
 * Puts the run AND the world into a journey stage, consistently:
 * - Progress is rebuilt: phase, waves cleared, completed objectives, unlocked abilities on the right ships,
 *   Dominion flags and the return point. Takes, stats and run time are kept.
 * - A galaxy stage while in the galaxy is applied in place:
 *   - the wave director is cleared and restarted from progress
 *   - the Dominion syncs (warps in / drops its shields)
 *   - the squad is respawned at the stage's return point
 * - Any other stage travels to its destination with that return point (on foot = "<Planet>_Pad").
 */
struct UNREALGAMEJAM_API FTDJourney
{
	static FTDProgress MakeProgress(ETDJourneyStage Stage, const FTDProgress& Keep);
	static ETDDestination DestinationOf(ETDJourneyStage Stage);
	static FName ReturnPointOf(ETDJourneyStage Stage);

	/** Stage start points computed from the Dominion itself (not layout return points). */
	static const FName DominionApproach;	// front-right-above, outside the shield, the whole hull in frame
	static const FName DominionTrench;		// lined up with the trench, the reactor dead ahead

	/**
	 * Where the squad starts for a return point: the two Dominion points above are framed from the live
	 * Dominion; anything else is the galaxy layout's return point. False if neither resolves.
	 */
	static bool FindStageStart(UWorld* World, FName ReturnPoint, FTransform& Out);

	/** False if there is no TD game instance, or the travel could not start (missing map). */
	static bool Go(UWorld* World, ETDJourneyStage Stage);

	static bool Parse(const FString& Name, ETDJourneyStage& Out);
	static FString ListStages();

private:
	static void ApplyGalaxyInPlace(UWorld* World, ETDJourneyStage Stage);
};

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

	/** False if there is no TD game instance, or the travel could not start (missing map). */
	static bool Go(UWorld* World, ETDJourneyStage Stage);

	static bool Parse(const FString& Name, ETDJourneyStage& Out);
	static FString ListStages();

private:
	static void ApplyGalaxyInPlace(UWorld* World, ETDJourneyStage Stage);
};

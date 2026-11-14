// Triad Drift - the combat warm-up list (Teammate 2, issue #89). T7's UTDWarmupSubsystem loads and holds everything a
// map needs behind the loading screen (and walks every DA_* itself); this adds what no data asset names: the combat
// classes (their CDOs pull in their constructor assets) and the few assets combat code loads by path. Per map: the
// galaxy gets space combat and the Dominion; planets and the interior get the on-foot set.
//
// T7 registers it: UTDWarmupSubsystem::RegisterProvider(TEXT("Combat"),
//     [](const UWorld& W, TArray<FSoftObjectPath>& Out) { FTDCombatAssetList::Collect(W, Out); });

#pragma once

#include "CoreMinimal.h"

class UWorld;

struct UNREALGAMEJAM_API FTDCombatAssetList
{
	/** Appends this map's combat classes and path-loaded assets (unique). */
	static void Collect(const UWorld& World, TArray<FSoftObjectPath>& OutPaths);
	/** The same, by phase: bSpace = galaxy combat and the Dominion; otherwise on foot. */
	static void CollectFor(bool bSpace, TArray<FSoftObjectPath>& OutPaths);
};

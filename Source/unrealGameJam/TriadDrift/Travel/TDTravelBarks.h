// Triad Drift - Travel (Teammate 1). ENG-43: the travel and flight bark triggers T1 owns in dialogs.md. Each fires once per
// condition (an edge), through T2's FTDCombatBarks helpers into T3's squad limiter (global gap, per-id cooldown, priority);
// the words are dialogs.md's (T3's FTDBarkLines, Nicholas's DT_Barks wins row by row).

#pragma once

#include "CoreMinimal.h"

namespace TDBarkIdsT1
{
	inline const FName BoostEmpty(TEXT("Bark_BoostEmpty"));     // flown pilot, Flavour: the meter ran dry while boost was held
	inline const FName HyperSpool(TEXT("Bark_Hyper_Spool"));    // flown pilot, Normal: the pilot's hyperdrive starts charging
	inline const FName HyperBlocked(TEXT("Bark_Hyper_Blocked")); // flown pilot, Normal: refused in combat (the enemy lock)
	inline const FName AtmoEntry(TEXT("Bark_Atmo_Entry"));      // flown pilot, Normal: the atmospheric entry commits
	inline const FName LandReady(TEXT("Bark_Land_Ready"));      // flown pilot, Normal: a landing pad comes in range
	inline const FName LandDown(TEXT("Bark_Land_Down"));        // flown pilot, Normal: touchdown (T1 fires it; T5 does not)
}

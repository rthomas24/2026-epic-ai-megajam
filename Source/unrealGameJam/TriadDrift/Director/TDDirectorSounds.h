// Triad Drift - Director's Cut sounds (ENG-35): the slate clap, the "CUT!" sting, slow motion in and out.
// Owner: Teammate 3.
//
// The sounds live on Nick's UI style (DA_UI_Style, category "Director Sounds"; the engineering fallback fills them until
// his land). Every one is optional: unset = silent. They play as 2D UI sounds: not positioned, not slowed by a money
// shot's time dilation, heard while paused.

#pragma once

#include "CoreMinimal.h"

class UObject;
class UTDUIStyle;
struct FTDFXCue;

enum class ETDDirectorSound : uint8
{
	SlateClap,
	CutSting,
	SlowMoIn,
	SlowMoOut
};

struct UNREALGAMEJAM_API FTDDirectorSounds
{
	/** Plays the style's cue for this moment, if it has a sound. Returns true when a sound was asked to play. */
	static bool Play(const UObject* WorldContext, ETDDirectorSound Which);

	/** The style's cue for a moment (null when there is no style). */
	static const FTDFXCue* GetCue(const UTDUIStyle* Style, ETDDirectorSound Which);

	static const TCHAR* Name(ETDDirectorSound Which);

#if WITH_DEV_AUTOMATION_TESTS
	/** Tests: use this style instead of DA_UI_Style, and record every moment that played a sound. */
	static TWeakObjectPtr<const UTDUIStyle> StyleOverrideForTests;
	static TArray<ETDDirectorSound> PlayedForTests;
#endif
};

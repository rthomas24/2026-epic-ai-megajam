// Triad Drift - every weapon FX cue the combat code plays goes through here (Teammate 2). Tests switch the log on and
// check, per weapon, that firing played its Muzzle / Impact / Detonation cues and that those cues carry a sound
// (the Lead's deterministic weapon-sound check, Nov 13: replaces hunting for one-shots in au.DumpActiveSounds snapshots).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDPresentation.h"

/** Which slot of a weapon's FX asset a cue came from. */
enum class ETDCombatCueKind : uint8
{
	Muzzle,
	ImpactHull,
	ImpactShield,
	ImpactWorld,
	Detonation,
	Deflected
};

/** One cue play, as logged. */
struct FTDCombatCuePlay
{
	ETDCombatCueKind Kind = ETDCombatCueKind::Muzzle;
	/** The weapon's FX asset name (DA_<Weapon>_FX), or NAME_None. */
	FName FXName = NAME_None;
	/** The cue had a sound that loads (what the player hears). */
	bool bHasSound = false;
};

struct UNREALGAMEJAM_API FTDCombatCueLog
{
	/** Plays Cue through UTDStatics::PlayCue, recording it while logging is on. */
	static void Play(const UObject* WorldContext, const FTDFXCue& Cue, ETDCombatCueKind Kind, const UObject* FXAsset, const FVector& Location,
		const FRotator& Rotation);

	/** Tests: start (and clear) or stop recording. */
	static void SetLogging(bool bOn);
	static const TArray<FTDCombatCuePlay>& Get();

	static const TCHAR* KindName(ETDCombatCueKind Kind);
};

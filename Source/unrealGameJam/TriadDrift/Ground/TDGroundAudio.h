// Triad Drift - on-foot sound hooks (ENG-35): the pilot's steps, dash, jump, landing and hurt, the crew rifle shot and
// the drone hum. Each event plays the matching cue of DA_Ground_FX (UTDGroundFX: Presentation first, then the
// engineering copy under Gameplay/Characters), and only when that slot is empty the engineering sound in code (soft
// slots defaulting to /Game/TriadDrift/Gameplay/Audio). A missing asset plays nothing. Every hook also records its
// event name, so tests can check the hooks fire with no audio device. Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

class USoundBase;
class UAudioComponent;
class USceneComponent;
class UTDGroundFX;
struct FTDFXCue;

namespace TDGroundAudio
{
	/** A default fallback slot value: /Game/TriadDrift/Gameplay/Audio/<Relative> (e.g. "Foot/MSS_Foot_Jump"). */
	UNREALGAMEJAM_API TSoftObjectPtr<USoundBase> Default(const TCHAR* Relative);

	/** DA_Ground_FX (or the test override). Null when none exists. */
	UNREALGAMEJAM_API const UTDGroundFX* GroundFX();
	/** The cue of DA_Ground_FX for an event (StepSoil, StepRock, Dash, Jump, Land, Hurt, RifleShot, DroneHum); null if empty. */
	UNREALGAMEJAM_API const FTDFXCue* CueFor(FName Event);
	/** Tests: use this asset instead of DA_Ground_FX (null restores the lookup). */
	UNREALGAMEJAM_API void SetGroundFXOverride(const UTDGroundFX* FX);
	/** Tests: the sound the last hook resolved (the cue's, the fallback's, or null) and whether it came from a cue slot. */
	UNREALGAMEJAM_API USoundBase* GetLastResolvedSound();
	UNREALGAMEJAM_API bool WasLastFromCue();

	/** Records Event in Log (the last 32 kept), then plays the event's cue, else Fallback, at At (or its sound in 2D). */
	UNREALGAMEJAM_API void Play(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Fallback, const FVector& At, bool b2D,
		TArray<FName>& Log, FName Event);

	/** A looping sound attached to a component (the drone hum): the cue's, else Fallback. Records Event; returns the audio. */
	UNREALGAMEJAM_API UAudioComponent* PlayAttached(const TSoftObjectPtr<USoundBase>& Fallback, USceneComponent* AttachTo, TArray<FName>& Log, FName Event);
}

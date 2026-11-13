// Triad Drift - on-foot sound hooks (ENG-35): the pilot's steps, dash, jump, landing and hurt, the crew rifle shot and
// the drone hum. Sounds are soft slots with engineering defaults under /Game/TriadDrift/Gameplay/Audio (each asset
// carries its own attenuation, class and concurrency); a missing asset plays nothing. Every hook also records its
// event name, so tests can check the hooks fire with no audio device. Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

class USoundBase;
class UAudioComponent;
class USceneComponent;

namespace TDGroundAudio
{
	/** A default slot value: /Game/TriadDrift/Gameplay/Audio/<Relative> (e.g. "Foot/MSS_Foot_Jump"). */
	UNREALGAMEJAM_API TSoftObjectPtr<USoundBase> Default(const TCHAR* Relative);

	/** Records Event in Log (the last 32 kept), then plays Sound at At (or in 2D). Null-safe; returns nothing. */
	UNREALGAMEJAM_API void Play(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, const FVector& At, bool b2D,
		TArray<FName>& Log, FName Event);

	/** A looping sound attached to a component (the drone hum). Records Event; returns the component (null without a sound). */
	UNREALGAMEJAM_API UAudioComponent* PlayAttached(const TSoftObjectPtr<USoundBase>& Sound, USceneComponent* AttachTo, TArray<FName>& Log, FName Event);
}

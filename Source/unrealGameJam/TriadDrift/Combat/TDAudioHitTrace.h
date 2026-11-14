// Triad Drift - a timestamped trace of every real projectile hit's impact sound, for marking hits in recorded game
// audio (Nick's Audio, board N-069; Teammate 2). Development builds only, behind TD.Audio.TraceHits 1.
//
// One log line per hull / shield hit, written at the moment the projectile picks the cue (before the damage, so the
// shield state is the one the sound was chosen from):
//   TD audio hit: utc=2026-11-13T20:41:07.512Z app=123.456s world=98.765s audio=97.210s frame=4521 cue=ImpactShield
//     sound=/Game/.../SFX_Impact_Shield shooter=TDShip_0 victim=TDAudioTarget_ShieldUp shield=up 1000000/1000000 hull=1000000/1000000
// TD.Audio.SpawnTargets puts two held, unkillable targets 40 m ahead of the flown ship: one whose shield never drops
// (every hit is ImpactShield) and one with no shield (every hit is ImpactHull).

#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;
class UTDHealthComponent;
struct FTDFXCue;
enum class ETDCombatCueKind : uint8;

struct UNREALGAMEJAM_API FTDAudioHitTrace
{
	/** TD.Audio.TraceHits is on (always false in Shipping). */
	static bool IsOn();
	static void SetOn(bool bOn);

	/** Write the trace line for one hit (no-op unless on). VictimHealth is read as it is now: call before the damage. */
	static void Trace(const UObject* WorldContext, const FTDFXCue* Cue, ETDCombatCueKind Kind, const AActor* Shooter, const AActor* Victim,
		const UTDHealthComponent* VictimHealth);

	/** The last lines written (tests; kept while on, capped at 64). */
	static const TArray<FString>& GetRecent();

	/** Spawn the two deterministic targets At + Forward * Metres, 6 m either side (shield up on the left, down on the right). */
	static void SpawnTargets(UWorld* World, const FVector& At, const FVector& Forward, float Metres, AActor*& OutShieldUp, AActor*& OutShieldDown);

	/** Shield and hull of the targets: high enough that a recording session never kills or strips them. */
	static constexpr float TargetPoints = 1000000.f;
};

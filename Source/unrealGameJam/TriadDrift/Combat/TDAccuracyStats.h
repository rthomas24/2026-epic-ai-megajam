// Triad Drift - per-ship accuracy for the aim-assist balance pass (issue #192; Teammate 2).
//
// Counts the player's own fire only (the flown ship's weapons, not wingmen): shots per weapon slot, hits (a shot counts
// once, however many things it or its shards damage), and kills. TD.Weapons.Accuracy logs it; "reset" starts a new
// window. Dev only: the counting is a few integer adds and costs nothing measurable.

#pragma once

#include "CoreMinimal.h"

struct UNREALGAMEJAM_API FTDAccuracyStats
{
	/** [ship 0..2][slot 0 primary, 1 secondary] */
	static int32 Shots[3][2];
	static int32 Hits[3][2];
	static int32 Kills[3][2];
	/** Seconds each ship was flown in this window (for kills per minute). */
	static double FlownSeconds[3];
	static double WindowStart;

	static void Reset(double Now);
	static void NoteShot(int32 Ship, int32 Slot);
	/** Root = the launch serial of the shot the weapon fired (shards pass their parent's): one hit per root. */
	static void NoteHit(int32 Ship, int32 Slot, uint32 Root, bool bKill);
	static void NoteFlown(int32 Ship, double Seconds);
	static FString Report();

private:
	static TSet<uint32> HitRoots;
};

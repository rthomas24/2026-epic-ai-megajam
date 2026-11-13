// Triad Drift - World (Teammate 4). ENG-38 stage 2: the globe seen from space shows the same world as the ground.
#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"

/**
 * Bakes an equirectangular surface (base colour + emissive) for a planet's globe from its FTDTerrainField, so the
 * continents, ranges, seas and lava seen from orbit are the ones you land in:
 * - Around the landing zone (the globe's north pole, the planet actor's +Z) the globe shows the flat world through
 *   the same mapping the descent uses (FTDPlanetEntry): 0..60 deg from the pole = the pad's 0..7 km, then out to
 *   40 km (the far ring) by 120 deg. Aim at a feature from orbit and you arrive over it.
 * - Beyond that, planet-wide noise from the same seed and palette, blended in from 105 to 125 deg.
 * - A beacon marks the pad at the pole.
 * Pure and thread-safe (rows run in parallel); the planet actor calls it off the game thread.
 * Pixel (i, j): longitude = (i + 0.5) / W * 360 - 180, latitude = 90 - (j + 0.5) / H * 180.
 */
struct UNREALGAMEJAM_API FTDGlobeBake
{
	/** Ground distance from the pad (cm) for an angle from the landing zone (deg); clamps at the far ring. */
	static float GroundDistanceForAngle(float AngleDeg);

	/** Bakes W x H texels (sRGB BGRA) into OutBase and OutEmissive (sized W * H). */
	static void Bake(ETDDestination Planet, int32 W, int32 H, TArray<FColor>& OutBase, TArray<FColor>& OutEmissive, bool bCalibrationMarks = false);

	/** One texel at a globe point (lat / lon deg), for tests. */
	static void SampleAt(ETDDestination Planet, float LatDeg, float LonDeg, FLinearColor& OutBase, FLinearColor& OutEmissive);
};

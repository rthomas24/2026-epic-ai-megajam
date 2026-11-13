// Triad Drift - World (Teammate 4). ENG-34: the planet height field, pure math (no meshes): seeded per planet, layered
// noise with ridged mountains, a flat basin under the objective course, distant ranges closing the horizon.
#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"

/** Per-planet character of the terrain (all lengths in cm). */
struct UNREALGAMEJAM_API FTDTerrainStyle
{
	int32 Seed = 1;
	/** Rolling ground: amplitude and feature size. */
	float HillHeight = 18000.f;
	float HillScale = 220000.f;
	/** Ridged mountains: peak height and range size. */
	float MountainHeight = 90000.f;
	float MountainScale = 600000.f;
	/** 0 soft rounded ranges (Verdance) .. 1 sharp jagged crests (Cinder). */
	float Sharpness = 0.5f;
	/** Cinder: caldera bowls in the mountains; lava-river valleys carved along a noise contour. */
	float CalderaAmount = 0.f;
	float RiverDepth = 0.f;
	/** Domain warp strength (bends ranges into natural curves). */
	float Warp = 120000.f;

	/** The flattened course basin: inside this box (centred on the origin, half extents in cm) the terrain sits just
	 *  under the course slab; it blends up to full terrain over BasinBlend beyond the box. */
	FVector2D BasinHalfExtent = FVector2D(320000.f, 320000.f);
	float BasinFloor = -60.f;
	float BasinBlend = 150000.f;
	/** Walkable / flyable core half size, and the far ring where ranges rise to close the horizon. */
	float CoreHalfSize = 800000.f;
	float FarHalfSize = 4000000.f;
	float HorizonRise = 160000.f;

	static FTDTerrainStyle ForPlanet(ETDDestination Planet);
};

/** Height and masks at a point (all world cm on the planet map). */
struct UNREALGAMEJAM_API FTDTerrainSample
{
	float Height = 0.f;
	float Slope = 0.f;		// 0 flat .. 1 vertical
	float Height01 = 0.f;	// normalised over the planet's range
	float BroadMask = 0.f;	// 0..1 biome / lava-valley mask
	float FineMask = 0.f;	// 0..1 breakup
};

class UNREALGAMEJAM_API FTDTerrainField
{
public:
	explicit FTDTerrainField(const FTDTerrainStyle& InStyle) : Style(InStyle) {}

	const FTDTerrainStyle& GetStyle() const { return Style; }

	/** Terrain height (cm) at world XY. Deterministic for a style. */
	float Height(float X, float Y) const;

	/** Height plus slope (finite differences over Step cm) and the material masks. */
	FTDTerrainSample Sample(float X, float Y, float Step = 400.f) const;

	/** 0 inside the course basin, 1 where full terrain starts (smooth). */
	float BasinWeight(float X, float Y) const;

	/** Highest terrain the style can produce (normalisation). */
	float MaxHeight() const { return Style.HillHeight + Style.MountainHeight + Style.HorizonRise; }

	/** Seeded value noise in -1..1 (smooth, 2D). */
	static float Noise2D(float X, float Y, int32 Seed);

private:
	float Fbm(float X, float Y, int32 Octaves, int32 Seed) const;
	float Ridged(float X, float Y, int32 Octaves, int32 Seed, float Sharp) const;
	float RawHeight(float X, float Y) const;

	FTDTerrainStyle Style;
};

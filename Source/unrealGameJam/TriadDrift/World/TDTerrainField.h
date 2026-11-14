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
	/** The basin box's centre (course-local; T5's course ground box is offset from the pad). */
	FVector2D BasinCentre = FVector2D::ZeroVector;
	/** The arrival descent (T5's travel rig: from 600 m behind the pad, 300 m up, ~27 deg): the terrain stays this far
	 *  under the glide slope inside a cone around -X out to ApproachLength. */
	float ApproachHalfAngleDeg = 32.f;
	float ApproachLength = 250000.f;
	float ApproachClearance = 6000.f;
	float BasinFloor = 0.f;	// exactly T5's slab top: the slab hides (collision kept) and this mesh is the one floor drawn
	float BasinBlend = 150000.f;
	/** Walkable / flyable core half size, and the far ring where ranges rise to close the horizon. */
	float CoreHalfSize = 800000.f;
	float FarHalfSize = 4000000.f;
	float HorizonRise = 160000.f;

	/** Second ridged octave for erosion-like gullies on the ranges (fraction of MountainHeight). */
	float GullyAmount = 0.15f;
	/** Drainage gullies (environment builder, realism pass): parallel crests and channels that run straight down the
	 *  slope of the ranges, like water-cut ground, in two scales (fraction of MountainHeight; 0 = off). */
	float ErosionAmount = 0.f;
	float ErosionScale = 70000.f;	// the larger gully spacing (cm)

	/** The hero peak: one clearly tallest summit, visible from the course (centre, radius, height, all cm).
	 *  bVolcano = a cone with a crater (CraterRadius / CraterDepth) instead of a ridged massif. */
	FVector2D HeroCentre = FVector2D(650000.f, 250000.f);
	float HeroRadius = 300000.f;
	float HeroHeight = 250000.f;
	bool bVolcano = false;
	float CraterRadius = 45000.f;
	float CraterDepth = 30000.f;

	/** Named regions by bearing from the course (degrees, 0 = +X): each blends in over RegionBlendDeg. */
	float RiftBearing = 90.f;		// a deep meandering canyon
	float MesaBearing = 0.f;		// terraced flat-topped mesas
	float BasinBearing = 270.f;	// a lake (Verdance) / lava sea (Cinder) in a broad depression
	float PlateauBearing = 180.f;	// a high flat plateau with escarpment edges
	float RegionBlendDeg = 30.f;
	float RiftDepth = 50000.f;
	float MesaStep = 22000.f;
	float BasinDepth = 40000.f;
	float PlateauHeight = 70000.f;
	/** The lake / lava-sea surface (cm) and whether it glows (lava). */
	float SeaLevel = -15000.f;
	bool bLavaSea = false;

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
	FVector Normal = FVector::UpVector;
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
	/** Same shape over a custom blend distance. */
	float BasinWeightAt(float X, float Y, float Blend) const;

	/** Highest terrain the style can produce (normalisation). */
	float MaxHeight() const { return FMath::Max(Style.HeroHeight, Style.HillHeight + Style.MountainHeight * 1.5f) + Style.HorizonRise * 1.45f; }

	/** 0..1 weight of a named region at a point (bearing sectors around the course, 0 inside the basin). */
	float RegionWeight(float X, float Y, float Bearing) const;

	/** The sea's footprint centre and radius (the actor puts a water / lava surface there). */
	FVector2D SeaCentre() const;
	float SeaRadius() const { return Style.CoreHalfSize * 0.45f; }

	/** The river noise that carves the lava-river beds (0 on a river's centre line), domain-warped like the height. */
	float RiverNoise(float X, float Y) const;

	/** Real relief of the walkable core (m): lowest sea / canyon floor to the hero summit (the material's HeightRangeM). */
	float ReliefMeters() const;

	/** Seeded value noise in -1..1 (smooth, 2D). */
	static float Noise2D(float X, float Y, int32 Seed);

private:
	float Fbm(float X, float Y, int32 Octaves, int32 Seed) const;
	float Ridged(float X, float Y, int32 Octaves, int32 Seed, float Sharp) const;
	float RawHeight(float X, float Y) const;
	/** Slope-aligned gully height (cm) for the drainage pass, given the downhill gradient of the large forms. */
	float Erosion(float X, float Y, const FVector2D& Grad) const;
	/** The large forms only (hills + 3 octaves of the ranges): cheap, for the erosion's slope direction. */
	float LargeForms(float X, float Y) const;

	FTDTerrainStyle Style;
};

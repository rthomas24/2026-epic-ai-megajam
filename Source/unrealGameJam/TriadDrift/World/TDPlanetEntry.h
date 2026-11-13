// Triad Drift - World (Teammate 4). ENG-38 planetary descent: where an entry on the globe arrives over the flat
// planet world, and where a take-off comes out on the globe. Pure functions (no world), shared with T1's descent.
#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"

/** A point on the globe. Globe frame: the landing zone is the north pole (+Z); longitude 0 is the course's +X. */
struct FTDGlobePoint
{
	float LatDeg = 90.f;
	float LonDeg = 0.f;

	/** Unit position on the globe (globe-local). */
	FVector ToUnit() const;
	static FTDGlobePoint FromUnit(const FVector& Unit);
	/** Angular distance from the landing zone (deg, 0..180). */
	float AngleFromPadDeg() const { return 90.f - LatDeg; }
};

/**
 * ENG-38 stage 1 (agreed with T1). The planet's globe and its flat world are tied together around the landing zone:
 * - The angular distance from the landing zone maps linearly onto ground distance from the pad,
 *   d = MaxArrivalRadius * min(1, angle / SpreadDeg). Entries farther out than SpreadDeg clamp to the ring, so the
 *   pad is always within MaxArrivalRadius (inside the 8 km collision core and T1's 17 km flight zone).
 * - The bearing is the longitude (course +X at 0).
 * - The heading is the entry velocity's component along the surface, in the local north (toward the pad) / east
 *   basis, laid onto the flat plane.
 * - Inside the ring the inverse is exact.
 * Everything is in COURSE-LOCAL space (the pad at the origin); transform by the course actor.
 */
struct UNREALGAMEJAM_API FTDPlanetEntry
{
	static constexpr float MaxArrivalRadius = 600000.f;	// 6 km: T1's flight zone warns from ~7 km (one number, agreed with T1)
	static constexpr float SpreadDeg = 60.f;

	/**
	 * Entry at P with GlobeVelocity (globe-local; only its direction along the surface matters) -> the arrival:
	 * XY inside MaxArrivalRadius, Z = the ground under it (never below the basin floor) + AltitudeCm, yaw = the
	 * surface heading, pitch and roll 0. A velocity straight down (no surface component) faces the pad.
	 */
	static FTransform ArrivalFromGlobe(ETDDestination Planet, const FTDGlobePoint& P, const FVector& GlobeVelocity, float AltitudeCm);

	/**
	 * Take-off: a course-local departure (position and heading) -> where on the globe the ship comes out, and the
	 * outward globe-local direction (the surface heading tilted up by ClimbDeg).
	 */
	static FTDGlobePoint GlobeFromDeparture(const FTransform& CourseLocalDeparture, FVector& OutGlobeDir, float ClimbDeg = 30.f);

	/**
	 * Galaxy side (T1's entry and take-off). The globe frame is ATDPlanetAtmosphere::GetGlobeQuat(): the landing zone
	 * pole is its +Z (with the globe bake on, facing the combat zone), longitude 0 its +X. WorldPoint / WorldVelocity at the atmosphere -> the globe point and the
	 * globe-local velocity.
	 */
	static FTDGlobePoint GlobeFromWorld(const class ATDPlanetAtmosphere* Planet, const FVector& WorldPoint, const FVector& WorldVelocity, FVector& OutGlobeVelocity);

	/** Take-off: where on the galaxy globe a departure comes out (on the atmosphere shell), and its world direction. */
	static FVector WorldFromGlobe(const class ATDPlanetAtmosphere* Planet, const FTDGlobePoint& P, const FVector& GlobeDir, FVector& OutWorldDir);

	/** Ground position (course-local XY) for a globe point, and back (exact inside the ring). */
	static FVector2D GroundFromGlobe(const FTDGlobePoint& P);
	static FTDGlobePoint GlobeFromGround(const FVector2D& XY);
};

// Triad Drift - World (Teammate 4). ENG-38 planetary descent mapping.
#include "TriadDrift/World/TDPlanetEntry.h"

#include "TriadDrift/World/TDTerrainField.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"

FVector FTDGlobePoint::ToUnit() const
{
	const float Lat = FMath::DegreesToRadians(LatDeg), Lon = FMath::DegreesToRadians(LonDeg);
	return FVector(FMath::Cos(Lat) * FMath::Cos(Lon), FMath::Cos(Lat) * FMath::Sin(Lon), FMath::Sin(Lat));
}

FTDGlobePoint FTDGlobePoint::FromUnit(const FVector& Unit)
{
	const FVector U = Unit.GetSafeNormal();
	FTDGlobePoint P;
	P.LatDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(U.Z, -1.f, 1.f)));
	P.LonDeg = FVector2D(U.X, U.Y).IsNearlyZero() ? 0.f : FMath::RadiansToDegrees(FMath::Atan2(U.Y, U.X));
	return P;
}

FVector2D FTDPlanetEntry::GroundFromGlobe(const FTDGlobePoint& P)
{
	const float D = MaxArrivalRadius * FMath::Min(1.f, FMath::Max(0.f, P.AngleFromPadDeg()) / SpreadDeg);
	const float Lon = FMath::DegreesToRadians(P.LonDeg);
	return FVector2D(FMath::Cos(Lon), FMath::Sin(Lon)) * D;
}

FTDGlobePoint FTDPlanetEntry::GlobeFromGround(const FVector2D& XY)
{
	const float D = FMath::Min((float)XY.Size(), MaxArrivalRadius);
	FTDGlobePoint P;
	P.LatDeg = 90.f - SpreadDeg * D / MaxArrivalRadius;
	P.LonDeg = XY.IsNearlyZero() ? 0.f : FMath::RadiansToDegrees(FMath::Atan2(XY.Y, XY.X));
	return P;
}

namespace
{
	/** Local surface basis at a globe point: "north" points toward the landing zone (the pole), "east" along +longitude. */
	void TDPlanetEntry_Basis(const FTDGlobePoint& P, FVector& OutUp, FVector& OutNorth, FVector& OutEast)
	{
		OutUp = P.ToUnit();
		const float Lon = FMath::DegreesToRadians(P.LonDeg);
		OutEast = FVector(-FMath::Sin(Lon), FMath::Cos(Lon), 0.f);
		OutNorth = FVector::CrossProduct(OutUp, OutEast).GetSafeNormal();
		if (OutNorth.IsNearlyZero()) { OutNorth = FVector(-FMath::Cos(Lon), -FMath::Sin(Lon), 0.f); }
	}

	/** The flat-plane directions matching north / east at a ground point (toward the pad / counter-clockwise around it). */
	void TDPlanetEntry_FlatBasis(const FTDGlobePoint& P, FVector2D& OutNorth, FVector2D& OutEast)
	{
		const float Lon = FMath::DegreesToRadians(P.LonDeg);
		OutNorth = -FVector2D(FMath::Cos(Lon), FMath::Sin(Lon));
		OutEast = FVector2D(-FMath::Sin(Lon), FMath::Cos(Lon));
	}
}

FTransform FTDPlanetEntry::ArrivalFromGlobe(ETDDestination Planet, const FTDGlobePoint& P, const FVector& GlobeVelocity, float AltitudeCm)
{
	const FVector2D XY = GroundFromGlobe(P);
	FVector Up, North, East;
	TDPlanetEntry_Basis(P, Up, North, East);
	FVector2D FlatNorth, FlatEast;
	TDPlanetEntry_FlatBasis(P, FlatNorth, FlatEast);
	const float N = FVector::DotProduct(GlobeVelocity, North);
	const float E = FVector::DotProduct(GlobeVelocity, East);
	FVector2D Heading = FlatNorth * N + FlatEast * E;
	if (P.AngleFromPadDeg() < 0.01f)
	{
		Heading = FVector2D(GlobeVelocity.X, GlobeVelocity.Y);	// at the pole the globe's XY is the course's XY
	}
	// A near-radial dive (T1's live finding: TD.Travel.ToPlanet aims at the centre) has only a sliver of velocity along
	// the surface, and its direction is noise: the same entry arrived 180 deg apart. Below 10% of the speed, face the pad.
	const float Speed = GlobeVelocity.Size();
	if (Heading.IsNearlyZero(1e-4f) || FVector2D(N, E).Size() < 0.1f * Speed)
	{
		Heading = XY.IsNearlyZero() ? FVector2D(1.f, 0.f) : -XY;	// straight down: face the pad
	}
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X));
	const FTDTerrainField Field(FTDTerrainStyle::ForPlanet(Planet));
	const float Ground = FMath::Max(Field.Height(XY.X, XY.Y), Field.GetStyle().BasinFloor);
	return FTransform(FRotator(0.f, Yaw, 0.f), FVector(XY.X, XY.Y, Ground + AltitudeCm));
}

FTDGlobePoint FTDPlanetEntry::GlobeFromDeparture(const FTransform& CourseLocalDeparture, FVector& OutGlobeDir, float ClimbDeg)
{
	const FVector Loc = CourseLocalDeparture.GetLocation();
	const FTDGlobePoint P = GlobeFromGround(FVector2D(Loc.X, Loc.Y));
	FVector Up, North, East;
	TDPlanetEntry_Basis(P, Up, North, East);
	FVector2D FlatNorth, FlatEast;
	TDPlanetEntry_FlatBasis(P, FlatNorth, FlatEast);
	const FVector Fwd = CourseLocalDeparture.GetRotation().GetForwardVector();
	FVector2D H(Fwd.X, Fwd.Y);
	if (H.IsNearlyZero()) { H = FVector2D(1.f, 0.f); }
	H.Normalize();
	FVector Surface;
	if (P.AngleFromPadDeg() < 0.01f)
	{
		Surface = FVector(H.X, H.Y, 0.f);
	}
	else
	{
		Surface = North * FVector2D::DotProduct(H, FlatNorth) + East * FVector2D::DotProduct(H, FlatEast);
	}
	const float Climb = FMath::DegreesToRadians(ClimbDeg);
	OutGlobeDir = (Surface.GetSafeNormal() * FMath::Cos(Climb) + Up * FMath::Sin(Climb)).GetSafeNormal();
	return P;
}

FTDGlobePoint FTDPlanetEntry::GlobeFromWorld(const ATDPlanetAtmosphere* Planet, const FVector& WorldPoint, const FVector& WorldVelocity, FVector& OutGlobeVelocity)
{
	if (!Planet)
	{
		OutGlobeVelocity = WorldVelocity;
		return FTDGlobePoint();
	}
	const FQuat Q = Planet->GetGlobeQuat();
	OutGlobeVelocity = Q.UnrotateVector(WorldVelocity);
	return FTDGlobePoint::FromUnit(Q.UnrotateVector(WorldPoint - Planet->GetPlanetCenter()));
}

FVector FTDPlanetEntry::WorldFromGlobe(const ATDPlanetAtmosphere* Planet, const FTDGlobePoint& P, const FVector& GlobeDir, FVector& OutWorldDir)
{
	if (!Planet)
	{
		OutWorldDir = GlobeDir;
		return P.ToUnit();
	}
	const FQuat Q = Planet->GetGlobeQuat();
	OutWorldDir = Q.RotateVector(GlobeDir).GetSafeNormal();
	return Planet->GetPlanetCenter() + Q.RotateVector(P.ToUnit()) * Planet->GetAtmosphereRadius();
}

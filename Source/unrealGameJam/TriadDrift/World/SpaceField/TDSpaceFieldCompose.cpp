// Triad Drift - the space field's composed regions (Teammate 6; docs/eng/SPACE-DESIGN.md, approved Nov 14).
// Ryan: space must read as a real region with belts and distinct areas, not "a bunch of rocks bundled in a pile".
// td.SpaceField.Compose 1 (default) builds the regions; 0 builds the Nov 13 composition (random tilted rings and
// cluster shells), kept and indexed in docs/eng/ART-HISTORY.md.
//
// Region 2, the Shepherd Belt: a true ring round the combat zone in ONE thin plane, rocks on circular orbits, larger
// rocks on the inner edge grading to gravel outside, thicker and thinner stretches along the arc, a few landmark rocks,
// clean gaps where the routes cross (900 m+ wide on the Verdance bearing: T4's tutorial escape crosses there), and a
// shepherd moonlet on each edge of the Verdance and Dominion gaps.

#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"

namespace TDSpaceFieldCompose
{
	static int32 GCompose = 1;
	static FAutoConsoleVariableRef CVarCompose(TEXT("td.SpaceField.Compose"), GCompose,
		TEXT("1 (default): the composed regions of docs/eng/SPACE-DESIGN.md. 0: the Nov 13 composition (random rings and cluster shells). Read at level load."));

	/** Size by the power law used everywhere (many small, few large). */
	static float Power(FRandomStream& Rng, float Min, float Max, float Exponent) { return FMath::Lerp(Min, Max, FMath::Pow(Rng.FRand(), Exponent)); }
	/** A standard normal sample (Box-Muller). */
	static float Gauss(FRandomStream& Rng) { const float U = FMath::Max(1e-6f, Rng.FRand()); return FMath::Sqrt(-2.f * FMath::Loge(U)) * FMath::Cos(2.f * PI * Rng.FRand()); }
}

bool ATDSpaceField::UseComposedRegions() const
{
	return TDSpaceFieldCompose::GCompose != 0;
}

void ATDSpaceField::BuildShepherdBelt(FRandomStream& Rng)
{
	using namespace TDSpaceFieldCompose;
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);

	// The plane: tilted BeltTiltDeg about a seeded horizontal axis. Basis U, V in the plane, N its normal.
	const FQuat Tilt = FQuat(FVector::UpVector, FMath::DegreesToRadians(Rng.FRandRange(0.f, 360.f))) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(BeltTiltDeg));
	const FVector U = Tilt.RotateVector(FVector::ForwardVector);
	const FVector V = Tilt.RotateVector(FVector::RightVector);
	const FVector N = Tilt.RotateVector(FVector::UpVector);
	BeltNormal = N;
	BeltCenter = Zone;

	// Gaps: every route out of the zone (to the planets, the Dominion, the gate ends), as a bearing in the plane with a
	// half width. The Verdance bearing is wide (T4: the escape line can start anywhere in the pocket).
	struct FGap { FVector Dir; float HalfWidth; bool bShepherds; };
	TArray<FGap> Gaps;
	auto InPlane = [&](const FVector& WorldTarget) { FVector D = WorldTarget - Zone; D -= N * FVector::DotProduct(D, N); return D.GetSafeNormal(); };
	for (const FTDPlanetSpec& Pl : L.Planets)
	{
		const bool bVerdance = Pl.Destination == ETDDestination::Verdance;
		Gaps.Add({ InPlane(Xf.TransformPosition(Pl.Location)), bVerdance ? BeltVerdanceGapHalf : BeltGapHalf, true });
	}
	Gaps.Add({ InPlane(Xf.TransformPosition(L.Dominion.GetLocation())), BeltGapHalf, true });
	for (const FTDGatePairSpec& G : L.Gates)
	{
		for (const FTransform& E : { G.A, G.B })
		{
			const FVector P = Xf.TransformPosition(E.GetLocation());
			if (FVector::Dist(P, Zone) > BeltInner * 0.5f) { Gaps.Add({ InPlane(P), BeltGapHalf, false }); }
		}
	}
	auto InGap = [&](const FVector& Local, float Pad)	// Local = offset from the zone, in the plane
	{
		for (const FGap& G : Gaps)
		{
			const float Along = FVector::DotProduct(Local, G.Dir);
			if (Along <= 0.f) { continue; }
			if ((Local - G.Dir * Along).Size() < G.HalfWidth + Pad) { return true; }
		}
		return false;
	};

	// Shepherd moonlets: one on each edge of the gaps that have them, on the ring's middle.
	const float Mid = (BeltInner + BeltOuter) * 0.5f;
	for (const FGap& G : Gaps)
	{
		if (!G.bShepherds) { continue; }
		const FVector Side = FVector::CrossProduct(N, G.Dir).GetSafeNormal();
		for (const float S : { -1.f, 1.f })
		{
			const float Size = Rng.FRandRange(MoonletMinSize, MoonletMaxSize);
			const FVector Local = G.Dir * Mid + Side * S * (G.HalfWidth + Size * 0.5f + 8000.f);
			AddRock(Zone + Local, Size, ETDSpaceFieldKind::Belt, Rng, 0);
		}
	}

	// Landmarks: big rocks spread round the ring in the plane (so the belt is not uniform noise).
	for (int32 i = 0, Try = 0; i < BeltLandmarks && Try < BeltLandmarks * 6; ++Try)
	{
		const float A = (2.f * PI) * (i + Rng.FRandRange(0.f, 0.8f)) / BeltLandmarks;
		const float R = FMath::Lerp(BeltInner, Mid, Rng.FRand());
		const FVector Local = (U * FMath::Cos(A) + V * FMath::Sin(A)) * R + N * Gauss(Rng) * BeltThickness * 0.3f;
		const float Size = Rng.FRandRange(6000.f, 12000.f);
		if (InGap(Local, Size * 0.5f + 3000.f)) { continue; }
		i += AddRock(Zone + Local, Size, ETDSpaceFieldKind::Belt, Rng, Rng.FRand() < 0.6f ? 0 : 2) ? 1 : 0;
	}

	// The ring: orbit radius across the band, density modulated along the arc (three thick and three thin stretches plus
	// noise), size graded from the inner edge (to 90 m) to the outer edge (gravel to 15 m), thickness thinner outside.
	const float PhaseA = Rng.FRandRange(0.f, 2.f * PI), PhaseB = Rng.FRandRange(0.f, 2.f * PI);
	int32 Placed = 0;
	for (int32 Try = 0; Placed < ShepherdRocks && Try < ShepherdRocks * 6; ++Try)
	{
		const float A = Rng.FRandRange(0.f, 2.f * PI);
		const float Density = 0.55f + 0.3f * FMath::Sin(3.f * A + PhaseA) + 0.15f * FMath::Sin(7.f * A + PhaseB);
		if (Rng.FRand() > Density) { continue; }
		const float T = FMath::Clamp(0.5f + Gauss(Rng) * 0.22f, 0.f, 1.f);	// across the band: dense middle, soft edges
		const float R = FMath::Lerp(BeltInner, BeltOuter, T);
		const float Thick = BeltThickness * (1.f - 0.5f * T);
		const FVector Local = (U * FMath::Cos(A) + V * FMath::Sin(A)) * R + N * Gauss(Rng) * Thick;
		const float MaxSize = FMath::Lerp(9000.f, 1500.f, T);
		const float Size = Power(Rng, 400.f, MaxSize, 3.5f);
		if (InGap(Local, Size * 0.5f)) { continue; }
		// Families: rock and metal inside, ice toward the outer edge.
		const int32 Fam = Rng.FRand() < T ? 1 : (Rng.FRand() < 0.5f ? 0 : 2);
		Placed += AddRock(Zone + Local, Size, ETDSpaceFieldKind::Belt, Rng, Fam) ? 1 : 0;
	}
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: Shepherd Belt %d rocks, %d gaps, r %.1f-%.1f km, tilt %.0f deg"), Placed, Gaps.Num(), BeltInner / 100000.f,
		BeltOuter / 100000.f, BeltTiltDeg);
}

// Triad Drift - World (Teammate 4). ENG-38 stage 2 globe bake.
#include "TriadDrift/World/TDGlobeBake.h"

#include "TriadDrift/World/TDTerrainField.h"
#include "TriadDrift/World/TDPlanetEntry.h"
#include "Async/ParallelFor.h"

namespace
{
	constexpr float TDGlobeBake_FarAngle = 120.f;		// the far ring's edge on the globe
	constexpr float TDGlobeBake_BlendFrom = 105.f;	// planet-wide noise starts taking over
	constexpr float TDGlobeBake_BlendTo = 125.f;

	/** The planet's palette from height, slope and the biome / lava mask (sRGB-ish linear values). */
	/** HeightM: metres above the basin floor (the field's Height01 spans a wider range than the visible relief). */
	void TDGlobeBake_Shade(bool bCinder, float HeightM, float Slope, float Broad, bool bSea, FLinearColor& Base, FLinearColor& Glow)
	{
		Glow = FLinearColor::Black;
		if (bCinder)
		{
			if (bSea || FMath::Abs(Broad - 0.5f) < 0.06f)
			{
				// The lava sea and the carved lava rivers: they glow.
				Base = FLinearColor(0.35f, 0.08f, 0.02f);
				Glow = FLinearColor(1.f, 0.32f, 0.05f) * (bSea ? 1.f : 0.8f);
				return;
			}
			const FLinearColor Basalt(0.05f, 0.045f, 0.045f), Ash(0.2f, 0.16f, 0.14f), Rust(0.32f, 0.12f, 0.06f);
			Base = FMath::Lerp(FMath::Lerp(Basalt, Rust, FMath::Clamp(Broad, 0.f, 1.f) * 0.5f), Ash, FMath::Clamp((HeightM - 500.f) / 1200.f, 0.f, 1.f));
		}
		else
		{
			if (bSea)
			{
				Base = FLinearColor(0.02f, 0.13f, 0.16f);
				Glow = FLinearColor(0.f, 0.06f, 0.05f);	// the bioluminescent shallows, faint
				return;
			}
			const FLinearColor Jungle(0.04f, 0.24f, 0.08f), Moss(0.16f, 0.32f, 0.12f), Rock(0.46f, 0.47f, 0.5f), Snow(0.88f, 0.92f, 0.96f);
			Base = FMath::Lerp(Jungle, Moss, FMath::Clamp(Broad, 0.f, 1.f));
			Base = FMath::Lerp(Base, Rock, FMath::Clamp((HeightM - 600.f) / 600.f, 0.f, 1.f));
			Base = FMath::Lerp(Base, Snow, FMath::Clamp((HeightM - 1700.f) / 400.f, 0.f, 1.f));
			// Bioluminescent jungle patches at night, by biome (not by height: the carved arrival corridor must not show).
			Glow = FLinearColor(0.f, 0.02f, 0.016f) * FMath::Clamp((0.65f - Broad) * 4.f, 0.f, 1.f) * FMath::Clamp(1.f - HeightM / 900.f, 0.f, 1.f);	// was a teal wash up close
		}
		Base *= 1.f - 0.2f * FMath::Clamp(Slope, 0.f, 1.f);	// cliffs a little darker: the ranges show their relief
	}

	/** Planet-wide terrain past the far ring: same palette, broad continents from 3D noise on the unit sphere. */
	void TDGlobeBake_Global(bool bCinder, int32 Seed, const FVector& U, FLinearColor& Base, FLinearColor& Glow)
	{
		const FVector Off(Seed * 0.0131f, Seed * 0.0077f, Seed * 0.0193f);
		float N = 0.f, Amp = 0.5f, Freq = 2.2f;
		for (int32 o = 0; o < 5; ++o)
		{
			N += FMath::PerlinNoise3D(U * Freq + Off) * Amp;
			Freq *= 2.1f;
			Amp *= 0.5f;
		}
		const float H01 = FMath::Clamp(0.5f + N, 0.f, 1.f);
		const float Broad = FMath::Clamp(0.5f + FMath::PerlinNoise3D(U * 1.3f - Off) * 0.9f, 0.f, 1.f);
		TDGlobeBake_Shade(bCinder, (H01 - 0.42f) * 5000.f, FMath::Clamp(FMath::Abs(N) * 1.5f, 0.f, 1.f) * 0.5f, Broad, H01 < 0.42f, Base, Glow);
	}
}

float FTDGlobeBake::GroundDistanceForAngle(float AngleDeg)
{
	const float A = FMath::Clamp(AngleDeg, 0.f, 180.f);
	const float Ring = FTDPlanetEntry::MaxArrivalRadius;
	if (A <= FTDPlanetEntry::SpreadDeg)
	{
		return Ring * A / FTDPlanetEntry::SpreadDeg;	// exactly the descent mapping
	}
	const float FarHalf = FTDTerrainStyle::ForPlanet(ETDDestination::Verdance).FarHalfSize;
	return FMath::Lerp(Ring, FarHalf, FMath::Clamp((A - FTDPlanetEntry::SpreadDeg) / (TDGlobeBake_FarAngle - FTDPlanetEntry::SpreadDeg), 0.f, 1.f));
}

namespace
{
	void TDGlobeBake_Texel(const FTDTerrainField& Field, bool bCinder, float LatDeg, float LonDeg, FLinearColor& Base, FLinearColor& Glow)
	{
		FTDGlobePoint P;
		P.LatDeg = LatDeg;
		P.LonDeg = LonDeg;
		const float Angle = P.AngleFromPadDeg();
		FLinearColor LocalBase = FLinearColor::Black, LocalGlow = FLinearColor::Black;
		const float GlobalW = FMath::SmoothStep(TDGlobeBake_BlendFrom, TDGlobeBake_BlendTo, Angle);
		if (GlobalW < 1.f)
		{
			const float D = FTDGlobeBake::GroundDistanceForAngle(Angle);
			const float Lon = FMath::DegreesToRadians(LonDeg);
			const float X = FMath::Cos(Lon) * D, Y = FMath::Sin(Lon) * D;
			const FTDTerrainSample S = Field.Sample(X, Y, 2000.f);
			TDGlobeBake_Shade(bCinder, (S.Height - Field.GetStyle().BasinFloor) / 100.f, S.Slope, S.BroadMask, S.Height < Field.GetStyle().SeaLevel, LocalBase, LocalGlow);
			// Close up in a dive the cap fills the frame: fine breakup (the field's own detail mask) in brightness and a
			// little hue, so it never reads as one flat colour.
			const float Fine = S.FineMask;
			LocalBase *= 0.78f + 0.44f * Fine;
			LocalBase = FMath::Lerp(LocalBase, LocalBase * FLinearColor(1.15f, 0.95f, 0.8f), FMath::Clamp((Fine - 0.6f) * 2.f, 0.f, 1.f) * 0.5f);
		}
		FLinearColor GlobalBase = FLinearColor::Black, GlobalGlow = FLinearColor::Black;
		if (GlobalW > 0.f)
		{
			TDGlobeBake_Global(bCinder, Field.GetStyle().Seed, P.ToUnit(), GlobalBase, GlobalGlow);
		}
		Base = FMath::Lerp(LocalBase, GlobalBase, GlobalW);
		Glow = FMath::Lerp(LocalGlow, GlobalGlow, GlobalW);
		// No beacon in the texture (it became a white target disc filling every dive frame): the landing site's light
		// is a small world-space marker on ATDPlanetAtmosphere that fades out as the ship closes.
	}
}

void FTDGlobeBake::SampleAt(ETDDestination Planet, float LatDeg, float LonDeg, FLinearColor& OutBase, FLinearColor& OutEmissive)
{
	const FTDTerrainField Field(FTDTerrainStyle::ForPlanet(Planet));
	TDGlobeBake_Texel(Field, Planet == ETDDestination::Cinder, LatDeg, LonDeg, OutBase, OutEmissive);
}

void FTDGlobeBake::Bake(ETDDestination Planet, int32 W, int32 H, TArray<FColor>& OutBase, TArray<FColor>& OutEmissive, bool bCalibrationMarks)
{
	W = FMath::Max(W, 4);
	H = FMath::Max(H, 2);
	OutBase.SetNumUninitialized(W * H);
	OutEmissive.SetNumUninitialized(W * H);
	const FTDTerrainField Field(FTDTerrainStyle::ForPlanet(Planet));
	const bool bCinder = Planet == ETDDestination::Cinder;
	ParallelFor(H, [&](int32 j)
	{
		const float Lat = 90.f - (j + 0.5f) / H * 180.f;
		for (int32 i = 0; i < W; ++i)
		{
			const float Lon = (i + 0.5f) / W * 360.f - 180.f;
			FLinearColor B, E;
			TDGlobeBake_Texel(Field, bCinder, Lat, Lon, B, E);
			if (bCalibrationMarks)
			{
				// Mesh UV calibration: lon 0 magenta, lon 90 yellow, the equator white.
				if (FMath::Abs(Lon) < 2.f) { B = E = FLinearColor(1.f, 0.f, 1.f); }
				else if (FMath::Abs(Lon - 90.f) < 2.f) { B = E = FLinearColor(1.f, 1.f, 0.f); }
				else if (FMath::Abs(Lat) < 1.f) { B = E = FLinearColor::White; }
			}
			OutBase[j * W + i] = B.ToFColor(/*bSRGB*/ true);
			OutEmissive[j * W + i] = E.ToFColor(true);
		}
	});
}

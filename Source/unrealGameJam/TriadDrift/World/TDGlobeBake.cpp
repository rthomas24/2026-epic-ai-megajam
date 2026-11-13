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
			// Environment pass (Nov 13): the same palette as MI_Terrain_Verdance (teal / emerald flats by biome, slate cliffs
			// by slope, tinted lichen on the highest ground only: no white), so the globe and the ground are one world.
			const FLinearColor Jungle(0.02f, 0.17f, 0.15f), Moss(0.05f, 0.30f, 0.10f), Rock(0.15f, 0.17f, 0.23f), Snow(0.36f, 0.56f, 0.50f);
			Base = FMath::Lerp(Jungle, Moss, FMath::Clamp(Broad, 0.f, 1.f));
			Base = FMath::Lerp(Base, Rock, FMath::Clamp(FMath::Max((Slope - 0.12f) / 0.2f, (HeightM - 1000.f) / 900.f), 0.f, 1.f));
			Base = FMath::Lerp(Base, Snow, FMath::Clamp((HeightM - 1450.f) / 550.f, 0.f, 1.f));
			// Bioluminescent jungle patches at night, by biome (not by height: the carved arrival corridor must not show).
			Glow = FMath::Lerp(FLinearColor(0.f, 0.16f, 0.18f), FLinearColor(0.10f, 0.03f, 0.20f), FMath::Clamp((Broad - 0.35f) * 3.f, 0.f, 1.f))
				* FMath::Clamp(FMath::Abs(Broad - 0.5f) * 5.f, 0.15f, 1.f) * FMath::Clamp(1.f - HeightM / 900.f, 0.f, 1.f);
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
		}
		FLinearColor GlobalBase = FLinearColor::Black, GlobalGlow = FLinearColor::Black;
		if (GlobalW > 0.f)
		{
			TDGlobeBake_Global(bCinder, Field.GetStyle().Seed, P.ToUnit(), GlobalBase, GlobalGlow);
		}
		Base = FMath::Lerp(LocalBase, GlobalBase, GlobalW);
		Glow = FMath::Lerp(LocalGlow, GlobalGlow, GlobalW);
		// The pad's beacon: a bright core and a ring around it, so the landing site is findable from orbit.
		if (Angle < 1.2f)
		{
			Base = FLinearColor(0.9f, 1.f, 1.f);
			Glow = FLinearColor(0.6f, 1.f, 1.f);
		}
		else if (Angle > 2.f && Angle < 2.6f)
		{
			Base = FMath::Lerp(Base, FLinearColor(0.4f, 1.f, 1.f), 0.7f);
			Glow = FLinearColor(0.3f, 0.9f, 1.f);
		}
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

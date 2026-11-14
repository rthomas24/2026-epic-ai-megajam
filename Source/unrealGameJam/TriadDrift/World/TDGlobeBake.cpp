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
				// #145 (B1-5): at 0.06 the shallows glowed as a flat, sharp-edged teal patch on the menu planet's limb (it
				// stays with the ring and the atmosphere hidden, and goes with the surface): a deep, darker sea, barely lit.
				Base = FLinearColor(0.015f, 0.07f, 0.09f);
				Glow = FLinearColor(0.f, 0.012f, 0.01f);	// the bioluminescent shallows, faint
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
	void TDGlobeBake_Texel(const FTDTerrainField& Field, bool bCinder, float LatDeg, float LonDeg, FLinearColor& Base, FLinearColor& Glow, float TexelLatDeg = 0.f, float TexelLonDeg = 0.f)
	{
		FTDGlobePoint P;
		P.LatDeg = LatDeg;
		P.LonDeg = LonDeg;
		const float Angle = P.AngleFromPadDeg();
		FLinearColor LocalBase = FLinearColor::Black, LocalGlow = FLinearColor::Black;
		const float GlobalW = FMath::SmoothStep(TDGlobeBake_BlendFrom, TDGlobeBake_BlendTo, Angle);
		if (GlobalW < 1.f)
		{
			// Lead (Nov 14, played at 1.6 radii): the cap around the landing zone is the texture's pole. One point sample of
			// the fine terrain per texel aliased into radial streaks (texels there are long in latitude, thin in
			// longitude), and the cap was one jungle-teal with no broad structure. Each texel now averages a grid of
			// samples across its own footprint, the fine breakup is gentler, and the biome mask gives light and dark.
			const int32 NLat = TexelLatDeg > 0.f ? 4 : 1, NLon = TexelLonDeg > 0.f ? 2 : 1;
			FLinearColor SumBase = FLinearColor::Black, SumGlow = FLinearColor::Black;
			for (int32 a = 0; a < NLat; ++a)
			{
				for (int32 b = 0; b < NLon; ++b)
				{
					FTDGlobePoint Q;
					Q.LatDeg = LatDeg + (NLat > 1 ? ((a + 0.5f) / NLat - 0.5f) * TexelLatDeg : 0.f);
					Q.LonDeg = LonDeg + (NLon > 1 ? ((b + 0.5f) / NLon - 0.5f) * TexelLonDeg : 0.f);
					const float D = FTDGlobeBake::GroundDistanceForAngle(Q.AngleFromPadDeg());
					const float Lon = FMath::DegreesToRadians(Q.LonDeg);
					const float X = FMath::Cos(Lon) * D, Y = FMath::Sin(Lon) * D;
					const FTDTerrainSample S = Field.Sample(X, Y, 2000.f);
					FLinearColor SB, SG;
					TDGlobeBake_Shade(bCinder, (S.Height - Field.GetStyle().BasinFloor) / 100.f, S.Slope, S.BroadMask, S.Height < Field.GetStyle().SeaLevel, SB, SG);
					SB *= 0.9f + 0.2f * S.FineMask;	// fine breakup, gentle (it was 0.78 + 0.44 and aliased)
					SB *= FMath::Lerp(0.62f, 1.25f, FMath::Clamp(S.BroadMask, 0.f, 1.f));	// broad light and dark by biome
					SumBase += SB;
					SumGlow += SG;
				}
			}
			LocalBase = SumBase / (float)(NLat * NLon);
			LocalGlow = SumGlow / (float)(NLat * NLon);
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
			TDGlobeBake_Texel(Field, bCinder, Lat, Lon, B, E, 180.f / H, 360.f / W);
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
	if (bCalibrationMarks)
	{
		return;
	}
	// Played (Nov 14, 1.6 radii over the landing zone): the texture has one mip, and near a pole a whole row of W texels
	// wraps a small circle, so per-texel differences along longitude alias into radial streaks. Each row is box-blurred
	// along longitude over the span that one equator texel covers there (1 / cos(lat) texels, up to a quarter turn): the
	// cap keeps its rings and patches and loses the streaks.
	ParallelFor(H, [&](int32 j)
	{
		const float Lat = 90.f - (j + 0.5f) / H * 180.f;
		const float Cos = FMath::Max(FMath::Cos(FMath::DegreesToRadians(Lat)), 1e-3f);
		const int32 R = FMath::Clamp(FMath::RoundToInt32((1.f / Cos - 1.f) * 0.5f), 0, W / 8);
		if (R <= 0)
		{
			return;
		}
		for (TArray<FColor>* Tex : { &OutBase, &OutEmissive })
		{
			TArray<FColor> Row;
			Row.Append(Tex->GetData() + j * W, W);
			int32 SumR = 0, SumG = 0, SumB = 0;
			for (int32 k = -R; k <= R; ++k)
			{
				const FColor& C = Row[((k % W) + W) % W];
				SumR += C.R; SumG += C.G; SumB += C.B;
			}
			const int32 N = 2 * R + 1;
			for (int32 i = 0; i < W; ++i)
			{
				(*Tex)[j * W + i] = FColor((uint8)(SumR / N), (uint8)(SumG / N), (uint8)(SumB / N), Row[i].A);
				const FColor& Out = Row[(((i - R) % W) + W) % W];
				const FColor& In = Row[(i + R + 1) % W];
				SumR += In.R - Out.R; SumG += In.G - Out.G; SumB += In.B - Out.B;
			}
		}
	});
}

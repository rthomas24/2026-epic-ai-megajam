// Triad Drift - World (Teammate 4). ENG-34 planet height field.

#include "TriadDrift/World/TDTerrainField.h"
#include "HAL/IConsoleManager.h"

namespace TDTerrainFieldImpl
{
	uint32 HashCell(int32 X, int32 Y, int32 Seed)
	{
		uint32 H = (uint32)X * 0x8da6b343u ^ (uint32)Y * 0xd8163841u ^ (uint32)Seed * 0xcb1ab31fu;
		H ^= H >> 13; H *= 0x5bd1e995u; H ^= H >> 15;
		return H;
	}

	float CellValue(int32 X, int32 Y, int32 Seed)
	{
		return (float)(HashCell(X, Y, Seed) & 0xffffffu) / (float)0xffffffu * 2.f - 1.f;
	}

	float SmootherStep(float T)
	{
		return T * T * T * (T * (T * 6.f - 15.f) + 10.f);
	}
}

namespace TDTerrainFieldFlats
{
	FCriticalSection& Lock() { static FCriticalSection L; return L; }
	TMap<uint8, TSharedPtr<const FTDCourseFlats, ESPMode::ThreadSafe>>& Map() { static TMap<uint8, TSharedPtr<const FTDCourseFlats, ESPMode::ThreadSafe>> M; return M; }
}

void FTDTerrainStyle::RegisterCourseFlats(ETDDestination Planet, const FTDCourseFlats& InFlats)
{
	FScopeLock Guard(&TDTerrainFieldFlats::Lock());
	TDTerrainFieldFlats::Map().Add((uint8)Planet, MakeShared<const FTDCourseFlats, ESPMode::ThreadSafe>(InFlats));
}

void FTDTerrainStyle::ClearCourseFlats(ETDDestination Planet)
{
	FScopeLock Guard(&TDTerrainFieldFlats::Lock());
	TDTerrainFieldFlats::Map().Remove((uint8)Planet);
}

float FTDCourseFlats::DistanceTo(float X, float Y) const
{
	const FVector2D P(X, Y);
	float Best = BIG_NUMBER;
	for (const FBox2D& B : Boxes)
	{
		const float DX = FMath::Max3((float)B.Min.X - X, 0.f, X - (float)B.Max.X), DY = FMath::Max3((float)B.Min.Y - Y, 0.f, Y - (float)B.Max.Y);
		Best = FMath::Min(Best, FMath::Sqrt(DX * DX + DY * DY));
	}
	for (const FSeg& S : Segs)
	{
		const FVector2D AB = S.B - S.A;
		const float L2 = (float)AB.SizeSquared();
		const float T = L2 > 1.f ? FMath::Clamp((float)FVector2D::DotProduct(P - S.A, AB) / L2, 0.f, 1.f) : 0.f;
		Best = FMath::Min(Best, FMath::Max(0.f, (float)FVector2D::Distance(P, S.A + AB * T) - S.HalfWidth));
	}
	return Best;
}

FTDTerrainStyle FTDTerrainStyle::ForPlanet(ETDDestination Planet)
{
	FTDTerrainStyle S;
	if (Planet == ETDDestination::Cinder)
	{
		S.Seed = 7331;
		S.BasinCentre = FVector2D(8500.f, -500.f);			// T5's Cinder ground box x -3100..20100, y -10600..9600
		S.BasinHalfExtent = FVector2D(11600.f, 10100.f);
		S.HillHeight = 12000.f;
		S.HillScale = 160000.f;
		S.MountainHeight = 125000.f;
		S.MountainScale = 260000.f;
		S.Sharpness = 0.95f;
		S.CalderaAmount = 0.6f;
		S.RiverDepth = 9000.f;
		S.Warp = 90000.f;
		S.GullyAmount = 0.2f;
		S.ErosionAmount = 0.05f;	// sharp ash-cut gullies on the cones and ranges
		S.ErosionScale = 60000.f;
		// The volcano: north-east of the course, tallest thing on the planet, glowing crater.
		S.HeroCentre = FVector2D(560000.f, -420000.f);
		S.HeroRadius = 330000.f;
		S.HeroHeight = 250000.f;
		S.bVolcano = true;
		S.RiftBearing = 60.f; S.MesaBearing = 150.f; S.BasinBearing = 230.f; S.PlateauBearing = 320.f;
		S.SeaLevel = -12000.f;
		S.bLavaSea = true;
		S.HorizonRise = 320000.f;
	}
	else
	{
		S.Seed = 4242;
		S.BasinCentre = FVector2D(5500.f, -250.f);			// T5's Verdance ground box x -3100..14100, y -10600..10100
		S.BasinHalfExtent = FVector2D(8600.f, 10350.f);
		S.HillHeight = 22000.f;
		S.HillScale = 240000.f;
		S.MountainHeight = 135000.f;
		S.MountainScale = 300000.f;
		S.Sharpness = 0.85f;
		S.Warp = 140000.f;
		S.GullyAmount = 0.16f;
		S.ErosionAmount = 0.045f;	// rain-cut drainage on every range
		S.ErosionScale = 80000.f;
		// The massif: a great ridged mountain east of the course.
		S.HeroCentre = FVector2D(620000.f, 300000.f);
		S.HeroRadius = 380000.f;
		S.HeroHeight = 250000.f;
		S.RiftBearing = 110.f; S.MesaBearing = 200.f; S.BasinBearing = 290.f; S.PlateauBearing = 20.f;
		S.SeaLevel = -14000.f;
		S.HorizonRise = 300000.f;
	}
	// Ryan (#194): mountains all around the pad, close and tall, and relief underfoot once the course has said where
	// its walked ground is.
	S.bCinderGround = Planet == ETDDestination::Cinder;
	static IConsoleVariable* ReliefVar = IConsoleManager::Get().RegisterConsoleVariable(TEXT("td.Terrain.Relief"), 1,
		TEXT("#194: 1 = relief underfoot and the mountain ring (default); 0 = the flat ground before them (read when a planet loads)."), ECVF_Default);
	if (ReliefVar && ReliefVar->GetInt() == 0) { return S; }
	S.RingRidgeHeight = S.bCinderGround ? 32000.f : 26000.f;	// x (0.4..1.25) crest: about 100-400 m (Lead: 150-400 m)
	{
		FScopeLock Guard(&TDTerrainFieldFlats::Lock());
		if (const TSharedPtr<const FTDCourseFlats, ESPMode::ThreadSafe>* F = TDTerrainFieldFlats::Map().Find((uint8)Planet))
		{
			S.Flats = *F;
			const FVector2D Half = FVector2D(S.BasinHalfExtent) + FVector2D(S.MicroMargin, S.MicroMargin);
			S.MicroRegion = FBox2D(FVector2D(S.BasinCentre) - Half, FVector2D(S.BasinCentre) + Half);
		}
	}
	return S;
}

float FTDTerrainField::Noise2D(float X, float Y, int32 Seed)
{
	using namespace TDTerrainFieldImpl;
	const int32 X0 = FMath::FloorToInt(X);
	const int32 Y0 = FMath::FloorToInt(Y);
	const float TX = SmootherStep(X - X0);
	const float TY = SmootherStep(Y - Y0);
	const float A = CellValue(X0, Y0, Seed);
	const float B = CellValue(X0 + 1, Y0, Seed);
	const float C = CellValue(X0, Y0 + 1, Seed);
	const float D = CellValue(X0 + 1, Y0 + 1, Seed);
	return FMath::Lerp(FMath::Lerp(A, B, TX), FMath::Lerp(C, D, TX), TY);
}

float FTDTerrainField::Fbm(float X, float Y, int32 Octaves, int32 Seed) const
{
	float Sum = 0.f, Amp = 0.5f, Freq = 1.f, Norm = 0.f;
	for (int32 i = 0; i < Octaves; ++i)
	{
		Sum += Amp * Noise2D(X * Freq, Y * Freq, Seed + i * 101);
		Norm += Amp;
		Amp *= 0.5f;
		Freq *= 2.03f;
	}
	return Sum / FMath::Max(Norm, KINDA_SMALL_NUMBER);	// -1..1
}

float FTDTerrainField::Ridged(float X, float Y, int32 Octaves, int32 Seed, float Sharp) const
{
	// Ridged multifractal: 1 - |noise| makes crests; squaring sharpens them; each octave weighted by the last.
	float Sum = 0.f, Amp = 0.5f, Freq = 1.f, Weight = 1.f, Norm = 0.f;
	const float Power = FMath::Lerp(1.4f, 2.6f, FMath::Clamp(Sharp, 0.f, 1.f));
	for (int32 i = 0; i < Octaves; ++i)
	{
		float N = 1.f - FMath::Abs(Noise2D(X * Freq, Y * Freq, Seed + 7 + i * 131));
		N = FMath::Pow(FMath::Max(N, 0.f), Power) * Weight;
		Weight = FMath::Clamp(N * 1.6f, 0.f, 1.f);
		Sum += N * Amp;
		Norm += Amp;
		Amp *= 0.5f;
		Freq *= 2.1f;
	}
	return Sum / FMath::Max(Norm, KINDA_SMALL_NUMBER);	// 0..1
}

float FTDTerrainField::BasinWeightAt(float X, float Y, float Blend) const
{
	const float DX = FMath::Max(0.f, FMath::Abs(X - (float)Style.BasinCentre.X) - (float)Style.BasinHalfExtent.X);
	const float DY = FMath::Max(0.f, FMath::Abs(Y - (float)Style.BasinCentre.Y) - (float)Style.BasinHalfExtent.Y);
	return TDTerrainFieldImpl::SmootherStep(FMath::Clamp(FMath::Sqrt(DX * DX + DY * DY) / FMath::Max(1.f, Blend), 0.f, 1.f));
}

float FTDTerrainField::BasinWeight(float X, float Y) const
{
	return BasinWeightAt(X, Y, Style.BasinBlend);
}

float FTDTerrainField::RawHeight(float X, float Y) const
{
	const int32 S = Style.Seed;
	// Domain warp: bend coordinates by low-frequency noise so ranges curve naturally.
	const float WX = X + Style.Warp * Noise2D(X / 500000.f, Y / 500000.f, S + 11);
	const float WY = Y + Style.Warp * Noise2D(X / 500000.f + 31.7f, Y / 500000.f - 12.3f, S + 13);

	const float Hills = Fbm(WX / Style.HillScale, WY / Style.HillScale, 5, S + 1) * 0.5f + 0.5f;	// 0..1
	// Where mountains rise: a broad mask so ranges cluster with valleys between them.
	// Ranges cluster (a broad mask) but never vanish entirely, so every bearing from the course meets mountains.
	const float RangeMask = 0.35f + 0.65f * FMath::SmoothStep(-0.3f, 0.35f, Fbm(WX / (Style.MountainScale * 1.6f), WY / (Style.MountainScale * 1.6f), 3, S + 3));
	const float Mount = Ridged(WX / Style.MountainScale, WY / Style.MountainScale, 6, S + 5, Style.Sharpness);
	float H = Hills * Style.HillHeight + Mount * RangeMask * Style.MountainHeight;
	// A ring of foothills rising from the basin edge: the course sits in a valley with peaks all round.
	const float DX = FMath::Max(0.f, FMath::Abs(X) - Style.BasinHalfExtent.X), DY = FMath::Max(0.f, FMath::Abs(Y) - Style.BasinHalfExtent.Y);
	const float Ring = FMath::SmoothStep(Style.BasinBlend * 0.2f, Style.BasinBlend * 1.6f, FMath::Sqrt(DX * DX + DY * DY));
	H += Ring * Style.MountainHeight * 0.5f * (0.45f + 0.55f * Mount);
	// Gullies: a second, finer ridged octave carved into the ranges, so silhouettes break up.
	if (Style.GullyAmount > 0.f)
	{
		const float G = Ridged(WX / (Style.MountainScale * 0.22f), WY / (Style.MountainScale * 0.22f), 3, S + 91, 1.f);
		H += (G - 0.45f) * Style.GullyAmount * Style.MountainHeight * RangeMask * FMath::Clamp(Mount * 1.5f, 0.f, 1.f);
	}

	// Named regions (each fades out toward the course basin and across its sector edges).
	{
		const float Rift = RegionWeight(X, Y, Style.RiftBearing);
		if (Rift > 0.f)
		{
			// A meandering canyon: carve along the zero contour of a slow noise.
			const float RN = Fbm(WX / 180000.f, WY / 180000.f, 3, S + 101);
			const float Canyon = FMath::Exp(-FMath::Square(RN * 6.f));
			H -= Rift * Canyon * Style.RiftDepth;
		}
		const float Mesa = RegionWeight(X, Y, Style.MesaBearing);
		if (Mesa > 0.f)
		{
			// Terraces: flat tops and steep risers.
			const float Steps = H / Style.MesaStep;
			const float Base = FMath::FloorToFloat(Steps);
			const float Frac = Steps - Base;
			const float Terraced = (Base + FMath::SmoothStep(0.75f, 0.95f, Frac)) * Style.MesaStep;
			H = FMath::Lerp(H, Terraced, Mesa);
		}
		const float Sea = RegionWeight(X, Y, Style.BasinBearing);
		if (Sea > 0.f)
		{
			// A broad depression under the sea level, shores rising out of it.
			const float D = FVector2D::Distance(FVector2D(X, Y), SeaCentre()) / FMath::Max(1.f, SeaRadius());
			const float Bowl = FMath::Clamp(1.f - D, 0.f, 1.f);
			H = FMath::Lerp(H, FMath::Min(H, Style.SeaLevel - Style.BasinDepth * Bowl + (1.f - Bowl) * H * 0.4f), Sea * FMath::SmoothStep(0.f, 0.35f, Bowl));
		}
		const float Plateau = RegionWeight(X, Y, Style.PlateauBearing);
		if (Plateau > 0.f)
		{
			// Raised flat country with escarpment edges and a little roll on top.
			const float Top = Style.PlateauHeight + Hills * Style.HillHeight * 0.4f;
			H = FMath::Lerp(H, FMath::Max(H * 0.35f, Top), FMath::SmoothStep(0.25f, 0.55f, Plateau));
		}
	}

	if (Style.CalderaAmount > 0.f)
	{
		// Calderas: on the high ground, a ring rim with a sunken bowl.
		const float CellSize = 900000.f;
		const float CX = FMath::FloorToFloat(X / CellSize), CY = FMath::FloorToFloat(Y / CellSize);
		const float OX = (Noise2D(CX * 3.1f, CY * 3.1f, S + 41) * 0.35f + 0.5f + CX) * CellSize;
		const float OY = (Noise2D(CX * 3.1f + 9.f, CY * 3.1f + 9.f, S + 43) * 0.35f + 0.5f + CY) * CellSize;
		const float R = FVector2D::Distance(FVector2D(X, Y), FVector2D(OX, OY)) / 160000.f;
		const float Rim = FMath::Exp(-FMath::Square((R - 1.f) * 3.f));
		const float Bowl = FMath::Clamp(1.f - R, 0.f, 1.f);
		H += Style.CalderaAmount * Style.MountainHeight * 0.45f * (Rim - Bowl * 0.8f) * RangeMask;
	}
	if (Style.RiverDepth > 0.f)
	{
		// Lava-river valleys: carve along the zero contour of a meandering noise.
		const float RN = RiverNoise(X, Y);
		const float River = FMath::Exp(-FMath::Square(RN * 9.f));
		H -= River * Style.RiverDepth * (0.4f + 0.6f * (1.f - RangeMask));
	}

	// The hero peak.
	{
		const float D = FVector2D::Distance(FVector2D(X, Y), Style.HeroCentre) / FMath::Max(1.f, Style.HeroRadius);
		if (D < 1.f)
		{
			if (Style.bVolcano)
			{
				// A cone (steepening toward the top) with a crater bowl.
				const float Cone = FMath::Pow(1.f - D, 1.6f);
				float V = Cone * Style.HeroHeight * (0.92f + 0.08f * Mount);
				// The crater: the cone stops at the rim height and a bowl sinks inside it.
				const float CraterD = FVector2D::Distance(FVector2D(X, Y), Style.HeroCentre) / FMath::Max(1.f, Style.CraterRadius);
				if (CraterD < 1.f)
				{
					const float RimH = FMath::Pow(1.f - Style.CraterRadius / FMath::Max(1.f, Style.HeroRadius), 1.6f) * Style.HeroHeight;
					V = RimH - Style.CraterDepth * (1.f - CraterD * CraterD);
					H = V;	// the bowl is carved, whatever stood there
				}
				else
				{
					H = FMath::Max(H, V);
				}
			}
			else
			{
				// An alpine massif: a broken summit line of arêtes and spires, buttress ridges running down into the
				// jungle. A falloff envelope times sharp ridged noise at the massif's own scale, plus radial buttresses.
				const FVector2D Rel = FVector2D(X, Y) - Style.HeroCentre;
				const float Env = FMath::Pow(FMath::Clamp(1.f - D, 0.f, 1.f), 1.25f);
				const float Alp = Ridged(Rel.X / (Style.HeroRadius * 0.32f), Rel.Y / (Style.HeroRadius * 0.32f), 5, S + 121, 1.f);
				const float Ang = FMath::Atan2(Rel.Y, Rel.X);
				const float Buttress = FMath::Pow(FMath::Abs(FMath::Cos(Ang * 3.5f + Noise2D(Rel.X / 90000.f, Rel.Y / 90000.f, S + 123) * 1.2f)), 6.f);
				const float Spires = FMath::Pow(FMath::Max(0.f, Noise2D(Rel.X / 26000.f, Rel.Y / 26000.f, S + 125)), 3.f);
				const float Shape = Env * (0.45f + 0.4f * Alp + 0.25f * Buttress * (1.f - Env * 0.6f)) + Spires * Env * Env * 0.25f;
				H = FMath::Max(H, FMath::Min(Shape, 1.f) * Style.HeroHeight);
			}
		}
	}

	if (Style.ErosionAmount > 0.f)
	{
		// Drainage: the gully pattern follows the downhill direction of the large forms (finite differences over 300 m);
		// on the hero peak, straight down its flanks (away from the summit)
		const float E = 30000.f;
		FVector2D Grad((LargeForms(X + E, Y) - LargeForms(X - E, Y)) / (2.f * E), (LargeForms(X, Y + E) - LargeForms(X, Y - E)) / (2.f * E));
		float Weight = RangeMask;
		const FVector2D Rel = FVector2D(X, Y) - Style.HeroCentre;
		const float HD = Rel.Size() / FMath::Max(1.f, Style.HeroRadius);
		if (HD < 1.f && HD > 0.01f)
		{
			const float HeroW = FMath::SmoothStep(1.f, 0.6f, HD);
			Grad = FMath::Lerp(Grad, -Rel / Rel.Size() * (Style.HeroHeight / Style.HeroRadius) * 1.4f, HeroW);
			Weight = FMath::Lerp(Weight, 1.f, HeroW);
		}
		H += Erosion(X, Y, Grad) * Weight;
	}

	// Beyond the walkable core, the ground rises into ranges that close the horizon.
	const float Edge = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
	const float Rise = FMath::Clamp((Edge - Style.CoreHalfSize) / FMath::Max(1.f, Style.FarHalfSize - Style.CoreHalfSize), 0.f, 1.f);
	if (Rise > 0.f)
	{
		// A layered skyline: a nearer band of ranges, then a taller, jagged far wall.
		const float Near = FMath::SmoothStep(0.f, 0.35f, Rise) * (0.5f + 0.5f * Mount);
		const float FarWall = FMath::SmoothStep(0.4f, 0.9f, Rise) * (0.45f + 0.55f * Ridged(WX / 900000.f, WY / 900000.f, 4, S + 111, Style.Sharpness));
		H += (Near * 0.45f + FarWall) * Style.HorizonRise;
	}
	return H;
}

float FTDTerrainField::LargeForms(float X, float Y) const
{
	const int32 S = Style.Seed;
	const float WX = X + Style.Warp * Noise2D(X / 500000.f, Y / 500000.f, S + 11);
	const float WY = Y + Style.Warp * Noise2D(X / 500000.f + 31.7f, Y / 500000.f - 12.3f, S + 13);
	const float Hills = Fbm(WX / Style.HillScale, WY / Style.HillScale, 2, S + 1) * 0.5f + 0.5f;
	const float Mount = Ridged(WX / Style.MountainScale, WY / Style.MountainScale, 3, S + 5, Style.Sharpness);
	return Hills * Style.HillHeight + Mount * Style.MountainHeight;
}

float FTDTerrainField::Erosion(float X, float Y, const FVector2D& Grad) const
{
	using namespace TDTerrainFieldImpl;
	// "Phacelle" gullies: in each cell of a jittered grid a cosine wave whose crests run along the downhill direction;
	// neighbouring cells blend by distance. The wave's zero crossings become sharp crests (1 - 2|w|) and its troughs the
	// rounded channels between them. Strength follows the slope: none on flats, full on mountainsides.
	const float Slope = Grad.Size();
	const float SlopeW = FMath::SmoothStep(0.04f, 0.45f, Slope);
	if (SlopeW <= 0.f) { return 0.f; }
	const FVector2D Down = Grad / FMath::Max(Slope, 1e-4f);
	const FVector2D Across(-Down.Y, Down.X);
	float Sum = 0.f, Amp = 1.f, Norm = 0.f;
	float Cell = Style.ErosionScale;
	for (int32 Oct = 0; Oct < 2; ++Oct)
	{
		const float CX = FMath::FloorToFloat(X / Cell), CY = FMath::FloorToFloat(Y / Cell);
		float Wave = 0.f, WSum = 0.f;
		for (int32 j = -1; j <= 1; ++j)
		{
			for (int32 i = -1; i <= 1; ++i)
			{
				const int32 IX = (int32)CX + i, IY = (int32)CY + j;
				const float PX = (IX + 0.5f + 0.4f * CellValue(IX, IY, Style.Seed + 701 + Oct)) * Cell;
				const float PY = (IY + 0.5f + 0.4f * CellValue(IX, IY, Style.Seed + 709 + Oct)) * Cell;
				const float DX = X - PX, DY = Y - PY;
				const float W = FMath::Exp(-(DX * DX + DY * DY) / (Cell * Cell * 0.55f));
				const float Phase = (DX * Across.X + DY * Across.Y) / Cell * 2.f * PI * 1.6f;
				Wave += FMath::Cos(Phase) * W;
				WSum += W;
			}
		}
		const float Wn = Wave / FMath::Max(WSum, 1e-4f);	// -1..1, softened where cells disagree
		Sum += (1.f - 2.f * FMath::Abs(Wn)) * Amp;
		Norm += Amp;
		Amp *= 0.45f;
		Cell *= 0.4f;
	}
	return Sum / Norm * SlopeW * Style.ErosionAmount * Style.MountainHeight;
}

float FTDTerrainField::RiverNoise(float X, float Y) const
{
	const int32 S = Style.Seed;
	const float WX = X + Style.Warp * Noise2D(X / 500000.f, Y / 500000.f, S + 11);
	const float WY = Y + Style.Warp * Noise2D(X / 500000.f + 31.7f, Y / 500000.f - 12.3f, S + 13);
	return Fbm(WX / 300000.f, WY / 300000.f, 3, S + 61);
}

float FTDTerrainField::ReliefMeters() const
{
	const float Low = FMath::Min(Style.SeaLevel - Style.BasinDepth, -Style.RiftDepth);
	return (Style.HeroHeight - Low) / 100.f;
}

float FTDTerrainField::RegionWeight(float X, float Y, float Bearing) const
{
	const float Deg = FMath::RadiansToDegrees(FMath::Atan2(Y, X));
	const float Diff = FMath::Abs(FMath::FindDeltaAngleDegrees(Deg, Bearing));
	const float Sector = 1.f - FMath::SmoothStep(45.f - Style.RegionBlendDeg * 0.5f, 45.f + Style.RegionBlendDeg * 0.5f, Diff);
	// Regions start beyond the foothill ring around the course.
	const float R = (float)FVector2D(X, Y).Size();
	const float Edge = (float)Style.BasinHalfExtent.X;
	const float Out = FMath::SmoothStep(Edge + Style.BasinBlend * 0.8f, Edge + Style.BasinBlend * 1.8f, R);
	return Sector * Out;
}

FVector2D FTDTerrainField::SeaCentre() const
{
	const float A = FMath::DegreesToRadians(Style.BasinBearing);
	return FVector2D(FMath::Cos(A), FMath::Sin(A)) * Style.CoreHalfSize * 0.72f;
}

float FTDTerrainField::Height(float X, float Y) const
{
	const float W = BasinWeight(X, Y);
	if (W <= 0.f) { return Style.BasinFloor + (Style.HasMicroRelief() ? MicroRelief(X, Y) : 0.f); }
	// Never below the course floor near the course: the course slab's edge would show as a raised table (Verdance
	// from 5 km). Depressions (seas, canyons) only start well beyond the blend.
	const float Raw = RawHeight(X, Y);
	const float DX = FMath::Max(0.f, FMath::Abs(X - (float)Style.BasinCentre.X) - (float)Style.BasinHalfExtent.X);
	const float DY = FMath::Max(0.f, FMath::Abs(Y - (float)Style.BasinCentre.Y) - (float)Style.BasinHalfExtent.Y);
	const float Near = 1.f - FMath::SmoothStep(250000.f, 450000.f, FMath::Sqrt(DX * DX + DY * DY));	// floored to 2.5 km beyond, gone by 4.5
	const float Floored = FMath::Lerp(Raw, FMath::Max(Raw, Style.BasinFloor + 300.f * W), Near);
	float H = FMath::Lerp(Style.BasinFloor, Floored, W);
	// Ryan (#194): a ring of ridgelines close around the pad (rising from RingInner, tallest about RingPeak, gone by
	// RingOuter), so every view from the pad is framed by peaks. The valley clamps below still cut the arrival notch
	// (-X) and the hero sightline.
	if (Style.RingRidgeHeight > 0.f)
	{
		// Played (turn 15): measured from the pad the ring stood as one sheer wall at the course's edge. It is measured
		// from the walked region's edge instead (foothills first, the crest well beyond), and broken into massifs and
		// saddles so it reads as mountains, not a canyon wall.
		float R = FMath::Sqrt(X * X + Y * Y);
		if (Style.HasMicroRelief())
		{
			const FBox2D& Rg = Style.MicroRegion;
			const float EX = FMath::Max3((float)Rg.Min.X - X, 0.f, X - (float)Rg.Max.X), EY = FMath::Max3((float)Rg.Min.Y - Y, 0.f, Y - (float)Rg.Max.Y);
			R = FMath::Sqrt(EX * EX + EY * EY) + Style.RingInner * 0.6f;
		}
		const float Rise = FMath::SmoothStep(Style.RingInner, Style.RingPeak, R) * (1.f - FMath::SmoothStep(Style.RingPeak + 15000.f, Style.RingOuter, R));
		if (Rise > 0.f)
		{
			const float Massif = FMath::SmoothStep(0.25f, 0.7f, Fbm(X / 70000.f, Y / 70000.f, 3, Style.Seed + 307));
			const float Crest = 0.4f + 0.85f * Ridged(X / 32000.f, Y / 32000.f, 4, Style.Seed + 301, Style.Sharpness);
			H += Rise * Style.RingRidgeHeight * Crest * (0.3f + 0.7f * Massif);
		}
	}
	// A valley that frames the hero peak from the pad: toward it, the ground stays under the sightline to its summit.
	{
		const FVector2D Q(X, Y);
		const float HeroDist = (float)Style.HeroCentre.Size();
		const float R = (float)Q.Size();
		const float Foot = HeroDist - Style.HeroRadius * 0.85f;
		if (R > 1.f && R < Foot && HeroDist > 1.f)
		{
			const float Off = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp((float)FVector2D::DotProduct(Q / R, Style.HeroCentre / HeroDist), -1.f, 1.f)));
			if (Off < 9.f)
			{
				const float Sight = 200.f + (Style.HeroHeight * 0.55f - 200.f) * (R / HeroDist) - 6000.f;
				const float Edge = (1.f - FMath::SmoothStep(5.f, 9.f, Off)) * (1.f - FMath::SmoothStep(Foot * 0.85f, Foot, R));	// SmoothStep needs A < B
				H = FMath::Lerp(H, FMath::Min(H, FMath::Max(Sight, Style.BasinFloor)), Edge);
			}
		}
	}
	// The arrival corridor: under the glide slope (27 deg up from the pad) inside a cone behind the pad (-X).
	if (X < 0.f)
	{
		const float R = FMath::Sqrt(X * X + Y * Y);
		const float Off = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Y), -X));
		if (R < Style.ApproachLength && Off < Style.ApproachHalfAngleDeg)
		{
			const float Glide = R * 0.51f - Style.ApproachClearance;	// tan(27 deg)
			const float Edge = (1.f - FMath::SmoothStep(Style.ApproachHalfAngleDeg * 0.7f, Style.ApproachHalfAngleDeg, Off)) * (1.f - FMath::SmoothStep(Style.ApproachLength * 0.8f, Style.ApproachLength, R));
			H = FMath::Lerp(H, FMath::Min(H, FMath::Max(Glide, Style.BasinFloor)), Edge);
		}
	}
	// Ryan (#194): relief underfoot (zero on the course's walked ground and outside the region).
	if (Style.HasMicroRelief())
	{
		H += MicroRelief(X, Y);
	}
	return H;
}

FTDTerrainSample FTDTerrainField::Sample(float X, float Y, float Step) const
{
	FTDTerrainSample S;
	S.Height = Height(X, Y);
	const float HX = Height(X + Step, Y) - Height(X - Step, Y);
	const float HY = Height(X, Y + Step) - Height(X, Y - Step);
	const FVector N = FVector(-HX, -HY, 2.f * Step).GetSafeNormal();
	S.Normal = N;
	S.Slope = FMath::Clamp(1.f - N.Z, 0.f, 1.f);
	S.Height01 = FMath::Clamp((S.Height - Style.BasinFloor) / FMath::Max(1.f, MaxHeight()), 0.f, 1.f);
	// Cinder: the material's lava follows the carved river beds (Lead's M_TD_Terrain, ValleyWidth ~0.35);
	// elsewhere a broad biome mask.
	S.BroadMask = Style.RiverDepth > 0.f
		? FMath::Clamp(0.5f + RiverNoise(X, Y) * 4.5f, 0.f, 1.f)
		: Fbm(X / 400000.f, Y / 400000.f, 3, Style.Seed + 71) * 0.5f + 0.5f;
	S.FineMask = Noise2D(X / 6000.f, Y / 6000.f, Style.Seed + 73) * 0.5f + 0.5f;
	// Cinder (#194): the cracks in the course ground are lava for the material (the river-bed mask's 0.5 band).
	if (Style.bCinderGround && Style.HasMicroRelief())
	{
		float Crack = 0.f;
		MicroRelief(X, Y, &Crack);
		if (Crack > 0.05f) { S.BroadMask = FMath::Lerp(S.BroadMask, 0.5f, FMath::Clamp(Crack * 1.4f, 0.f, 1.f)); }
	}
	return S;
}

float FTDTerrainField::MicroWeight(float X, float Y) const
{
	if (!Style.HasMicroRelief()) { return 0.f; }
	const FBox2D& R = Style.MicroRegion;
	const float In = FMath::Min(FMath::Min(X - (float)R.Min.X, (float)R.Max.X - X), FMath::Min(Y - (float)R.Min.Y, (float)R.Max.Y - Y));
	return FMath::SmoothStep(0.f, 4000.f, In);	// fades to 0 over the region's outer 40 m (meets the coarse chunks)
}

float FTDTerrainField::MicroRelief(float X, float Y, float* OutCrack) const
{
	if (OutCrack) { *OutCrack = 0.f; }
	const float Edge = MicroWeight(X, Y);
	if (Edge <= 0.f) { return 0.f; }
	// Flat on the course's walked ground (pad, routes at walking width, sites, structures, spawns), rising over 6 m.
	const float Flat = FMath::SmoothStep(0.f, 600.f, Style.Flats->DistanceTo(X, Y));
	if (Flat <= 0.f) { return 0.f; }
	const int32 S = Style.Seed + 5001;
	float H = 0.f;
	// Rolling ground: never one plane.
	H += Fbm(X / 2600.f, Y / 2600.f, 3, S + 1) * 55.f + Fbm(X / 9000.f, Y / 9000.f, 2, S + 2) * 140.f;
	// Low ridges and gullies (ridged noise about 60 m across, +-1.3 m).
	H += (Ridged(X / 6000.f, Y / 6000.f, 2, S + 3, Style.bCinderGround ? 0.8f : 0.4f) - 0.5f) * 260.f;
	// Features on a 25 m grid: craters / root hollows (with a rim), mounds, pits / sinkholes.
	const float Cell = 4500.f;	// a feature is at most 40 m across: the 3 x 3 neighbourhood covers it
	const int32 CX = FMath::FloorToInt32(X / Cell), CY = FMath::FloorToInt32(Y / Cell);
	for (int32 dy = -1; dy <= 1; ++dy)
	{
		for (int32 dx = -1; dx <= 1; ++dx)
		{
			const uint32 Hash = HashCombine(GetTypeHash(CX + dx), HashCombine(GetTypeHash(CY + dy), GetTypeHash(S)));
			FRandomStream R((int32)Hash);
			const float Kind = R.FRand();
			if (Kind > 0.82f) { continue; }	// some cells are just ground
			const FVector2D Ctr((CX + dx + R.FRandRange(0.15f, 0.85f)) * Cell, (CY + dy + R.FRandRange(0.15f, 0.85f)) * Cell);
			const float D = (float)FVector2D::Distance(FVector2D(X, Y), Ctr);
			if (Kind < 0.42f)
			{
				// Crater / root hollow (Lead): 15-40 m across, 2-6 m deep, with a raised lip; a few small divots too.
				const bool bSmall = R.FRand() < 0.35f;
				const float Rad = bSmall ? R.FRandRange(150.f, 600.f) : R.FRandRange(750.f, 2000.f);
				const float Depth = bSmall ? R.FRandRange(50.f, 150.f) : FMath::Lerp(200.f, 600.f, (Rad - 750.f) / 1250.f) * R.FRandRange(0.8f, 1.f);
				const float U = D / Rad;
				if (U < 1.f) { H -= Depth * (1.f - U * U); }
				H += Depth * 0.22f * FMath::Exp(-FMath::Square((U - 1.08f) / 0.22f));
			}
			else if (Kind < 0.70f)
			{
				// Mound / outcrop: 10-36 m across, 1-4 m tall.
				const float Rad = R.FRandRange(500.f, 1800.f), Tall = R.FRandRange(100.f, 400.f);
				const float U = D / Rad;
				if (U < 1.f) { H += Tall * FMath::Square(1.f - U * U); }
			}
			else if (Kind < 0.78f)
			{
				// Pit / sinkhole: narrow and deep (3-7 m across, 3-6 m deep), steep walls.
				const float Rad = R.FRandRange(150.f, 350.f), Depth = R.FRandRange(300.f, 600.f);
				const float U = D / Rad;
				if (U < 1.f) { H -= Depth * (1.f - FMath::Pow(U, 4.f)); }
			}
			// else: a cell with only the rolling ground
		}
	}
	// Verdance: soft terraces on the rises. Cinder: cracks along a noise contour (lava, see Sample).
	if (!Style.bCinderGround)
	{
		const float Step = 120.f;
		const float T = H / Step, Base = FMath::FloorToFloat(T);
		H = FMath::Lerp(H, (Base + FMath::SmoothStep(0.6f, 0.95f, T - Base)) * Step, 0.45f);
	}
	else
	{
		const float N = Noise2D(X / 3500.f, Y / 3500.f, S + 9) + 0.5f * Noise2D(X / 1200.f, Y / 1200.f, S + 10);
		const float Crack = FMath::Exp(-FMath::Square(N * 14.f));
		H -= Crack * 110.f;
		if (OutCrack) { *OutCrack = Crack * Flat * Edge; }
	}
	return H * Flat * Edge * Style.MicroAmount;
}

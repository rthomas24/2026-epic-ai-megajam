// Triad Drift - World (Teammate 4). ENG-34 planet height field.

#include "TriadDrift/World/TDTerrainField.h"

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

FTDTerrainStyle FTDTerrainStyle::ForPlanet(ETDDestination Planet)
{
	FTDTerrainStyle S;
	if (Planet == ETDDestination::Cinder)
	{
		S.Seed = 7331;
		S.HillHeight = 12000.f;
		S.HillScale = 160000.f;
		S.MountainHeight = 110000.f;
		S.MountainScale = 240000.f;
		S.Sharpness = 0.9f;
		S.CalderaAmount = 0.6f;
		S.RiverDepth = 9000.f;
		S.Warp = 90000.f;
	}
	else
	{
		S.Seed = 4242;
		S.HillHeight = 22000.f;
		S.HillScale = 240000.f;
		S.MountainHeight = 90000.f;
		S.MountainScale = 320000.f;
		S.Sharpness = 0.35f;
		S.Warp = 140000.f;
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

float FTDTerrainField::BasinWeight(float X, float Y) const
{
	const float DX = FMath::Max(0.f, FMath::Abs(X) - Style.BasinHalfExtent.X);
	const float DY = FMath::Max(0.f, FMath::Abs(Y) - Style.BasinHalfExtent.Y);
	const float D = FMath::Sqrt(DX * DX + DY * DY);
	const float T = FMath::Clamp(D / FMath::Max(1.f, Style.BasinBlend), 0.f, 1.f);
	return TDTerrainFieldImpl::SmootherStep(T);
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
	const float Ring = FMath::SmoothStep(Style.BasinBlend * 0.5f, Style.BasinBlend * 2.2f, FMath::Sqrt(DX * DX + DY * DY));
	H += Ring * Style.MountainHeight * 0.5f * (0.45f + 0.55f * Mount);

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
		const float RN = Fbm(WX / 300000.f, WY / 300000.f, 3, S + 61);
		const float River = FMath::Exp(-FMath::Square(RN * 9.f));
		H -= River * Style.RiverDepth * (0.4f + 0.6f * (1.f - RangeMask));
	}

	// Beyond the walkable core, the ground rises into ranges that close the horizon.
	const float Edge = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
	const float Rise = FMath::Clamp((Edge - Style.CoreHalfSize) / FMath::Max(1.f, Style.FarHalfSize - Style.CoreHalfSize), 0.f, 1.f);
	H += TDTerrainFieldImpl::SmootherStep(Rise) * Style.HorizonRise * (0.6f + 0.4f * Mount);
	return H;
}

float FTDTerrainField::Height(float X, float Y) const
{
	const float W = BasinWeight(X, Y);
	if (W <= 0.f) { return Style.BasinFloor; }
	return FMath::Lerp(Style.BasinFloor, RawHeight(X, Y), W);
}

FTDTerrainSample FTDTerrainField::Sample(float X, float Y, float Step) const
{
	FTDTerrainSample S;
	S.Height = Height(X, Y);
	const float HX = Height(X + Step, Y) - Height(X - Step, Y);
	const float HY = Height(X, Y + Step) - Height(X, Y - Step);
	const FVector N = FVector(-HX, -HY, 2.f * Step).GetSafeNormal();
	S.Slope = FMath::Clamp(1.f - N.Z, 0.f, 1.f);
	S.Height01 = FMath::Clamp((S.Height - Style.BasinFloor) / FMath::Max(1.f, MaxHeight()), 0.f, 1.f);
	S.BroadMask = Fbm(X / 400000.f, Y / 400000.f, 3, Style.Seed + 71) * 0.5f + 0.5f;
	S.FineMask = Noise2D(X / 6000.f, Y / 6000.f, Style.Seed + 73) * 0.5f + 0.5f;
	return S;
}

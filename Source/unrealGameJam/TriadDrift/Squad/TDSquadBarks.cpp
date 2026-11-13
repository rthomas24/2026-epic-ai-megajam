// Triad Drift - Squad (Teammate 3). Squad bark limiter (see header).

#include "TriadDrift/Squad/TDSquadBarks.h"

FName TDBarkIds::ShipDownFor(ETDShipId Ship)
{
	switch (Ship)
	{
	case ETDShipId::Lancer:  return ShipDownLancer;
	case ETDShipId::Bastion: return ShipDownBastion;
	default:                 return ShipDownRift;
	}
}

FName TDBarkIds::SpeakerFor(ETDShipId Ship)
{
	switch (Ship)
	{
	case ETDShipId::Lancer:  return TEXT("Kato");
	case ETDShipId::Bastion: return TEXT("Moss");
	default:                 return TEXT("Wren");
	}
}

FTDBarkTuning::FTDBarkTuning()
{
	CooldownById.Add(TDBarkIds::SwitchIn, 6.f);
	CooldownById.Add(TDBarkIds::FocusAck, 5.f);
	CooldownById.Add(TDBarkIds::WingmanKill, 12.f);
	CooldownById.Add(TDBarkIds::LowHull, 20.f);
	CooldownById.Add(TDBarkIds::Respawn, 8.f);
	CooldownById.Add(TDBarkIds::SquadWipe, 5.f);
}

bool FTDBarkLimiter::TryConsume(const FTDBarkTuning& Tuning, FName BarkId, ETDBarkPriority Priority, double RealNow)
{
	while (Recent.Num() > 0 && RealNow - Recent[0] > Tuning.WindowSeconds)
	{
		Recent.RemoveAt(0, EAllowShrinking::No);
	}
	if (Priority != ETDBarkPriority::Critical && Recent.Num() >= Tuning.MaxPerWindow)
	{
		return false;
	}
	const float* Cd = Tuning.CooldownById.Find(BarkId);
	if (const double* Last = LastById.Find(BarkId); Last && RealNow - *Last < (Cd ? *Cd : Tuning.DefaultCooldownSeconds))
	{
		return false;
	}
	if (Priority == ETDBarkPriority::Normal && RealNow - LastAny < Tuning.GlobalMinGapSeconds)
	{
		return false;
	}
	LastById.Add(BarkId, RealNow);
	Recent.Add(RealNow);
	LastAny = RealNow;
	return true;
}

void FTDBarkLimiter::Reset()
{
	LastById.Reset();
	Recent.Reset();
	LastAny = -1000.0;
}

FName TDBarkIds::EngageCallFor(const FTransform& Camera, const FVector& Where)
{
	const FVector L = Camera.InverseTransformPositionNoScale(Where); // X forward, Y right, Z up
	const float Elevation = FMath::RadiansToDegrees(FMath::Atan2(L.Z, FVector2D(L.X, L.Y).Size()));
	const float Azimuth = FMath::RadiansToDegrees(FMath::Atan2(L.Y, L.X));
	const TCHAR* Vert = Elevation > 25.f ? TEXT("High") : (Elevation < -25.f ? TEXT("Low") : TEXT(""));
	const TCHAR* Horiz = FMath::Abs(Azimuth) <= 35.f ? TEXT("Ahead")
		: (FMath::Abs(Azimuth) >= 145.f ? TEXT("Behind") : (Azimuth > 0.f ? TEXT("Right") : TEXT("Left")));
	return FName(*FString::Printf(TEXT("Bark_SquadEngage%s%s"), Vert, Horiz));
}

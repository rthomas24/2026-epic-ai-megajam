// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "Engine/World.h"

UTDWorldSubsystem* UTDWorldSubsystem::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDWorldSubsystem>() : nullptr;
}

int32 UTDWorldSubsystem::AddObstacle(const FVector& Center, float Radius)
{
	return Obstacles.Add(FSphere(Center, FMath::Max(0.f, Radius)));
}

void UTDWorldSubsystem::ClearObstacles()
{
	Obstacles.Reset();
}

bool UTDWorldSubsystem::FindAvoidance(const FVector& Origin, const FVector& Direction, float ProbeDistance, float ShipRadius, FVector& OutAvoidDirection) const
{
	const FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero() || ProbeDistance <= 0.f)
	{
		return false;
	}

	const FSphere* Nearest = nullptr;
	float NearestAlong = TNumericLimits<float>::Max();
	for (const FSphere& S : Obstacles)
	{
		const FVector To = S.Center - Origin;
		const float Along = FVector::DotProduct(To, Dir);
		const float Reach = S.W + ShipRadius;
		if (Along < -Reach || Along > ProbeDistance + Reach)
		{
			continue;
		}
		const float LateralSq = (To - Dir * Along).SizeSquared();
		if (LateralSq <= FMath::Square(Reach) && Along < NearestAlong)
		{
			Nearest = &S;
			NearestAlong = Along;
		}
	}
	if (!Nearest)
	{
		return false;
	}

	// Slide around: steer toward the side of the obstacle we're already offset to (or "up" if dead centre).
	const FVector To = Nearest->Center - Origin;
	FVector Lateral = -(To - Dir * FVector::DotProduct(To, Dir));
	if (Lateral.SizeSquared() < 1.f)
	{
		Lateral = FVector::CrossProduct(Dir, FVector::RightVector);
		if (Lateral.SizeSquared() < 1e-4f) { Lateral = FVector::UpVector; }
	}
	Lateral.Normalize();
	// The closer the obstacle, the harder we turn away.
	const float Urgency = FMath::Clamp(1.f - NearestAlong / FMath::Max(1.f, ProbeDistance), 0.25f, 1.f);
	OutAvoidDirection = (Dir * (1.f - Urgency) + Lateral * (0.6f + Urgency)).GetSafeNormal();
	return true;
}

bool UTDWorldSubsystem::IsInsideObstacle(const FVector& Point, float Margin) const
{
	for (const FSphere& S : Obstacles)
	{
		if (FVector::DistSquared(Point, S.Center) <= FMath::Square(S.W + Margin))
		{
			return true;
		}
	}
	return false;
}

void UTDWorldSubsystem::SetPlayBounds(const FVector& Center, float Radius)
{
	BoundsCenter = Center;
	BoundsRadius = Radius;
	bHasBounds = Radius > 0.f;
}

bool UTDWorldSubsystem::GetPlayBounds(FVector& OutCenter, float& OutRadius) const
{
	OutCenter = BoundsCenter;
	OutRadius = BoundsRadius;
	return bHasBounds;
}

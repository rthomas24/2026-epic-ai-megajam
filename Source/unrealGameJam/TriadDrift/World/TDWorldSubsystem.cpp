// Triad Drift - World (Teammate 4; obstacle grid and removal by Teammate 6).
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "Engine/World.h"

UTDWorldSubsystem* UTDWorldSubsystem::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDWorldSubsystem>() : nullptr;
}

FIntVector UTDWorldSubsystem::CellOf(const FVector& P)
{
	return FIntVector(FMath::FloorToInt(P.X / GridCellSize), FMath::FloorToInt(P.Y / GridCellSize), FMath::FloorToInt(P.Z / GridCellSize));
}

int32 UTDWorldSubsystem::AddObstacle(const FVector& Center, float Radius)
{
	const float R = FMath::Max(0.f, Radius);
	const int32 Handle = Obstacles.Add(FSphere(Center, R));
	if (R <= GridCellSize * 0.5f)
	{
		Grid.FindOrAdd(CellOf(Center)).Add(Handle);
		MaxGridRadius = FMath::Max(MaxGridRadius, R);
	}
	else
	{
		Big.Add(Handle);
	}
	return Handle;
}

void UTDWorldSubsystem::RemoveObstacle(int32 Handle)
{
	if (!Obstacles.IsValidIndex(Handle) || Obstacles[Handle].W < 0.f)
	{
		return;
	}
	if (TArray<int32>* Cell = Grid.Find(CellOf(Obstacles[Handle].Center)))
	{
		Cell->RemoveSingleSwap(Handle);
	}
	Big.RemoveSingleSwap(Handle);
	Obstacles[Handle].W = -1.f;
	++Removed;
}

void UTDWorldSubsystem::ClearObstacles()
{
	Obstacles.Reset();
	Grid.Reset();
	Big.Reset();
	MaxGridRadius = 0.f;
	Removed = 0;
}

void UTDWorldSubsystem::Gather(const FVector& Min, const FVector& Max, TArray<int32, TInlineAllocator<256>>& Out) const
{
	Out.Append(Big);
	const FIntVector A = CellOf(Min - FVector(MaxGridRadius));
	const FIntVector B = CellOf(Max + FVector(MaxGridRadius));
	const int64 Cells = int64(B.X - A.X + 1) * int64(B.Y - A.Y + 1) * int64(B.Z - A.Z + 1);
	if (Cells > int64(Grid.Num()))
	{
		// A huge query box: walking the filled cells is cheaper than walking the box.
		for (const TPair<FIntVector, TArray<int32>>& KV : Grid)
		{
			const FIntVector& C = KV.Key;
			if (C.X >= A.X && C.X <= B.X && C.Y >= A.Y && C.Y <= B.Y && C.Z >= A.Z && C.Z <= B.Z) { Out.Append(KV.Value); }
		}
		return;
	}
	for (int32 X = A.X; X <= B.X; ++X)
	{
		for (int32 Y = A.Y; Y <= B.Y; ++Y)
		{
			for (int32 Z = A.Z; Z <= B.Z; ++Z)
			{
				if (const TArray<int32>* Cell = Grid.Find(FIntVector(X, Y, Z))) { Out.Append(*Cell); }
			}
		}
	}
}

bool UTDWorldSubsystem::FindAvoidance(const FVector& Origin, const FVector& Direction, float ProbeDistance, float ShipRadius, FVector& OutAvoidDirection) const
{
	const FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero() || ProbeDistance <= 0.f || Obstacles.Num() == 0)
	{
		return false;
	}

	const FVector End = Origin + Dir * ProbeDistance;
	TArray<int32, TInlineAllocator<256>> Candidates;
	Gather(Origin.ComponentMin(End) - FVector(ShipRadius), Origin.ComponentMax(End) + FVector(ShipRadius), Candidates);

	const FSphere* Nearest = nullptr;
	float NearestAlong = TNumericLimits<float>::Max();
	for (const int32 Handle : Candidates)
	{
		const FSphere& S = Obstacles[Handle];
		if (S.W < 0.f)
		{
			continue;
		}
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
	if (Obstacles.Num() == 0)
	{
		return false;
	}
	TArray<int32, TInlineAllocator<256>> Candidates;
	Gather(Point - FVector(Margin), Point + FVector(Margin), Candidates);
	for (const int32 Handle : Candidates)
	{
		const FSphere& S = Obstacles[Handle];
		if (S.W >= 0.f && FVector::DistSquared(Point, S.Center) <= FMath::Square(S.W + Margin))
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

// Triad Drift - World (Teammate 4; obstacle grid and removal by Teammate 6 for the space field). Static obstacles +
// play-space bounds, queryable without actors.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDWorldSubsystem.generated.h"

/**
 * Knows every static obstacle in the galaxy (asteroids, ruins, planets, the Dominion hull, the space field) as
 * spheres, and the soft play-space bounds. Enemy and wingman AI use FindAvoidance for obstacle avoidance; flight can
 * read GetPlayBounds. Pure data: no ticking, no actor iteration.
 *
 * Obstacles up to GridCellSize across are filed in a uniform grid by their centre, larger ones in a short list that is
 * always checked, so a query costs the same with 200 obstacles or 5,000 (the space field registers thousands).
 * Removed obstacles keep their handle (handles never move).
 */
UCLASS()
class UNREALGAMEJAM_API UTDWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDWorldSubsystem* Get(const UObject* WorldContext);

	/** Register an obstacle sphere. Returns its handle (index). */
	int32 AddObstacle(const FVector& Center, float Radius);
	/** Take an obstacle out (a broken rock). Its handle stays reserved; queries skip it. */
	void RemoveObstacle(int32 Handle);
	void ClearObstacles();
	/** Handles ever issued (removed ones included). */
	int32 GetObstacleCount() const { return Obstacles.Num(); }
	int32 GetActiveObstacleCount() const { return Obstacles.Num() - Removed; }
	/** Every handle's sphere; a removed obstacle has W < 0. */
	const TArray<FSphere>& GetObstacles() const { return Obstacles; }

	/**
	 * Something solid ahead? Sweeps a sphere of ShipRadius from Origin along Direction for
	 * ProbeDistance. If it would hit, returns true and OutAvoidDirection: a unit direction that
	 * slides around the nearest blocking obstacle.
	 */
	bool FindAvoidance(const FVector& Origin, const FVector& Direction, float ProbeDistance, float ShipRadius, FVector& OutAvoidDirection) const;

	/** True if Point is inside (or within Margin of) any obstacle. */
	bool IsInsideObstacle(const FVector& Point, float Margin = 0.f) const;

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|World")
	void SetPlayBounds(const FVector& Center, float Radius);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|World")
	bool GetPlayBounds(FVector& OutCenter, float& OutRadius) const;

	/** Grid cell edge (cm). Obstacles with a radius up to half of it go in the grid. */
	static constexpr float GridCellSize = 20000.f;

private:
	/** The handles worth checking for a box (grid cells it touches, widened by the largest gridded radius, plus the big list). */
	void Gather(const FVector& Min, const FVector& Max, TArray<int32, TInlineAllocator<256>>& Out) const;
	static FIntVector CellOf(const FVector& P);

	TArray<FSphere> Obstacles;
	TMap<FIntVector, TArray<int32>> Grid;
	TArray<int32> Big;
	float MaxGridRadius = 0.f;
	int32 Removed = 0;
	FVector BoundsCenter = FVector::ZeroVector;
	float BoundsRadius = 0.f;
	bool bHasBounds = false;
};

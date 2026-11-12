// Triad Drift - World (Teammate 4). Static obstacles + play-space bounds, queryable without actors.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDWorldSubsystem.generated.h"

/**
 * Knows every static obstacle in the galaxy (asteroids, ruins, planets, the Dominion hull) as
 * spheres, and the soft play-space bounds. Enemy and wingman AI use FindAvoidance for obstacle
 * avoidance; flight can read GetPlayBounds. Pure data: no ticking, no actor iteration.
 */
UCLASS()
class UNREALGAMEJAM_API UTDWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDWorldSubsystem* Get(const UObject* WorldContext);

	/** Register an obstacle sphere. Returns its handle (index). */
	int32 AddObstacle(const FVector& Center, float Radius);
	void ClearObstacles();
	int32 GetObstacleCount() const { return Obstacles.Num(); }
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

private:
	TArray<FSphere> Obstacles;
	FVector BoundsCenter = FVector::ZeroVector;
	float BoundsRadius = 0.f;
	bool bHasBounds = false;
};

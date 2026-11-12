// Triad Drift - cover points for crew (ENG-23 "troopers use cover"). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDCrewCover.generated.h"

/** One spot behind a piece of cover. */
USTRUCT()
struct FTDCoverPoint
{
	GENERATED_BODY()

	/** Where to stand (ground level + capsule half height is the caller's business). */
	FVector Location = FVector::ZeroVector;
	/** Outward from the cover surface (the side the point is on). */
	FVector Normal = FVector::ForwardVector;
	/** Along the cover face: peek positions are Location +/- Tangent * PeekReach. */
	FVector Tangent = FVector::RightVector;
	float PeekReach = 120.f;

	/** The cover itself. A destroyed cover invalidates its points. */
	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<AActor> ClaimedBy;

	bool IsValid() const { return Owner.IsValid(); }
};

/**
 * Cover registry (no actor iteration at runtime). Encounters and the planet builders register the blocks
 * they spawn; each block gives four points, one per side. Crew claim a point, hide behind it, peek out
 * to shoot, and release it when the point stops protecting them.
 */
UCLASS()
class UNREALGAMEJAM_API UTDCrewCoverSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDCrewCoverSubsystem* Get(const UObject* WorldContext);

	/** Adds four points around Actor's bounds (one per face), StandOff cm out from the surface. */
	void RegisterCoverActor(AActor* Actor, float StandOff = 70.f);
	void UnregisterCoverActor(AActor* Actor);

	/**
	 * Best free point for Seeker hiding from Threat: Threat's line of sight to the point is blocked by the
	 * cover, Seeker can get there (within MaxTravel), and it keeps a usable distance to Threat. INDEX_NONE if none.
	 */
	int32 FindCover(const AActor* Seeker, const AActor* Threat, float MaxTravel, float MinThreatDistance, float MaxThreatDistance) const;

	bool Claim(int32 Index, AActor* By);
	void Release(AActor* By);

	/** Is the point still hiding a body at that spot from Threat? (false = flanked or cover gone) */
	bool IsProtected(int32 Index, const AActor* Threat, const AActor* Ignore) const;

	/** A position next to the point with line of sight to Threat (peek); returns false if neither side has one. */
	bool FindPeekLocation(int32 Index, const AActor* Threat, const AActor* Ignore, FVector& OutLocation) const;

	const FTDCoverPoint* GetPoint(int32 Index) const { return Points.IsValidIndex(Index) ? &Points[Index] : nullptr; }
	int32 Num() const { return Points.Num(); }

private:
	bool Blocked(const FVector& From, const FVector& To, const AActor* IgnoreA, const AActor* IgnoreB) const;

	UPROPERTY()
	TArray<FTDCoverPoint> Points;
};

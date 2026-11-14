// Triad Drift - World (Teammate 4). ENG-31 escape flight: a burning hull chunk falling across the way out.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDEscapeDebris.generated.h"

class UStaticMeshComponent;

/**
 * A chunk of the breaking Dominion drifting across the escape corridor. A squad ship that touches it takes
 * Damage (Environmental) and the chunk bursts. No collision (ships fly through the burst, not bounce): a distance
 * check against the squad's ships each frame. Gone after LifeSeconds.
 */
UCLASS()
class UNREALGAMEJAM_API ATDEscapeDebris : public AActor
{
	GENERATED_BODY()

public:
	/** The largest a debris art piece is drawn (cm): big plates still leave the corridor flyable. */
	float MaxArtSizeCm = 2800.f;
	bool IsArtPiece() const { return bArtPiece; }
	ATDEscapeDebris();

	/** Size (cm across), drift velocity (cm/s) and spin (deg/s). */
	void Launch(float SizeCm, const FVector& InVelocity, const FRotator& InSpin);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape", meta = (ClampMin = "0"))
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape", meta = (ClampMin = "0.1"))
	float LifeSeconds = 12.f;

	bool HasHit() const { return bHit; }
	FVector GetDriftVelocity() const { return Velocity; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	void Burst();

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Escape")
	TObjectPtr<UStaticMeshComponent> Mesh;

	FVector Velocity = FVector::ZeroVector;
	FRotator Spin = FRotator::ZeroRotator;
	float Age = 0.f;
	float Radius = 1000.f;
	bool bHit = false;
	bool bArtPiece = false;
};

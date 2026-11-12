// Triad Drift - Powers (Teammate 2). Shared portal transit: Rift's portals (ENG-09) and T4's portal gates (ENG-16).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TDPortalTransit.generated.h"

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDPortalTransitParams
{
	GENERATED_BODY()

	/** Ships coming out of the exit can't be damaged for this long (ENG-09: 0.3 s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal", meta = (Units = "s"))
	float InvulnerableSeconds = 0.3f;

	/** Enemy ships that pass through lose tracking for this long (ENG-09: 2 s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal", meta = (Units = "s"))
	float EnemyTrackingLostSeconds = 2.f;

	/** Ships keep their speed through the portal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal")
	bool bKeepSpeed = true;

	/** Enemy projectiles that enter are deflected to the Player team and re-home on whoever fired them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal")
	bool bReturnEnemyProjectiles = true;

	/** Speed multiplier for returned enemy projectiles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal")
	float ReturnSpeedMultiplier = 1.5f;

	/** Travellers come out this far in front of the exit plane (clear of the exit trigger). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal", meta = (Units = "cm"))
	float ExitForwardOffset = 400.f;

	/** The same actor can't transit again for this long (stops bouncing between a pair). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal", meta = (Units = "s"))
	float ReentryCooldownSeconds = 0.5f;
};

/**
 * Moves a traveller from an Entry transform to an Exit transform. Both transforms' forward (X) axis
 * points in the direction of travel: into the entry, out of the exit. Lateral offset from the
 * entry centre is preserved; scale is ignored.
 *
 *  - Spacecraft (ATDSpacecraft): TeleportSpacecraft(exit, exit facing, keep speed) (which fires the
 *    bus OnSpacecraftTeleported so wingmen follow), invulnerability, PortalsUsed stat for the player.
 *    Enemy ships additionally LoseTracking.
 *  - Projectiles (ATDProjectile): re-aimed through the exit. Enemy ones are Deflect()ed to the Player
 *    team (re-homing on their shooter); missiles count as MissilesReturned and fire a MissileReturn
 *    highlight.
 * Returns false (and does nothing) for other actors or during the re-entry cooldown.
 */
UCLASS()
class UNREALGAMEJAM_API UTDPortalTransit : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Portal")
	static bool TransitActor(AActor* Actor, const FTransform& Entry, const FTransform& Exit, AActor* PortalInstigator,
		const FTDPortalTransitParams& Params);

	/** Kind of traveller (spacecraft or projectile) and not in its re-entry cooldown. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Portal")
	static bool CanTransit(const AActor* Actor, float CooldownSeconds = 0.5f);

	/** Pure mapping helpers (tests, previews). */
	static FVector MapLocation(const FVector& Location, const FTransform& Entry, const FTransform& Exit, float ExitForwardOffset);
	static FVector MapDirection(const FVector& Direction, const FTransform& Entry, const FTransform& Exit);
};

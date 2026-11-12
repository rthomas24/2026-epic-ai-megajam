// Triad Drift - Enemies & Waves (Teammate 4).
// What the wave director needs from a spawned enemy. ATDEnemyShip implements it; tests use stand-ins.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TDWaveEnemy.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTDWaveEnemy : public UInterface
{
	GENERATED_BODY()
};

class UNREALGAMEJAM_API ITDWaveEnemy
{
	GENERATED_BODY()

public:
	/** Configure the spawned actor as enemy type EnemyId (UTDEnemyLibrary). */
	virtual void InitWaveEnemy(FName EnemyId) PURE_VIRTUAL(ITDWaveEnemy::InitWaveEnemy, );

	/** Join a pack: follow Leader at LocalOffset. Leader == null means fly solo / lead. */
	virtual void JoinFormation(AActor* Leader, const FVector& LocalOffset) {}

	/** Play the warp-in arrival on the spawned actor (presentation only). */
	virtual void PlayWarpIn() {}
};

// Small actors for automation tests that need a stand-in for a gameplay contract.
// They live in a header because Unreal's reflection needs one; nothing in the game spawns them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDTestActors.generated.h"

/** A non-pawn HUD data source (what the squad manager is to the HUD and to audio) with settable values. */
UCLASS(NotBlueprintable, NotPlaceable, Transient)
class UNREALGAMEJAM_API ATDTestHUDSource : public AActor, public ITDHUDData
{
	GENERATED_BODY()

public:
	TArray<FTDShipHUD> Cards;
	FTDWaveInfo Wave;
	float Speed01 = 0.f;
	float HyperdriveCharge01 = 0.f;
	bool bOutOfBounds = false;
	ETDMissileWarning MissileWarning = ETDMissileWarning::None;

	virtual TArray<FTDShipHUD> GetSquadHUD() const override { return Cards; }
	virtual FTDWaveInfo GetWaveInfo() const override { return Wave; }
	virtual float GetSpeed01() const override { return Speed01; }
	virtual float GetHyperdriveCharge01() const override { return HyperdriveCharge01; }
	virtual bool IsOutOfBounds() const override { return bOutOfBounds; }
	virtual ETDMissileWarning GetMissileWarning() const override { return MissileWarning; }
};

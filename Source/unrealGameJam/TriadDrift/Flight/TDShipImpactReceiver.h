// Triad Drift - Flight (Teammate 1). A ship hitting something solid, told to the thing it hit (agreed with T6, Nov 13):
// T1 owns the ship's response and the ship's damage (UTDShipMovementComponent::HandleShipImpact); whatever was hit owns
// its own damage by implementing this interface (T6's ATDSpaceField: asteroids, wrecks, big debris).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TDShipImpactReceiver.generated.h"

class ATDShip;
class UPrimitiveComponent;

UINTERFACE(MinimalAPI)
class UTDShipImpactReceiver : public UInterface
{
	GENERATED_BODY()
};

class UNREALGAMEJAM_API ITDShipImpactReceiver
{
	GENERATED_BODY()

public:
	/**
	 * A ship hit this actor (every blocking hit faster than the scrape threshold, BEFORE the ship's own response).
	 * @param HitComponent  the component hit (the field's ISM, say)
	 * @param Item          the hit's item: the ISM instance index, or INDEX_NONE
	 * @param ImpactPoint   world point of contact
	 * @param NormalSpeed   speed into the surface (cm/s)
	 * @param HeadOn01      0 = grazing, 1 = straight in
	 * @param Ship          the ship that hit it (its UTDRamPower may be active: Lancer's ram smashes smalls and mediums)
	 * @return true = the piece broke and its collision is gone this frame: the ship takes no bounce and no damage and
	 *         carries on through (the rest of the move is swept again); false = it stands: the ship's normal response.
	 */
	virtual bool ReceiveShipImpact(UPrimitiveComponent* HitComponent, int32 Item, const FVector& ImpactPoint, float NormalSpeed, float HeadOn01, ATDShip* Ship) = 0;
};

// Test stand-in for the space HUD's data source (ENG-36 tests, Teammate 2). Nothing in the game spawns it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDFlightHUDTestSource.generated.h"

/** ITDHUDData with settable squad cards, wave and objective text. */
UCLASS(NotBlueprintable, NotPlaceable, Transient)
class UNREALGAMEJAM_API ATDFlightHUDTestSource : public AActor, public ITDHUDData
{
	GENERATED_BODY()

public:
	TArray<FTDShipHUD> Cards;
	FTDWaveInfo Wave;
	FText Objective;

	virtual TArray<FTDShipHUD> GetSquadHUD() const override { return Cards; }
	virtual FTDWaveInfo GetWaveInfo() const override { return Wave; }
	virtual FText GetObjectiveText() const override { return Objective; }

	/** Three healthy ships, Flown under the player's control, listed in a shuffled order (the strip must not care). */
	void SetSquad(ETDShipId Flown)
	{
		Cards.Reset();
		for (const ETDShipId Ship : { ETDShipId::Bastion, ETDShipId::Rift, ETDShipId::Lancer })
		{
			FTDShipHUD& Card = Cards.AddDefaulted_GetRef();
			Card.Ship = Ship;
			Card.bPlayerControlled = Ship == Flown;
		}
	}
	FTDShipHUD& Card(ETDShipId Ship)
	{
		FTDShipHUD* Found = Cards.FindByPredicate([Ship](const FTDShipHUD& C) { return C.Ship == Ship; });
		check(Found);
		return *Found;
	}
};

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

	// P2
	TArray<FTDTargetMarker> Markers;
	TArray<FTDDamageIndicator> Hits;
	ETDMissileWarning Missile = ETDMissileWarning::None;
	FText Prompt;
	FText Blocked;
	FText Boundary;
	float SwitchStrike = 0.f;
	float Altitude = -1.f;
	float Hyperdrive = 0.f;

	virtual TArray<FTDShipHUD> GetSquadHUD() const override { return Cards; }
	virtual FTDWaveInfo GetWaveInfo() const override { return Wave; }
	virtual FText GetObjectiveText() const override { return Objective; }
	virtual TArray<FTDTargetMarker> GetTargetMarkers() const override { return Markers; }
	virtual TArray<FTDDamageIndicator> GetDamageIndicators() const override { return Hits; }
	virtual ETDMissileWarning GetMissileWarning() const override { return Missile; }
	virtual FText GetInteractPrompt() const override { return Prompt; }
	virtual FText GetHyperdriveBlockText() const override { return Blocked; }
	virtual FText GetBoundaryWarningText() const override { return Boundary; }
	virtual float GetSwitchStrike01() const override { return SwitchStrike; }
	virtual float GetAltitude() const override { return Altitude; }
	virtual float GetHyperdriveCharge01() const override { return Hyperdrive; }

	FTDTargetMarker& AddMarker(bool bOnScreen, FVector2D Screen01, FVector2D Dir, float DistanceCm)
	{
		FTDTargetMarker& M = Markers.AddDefaulted_GetRef();
		M.bOnScreen = bOnScreen;
		M.ScreenPosition01 = Screen01;
		M.ScreenDirection = Dir;
		M.Distance = DistanceCm;
		return M;
	}

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

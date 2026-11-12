// Triad Drift - records event bus payloads for Flow automation tests. Owner: Teammate 5.
// A UCLASS has to live in a header for UHT; it's tiny and inert outside tests.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDFlowTestListener.generated.h"

class UTDEventBus;

struct FTDRecordedCut
{
	FName SceneId;
	int32 NextTake = 0;
	float Delay = 0.f;
};

UCLASS(Transient, NotBlueprintable, NotBlueprintType)
class UTDFlowTestListener : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UTDEventBus* Bus);

	TArray<ETDGamePhase> Phases;
	TArray<TPair<ETDAbilityId, ETDShipId>> Unlocks;
	TArray<TPair<FName, int32>> Takes;
	TArray<FName> Tips;
	TArray<TPair<FName, FName>> Barks;
	TArray<FTDRecordedCut> Cuts;
	TArray<TPair<FName, int32>> ObjectiveSteps;
	TArray<FText> ObjectiveTexts;

	UFUNCTION() void OnPhase(ETDGamePhase Phase) { Phases.Add(Phase); }
	UFUNCTION() void OnUnlock(ETDAbilityId Ability, ETDShipId Ship) { Unlocks.Emplace(Ability, Ship); }
	UFUNCTION() void OnTake(FName SceneId, int32 TakeNumber) { Takes.Emplace(SceneId, TakeNumber); }
	UFUNCTION() void OnTip(FName TipId) { Tips.Add(TipId); }
	UFUNCTION() void OnBark(FName BarkId, FName Speaker) { Barks.Emplace(BarkId, Speaker); }
	UFUNCTION() void OnObjective(FName ObjectiveId, int32 Step, FText Text) { ObjectiveSteps.Emplace(ObjectiveId, Step); ObjectiveTexts.Add(Text); }
	UFUNCTION() void OnCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds) { Cuts.Add({ SceneId, NextTakeNumber, DelaySeconds }); }
};

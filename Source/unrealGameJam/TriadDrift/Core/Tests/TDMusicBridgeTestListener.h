// Test-only listener for TriadDrift.Core.MusicBridge (UHT only parses headers, so the UCLASS lives here).
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TDMusicBridgeTestListener.generated.h"

UCLASS(Transient, NotBlueprintable)
class UNREALGAMEJAM_API UTDMusicBridgeTestListener : public UObject
{
	GENERATED_BODY()

public:
	struct FEvent { FName Id; TWeakObjectPtr<UObject> World; FName Detail; bool bFlag = false; };
	TArray<FEvent> Events;
	UFUNCTION() void Handle(FName EventId, UObject* WorldContext, FName DetailId, bool bFlag) { Events.Add({ EventId, WorldContext, DetailId, bFlag }); }
	const FEvent* Last(FName Id) const { for (int32 i = Events.Num() - 1; i >= 0; --i) { if (Events[i].Id == Id) { return &Events[i]; } } return nullptr; }
};

// Triad Drift - tests only: a stand-in for Nick's UTDAbilityRewardWidget (TriadDriftHUD plugin, not linked by this module)
// with the same function names, parameter struct field names and delegate, so the host's by-name mount is tested here.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDRewardStandInWidget.generated.h"

USTRUCT()
struct FTDRewardStandInDisplay
{
	GENERATED_BODY()
	UPROPERTY() FName AbilityId;
	UPROPERTY() FText AbilityName;
	UPROPERTY() FText ShipName;
	UPROPERTY() FText AvailabilityText;
	UPROPERTY() FText ActivationKey;
	UPROPERTY() FLinearColor Accent = FLinearColor::White;
	UPROPERTY() bool bCanUseNow = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDRewardStandInDismissed, FName, AbilityId);

UCLASS(NotBlueprintable, HideDropdown)
class UNREALGAMEJAM_API UTDRewardStandInWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** As the contract: false for a different identity while an undismissed award is up. */
	UFUNCTION() bool PresentReward(const FTDRewardStandInDisplay& Display)
	{
		if (Display.AbilityId.IsNone() || (bActive && Display.AbilityId != Data.AbilityId))
		{
			return false;
		}
		Data = Display;
		bActive = true;
		++Presents;
		return true;
	}
	UFUNCTION() bool FocusDismissButton() { ++Focuses; return bActive; }
	UFUNCTION() void RequestDismiss()
	{
		if (!bActive) { return; }
		bActive = false;
		OnRewardDismissed.Broadcast(Data.AbilityId);
	}
	UFUNCTION() bool IsRewardActive() const { return bActive; }
	UPROPERTY() FTDRewardStandInDismissed OnRewardDismissed;

	FTDRewardStandInDisplay Data;
	bool bActive = false;
	int32 Presents = 0;
	int32 Focuses = 0;
};

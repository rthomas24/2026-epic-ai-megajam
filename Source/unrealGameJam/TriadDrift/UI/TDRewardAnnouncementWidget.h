// Triad Drift - the native ability-unlock announcement (Teammate 2; the Lead, Nov 13: T7's Verdance playtest found the
// rewards granted silently). Mounted by UTDUIHostSubsystem when no reward screen class is set (DA_UI_Style's
// AbilityRewardWidgetClass, or Nick's /Script/TriadDriftHUD.TDAbilityRewardWidget): his screen always wins.
//
// It answers the same contract by name as Nick's widget (PresentReward(Display) -> bool, FocusDismissButton,
// RequestDismiss, ResetPresentation, IsRewardActive, OnRewardDismissed(FName)), so the host treats both alike. A plate at
// the top centre names the ability, the ship that owns it and the key (only while that ship is flown); it shows 4 s and
// dismisses itself, takes no focus and pauses nothing. Painted in the flight HUD's tokens in a 1920x1080 design frame.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDRewardAnnouncementWidget.generated.h"

USTRUCT(BlueprintType)
struct FTDRewardAnnouncementDisplay
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward") FName AbilityId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward") FText AbilityName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward") FText ShipName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward") FText AvailabilityText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward") FText ActivationKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward") FLinearColor Accent = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward") bool bCanUseNow = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDRewardAnnouncementDismissed, FName, AbilityId);

UCLASS()
class UNREALGAMEJAM_API UTDRewardAnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr float ShowSeconds = 4.f;
	static constexpr float FadeSeconds = .25f;

	/** False for no identity, or a different award while one is still up. The same award refreshes (e.g. the key). */
	UFUNCTION(BlueprintCallable, Category = "Reward") bool PresentReward(const FTDRewardAnnouncementDisplay& Display);
	/** Takes no focus (pausing nothing): false. */
	UFUNCTION(BlueprintCallable, Category = "Reward") bool FocusDismissButton() { return false; }
	/** Hide now and broadcast OnRewardDismissed once. */
	UFUNCTION(BlueprintCallable, Category = "Reward") void RequestDismiss();
	/** Hide without acknowledging (the owner re-presents later). */
	UFUNCTION(BlueprintCallable, Category = "Reward") void ResetPresentation();
	UFUNCTION(BlueprintPure, Category = "Reward") bool IsRewardActive() const { return bActive; }
	UPROPERTY(BlueprintAssignable, Category = "Reward") FTDRewardAnnouncementDismissed OnRewardDismissed;

	const FTDRewardAnnouncementDisplay& GetDisplay() const { return Data; }
	float GetAge() const { return Age; }
	/** Tests: advance without a tick. */
	void Advance(float Dt);
	/** Named boxes painted last frame (tests). */
	mutable TArray<TPair<FName, FBox2D>> LastBoxes;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY(Transient) FTDRewardAnnouncementDisplay Data;
	bool bActive = false;
	float Age = 0.f;
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "TDAbilityRewardWidget.generated.h"

/** Adapter supplies localized display data from T2's pending award; no game-module dependency. */
USTRUCT(BlueprintType)
struct TRIADDRIFTHUD_API FTDAbilityRewardDisplay
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Reward") FName AbilityId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Reward") FText AbilityName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Reward") FText ShipName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Reward") FText AvailabilityText;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Reward") FText ActivationKey;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Reward") FLinearColor Accent=FLinearColor::White;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Reward") bool bCanUseNow=false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDRewardDismissed,FName,AbilityId);

/** Presentation only. Adapter mounts one instance and acknowledges the matching pending award on dismissal. */
UCLASS(BlueprintType,Blueprintable)
class TRIADDRIFTHUD_API UTDAbilityRewardWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UTDAbilityRewardWidget(const FObjectInitializer& ObjectInitializer);
    /** False for invalid data, a dismissed identity, or replacing an undismissed different award. */
    UFUNCTION(BlueprintCallable,Category="Reward") bool PresentReward(const FTDAbilityRewardDisplay& Display);
    UFUNCTION(BlueprintCallable,Category="Reward") void RequestDismiss();
    /** Teardown/suppression without acknowledgment. Owner re-presents its pending award later. */
    UFUNCTION(BlueprintCallable,Category="Reward") void ResetPresentation();
    /** Explicit owner opt-in after mounting; this widget never changes the controller's input mode. */
    UFUNCTION(BlueprintCallable,Category="Reward") bool FocusDismissButton();
    UFUNCTION(BlueprintPure,Category="Reward") bool IsRewardActive() const { return bActive; }
    UFUNCTION(BlueprintPure,Category="Reward") FName GetPresentedAbilityId() const { return Data.AbilityId; }
    UFUNCTION(BlueprintPure,Category="Reward") FText GetActivationKey() const;
    UFUNCTION(BlueprintPure,Category="Reward") FText GetAvailabilityText() const;
    UFUNCTION(BlueprintPure,Category="Reward") int32 GetDismissNotificationCount() const { return DismissNotifications; }
    UPROPERTY(BlueprintAssignable,Category="Reward") FTDRewardDismissed OnRewardDismissed;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnKeyDown(const FGeometry&,const FKeyEvent&) override;
    virtual void NativeDestruct() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;
private:
    /** A hard CDO reference keeps the shared frame in cooked builds with this widget. */
    UPROPERTY() TObjectPtr<class UTexture2D> TerminalFrameTexture;
    FSlateBrush TerminalFrameBrush;
    FButtonStyle TerminalButtonStyle;
    void RestoreFocusIfOwned();
    UPROPERTY(Transient) FTDAbilityRewardDisplay Data;
    bool bActive=false;
    bool bDismissed=false;
    int32 DismissNotifications=0;
    TSharedPtr<class SButton> DismissButton;
    TWeakPtr<SWidget> PreviousFocus;
};

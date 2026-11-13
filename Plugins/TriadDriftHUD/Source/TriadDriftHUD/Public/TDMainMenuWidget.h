#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "TDMainMenuWidget.generated.h"

class SButton;
class STextBlock;

/** Presentation for the existing menu host. No automatic installation or gameplay dependency.
 * Assign only on the main-menu path; the existing victory/statistics screen remains host-owned.
 */
UCLASS(BlueprintType, Blueprintable)
class TRIADDRIFTHUD_API UTDMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    /** Stops decorative orbital motion. Does not change gameplay cameras or post processing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu|Accessibility")
    bool bReducedMenuMotion = false;

    UFUNCTION(BlueprintCallable, Category="Menu") void SetReducedMenuMotion(bool bReduced);
    UFUNCTION(BlueprintPure, Category="Menu") bool IsTransitionPending() const { return bTransitionPending; }

    /** Strict zero-input reflected bool/void contract, shared with focused compatibility tests. */
    static bool InvokeHost(UObject* Host, FName Action, bool& Result);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry&,float DeltaSeconds) override;
    virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    TSharedRef<SWidget> BuildActions();
    TSharedRef<SWidget> MakeAction(const FText& Label, FName Action, bool bEnabled = true);
    FReply Activate(FName Action);
    void ShowSettings(bool bShow);
    bool QueryHost(FName Action) const;
    bool HasHostAction(FName Action) const;
    void Status(const FText& Message);

    FButtonStyle ActionStyle;
    TSharedPtr<SButton> FirstButton;
    TSharedPtr<STextBlock> StatusLabel;
    TSharedPtr<class SBox> ActionsBox;
    TSharedPtr<SWidget> Backdrop;
    bool bSettingsOpen = false;
    bool bTransitionPending = false;
};

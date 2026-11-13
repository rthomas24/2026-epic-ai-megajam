#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDLoadingWidget.generated.h"

/** Optional presentation for the existing transition host's LoadingWidgetClass slot.
 * No travel, progress polling, input capture or automatic installation. */
UCLASS(BlueprintType, Blueprintable)
class TRIADDRIFTHUD_API UTDLoadingWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loading")
    FText SceneTitle = NSLOCTEXT("TDLoading", "NextScene", "NEXT SCENE");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loading")
    FText StatusText = NSLOCTEXT("TDLoading", "Preparing", "Preparing the next scene...");

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};

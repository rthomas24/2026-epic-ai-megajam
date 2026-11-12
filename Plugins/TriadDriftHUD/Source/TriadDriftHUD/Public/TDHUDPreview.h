#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "TDHUDPreview.generated.h"
class UTDSpaceHUDWidget;

/** Explicit opt-in sandbox; never used by the game's space controller. */
UCLASS()
class TRIADDRIFTHUD_API ATDHUDPreviewController : public APlayerController
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;
private:
    UPROPERTY(Transient) TObjectPtr<UTDSpaceHUDWidget> Widget;
};

UCLASS()
class TRIADDRIFTHUD_API ATDHUDPreviewGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ATDHUDPreviewGameMode();
};

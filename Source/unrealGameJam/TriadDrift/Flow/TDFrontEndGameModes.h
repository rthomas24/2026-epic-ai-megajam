// Triad Drift - game modes for the non-gameplay maps (L_MainMenu, L_Transition). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDFrontEndGameModes.generated.h"

class UTDUIStyle;

/** Front-end widget lookup: an explicit override wins, else Nick's DA_UI_Style, else nothing. */
struct UNREALGAMEJAM_API FTDFrontEndWidgets
{
	enum class EKind : uint8 { MainMenu, Loading };

	/** /Game/TriadDrift/Presentation/DA_UI_Style if it exists, else null. */
	static UTDUIStyle* LoadDefaultStyle();

	static UClass* Resolve(UClass* Override, const UTDUIStyle* Style, EKind Kind);
};

/**
 * L_MainMenu: no pawn, mouse cursor on, run timer stopped, never checkpoints. Nick's menu widget lives
 * in the map (or a BP child sets MenuWidgetClass). New Game: UTDGameInstance::StartNewRun() then
 * TravelTo(Galaxy); Continue: ContinueRun() (enabled when HasContinue()).
 */
UCLASS()
class UNREALGAMEJAM_API ATDMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDMenuGameMode();

	/** Optional menu widget override. When unset, DA_UI_Style's MainMenuWidgetClass is used (if any). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TSubclassOf<class UUserWidget> MenuWidgetClass;

	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
};

/** L_Transition: the loading screen between destinations. No pawn; the game instance drives the travel. */
UCLASS()
class UNREALGAMEJAM_API ATDTransitionGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDTransitionGameMode();

	/** Optional loading-screen override. When unset, DA_UI_Style's LoadingWidgetClass is used (if any). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	TSubclassOf<class UUserWidget> LoadingWidgetClass;

	virtual void StartPlay() override;
};

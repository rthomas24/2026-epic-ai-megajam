// Triad Drift - game modes for the non-gameplay maps (L_MainMenu, L_Transition). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDFrontEndGameModes.generated.h"

class UTDUIStyle;
class UAudioComponent;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// The menu's actions (Nick's widget calls these too).

	/** A fresh run: progress reset, then travel to the galaxy (wave 1 starts there). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Menu")
	bool NewGame();

	/** Resume the saved run (false when there is none, or it is a won run). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Menu")
	bool ContinueGame();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Menu")
	void QuitGame();

	/** From the victory screen back to the main menu. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Menu")
	void ReturnToMenu();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Menu")
	bool CanContinue() const;

	/** The run was just won: the map shows the victory screen first. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Menu")
	bool IsShowingVictory() const { return bShowingVictory; }

	/** True while the code-built placeholder menu is up (no widget assigned in DA_UI_Style). */
	bool IsPlaceholderShown() const { return Placeholder.IsValid(); }
	/** The code-built menu is used (no widget class resolved), whether or not a viewport exists to show it. */
	bool UsesPlaceholderMenu() const { return bPlaceholderWanted; }

	/** The menu music playing (DA_UI_Style.MenuMusic, 2D, UI sound); null when the style has none. */
	class UAudioComponent* GetMusic() const { return Music; }
	bool WasMusicStarted() const { return bMusicStarted; }
	bool IsMusicFadingOut() const { return bMusicFading; }

	/** Tests: use this style instead of DA_UI_Style. */
	UPROPERTY(Transient)
	TObjectPtr<UTDUIStyle> StyleOverride;

private:
	void StartMusic();
	void FadeOutMusic();
	UPROPERTY(Transient) TObjectPtr<class UAudioComponent> Music;
	void ShowPlaceholder();
	void HidePlaceholder();
	TSharedPtr<class SWidget> Placeholder;
	bool bShowingVictory = false;
	bool bMusicStarted = false;
	bool bPlaceholderWanted = false;
	bool bMusicFading = false;
	bool bQuitRequested = false;
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

// Triad Drift - game modes for the non-gameplay maps (L_MainMenu, L_Transition). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
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

/** The front-end maps draw no in-game HUD (the dev HUD only replaces the stock AHUD, so this keeps it off the menu). */
UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDFrontEndHUD : public AHUD
{
	GENERATED_BODY()
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
	virtual void Tick(float DeltaSeconds) override;

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
	/** The widget class the menu resolved last (victory or main menu); null when the code-built screen is up. */
	UClass* GetShownWidgetClass() const { return ShownWidgetClass.Get(); }

	/** The menu music playing (DA_UI_Style.MenuMusic, 2D, UI sound); null when the style has none. */
	class UAudioComponent* GetMusic() const { return Music; }
	bool WasMusicStarted() const { return bMusicStarted; }
	bool IsMusicFadingOut() const { return bMusicFading; }
	/** How many times the menu music has restarted on its own (the track is not a looping asset). */
	int32 GetMusicLoops() const { return MusicLoops; }

	/** Tests: use this style instead of DA_UI_Style. */
	UPROPERTY(Transient)
	TObjectPtr<UTDUIStyle> StyleOverride;

private:
	void StartMusic();
	void FadeOutMusic();
	/** Show the screen for the current state: the victory widget / screen, or the main-menu widget / placeholder. */
	void ShowMenuUI();
	UPROPERTY(Transient) TObjectPtr<class UUserWidget> ShownWidget;
	UPROPERTY(Transient) TObjectPtr<UClass> ShownWidgetClass;
	/** The menu track ended: play it again from the top, unless it is fading out for a travel. */
	UFUNCTION() void HandleMusicFinished();
	int32 MusicLoops = 0;
	UPROPERTY(Transient) TObjectPtr<class UAudioComponent> Music;
	void ShowPlaceholder();
	void HidePlaceholder();
	/** Keyboard / pad focus is kept inside the placeholder (the viewport takes it back after the first frames). */
	void KeepMenuFocus();
	TSharedPtr<class SWidget> Placeholder;
	TWeakPtr<class SWidget> FocusButton;
	/** The menu button that last had focus (SETTINGS after opening the page): focus comes back to it, not to the first. */
	TWeakPtr<class SWidget> LastMenuFocus;
public:
	/** Where focus returns when it left the menu (the Settings page closed): the last focused menu button if it can
	 *  still take focus, else the first one. Public for tests. */
	static TSharedPtr<class SWidget> ChooseRefocus(const TSharedPtr<class SWidget>& LastFocused, const TSharedPtr<class SWidget>& First);
private:
	bool bShowingVictory = false;
	bool bReturnPending = false; // the stinger plays before the menu comes back
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

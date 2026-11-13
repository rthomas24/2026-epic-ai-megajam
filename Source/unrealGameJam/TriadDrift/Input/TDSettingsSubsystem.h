// Triad Drift - player options (controls, hints, audio, video): one object the menus call. Owner: Teammate 1 (Input).
//
// Every setter applies at once and saves to its own slot ("TD_Settings", separate from the run save and the hint
// profile). The saved options are applied at start-up (video and audio when the game instance starts; controls on
// every ATDSpacePlayerController as it begins play). Menus (Nick's, T5's placeholder OPTIONS page) call the
// BlueprintCallable functions below: Get Game Instance -> Get Subsystem (TDSettingsSubsystem).
//
// Audio: master volume is the main audio device's transient primary volume (works with no assets). Music and effects
// scale the sound classes SC_TD_Music and SC_TD_SFX (/Game/TriadDrift/Audio/Mix/, Nick's names) through a runtime sound
// mix; until those classes exist, the values are kept and applied as soon as they do (start-up / next change).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TDSettingsSubsystem.generated.h"

class ATDSpacePlayerController;
class USoundMix;
class USoundClass;

/** On-foot view (ENG-32). Space flight keeps its own chase / cockpit cameras. */
UENUM(BlueprintType)
enum class ETDOnFootCamera : uint8
{
	FirstPerson,
	ThirdPerson
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnFootCameraModeChanged, ETDOnFootCamera, NewMode);

UENUM(BlueprintType)
enum class ETDWindowMode : uint8
{
	Fullscreen,
	WindowedFullscreen,
	Windowed
};

/** Every player option. Sensitivities are multipliers of the shipped tuning (1 = as designed). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDPlayerSettings
{
	GENERATED_BODY()

	/** ENG-32: first or third person on foot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Camera")
	ETDOnFootCamera OnFootCameraMode = ETDOnFootCamera::FirstPerson;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Controls", meta = (ClampMin = "0.25", ClampMax = "3"))
	float MouseSensitivity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Controls")
	bool bInvertMouseY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Controls", meta = (ClampMin = "0.25", ClampMax = "3"))
	float GamepadSensitivity = 1.f;

	/** Right-stick dead zone (0..0.5 of full deflection). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Controls", meta = (ClampMin = "0", ClampMax = "0.5"))
	float GamepadDeadzone = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Controls")
	bool bInvertGamepadY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Gameplay")
	bool bHintsEnabled = true;

	/** The player's chosen difficulty: the default for New Game. A continued run keeps the one saved with it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Gameplay")
	ETDDifficulty Difficulty = ETDDifficulty::Standard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
	float MasterVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
	float MusicVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
	float EffectsVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Video")
	ETDWindowMode WindowMode = ETDWindowMode::WindowedFullscreen;

	/** 0,0 = the desktop resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Video")
	FIntPoint Resolution = FIntPoint::ZeroValue;

	/**
	 * The player has chosen video options in a menu. Until then the game keeps the engine's own window settings
	 * (GameUserSettings, -windowed / -ResX), so a first start never forces a mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Video")
	bool bVideoChosen = false;
};

UCLASS()
class UNREALGAMEJAM_API UTDSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(SaveGame)
	int32 Version = CurrentVersion;

	UPROPERTY(SaveGame)
	FTDPlayerSettings Settings;
};

UCLASS()
class UNREALGAMEJAM_API UTDSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Settings", meta = (WorldContext = "WorldContextObject"))
	static UTDSettingsSubsystem* GetTDSettings(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Settings")
	const FTDPlayerSettings& GetSettings() const { return Settings; }

	// ---- On-foot camera (ENG-32). The ground character binds OnOnFootCameraModeChanged and reads the mode at BeginPlay.
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Settings") ETDOnFootCamera GetOnFootCameraMode() const { return Settings.OnFootCameraMode; }
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetOnFootCameraMode(ETDOnFootCamera Mode);
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void ToggleOnFootCameraMode();

	/** Fires on every change of the on-foot camera mode (menu, V / View, reset, ApplyAll). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Settings")
	FTDOnFootCameraModeChanged OnOnFootCameraModeChanged;

	// ---- The Settings page (ENG-32): Nick's UTDUIStyle::SettingsWidgetClass if set, else the code-built page.
	/** Viewport layer of the settings page: above every HUD and the pause screen (UI host ZOrder + 100), which opens it. */
	static constexpr int32 PageZOrder = 1000;

	/** Opens the page over the menu or the pause screen. False when there is no viewport to show it in. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings", meta = (WorldContext = "WorldContextObject"))
	static bool OpenSettingsPage(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings", meta = (WorldContext = "WorldContextObject"))
	static void CloseSettingsPage(const UObject* WorldContextObject);

	/** Menus keeping their own focus check this and leave the page alone while it is up. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Settings", meta = (WorldContext = "WorldContextObject"))
	static bool IsSettingsPageOpen(const UObject* WorldContextObject);

	/** The widget class the page uses: UTDUIStyle::SettingsWidgetClass when the style asset has one, else null. */
	static UClass* ResolveSettingsWidgetClass(const UObject* Style);

	// ---- Controls
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetMouseSensitivity(float Multiplier);
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetInvertMouseY(bool bInvert);
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetGamepadSensitivity(float Multiplier);
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetGamepadDeadzone(float Deadzone);
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetInvertGamepadY(bool bInvert);

	// ---- Gameplay
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetHintsEnabled(bool bEnabled);

	/**
	 * Difficulty (features.md Settings; T4's UTDDifficultySettings presets). Saved here as the New Game default and
	 * written to the current run through the game instance. Enemies read it when they spawn, so ships already in the
	 * fight keep their strength: it takes effect as new enemies and waves arrive.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetDifficulty(ETDDifficulty Difficulty);

	/** The difficulty in play now (the run's; may differ from the saved preference after a Continue). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Settings") static ETDDifficulty GetDifficultyInPlay();

	/** Makes the saved preference the difficulty in play (start-up and New Game). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void ApplyPreferredDifficulty();

	/** "Story" / "Standard" / "Ace", and a one-line description of what that preset changes (from T4's tables). */
	static FText DifficultyName(ETDDifficulty Difficulty);
	static FText DifficultyDescription(ETDDifficulty Difficulty);

	// ---- Audio (0..1)
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetMasterVolume(float Volume);
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetMusicVolume(float Volume);
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetEffectsVolume(float Volume);

	// ---- Video
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetWindowMode(ETDWindowMode Mode);
	/** 0,0 = desktop resolution. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void SetResolution(FIntPoint Resolution);
	/** The fullscreen resolutions the display offers (for a menu list). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") TArray<FIntPoint> GetSupportedResolutions() const;

	/** Everything back to the shipped defaults (applied and saved). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void ResetToDefaults();

	/** Replace every option at once (a menu's Apply button). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Settings") void ApplyAll(const FTDPlayerSettings& NewSettings);

	/** Pushes the control options into one controller (called from its BeginPlay and on every change). */
	void ApplyToController(ATDSpacePlayerController* PC) const;

	/** Tests: use another slot (and no video changes) so the player's options are never touched. */
	void UseSlotForTests(const FString& SlotName);
	/** Tests: read the slot again, as a fresh start-up would. */
	void ReloadForTests() { Load(); }

	/** What audio actually received (tests): master on the device, and the class overrides found. */
	float GetAppliedMasterVolume() const { return AppliedMaster; }
	bool HasMusicClass() const { return MusicClass != nullptr; }
	bool HasEffectsClass() const { return EffectsClass != nullptr; }

	static constexpr const TCHAR* DefaultSlot = TEXT("TD_Settings");

private:
	void Changed(bool bVideo = false, bool bAudio = false);
	void Load();
	void Save() const;
	void ApplyAudio();
	void ApplyVideo(bool bStartup = false);
	void ApplyControls();

	FTDPlayerSettings Settings;
	FString SlotName = DefaultSlot;
	bool bApplyVideo = true;
	float AppliedMaster = 1.f;

	void BroadcastCameraIfChanged(ETDOnFootCamera Before);
	TSharedPtr<class STDSettingsPage> Page;
	UPROPERTY(Transient) TObjectPtr<class UUserWidget> PageWidget;
	bool bPageCursorWasShown = false;
	FDelegateHandle PreLoadMapHandle;
	void ClosePageInternal(UWorld* World);
	friend class STDSettingsPage;

	UPROPERTY(Transient) TObjectPtr<USoundMix> VolumeMix;
	UPROPERTY(Transient) TObjectPtr<USoundClass> MusicClass;
	UPROPERTY(Transient) TObjectPtr<USoundClass> EffectsClass;
};

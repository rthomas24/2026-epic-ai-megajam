// Triad Drift - UI host settings (Teammate 2). The one switch for Nick's native space HUD.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "TDUIHostSettings.generated.h"

/** Which space HUD the host shows in space gameplay (ENG-36). Every one stays selectable. */
UENUM(BlueprintType)
enum class ETDSpaceHUDChoice : uint8
{
	/** Not set: the legacy bUseNativeSpaceHUD decides (true = Nick, false = Dev). */
	Unset,
	/** Engineering's HUD (UTDFlightHUDWidget, ENG-36, docs/eng/hud-redesign.md). */
	Engineering,
	/** Nicholas's HUD (Plugins/TriadDriftHUD, WBP_SpaceHUD). */
	Nick,
	/** No widget: the dev HUD (ATDDebugHUD) draws its full read-out. */
	Dev
};

/**
 * Project Settings > Game > Triad Drift UI Host.
 * Whether the engineering host puts Nick's native space HUD (Plugins/TriadDriftHUD) on screen in space
 * gameplay. Console override: TD.UI.NativeHUD (-1 = use this setting, 0 = force off, 1 = force on).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Triad Drift UI Host"))
class UNREALGAMEJAM_API UTDUIHostSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UTDUIHostSettings* Get() { return GetDefault<UTDUIHostSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Show the native space HUD in space gameplay. Off until the Lead has seen it in play. */
	UPROPERTY(Config, EditAnywhere, Category = "Space HUD")
	bool bUseNativeSpaceHUD = false;

	/**
	 * Widget created for the space HUD (Nick's Widget Blueprint, a child of UTDSpaceHUDWidget). Loaded by
	 * path, so /Game/TriadDrift/UI/TriadHUD must be in the always-cook list.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Space HUD", meta = (MetaClass = "/Script/UMG.UserWidget"))
	FSoftClassPath SpaceHUDClass = FSoftClassPath(TEXT("/Game/TriadDrift/UI/TriadHUD/WBP_SpaceHUD.WBP_SpaceHUD_C"));

	/** Used when SpaceHUDClass can't be loaded: the plugin's native class itself. */
	UPROPERTY(Config, EditAnywhere, Category = "Space HUD", meta = (MetaClass = "/Script/UMG.UserWidget"))
	FSoftClassPath NativeFallbackClass = FSoftClassPath(TEXT("/Script/TriadDriftHUD.TDSpaceHUDWidget"));

	/** Viewport Z order. Above gameplay widgets, below menus. */
	UPROPERTY(Config, EditAnywhere, Category = "Space HUD")
	int32 ZOrder = 10;

	/**
	 * Which space HUD to show. Set SpaceHUD=Engineering (or Nick, Dev) in DefaultGame.ini; Unset keeps the legacy
	 * bUseNativeSpaceHUD meaning. Console: TD.UI.SpaceHUD (-1 = settings, 0 = dev, 1 = Nick, 2 = engineering).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Space HUD")
	ETDSpaceHUDChoice SpaceHUD = ETDSpaceHUDChoice::Unset;

	/**
	 * The space HUD in effect: TD.UI.SpaceHUD if set; else TD.UI.NativeHUD 1 forces Nick's (0 turns his off, to the dev
	 * HUD); else SpaceHUD; else bUseNativeSpaceHUD (Nick or Dev). Never Unset.
	 */
	static ETDSpaceHUDChoice GetSpaceHUDChoice();

	/** GetSpaceHUDChoice() == Nick (Nicholas's widget is the one to show). */
	static bool IsNativeSpaceHUDEnabled();

	/** Show the engineering on-foot HUD (UTDGroundHUDWidget, or UTDUIStyle::GroundHUDWidgetClass) on foot. */
	UPROPERTY(Config, EditAnywhere, Category = "Ground HUD")
	bool bUseNativeGroundHUD = true;

	/** Show the pause screen (UTDPauseMenuWidget, or UTDUIStyle::PauseWidgetClass) while the player has paused. */
	UPROPERTY(Config, EditAnywhere, Category = "Pause")
	bool bUseNativePauseScreen = true;

	/** TD.UI.GroundHUD if set (0/1), else bUseNativeGroundHUD. Off = the dev HUD's text read-out on foot. */
	static bool IsNativeGroundHUDEnabled();

	/** TD.UI.PauseScreen if set (0/1), else bUseNativePauseScreen. Off = the dev HUD's PAUSED line. */
	static bool IsNativePauseScreenEnabled();
};

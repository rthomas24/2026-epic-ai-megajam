// Triad Drift - UI host settings (Teammate 2). The one switch for Nick's native space HUD.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "TDUIHostSettings.generated.h"

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

	/** True when the space HUD should be used: TD.UI.NativeHUD if set (0/1), else bUseNativeSpaceHUD. */
	static bool IsNativeSpaceHUDEnabled();
};

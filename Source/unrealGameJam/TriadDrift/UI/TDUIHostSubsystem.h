// Triad Drift - engineering host for Nick's native space HUD (Teammate 2). See docs/eng/ui-host.md.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDUIHostSubsystem.generated.h"

class UUserWidget;
class APlayerController;

/**
 * The HUD surfaces the fallback (dev) HUD can draw on top of the native space HUD until Nick's widget shows them.
 * The fallback HUD asks UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD for each one and skips the covered ones.
 */
UENUM(BlueprintType)
enum class ETDHUDSurface : uint8
{
	DamageIndicators,	// hit-direction chevrons (ITDHUDData::GetDamageIndicators)
	SquadMarkers,		// named wing arrows (GetSquadMarkers)
	InteractPrompt,		// landing prompt / switch refusal (GetInteractPrompt)
	HyperdriveBlockText,	// why the hyperdrive is locked (GetHyperdriveBlockText)
	Altitude,		// atmosphere flight (GetAltitude)
	Subtitles,		// bark subtitles
	Pause,			// the PAUSED line / menu
	BoundaryWarning		// edge-of-zone text (GetBoundaryWarningText)
};

/**
 * One squad card as the HUD widget decoded it (read back from the widget's reflected snapshot). For
 * tests and diagnostics only: the widget, not the host, draws it.
 */
struct FTDHostShipView
{
	float Hull = 0.f;
	float Shield = 0.f;
	float Power = 0.f;
	float Power2 = 0.f;
	float Ability = 0.f;
	float RespawnSeconds = 0.f;
	float Boost = 0.f;
	float Heat = 0.f;
	int32 Ammo = 0;
	bool bActive = false;
	bool bAlive = false;
	bool bUnlocked = false;
	bool bAvailable = false;
};

/**
 * Puts Nick's UTDSpaceHUDWidget (Plugins/TriadDriftHUD) on the local player's screen during space
 * gameplay and keeps it there across ship switches.
 *
 * - Shown when the switch is on (UTDUIHostSettings / TD.UI.NativeHUD), this is a game world, a local
 *   player controller exists, the world has a squad manager, and that controller flies an ATDShip.
 *   So menu and transition maps (no squad manager) and on-foot play (pawn isn't a ship) get none.
 * - The data source is the squad manager (ITDHUDData). The widget samples it on its own timer.
 * - The subsystem lives and dies with its world: level travel removes the widget, arrival re-creates it.
 * - No compile-time dependency on the plugin: the class is loaded by path and its functions are called
 *   through reflection. If the plugin or the names it exposes change, the host logs once and stays off
 *   (the dev HUD keeps drawing); TriadDrift.UI.HUDHost.FrozenNames fails on the change.
 */
UCLASS()
class UNREALGAMEJAM_API UTDUIHostSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDUIHostSubsystem* Get(const UObject* WorldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Re-check the conditions now and create, re-target or remove the widget. Ticked every 0.2 s. */
	void Evaluate();

	/** The live space HUD widget, or null while it isn't shown. */
	UUserWidget* GetSpaceHUD() const { return Widget; }

	bool IsSpaceHUDShown() const { return Widget != nullptr; }

	/** Data source currently handed to the widget. */
	UObject* GetDataSource() const { return DataSource.Get(); }

	/** Loads the configured widget class (falls back to the plugin's native class). Null if neither loads. */
	static UClass* ResolveWidgetClass();

	/** The plugin's native widget class, found by name (null when the plugin isn't loaded). */
	static UClass* FindNativeWidgetClass();

	/** UTDSpaceHUDWidget::SetDataSource through reflection. False if the function is missing or changed. */
	static bool CallSetDataSource(UUserWidget* HUD, UObject* Source);

	/** UTDSpaceHUDWidget::RefreshData through reflection. */
	static bool CallRefreshData(UUserWidget* HUD);

	/** Reads the widget's decoded squad snapshot (its reflected "Ships" array). False if unreadable. */
	static bool ReadSnapshot(const UUserWidget* HUD, TArray<FTDHostShipView>& Out);

	/**
	 * True when the native space HUD is on screen in WorldContext's world AND its widget draws Surface. Nick's side
	 * switches a surface on by adding a bool UPROPERTY named "bCovers<Surface>" (e.g. bCoversDamageIndicators) set to
	 * true on the widget class or its Blueprint; until then every surface reads false and the fallback HUD keeps
	 * drawing it. Also forced per surface by the console: TD.UI.Covers <Surface> 0|1 (dev, -1 = from the widget).
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static bool IsSurfaceCoveredByNativeHUD(const UObject* WorldContext, ETDHUDSurface Surface);

	/** The reflected property name the widget uses to claim Surface ("bCoversDamageIndicators", ...). */
	static FName GetCoverPropertyName(ETDHUDSurface Surface);

	/** When false the widget is created but never added to the viewport (headless tests). */
	bool bAddToViewport = true;

private:
	APlayerController* FindLocalController() const;
	bool WantsSpaceHUD(APlayerController*& OutController, UObject*& OutSource) const;
	void ShowFor(APlayerController* Controller, UObject* Source);
	void Hide();
	void SetDevHUDSuppressed(bool bSuppressed);

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Widget;

	TWeakObjectPtr<APlayerController> Owner;
	TWeakObjectPtr<UObject> DataSource;
	float TimeUntilCheck = 0.f;
	bool bClassWarningLogged = false;
	bool bDevHUDSuppressed = false;
};

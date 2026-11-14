// Triad Drift - engineering host for Nick's native space HUD (Teammate 2). See docs/eng/ui-host.md.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/UI/TDUIHostSettings.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDUIHostSubsystem.generated.h"

class UUserWidget;
class APlayerController;
class UMeshComponent;

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
	BoundaryWarning,	// edge-of-zone text (GetBoundaryWarningText)
	CombatStates,		// the Switch Strike window and a focus-fire order (GetSwitchStrike01, GetFocusRemaining01)
	ObjectiveMarkers	// waypoint diamonds: planets, gates, generators, landing pads (GetObjectiveMarkers)
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

	// ---- on foot and pause (engineering widgets in the space HUD's style; Nick restyles through UTDUIStyle)

	/** The on-foot HUD while the local player walks (null in space, menus and with TD.UI.GroundHUD 0). */
	UUserWidget* GetGroundHUD() const { return GroundWidget; }

	/** The pause screen while the player has paused (null otherwise). */
	UUserWidget* GetPauseScreen() const { return PauseWidget; }

	/** UTDUIStyle::GroundHUDWidgetClass when set and loadable, else UTDGroundHUDWidget. */
	static UClass* ResolveGroundWidgetClass();

	/** UTDUIStyle::PauseWidgetClass when set and loadable, else UTDPauseMenuWidget. */
	static UClass* ResolvePauseWidgetClass();

	/**
	 * Hides every HUD while a cinematic plays (T3's intro, ENG-33): true at the start, false at the end. Takes effect at
	 * once: Nick's space HUD, the on-foot HUD and the pause screen are removed (and the pause screen can't open while
	 * hidden), and the fallback debug HUD stops drawing (the local player's AHUD::bShowHUD, restored to exactly what it
	 * was). The HUDs come back by themselves on false.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI")
	void SetCinematicHidden(bool bHidden);

	/**
	 * A story transmission (T3's transmissions system, "The Paradox"): Line in the space HUD's subtitle slot for Seconds,
	 * speaker tag in the pilot's colour. Style: 0 = clean (squad), 1 = static ("Unknown"), 2 = echo ("Older"). Returns
	 * false when no HUD that shows transmissions is up (Nick's HUD, the dev HUD, on foot): the caller may fall back.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static bool ShowTransmission(const UObject* WorldContext, FName Speaker, FText Line, uint8 Style, float Seconds);

	/**
	 * Aftermath (storyline.md section 12): the tactical display of contacts leaving the Dominion in lanes toward Verdance,
	 * for Seconds (about 20). Its own widget, so it shows during a cinematic that hides the HUD. False without a player.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static bool ShowTacticalDisplay(const UObject* WorldContext, float Seconds = 20.f);

	/**
	 * Aftermath's navigation pulse: the flight HUD faintly marks the corridor From -> To (world) for Seconds, then clears;
	 * it must read as sensor noise. False when the engineering flight HUD isn't up.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static bool ShowNavigationPulse(const UObject* WorldContext, FVector From, FVector To, float Seconds = 1.5f);

	/**
	 * storyline.md §18, the Verdance traverse: a sensor marker on a squad transponder at At reads "UNKNOWN", then
	 * "VERIFIED" for a moment (SensorVerifiedAt s in), then "UNKNOWN" again; shown for Seconds on the on-foot HUD. Nobody
	 * explains it. T5 calls it at the trigger point on the route.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static bool ShowSensorFlicker(const UObject* WorldContext, FVector At, float Seconds = 6.f);
	/** The sensor marker while it shows: where, and seconds since it started. */
	bool GetSensorFlicker(FVector& OutAt, float& OutAge) const;
	static constexpr float SensorVerifiedAt = 1.4f;

	/**
	 * A focus hold on the flight HUD, by reason (T1's atmosphere entry: e.g. Reason "Entry", Opacity 0.35, bHideCombat true).
	 * While any hold is up the whole HUD eases to the lowest Opacity, and with bHideCombat the objective pill and waypoints
	 * leave the screen. Pushing the same Reason again replaces it; PopHUDFocus ends it. Holds survive a widget re-creation
	 * (ship switch) and end with the world. Cheap to call every frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static void PushHUDFocus(const UObject* WorldContext, FName Reason, float Opacity, bool bHideCombat);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static void PopHUDFocus(const UObject* WorldContext, FName Reason);

	/** True while a hold asks for combat presentation to stay off screen: wave slates and combat cards check this (T3, T4). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static bool IsCombatPresentationHeld(const UObject* WorldContext);

	/**
	 * The scan monitor for Cinematic 03 and 05 (T3): Stage 1 the Verdance structure, 2 + scan sites, 3 + the Cinder
	 * correlation, 4 the Cinder overlay with matching angles, 5 + the crystal cache and field anchor markers. Calling again
	 * with a higher stage draws the new layers in. Seconds 0 = until HideScanMonitor. Its own widget: shows during a
	 * cinematic that hides the HUD. False without a local player.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static bool ShowScanMonitor(const UObject* WorldContext, int32 Stage, float Seconds = 0.f);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static void HideScanMonitor(const UObject* WorldContext);

	UUserWidget* GetScanMonitor() const { return ScanWidget; }

	/**
	 * The terminal's in-world screen shows the same scan (issue #62): a dynamic instance on Mesh's material slot
	 * ScreenSlot (by name; None = slot 0) gets texture parameter "ScreenTexture" = UTDUIStyle::ScanVerdance or
	 * ScanCinder and scalar "ScreenPower" = Power (the structures builder's console material). False when there is no
	 * mesh, no such slot or no scan texture (the screen keeps its own look). Call when the terminal is used (T5 / T3).
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI")
	static bool ShowScanOnScreen(UMeshComponent* Mesh, FName ScreenSlot, bool bCinder, float Power = 1.f);

	/** The tactical display while it plays (tests). */
	UUserWidget* GetTacticalDisplay() const { return TacticalWidget; }

	/** SetCinematicHidden on WorldContext's world (no-op without a game world). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|UI", meta = (WorldContext = "WorldContext"))
	static void SetCinematicHiddenIn(const UObject* WorldContext, bool bHidden);

	bool IsCinematicHidden() const { return bCinematicHidden; }

	/**
	 * The event bus's OnHitConfirmed: plays DA_UI_Style's KillConfirm (a kill) or HitConfirm (a hit) as a 2D sound, no
	 * overrides (the asset's class, volume and concurrency apply). Silent when the slot is unset.
	 */
	UFUNCTION()
	void HandleHitConfirmed(AActor* Target, bool bKill);

	/** Tests: use this style instead of DA_UI_Style. */
	UPROPERTY(Transient)
	TObjectPtr<class UTDUIStyle> StyleOverride;

	/** Combat feedback sounds played so far (tests, diagnostics). */
	int32 GetHitSoundsPlayed() const { return HitSoundsPlayed; }
	int32 GetKillSoundsPlayed() const { return KillSoundsPlayed; }
	int32 GetHitSoundsThrottled() const { return HitSoundsThrottled; }

	/** The hit tick plays at most once per this many real seconds (the Lead: about 60 ms). Kills are never throttled. */
	float HitSoundMinInterval = .06f;

	/** The controller's OnPauseChanged: opens or closes the pause screen. */
	UFUNCTION()
	void HandlePauseChanged(bool bPaused);

	/** The reward screen's OnRewardDismissed: acknowledge that award, then present the next pending one at once. */
	UFUNCTION()
	void HandleRewardDismissed(FName AbilityId);

	/** The mounted ability reward screen, and the award it shows (None when idle). */
	UUserWidget* GetRewardWidget() const { return RewardWidget; }
	ETDAbilityId GetPresentedReward() const { return PresentedReward; }
	/** Tests: mount this class instead of DA_UI_Style's / Nick's (a stand-in with the same functions and fields). */
	UPROPERTY(Transient)
	TObjectPtr<UClass> RewardWidgetClassOverride;
	/** Present / refresh / mount the pending award now (also run on every host check). */
	void UpdateAbilityReward();

private:
	APlayerController* FindLocalController() const;
	bool WantsSpaceHUD(APlayerController*& OutController, UObject*& OutSource) const;
	void ShowFor(APlayerController* Controller, UObject* Source);
	void Hide();
	void SetDevHUDSuppressed(bool bSuppressed);
	/** Dev HUD in supplement mode while any engineering-hosted HUD (space or ground) is up. */
	void UpdateDevHUDSuppression();
	bool WantsGroundHUD(APlayerController* PC) const;
	void ShowGround(APlayerController* PC);
	void HideGround();
	void BindPause(APlayerController* PC);
	void BindHitSounds(bool bBind);
	void ShowPause(APlayerController* PC);
	void HidePause();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Widget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> GroundWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> PauseWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TacticalWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ScanWidget;
	FVector SensorAt = FVector::ZeroVector;
	double SensorStart = -1e9;
	float SensorSeconds = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> RewardWidget;
	ETDAbilityId PresentedReward = ETDAbilityId::None;
	bool bRewardShownGamepad = false;
	bool bRewardShownUsable = false;
	UClass* ResolveRewardWidgetClass() const;

	struct FTDHUDFocus
	{
		float Opacity = 1.f;
		bool bHideCombat = false;
	};
	TMap<FName, FTDHUDFocus> Focus;
	/** Push the focus holds onto the live flight HUD (every tick, and on push / pop). */
	void ApplyFocus();

	TWeakObjectPtr<APlayerController> GroundOwner;
	TWeakObjectPtr<APlayerController> PauseBoundTo;
	TWeakObjectPtr<class UTDEventBus> HitBus;
	int32 HitSoundsPlayed = 0;
	int32 KillSoundsPlayed = 0;
	int32 HitSoundsThrottled = 0;
	double LastHitSoundTime = -1e9;

	TWeakObjectPtr<APlayerController> Owner;
	TWeakObjectPtr<UObject> DataSource;
	/** The choice the live widget was made for (a change of TD.UI.SpaceHUD replaces it). */
	ETDSpaceHUDChoice ShownChoice = ETDSpaceHUDChoice::Unset;
	float TimeUntilCheck = 0.f;
	bool bClassWarningLogged = false;
	bool bDevHUDSuppressed = false;
	bool bCinematicHidden = false;
	/** The local AHUD's bShowHUD before the cinematic (restored exactly). */
	TWeakObjectPtr<class AHUD> CinematicHUD;
	bool bCinematicHUDWasShown = true;
};

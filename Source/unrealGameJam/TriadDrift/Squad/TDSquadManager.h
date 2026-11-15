// Triad Drift - Squad (Teammate 3). ENG-07: squad manager & instant switching (PROTECT), ENG-03 respawn.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSwitchCut.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Squad/TDSquadBarks.h"
#include "TDSquadManager.generated.h"

class ATDShip;
class ATDWingmanAIController;
class APlayerController;
class UTDShipVisuals;

/** C-03 #1: a switch, as a director's cut. Same frame as the switch; presentation only. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnSwitchCut, const FTDSwitchCut&, Cut);

/** Why a switch request was refused (HUD feedback, M6). */
UENUM(BlueprintType)
enum class ETDSwitchRefusal : uint8
{
	Cooldown,
	AlreadyActive,
	TargetDown,
	HyperdriveActive,
	Landing,
	/** Any other T1 switch lock (atmospheric entry); see GetLastRefusalText. */
	Locked
};

/** Why the active ship changed (logged on every switch: "TDSQUAD switch A -> B cause=..."). */
UENUM(BlueprintType)
enum class ETDSwitchCause : uint8
{
	None,			// a path that didn't say: a bug if it ever shows up in the log
	Manual,			// the player's own press (Tab / 1-2-3 / cheats / bots calling RequestSwitch*)
	Buffered,		// a press made during the cooldown, run when it ended (at most one cooldown later)
	AutoOnDeath,	// the player's ship finished dying: cut to the healthiest survivor
	RespawnRescue,	// no flow took the wipe; a ship came back while the player sat in a downed one
	Retake			// SpawnSquad: the take restarts in StartingShip
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnSwitchRefused, ETDSwitchRefusal, Reason, ETDShipId, RequestedShip);

/** #201: shadow mode changed (true = wingmen shadow the player; false = they hunt alone, stronger). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnShadowModeChanged, bool, bShadowOn);

/** Life state of one squad slot. */
UENUM(BlueprintType)
enum class ETDSquadSlotState : uint8
{
	NotSpawned,
	Alive,
	/** Death tumble (0.6 s) before the ship is hidden and the respawn timer runs. */
	Dying,
	/** Hidden, waiting for the respawn timer. */
	Downed
};

/** Runtime record for one of the three ships. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDSquadSlot
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Squad")
	ETDShipId ShipId = ETDShipId::Rift;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<ATDShip> Ship = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<ATDWingmanAIController> Wingman = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Squad")
	ETDSquadSlotState State = ETDSquadSlotState::NotSpawned;

	/** World time at which a Dying/Downed ship comes back. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Squad")
	double RespawnAtTime = 0.0;

	/** World time at which this ship was downed (Dying started). */
	double DownedAtTime = 0.0;

	/** Death tumble motion (Squad drives it while the flight model is parked). */
	FVector TumbleVelocity = FVector::ZeroVector;

	/** Real time this ship last received the manual switch-in grace (M4 lockout). */
	double LastManualGraceTime = -1000.0;

	/** The low-hull line is armed (re-armed once the hull is repaired). */
	bool bLowHullBarkArmed = true;
	/** Real time the hull first dipped low this time (the line only plays close to that moment). */
	double LowHullSince = -1.0;
	double NextLowHullTry = 0.0;
	double NextTumblePuffTime = 0.0;
	/** Real time of this wingman's last engage call. */
	double LastEngageCallTime = -1000.0;
};

/**
 * Owns the three squad ships and who the player flies.
 *
 * - SpawnSquad spawns Rift/Lancer/Bastion, applies their ENG-04 specs and gives each a wingman AI.
 * - RequestSwitch* possesses the target on the SAME frame; the ship you left keeps its velocity and
 *   is handed to its wingman AI. 0.4 s cooldown; downed ships can't be selected.
 * - Active ship death auto-switches to the healthiest living ship; downed ships warp back after 12 s
 *   with 2 s of invulnerability. All three down = OnSquadWiped + ITDGameFlow::ReportPlayerDefeated.
 * - Switch Strike: switching within 1.5 s of using a power gives +25% damage for 4 s.
 * - Fires OnShipSwitched / OnShipDowned / OnShipRespawned / OnSquadWiped on UTDEventBus.
 * - Implements ITDHUDData for the space HUD.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDSquadManager : public AActor, public ITDHUDData
{
	GENERATED_BODY()

public:
	ATDSquadManager();

	/** The squad manager of this world (registered on BeginPlay), or null. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad", meta = (WorldContext = "WorldContext"))
	static ATDSquadManager* Get(const UObject* WorldContext);

	/**
	 * Spawn the three ships around Origin (StartingShip leads, wingmen in their V slots) and give the
	 * player StartingShip. Safe to call again for a retake (C-03 #3): every ship is revived to full
	 * health, unhidden, repositioned at Origin at cruise speed and handed back to its wingman; switch
	 * cooldown, Switch Strike, focus, respawn timers and the wipe state are all reset. No events fire
	 * for the reset itself. Set StartingShip first to resume in a different ship.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	void SpawnSquad(const FTransform& Origin);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	ATDShip* GetShip(ETDShipId Ship) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	ATDShip* GetActiveShip() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	ETDShipId GetActiveShipId() const { return ActiveShipId; }

	/** Instant switch. False if on cooldown, the target is downed/missing, or it is already active. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	bool RequestSwitchTo(ETDShipId Ship);

	/** Next living ship in SwitchOrder (wraps). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	bool RequestSwitchNext();

	/** Previous living ship in SwitchOrder (wraps). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	bool RequestSwitchPrev();

	/** All slots (no allocation; check State == Alive). For per-frame use by wingmen and tests. */
	const TArray<FTDSquadSlot>& GetSlots() const { return Slots; }

	/** Living (Alive) ships, in SwitchOrder. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	TArray<ATDShip*> GetLivingShips() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	bool IsShipAlive(ETDShipId Ship) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	ETDSquadSlotState GetShipState(ETDShipId Ship) const;

	/** Seconds until a downed ship returns (0 when alive). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	float GetRespawnSeconds(ETDShipId Ship) const;

	/** 0..1 remaining switch cooldown (0 = ready). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	float GetSwitchCooldown01() const;

	/** "Focus my target" (IA_SquadCommand): wingmen prioritise Target for FocusCommandSeconds. Null = clear. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	void IssueFocusCommand(AActor* Target);

	// ---------------- Shadow mode (#201; tunables: Squad/TDShadowMode.h, td.Squad.*) ----------------

	/** True (default): the wingmen shadow the player's ship inside a leash and cover it. False: they hunt on their
	 *  own, about three times further out (capped), hit harder and aim tighter, and no longer cover the player. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	bool IsShadowMode() const { return bShadowMode; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	void SetShadowMode(bool bOn);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	void ToggleShadowMode() { SetShadowMode(!bShadowMode); }

	/** Fired on every real change of shadow mode (the HUD's SHADOW ON / OFF indicator). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Squad")
	FTDOnShadowModeChanged OnShadowModeChanged;

	/** The leash the wingmen are on right now (cm): the shadow leash, or the capped solo leash. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	float GetWingmanLeash() const;

	/** Weapon-component multiplier key for the solo (shadow off) damage buff. */
	static const FName ShadowSoloKey;

	// ---- #201 "your fight" (shadow ON) ----
	/**
	 * Called from the enemy ship's damage pre-filter with the amount about to land. Notes the player's own hits (the
	 * enemy becomes "the player's" for td.Squad.Shadow.YourFightSeconds) and, in shadow mode, cuts a WINGMAN's damage
	 * so that enemy keeps td.Squad.Shadow.YourFightFloor of its max hull: the kill is the player's.
	 */
	static float FilterDamageForPlayersFight(AActor* Victim, const FTDDamageInfo& Info, float Amount);
	/** The player hit, targeted or locked Enemy within the last YourFightSeconds. */
	bool IsPlayerClaimed(const AActor* Enemy) const;
	/** Enemy is the player's and already down at the floor the wingmen may not go below. */
	bool IsHeldAtFloorForPlayer(const AActor* Enemy) const;
	/** Shadow ON and the wave number is within td.Squad.Shadow.EarlyWaves: soft wingmen, the first kills are the player's. */
	bool IsEarlyWaveForWingmen() const;
	/** Weapon-component multiplier key for the early-wave wingman damage cut. */
	static const FName ShadowEarlyKey;

	/** #201: world time of the last power any wingman used (the squad-wide gap, td.Squad.Powers.Gap). */
	double LastWingmanPowerTime = -1000.0;
	/** #201 playtest (TD.Squad.SwitchOnRam): switch into Lancer the moment its wingman's ram dash starts. One shot. */
	bool bDebugSwitchOnWingmanRam = false;

	/** TD.Squad.Dump: one line per ship (mode, state, target, distance to the player, power states). */
	void DumpSquad(TArray<FString>& OutLines) const;

	/** True while the Switch Strike damage buff is running. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	bool IsSwitchStrikeActive() const;
	/** HUD feedback (Lead item 2): Switch Strike time left, 1 at arming -> 0 at its end (0 when off). */
	float GetSwitchStrikeRemaining01() const;
	/** ITDHUDData: Switch Strike time left (Core's fallback HUD draws it). */
	virtual float GetSwitchStrike01() const override { return GetSwitchStrikeRemaining01(); }
	/** HUD feedback (Lead item 3): the "focus my target" order's time left, 1 -> 0 (0 without an order). */
	virtual float GetFocusRemaining01() const override;

	/** Placeholder feedback until Nick's art: squad-colour pulses at the Switch Strike ship, white pulses
	 *  on the focus target, every this many seconds. 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Feedback", meta = (ClampMin = "0", Units = "s"))
	float FeedbackPulseInterval = 0.4f;
	double NextStrikePulse = 0.0;
	double NextFocusPulse = 0.0;

	/** Outgoing damage multiplier for a ship (1.25 on the active ship during Switch Strike, else 1). T2 reads this. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	float GetDamageMultiplier(const AActor* SourceShip) const;

	/**
	 * A signature power or ability fired (arms Switch Strike). Called automatically from
	 * UTDEventBus::OnPowerUsed, which T2 broadcasts; callable directly for tests and Blueprints.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	void NotifyPowerUsed(ETDShipId Ship);

	/** UTDEventBus::OnPowerUsed handler. */
	UFUNCTION()
	void HandlePowerUsed(ETDShipId Ship, FName PowerName);

	/** A manual switch was refused (cooldown, downed target, mid-hyperdrive, landing). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Squad")
	FTDOnSwitchRefused OnSwitchRefused;

	/** C-03 #1: fired in the same frame as every successful switch, after OnShipSwitched. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Squad")
	FTDOnSwitchCut OnSwitchCut;

	/** The most recent cut (SwitchIndex 0 = none yet this take). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	const FTDSwitchCut& GetLastSwitchCut() const { return LastSwitchCut; }

	/**
	 * Landing (ENG-20) and cinematics: passive wingmen release triggers, drop targets and stop
	 * sending flight input until made active again (they then regroup).
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	void SetWingmenPassive(bool bPassive);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	bool AreWingmenPassive() const { return bWingmenPassive; }

	/** Override the controller the squad drives (default: first local player controller). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	void SetSquadPlayerController(APlayerController* InController);

	/** Effective spec for a ship (override if set, else UTDShipLibrary). */
	const FTDShipSpec& GetSpec(ETDShipId Ship) const;

	/** Wingman controllers ask for their formation anchor (the player's ship). */
	ATDShip* GetFormationLeader() const { return GetActiveShip(); }

	/** World-space formation slot for a ship, relative to the current leader. */
	FVector GetFormationSlotLocation(ETDShipId Ship) const;

	/** Current focus target (null when none or expired). */
	virtual AActor* GetFocusTarget() const override;

	/** The player controller the squad drives (first local player by default). */
	APlayerController* GetSquadPlayerController() const;

	// ---------------- ITDHUDData (space half) ----------------
	virtual TArray<FTDShipHUD> GetSquadHUD() const override;
	virtual AActor* GetCurrentTarget() const override;
	virtual TArray<AActor*> GetLockedTargets() const override;
	virtual FTDWaveInfo GetWaveInfo() const override;
	virtual float GetSpeed01() const override;
	virtual TArray<FTDTargetMarker> GetTargetMarkers() const override;
	virtual ETDMissileWarning GetMissileWarning() const override;
	virtual bool GetLeadIndicator(FVector& OutWorldLocation) const override;
	virtual float GetHyperdriveCharge01() const override;
	virtual float GetAltitude() const override;
	virtual FText GetHyperdriveBlockText() const override;
	/** ENG-06: where to fly. Every live actor tagged ObjectiveMarkerTag (set by the world / flow actors
	 *  as scenes change); with none tagged, during a wave whose fighters are all off the player's
	 *  screen, the nearest enemy ("the fight is this way"). Cached, refreshed every ObjectiveRefreshSeconds. */
	virtual TArray<AActor*> GetObjectiveMarkers() const override;
	/** The player's ship's landing prompt ("Hold F to land", "SLOW DOWN TO LAND - ..."); empty otherwise. */
	virtual FText GetInteractPrompt() const override;
	/** One entry per squad ship for the HUD's wing markers; bEngaging while its wingman has a live target. */
	virtual TArray<FTDSquadMarker> GetSquadMarkers() const override;
	/** T1's feel pass: where the hits on the player's ship come from (newest first, real time). */
	virtual TArray<FTDDamageIndicator> GetDamageIndicators() const override;

	/** Damage indicators: how long a hit stays (real s), how many are kept, burst merge window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|HUD", meta = (Units = "s"))
	float DamageIndicatorSeconds = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|HUD", meta = (ClampMin = "1"))
	int32 MaxDamageIndicators = 6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|HUD", meta = (Units = "s"))
	float DamageIndicatorMergeSeconds = 0.15f;

	UFUNCTION()
	void HandleShipDamaged(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);
	virtual FText GetBoundaryWarningText() const override;
	/** Nick's HUD objective line: the Text of the last OnObjectiveUpdated (empty before the first). */
	virtual FText GetObjectiveText() const override { return ObjectiveText; }

	/** The actor tag that marks "the place to go now": Core's TDTags::ObjectiveMarker. */
	static const FName ObjectiveMarkerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|HUD", meta = (ClampMin = "0", Units = "s"))
	float ObjectiveRefreshSeconds = 0.5f;

	/** How long a refused switch's reason shows through GetInteractPrompt (real seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|HUD", meta = (ClampMin = "0", Units = "s"))
	float RefusalPromptSeconds = 1.5f;

	/** HUD-ready text for the last lock refusal (from T1's IsSwitchLocked). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	FText GetLastRefusalText() const { return LastRefusalText; }
	/** Why the active ship last changed (tests, logs). */
	ETDSwitchCause GetLastSwitchCause() const { return LastSwitchCause; }
	virtual bool IsOutOfBounds() const override;

	// ---------------- Tuning (features.md defaults) ----------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Classes")
	TSubclassOf<ATDShip> ShipClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Classes")
	TSubclassOf<ATDWingmanAIController> WingmanControllerClass;

	/** Optional per-ship spec overrides; unset ships use UTDShipLibrary::Get. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning")
	TMap<ETDShipId, FTDShipSpec> ShipSpecOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning")
	TArray<ETDShipId> SwitchOrder = { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning")
	ETDShipId StartingShip = ETDShipId::Rift;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float SwitchCooldown = 0.4f;

	/** Invulnerability for the ship an AUTO-switch (death) puts you in (ENG-03). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float AutoSwitchInvulnerability = 2.f;

	/** A short grace on a MANUAL switch-in (Lead decision: 0.35 s, not farmable)... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float ManualSwitchInvulnerability = 0.35f;

	/** ...granted to the same ship at most once per this many seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float ManualSwitchGraceLockout = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float DeathTumbleSeconds = 0.6f;

	/** Death tumble spin (deg/s, pitch / yaw / roll) and how fast its drift bleeds off (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning")
	FRotator DeathTumbleSpin = FRotator(260.f, 40.f, 620.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (ClampMin = "0"))
	float DeathTumbleDrag = 1.5f;

	/** Seconds between the placeholder trail puffs while a ship tumbles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Death", meta = (ClampMin = "0.02", Units = "s"))
	float DeathTumblePuffInterval = 0.12f;

	/** The player's own ship: seconds the camera holds on its explosion before the auto-switch cut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Death", meta = (ClampMin = "0", Units = "s"))
	float DeathExplosionHoldSeconds = 0.35f;

	/** After an auto-switch on death or a respawn rescue the newly flown ship takes no damage for this long (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Squad", meta = (ClampMin = "0"))
	float AutoSwitchGraceSeconds = 1.5f;
	/** The 1.5 s shield shimmer played on the ship for the grace (User.Color = the pilot's colour). A class default, so it cooks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Squad")
	TSoftObjectPtr<class UNiagaraSystem> SwitchGraceFX = TSoftObjectPtr<class UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Ship_SwitchGrace.NS_TD_Ship_SwitchGrace")));
	double PendingAutoSwitchAt = -1.0;
	void RunPendingAutoSwitch();

	/** Measured from the moment the ship is downed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float RespawnDelay = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float RespawnInvulnerability = 2.f;

	/** Respawned ships warp in this far behind the leader. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "cm"))
	float RespawnBehindDistance = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float SwitchStrikeWindow = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float SwitchStrikeDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning")
	float SwitchStrikeDamageMultiplier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (Units = "s"))
	float FocusCommandSeconds = 8.f;

	/** Suggested presentation timing copied into every FTDSwitchCut (C-03 #1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Cut")
	FTDSwitchCutTiming CutTiming;

	/** Where the wingmen fly, in the leader's heading + world-up frame (X forward, Y right, Z up).
	 *  A forward-swept wedge: both wingmen ahead and outboard of the leader at different depths, so
	 *  the chase camera (1400 cm behind) always has them in frame as a V, well clear of the crosshair
	 *  and the leader's line of fire (ENG-08: 1500-2500 cm from the leader). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Formation")
	FVector FormationLeftOffset = FVector(600.f, -1500.f, 220.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Formation")
	FVector FormationRightOffset = FVector(1500.f, 1750.f, -160.f);

	/** ENG-03: percent of max hull per second repaired on living squad ships between waves (after OnWaveCleared until OnWaveStarted). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Tuning", meta = (ClampMin = "0", Units = "Percent"))
	float BetweenWaveHullRepairPercentPerSecond = 8.f;

	/** Voice lines (ART-11): per-bark cooldowns, a global gap, a window cap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Barks")
	FTDBarkTuning BarkTuning;

	/** A low-hull line not played within this many real seconds of the dip is dropped (never late). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Barks", meta = (ClampMin = "0", Units = "s"))
	float LowHullBarkWindowSeconds = 1.5f;

	/** At most one ship-down line (any ship) per this many real seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Barks", meta = (ClampMin = "0", Units = "s"))
	float ShipDownBarkGapSeconds = 10.f;
	double LastShipDownBarkTime = -1000.0;
	/** Bark_Swarm_Saved (dialogs.md): Moss, when a swarm missile kills an enemy that was on a hurt wingman. */
	double LastSwarmSavedBarkTime = -1000.0;

	/** A wingman just entered a fight: it calls where it is, seen from the player's camera. */
	void NotifyWingmanEngaged(ETDShipId Ship);

	/** At most one engage call per wingman this often (real seconds), on top of the limiter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Barks", meta = (ClampMin = "0", Units = "s"))
	float EngageCallCooldownSeconds = 10.f;

	/** Ask for a squad line; false if the limiter held it back. */
	bool RequestSquadBark(FName BarkId, ETDShipId Speaker, ETDBarkPriority Priority = ETDBarkPriority::Normal);

	UFUNCTION()
	void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);

	/** Caches the flow's current objective line for GetObjectiveText (overwritten, never cleared). */
	UFUNCTION()
	void HandleObjectiveUpdated(FName ObjectiveId, int32 Step, FText Text);

	UFUNCTION()
	void HandleWaveCleared(int32 WaveNumber);

	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber, float Intensity01);

	// ENG-43: dialogs.md's T3 bark triggers (TDSquadMoreBarks.cpp).
	UFUNCTION()
	void HandleShieldBroken(UTDHealthComponent* Health, const FTDDamageInfo& Info);
	UFUNCTION()
	void HandleTakeStartedForBarks(FName SceneId, int32 TakeNumber);
	UFUNCTION()
	void HandleAbilityUnlockedForBarks(ETDAbilityId Ability, ETDShipId Ship);
	UFUNCTION()
	void HandleObjectiveForBarks(FName ObjectiveId, int32 Step, FText Text);
	/** T4's UTDArmadaForesight::OnAnticipated: a wingman notices (the paradox clue). */
	UFUNCTION()
	void HandleAnticipatedForBarks(class ATDEnemyShip* Enemy, FName Kind);
	/** Chances (tests set 1): a wingman reacting to the player's kill; Command's line on a retake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Barks", meta = (ClampMin = "0", ClampMax = "1"))
	float PlayerKillBarkChance = 1.f / 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Barks", meta = (ClampMin = "0", ClampMax = "1"))
	float RetakeBarkChance = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Barks", meta = (Units = "s"))
	float BreatherDelaySeconds = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Barks", meta = (Units = "s"))
	float OnMySixSeconds = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Barks", meta = (Units = "cm"))
	float OnMySixRange = 5000.f;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad")
	bool IsBetweenWaves() const { return bBetweenWaves; }

	/** Weapon-component multiplier key for Switch Strike (agreed with T2). */
	static const FName SwitchStrikeKey;

	/** Weapon-block key while a squad ship is down. */
	static const FName DownedWeaponsKey;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	FTDSquadSlot* FindSlot(ETDShipId Ship);
	const FTDSquadSlot* FindSlot(ETDShipId Ship) const;

	/** Core of switching; bForced bypasses the cooldown (auto-switch on death). */
	bool SwitchToInternal(ETDShipId Ship, bool bForced);
	/** SwitchToInternal with the cause recorded for the log and GetLastSwitchCause. */
	bool SwitchWithCause(ETDShipId Ship, bool bForced, ETDSwitchCause Cause);
	ETDSwitchCause CurrentSwitchCause = ETDSwitchCause::None;
	ETDSwitchCause LastSwitchCause = ETDSwitchCause::None;
	void LogSwitch(ETDShipId From, ETDShipId To, ETDSwitchCause Cause) const;
	bool CycleSwitch(int32 Direction);
	void BufferIfCoolingDown(int32 Kind, ETDShipId Ship);
	void RunPendingSwitch();

	void HandleShipDeath(ETDShipId Ship);
	void FinishDying(FTDSquadSlot& Slot);
	void RespawnShip(FTDSquadSlot& Slot);
	void HandleSquadWiped();

	/** Healthiest living ship (hull + shield fraction), excluding Except. False if none. */
	bool FindHealthiestLivingShip(ETDShipId Except, ETDShipId& OutShip) const;

	void StartSwitchStrike(ATDShip* OnShip);
	void EndSwitchStrike();
	ATDWingmanAIController* SpawnWingmanFor(FTDSquadSlot& Slot);
	double Now() const;
	/** Real (undilated) time: switch input must never be slowed by a money shot (M5). */
	double RealNow() const;
	void Refuse(ETDSwitchRefusal Reason, ETDShipId Ship);
	/** Everything a ship needs to fly again (respawn, retake): unpark, reset flight, weapons, powers, travel. */
	void RestoreShipForFlight(ATDShip* Ship, const FVector& Location, const FRotator& Facing, const FVector& Velocity);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Squad|State")
	TArray<FTDSquadSlot> Slots;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Squad|State")
	ETDShipId ActiveShipId = ETDShipId::Rift;

	double LastSwitchTime = -1000.0;
	double LastPowerTime = -1000.0;
	double SwitchStrikeEndTime = -1000.0;
	double FocusEndTime = -1000.0;
	TWeakObjectPtr<AActor> FocusTarget;
	TWeakObjectPtr<ATDShip> SwitchStrikeShip;
	TWeakObjectPtr<APlayerController> SquadPC;
	bool bSquadWiped = false;
	bool bWingmenPassive = false;
	bool bBetweenWaves = false;
	/** #201. Kept for the session (level travel keeps the player's choice); a fresh process starts ON. */
	bool bShadowMode = true;
	/** The solo buff sits on exactly the AI-flown ships while shadow mode is off (re-applied on a short timer too). */
	void RefreshShadowBuffs();
	double NextShadowBuffRefresh = 0.0;
	/** #201 "your fight": enemy -> world time of the player's last hit, target or lock on it. */
	TMap<TWeakObjectPtr<const AActor>, double> PlayerClaims;
	void NotePlayerClaim(const AActor* Enemy);
	/** A wingman must leave Enemy alive: it is the player's, or (early waves) the wingmen are not behind the player on kills. */
	bool ShouldWingmenHoldKill(const AActor* Enemy) const;
	int32 PlayerKillsThisWave = 0;
	int32 WingmanKillsThisWave = 0;
	double LastPlayerHitTime = -1000.0;
	double NextClaimScan = 0.0;
	mutable bool bEarlyWaveCached = false;
	mutable double NextEarlyWaveCheck = -1.0;
	void NotePlayerKill();
	void TickMoreBarks(double Now);
	bool RequestBarkFromAWingman(FName BarkId, ETDBarkPriority Priority);
	TArray<double> PlayerKillTimes;
	TMap<ETDShipId, double> ShieldsBroken;   // when it broke (ShieldBack retries while full, up to 15 s after)
	TSet<ETDShipId> ShieldDownPending;
	FName LastSpaceObjective = NAME_None;
	bool CommandSays(FName BarkId, ETDBarkPriority Priority);       // said on the next tick, unless the same hit left the hull low
	double BreatherAt = -1.0;
	double NextSixCheck = 0.0;
	TWeakObjectPtr<AActor> SixEnemy;
	double SixSince = -1.0;
	FTDBarkLimiter BarkLimiter;

	// Banter (dialogs.md §9): the exchange in progress, and the ones already heard.
	struct FTDBanterStep
	{
		FName Id;
		FName Speaker;
		double At = 0.0;
	};
	TArray<FTDBanterStep> BanterSteps;
	TSet<int32> BantersPlayed;
	bool StartBanter();
	void TickBanter();
public:
	/** Tests and tools: the banter steps still to come. */
	int32 GetPendingBanterSteps() const { return BanterSteps.Num(); }
private:
	FText LastRefusalText;
	double LastLockRefusalRealTime = -1000.0;
	mutable TDSquad::FWaveDirectorCache WaveDirectorCache;
	mutable TArray<TWeakObjectPtr<AActor>> ObjectiveCache;

	struct FDamageHit
	{
		TWeakObjectPtr<AActor> Attacker;
		FVector Source = FVector::ZeroVector;	// where the hit came from (world), fixed at hit time
		double RealTime = 0.0;
		float Strength01 = 0.f;
		bool bShieldOnly = true;
	};
	TArray<FDamageHit> DamageHits;		// newest first
	void ClearDamageIndicators() { DamageHits.Reset(); }
	FText ObjectiveText;
	mutable double ObjectiveRefreshAt = -1.0;
	int32 SwitchCount = 0;

	UPROPERTY(Transient)
	FTDSwitchCut LastSwitchCut;

	/** m14: one buffered switch request while the cooldown runs (0 = none, +/-1 = cycle, 2 = direct). */
	int32 PendingSwitchKind = 0;
	ETDShipId PendingSwitchShip = ETDShipId::Rift;
	double PendingSwitchExpire = -1000.0;
	/** True when a game flow took the defeat; it owns the restart, so timers stop. */
	bool bWipeHandledByFlow = false;
};

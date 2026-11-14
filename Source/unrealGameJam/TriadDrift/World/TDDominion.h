// Triad Drift - World (Teammate 4). ENG-18 Dominion mothership: assault + core space finale.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Enemies/TDEnemyTypes.h"
#include "TriadDrift/Waves/TDWaveTypes.h"
#include "TDDominion.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UTDDominionVisuals;
class UTDEnemyVisuals;
class ATDShieldGenerator;
class ATDDominionReactor;
class ATDEnemyTurret;
class ATDEnemyShip;
class ATDDominion;
class ATDDockingBay;
class ATDDistortionField;

/** ENG-29: which ending the generators' fall leads to. */
UENUM(BlueprintType)
enum class ETDBoardingMode : uint8
{
	/** Boarding when td.Finale.Boarding is 1 (default), the map is a real level (not a /Temp test world) and
	 *  L_Dominion_Interior exists; otherwise the space reactor kill. */
	Auto,
	ForceOn,	// always board (tests)
	ForceOff	// always the space reactor kill
};

UENUM(BlueprintType)
enum class ETDDominionState : uint8
{
	Hidden,			// not revealed yet (hull hidden, no collision)
	WarpingIn,		// Nick's reveal plays; nothing fights yet
	ShieldsUp,		// generators + turrets + launches; hull and reactor immune
	ReactorExposed,	// all generators down: fly the trench, kill the reactor
	DockingOpen,	// boarding finale (ENG-29): all generators down, the docking bay is open, the reactor stays shielded
	EscapeFlight,	// ENG-31: back from the interior (Phase Escape): the hull burns, debris falls, fly clear in time
	Breakup,		// reactor dead; breakup sequence plays, then Victory
	Finished		// TravelTo(Victory) requested
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDDominionStateSignature, ATDDominion*, Dominion, ETDDominionState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDDominionGeneratorSignature, ATDDominion*, Dominion, int32, GeneratorsRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDDominionPhaseSignature, ATDDominion*, Dominion, int32, ReactorPhase);

/** Per reactor phase escalation (index 0 = shields-up stage, 1..3 = reactor phases). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDDominionStage
{
	GENERATED_BODY()

	/** Seconds between fighter launches. 0 = no launches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dominion", meta = (ClampMin = "0"))
	float LaunchInterval = 15.f;

	/** Turret pause between bursts (lower = more fire). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dominion", meta = (ClampMin = "0"))
	float TurretBurstCooldown = 2.5f;

	/** Turret charge-shot telegraph (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dominion", meta = (ClampMin = "0"))
	float TurretTelegraph = 1.2f;
};

/**
 * The Dominion: a huge placeholder hull (engine basic shapes) that warps in once the Dominion is
 * revealed (ITDGameFlow progress, set by the wave director after wave 5). While its main shield is
 * up nothing can hurt the hull or the reactor; four ATDShieldGenerators on pylons outside the
 * shield, each guarded by turrets, must die first. Each generator kill drops a shield section and is
 * a GeneratorKill highlight. All four down -> SetDominionState(true, true) and the reactor trench
 * opens. The reactor has a 3-phase health bar with escalating turret fire and fighter launches.
 * Reactor kill -> ReactorKill highlight, OnBreakupStarted (Nick's sequence), then after
 * BreakupSeconds Flow->SetPhase(Victory) (a phase on L_Galaxy; there is no victory map).
 *
 * Stretch (ENG-29): set bDockingFinale and override BeginDockingFinale() in a subclass; the core
 * reactor path is then skipped. Not built.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDDominion : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDDominion();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion", meta = (WorldContext = "WorldContext"))
	static ATDDominion* Get(const UObject* WorldContext);

	/** Start the arrival now (normally automatic from progress). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Dominion")
	void WarpIn();

	/** Re-read progress: warp in if revealed, skip straight to the reactor if defenses are down. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Dominion")
	void SyncWithProgress();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	ETDDominionState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	int32 GetGeneratorsRemaining() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	bool IsMainShieldUp() const { return State == ETDDominionState::Hidden || State == ETDDominionState::WarpingIn || State == ETDDominionState::ShieldsUp; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	TArray<ATDShieldGenerator*> GetGenerators() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	TArray<ATDEnemyTurret*> GetTurrets() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	ATDDominionReactor* GetReactor() const { return Reactor; }

	/** Where launched fighters appear (world space). */
	FVector GetHangarLocation() const;

	/**
	 * ENG-33 (T3's intro / showcase shots): show only the hull, bridge, trench walls and hull detail (no generators,
	 * turrets, reactor, shield, emitters or launches), optionally at Override. No state change, no collision.
	 * Refused (returns false) while the assault runs (WarpingIn..Breakup); a real state change ends it.
	 * SetShowcase(false) restores the previous visibility, collision and transform.
	 */
	bool SetShowcase(bool bShow, const FTransform* Override = nullptr);

	bool IsShowcasing() const { return bShowcase; }

	/**
	 * Where a journey stage starts the squad, from the hull's real size (art or placeholder): the approach is front-
	 * right and a little above, 2.5 hull radii out (well outside the hull, the shield and the generator ring),
	 * facing the Dominion; the trench start is 200 m off the actual bow on the trench line, facing aft.
	 */
	FTransform GetStageStart(bool bTrench) const;

	/** ENG-31: where the escape flight starts (TDReturnPoints::DominionHangarExit): just out of the bay mouth, facing out. */
	FTransform GetEscapeStart() const;

	/** ENG-31 escape flight: seconds left, how far out counts as clear (cm from the Dominion's centre), and state. */
	float GetEscapeTimeLeft() const { return State == ETDDominionState::EscapeFlight ? FMath::Max(0.f, EscapeSeconds - StateTime) : 0.f; }
	float GetEscapeExitRadius() const;
	bool IsEscapeClear() const { return bEscapeClear; }
	/** The clock ran out this take (the Dominion went up with the squad; CUT was called). */
	bool HasEscapeFailed() const { return bEscapeFailReported; }
	int32 GetEscapeDebrisSpawned() const { return EscapeDebrisSpawned; }

	/** The hull, bridge and trench walls' extent in the Dominion's local space (art collision included). */
	FBox GetLocalHullBounds() const;

	/** Living fighters launched by the Dominion. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	int32 GetLaunchedAlive() const;

	/** Total launch events so far (tests, stats). */
	int32 GetLaunchCount() const { return LaunchCount; }

	/** Current escalation stage: 0 = shields up, 1..3 = reactor phase. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	int32 GetStage() const { return Stage; }

	/** Debug / cheat: kill every generator. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Dominion")
	void DebugDropShields();

	// --- Tuning ---------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	float WarpInSeconds = 6.f;

	/** The finale locks the hyperdrive only once a squad ship is this close (the galaxy layout sets its gravity-well
	 *  radius here: jumps drop out at the well, then the lock holds for the fight). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	float HyperdriveLockRadius = 60000.f;

	/** Hull material until a hull mesh exists (dark panelled armour). Default: the builder's panel material; missing =
	 *  the placeholder tint (#1a1b1e) with seam lines and eye lights built in code. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSoftObjectPtr<UMaterialInterface> HullMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/Dominion/MI_TD_DominionPanel.MI_TD_DominionPanel")));

	/** The flow's scene for the whole assault (UTDGameInstance::DominionScene); its retakes reset the fight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	FName DominionSceneId = TEXT("Dominion");

	/** ENG-31: the escape flight is its own scene: a fail retakes it at the bay mouth, never the interior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape")
	FName EscapeSceneId = TEXT("Escape");

	/** Seconds to fly clear before the Dominion goes up with the squad (fail = CUT, the next take). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape", meta = (ClampMin = "5"))
	float EscapeSeconds = 60.f;

	/** Clear = this many hull radii from the centre (at least twice the shield radius, and EscapeExitDistance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape", meta = (ClampMin = "1"))
	float EscapeExitHullRadii = 2.5f;

	/**
	 * Clear is at least this far (cm) from the centre: a real run, not a 6 s boost (T7's playtest, Nov 14: 1068 m cleared in
	 * ~6 s). With boost's duty cycle (3 s at 108 m/s, ~3.7 s at 60 m/s) a squad averages ~80 m/s, so 3.5 km from the bay
	 * mouth takes ~40 of the 60 s, under falling debris.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape", meta = (ClampMin = "0"))
	float EscapeExitDistance = 350000.f;

	/** A debris chunk falls across the way out this often (s), faster near the end. 0 = no debris. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape", meta = (ClampMin = "0"))
	float EscapeDebrisInterval = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape", meta = (ClampMin = "0"))
	float EscapeDebrisDamage = 20.f;

	/** After clearing: the final blast, then victory this many seconds later. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Escape", meta = (ClampMin = "0"))
	float EscapeVictoryDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	float BreakupSeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "1"))
	float GeneratorHull = 800.f;

	/** Turrets guarding each generator (2 x 4 = 8) plus ExtraTurrets on the hull: 8-12 total. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	int32 TurretsPerGenerator = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	int32 ExtraTurrets = 2;

	/** Turrets in the reactor trench, active once it is exposed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	int32 TrenchTurrets = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "1"))
	float TurretHull = 150.f;

	/** Turret guns (telegraphed charge shots). TelegraphSeconds comes from the stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	FTDEnemyAttackSpec TurretGuns;

	/** Stage 0 (shields up) then reactor phases 1..3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	TArray<FTDDominionStage> Stages;

	/** Groups launched from the hangar, cycled per launch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	TArray<FTDWaveGroup> LaunchGroups;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	int32 MaxLaunchedAlive = 10;

	/** Hull half-extents (cm). Shapes, generator pylons and the trench are laid out from this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Dominion")
	FVector HullExtent = FVector(30000.f, 8000.f, 5000.f);

	/** Main shield radius (cm); generators sit just outside it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	float ShieldRadius = 36000.f;

	/** Poll progress for the reveal this often while hidden (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0.05"))
	float ProgressPollSeconds = 0.5f;

	/** Pawn class for launches. Null = ATDEnemyShip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	TSubclassOf<ATDEnemyShip> LaunchedShipClass;

	/**
	 * ENG-29: when the generators fall, board (the docking bay opens, BeginDockingFinale) or kill the reactor from
	 * space. `td.Finale.Boarding 0` forces the space ending (our fallback). Decided when the defenses go down.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	ETDBoardingMode BoardingMode = ETDBoardingMode::Auto;

	/** True when the defenses' fall leads to boarding (BoardingMode, the cvar, the map). */
	bool UsesBoardingFinale() const;

	/**
	 * Story Act 1: before the passage opens (bDominionRevealed) the Dominion is seen far off behind its distortion
	 * field (ATDDistortionField, a mirage of the hull): visible from wave 1, unreachable. False = hidden as before.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	bool bShowBehindField = true;

	ATDDistortionField* GetDistortionField() const { return DistortionField; }

	/** The docking bay (null until the boarding finale opens it). */
	ATDDockingBay* GetDockingBay() const { return DockingBay; }
	UStaticMeshComponent* GetHullMesh() const { return HullMesh; }

	/** Hull socket the docking bay and its interior sit on (art hull r2: hull-local (30000, 0, -2000), +X out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	FName BaySocket = TEXT("Bay");

	// --- Presentation (all optional) ------------------------------------------------------------

	/** Turret cues (TurretTelegraph, TurretDestroyed). Empty = convention asset DA_DominionTurret_Visuals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDEnemyVisuals> TurretVisuals;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	bool IsVictoryAccepted() const { return bVictoryAccepted; }

	/** Nick's single presentation asset for the whole mothership (hull, generators, reactor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDDominionVisuals> Visuals;

	// --- Events ---------------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Dominion")
	FTDDominionStateSignature OnStateChanged;

	/** A generator died; a shield section dropped. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Dominion")
	FTDDominionGeneratorSignature OnShieldSectionDown;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Dominion")
	FTDDominionPhaseSignature OnReactorPhaseChanged;

	/** Reactor dead: Nick's breakup sequence starts now; Victory follows after BreakupSeconds. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Dominion")
	FTDDominionStateSignature OnBreakupStarted;

	/** The hull itself is Neutral so it blocks EVERY team's projectiles (trench cover works both ways). Its parts are Enemy. */
	virtual ETDTeam GetTeam() const override { return ETDTeam::Neutral; }
	virtual void Tick(float DeltaSeconds) override;

	/** The hull's true size (the root is a bare scene component). */
	virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const override
	{
		CollisionRadius = HullExtent.GetMax();
		CollisionHalfHeight = HullExtent.Z;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** ENG-29: opens the docking bay on the bow (bPlayCues false when resuming a save or a retake). */
	virtual void BeginDockingFinale(bool bPlayCues = true);

	/** The hull's Bay socket in world space, yaw only (+X out, the Dominion's up). */
	FTransform BaySocketWorld() const;

	UPROPERTY(Transient)
	TObjectPtr<ATDDockingBay> DockingBay;

	/** The structures builder's bay pocket (BP_DockingBay_Interior) on the art hull's Bay socket, with the bay. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> DockingInterior;


	UPROPERTY(Transient)
	TObjectPtr<ATDDistortionField> DistortionField;

	void SetState(ETDDominionState NewState);
	/** Hyperdrive "Finale" lock: assault states AND a squad ship has been inside HyperdriveLockRadius. Idempotent. */
	void SyncHyperdriveLock();
	UFUNCTION()
	void HandleTakeStarted(FName SceneId, int32 TakeNumber);

	UFUNCTION()
	void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);
	/** A new take of the assault scene: see docs/eng/world.md (what stays, what restarts). */
	void ResetForNewTake();
	/** Victory phase reached without the breakup (won save, TD.Stage Victory): gone, silent, unlocked. */
	void FinishAfterVictory();
	void BuildParts();
	void ShowHull(bool bVisible);
	void ApplyStage(int32 NewStage);
	void TickLaunches(float DeltaSeconds);
	void LaunchGroup();
	void OnDefensesDown(bool bFromSave);
	void ExposeReactor(bool bPlayCues = true);
	void LayoutParts();
	virtual void OnConstruction(const FTransform& Transform) override;
	void StartShieldIdle();
	void StopShieldIdle();

	UFUNCTION()
	void HandleGeneratorDestroyed(ATDShieldGenerator* Generator, AActor* Killer);

	UFUNCTION()
	void HandleReactorPhase(ATDDominionReactor* InReactor, int32 NewPhase);

	UFUNCTION()
	void HandleReactorDestroyed(ATDDominionReactor* InReactor, AActor* Killer);

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UStaticMeshComponent> HullMesh;

	/** Placeholder hull detail: red-orange seam lines and white eye lights (hidden under a hull mesh). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<class UInstancedStaticMeshComponent> HullSeams;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<class UInstancedStaticMeshComponent> HullEyes;

	void BuildHullDetail(bool bShow);

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UStaticMeshComponent> BridgeMesh;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UStaticMeshComponent> TrenchWallLeft;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UStaticMeshComponent> TrenchWallRight;

	/** Four visual shield quarters (no collision); one hides per generator kill. */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TArray<TObjectPtr<UStaticMeshComponent>> ShieldSections;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<USceneComponent> HangarPoint;

	/** Placeholder breakup while Nick's BreakupSequence has no VFX: explosion chain, final blast, hull gone. */
	void TickPlaceholderBreakup(float DeltaSeconds);
	/** One burst on the hull (HullSectionExplosion, else the placeholder); T01 grows its size. */
	void PlayHullSectionBurst(float T01, int32 Seed);
	/** The flash that swallows the ship (BreakupFinalBlast, else the placeholder) and hides the hull. */
	void PlayFinalBlast();

	// ENG-31 escape flight
	void BeginEscapeFlight();
	void ResetEscapeTake();
	void TickEscapeFlight(float DeltaSeconds);
	void SpawnEscapeDebris();
	float EscapeBoomTimer = 0.f;
	float EscapeDebrisTimer = 0.f;
	float EscapeClearTime = 0.f;
	int32 EscapeDebrisSpawned = 0;
	bool bEscapeClear = false;
	bool bEscapeFailReported = false;
	bool bEscapeBarkSaid = false;
	// ART-18: the Aftermath light row.
	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> LightRowFX;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> LightRowAnchor;
	bool bLightRowOn = false;
	void HandleAftermathBegins(bool bSkippedTutorial);
	UFUNCTION()
	void HandleHyperdriveForLightRow(ETDHyperdriveState NewState);
public:
	/** ART-18: start the light row at the hull point nearest WorldPoint (the escape line, or the start on a skip). */
	void StartAftermathLightRow(const FVector& WorldPoint);
	void StopAftermathLightRow();
	bool IsAftermathLightRowActive() const;
	/** Where the row sits (hull-local) and faces, for tests. */
	FTransform GetAftermathLightRowTransform() const;
protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> EscapeDebris;
	bool bPlaceholderBreakup = false;
	bool bBreakupFinalBlast = false;
	float BreakupBoomTimer = 0.f;
	int32 BreakupBooms = 0;

public:
	int32 GetBreakupBoomCount() const { return BreakupBooms; }
	bool HasBreakupFinalBlast() const { return bBreakupFinalBlast; }
protected:

	bool bShowcase = false;
	bool bShowcasePrevHidden = true;
	bool bShowcasePrevCollision = false;
	FTransform ShowcasePrevTransform;
	TArray<TPair<TWeakObjectPtr<UPrimitiveComponent>, bool>> ShowcaseComponentVisibility;
	TArray<TPair<TWeakObjectPtr<AActor>, bool>> ShowcaseActorHidden;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDShieldGenerator>> Generators;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDEnemyTurret>> Turrets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDEnemyTurret>> TrenchTurretActors;

	UPROPERTY(Transient)
	TObjectPtr<ATDDominionReactor> Reactor;

	TArray<TWeakObjectPtr<ATDEnemyShip>> Launched;

	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> ShieldIdleFX;

	ETDDominionState State = ETDDominionState::Hidden;
	float StateTime = 0.f;
	float PollTimer = 0.f;
	float LockSyncTimer = 0.f;
	bool bSquadArrived = false;
	float LaunchTimer = 0.f;
	int32 LaunchCursor = 0;
	int32 LaunchCount = 0;
	int32 Stage = 0;
	bool bPartsBuilt = false;
	bool bVictoryAccepted = false;

};

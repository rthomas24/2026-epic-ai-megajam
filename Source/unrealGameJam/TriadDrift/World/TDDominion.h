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

UENUM(BlueprintType)
enum class ETDDominionState : uint8
{
	Hidden,			// not revealed yet (hull hidden, no collision)
	WarpingIn,		// Nick's reveal plays; nothing fights yet
	ShieldsUp,		// generators + turrets + launches; hull and reactor immune
	ReactorExposed,	// all generators down: fly the trench, kill the reactor
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
 * BreakupSeconds Flow->TravelTo(Victory).
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

	/** Stretch ENG-29: when true, defenses down calls BeginDockingFinale() instead of exposing the reactor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion")
	bool bDockingFinale = false;

	// --- Presentation (all optional) ------------------------------------------------------------

	/** Turret cues (TurretTelegraph, TurretDestroyed). Empty = convention asset DA_DominionTurret_Visuals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDEnemyVisuals> TurretVisuals;

	/** Seconds between TravelTo(Victory) attempts until the flow accepts one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0.1"))
	float VictoryRetrySeconds = 1.f;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	bool IsVictoryAccepted() const { return bVictoryAccepted; }

	int32 GetVictoryAttempts() const { return VictoryAttempts; }

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

	virtual ETDTeam GetTeam() const override { return ETDTeam::Enemy; }
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

	/** Stretch hook (ENG-29). Default: logs and falls back to the core reactor ending. */
	virtual void BeginDockingFinale();

	void SetState(ETDDominionState NewState);
	void BuildParts();
	void ShowHull(bool bVisible);
	void ApplyStage(int32 NewStage);
	void TickLaunches(float DeltaSeconds);
	void LaunchGroup();
	void OnDefensesDown(bool bFromSave);
	void ExposeReactor();
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
	float LaunchTimer = 0.f;
	int32 LaunchCursor = 0;
	int32 LaunchCount = 0;
	int32 Stage = 0;
	bool bPartsBuilt = false;
	bool bVictoryAccepted = false;
	float VictoryRetryTimer = 0.f;
	int32 VictoryAttempts = 0;
};

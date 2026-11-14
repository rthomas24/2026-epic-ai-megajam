// Triad Drift - atmosphere entry (galaxy) and atmosphere flight zone (planets), ENG-20. Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDAtmosphereZones.generated.h"

class UBoxComponent;
class USphereComponent;
class ATDShip;
class UTDTravelFX;
class UTDAtmosphereEntryComponent;
class UTDHealthComponent;
struct FTDDamageInfo;

UENUM(BlueprintType)
enum class ETDAtmosphereEntryState : uint8
{
	Idle,
	Entering,  // 2.5 s of heat and shake; the ship is committed
	Travelling, // TravelTo was called
	Holding     // in the atmosphere but a must-hear story sequence is playing (Aftermath): flying freely, commits when it ends
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnAtmosphereEntry, UTDAtmosphereEntryComponent*, Entry);

/**
 * Atmospheric entry on a squad ship (features.md section 7 steps 2-3). T4's galaxy ATDPlanetAtmosphere
 * detects the player ship entering and calls BeginEntry. The ship is then committed: speed is held,
 * steering is reduced, hyperdrive is shut off and the AtmosphericEntry cue plays. After EntrySeconds,
 * ITDGameFlow::TravelTo(Destination, ReturnPoint) is called. If there's no flow, or travel fails,
 * control is handed back.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDAtmosphereEntryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDAtmosphereEntryComponent();

	/**
	 * Start the entry. Returns false if one is already running or the owner isn't a spacecraft.
	 * PlanetSurfaceRadius (the solid planet body, 0 = unknown) keeps the committed dive from reaching
	 * the surface before the level change; the ship is also invulnerable while committed.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Atmosphere")
	bool BeginEntry(ETDDestination InDestination, FName InReturnPoint, FVector PlanetCenter, float PlanetSurfaceRadius = 0.f);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	ETDAtmosphereEntryState GetEntryState() const { return EntryState; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	bool IsEntering() const { return EntryState == ETDAtmosphereEntryState::Entering || EntryState == ETDAtmosphereEntryState::Travelling; }

	/**
	 * T3's live walk (the Lead, Nov 13): a player boosting toward Verdance right after the tutorial escape entered the
	 * atmosphere during the Aftermath sequence and the level change cut Command's orders. While one of these story
	 * sequences plays (UTDStorySubsystem::IsSequencePlaying), entry does not commit: the ship keeps flying (no wall), the
	 * HUD's travel line says "AWAITING ORDERS", and entry commits the moment the sequence ends if the ship is still in
	 * the atmosphere (it re-arms on the next pass otherwise).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	TArray<FName> HoldEntryForSequences = { TEXT("Aftermath") };

	/** The ship is in the atmosphere, waiting for a must-hear sequence to end before it commits. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	bool IsHoldingForStory() const { return EntryState == ETDAtmosphereEntryState::Holding; }

	/** A must-hear sequence is playing now (the hold applies). */
	bool IsStoryHoldingEntry() const;

	/** While held the ship crawls at this speed (cm/s; steerable) so it stops at the atmosphere's edge, not inside the planet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0", Units = "cm/s"))
	float StoryHoldSpeed = 1500.f;
	static const FName StoryHoldOverride; // "StoryHold"
	void EndStoryHold();

	/** How far past the hold point (further from the planet) the ship may stray and still commit when the hold ends (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0", Units = "cm"))
	float HoldLeaveMargin = 50000.f;

	/** 0..1 through the entry (heat / shake intensity for presentation). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	float GetEntry01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	ETDDestination GetDestination() const { return Destination; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float EntrySeconds = 2.5f;

	/** ENG-38: on the covered route, the white cover ramps in over the last this-many seconds of the dive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0.05", Units = "s"))
	float CoverRampInSeconds = 0.6f;

	/** Speed held during entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float EntrySpeed = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0", ClampMax = "1"))
	float EntrySteeringScale = 0.35f;

	/** The dive stops this far above the planet surface at most (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float EntrySurfaceClearance = 3000.f;

	/** Slowest the committed dive goes, even when very close to the surface (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float EntryMinSpeed = 1500.f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Atmosphere")
	FTDOnAtmosphereEntry OnEntryStarted;

	/** TravelTo was called (fires in worlds without a game flow too). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Atmosphere")
	FTDOnAtmosphereEntry OnEntryCompleted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	TObjectPtr<UTDTravelFX> TravelFX;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Speed-override owner key used on the movement component. */
	static const FName OverrideOwner;

	/** Abandon an entry in progress (death, retake): back to normal flight, no travel. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Atmosphere")
	void AbortEntry();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOwnerDeath(UTDHealthComponent* HealthComp, const FTDDamageInfo& KillingBlow);

private:
	void FinishEntry();
	void RestoreFlight();
	FDelegateHandle SceneRestartHandle;

	TWeakObjectPtr<class UNiagaraComponent> EntryFX;
	TWeakObjectPtr<class UAudioComponent> EntryAudio;
	void StopEntryCue();

	bool bCoveredEntry = false; // ENG-38: this entry runs the white cover and the persistent roar
	ETDAtmosphereEntryState EntryState = ETDAtmosphereEntryState::Idle;
	ETDDestination Destination = ETDDestination::Verdance;
	FName ReturnPoint = NAME_None;
	float EntryTime = 0.f;
	// The held entry's arguments (committed with them when the sequence ends).
	FVector HeldPlanetCenter = FVector::ZeroVector;
	float HeldSurfaceRadius = 0.f;
	float HeldStartDistance = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnAtmosphereExit, ATDShip*, Ship);

/**
 * Planet side (features.md §7 steps 4–5 and 10). Every squad ship inside the box flies in
 * atmosphere mode: gravity, speed capped at 60% of space cruise, no hyperdrive, Land on F/A. The
 * box's walls and ceiling are soft boundaries. Once the planet's objectives are done (or
 * SetExitAllowed is called), flying out through the ceiling calls
 * TravelTo(Galaxy, "<PlanetId>_Orbit").
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDAtmosphereFlightZone : public AActor
{
	GENERATED_BODY()

public:
	ATDAtmosphereFlightZone();

	/** "Verdance" / "Cinder": used for the exit return point "<PlanetId>_Orbit". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	FName PlanetId = TEXT("Verdance");

	/** Exit is allowed once all of these are in FTDProgress::CompletedObjectives (empty = always). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	TArray<FName> RequiredObjectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float Gravity = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0.05", ClampMax = "1"))
	float SpeedCapFraction = 0.6f;

	/** How far above the ceiling the player must climb to leave once the exit is open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float ExitHeightAboveCeiling = 3000.f;

	/** Tip shown (once per approach) when the player hits the ceiling before the objectives are done. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	FName CeilingBlockedTip = TEXT("Tip_FinishObjectivesFirst");

	/** Force the exit open or closed regardless of objectives (scripted sequences, cheats). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Atmosphere")
	void SetExitAllowed(bool bAllowed) { bExitOverride = true; bExitAllowedOverride = bAllowed; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	bool IsExitAllowed() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	FBox GetZoneBox() const;

	/** Half-size of the zone box (cm). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Atmosphere")
	void SetZoneExtent(const FVector& Extent);

	/** The ceiling exit fired (TravelTo(Galaxy, "<PlanetId>_Orbit") was requested). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	bool IsExitTriggered() const { return bExitTriggered; }
	/** ENG-38: the forced climb out before the travel is running. */
	bool IsClimbingOut() const { return ExitClimbTime >= 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	bool IsShipInZone(const ATDShip* Ship) const;

	/**
	 * ENG-34: a planet with T4's generated terrain is explorable, so the zone grows (never shrinks) to the terrain's
	 * whole collision core plus TerrainEdgeExtension (so the edge warning only starts at the edge of the 16 km world),
	 * and its ceiling to TerrainCeilingClearance above the highest ground in it, so no mountain top sits above the
	 * ceiling (or in the exit). Found automatically within a second. Past the collision core (T4's far ring is
	 * visual only) the zone keeps ships above the generated ground itself.
	 */
	void FitToTerrain(const class ATDPlanetTerrain* Terrain);
	bool IsFittedToTerrain() const { return bFittedToTerrain; }

	/** ENG-38 climb out: seconds of forced climb (exit burn, hull heat, cover at the end) before the travel; 0 = at once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0", Units = "s"))
	float ExitClimbSeconds = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float ExitClimbSpeed = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float TerrainEdgeExtension = 50000.f;

	/**
	 * Ryan's one dressed zone (Nov 13, WORLD-DENSITY-PLAN 2.0 / 3.6): the fitted zone is DressedHalfSize around the pad
	 * (turn-back at 5 km), its walls warn DressedWallWarnDistance inside that (4 km), and its ceiling sits
	 * DressedCeilingAbovePad over the pad (never under the highest ground in it + TerrainCeilingClearance). Off: the whole core, as before.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	bool bDressedZone = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "10000"))
	float DressedHalfSize = 500000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float DressedWallWarnDistance = 100000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "10000"))
	float DressedCeilingAbovePad = 160000.f;
	/** Seconds between two wingman "turn back" lines at the edge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float EdgeBarkInterval = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float TerrainCeilingClearance = 40000.f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Atmosphere")
	FTDOnAtmosphereExit OnExitTriggered;

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Atmosphere")
	TObjectPtr<UBoxComponent> ZoneBox;

private:
	void ApplyToShip(ATDShip* Ship, bool bExitOpen);

	float LastEdgeBarkTime = -1000.f;
	bool bWasNearEdge = false;
	bool bFittedToTerrain = false;
	TWeakObjectPtr<const class ATDPlanetTerrain> FittedTerrain;
	float TerrainCoreHalf = 0.f;
	/** The rig's original ceiling height above the zone floor (cm), kept when the zone grows over the terrain:
	 *  once the exit is open the climb-out triggers this high above the ground under the ship (review m6). */
	float ExitAltitudeAboveGround = -1.f;
	void KeepAboveFarGround(class ATDShip* Ship) const;
	float TerrainPoll = 0.f;
	bool bExitOverride = false;
	bool bExitAllowedOverride = false;
	float ExitClimbTime = -1.f;
	bool bBreakawayPlayed = false;
	void PlayBreakaway(ATDShip* Ship);
	TWeakObjectPtr<ATDShip> ExitClimbShip;
	bool bExitTriggered = false;
	bool bTipShown = false;
	TArray<TWeakObjectPtr<ATDShip>> ManagedShips;
};

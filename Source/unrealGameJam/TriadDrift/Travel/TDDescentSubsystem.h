// Triad Drift - planetary descent (ENG-38 stage 1). Owner: Teammate 1.
//
// Game-instance level, so it outlives the galaxy world and is there on the planet's first frame:
//  - the timing log around a planet entry (commit -> OpenLevel -> post-load -> first frame -> terrain built -> control);
//  - the white cover: a Slate widget on the game viewport (not a UMG widget, which is torn down with the world);
//  - the entry roar: a 2D voice flagged to persist across the level change.
// Stage 1 step 1 uses it to measure the hold and to prove the cover and the roar survive OpenLevel (TD.Descent.Probe).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/World/TDPlanetEntry.h"
#include "TDDescentSubsystem.generated.h"

class SWidget;
class STDDescentCover;
class UAudioComponent;
class USoundBase;
class ATDPlanetAtmosphere;

/** ENG-38: the squad as it left one map for the other (galaxy <-> planet): who was flown, and every ship's hull and shield. */
struct FTDSquadCarry
{
	bool bValid = false;
	ETDShipId Active = ETDShipId::Rift;
	TMap<ETDShipId, float> Hull01;
	TMap<ETDShipId, float> Shield01;
};

/** The cover starts to lift on the destination (planet or galaxy): music, HUD and story can start their arrival beat. */
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnDescentRevealed, ETDDestination /*Destination*/);

/** Cover phases of a covered (white) travel. */
enum class ETDDescentCover : uint8 { Off, RampIn, Held, Revealing, Aborting };

UCLASS()
class UNREALGAMEJAM_API UTDDescentSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UTDDescentSubsystem* Get(const UObject* WorldContext);
	/** ENG-38 HUD (T2): the hull heat now, 0..1 (the entry burn and the climb out; 0 outside them). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Descent", meta = (WorldContext = "WorldContext"))
	static float GetEntryHeat01(const UObject* WorldContext) { const UTDDescentSubsystem* D = Get(WorldContext); return D ? FMath::Clamp(D->Heat, 0.f, 1.f) : 0.f; }

	// ---- Timing log (one line each: "TD.Descent.Timing <step> +X.XXX s").
	/** The entry committed: t = 0 for every later step until control (or 40 s). */
	void BeginTiming(const TCHAR* What);
	void Mark(const TCHAR* Step);
	bool IsTiming() const { return TimingStart >= 0.0; }

	// ---- White cover (Slate, on the game viewport).
	void ShowCover(float Alpha, FLinearColor Color = FLinearColor::White);
	void HideCover();
	bool IsCoverShown() const { return Cover.IsValid(); }
	/** Frames the cover has painted (all time). */
	static int32 GetCoverPaints();

	// ---- Roar (2D, persists across the level change).
	void StartRoar(USoundBase* Sound);
	/** The roar for a leg (the Lead, Nov 14): entry = DescentRoar, else AtmosphericEntry's sound; climb out = AtmosphericExit's
	 *  sound, else the entry roar, so both legs always have sound. Loads it; null when nothing is assigned. */
	static USoundBase* ResolveRoar(const class UTDTravelFX* FX, bool bExit);
	void StopRoar(float FadeSeconds = 0.5f);
	bool IsRoarPlaying() const;

	// ---- Covered route (ENG-38): galaxy <-> planet with the white cover instead of the black fade and L_Transition.
	/** td.Descent.Legacy 0 (default) and a galaxy <-> planet trip. Every other route keeps today's fade. */
	static bool IsCoveredRoute(ETDDestination From, ETDDestination To);
	static bool IsLegacy();
	/** The take-off climb out (ENG-38): the next covered travel waits this long and the climb drives the cover itself. */
	void SetClimbHold(float Seconds) { ClimbHoldSeconds = FMath::Max(Seconds, 0.f); }
	/** The flow's TravelTo on a covered route: brings the cover to full white; returns the seconds until it is (0 = now). */
	float BeginCoveredTravel(ETDDestination To);
	/** The atmosphere entry ramps the cover with its plasma (0..1, never down while a travel is pending). */
	void SetEntryCover(float Alpha01);
	/** The travel fell through (voided, timed out, refused) or the entry aborted: cover and roar go. */
	void AbortCover(const TCHAR* Why);
	ETDDescentCover GetCoverPhase() const { return CoverPhase; }
	/** Fires once per covered travel when the cover starts to lift (T7's Verdance theme, music, HUD). */
	FTDOnDescentRevealed OnRevealed;
	float GetCoverAlpha() const { return CoverAlpha; }

	// ---- Arrival from the entry point (ENG-38 stage 1).
	/** At the entry commit: where on the globe and which way the squad went in. */
	void RecordEntry(ETDDestination Planet, const ATDPlanetAtmosphere* Atmosphere, const FVector& WorldPoint, const FVector& WorldVelocity);
	/** The same, already in globe terms (tests). */
	void RecordEntryGlobe(ETDDestination Planet, const FTDGlobePoint& P, const FVector& GlobeVelocity);
	bool HasPendingArrival() const { return bArrivalPending; }
	void ClearArrival() { bArrivalPending = false; ArrivalWorld.Reset(); }
	/**
	 * The planet rig's fresh-arrival transform: from the recorded entry (consumed by the first world that asks, kept
	 * for that world's retakes), or false (no entry recorded: a Continue, a dev jump) for today's fixed descent start.
	 */
	bool GetArrival(UWorld* World, ETDDestination Planet, const FTransform& CourseToWorld, FTransform& OutWorld);
	/** Arrival height above the ground under it. 1,000 m: under the 1.2 km take-off exit height and near the fitted
	 *  zone ceiling (highest ground + 400 m) until stage 2's glide owns the altitude. */
	static constexpr float ArrivalAltitudeCm = 100000.f;
	/** Arrivals sit at least this far under the fitted zone ceiling (its warning band is 500 m). */
	static constexpr float CeilingClearanceCm = 65000.f;

	// ---- Squad carried through the travel (ENG-38; the Lead, Nov 13: the flown ship is the core mechanic).
	/** At the entry commit and the take-off exit: the world's squad (its manager) as it leaves. */
	void RecordSquad(UWorld* World);
	void RecordSquadForTests(const FTDSquadCarry& Carry) { SquadCarry = Carry; SquadCarry.bValid = true; SquadCarryWorld.Reset(); }
	/**
	 * T3's ATDSquadManager::SpawnSquad asks this. The first spawn in a new world gets the flown ship and the hulls
	 * (bOutFirstSpawnInWorld); later spawns in that world (retakes) get the flown ship only. False: no record (a
	 * Continue, a dev jump): spawn as before.
	 */
	bool ConsumeSquadCarry(UWorld* World, FTDSquadCarry& Out, bool& bOutFirstSpawnInWorld);
	void ClearSquadCarry() { SquadCarry = FTDSquadCarry(); SquadCarryWorld.Reset(); }

	// ---- Hull heat (ENG-38): M_TD_Ship's HeatGlow 0..1 on every squad ship (player and wingmen), on dynamic instances
	// made only for materials that expose the parameter. Rises with the dive, holds under the cover, decays over
	// HeatDecaySeconds after the reveal, cleared at once on an abort. Applied to whatever squad ships the current world
	// has, so a switch mid-entry or the new squad on the planet carries the same value.
	/** The entry's dive progress (0..1); never goes down while entering. */
	void SetEntryHeat(float Heat01);
	/** Abort: back to 0 now. */
	void ClearHeat();
	float GetHeat() const { return Heat; }
	static const FName HeatGlowParam; // "HeatGlow"
	static constexpr float HeatDecaySeconds = 2.f;
	/** Sets HeatGlow on one ship's materials that expose it (makes dynamic instances on first use). Returns slots set. */
	static int32 ApplyHeatToShip(class ATDShip* Ship, float Heat) { return SetShipMaterialScalar(Ship, HeatGlowParam, Heat); }
	/**
	 * Any material scalar on a ship (HeatGlow, Rift's PortalPower for T2): only on the materials that expose it, with
	 * a dynamic instance made on first non-zero use. Returns the number of slots set.
	 */
	static int32 SetShipMaterialScalar(class ATDShip* Ship, FName Param, float Value);

	// ---- The burn (ENG-38): the entry / exit Niagara loop on every squad ship, its User.Intensity = the heat curve.
	/** The climb out (exit) instead of the dive: the burn uses UTDTravelFX::AtmosphericExit. */
	void SetBurnIsExit(bool bExit) { if (bBurnExit != bExit) { bBurnExit = bExit; StopBurnFX(); } }
	int32 GetBurnFXCount() const;
	static const FName IntensityParam; // "User.Intensity"
	void StopBurnFX();

	// ---- The Director's Cut shot of the burn (ENG-38; the Lead, Nov 13): T3's director camera ahead of, beside and below
	// the ship, looking back at it in three-quarter front profile (the bow shock side-on, the planet's limb and black space
	// behind), a slow push-in; the cut back to the chase camera happens under the full white. td.Descent.DirectorShot 0
	// keeps the chase camera. Reduced motion: no push-in.
	/** Starts the shot for this ship (entry or climb out); false when the director camera is off, there's no player, or the cvar. */
	bool BeginBurnShot(class ATDShip* Ship, bool bExit, float Seconds);
	/** Ends it (a hard cut back to the pawn), if it runs. */
	void EndBurnShot();
	/** The camera, in the ship's own frame, Alpha 0..1 through the shot (also for tests and the effects builder). */
	static FTransform BurnShotCamera(const class ATDShip* Ship, bool bExit, float Alpha, bool bReducedMotion);
	static bool IsDirectorShotEnabled();

	// ---- ENG-38 stage 2, the glide-lite (the Lead, Nov 14): the arrival sits GlideAboveDeckCm over the cloud deck; when the
	// cover lifts, a forced glide (GlideSeconds at GlideSpeed, steering GlideSteering, invulnerable, the edge warning silent)
	// carries the flown ship down through the deck to GlideEndAboveGroundCm over the ground, levels off and hands back over
	// GlideHandBackSeconds. The load hides behind the white, then the cloud. td.Descent.Glide 0 = today's arrival.
	static constexpr float GlideAboveDeckCm = 30000.f;
	static constexpr float GlideEndAboveGroundCm = 90000.f;
	static constexpr float GlideSeconds = 9.f;
	static constexpr float GlideSpeed = 12000.f;
	static constexpr float GlideSteering = 0.5f;
	static constexpr float GlideHandBackSeconds = 1.f;
	static const FName GlideOverride; // "DescentGlide" (a sequence owner: the edge warning stays silent)
	static bool IsGlideEnabled();
	/** Starts the glide for this ship now. GroundZFallback is used where there is no planet terrain (tests). */
	bool BeginGlide(class ATDShip* Ship, float GroundZFallback = 0.f);
	/** Ends it at once (level change, cover abort, retake): the ship is handed back. */
	void EndGlide(const TCHAR* Why);
	bool IsGliding() const { return GlideShip.IsValid() && GlideTime >= 0.f; }
	/** -1 when no glide runs; else 0 at the arrival (deck + 300 m) to 1 at the glide end, by altitude (T4's fog). */
	static float GetDescent01(const UObject* WorldContext);
	/** The deck Z the current glide used (world cm), 0 when none. */
	float GetDescentDeckZ() const { return GlideDeckZ; }
	/** The cloud deck's world Z in a world (0 = no deck). */
	static float FindDeckZ(UWorld* World);
	void TickGlide(float Dt);
	/** The glide's pass through the deck: the CloudWisps layer on the camera while within this of the deck (cm). */
	static constexpr float GlideDeckPassBand = 4000.f;
	/** ENG-38 exposure ramp: the DescentWhitePP blendable's weight now (0 = not applied). */
	float GetExposureRampWeight() const { return ExposureWeight; }
	void TickExposureRamp();

	// ---- ENG-38 stage 2, the mirrored take-off (the Lead, Nov 14): where the squad leaves the planet's ground decides where
	// it comes out on the galaxy globe (T4's FTDPlanetEntry::GlobeFromDeparture / WorldFromGlobe), not a fixed <Planet>_Orbit.
	/** The climb out: the flown ship's course-local (pad frame) departure. */
	void RecordDeparture(ETDDestination Planet, const FTransform& CourseLocalDeparture);
	bool HasPendingDeparture() const { return bDepartPending; }
	/** Places the squad outside that planet's atmosphere at the departure's globe point, heading out; false until it can. */
	bool PlaceDeparture(UWorld* World);
	/** Distance from the planet's centre the squad comes out at, in atmosphere radii (outside it: no re-entry). */
	static constexpr float DepartureRadiusScale = 1.3f;

	// ---- Tuning
	static constexpr float CoverRampSeconds = 0.35f;   // a travel with no entry ramp in front of it (take-off, dev)
	static constexpr float RevealMinHoldSeconds = 0.5f; // after the destination's first frame
	static constexpr float RevealMaxHoldSeconds = 4.f;  // after the first frame, terrain or not
	static constexpr float RevealSeconds = 1.2f;
	static constexpr float CoverHardLimitSeconds = 12.f; // since full white: never stuck

	/** Probe (TD.Descent.Probe): when the run reaches control, log what survived and how long it took, then clean up. */
	void ArmProbe() { bProbe = true; }

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTDDescentSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual bool IsTickableWhenPaused() const override { return true; }

private:
	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* World);
	void EndTiming(const TCHAR* Why);
	void TickCover(float RealDt);
	void TickHeat(float RealDt);
	/** The cloud deck (ATDCloudDeck): CloudPunch when the flown ship crosses it, CloudWisps while within DeckBand of it. */
	void TickCloudDeck();
public:
	static constexpr float CloudDeckBand = 15000.f;
	int32 GetCloudPunches() const { return CloudPunches; }
	bool AreCloudWispsOn() const { return CloudWisps.IsValid(); }
private:
	int32 CloudPunches = 0;
	float LastShipDeckSide = 0.f; // sign of (ship Z - deck Z) last frame, 0 = unknown
	TWeakObjectPtr<class AActor> LastDeckShip;
	TWeakObjectPtr<class UNiagaraComponent> CloudWisps;
	void SetCoverAlpha(float Alpha);

	double TimingStart = -1.0;
	double LastMark = 0.0;
	double LastTickTime = 0.0;
	double LongestFrame = 0.0;
	bool bWaitFirstFrame = false;
	bool bWaitTerrain = false;
	bool bWaitControl = false;
	bool bProbe = false;
	uint64 PostLoadFrame = 0;
	int32 PaintsAtPostLoad = -1;
	int32 PaintsAtFirstFrame = -1;
	int32 RoarAtFirstFrame = -1;
	double ControlTime = -1.0;
	double FirstFrameTime = -1.0;
	double TerrainTime = -1.0;
	double PostLoadTime = -1.0;
	double OpenLevelTime = -1.0;
	TWeakObjectPtr<UWorld> LoadedWorld;

	FTDSquadCarry SquadCarry;
	TWeakObjectPtr<UWorld> SquadCarryWorld; // the world that consumed it (its retakes keep the flown ship)

	float Heat = 0.f;
	float AppliedHeat = -1.f;
	float HeatFedLeft = 0.f;
	bool bBurnExit = false;
	bool bBurnShot = false;
public:
	bool IsBurnShotRunning() const { return bBurnShot; }
private:
	float ClimbHoldSeconds = 0.f;
	bool bHUDHeld = false;
	TMap<TWeakObjectPtr<class ATDShip>, TWeakObjectPtr<class UNiagaraComponent>> BurnFX;
	TWeakObjectPtr<UWorld> HeatWorld;

	ETDDescentCover CoverPhase = ETDDescentCover::Off;
	float CoverAlpha = 0.f;
	float CoverRampRate = 0.f;
	double CoverFullAt = -1.0;
	double DestFirstFrameAt = -1.0;
	uint64 DestPostLoadFrame = 0;
	bool bDestLoaded = false;
	bool bDestFirstFrameSeen = false;
	ETDDestination CoverDestination = ETDDestination::Galaxy;
	TWeakObjectPtr<UWorld> DestWorld;
	bool bFirstViewBuilt = false;
	bool bArrivalSettled = false;
	bool bDepartPending = false;
	ETDDestination DepartPlanet = ETDDestination::Verdance;
	FTDGlobePoint DepartPoint;
	FVector DepartGlobeDir = FVector::UpVector;
	TWeakObjectPtr<class ATDShip> GlideShip;
	float GlideTime = -1.f;
	float GlideStartZ = 0.f;
	float GlideEndZ = 0.f;
	float GlideDeckZ = 0.f;
	float GlideGroundFallback = 0.f;
	float GlideProgress = -1.f;
	bool bGlideLevelling = false;
	float ExposureWeight = 0.f;
public:
	int32 GlideDeckPasses = 0; // tests and the timing log
	float GetCloudIntensity() const { return CloudIntensity; }
	float GetGlideWindVolume() const { return GlideWindVolume; }
	bool bInGlideDeckPass = false;
	/** The in-cloud layer's User.Intensity now (0 outside the deck's band). */
	float CloudIntensity = 0.f;
	/** The glide's wind loop (DA_Travel_FX.GlideWind): its volume now (0 when not gliding). */
	float GlideWindVolume = 0.f;
private:
	TWeakObjectPtr<class UCameraComponent> ExposureCamera;
	UPROPERTY(Transient) TObjectPtr<class UAudioComponent> GlideWind;
	void TickGlideWind(float Dt);
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> ExposureMID;
	float GlideLevelTime = 0.f;

	bool bArrivalPending = false;
	ETDDestination ArrivalPlanet = ETDDestination::Verdance;
	FTDGlobePoint ArrivalPoint;
	FVector ArrivalGlobeVelocity = FVector::ZeroVector;
	TWeakObjectPtr<UWorld> ArrivalWorld;
	FTransform ArrivalWorldXf;
	FVector ArrivalCourseLocal = FVector::ZeroVector;

	TSharedPtr<STDDescentCover> Cover;
	TWeakPtr<class SWidget> CoverInViewport;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Roar;
	FDelegateHandle PreLoadHandle;
	FDelegateHandle PostLoadHandle;
};

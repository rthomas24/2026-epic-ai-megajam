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
	void StopRoar(float FadeSeconds = 0.5f);
	bool IsRoarPlaying() const;

	// ---- Covered route (ENG-38): galaxy <-> planet with the white cover instead of the black fade and L_Transition.
	/** td.Descent.Legacy 0 (default) and a galaxy <-> planet trip. Every other route keeps today's fade. */
	static bool IsCoveredRoute(ETDDestination From, ETDDestination To);
	static bool IsLegacy();
	/** The flow's TravelTo on a covered route: brings the cover to full white; returns the seconds until it is (0 = now). */
	float BeginCoveredTravel(ETDDestination To);
	/** The atmosphere entry ramps the cover with its plasma (0..1, never down while a travel is pending). */
	void SetEntryCover(float Alpha01);
	/** The travel fell through (voided, timed out, refused) or the entry aborted: cover and roar go. */
	void AbortCover(const TCHAR* Why);
	ETDDescentCover GetCoverPhase() const { return CoverPhase; }
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
	/** Arrivals stay within 6 km of the pad, inside the zone's edge-warning band (T4's ring reaches 7 km). */
	static constexpr float MaxArrivalRadiusCm = 600000.f;
	/** Arrivals sit at least this far under the fitted zone ceiling (its warning band is 500 m). */
	static constexpr float CeilingClearanceCm = 65000.f;

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
	static int32 ApplyHeatToShip(class ATDShip* Ship, float Heat);

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

	float Heat = 0.f;
	float AppliedHeat = -1.f;
	float HeatFedLeft = 0.f;
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

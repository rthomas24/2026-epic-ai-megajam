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
#include "TDDescentSubsystem.generated.h"

class SWidget;
class STDDescentCover;
class UAudioComponent;
class USoundBase;

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

	TSharedPtr<STDDescentCover> Cover;
	TWeakPtr<class SWidget> CoverInViewport;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Roar;
	FDelegateHandle PreLoadHandle;
	FDelegateHandle PostLoadHandle;
};

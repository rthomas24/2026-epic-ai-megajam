// Copyright (c) 2026 Triad Drift team. Epic AI MegaJam entry.
// T7 perf (#89, Ryan: "zero lag"). First-use hitches: the first explosion, hit, power, crew spawn or score of a session
// used to load its effect, sound, mesh and materials on the game thread at the moment it was needed, then pay the
// Niagara system's first activation and its render pipelines (PSOs) in that same frame. This world subsystem does all of
// that at map start instead, behind the intro / loading screen: it streams every presentation and fallback data asset
// and everything they point at in the background, holds them for the session, then activates each effect once, a few
// per frame, far below the playfield, so its first real use is already warm (and its pooled component exists).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "TDWarmupSubsystem.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
struct FStreamableHandle;

UENUM()
enum class ETDWarmupStage : uint8
{
	Idle,
	Waiting,         // a moment after the map starts, so the map's own start-up loads go first
	LoadingData,     // the data assets themselves
	LoadingContent,  // what they point at (effects, sounds, meshes, materials, animations)
	Activating,      // each effect once, a few per frame
	Done
};

/**
 * Keeps what the warm-up loaded in memory for the whole run (T6, #89). The world subsystem below is rebuilt on every level
 * load and released its assets with the level, so every return to the galaxy streamed the same 240 assets again (3.4 s
 * if blocked on, or nine flushes in the first seconds of play if not). Held here, the next warm-up finds them resident.
 */
UCLASS()
class UNREALGAMEJAM_API UTDWarmupKeep : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Keep(const TArray<TObjectPtr<UObject>>& Assets);
	int32 GetKeptCount() const { return Kept.Num(); }
	void Reset() { Kept.Reset(); }

private:
	UPROPERTY(Transient) TSet<TObjectPtr<UObject>> Kept;
};

UCLASS()
class UNREALGAMEJAM_API UTDWarmupSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDWarmupSubsystem* Get(const UObject* WorldContextObject);

	/** True until what the data assets point at is in memory (waiting, loading the data, loading the content). */
	bool IsLoading() const { return Stage == ETDWarmupStage::Waiting || Stage == ETDWarmupStage::LoadingData || Stage == ETDWarmupStage::LoadingContent; }
	/**
	 * Under a full-screen cover (the white of a planet climb-out, T6 for #89): start now and block on the async loads,
	 * one flush per call, so the stream is finished before the player sees the level. Left to run in the open, the
	 * first enemy to warp in forced nine FlushAsyncLoading calls on 445 pending packages in the first 6 s (239 ms worst).
	 */
	void HurryUnderCover();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return Stage == ETDWarmupStage::Waiting || Stage == ETDWarmupStage::Activating; }

	ETDWarmupStage GetStage() const { return Stage; }
	int32 GetHeldCount() const { return Held.Num(); }
	int32 GetWarmedEffectCount() const { return WarmedEffects; }
	int32 GetContentPathCount() const { return ContentPaths.Num(); }

	/** Folders whose data assets are warmed (the presentation folder and the engineering fallbacks). */
	static TArray<FName> GetWarmFolders();
	/** Every soft reference reachable from Object's properties (structs, arrays, maps and sets included). Music and
	 *  ambience slots are left out: they stream on their own and would cost memory for the whole session. */
	static void CollectSoftPaths(const UObject* Object, TArray<FSoftObjectPath>& Out);
	/** Whether a warm folder's asset is warmed on this map (the galaxy skips the on-foot characters, the planets and the
	 *  interior skip the fleet and the Dominion). */
	static bool IsWantedForMap(const FString& PackagePath, const FString& MapName);

	/** Another system's own list of what to warm (#89: T2's combat cues, classes, meshes). Called once per map start
	 *  with the world (decide by map / phase); add object or class paths (FSoftClassPath is an FSoftObjectPath).
	 *  Register once, e.g. from a static initializer: UTDWarmupSubsystem::RegisterProvider(TEXT("Combat"), &Collect). */
	using FProvider = TFunction<void(const UWorld& World, TArray<FSoftObjectPath>& OutPaths)>;
	static void RegisterProvider(FName Name, FProvider Provider);
	static void UnregisterProvider(FName Name);
	static TMap<FName, FProvider>& Providers();

		/** Effects activated per frame in the warm pass (dev: TD.Warmup.PerFrame). */
	static int32 EffectsPerFrame;

private:
	void StartDataLoad();
	bool IsWantedOnThisMap(const FString& PackagePath) const;
	void OnDataLoaded();
	void OnContentLoaded();
	void FinishActivations(bool bAll);

	ETDWarmupStage Stage = ETDWarmupStage::Idle;
	TSharedPtr<FStreamableHandle> DataHandle;
	TSharedPtr<FStreamableHandle> ContentHandle;
	TArray<FSoftObjectPath> DataPaths;
	TArray<FSoftObjectPath> ContentPaths;
	UPROPERTY(Transient) TArray<TObjectPtr<UObject>> Held;
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraSystem>> ToActivate;
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> Active;
	TArray<float> ActiveAge;
	int32 WarmedEffects = 0;
	double StartSeconds = 0.0;
	float WaitLeft = 0.f;
};

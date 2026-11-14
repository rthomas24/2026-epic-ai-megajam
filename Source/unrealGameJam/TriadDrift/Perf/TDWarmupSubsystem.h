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

UCLASS()
class UNREALGAMEJAM_API UTDWarmupSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDWarmupSubsystem* Get(const UObject* WorldContextObject);

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

	/** Effects activated per frame in the warm pass (dev: TD.Warmup.PerFrame). */
	static int32 EffectsPerFrame;

private:
	void StartDataLoad();
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

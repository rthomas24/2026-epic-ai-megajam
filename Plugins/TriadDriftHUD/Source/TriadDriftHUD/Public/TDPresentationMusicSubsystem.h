#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TDPresentationMusicSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class UWorld;
struct FWorldContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDBoardingMusicOwnership, bool, bHolding);

struct FTDBoardingMusicState
{
	bool bActive = false;
	bool bEnding = false;
	uint64 Generation = 0;
	bool Begin() { if (bActive) return false; bActive = true; bEnding = false; ++Generation; return true; }
	bool End() { if (!bActive || bEnding) return false; bEnding = true; return true; }
	void Reset() { bActive = false; bEnding = false; ++Generation; }
};

/** Small playback state shared by the owner and its callback-race tests. */
struct FTDPresentationPlaylistState
{
	uint64 Generation = 0;
	int32 Index = INDEX_NONE;
	bool Begin()
	{
		if (Index != INDEX_NONE) return false;
		++Generation;
		Index = 0;
		return true;
	}
	bool Finish(uint64 CallbackGeneration)
	{
		if (Index == INDEX_NONE || CallbackGeneration != Generation) return false;
		++Generation;
		Index = Index == 0 ? 1 : INDEX_NONE;
		return true;
	}
	void Stop() { ++Generation; Index = INDEX_NONE; }
};

/** Presentation music owner. Callers yield competing music before entry and reclaim
 * it on exit. Boarding retains ownership through its end fade. No inferred phase hooks. */
UCLASS()
class TRIADDRIFTHUD_API UTDPresentationMusicSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UTDPresentationMusicSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Idempotent during this roll, including after both tracks finish. */
	UFUNCTION(BlueprintCallable, Category="Triad Drift|Music")
	void StartCreditsMusic();
	/** Ends the roll; also called on map travel. A subsequent roll may start anew. */
	UFUNCTION(BlueprintCallable, Category="Triad Drift|Music")
	void StopCreditsMusic();

	/** Call at confirmed interior sequence entry, after the combat director yields. */
	UFUNCTION(BlueprintCallable, Category="Triad Drift|Music")
	void StartBoardingMusic();
	/** Actual sequence completion only: full 20-second fade, including across travel. */
	UFUNCTION(BlueprintCallable, Category="Triad Drift|Music")
	void EndBoardingMusic();
	/** Encounter owner supplies this signal; ducks the same loop without restarting. */
	UFUNCTION(BlueprintCallable, Category="Triad Drift|Music")
	void SetBoardingCombatActive(bool bInCombat);
	UFUNCTION(BlueprintPure, Category="Triad Drift|Music")
	bool IsBoardingMusicHolding() const { return BoardingState.bActive; }
	/** Engineering must suppress general combat music while true, including the fade. */
	UPROPERTY(BlueprintAssignable, Category="Triad Drift|Music")
	FTDBoardingMusicOwnership OnBoardingMusicOwnershipChanged;

private:
	void PlayCurrentTrack();
	void ReleaseVoice();
	void HandleFinished(UAudioComponent* FinishedVoice, uint64 CallbackGeneration);
	void HandlePreLoadMap(const FWorldContext& Context, const FString& MapName);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void ReleaseBoardingVoice();
	void HandleBoardingFinished(UAudioComponent* FinishedVoice, uint64 CallbackGeneration);

	// Explicit soft references for inspection. Shipping cook must include these packages
	// through an AlwaysCook presentation asset/rule; native defaults alone are no cook proof.
	UPROPERTY() TArray<TSoftObjectPtr<USoundBase>> CreditsTracks;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;
	UPROPERTY() TSoftObjectPtr<USoundBase> BoardingMusic;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> BoardingVoice;
	TWeakObjectPtr<UWorld> BoardingWorld;
	FTDBoardingMusicState BoardingState;
	bool bBoardingCombatActive = false;
	FTDPresentationPlaylistState Playlist;
	bool bCreditsRollActive = false;
	bool bCreditsStartPending = false;
	TWeakObjectPtr<UWorld> PendingCreditsWorld;
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle WorldCleanupHandle;
};

// Triad Drift - the music event bridge (N-082; T1). Nick's audio plugin can't include game headers, so it can't bind our
// native delegates: this game-instance subsystem forwards the game moments music cares about through ONE reflected
// event, with fixed names, at the moment they happen (timing unchanged), plus a snapshot for late binders.
//
// EventId (DetailId, bFlag):
//   VictoryBegan    (None, false)          ATDMenuBackdrop::OnVictoryBegan: the victory screen's backdrop starts
//   VictoryStinger  (None, false)          ATDMenuBackdrop::OnStinger: the ending stinger plays
//   VictoryEnded    (None, false)          ATDMenuBackdrop::OnVictoryEnded: the victory screen ends
//   PlanetRevealed  (Verdance | Cinder | Galaxy, false)   UTDDescentSubsystem::OnRevealed: the white cover lifts
//   Takeoff         (Rift | Lancer | Bastion, false)      the flown ship lifts off a pad (landing state Ascending)
//   PhaseChanged    (the ETDGamePhase name, e.g. Waves1to2, DominionAssault, false)   UTDEventBus::OnPhaseChanged
//   TransmissionStarted (the transmission id, false)     UTDStorySubsystem::OnTransmission
//   TransmissionEnded   (the transmission id, bCompleted) UTDStorySubsystem::OnTransmissionEnded
//   VictoryStingerDone  (None, bCompleted)   the stinger's last line has ended: A5_DistressSignal then C9_Kato_Identifiers,
//                       C9_Wren_Source, C9_Kato_Decades (UTDMusicEventBridge::StingerLines). bCompleted false = it was cut short
//                       (the victory screen closed first). Always after VictoryStinger, always before VictoryEnded, once per stinger.
//   GroundCombat        (Verdance | Cinder, false)   on foot on a planet: a crew encounter has started (ATDEncounterVolume active)
//   GroundAllClear      (Verdance | Cinder, false)   ... and every active encounter is cleared again (exploration)
//   RevealEnded         (None, bCompleted)  the final record (story sequence Cinematic08) has finished; bCompleted false = skipped
// Order in a victory: VictoryBegan -> (C9_Moss_TakeThat) -> VictoryStinger -> TransmissionStarted/Ended A5 -> ... the three C9
// lines ... -> VictoryStingerDone -> VictoryEnded (when the player leaves the victory screen).
// WorldContext is the world the event came from: reject events whose world isn't the one you are playing in.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Story/TDStoryTypes.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TDMusicEventBridge.generated.h"

class ATDMenuBackdrop;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FTDOnMusicBridgeEvent, FName, EventId, UObject*, WorldContext, FName, DetailId, bool, bFlag);

/** Where the music-relevant game state stands now (for a binder that arrives late). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDMusicBridgeState
{
	GENERATED_BODY()

	/** The ETDGamePhase name. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") FName Phase;
	/** Between VictoryBegan and VictoryEnded. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") bool bVictory = false;
	/** The stinger has played in this victory. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") bool bVictoryStingerPlayed = false;
	/** A transmission is on the air. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") bool bTransmissionActive = false;
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") FName Transmission;
	/** Verdance / Cinder / Galaxy / MainMenu ... (the game instance's current destination). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") FName Destination;
	/** The white cover has lifted on this map (PlanetRevealed fired here); false during the descent cover. Cleared at every level change. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") bool bPlanetRevealed = false;
	/** The flown ship is parked on a pad (landing state Landed .. CanopyClosing); false again from take-off (Takeoff fires). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") bool bLanded = false;
	/** The stinger's lines have all ended (VictoryStingerDone fired); false between VictoryStinger and then. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") bool bVictoryStingerDone = false;
	/** On foot on a planet with a crew encounter running (between GroundCombat and GroundAllClear). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Music") bool bGroundCombat = false;
};

UCLASS()
class UNREALGAMEJAM_API UTDMusicEventBridge : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// The fixed EventId names.
	static const FName VictoryBegan;
	static const FName VictoryStinger;
	static const FName VictoryEnded;
	static const FName PlanetRevealed;
	static const FName Takeoff;
	static const FName PhaseChanged;
	static const FName TransmissionStarted;
	static const FName TransmissionEnded;
	static const FName VictoryStingerDone;
	static const FName GroundCombat;
	static const FName GroundAllClear;
	static const FName RevealEnded;

	/** The stinger's lines, in the order they air (ATDMenuBackdrop::PlayStinger). */
	static const TArray<FName>& StingerLines();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** The one event (see the header comment for the names). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Music")
	FTDOnMusicBridgeEvent OnMusicEvent;

	/** The bridge for any world (from the game instance). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music", meta = (WorldContext = "WorldContext"))
	static UTDMusicEventBridge* GetMusicEventBridge(const UObject* WorldContext);

	/** The state now: phase, victory, transmission, destination. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Music")
	FTDMusicBridgeState GetMusicState() const;

	// FTickableGameObject: follows the current world (binds its story subsystem and the flown ship's landing).
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTDMusicEventBridge, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableWhenPaused() const override { return true; }

	/** Forwards one event now (the handlers use it; tests too). */
	void Emit(FName EventId, UObject* WorldContext, FName DetailId, bool bFlag);

private:
	void HandleVictoryBegan(ATDMenuBackdrop* Backdrop);
	void HandleStinger(ATDMenuBackdrop* Backdrop);
	void HandleVictoryEnded(ATDMenuBackdrop* Backdrop);
	void HandleRevealed(ETDDestination Destination);
	UFUNCTION() void HandlePhase(ETDGamePhase Phase);
	UFUNCTION() void HandleTransmission(const FTDTransmissionView& Transmission);
	UFUNCTION() void HandleTransmissionEnded(FName Id, bool bCompleted);
	UFUNCTION() void HandleSequenceFinished(FName SequenceId, bool bSkipped);
	UFUNCTION() void HandleLandingState(UTDLandingComponent* Landing, ETDLandingState NewState);
	UWorld* CurrentWorld() const;
	void HandlePreLoadMap(const FString& MapName);
	void FinishStinger(bool bCompleted);
	void TickStinger(UWorld* World);
	void TickGround(UWorld* World);
	bool bGroundFight = false;
	FDelegateHandle PreLoadHandle;
	TArray<FName> StingerPending;
	bool bStingerWaiting = false;
	bool bStingerDone = false;
	bool bRevealed = false;
	bool bParked = false;

	FDelegateHandle VictoryBeganHandle, StingerHandle, VictoryEndedHandle, RevealedHandle;
	TWeakObjectPtr<UWorld> BoundWorld;
	TWeakObjectPtr<class UTDStorySubsystem> BoundStory;
	TWeakObjectPtr<UTDLandingComponent> BoundLanding;
	bool bPhaseBound = false;
	bool bVictory = false;
	bool bStinger = false;
	bool bTransmission = false;
	FName Transmission;
};

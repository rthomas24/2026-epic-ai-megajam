// Triad Drift - story transmissions: the backbone of "The Paradox" (storyline.md). Owner: Teammate 3.
//
// UTDStorySubsystem::Fire(WorldContext, Id) is the one call gameplay makes for a story beat (T4: generators, the
// assault; T5: planets, the interior; T3: intro, waves 1-2, the ending). It:
//  - looks the id up (C++ placeholder defaults, replaced by id from DA_Story_Transmissions when Nicholas adds it);
//  - plays it once per run (unless the entry is repeatable). Fired ids are kept in the run's progress, survive a
//    retake and are saved, so Continue never replays a line;
//  - queues it: one line on air at a time, never overlapping, held while a money shot or the intro owns the screen;
//  - on air: broadcasts the bark hook (UTDEventBus::OnBarkRequested, BarkId = the transmission id, Speaker = the
//    speaker's name) so presentation audio can answer it, plays the entry's Sound (2D) if it has one, fires
//    OnTransmission, and shows the line as a subtitle in the HUD's lower lane (T2's HUD reads GetCurrentTransmission
//    and sets bDrawnExternally; until then a small fallback overlay draws it).
// Console: TD.Story.Fire <Id>, TD.Story.List, TD.Story.Reset.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Story/TDStoryTypes.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDStorySubsystem.generated.h"

class SWidget;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnTransmission, const FTDTransmissionView&, Transmission);

UCLASS()
class UNREALGAMEJAM_API UTDStorySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDStorySubsystem* Get(const UObject* WorldContext);

	/** Fire a story beat. False when the id is unknown, or it already played this run (and isn't repeatable). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Story", meta = (WorldContext = "WorldContextObject"))
	static bool Fire(const UObject* WorldContextObject, FName Id);

	bool FireTransmission(FName Id);

	/** The line on air (bActive false between lines). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Story")
	FTDTransmissionView GetCurrentTransmission() const { return Current; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Story", meta = (WorldContext = "WorldContextObject"))
	static FTDTransmissionView GetCurrentTransmissionFor(const UObject* WorldContextObject);

	/** Fired when a line goes on air. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Story")
	FTDOnTransmission OnTransmission;

	/** Set by a HUD that draws the transmission itself (T2's): the fallback overlay then draws nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Story")
	bool bDrawnExternally = false;

	/** The entry for an id (defaults overlaid with Nicholas's asset). False when unknown. */
	static bool FindEntry(FName Id, FTDTransmission& Out);
	/** Every entry, in story order (TD.Story.List, the id list for the board). */
	static TArray<FTDTransmission> GetAllEntries();
	/** Text for an id, or Fallback when the id is unknown (the intro reads its narration this way). */
	static FText GetText(FName Id, const FText& Fallback);
	static FText SpeakerTag(ETDSpeaker Speaker);
	/** A pilot's trait line for the name cards (Trait_Wren / _Kato / _Moss): "BOLD · INSTINCTIVE". */
	static FText PilotTrait(ETDShipId Ship);
	static FLinearColor SpeakerColor(ETDSpeaker Speaker);
	/** On-screen seconds for an entry (Duration, or from the text length; at least the sound's length). */
	static float SecondsFor(const FTDTransmission& Entry);

	bool HasPlayed(FName Id) const;
	int32 GetQueueLength() const { return Queue.Num(); }
	/** Forget what played this run (TD.Story.Reset; a new run resets with the progress anyway). */
	void ResetPlayed();

	/** Gap between two queued lines (real seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Story")
	float GapSeconds = 0.5f;

	/** Seconds after arriving back in the galaxy from a finished planet before its departure transmissions start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Story")
	float DepartureDelaySeconds = 2.f;

	/** Cinematic 04 (Verdance done) and Cinematic 06 (Cinder done): the transmission sets, in order. */
	static const TArray<FName>& DepartureSet(ETDDestination Planet);

#if WITH_DEV_AUTOMATION_TESTS
	/** Tests: use this set instead of DA_Story_Transmissions. */
	static TWeakObjectPtr<const UTDTransmissionSet> SetOverrideForTests;
#endif

	// UWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	UFUNCTION() void HandleWaveStarted(int32 WaveNumber, float Intensity01);
	UFUNCTION() void HandleWaveCleared(int32 WaveNumber);
	void UpdateDepartures(double Now);
	double RealNow() const;
	bool IsHeld() const;
	void StartNext();
	void MarkPlayed(FName Id);
	void EnsureOverlay();
	void RemoveOverlay();

	TArray<FTDTransmission> Queue;
	FTDTransmission OnAir;
	FTDTransmissionView Current;
	double OnAirStart = -1000.0;
	float OnAirSeconds = 0.f;
	double NextAllowed = 0.0;
	/** Played this run when there is no TD game instance (tests, bare worlds). */
	TSet<FName> LocalPlayed;
	/** Cinematics 04 and 06 (transmissions over the take-off): checked once per galaxy world, after the arrival settles. */
	bool bDeparturesChecked = false;
	double DeparturesAt = -1.0;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Voice;

	TSharedPtr<SWidget> Overlay;
};

// Triad Drift - the engineering music fallback (R-047, Ryan's R-024 rule; T1, Nov 14). Nick's audio side is stopped, so
// the placements Nicholas already assigned (features.md, 497bfd73; board N-053 / N-089) play from our own music
// director's world, driven by the music event bridge (UTDMusicEventBridge), with Nick's imported assets by soft path:
//
//   Credits       after VictoryStingerDone until VictoryEnded: CreditsMusic01 Cathedral Pulse (1), then Ethereal Pulse
//                 (Remix) (the delivered order)
//   Cinder        on foot on Cinder after the reveal: CinderMusic01 World (exploration), Fall (combat: GroundCombat ..
//                 GroundAllClear)
//   Core chamber  CoreChamberEntered .. ReactorFightStarted: DominionCore01 Cathedral Pulse (the reveal plays inside it)
//   Reactor fight ReactorFightStarted .. ReactorFightEnded: DominionAssault01 Infinity Zone
//   Escape        EscapeStarted: Ritual - NOT IMPORTED: the slot is empty, nothing plays
// (The assault score and the quiet-flight bed are the director's own slots; they fall back to DominionAssault02 Push The
// Attack and IdleAmbient01 when DA_UI_Style leaves them empty: UTDMusicDirectorSubsystem::ResolveAssaultMusic / Idle.)
//
// Rules: unity gain, plain fades (2 s in, 2 s out); no loop points, offsets or mix decisions - a track loops only if
// the asset is set to loop, otherwise it plays once and the existing bed returns. It stands down (1 s fade) the moment
// ANY owner holds UTDMusicDirectorSubsystem::RequestExclusiveScore, so Nick's player takes over with no code change,
// and comes back when the claim is released if the placement still applies. While it plays, the director's combat
// bed, idle bed and the menu music hold. td.Music.Fallback 0 turns all of it off. Nothing of Nick's is touched.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDMusicFallbackSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

UENUM(BlueprintType)
enum class ETDMusicPlacement : uint8
{
	None,
	Credits,
	CinderExplore,
	CinderCombat,
	CoreChamber,
	ReactorFight,
	Escape,
	VerdanceExplore	// Lead, Nov 14: Nick's Verdance theme (N-059) after the reveal, until his own player claims the score
};

UCLASS()
class UNREALGAMEJAM_API UTDMusicFallbackSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

	static UTDMusicFallbackSubsystem* Get(const UObject* WorldContext);
	/** td.Music.Fallback (default 1). */
	static bool IsEnabled();
	/** The track list of a placement, in play order (soft paths; empty = not imported). */
	static const TArray<FSoftObjectPath>& TracksFor(ETDMusicPlacement Placement);

	/** The placement the game wants now (whether or not it is allowed to sound). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music") ETDMusicPlacement GetWantedPlacement() const { return Wanted; }
	/** The placement sounding now (None while it stands down, is off, or has nothing to play). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music") ETDMusicPlacement GetPlayingPlacement() const { return Playing; }
	/** The track sounding now (empty when none). */
	FSoftObjectPath GetPlayingTrack() const;
	/** True while a fallback track sounds (the director's beds and the menu music hold). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music") bool IsFallbackScorePlaying() const { return Playing != ETDMusicPlacement::None; }

	/** Tests: the current track ended (an audio component's OnAudioFinished does this in the game). */
	void NotifyTrackFinishedForTests() { HandleFinished(); }

	static constexpr float FadeInSeconds = 2.f;
	static constexpr float FadeOutSeconds = 2.f;
	static constexpr float StandDownSeconds = 1.f;

private:
	UFUNCTION() void HandleBridgeEvent(FName EventId, UObject* WorldContext, FName DetailId, bool bFlag);
	UFUNCTION() void HandleFinished();
	ETDMusicPlacement ComputeWanted() const;
	void StartTrack(ETDMusicPlacement Placement, int32 Index);
	void StopVoice(float FadeSeconds);

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;
	ETDMusicPlacement EventPlacement = ETDMusicPlacement::None; // from events: credits, core chamber, reactor, escape
	ETDMusicPlacement Wanted = ETDMusicPlacement::None;
	ETDMusicPlacement Playing = ETDMusicPlacement::None;
	ETDMusicPlacement FinishedPlacement = ETDMusicPlacement::None; // played once to its end: don't restart it
	int32 TrackIndex = 0;
	bool bBound = false;
	bool bTakenOff = false;	// the bridge's Takeoff: the planet theme fades as the ship lifts (a new world starts clear)
	bool bGroundFight = false; // GroundCombat .. GroundAllClear (the snapshot's bGroundCombat for a late binder)
};

// Triad Drift - the on-foot barks (ENG-43, dialogs.md's T5 rows). Owner: Teammate 5.
//
// One world subsystem says every on-foot and boarding bark through ITDGameFlow::RequestBark, with T3's limiter
// (FTDBarkLimiter: per-id cooldowns, the global gap, the window cap, Flavour only after a quiet spell) and one more
// rule: never while a story transmission is on air or a sequence plays (the must-hear lines win). Some barks are polled
// here, from the course's tick (arrival, health, idle, objective near, doors, hazard, the escape clock, boarded,
// welcome); the rest are told by
// the code that knows the moment (encounters, crew types, the blade, objectives, take-off, the reactor).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Squad/TDSquadBarks.h"
#include "TDGroundBarks.generated.h"

class ATDFirstPersonCharacter;
class ATDEncounterVolume;
class UTDHealthComponent;
struct FTDDamageInfo;

/** Who says it (dialogs.md's speaker column). */
enum class ETDFootSpeaker : uint8 { Pilot, Wingman, Kato, Command, Moss, Wren /* #209-allies: a follower speaks as itself */ };

UCLASS()
class UNREALGAMEJAM_API UTDGroundBarks : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDGroundBarks* Get(const UObject* WorldContext);

	/** Say a bark now if the limiter and the story allow it. Cooldown <= 0: the tuning's default. True if it was said. */
	bool Say(FName BarkId, ETDFootSpeaker Who, ETDBarkPriority Priority, float CooldownSeconds = 0.f);
	/** Once per run (a crew type's first sighting, the arrival). */
	bool SayOnce(FName BarkId, ETDFootSpeaker Who, ETDBarkPriority Priority);

	// Told by the code that knows the moment.
	void NotifyEncounterStarted(const ATDEncounterVolume* Encounter);
	void NotifyEncounterCleared(const ATDEncounterVolume* Encounter);
	void NotifyObjectiveDone();
	void NotifyTakeOff();
	void NotifyConduitDown();
	void NotifyCoreExposed();

	/** Every bark said, in order (tests). */
	const TArray<FName>& GetSaid() const { return Said; }
	/** Who said this bark last (tests: N-093's lines name their speaker). */
	FName GetLastSpeaker(FName BarkId) const { const FName* S = LastSpeaker.Find(BarkId); return S ? *S : NAME_None; }
	FName SpeakerName(ETDFootSpeaker Who) const;

	/** The polled barks; the course calls it every frame (on foot only: planets and the interior). */
	void TickBarks(float DeltaTime);

	/** Tests: forget the limiter, the once-per-run set and the log. */
	void ResetForTests();

private:
	void Bind(ATDFirstPersonCharacter* Pilot);
	UFUNCTION() void HandlePilotDamaged(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);
	bool IsStoryOnAir() const;

	FTDBarkTuning Tuning;
	FTDBarkLimiter Limiter;
	TSet<FName> SaidOnce;
	TArray<FName> Said;
	TMap<FName, FName> LastSpeaker;
	TWeakObjectPtr<ATDFirstPersonCharacter> Bound;
	float LastHull01 = 1.f;
	bool bLowArmed = true;
	float OnFootSeconds = 0.f;
	float IdleSeconds = 0.f;
	float SlamWindowLeft = 0.f;
	int32 SlamHits = 0;
	int32 Kills = 0;
	float EscapeLastSeconds = -1.f;
	TSet<TWeakObjectPtr<AActor>> NearMarkers;
	TSet<TWeakObjectPtr<AActor>> MagmaWarned; // each magma's edge warning, once per visit
public:
	/** The edge warning's reach: within this of a magma's footprint the first time, a wingman warns and a tip shows. */
	static constexpr float MagmaWarnDistance = 600.f;
	bool HasWarnedAbout(const AActor* Magma) const { return MagmaWarned.Contains(Magma); }
private:
	TMap<TWeakObjectPtr<AActor>, bool> DoorWasOpen;
	float DoorPushSeconds = 0.f;
};

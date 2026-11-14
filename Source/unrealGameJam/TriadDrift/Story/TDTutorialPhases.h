// Triad Drift - ENG-40 tutorial phase prompts over waves 1-2 (storyline.md §13 phases 01-03). Owner: Teammate 3.
//
// One taught action at a time, each finished by the player doing it, never a hard lock (a timed substitute):
//   Evade          "EVASIVE MANEUVERS"  from wave 1's start: boost or dodge, having flown a few seconds
//   TargetAndFire  "TARGET AND FIRE"    TB_Kato_Marking: kills by the player's own ship
//   SquadSystems   "SQUAD SYSTEMS"      TB_Moss_ThreeShips: a ship switch and a signature power
//   Done           heavy contact, the withdrawal and the escape are T4's (unchanged)
// The prompt is the objective line while a phase is up (T5's FTDObjectiveText reads GetPrompt). T4's wave director may
// gate enemy pressure on GetTeachPhase / OnTeachPhaseChanged. Not a tutorial run (ETDTutorialMode ForceOff, a test world
// without ForceOn, the tutorial skipped so wave 1 never starts): nothing runs. The withdrawal ends the phases at once.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDTutorialPhases.generated.h"

UENUM(BlueprintType)
enum class ETDTeachPhase : uint8
{
	None,
	Evade,
	TargetAndFire,
	SquadSystems,
	Done
};

UCLASS()
class UNREALGAMEJAM_API UTDTutorialPhases : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDTutorialPhases* Get(const UObject* WorldContext);

	/** The phase now (None before wave 1 of a tutorial run, or in any other run). */
	static ETDTeachPhase GetTeachPhase(const UObject* WorldContext);
	ETDTeachPhase GetPhase() const { return Phase; }

	/** The objective line while a phase is up: "TARGET AND FIRE  1 / 2". False when no prompt (T5's objective text). */
	static bool GetPrompt(const UObject* WorldContext, FName& OutId, FText& OutText);

	/** Every phase change (T4's wave pressure; the HUD). */
	TMulticastDelegate<void(ETDTeachPhase)> OnTeachPhaseChanged;

	/** End the lesson now (the tutorial skip, the withdrawal, TD.Tutorial.Phases skip). */
	void SkipAll();

	// Tuning
	float EvadeFlySeconds = 4.f;       // flown this long, plus a boost or a dodge
	float EvadeSubstituteSeconds = 30.f;
	int32 KillsToTeach = 2;
	float SquadSubstituteSeconds = 60.f;

	// UTickableWorldSubsystem
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

#if WITH_DEV_AUTOMATION_TESTS
	/** Tests: treat this world as a tutorial run (test worlds are not real levels). */
	bool bForceTutorialForTests = false;
	int32 GetKills() const { return Kills; }
#endif

private:
	UFUNCTION() void HandleWaveStarted(int32 WaveNumber, float Intensity01);
	UFUNCTION() void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);
	UFUNCTION() void HandleShipSwitched(ETDShipId NewShip, ETDShipId OldShip);
	UFUNCTION() void HandlePowerUsed(ETDShipId Ship, FName PowerName);

	bool IsTutorialRun() const;
	void SetPhase(ETDTeachPhase NewPhase);
	APawn* GetPlayerPawn() const;

	ETDTeachPhase Phase = ETDTeachPhase::None;
	float PhaseSeconds = 0.f;
	float FlySeconds = 0.f;
	bool bEvaded = false;
	int32 Kills = 0;
	bool bSwitched = false;
	bool bPowerUsed = false;
};

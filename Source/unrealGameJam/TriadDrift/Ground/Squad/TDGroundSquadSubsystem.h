// Triad Drift - the on-foot squad (#209): owns the two ally pilots that walk and fight beside the player on planets and
// in the Dominion interior. Owner: ground squad builder.
//
// It needs no call from the planet or interior code: every frame it looks at the player's pawn. While that is the
// on-foot pilot, visible and done climbing out of the ship, the two other pilots exist; when the pilot climbs back in
// (hidden) or the map is not an on-foot one, they are gone. A retake (the pilot alive again after dying) puts them
// back beside the pilot at full health.
//
// Dev: TD.Ground.Allies 0/1 (default 1), TD.Ground.AlliesDump, TD.Ground.AllyDown <0|1>, td.Ground.SquadScale <x>.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDGroundSquadSubsystem.generated.h"

class ATDGroundAlly;
class ATDFirstPersonCharacter;

/** One ally for the on-foot HUD (T2 draws it). */
USTRUCT(BlueprintType)
struct FTDGroundAllyStatus
{
	GENERATED_BODY()

	/** Wren, Kato or Moss. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") FName Pilot;
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") ETDShipId Ship = ETDShipId::Rift;
	/** The pilot's squad colour (the blade's colour). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") FLinearColor Color = FLinearColor::White;
	/** 0..1; 0 while down. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") float Health01 = 1.f;
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") bool bDown = false;
	/** 0..1 through the time on the ground (the get-up timer); 0 when standing. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") float GetUp01 = 0.f;
	/** Has an enemy of its own right now. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") bool bFighting = false;
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") TObjectPtr<AActor> Actor = nullptr;
};

/** What happened to a follower (for barks, the HUD and story code that must tell a follower from the player). */
UENUM(BlueprintType)
enum class ETDGroundSquadEvent : uint8
{
	Joined,
	Engaged,
	Kill,
	/** Knocked down for a few seconds. Temporary: never a death. */
	Down,
	Recovered,
	Boarded
};

USTRUCT(BlueprintType)
struct FTDGroundSquadEventInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") ETDGroundSquadEvent Event = ETDGroundSquadEvent::Joined;
	/** The pilot the player is (the speaker context): Wren, Kato or Moss. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") FName PossessedPilot;
	/** Who the event is about (and who would say a line about it): always a follower here. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") FName Subject;
	/** Always true for these events: the subject follows, the possessed pilot is the player. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") bool bSubjectIsFollower = true;
	/** The subject's state after the event. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") bool bDown = false;
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") TObjectPtr<AActor> SubjectActor = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|GroundSquad") TObjectPtr<AActor> Other = nullptr;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnGroundSquadEvent, const FTDGroundSquadEventInfo&);

UCLASS()
class UNREALGAMEJAM_API UTDGroundSquadSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDGroundSquadSubsystem* Get(const UObject* WorldContextObject);

	/** TD.Ground.Allies, and never inside an automation test unless the test asks (SetForcedForTests). */
	static bool AreAlliesEnabled();
	/** The encounter size multiplier for a squad of three (td.Ground.SquadScale, default 1.6); 1 when allies are off. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|GroundSquad")
	static float GetEncounterScale();
	/** Tests: allow allies in an automation world (off by default so the existing on-foot tests fight alone). */
	static void SetForcedForTests(bool bForce);

	/** For the on-foot HUD: one entry per ally (two while on foot, none otherwise), in squad order. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|GroundSquad")
	void GetAllyStatus(TArray<FTDGroundAllyStatus>& OutAllies) const;
	UFUNCTION(BlueprintPure, Category = "TriadDrift|GroundSquad")
	int32 GetAllyCount() const { return Allies.Num(); }
	ATDGroundAlly* GetAlly(int32 Index) const { return Allies.IsValidIndex(Index) ? Allies[Index] : nullptr; }

	/** The on-foot pilot the allies follow (null when the player is not on foot). */
	ATDFirstPersonCharacter* GetLeader() const { return Leader.Get(); }
	/** Where ally Slot stands: beside and a little behind the player, in the (smoothed) view frame. */
	FVector GetFormationSlot(int32 Slot) const;
	float GetFormationYaw() const { return FormationYaw; }
	/** Another ally already fights Target. */
	bool IsTargetTaken(const AActor* Target, const ATDGroundAlly* Asker) const;
	/** A standable, clear spot near the player for a spawn or a teleport (Slot picks the side). */
	FVector FindSpotNearLeader(int32 Slot) const;

	/** Every follower event, with the possessed pilot, the subject, its role and its down state. */
	FTDOnGroundSquadEvent OnSquadEvent;
	void NotifyAllyEvent(ATDGroundAlly* Ally, ETDGroundSquadEvent Event, AActor* Other = nullptr);

	void DumpToLog() const;

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTDGroundSquadSubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	/** Behind and to the side of the player (cm): abreast, a step back, so both are in the chase camera's frame edges
	 *  and never ahead of the player or on the camera's centre line. */
	float SlotBack = 40.f;
	float SlotSide = 240.f;
	/** A wall within this of the player on either side (a corridor under about 4 m, a doorway): single file behind. */
	float SingleFileHalfWidth = 200.f;
	float SingleFileBack = 190.f;
	float SingleFileGap = 150.f;
	/** How fast the formation frame follows the view (deg/s): slow, so a flick of the camera does not swing the squad round. */
	float FormationTurnRate = 70.f;

private:
	void SpawnAllies(ATDFirstPersonCharacter* InLeader);
	void DespawnAllies();

	UPROPERTY(Transient) TArray<TObjectPtr<ATDGroundAlly>> Allies;
	TWeakObjectPtr<ATDFirstPersonCharacter> Leader;
	FName LeaderPilot;
	float FormationYaw = 0.f;
	bool bLeaderWasDead = false;
	bool bBoarding = false;
	FVector LastLeaderLocation = FVector::ZeroVector;
};

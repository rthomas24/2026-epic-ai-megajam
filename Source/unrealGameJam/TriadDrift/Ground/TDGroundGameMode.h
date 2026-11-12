// Triad Drift - zero-asset ground sandbox (ENG-21/22/24). Owner: Teammate 5.
//
// UnrealEditor.exe <uproject> /Engine/Maps/Entry?game=/Script/unrealGameJam.TDGroundGameMode -game -windowed

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDGroundGameMode.generated.h"

class ATDObjective;
class ATDDummyTurret;
class ATDEncounterVolume;
class ATDInteractableActor;
class ATDDoor;
class ATDPlanetDirector;

/**
 * On any map (an empty one is fine) builds a lit training ground from engine basic shapes:
 * - a floor
 * - mantle ledges of 100/150/180/250 cm (the last is too tall: it needs the double jump)
 * - parkour platforms
 * - a practice turret that fires deflectable 3-round bursts
 * - two melee dummies that swing at you, so you can practise parries
 * - a sample objective: hold-interact a console for 2 s, then clear a room of three crew behind a door
 * The player is an ATDFirstPersonCharacter. Death -> CUT! -> respawn at the start as the next take.
 */
UCLASS(Config = Game)
class UNREALGAMEJAM_API ATDGroundGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDGroundGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	FTransform PlayerSpawn = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 100.f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	bool bSpawnEnvironment = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	bool bSpawnTrainingGround = true;

	/** Which planet this stands in for (checkpoints and scene names). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	ETDDestination Destination = ETDDestination::Verdance;

	/** Where the planet's landing pad goes on a real planet map (the layout of ENG-25/26 sets it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FTransform PlanetPadTransform = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 0.f));

	UFUNCTION(BlueprintPure, Category = "Planet") bool IsPlanetMode() const { return bPlanetMode; }
	UFUNCTION(BlueprintPure, Category = "Planet") ATDPlanetDirector* GetPlanetDirector() const { return PlanetDirector; }

	UFUNCTION(BlueprintPure, Category = "Sandbox") ATDDummyTurret* GetTurret() const { return Turret; }
	UFUNCTION(BlueprintPure, Category = "Sandbox") ATDObjective* GetSampleObjective() const { return SampleObjective; }
	UFUNCTION(BlueprintPure, Category = "Sandbox") ATDEncounterVolume* GetEncounter() const { return Encounter; }

	/** Where a retake puts the player: the start used, moved up by each objective step completed on foot. */
	UFUNCTION(BlueprintPure, Category = "Sandbox") FTransform GetRespawnTransform() const { return bHasRespawnTransform ? RespawnTransform : PlayerSpawn; }

	/** Puts the player back at the respawn point, full health (new take). */
	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void RespawnPlayer();

protected:
	void SpawnEnvironment();
	void SpawnTrainingGround();
	void SpawnPlanetVisit();
	AActor* SpawnBlock(const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, const TCHAR* Label);
	void HandleSceneRestart(FName SceneId, int32 TakeNumber);

	UFUNCTION()
	void HandleConsoleUsed(AActor* Interactable, AActor* Interactor);

	UFUNCTION()
	void HandleObjectiveUpdated(FName ObjectiveId, int32 Step, FText Text);

	UPROPERTY(Transient) TObjectPtr<AActor> SpawnPoint;
	UPROPERTY(Transient) TObjectPtr<ATDDummyTurret> Turret;
	UPROPERTY(Transient) TObjectPtr<ATDObjective> SampleObjective;
	UPROPERTY(Transient) TObjectPtr<ATDEncounterVolume> Encounter;
	UPROPERTY(Transient) TObjectPtr<ATDInteractableActor> Console;
	UPROPERTY(Transient) TObjectPtr<ATDDoor> RoomDoor;
	UPROPERTY(Transient) TObjectPtr<class ATDPlanetDirector> PlanetDirector;
	bool bPlanetMode = false;
	FTransform RespawnTransform;
	bool bHasRespawnTransform = false;

	FDelegateHandle SceneRestartHandle;
};

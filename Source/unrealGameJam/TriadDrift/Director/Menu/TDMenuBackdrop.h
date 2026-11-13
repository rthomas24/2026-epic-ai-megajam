// Triad Drift - the main menu's backdrop (first impression). Owner: Teammate 3.
//
// Spawned at runtime by the menu game mode (no map edit): the engineering nebula sky and its sun, ringed Verdance
// three-quarter lit, a slow drifting director camera, and the squad (their art) gliding past in formation every
// ~20 s. Off when the menu map brings its own scene (any actor tagged TD.Menu.Scene), when td.MenuBackdrop is 0, or
// with no local player. Everything it spawns goes with the map.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDMenuBackdrop.generated.h"

class ACameraActor;
class AGameModeBase;
class UStaticMeshComponent;

UCLASS()
class UNREALGAMEJAM_API ATDMenuBackdrop : public AActor
{
	GENERATED_BODY()

public:
	ATDMenuBackdrop();

	/** The menu mode's StartPlay calls this. Null when the backdrop is off (see the class comment). */
	static ATDMenuBackdrop* SpawnFor(AGameModeBase* MenuMode);

	/** Tag on any actor of the menu map that is Nick's own menu scene: the backdrop stays off. */
	static const FName MenuSceneTag; // "TD.Menu.Scene"

	/** Seconds between two squad fly-bys, and how long one takes to cross the frame. */
	UPROPERTY(EditAnywhere, Category = "Menu Backdrop") float FlybyEverySeconds = 20.f;
	UPROPERTY(EditAnywhere, Category = "Menu Backdrop") float FlybySeconds = 9.f;
	/** The first fly-by comes this soon after the menu opens. */
	UPROPERTY(EditAnywhere, Category = "Menu Backdrop") float FirstFlybySeconds = 4.f;

	// State (tests, captures)
	ACameraActor* GetCamera() const { return Camera; }
	AActor* GetPlanet() const { return Planet; }
	bool IsFlybyActive() const { return FlybyT >= 0.f; }
	int32 GetFlybyCount() const { return Flybys; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetSquadMeshes() const { return SquadMeshes; }
	/** Captures: jump to a moment of the loop (seconds since the menu opened). */
	void DebugSetTime(float Seconds);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Build();
	void UpdateCamera();
	void UpdateFlyby(float Dt);

	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TObjectPtr<AActor> Planet;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Spawned;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> SquadMeshes;

	FVector PlanetAt = FVector::ZeroVector;
	float PlanetRadius = 60000.f;
	FVector CamBase = FVector::ZeroVector;
	FVector LookBase = FVector::ZeroVector;
	float Time = 0.f;
	float NextFlyby = 0.f;
	float FlybyT = -1.f;
	int32 Flybys = 0;
	FVector FlybyFrom = FVector::ZeroVector;
	FVector FlybyTo = FVector::ZeroVector;
};

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

/** Which scene: the main menu, or the victory screen (the Dominion breaking up, the squad flying home). */
UENUM()
enum class ETDBackdropScene : uint8
{
	Menu,
	Victory
};

UCLASS()
class UNREALGAMEJAM_API ATDMenuBackdrop : public AActor
{
	GENERATED_BODY()

public:
	ATDMenuBackdrop();

	/** The menu mode's StartPlay calls this. Null when the backdrop is off (see the class comment). */
	static ATDMenuBackdrop* SpawnFor(AGameModeBase* MenuMode, ETDBackdropScene Scene = ETDBackdropScene::Menu);

	UPROPERTY(EditAnywhere, Category = "Menu Backdrop") ETDBackdropScene Scene = ETDBackdropScene::Menu;
	AActor* GetDominion() const { return Dominion; }
	/** Seconds into the victory scene before the distress-signal stinger (A5_DistressSignal) and the cut to black. */
	UPROPERTY(EditAnywhere, Category = "Menu Backdrop") float StingerSeconds = 18.f;
	bool HasPlayedStinger() const { return bStingerPlayed; }
	int32 GetFireCount() const { return Fires; }

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
	void BuildVictory(UWorld* World);
	void UpdateBreakup(float Dt);

	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TObjectPtr<AActor> Planet;
	UPROPERTY(Transient) TObjectPtr<AActor> Dominion;
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
	// Victory: the Dominion's breakup.
	FTransform DominionStart = FTransform::Identity;
	float DominionRadius = 31000.f;
	float NextFire = 0.f;
	int32 Fires = 0;
	bool bStingerPlayed = false;
	FRandomStream FireRng;
};

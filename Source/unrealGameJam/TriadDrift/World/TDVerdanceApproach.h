// Triad Drift - World (Teammate 4). Story (storyline 14-18, reduced): the approach to Verdance - Armada convoys
// ferrying scanning gear, one of them scannable, and a transponder that flickers.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDVerdanceApproach.generated.h"

class UStaticMeshComponent;

/**
 * A column of Armada haulers on a lane, carrying scanning gear rather than troops. They ignore the squad (no weapons,
 * not targetable, no collision) and loop along the lane while the run is on its way to Verdance (Phase Verdance);
 * hidden otherwise. One convoy can carry the scannable hauler: fly within ScanRadius of it and stay there for
 * ScanSeconds (a hold, no HUD mode: the hauler's scanner ring brightens while it is being read) and the
 * A2_ConvoyScanned transmission plays, once per run.
 */
UCLASS()
class UNREALGAMEJAM_API ATDArmadaConvoy : public AActor
{
	GENERATED_BODY()

public:
	ATDArmadaConvoy();

	/** Lane from A to B (world), Count haulers ScanGap apart; bScannable makes the lead hauler readable. */
	void SetupLane(const FVector& A, const FVector& B, int32 Count, bool bScannable);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Convoy", meta = (ClampMin = "0"))
	float Speed = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Convoy", meta = (ClampMin = "0"))
	float Spacing = 30000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Convoy", meta = (ClampMin = "0"))
	float ScanRadius = 25000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Convoy", meta = (ClampMin = "0.1"))
	float ScanSeconds = 3.f;

	bool IsActive() const { return bActive; }
	bool IsScanned() const { return bScanned; }
	float GetScanProgress01() const { return FMath::Clamp(ScanTime / FMath::Max(ScanSeconds, 0.1f), 0.f, 1.f); }
	int32 GetHaulerCount() const { return Haulers.Num(); }
	FVector GetHaulerLocation(int32 Index) const;

	/** Tests / dev: force active regardless of the phase (-1 = follow the phase). */
	void SetActiveOverride(int32 InOverride) { ActiveOverride = InOverride; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	struct FHauler
	{
		TObjectPtr<UStaticMeshComponent> Body;
		TObjectPtr<UStaticMeshComponent> PodL;
		TObjectPtr<UStaticMeshComponent> PodR;
		TObjectPtr<UStaticMeshComponent> Ring;
		float Along = 0.f;	// cm along the lane
	};

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Convoy")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	TArray<FHauler> Haulers;
	FVector LaneA = FVector::ZeroVector;
	FVector LaneB = FVector::ZeroVector;
	bool bScannable = false;
	bool bScanned = false;
	bool bActive = false;
	int32 ActiveOverride = -1;
	float ScanTime = 0.f;
	float PollTimer = 0.f;
	float RingSpin = 0.f;
	bool bRingHot = false;

	UPROPERTY(Transient)
	TObjectPtr<class UStaticMesh> CubeMesh;
	UPROPERTY(Transient)
	TObjectPtr<class UStaticMesh> SphereMesh;
	UPROPERTY(Transient)
	TObjectPtr<class UStaticMesh> CylinderMesh;
};

/**
 * A transponder near Verdance's orbit that flickers in an irregular pattern (on the run's way to Verdance; hidden
 * otherwise). The first time the squad comes within NoticeRadius: A2_TransponderSighted (once per run).
 */
UCLASS()
class UNREALGAMEJAM_API ATDTransponderBeacon : public AActor
{
	GENERATED_BODY()

public:
	ATDTransponderBeacon();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Transponder", meta = (ClampMin = "0"))
	float NoticeRadius = 200000.f;

	bool IsActive() const { return bActive; }
	bool IsLit() const { return bLit; }
	bool WasSighted() const { return bSighted; }
	void SetActiveOverride(int32 InOverride) { ActiveOverride = InOverride; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Transponder")
	TObjectPtr<UStaticMeshComponent> Light;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Transponder")
	TObjectPtr<UStaticMeshComponent> Mast;

	float Time = 0.f;
	float PollTimer = 0.f;
	int32 ActiveOverride = -1;
	bool bActive = false;
	bool bLit = false;
	bool bSighted = false;
};

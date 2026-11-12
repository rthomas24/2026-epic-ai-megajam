// Triad Drift - Enemies & Waves (Teammate 4). ENG-14 wave table types.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDWaveTypes.generated.h"

/** Where a group warps in, relative to the player's active ship. */
UENUM(BlueprintType)
enum class ETDSpawnZone : uint8
{
	Behind,
	FlankLeft,
	FlankRight,
	Above,
	Below,
	Ahead,
	AnyFlank	// picks Behind / FlankLeft / FlankRight per group
};

/** One group of identical enemies: Packs x PerPack ships of EnemyId. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDWaveGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	FName EnemyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "1"))
	int32 Packs = 1;

	/** Ships per pack. Packs of fighters fly in formation (first ship leads). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "1"))
	int32 PerPack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	ETDSpawnZone Zone = ETDSpawnZone::AnyFlank;

	/** Seconds after the wave starts before this group warps in (staggered arrivals). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0"))
	float Delay = 0.f;

	int32 Total() const { return FMath::Max(0, Packs) * FMath::Max(0, PerPack); }
};

/** One wave ("scene"). Also usable as a DT_Waves row. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDWaveDef : public FTableRowBase
{
	GENERATED_BODY()

	/** Director's Cut scene id ("Wave1"...). Used for takes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	FName SceneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	FText Title;

	/** Drives music and presentation (OnWaveStarted). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0", ClampMax = "1"))
	float Intensity01 = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FTDWaveGroup> Groups;

	/** Tutorial tip shown when the wave starts (NAME_None = none). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	FName StartTip = NAME_None;

	/** Planet marker revealed when this wave is cleared. Galaxy = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bRevealsMarker = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (EditCondition = "bRevealsMarker"))
	ETDDestination MarkerRevealed = ETDDestination::Galaxy;

	/** Clearing this wave reveals the Dominion (ENG-18). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bRevealsDominion = false;

	/**
	 * Game phase this wave belongs to; clearing the LAST wave of a phase ends the run of waves
	 * (the flow sends the player to the planet). Waves 1-2 -> Waves1to2 etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	ETDGamePhase Phase = ETDGamePhase::Waves1to2;

	int32 TotalEnemies() const
	{
		int32 N = 0;
		for (const FTDWaveGroup& G : Groups) { N += G.Total(); }
		return N;
	}
};

/** Tip ids the wave director requests (Nick's DT_Tips rows use the same names). */
namespace TDTipIds
{
	inline const FName Switching(TEXT("Tip_Switching"));
	inline const FName Drift(TEXT("Tip_Drift"));
	inline const FName Hyperdrive(TEXT("Tip_Hyperdrive"));
	inline const FName Gunships(TEXT("Tip_Gunships"));
	inline const FName Carriers(TEXT("Tip_Carriers"));
}

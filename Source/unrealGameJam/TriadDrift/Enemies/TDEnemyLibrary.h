// Triad Drift - Enemies & Waves (Teammate 4). ENG-13 enemy table as C++ defaults.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TriadDrift/Enemies/TDEnemyTypes.h"
#include "TDEnemyLibrary.generated.h"

/** Stable enemy ids used by waves, tests and Blueprints. */
namespace TDEnemyIds
{
	inline const FName ShardFighter(TEXT("ShardFighter"));
	inline const FName HookInterceptor(TEXT("HookInterceptor"));
	inline const FName WallGunship(TEXT("WallGunship"));
	inline const FName MaulBomber(TEXT("MaulBomber"));
	inline const FName HiveCarrier(TEXT("HiveCarrier"));
}

/**
 * Enemy weapon ids that T2 registers in UTDWeaponLibrary (ENG-05/13). Enemy code only passes
 * these ids to UTDWeaponComponent::EquipWeapon; mines and fighter launches are enemy-side.
 */
namespace TDEnemyWeaponIds
{
	inline const FName PulseGuns(TEXT("Enemy_PulseGuns"));			// 5 dmg, 4/s
	inline const FName HomingMissile(TEXT("Enemy_HomingMissile"));	// 25 dmg, homing
	inline const FName HeavyBolt(TEXT("Enemy_HeavyBolt"));			// 20 dmg, slow
	inline const FName SalvoMissile(TEXT("Enemy_SalvoMissile"));	// 3 x 20 dmg
	inline const FName PointDefense(TEXT("Enemy_PointDefense"));	// 6 dmg
}

/**
 * The five Null Armada ship types (features.md ENG-13) as C++ defaults.
 * Get() never fails: an unknown id logs a warning and returns an invalid spec (Id == NAME_None).
 */
UCLASS()
class UNREALGAMEJAM_API UTDEnemyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The spec for an enemy id. Returns a static invalid spec for unknown ids. */
	static const FTDEnemySpec& Get(FName EnemyId);

	/** Blueprint copy of Get(). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemies", meta = (DisplayName = "Get Enemy Spec"))
	static FTDEnemySpec GetEnemySpec(FName EnemyId, bool& bFound);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemies")
	static bool HasEnemy(FName EnemyId);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemies")
	static TArray<FName> GetAllEnemyIds();

	/**
	 * Replace or add a spec at runtime (tuning, debug menu, tests). Not persisted.
	 * Call ResetToDefaults() to undo.
	 */
	static void OverrideSpec(const FTDEnemySpec& Spec);
	static void ResetToDefaults();

private:
	static TMap<FName, FTDEnemySpec>& Table();
	static void BuildDefaults(TMap<FName, FTDEnemySpec>& Out);
};

// Triad Drift - Combat (Teammate 2). Every weapon in the game as C++ defaults (ENG-05, ENG-13).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TDWeaponLibrary.generated.h"

UCLASS()
class UNREALGAMEJAM_API UTDWeaponLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The spec for WeaponId. Unknown ids log a warning and return an invalid (IsValid() == false) spec. */
	static const FTDWeaponSpec& Get(FName WeaponId);

	/** True if WeaponId is in the library. */
	static bool Has(FName WeaponId);

	/** Every id in the library (squad weapons first, then enemy weapons). */
	static TArray<FName> GetAllIds();

	/** Blueprint copy of Get. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Combat", meta = (DisplayName = "Get Weapon Spec"))
	static FTDWeaponSpec GetWeaponSpec(FName WeaponId) { return Get(WeaponId); }

private:
	static const TMap<FName, FTDWeaponSpec>& GetTable();
};

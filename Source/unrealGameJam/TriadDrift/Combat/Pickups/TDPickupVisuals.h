// Triad Drift - the look of space pickups (Teammate 2). The effects builder's systems and Nicholas's sounds go here.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TDPickupVisuals.generated.h"

/** One pickup type's look: Loop plays attached while it floats (VFX + an optional sound), Collect at the moment it is taken. */
USTRUCT(BlueprintType)
struct FTDPickupLook
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FTDFXCue Loop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FTDFXCue Collect;
};

/**
 * DA_Pickup_Visuals: one look per pickup type, found through UTDStatics::LoadPresentation like DA_UI_Style. The
 * engineering asset lives in the fallback folder, /Game/TriadDrift/Gameplay/Characters/FX/DA_Pickup_Visuals (never in
 * Nick's /Game/TriadDrift/Presentation/); an asset of Nick's at Presentation/DA_Pickup_Visuals would win field by field.
 * Everything is optional: a type whose Loop has no VFX keeps engineering's glowing primitive shape; an empty Collect
 * plays nothing. e.g. Hull.Loop.VFX = NS_TD_Pickup_Hull, Hull.Collect.VFX = NS_TD_Pickup_Hull_Collect.
 */
UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDPickupVisuals : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickups") FTDPickupLook Hull;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickups") FTDPickupLook Shield;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickups") FTDPickupLook Power;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickups") FTDPickupLook Overcharge;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickups") FTDPickupLook Boost;
	/** A soft tick (sound, optional VFX on the ship) when a broken asteroid or wreck tops up the boost meter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickups") FTDFXCue BreakBoost;

	/** The asset's name (Presentation first, then the engineering fallback folder; merged field by field). */
	static constexpr const TCHAR* AssetName = TEXT("DA_Pickup_Visuals");
	/** The loaded asset, or null (tests may set an override). */
	static const UTDPickupVisuals* Get();
	static void SetOverrideForTests(const UTDPickupVisuals* Override);

	const FTDPickupLook& LookOf(uint8 Type) const
	{
		return Type == 0 ? Hull : Type == 1 ? Shield : Type == 2 ? Power : Type == 3 ? Overcharge : Boost;
	}
};

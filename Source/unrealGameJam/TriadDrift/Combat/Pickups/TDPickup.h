// Triad Drift - space pickups (Teammate 2). docs/eng/WORLD-DENSITY-PLAN.md section 3.5; the Lead's spec, Nov 13.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDPickup.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class ETDPickupType : uint8
{
	Hull,		// +hull on the flown ship (green, a plus)
	Shield,		// full shield on the flown ship, half on the wingmen (white-blue, a disc)
	Power,		// signature power ready now (violet, a double point)
	Overcharge	// weapon damage up for a few seconds (gold-white, a turned cube)
};

/**
 * A pickup floating in space: a glowing shape per type from engine primitives and the placeholder glow material (the
 * effects builder replaces the look). No collision at all: it never blocks a shot or a ship; UTDPickupSubsystem pulls it
 * to the flown ship and collects it by distance. Lives a fixed time, blinks before it goes.
 */
UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDPickup : public AActor
{
	GENERATED_BODY()

public:
	ATDPickup();

	void Init(ETDPickupType InType, float InLifetime, float InBlinkSeconds);
	ETDPickupType GetType() const { return Type; }
	float GetAge() const { return Age; }
	float GetLifetime() const { return Lifetime; }
	bool IsExpired() const { return Age >= Lifetime; }
	/** Game time: advanced by the subsystem (a retake clears them anyway). Spin, bob and the end blink. */
	void Advance(float Dt);

	/** The type's colour (never a squad or enemy colour) and HUD word. */
	static FLinearColor ColorOf(ETDPickupType Type);
	static const TCHAR* WordOf(ETDPickupType Type);

	/** Visible size (cm across): readable at range, small next to a ship. */
	static constexpr float Size = 340.f;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Spinner;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Glow;
	/** The effects builder's loop (DA_Pickup_Visuals), when there is one: then no primitive shapes. */
	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> LoopFX;
	bool bArtLook = false;

public:
	/** True when the pickup shows DA_Pickup_Visuals' loop instead of the primitive shapes (tests). */
	bool HasArtLook() const { return bArtLook; }

private:

	UStaticMeshComponent* AddPart(UStaticMesh* Mesh, const FVector& Offset, const FRotator& Rotation, const FVector& Scale);

	ETDPickupType Type = ETDPickupType::Hull;
	float Age = 0.f;
	float Lifetime = 20.f;
	float BlinkSeconds = 4.f;
	FVector Home = FVector::ZeroVector;
};

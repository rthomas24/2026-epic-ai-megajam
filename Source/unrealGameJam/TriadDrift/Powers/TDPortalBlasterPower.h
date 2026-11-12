// Triad Drift - Powers (Teammate 2). Rift: Portal Blaster (ENG-09) with the "Blink" fallback.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDPortalTransit.h"
#include "TDPortalBlasterPower.generated.h"

class UTDTravelFX;
class UNiagaraComponent;

/**
 * One end of a Rift portal pair: a two-sided disc. Every tick it tests the swept motion of every
 * spacecraft in the combat registry and every live projectile against its plane (no trigger volume,
 * nothing tunnels). A crossing within Radius sends the traveller to the partner via UTDPortalTransit.
 */
UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDPortal : public AActor
{
	GENERATED_BODY()

public:
	ATDPortal();

	void Init(AActor* InInstigator, float InRadius, const FTDPortalTransitParams& InParams, UTDTravelFX* InFX);
	void SetPartner(ATDPortal* InPartner) { Partner = InPartner; }
	void Close();

	/** Tell this portal a traveller just appeared at Location, so its next motion isn't seen as a crossing. */
	void NotifyArrived(const AActor* Traveller, const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Portal")
	ATDPortal* GetPartner() const { return Partner.Get(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Portal")
	float GetRadius() const { return Radius; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Portal")
	int32 GetTransitCount() const { return TransitCount; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool TryCross(AActor* Traveller, const FVector& Prev, const FVector& Cur);
	void BroadcastState(ETDPortalState State);

	UPROPERTY(Transient)
	TObjectPtr<UTDTravelFX> FX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> IdleFX;

	TWeakObjectPtr<ATDPortal> Partner;
	TWeakObjectPtr<AActor> PortalInstigator;
	TMap<TWeakObjectPtr<const AActor>, FVector> LastPositions;
	FTDPortalTransitParams Params;
	float Radius = 600.f;
	int32 TransitCount = 0;
	bool bClosed = false;
};

/**
 * Q: first press opens portal A where the aim sweep hits (or at 12 000 cm), the second opens B, a third
 * recycles the oldest. The pair stays open for 12 s from the first press, then both close and the 8 s
 * cooldown starts. bUseBlinkFallback (or console "TD.PortalBlink 1") swaps in Blink: teleport to the
 * impact point and drop a 3 s missile-reflect field.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDPortalBlasterPower : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	UTDPortalBlasterPower();

	virtual void HandleInputPressed() override;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Portal")
	bool IsUsingBlink() const;

	const TArray<TWeakObjectPtr<ATDPortal>>& GetPortals() const { return Portals; }

	/** Where the next portal (or blink) would land: sweep from the owner along the aim direction. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Portal")
	FVector ComputePlacement(FVector& OutFacing) const;

	// --- Tuning (ENG-09) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal", meta = (Units = "cm"))
	float MaxRange = 12000.f;

	/** Portals open this far off the surface the shot hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal", meta = (Units = "cm"))
	float SurfaceOffset = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal", meta = (Units = "cm"))
	float PortalRadius = 600.f;

	/** How long the pair stays open, from the first press. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal", meta = (Units = "s"))
	float WindowSeconds = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal")
	FTDPortalTransitParams TransitParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Portal")
	TObjectPtr<UTDTravelFX> TravelFX = nullptr;

	// --- Fallback: Blink ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Blink")
	bool bUseBlinkFallback = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Blink", meta = (Units = "cm"))
	float BlinkSurfaceOffset = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Blink", meta = (Units = "s"))
	float BlinkReflectSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Blink", meta = (Units = "cm"))
	float BlinkReflectRadius = 1500.f;

protected:
	virtual bool OnActivate() override;
	virtual void OnTickActive(float DeltaTime) override;
	virtual void OnEnd(bool bInterrupted) override;

private:
	ATDPortal* PlacePortal();
	bool DoBlink();

	TArray<TWeakObjectPtr<ATDPortal>> Portals;
};

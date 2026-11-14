// Triad Drift - World (Teammate 4). ENG-18 Dominion shield generator.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TDShieldGenerator.generated.h"

class UStaticMeshComponent;
class UTDHealthComponent;
class UTDDominionVisuals;
class ATDShieldGenerator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDGeneratorSignature, ATDShieldGenerator*, Generator, AActor*, Killer);

/**
 * One of the Dominion's four shield generators (800 HP). Sits on a pylon outside the main shield so
 * it can be shot while the shield is up. Destroying it drops one shield section.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDShieldGenerator : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	/**
	 * Height (cm) of the actor (the pylon's centre, where aim assist, lead and the director aim) above the art pylon's
	 * base, which sits on its plinth socket; 0 for the placeholder, whose pivot is its centre.
	 */
	float GetArtCentreHeight() const { return ArtCentreHeight; }

	ATDShieldGenerator();

	void Init(float Hull, int32 InSectionIndex);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	int32 GetSectionIndex() const { return SectionIndex; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	UTDHealthComponent* GetHealth() const { return Health; }

	/** Destroyed state without a kill (resume from a save where defenses are already down). */
	void SetDestroyedSilently();

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Dominion")
	FTDGeneratorSignature OnGeneratorDestroyed;

	/** Explosion etc. (Nick). Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDDominionVisuals> Visuals;

	virtual ETDTeam GetTeam() const override { return ETDTeam::Enemy; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	UFUNCTION()
	void HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);

	void ShowDestroyed();

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Art pylon only: its base sits this far (cm) below the actor, which stays at the pylon's centre. */
	float ArtCentreHeight = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UTDHealthComponent> Health;

	int32 SectionIndex = 0;
	bool bDestroyed = false;
	bool bHalfSaid = false;
};

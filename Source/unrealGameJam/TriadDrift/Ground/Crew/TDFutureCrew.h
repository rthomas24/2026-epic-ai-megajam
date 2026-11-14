// Triad Drift - the future crew in the reactor room (Nicholas, N-052 / N-060, issue #29). Owner: Teammate 5.
//
// All three future pilots stand in the reactor room whichever pilot boarded. The two who are not the boarder defend it
// (ETDCrewType::FutureCrew, ordinary crew with the pilot's look and a blade). The boarder's own future self is staged:
// present, idle, facing the player, not targetable, and it fires the personal exchange once (T3's ids, Nicholas's words).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDFutureCrew.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UCapsuleComponent;
class UAnimSequenceBase;
class UTDPilotVisuals;

/** The shared future-pilot look: the pilot rig, its loops, a blade on the right hand in the pilot's squad colour. */
namespace TDFutureCrew
{
	/** Wren (Rift), Kato (Lancer), Moss (Bastion). */
	UNREALGAMEJAM_API FName PilotOf(ETDShipId Ship);
	UNREALGAMEJAM_API ETDShipId ShipOf(FName Pilot);
	/** Squad colours: Rift magenta, Lancer cyan, Bastion amber. */
	UNREALGAMEJAM_API FLinearColor ColorOf(FName Pilot);
	/** The other two pilots, in squad order. */
	UNREALGAMEJAM_API TArray<FName> OthersThan(FName Pilot);
	/** A pilot animation by name (Gameplay/Characters/Pilot/Anims/<Name>), or null. */
	UNREALGAMEJAM_API UAnimSequenceBase* PilotAnim(const TCHAR* Name);
	/** Puts DA_Pilot_Visuals on Mesh (feet at -HalfHeight) with the C++ loop player. Null when there is no pilot look. */
	UNREALGAMEJAM_API const UTDPilotVisuals* ApplyPilotLook(USkeletalMeshComponent* Mesh, float CapsuleHalfHeight);
	/** A hilt and a lit beam on hand_r (or BladeSocket), tinted Color. Returns the beam (null without a look). */
	UNREALGAMEJAM_API UStaticMeshComponent* AttachBlade(AActor* Owner, USkeletalMeshComponent* Mesh, const FLinearColor& Color);
}

DECLARE_MULTICAST_DELEGATE_TwoParams(FTDOnFutureSelfExchange, AActor* /*FutureSelf*/, FName /*Pilot*/);

/**
 * The boarding pilot's own future self in the reactor room: present, idle, turned toward the player, never a target
 * (no health, ignored by traces and the blade), solid to walk into. When the player first comes within ExchangeRadius
 * it fires the exchange once: OnAnyExchange (T3's hook) and the story id for that pilot.
 */
UCLASS()
class UNREALGAMEJAM_API ATDFutureSelf : public AActor
{
	GENERATED_BODY()

public:
	ATDFutureSelf();

	/** Wren, Kato or Moss. Set before BeginPlay (SetPilot). */
	void SetPilot(FName InPilot);
	FName GetPilot() const { return Pilot; }

	/** The story id for this pilot's exchange: FutureSelf_<Pilot> (T3 maps it to Nicholas's lines). */
	FName GetExchangeId() const;
	bool HasExchanged() const { return bExchanged; }
	/** Re-arm on a retake of the reactor scene. */
	void ResetExchange() { bExchanged = false; }

	UPROPERTY(EditAnywhere, Category = "Future Self", meta = (Units = "cm")) float ExchangeRadius = 900.f;
	UPROPERTY(EditAnywhere, Category = "Future Self", meta = (Units = "deg/s")) float TurnRate = 120.f;

	static FTDOnFutureSelfExchange OnAnyExchange;

	USkeletalMeshComponent* GetLook() const { return Look; }
	UCapsuleComponent* GetCapsule() const { return Capsule; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Future Self") TObjectPtr<UCapsuleComponent> Capsule;
	UPROPERTY(VisibleAnywhere, Category = "Future Self") TObjectPtr<USkeletalMeshComponent> Look;
	/** No pilot look on disk: a tinted block so the beat still reads. */
	UPROPERTY(VisibleAnywhere, Category = "Future Self") TObjectPtr<UStaticMeshComponent> Placeholder;

private:
	FName Pilot = TEXT("Wren");
	bool bExchanged = false;
};

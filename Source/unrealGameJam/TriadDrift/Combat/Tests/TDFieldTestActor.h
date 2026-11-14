// Triad Drift - a stand-in destructible field for the ITDFieldDamageable tests (Teammate 2).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "TDFieldTestActor.generated.h"

UCLASS(NotPlaceable, Transient, HideDropdown)
class ATDFieldTestActor : public AActor, public ITDFieldDamageable
{
	GENERATED_BODY()

public:
	ATDFieldTestActor()
	{
		Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
		Box->SetBoxExtent(FVector(300.f));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box->SetCollisionResponseToChannel(TDCollision::Projectile, ECR_Block);
		SetRootComponent(Box);
	}

	virtual void ReceiveProjectileHit(const FHitResult& Hit, float Damage, AActor* InInstigator, ETDDamageKind Kind) override
	{
		++ProjectileHits;
		LastDamage = Damage;
		LastHitComponent = Hit.GetComponent();
	}

	virtual bool ReceiveRamSweep(const FVector& From, const FVector& To, float Radius, AActor* InInstigator, FVector& OutBlock) override
	{
		// Touched when the swept capsule reaches the box.
		if (FMath::PointDistToSegment(GetActorLocation(), From, To) > Radius + 300.f)
		{
			return false;
		}
		++RamSweeps;
		OutBlock = GetActorLocation();
		return bBlockRam;
	}

	virtual void ReceiveSplash(const FVector& Center, float Radius, float Damage, AActor* InInstigator) override
	{
		if (FVector::Dist(Center, GetActorLocation()) <= Radius + 300.f)
		{
			++Splashes;
		}
	}

	UPROPERTY()
	TObjectPtr<UBoxComponent> Box;
	int32 Splashes = 0;
	int32 ProjectileHits = 0;
	float LastDamage = 0.f;
	TWeakObjectPtr<UPrimitiveComponent> LastHitComponent;
	int32 RamSweeps = 0;
	bool bBlockRam = false;
};

// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDDominionReactor.h"

#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATDDominionReactor::ATDDominionReactor()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.AddUnique(TDDirectorTags::Reactor);	// ReactorKill money shot (C-03 #2)

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetRelativeScale3D(FVector(25.f));	// 25 m core
	if (Sphere.Succeeded()) { Mesh->SetStaticMesh(Sphere.Object); }

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->DefaultTeam = ETDTeam::Enemy;
	Health->MaxShield = 0.f;
	Health->bTargetable = false;
}

void ATDDominionReactor::BeginPlay()
{
	Super::BeginPlay();
	if (Health)
	{
		Health->InitMaxValues(PhaseHull * PhaseCount, 0.f, true);
		Health->SetTargetable(bExposed);
		Health->DamageFilter.BindUObject(this, &ATDDominionReactor::FilterDamage);
		Health->OnDamaged.AddUniqueDynamic(this, &ATDDominionReactor::HandleDamaged);
		Health->OnDeath.AddUniqueDynamic(this, &ATDDominionReactor::HandleDeath);
	}
	if (Visuals && !Visuals->ReactorMesh.IsNull())
	{
		if (UStaticMesh* Custom = Visuals->ReactorMesh.LoadSynchronous())
		{
			Mesh->SetStaticMesh(Custom);
			Mesh->SetRelativeScale3D(FVector::OneVector);
		}
	}
	else if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.85f, 0.2f));
	}
}

void ATDDominionReactor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Health)
	{
		Health->DamageFilter.Unbind();
		Health->OnDamaged.RemoveDynamic(this, &ATDDominionReactor::HandleDamaged);
		Health->OnDeath.RemoveDynamic(this, &ATDDominionReactor::HandleDeath);
	}
	Super::EndPlay(EndPlayReason);
}

void ATDDominionReactor::SetExposed(bool bInExposed, bool bPlayCue)
{
	bExposed = bInExposed;
	if (Health)
	{
		Health->SetTargetable(bExposed && !bDestroyed);
	}
	if (bExposed && Visuals && bPlayCue)
	{
		UTDStatics::PlayCue(this, Visuals->ReactorExposed, GetActorLocation(), GetActorRotation());
	}
}

bool ATDDominionReactor::IsAlive() const
{
	return !bDestroyed && Health && Health->IsAlive();
}

float ATDDominionReactor::GetPhaseHealth01() const
{
	if (!Health || bDestroyed)
	{
		return 0.f;
	}
	const float PhaseFloor = PhaseHull * (PhaseCount - Phase);
	return FMath::Clamp((Health->GetHull() - PhaseFloor) / PhaseHull, 0.f, 1.f);
}

float ATDDominionReactor::GetTotalHealth01() const
{
	return (Health && !bDestroyed) ? Health->GetHull01() : 0.f;
}

float ATDDominionReactor::FilterDamage(const FTDDamageInfo& Info)
{
	// The Dominion's main shield: nothing gets through until the generators are down.
	if (!bExposed || bDestroyed || !Health)
	{
		const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		if (!bExposed && !bDestroyed && Visuals && Now - LastShieldHitCueTime > 0.1)
		{
			LastShieldHitCueTime = Now;
			UTDStatics::PlayCue(this, Visuals->ShieldHit, Info.HitLocation.IsNearlyZero() ? GetActorLocation() : Info.HitLocation);
		}
		return 0.f;
	}
	// One phase per hit: damage stops at the phase boundary so every phase change reads.
	if (Phase < PhaseCount)
	{
		const float PhaseFloor = PhaseHull * (PhaseCount - Phase);
		return FMath::Min(Info.Amount, FMath::Max(0.f, Health->GetHull() - PhaseFloor));
	}
	return Info.Amount;
}

void ATDDominionReactor::HandleDamaged(UTDHealthComponent* /*InHealth*/, const FTDDamageInfo& Info, float /*ShieldDamage*/, float HullDamage)
{
	if (bDestroyed || !Health || HullDamage <= 0.f)
	{
		return;
	}
	if (Visuals)
	{
		UTDStatics::PlayCue(this, Visuals->ReactorHit, Info.HitLocation.IsNearlyZero() ? GetActorLocation() : Info.HitLocation);
	}
	const float PhaseFloor = PhaseHull * (PhaseCount - Phase);
	if (Phase < PhaseCount && Health->GetHull() <= PhaseFloor + KINDA_SMALL_NUMBER)
	{
		++Phase;
		Health->SetInvulnerableFor(PhaseChangeInvulnerableSeconds);
		if (Visuals)
		{
			UTDStatics::PlayCue(this, Visuals->ReactorPhaseChange, GetActorLocation(), GetActorRotation());
		}
		OnPhaseChanged.Broadcast(this, Phase);
	}
}

void ATDDominionReactor::HandleDeath(UTDHealthComponent* /*InHealth*/, const FTDDamageInfo& KillingBlow)
{
	if (bDestroyed)
	{
		return;
	}
	bDestroyed = true;
	Phase = PhaseCount + 1;
	if (Health) { Health->SetTargetable(false); }
	if (Visuals)
	{
		UTDStatics::PlayCue(this, Visuals->ReactorDestroyed, GetActorLocation(), GetActorRotation());
	}
	OnReactorDestroyed.Broadcast(this, KillingBlow.Instigator);
}

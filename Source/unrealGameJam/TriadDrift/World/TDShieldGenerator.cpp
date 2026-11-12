// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDShieldGenerator.h"

#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATDShieldGenerator::ATDShieldGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.AddUnique(TDDirectorTags::Generator);	// GeneratorKill money shot (C-03 #2)

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pylon"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetRelativeScale3D(FVector(12.f, 12.f, 20.f));	// 12 m wide, 20 m tall
	if (Cylinder.Succeeded()) { Mesh->SetStaticMesh(Cylinder.Object); }

	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
	Core->SetupAttachment(Mesh);
	Core->SetCollisionProfileName(TEXT("NoCollision"));
	Core->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Core->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.5f));
	if (Sphere.Succeeded()) { Core->SetStaticMesh(Sphere.Object); }

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->DefaultTeam = ETDTeam::Enemy;
	Health->MaxHull = 800.f;
	Health->MaxShield = 0.f;
}

void ATDShieldGenerator::Init(float Hull, int32 InSectionIndex)
{
	SectionIndex = InSectionIndex;
	if (Health)
	{
		Health->InitMaxValues(FMath::Max(1.f, Hull), 0.f, true);
	}
	if (Visuals && !Visuals->GeneratorMesh.IsNull())
	{
		if (UStaticMesh* Custom = Visuals->GeneratorMesh.LoadSynchronous())
		{
			Mesh->SetStaticMesh(Custom);
			Mesh->SetRelativeScale3D(FVector::OneVector);
			Core->SetVisibility(false);
		}
	}
	else if (UMaterialInstanceDynamic* MID = Core->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.3f, 0.8f, 1.f));	// glowing weak point
	}
}

void ATDShieldGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (Health)
	{
		Health->OnDeath.AddUniqueDynamic(this, &ATDShieldGenerator::HandleDeath);
		Health->OnDamaged.AddUniqueDynamic(this, &ATDShieldGenerator::HandleDamaged);
	}
}

void ATDShieldGenerator::HandleDamaged(UTDHealthComponent* /*InHealth*/, const FTDDamageInfo& Info, float /*ShieldDamage*/, float HullDamage)
{
	if (Visuals && HullDamage > 0.f && !bDestroyed)
	{
		UTDStatics::PlayCue(this, Visuals->GeneratorHit, Info.HitLocation.IsNearlyZero() ? GetActorLocation() : Info.HitLocation);
	}
}

void ATDShieldGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Health)
	{
		Health->OnDeath.RemoveDynamic(this, &ATDShieldGenerator::HandleDeath);
		Health->OnDamaged.RemoveDynamic(this, &ATDShieldGenerator::HandleDamaged);
	}
	Super::EndPlay(EndPlayReason);
}

bool ATDShieldGenerator::IsAlive() const
{
	return !bDestroyed && Health && Health->IsAlive();
}

void ATDShieldGenerator::ShowDestroyed()
{
	bDestroyed = true;
	if (Core)
	{
		Core->SetVisibility(false);
	}
	if (Mesh)
	{
		// Leave a stump: the pylon stays as cover, the core is gone.
		Mesh->SetRelativeScale3D(Mesh->GetRelativeScale3D() * FVector(1.f, 1.f, 0.4f));
	}
}

void ATDShieldGenerator::SetDestroyedSilently()
{
	if (bDestroyed)
	{
		return;
	}
	// No Kill(): a resume must not count as kills or fire death events. The health stays "alive"
	// but untargetable and invulnerable, and IsAlive() reports false via bDestroyed.
	if (Health)
	{
		Health->SetTargetable(false);
		Health->SetInvulnerable(true);
	}
	if (Mesh) { Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }
	ShowDestroyed();
}

void ATDShieldGenerator::HandleDeath(UTDHealthComponent* /*InHealth*/, const FTDDamageInfo& KillingBlow)
{
	if (bDestroyed)
	{
		return;
	}
	ShowDestroyed();
	if (Visuals)
	{
		UTDStatics::PlayCue(this, Visuals->GeneratorExplosion, GetActorLocation(), GetActorRotation());
	}
	OnGeneratorDestroyed.Broadcast(this, KillingBlow.Instigator);
}

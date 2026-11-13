// Triad Drift - World (Teammate 4). ENG-31 escape flight debris.
#include "TriadDrift/World/TDEscapeDebris.h"

#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDSpacecraft.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ATDEscapeDebris::ATDEscapeDebris()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chunk"));
	SetRootComponent(Mesh);
	if (Cube.Succeeded()) { Mesh->SetStaticMesh(Cube.Object); }
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

void ATDEscapeDebris::Launch(float SizeCm, const FVector& InVelocity, const FRotator& InSpin)
{
	Velocity = InVelocity;
	Spin = InSpin;
	Radius = SizeCm * 0.6f;
	Mesh->SetRelativeScale3D(FVector(SizeCm, SizeCm * 0.6f, SizeCm * 0.35f) / 100.f);
	// Dark armour with a burning edge: reads against the bright breakup.
	UTDStatics::TintPlaceholder(Mesh, FLinearColor(0.15f, 0.04f, 0.01f), 3.f);
}

void ATDEscapeDebris::Burst()
{
	if (UTDProjectileSubsystem* Booms = UTDProjectileSubsystem::Get(this))
	{
		Booms->PlayPlaceholderExplosion(GetActorLocation(), Radius * 1.5f, FLinearColor(1.f, 0.45f, 0.1f), 4);
	}
	Destroy();
}

void ATDEscapeDebris::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (Age >= LifeSeconds)
	{
		Destroy();
		return;
	}
	AddActorWorldOffset(Velocity * DeltaSeconds);
	AddActorLocalRotation(Spin * DeltaSeconds);
	if (bHit)
	{
		return;
	}
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Registry)
	{
		return;
	}
	TArray<AActor*> Squad;
	Registry->GetActorsOfTeam(ETDTeam::Player, Squad, true);
	for (AActor* A : Squad)
	{
		const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(A);
		if (!Craft || Craft->IsHidden())
		{
			continue;
		}
		float ShipRadius = 0.f, HalfHeight = 0.f;
		Craft->GetSimpleCollisionCylinder(ShipRadius, HalfHeight);
		if (FVector::DistSquared(Craft->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius + FMath::Max(ShipRadius, 300.f)))
		{
			bHit = true;
			UTDStatics::ApplyTDDamage(A, Damage, ETDDamageKind::Environmental, this, this);
			Burst();
			return;
		}
	}
}

// Triad Drift - Waves (Teammate 4).
#include "TriadDrift/Waves/TDWarpInMarker.h"

#include "TriadDrift/Core/TDStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATDWarpInMarker::ATDWarpInMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	SetRootComponent(Ring);
	Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ring->SetGenerateOverlapEvents(false);
	Ring->SetCastShadow(false);
	if (Cylinder.Succeeded()) { Ring->SetStaticMesh(Cylinder.Object); }
	if (Glow.Succeeded()) { GlowMaterial = Glow.Object; }
	SetActorEnableCollision(false);
}

void ATDWarpInMarker::Begin(float Seconds, float Size, const FVector& FaceToward)
{
	Duration = FMath::Max(0.1f, Seconds);
	FullSize = Size;
	Elapsed = 0.f;
	// Cylinder axis (Z) toward the viewer: a disc facing the player.
	const FVector Dir = (FaceToward - GetActorLocation()).GetSafeNormal();
	if (!Dir.IsNearlyZero()) { SetActorRotation(FRotationMatrix::MakeFromZ(Dir).Rotator()); }
	UTDStatics::TintPlaceholder(Ring, FLinearColor(1.f, 0.12f, 0.05f), 4.f);
	Tick(0.f);
}

void ATDWarpInMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	const float T = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
	const float D = FullSize / 100.f * FMath::Lerp(0.2f, 1.f, T);	// 100 cm cylinder
	Ring->SetRelativeScale3D(FVector(D, D, 0.03f));
	if (Elapsed >= Duration + 0.15f)
	{
		Destroy();
	}
}

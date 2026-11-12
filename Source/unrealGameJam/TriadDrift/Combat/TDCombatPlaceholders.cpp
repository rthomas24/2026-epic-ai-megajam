// Triad Drift - Combat (Teammate 2). Placeholder streaks and flashes.

#include "TriadDrift/Combat/TDCombatPlaceholders.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"

FTDStreakLook FTDCombatPlaceholders::ComputeStreak(const FTDProjectileSpec& Spec)
{
	FTDStreakLook Look;
	Look.bStreak = Spec.Speed >= 2000.f;
	Look.bHeavy = Spec.bIsMissile || Spec.bHoming || Spec.SplashRadius > 0.f || Spec.PlaceholderScale.Y >= 0.5f;
	// Missiles and heavy shells carry a hot head: the slow heavy bolt must outweigh a needle on screen.
	Look.bHead = Spec.bHoming || Spec.bIsMissile || Look.bHeavy;
	const float Width = FMath::Max(Spec.PlaceholderScale.Y, Spec.PlaceholderScale.Z) * 100.f;
	if (Look.bHeavy)
	{
		Look.Length = FMath::Clamp(Spec.Speed * 0.015f, 250.f, 500.f);
		// Slim missiles (the 12-missile swarm) stay slimmer than shells: packed at launch, fat heads merged into a blob.
		Look.Thickness = FMath::Clamp(Width * 1.4f, Spec.bIsMissile ? 40.f : 70.f, 140.f);
	}
	else
	{
		Look.Length = FMath::Clamp(Spec.Speed * 0.015f, 300.f, 800.f);
		Look.Thickness = FMath::Clamp(Width * 2.5f, 35.f, 60.f);
	}
	Look.HeadDiameter = Look.bHead ? Look.Thickness * (Spec.bIsMissile ? 1.4f : 1.8f) : 0.f;
	return Look;
}

bool FTDCombatPlaceholders::HasVisualArt(const FTDFXCue& Cue)
{
	return !Cue.VFX.IsNull();
}

float FTDCombatPlaceholders::DistanceGrowth(const FTDStreakLook& Look, float DistanceToCamera)
{
	if (Look.Thickness <= 0.f)
	{
		return 1.f;
	}
	// Pixels across = Thickness * 1280 / (2 * Distance * tan(45 deg)).
	const float Needed = (Look.bHeavy ? MinHeavyStreakPixels : MinStreakPixels) * 2.f * DistanceToCamera / 1280.f;
	return FMath::Clamp(Needed / Look.Thickness, 1.f, MaxDistanceGrowth);
}

float FTDCombatPlaceholders::CapMuzzleFlashRadius(float Radius, float HullRadius)
{
	return HullRadius > 0.f ? FMath::Min(Radius, HullRadius / 3.6f) : Radius;
}

float FTDCombatPlaceholders::MuzzleFlashRadius(const FTDProjectileSpec& Spec)
{
	const FTDStreakLook Look = ComputeStreak(Spec);
	return FMath::Clamp(Look.Thickness * 1.6f, 60.f, 180.f);
}

float FTDCombatPlaceholders::ImpactFlashRadius(const FTDProjectileSpec& Spec)
{
	if (Spec.SplashRadius > 0.f)
	{
		return FMath::Clamp(Spec.SplashRadius * 0.35f, 220.f, 900.f);
	}
	return ComputeStreak(Spec).bHeavy ? 200.f : 140.f;
}

// ------------------------------------------------------------------------------------------ flash actor

ATDCombatFlash::ATDCombatFlash()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flash"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->bReceivesDecals = false;
	Mesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		SphereMesh = Sphere.Object;
		Mesh->SetStaticMesh(Sphere.Object);
	}
}

void ATDCombatFlash::Play(const FVector& Location, float InRadius, const FLinearColor& InColor, float Seconds, USceneComponent* AttachTo)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(Location);
	if (AttachTo)
	{
		AttachToComponent(AttachTo, FAttachmentTransformRules::KeepWorldTransform);
	}
	if (!GlowMaterial)
	{
		GlowMaterial = UTDStatics::GetPlaceholderGlowMaterial(); // project M_TD_PlaceholderGlow, always cooked
	}
	if (!MID && GlowMaterial)
	{
		MID = Mesh->CreateDynamicMaterialInstance(0, GlowMaterial);
	}
	Radius = FMath::Max(InRadius, 1.f);
	Color = InColor;
	Life = FMath::Max(Seconds, 0.01f);
	Age = 0.f;
	bPlaying = true;
	Mesh->SetVisibility(true);
	SetActorTickEnabled(true);
	Apply(0.f);
}

void ATDCombatFlash::Stop()
{
	bPlaying = false;
	SetActorTickEnabled(false);
	Mesh->SetVisibility(false);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ATDCombatFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bPlaying)
	{
		return;
	}
	Age += DeltaSeconds;
	if (Age >= Life)
	{
		Stop();
		return;
	}
	Apply(Age / Life);
}

void ATDCombatFlash::Apply(float Alpha)
{
	// Swells from 60% to 120% of its radius while the glow falls off.
	const float Size = Radius * (0.6f + 0.6f * Alpha);
	SetActorScale3D(FVector(Size / 50.f)); // the engine sphere is 100 cm across
	if (MID)
	{
		const FLinearColor Glow = Color * (FTDCombatPlaceholders::FlashGlow * FMath::Square(1.f - Alpha));
		MID->SetVectorParameterValue(TEXT("Color"), Glow);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Glow);
	}
}

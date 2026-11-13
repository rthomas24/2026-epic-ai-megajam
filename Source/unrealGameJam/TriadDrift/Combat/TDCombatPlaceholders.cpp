// Triad Drift - Combat (Teammate 2). Placeholder streaks and flashes.

#include "TriadDrift/Combat/TDCombatPlaceholders.h"

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
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

FTDStreakLook FTDCombatPlaceholders::ComputeOnFootStreak(const FTDProjectileSpec& Spec)
{
	FTDStreakLook Look;
	Look.bStreak = Spec.Speed >= 600.f;
	Look.bHeavy = false;
	Look.bHead = true; // a hot core so the bolt reads as a bolt, not a stick
	// Heavy pellets (r16, 2600 cm/s): 6 cm x 52 cm. Trooper bolt (r22, 2200): 8 x 44. Drone (r14, 4000): 5 x 80.
	Look.Length = FMath::Clamp(Spec.Speed * 0.02f, 40.f, 80.f);
	Look.Thickness = FMath::Clamp(Spec.CollisionRadius * 0.35f, 5.f, 9.f);
	Look.HeadDiameter = Look.Thickness * 1.4f;
	return Look;
}

float FTDCombatPlaceholders::OnFootDistanceGrowth(const FTDStreakLook& Look, float DistanceToCamera)
{
	if (Look.Thickness <= 0.f)
	{
		return 1.f;
	}
	const float Needed = MinOnFootStreakPixels * 2.f * DistanceToCamera / 1280.f;
	return FMath::Clamp(Needed / Look.Thickness, 1.f, OnFootMaxGrowth);
}

bool FTDCombatPlaceholders::IsArmadaHue(const FLinearColor& Color)
{
	const FLinearColor HSV = Color.LinearRGBToHSV();
	return HSV.R <= 45.f && HSV.G >= 0.6f && HSV.B > 0.05f;
}

FLinearColor FTDCombatPlaceholders::BladeColorOf(const AActor* Shooter)
{
	const UTDEnergyBladeComponent* Blade = Shooter ? Shooter->FindComponentByClass<UTDEnergyBladeComponent>() : nullptr;
	const UTDBladeFX* FX = Blade ? Blade->BladeFX.Get() : nullptr;
	return FX ? FX->BladeColor : FLinearColor(0.1f, 0.65f, 1.f); // ATDFirstPersonCharacter::ApplyBladeVisuals' default
}

FLinearColor FTDCombatPlaceholders::ProjectileColor(const FTDProjectileSpec& Spec, ETDTeam Team, const AActor* Shooter, bool bDeflected)
{
	if (bDeflected)
	{
		if (const ATDShip* Ship = Cast<ATDShip>(Shooter))
		{
			return UTDStatics::ShipColor(Ship->GetShipId()); // Bastion's dome, Rift's portal
		}
		if (Team == ETDTeam::Enemy)
		{
			return UTDStatics::TeamColor(ETDTeam::Enemy);
		}
		return BladeColorOf(Shooter);
	}
	if (Spec.FX && !Spec.FX->Color.Equals(FLinearColor::White))
	{
		return Spec.FX->Color; // Nick's colour wins
	}
	if (Team == ETDTeam::Enemy)
	{
		return IsArmadaHue(Spec.PlaceholderColor) ? Spec.PlaceholderColor : UTDStatics::TeamColor(ETDTeam::Enemy);
	}
	if (const ATDShip* Ship = Cast<ATDShip>(Shooter))
	{
		return UTDStatics::ShipColor(Ship->GetShipId());
	}
	return Spec.PlaceholderColor;
}

bool FTDCombatPlaceholders::HasVisualArt(const FTDFXCue& Cue)
{
	return !Cue.VFX.IsNull();
}

float FTDCombatPlaceholders::DistanceGrowth(const FTDStreakLook& Look, float DistanceToCamera, float MinPixels)
{
	if (Look.Thickness <= 0.f)
	{
		return 1.f;
	}
	const float Needed = MinPixels * 2.f * DistanceToCamera / 1280.f;
	return FMath::Clamp(Needed / Look.Thickness, 1.f, MaxDistanceGrowth * 2.5f);
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
	bDebris = false;
	SetActorRotation(FRotator::ZeroRotator);
	Radius = FMath::Max(InRadius, 1.f);
	Color = InColor;
	Life = FMath::Max(Seconds, 0.01f);
	Age = 0.f;
	bPlaying = true;
	Mesh->SetVisibility(true);
	SetActorTickEnabled(true);
	Apply(0.f);
}

void ATDCombatFlash::PlayDebris(const FVector& Location, const FVector& Velocity, float Length, float Thickness, const FLinearColor& InColor,
	float Seconds)
{
	Play(Location, Thickness * 0.5f, InColor, Seconds, nullptr);
	bDebris = true;
	DebrisVelocity = Velocity;
	DebrisLength = FMath::Max(Length, Thickness);
	SetActorRotation(Velocity.Rotation());
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
	if (bDebris)
	{
		SetActorLocation(GetActorLocation() + DebrisVelocity * DeltaSeconds);
		DebrisVelocity *= FMath::Exp(-1.5f * DeltaSeconds); // drag: streaks slow as they fade
	}
	Apply(Age / Life);
}

void ATDCombatFlash::Apply(float Alpha)
{
	if (bDebris)
	{
		// A stretched streak along its flight, thinning as it fades.
		const float Shrink = 1.f - 0.6f * Alpha;
		SetActorScale3D(FVector(DebrisLength * Shrink, Radius * 2.f * Shrink, Radius * 2.f * Shrink) / 100.f);
	}
	else
	{
		// Swells from 60% to 120% of its radius in the first 30%, then collapses as the glow falls off. The glow material
		// is opaque unlit, so a fading colour reads darker, not clearer: a shell that kept its size turned into a dark
		// disc near the camera (T3's death playtest). Collapsing, the dim end is small.
		float Size = Alpha < 0.3f ? Radius * (0.6f + 2.f * Alpha) : FMath::Max(Radius * 1.2f * FMath::Square(1.f - (Alpha - 0.3f) / 0.7f), 1.f);
		// Never fills the frame: at most MaxRadiusPerCameraDistance of its distance to the camera (the Lead's frame-10
		// white-out; T3's death shell peaks at 0.24, under the cap).
		const UWorld* World = GetWorld();
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerCameraManager)
		{
			const float Distance = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), GetActorLocation());
			Size = FMath::Min(Size, FMath::Max(Distance * MaxRadiusPerCameraDistance, 1.f));
		}
		SetActorScale3D(FVector(Size / 50.f)); // the engine sphere is 100 cm across
	}
	if (MID)
	{
		const FLinearColor Glow = Color * (FTDCombatPlaceholders::FlashGlow * FMath::Square(1.f - Alpha));
		MID->SetVectorParameterValue(TEXT("Color"), Glow);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Glow);
	}
}

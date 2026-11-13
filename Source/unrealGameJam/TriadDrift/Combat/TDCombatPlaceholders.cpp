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
#include "Misc/PackageName.h"
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

FTDStreakLook FTDCombatPlaceholders::ComputeSquadStreak(const FTDProjectileSpec& Spec)
{
	FTDStreakLook Look = ComputeStreak(Spec);
	if (!Look.bStreak)
	{
		return Look;
	}
	if (Look.bHeavy)
	{
		Look.HeadDiameter = Look.bHead ? Look.Thickness * 1.15f : 0.f;
	}
	else
	{
		Look.Length = FMath::Clamp(Spec.Speed * SquadTracerSeconds, Look.Length, MaxSquadTracerCm);
	}
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		CubeMesh = Cube.Object;
	}
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
	bStyled = false;
	DelayLeft = 0.f;
	if (bChunk)
	{
		bChunk = false;
		if (SphereMesh)
		{
			Mesh->SetStaticMesh(SphereMesh);
		}
	}
	if (MID && Mesh->GetMaterial(0) != MID)
	{
		Mesh->SetMaterial(0, MID);
	}
	CameraCap = MaxRadiusPerCameraDistance;
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

UMaterialInterface* ATDCombatFlash::GetSmokeMaterial()
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	static bool bLooked = false;
	if (!Cached.IsValid() && !bLooked)
	{
		bLooked = true;
		static const TCHAR* Path = TEXT("/Game/TriadDrift/Gameplay/Materials/M_TD_SmokePuff.M_TD_SmokePuff");
		if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path))))
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}
	return Cached.Get();
}

void ATDCombatFlash::PlayChunk(const FVector& Location, const FVector& Velocity, float Size, UMaterialInterface* HullMaterial, float Seconds,
	const FVector& AngularDegPerSec)
{
	Play(Location, Size * 0.5f, FLinearColor(0.03f, 0.03f, 0.035f), Seconds, nullptr);
	bChunk = true;
	bDebris = false;
	ChunkSize = FMath::Max(Size, 1.f);
	// Hull plates and shards (live, equal cubes read as toy blocks).
	ChunkAspect = FVector(1.f, FMath::FRandRange(0.45f, 0.8f), FMath::FRandRange(0.12f, 0.3f));
	DebrisVelocity = Velocity;
	ChunkSpin = FRotator(AngularDegPerSec.Y, AngularDegPerSec.Z, AngularDegPerSec.X);
	if (CubeMesh)
	{
		Mesh->SetStaticMesh(CubeMesh);
	}
	if (HullMaterial)
	{
		Mesh->SetMaterial(0, HullMaterial); // the hull's own (lit) material: a piece of the ship
	}
	SetActorRotation(FRotator(FMath::FRandRange(-180.f, 180.f), FMath::FRandRange(-180.f, 180.f), FMath::FRandRange(-180.f, 180.f)));
	Apply(0.f);
}

UMaterialInterface* ATDCombatFlash::GetFlashGlowMaterial()
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	static bool bLooked = false;
	if (!Cached.IsValid() && !bLooked)
	{
		bLooked = true;
		static const TCHAR* Path = TEXT("/Game/TriadDrift/Gameplay/Materials/M_TD_FlashGlow.M_TD_FlashGlow");
		if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path))))
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}
	return Cached.Get();
}

void ATDCombatFlash::UseStyledMaterial()
{
	if (StyledLook.bSmoke)
	{
		if (!SmokeMID)
		{
			if (UMaterialInterface* Smoke = GetSmokeMaterial())
			{
				SmokeMID = UMaterialInstanceDynamic::Create(Smoke, this);
			}
		}
		if (SmokeMID)
		{
			Mesh->SetMaterial(0, SmokeMID);
			SmokeMID->SetScalarParameterValue(TEXT("EdgePower"), StyledLook.EdgePower);
			return;
		}
	}
	if (!AdditiveMID)
	{
		if (UMaterialInterface* Additive = GetFlashGlowMaterial())
		{
			AdditiveMID = UMaterialInstanceDynamic::Create(Additive, this);
		}
	}
	UMaterialInstanceDynamic* Want = AdditiveMID ? AdditiveMID.Get() : MID.Get();
	if (Want && Mesh->GetMaterial(0) != Want)
	{
		Mesh->SetMaterial(0, Want);
	}
	if (AdditiveMID)
	{
		AdditiveMID->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		AdditiveMID->SetScalarParameterValue(TEXT("EdgePower"), StyledLook.EdgePower);
		AdditiveMID->SetScalarParameterValue(TEXT("Rim"), StyledLook.bRim ? 1.f : 0.f);
	}
}

void ATDCombatFlash::PlayLook(const FVector& Location, float InRadius, float Seconds, const FTDFlashLook& InLook)
{
	Play(Location, InRadius, InLook.Hot, Seconds, nullptr);
	bStyled = true;
	StyledLook = InLook;
	DelayLeft = FMath::Max(InLook.Delay, 0.f);
	UseStyledMaterial();
	if (InLook.bFaceCamera)
	{
		// The squashed axis (local Z) toward the camera: a shockwave ring seen face-on.
		const UWorld* World = GetWorld();
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerCameraManager)
		{
			const FVector ToCamera = (PC->PlayerCameraManager->GetCameraLocation() - Location).GetSafeNormal();
			if (!ToCamera.IsNearlyZero())
			{
				SetActorRotation(FRotationMatrix::MakeFromZ(ToCamera).Rotator());
			}
		}
	}
	Mesh->SetVisibility(DelayLeft <= 0.f);
	Apply(0.f);
}

void ATDCombatFlash::PlayDebrisLook(const FVector& Location, const FVector& Velocity, float Length, float Thickness, float Seconds,
	const FTDFlashLook& InLook)
{
	PlayDebris(Location, Velocity, Length, Thickness, InLook.Hot, Seconds);
	bStyled = true;
	StyledLook = InLook;
	DelayLeft = FMath::Max(InLook.Delay, 0.f);
	UseStyledMaterial();
	Mesh->SetVisibility(DelayLeft <= 0.f);
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
	if (DelayLeft > 0.f)
	{
		// A staggered pop: hidden until its turn, then it plays its whole life.
		DelayLeft -= DeltaSeconds;
		if (DelayLeft > 0.f)
		{
			return;
		}
		Mesh->SetVisibility(true);
	}
	Age += DeltaSeconds;
	if (Age >= Life)
	{
		Stop();
		return;
	}
	if (bDebris || bChunk)
	{
		SetActorLocation(GetActorLocation() + DebrisVelocity * DeltaSeconds);
		DebrisVelocity *= FMath::Exp(-1.5f * DeltaSeconds); // drag: streaks slow as they fade
	}
	if (bChunk)
	{
		AddActorLocalRotation(ChunkSpin * DeltaSeconds);
	}
	Apply(Age / Life);
}

void ATDCombatFlash::Apply(float Alpha)
{
	if (bChunk)
	{
		// A tumbling piece of hull, shrinking away in its last half (a lit mesh can't fade).
		float Size = ChunkSize * FMath::Clamp(2.f * (1.f - Alpha), 0.f, 1.f);
		const UWorld* World = GetWorld();
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerCameraManager)
		{
			Size = FMath::Min(Size, FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), GetActorLocation()) * 0.08f);
		}
		SetActorScale3D(ChunkAspect * (FMath::Max(Size, 0.1f) / 100.f));
		return;
	}
	if (bDebris)
	{
		// A stretched streak along its flight, thinning as it fades. Near the camera (a hulk exploding as it drifts past)
		// it is capped like the shell: live, a bomber's debris crossed the frame as metre-wide bars.
		const float Shrink = 1.f - 0.6f * Alpha;
		float Length = DebrisLength * Shrink;
		float Thick = Radius * 2.f * Shrink;
		const UWorld* World = GetWorld();
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerCameraManager)
		{
			const float Distance = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), GetActorLocation());
			Thick = FMath::Min(Thick, FMath::Max(Distance * MaxDebrisThicknessPerCameraDistance, 1.f));
			Length = FMath::Min(Length, FMath::Max(Distance * MaxDebrisLengthPerCameraDistance, Thick));
		}
		SetActorScale3D(FVector(Length, Thick, Thick) / 100.f);
	}
	else
	{
		// Swells from 60% to 120% of its radius in the first 30%, then collapses as the glow falls off. The glow material
		// is opaque unlit, so a fading colour reads darker, not clearer: a shell that kept its size turned into a dark
		// disc near the camera (T3's death playtest). Collapsing, the dim end is small.
		// Far away a flash keeps a readable size (hit sparks were ~10 px at 300 m); near the camera it never fills the frame
		// (at most MaxRadiusPerCameraDistance of its distance: the Lead's frame-10 white-out; T3's death shell peaks at 0.24).
		const UWorld* World = GetWorld();
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const float Distance = (PC && PC->PlayerCameraManager) ? FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), GetActorLocation()) : -1.f;
		const float R = Distance > 0.f ? FMath::Max(Radius, Distance * FTDCombatPlaceholders::MinFlashRadiusPerCameraDistance) : Radius;
		float Size = bStyled
			? FMath::Max(R * FMath::Lerp(StyledLook.GrowFrom, StyledLook.GrowTo, 1.f - FMath::Square(1.f - Alpha)), 1.f)
			: Alpha < 0.3f ? R * (0.6f + 2.f * Alpha) : FMath::Max(R * 1.2f * FMath::Square(1.f - (Alpha - 0.3f) / 0.7f), 1.f);
		if (Distance > 0.f)
		{
			Size = FMath::Min(Size, FMath::Max(Distance * CameraCap, 1.f));
		}
		const FVector Squash = bStyled ? StyledLook.Squash : FVector::OneVector;
		SetActorScale3D(Squash * (Size / 50.f)); // the engine sphere is 100 cm across
	}
	if (bStyled)
	{
		// Hot -> Mid over the first 40% of its life, Mid -> Cool after; glow falls off (or swells and dies for an ember).
		const FTDFlashLook& L = StyledLook;
		const FLinearColor Tint = Alpha < 0.4f ? FMath::Lerp(L.Hot, L.Mid, Alpha / 0.4f) : FMath::Lerp(L.Mid, L.Cool, (Alpha - 0.4f) / 0.6f);
		const float Glow = L.bEmber ? L.Glow * FMath::Sin(PI * FMath::Clamp(Alpha, 0.f, 1.f)) : L.Glow * FMath::Pow(FMath::Max(1.f - Alpha, 0.f), L.GlowExponent);
		if (L.bSmoke && SmokeMID && Mesh->GetMaterial(0) == SmokeMID)
		{
			// Smoke: its colour, its opacity swelling in fast and thinning out slowly.
			LastColor = Tint;
			SmokeMID->SetVectorParameterValue(TEXT("Color"), Tint);
			SmokeMID->SetScalarParameterValue(TEXT("Opacity"), L.Opacity * FMath::Sin(PI * FMath::Pow(FMath::Clamp(Alpha, 0.f, 1.f), 0.6f)));
			return;
		}
		LastColor = Tint * Glow;
		if (UMaterialInstanceDynamic* Target = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
		{
			Target->SetVectorParameterValue(TEXT("Color"), LastColor);
			Target->SetVectorParameterValue(TEXT("EmissiveColor"), LastColor);
		}
	}
	else if (MID)
	{
		const FLinearColor Glow = Color * (FTDCombatPlaceholders::FlashGlow * FMath::Square(1.f - Alpha));
		LastColor = Glow;
		MID->SetVectorParameterValue(TEXT("Color"), Glow);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Glow);
	}
}

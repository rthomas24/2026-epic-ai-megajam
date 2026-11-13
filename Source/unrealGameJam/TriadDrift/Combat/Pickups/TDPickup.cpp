// Triad Drift - space pickups (Teammate 2).

#include "TriadDrift/Combat/Pickups/TDPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TriadDrift/Core/TDStatics.h"

ATDPickup::ATDPickup()
{
	PrimaryActorTick.bCanEverTick = false; // the subsystem advances every pickup
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Spinner = CreateDefaultSubobject<USceneComponent>(TEXT("Spinner"));
	Spinner->SetupAttachment(Root);
	SetActorEnableCollision(false);
	SetCanBeDamaged(false);
}

FLinearColor ATDPickup::ColorOf(ETDPickupType Type)
{
	// Never magenta / cyan / amber (the squad) nor red / orange (the Armada): green, white-blue, violet, gold-white.
	switch (Type)
	{
	case ETDPickupType::Hull: return FLinearColor(.25f, 1.f, .35f);
	// Live at 200 m the first shield (.72, .86, 1) and power (.62, .35, 1) glowed plain white: more saturated.
	case ETDPickupType::Shield: return FLinearColor(.42f, .70f, 1.f);
	case ETDPickupType::Power: return FLinearColor(.55f, .22f, 1.f);
	default: return FLinearColor(1.f, .92f, .62f);
	}
}

const TCHAR* ATDPickup::WordOf(ETDPickupType Type)
{
	switch (Type)
	{
	case ETDPickupType::Hull: return TEXT("+HULL");
	case ETDPickupType::Shield: return TEXT("SHIELD CELL");
	case ETDPickupType::Power: return TEXT("POWER READY");
	default: return TEXT("OVERCHARGE");
	}
}

UStaticMeshComponent* ATDPickup::AddPart(UStaticMesh* Mesh, const FVector& Offset, const FRotator& Rotation, const FVector& Scale)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetStaticMesh(Mesh);
	Part->SetupAttachment(Spinner);
	Part->SetRelativeLocation(Offset);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCollisionResponseToAllChannels(ECR_Ignore);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCastShadow(false);
	Part->bReceivesDecals = false;
	if (Glow)
	{
		Part->SetMaterial(0, Glow);
	}
	Part->RegisterComponent();
	Parts.Add(Part);
	return Part;
}

void ATDPickup::Init(ETDPickupType InType, float InLifetime, float InBlinkSeconds)
{
	Type = InType;
	Lifetime = FMath::Max(1.f, InLifetime);
	BlinkSeconds = FMath::Clamp(InBlinkSeconds, 0.f, Lifetime);
	Age = 0.f;
	Home = GetActorLocation();
	if (UMaterialInterface* Base = UTDStatics::GetPlaceholderGlowMaterial())
	{
		Glow = UMaterialInstanceDynamic::Create(Base, this);
		const FLinearColor C = ColorOf(Type) * 2.2f; // glowing; x4 saturated the blue and violet to white (live)
		Glow->SetVectorParameterValue(TEXT("Color"), C);
		Glow->SetVectorParameterValue(TEXT("EmissiveColor"), C);
	}
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	const float S = Size / 100.f; // engine shapes are 100 cm
	// A distinct silhouette per type, so colour is never the only cue.
	switch (Type)
	{
	case ETDPickupType::Hull: // a plus
		AddPart(Cube, FVector::ZeroVector, FRotator::ZeroRotator, FVector(.28f, .28f, 1.f) * S);
		AddPart(Cube, FVector::ZeroVector, FRotator::ZeroRotator, FVector(.28f, 1.f, .28f) * S);
		break;
	case ETDPickupType::Shield: // a flat disc with a bright core
		AddPart(Cylinder, FVector::ZeroVector, FRotator(0.f, 0.f, 90.f), FVector(.9f, .9f, .12f) * S);
		AddPart(Sphere, FVector::ZeroVector, FRotator::ZeroRotator, FVector(.38f) * S);
		break;
	case ETDPickupType::Power: // two points, tip to tip outward (a double arrow)
		AddPart(Cone, FVector(0.f, 0.f, .3f * Size), FRotator::ZeroRotator, FVector(.7f, .7f, .6f) * S);
		AddPart(Cone, FVector(0.f, 0.f, -.3f * Size), FRotator(180.f, 0.f, 0.f), FVector(.7f, .7f, .6f) * S);
		break;
	default: // a cube turned on its corner, with a smaller one inside the other way
		AddPart(Cube, FVector::ZeroVector, FRotator(45.f, 0.f, 45.f), FVector(.62f) * S);
		AddPart(Cube, FVector::ZeroVector, FRotator(0.f, 45.f, 0.f), FVector(.4f) * S);
		break;
	}
	Advance(0.f);
}

void ATDPickup::Advance(float Dt)
{
	Age += Dt;
	// Slow spin and a gentle bob (1 m) around where it dropped; the subsystem moves Home when it pulls it in.
	Spinner->SetRelativeRotation(FRotator(0.f, Age * 70.f, 0.f));
	Spinner->SetRelativeLocation(FVector(0.f, 0.f, 50.f * FMath::Sin(Age * 2.f)));
	// The last BlinkSeconds: blink faster and faster; the very end shrinks away (an unlit glow can't fade).
	const float Left = Lifetime - Age;
	bool bVisible = true;
	if (Left < BlinkSeconds)
	{
		const float Rate = FMath::Lerp(3.f, 9.f, 1.f - Left / FMath::Max(BlinkSeconds, .01f));
		bVisible = FMath::Frac(Age * Rate) < .65f;
	}
	const float Shrink = FMath::Clamp(Left / .4f, 0.f, 1.f);
	Spinner->SetRelativeScale3D(FVector(FMath::Max(Shrink, .01f) * (1.f + .06f * FMath::Sin(Age * 6.f))));
	Spinner->SetVisibility(bVisible && Left > 0.f, true);
}

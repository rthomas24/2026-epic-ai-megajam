// Triad Drift - Powers (Teammate 2). Power presentation: data assets by convention + Shipping-safe placeholders.

#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

const FName FTDPowerPresentation::PlaceholderTag(TEXT("TDPowerPlaceholder"));

namespace
{
	/** Path -> loaded asset, or a remembered miss. Assets are long-lived; weak pointers survive editor reloads. */
	template <typename T>
	T* TDPresLoadCached(const FString& Name)
	{
		static TMap<FString, TWeakObjectPtr<UObject>> Found;
		static TSet<FString> Missing;
		if (Missing.Contains(Name))
		{
			return nullptr;
		}
		if (const TWeakObjectPtr<UObject>* Hit = Found.Find(Name))
		{
			if (UObject* Obj = Hit->Get())
			{
				return Cast<T>(Obj);
			}
		}
		const FString Path = FString::Printf(TEXT("/Game/TriadDrift/Presentation/%s.%s"), *Name, *Name);
		T* Asset = LoadObject<T>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (Asset)
		{
			Found.Add(Name, Asset);
		}
		else
		{
			Missing.Add(Name);
		}
		return Asset;
	}

	UStaticMesh* TDPresShapeMesh(FTDPowerPresentation::EShape Shape)
	{
		// From the flash actor's CDO: hard references, so the cook includes them (path-only loads don't ship).
		const ATDPlaceholderFlash* CDO = GetDefault<ATDPlaceholderFlash>();
		switch (Shape)
		{
		case FTDPowerPresentation::EShape::Cylinder: return CDO->CylinderMesh;
		case FTDPowerPresentation::EShape::Cone: return CDO->ConeMesh;
		case FTDPowerPresentation::EShape::Cube: return CDO->CubeMesh;
		default: return CDO->SphereMesh;
		}
	}

	template <typename TComp>
	TComp* TDPresNewComponent(AActor* Owner, USceneComponent* Parent)
	{
		TComp* Comp = NewObject<TComp>(Owner);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCastShadow(false);
		Comp->SetCanEverAffectNavigation(false);
		Comp->ComponentTags.Add(FTDPowerPresentation::PlaceholderTag);
		USceneComponent* AttachTo = Parent ? Parent : Owner->GetRootComponent();
		if (AttachTo)
		{
			Comp->SetupAttachment(AttachTo);
		}
		Comp->RegisterComponent();
		return Comp;
	}
}

UTDShipVisuals* FTDPowerPresentation::LoadShipVisuals(ETDShipId Ship)
{
	const TCHAR* Name = Ship == ETDShipId::Rift ? TEXT("DA_Rift_Visuals") : (Ship == ETDShipId::Lancer ? TEXT("DA_Lancer_Visuals") : TEXT("DA_Bastion_Visuals"));
	return TDPresLoadCached<UTDShipVisuals>(Name);
}

UTDAbilityFX* FTDPowerPresentation::LoadAbilityFX(ETDAbilityId Ability)
{
	switch (Ability)
	{
	case ETDAbilityId::GravityWell: return TDPresLoadCached<UTDAbilityFX>(TEXT("DA_Ability_GravityWell_FX"));
	case ETDAbilityId::Cloak: return TDPresLoadCached<UTDAbilityFX>(TEXT("DA_Ability_Cloak_FX"));
	case ETDAbilityId::EMPPulse: return TDPresLoadCached<UTDAbilityFX>(TEXT("DA_Ability_EMPPulse_FX"));
	default: return nullptr;
	}
}

UTDTravelFX* FTDPowerPresentation::LoadTravelFX()
{
	return TDPresLoadCached<UTDTravelFX>(TEXT("DA_Travel_FX"));
}

UTDWeaponFX* FTDPowerPresentation::LoadWeaponFX(FName WeaponId)
{
	return WeaponId.IsNone() ? nullptr : TDPresLoadCached<UTDWeaponFX>(FString::Printf(TEXT("DA_%s_FX"), *WeaponId.ToString()));
}

namespace
{
	TMap<FName, int32>& TDPresCueRequests()
	{
		static TMap<FName, int32> Requests;
		return Requests;
	}
}

UNiagaraComponent* FTDPowerPresentation::RequestCue(const UObject* WorldContext, FName Moment, const FTDFXCue& Cue, const FVector& Location,
	const FRotator& Rotation)
{
	++TDPresCueRequests().FindOrAdd(Moment);
	return UTDStatics::PlayCue(WorldContext, Cue, Location, Rotation);
}

UNiagaraComponent* FTDPowerPresentation::RequestCueAttached(FName Moment, const FTDFXCue& Cue, USceneComponent* AttachTo)
{
	++TDPresCueRequests().FindOrAdd(Moment);
	return UTDStatics::PlayCueAttached(Cue, AttachTo);
}

int32 FTDPowerPresentation::GetCueRequests(FName Moment)
{
	const int32* N = TDPresCueRequests().Find(Moment);
	return N ? *N : 0;
}

void FTDPowerPresentation::ResetCueRequests()
{
	TDPresCueRequests().Reset();
}

bool FTDPowerPresentation::HasArt(const FTDFXCue& Cue)
{
	return !Cue.VFX.IsNull();
}

UStaticMeshComponent* FTDPowerPresentation::AddShape(AActor* Owner, USceneComponent* Parent, EShape Shape, const FVector& Scale,
	const FRotator& RelativeRotation, FLinearColor Color, float EmissiveBoost)
{
	if (!Owner)
	{
		return nullptr;
	}
	UStaticMeshComponent* Mesh = TDPresNewComponent<UStaticMeshComponent>(Owner, Parent);
	Mesh->SetStaticMesh(TDPresShapeMesh(Shape));
	Mesh->SetRelativeRotation(RelativeRotation);
	Mesh->SetRelativeScale3D(Scale);
	UTDStatics::TintPlaceholder(Mesh, Color, EmissiveBoost);
	return Mesh;
}

USceneComponent* FTDPowerPresentation::AddRingLattice(AActor* Owner, USceneComponent* Parent, float Radius, int32 Rings,
	int32 SegmentsPerRing, float Thickness, FLinearColor Color, float EmissiveBoost)
{
	if (!Owner || Radius <= 0.f || SegmentsPerRing < 3)
	{
		return nullptr;
	}
	USceneComponent* Holder = NewObject<USceneComponent>(Owner);
	Holder->ComponentTags.Add(PlaceholderTag);
	if (USceneComponent* AttachTo = Parent ? Parent : Owner->GetRootComponent())
	{
		Holder->SetupAttachment(AttachTo);
	}
	Holder->RegisterComponent();

	const int32 Segments = FMath::Clamp(SegmentsPerRing, 3, 24);
	// Ring planes: horizontal, then the two vertical great circles.
	const FQuat Planes[3] = { FQuat::Identity, FQuat(FVector::ForwardVector, HALF_PI), FQuat(FVector::RightVector, HALF_PI) };
	const float SegmentLength = 2.f * PI * Radius / Segments * 1.05f;
	for (int32 R = 0; R < FMath::Clamp(Rings, 1, 3); ++R)
	{
		for (int32 i = 0; i < Segments; ++i)
		{
			const float A = 2.f * PI * (i + 0.5f) / Segments;
			const FVector Local(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius, 0.f);
			const FVector Tangent(-FMath::Sin(A), FMath::Cos(A), 0.f);
			const FQuat Rot = Planes[R] * FRotationMatrix::MakeFromX(Tangent).ToQuat();
			UStaticMeshComponent* Seg = TDPresNewComponent<UStaticMeshComponent>(Owner, Holder);
			Seg->ComponentTags.Remove(PlaceholderTag); // the holder is the placeholder; segments are its parts
			Seg->SetStaticMesh(TDPresShapeMesh(EShape::Cube));
			Seg->SetRelativeTransform(FTransform(Rot, Planes[R].RotateVector(Local), FVector(SegmentLength / 100.f, Thickness / 100.f, Thickness / 100.f)));
			UTDStatics::TintPlaceholder(Seg, Color, EmissiveBoost);
		}
	}
	return Holder;
}

void FTDPowerPresentation::DestroyPlaceholder(USceneComponent* Placeholder)
{
	if (!IsValid(Placeholder))
	{
		return;
	}
	TArray<USceneComponent*> Children;
	Placeholder->GetChildrenComponents(true, Children);
	for (USceneComponent* C : Children)
	{
		if (IsValid(C))
		{
			C->DestroyComponent();
		}
	}
	Placeholder->DestroyComponent();
}

int32 FTDPowerPresentation::CountPlaceholders(const AActor* Owner)
{
	if (!Owner)
	{
		return 0;
	}
	int32 N = 0;
	TInlineComponentArray<USceneComponent*> Comps(Owner);
	for (const USceneComponent* C : Comps)
	{
		N += (IsValid(C) && C->ComponentHasTag(PlaceholderTag) && C->IsVisible()) ? 1 : 0;
	}
	return N;
}

// ------------------------------------------------------------------------------------------ flash actor

ATDPlaceholderFlash::ATDPlaceholderFlash()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	SphereMesh = Sphere.Object;
	CylinderMesh = Cylinder.Object;
	ConeMesh = Cone.Object;
	CubeMesh = Cube.Object;
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	SetCanBeDamaged(false);
}

ATDPlaceholderFlash* ATDPlaceholderFlash::SpawnRingFlash(UWorld* World, const FVector& Location, float Radius, FLinearColor Color, float Seconds)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPlaceholderFlash* Flash = World->SpawnActor<ATDPlaceholderFlash>(ATDPlaceholderFlash::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Flash)
	{
		Flash->Life = FMath::Max(Seconds, 0.05f);
		FTDPowerPresentation::AddRingLattice(Flash, nullptr, Radius, 3, 48, 80.f, Color, 6.f);
	}
	return Flash;
}

void ATDPlaceholderFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	// Expand from 60% to 100% of the radius so the pulse reads as a wave.
	const float T = FMath::Clamp(Age / Life, 0.f, 1.f);
	GetRootComponent()->SetRelativeScale3D(FVector(FMath::Lerp(0.6f, 1.f, T)));
	if (Age >= Life)
	{
		Destroy();
	}
}

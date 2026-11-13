// Triad Drift - World (Teammate 4). Story Act 1: the distortion field around the far-off Dominion.
#include "TriadDrift/World/TDDistortionField.h"

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"

#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

const FName ATDDistortionField::LockId(TEXT("DistortionField"));

ATDDistortionField::ATDDistortionField()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Lattice = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Lattice"));
	Lattice->SetupAttachment(Root);
	Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
	Shell->SetupAttachment(Root);
	if (Sphere.Succeeded())
	{
		Lattice->SetStaticMesh(Sphere.Object);
		Shell->SetStaticMesh(Sphere.Object);
	}
	for (UPrimitiveComponent* C : { (UPrimitiveComponent*)Lattice, (UPrimitiveComponent*)Shell })
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
	}
	Shell->SetVisibility(false);
}

void ATDDistortionField::Init(ATDDominion* InDominion)
{
	Dominion = InDominion;
	if (!InDominion)
	{
		return;
	}
	SetActorLocationAndRotation(InDominion->GetActorLocation(), InDominion->GetActorRotation());
	FieldRadius = FMath::Max(InDominion->GetLocalHullBounds().GetExtent().Size() * HullRadii, InDominion->ShieldRadius * 2.f);

	// The mirage: what the player sees of the Dominion through the field (its hull, bridge and trench; no parts).
	TInlineComponentArray<UStaticMeshComponent*> Meshes(InDominion);
	for (UStaticMeshComponent* C : Meshes)
	{
		if (!C || C->IsA<UInstancedStaticMeshComponent>() || !C->GetStaticMesh() || !C->IsVisible() || C->GetName().StartsWith(TEXT("Shield")))
		{
			continue;
		}
		UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
		M->SetStaticMesh(C->GetStaticMesh());
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetupAttachment(Root);
		M->RegisterComponent();
		M->SetWorldTransform(C->GetComponentTransform());
		for (int32 i = 0; i < C->GetNumMaterials(); ++i) { M->SetMaterial(i, C->GetMaterial(i)); }
		Mirage.Add(M);
	}

	// The field: Nick's / the effects builder's effect and / or shell material, else the stand-in lattice.
	const UTDDominionVisuals* V = InDominion->Visuals;
	if (V && !V->DistortionFieldIdle.IsEmpty())
	{
		FTDFXCue Cue = V->DistortionFieldIdle;
		Cue.VFXScale *= FieldRadius / 100000.f;	// authored for a 1 km radius
		IdleFX = UTDStatics::PlayCueAttached(Cue, Root);
	}
	if (V && !V->DistortionFieldMaterial.IsNull())
	{
		if (UMaterialInterface* Mat = V->DistortionFieldMaterial.LoadSynchronous())
		{
			Shell->SetMaterial(0, Mat);
			Shell->SetRelativeScale3D(FVector(FieldRadius / 50.f));	// the engine sphere is 100 cm across
			Shell->SetVisibility(true);
		}
	}
	const bool bArt = IdleFX != nullptr || Shell->IsVisible();
	Lattice->ClearInstances();
	if (!bArt)
	{
		// Fibonacci sphere of lights: reads as a shell at any distance, and the hull shows through it.
		const int32 N = 520;
		const float Golden = PI * (3.f - FMath::Sqrt(5.f));
		const float Size = FieldRadius * 0.0045f / 50.f;
		for (int32 i = 0; i < N; ++i)
		{
			const float Y = 1.f - (i + 0.5f) * 2.f / N;
			const float R = FMath::Sqrt(1.f - Y * Y);
			const float A = Golden * i;
			const FVector P(FMath::Cos(A) * R, Y, FMath::Sin(A) * R);
			Lattice->AddInstance(FTransform(FRotator::ZeroRotator, P * FieldRadius, FVector(Size)));
		}
		UTDStatics::TintPlaceholder(Lattice, FLinearColor(0.55f, 0.25f, 1.f), 8.f);
	}
	Lattice->SetVisibility(!bArt);
}

void ATDDistortionField::SetWarning(bool bOn)
{
	if (bOn == bWarning)
	{
		return;
	}
	bWarning = bOn;
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		if (bOn) { Travel->AddHyperdriveLock(LockId, NSLOCTEXT("TD", "DistortionFieldNoApproach", "DISTORTION FIELD - NO APPROACH"), this); }
		else { Travel->RemoveHyperdriveLock(LockId); }
	}
}

void ATDDistortionField::Collapse()
{
	if (bCollapsed)
	{
		return;
	}
	bCollapsed = true;
	CollapseTime = 0.f;
	SetWarning(false);
	const ATDDominion* D = Dominion.Get();
	if (D && D->Visuals)
	{
		FTDFXCue Cue = D->Visuals->DistortionFieldCollapse;
		Cue.VFXScale *= FieldRadius / 100000.f;
		UTDStatics::PlayCue(this, Cue, GetActorLocation(), GetActorRotation());
	}
	if (IdleFX) { IdleFX->Deactivate(); }
}

void ATDDistortionField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCollapsed)
	{
		// The shell flares out and is gone; the real Dominion has already taken the mirage's place.
		CollapseTime += DeltaSeconds;
		Lattice->SetRelativeScale3D(FVector(1.f + CollapseTime * 0.6f));
		for (UStaticMeshComponent* M : Mirage) { if (M) { M->SetVisibility(false); } }
		if (CollapseTime >= 0.6f) { Destroy(); }
		return;
	}
	Lattice->AddLocalRotation(FRotator(0.f, 2.f * DeltaSeconds, 0.f));

	bool bNear = false;
	if (const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Squad;
		Registry->GetActorsOfTeam(ETDTeam::Player, Squad, true);
		const FVector C = GetActorLocation();
		for (AActor* A : Squad)
		{
			ATDSpacecraft* Craft = Cast<ATDSpacecraft>(A);
			if (!Craft || Craft->IsHidden())
			{
				continue;
			}
			const FVector Off = Craft->GetActorLocation() - C;
			const float Dist = Off.Size();
			bNear |= Dist <= FieldRadius * WarningRadiusScale;
			if (Dist >= FieldRadius + DeflectBuffer || Dist < KINDA_SMALL_NUMBER)
			{
				Touching.Remove(Craft);
				continue;
			}
			const FVector Out = Off / Dist;
			// One contact per approach: the shimmer where it turns the ship, and the transmission the first time.
			if (!Touching.Contains(Craft) && (FVector::DotProduct(Craft->GetVelocity(), Out) < 0.f || Dist < FieldRadius))
			{
				Touching.Add(Craft);
				++RepelCount;
				if (const ATDDominion* D = Dominion.Get(); D && D->Visuals)
				{
					UTDStatics::PlayCue(this, D->Visuals->DistortionFieldContact, C + Out * FieldRadius, Out.Rotation());
				}
				if (!bToldUnreachable)
				{
					bToldUnreachable = true;
					UTDStorySubsystem::Fire(this, TEXT("A1_FieldUnreachable"));
				}
			}
			// Approaching: the field turns the nose away (the reflected heading, tipped outward), harder the closer it
			// gets. Ships cruise along their nose, so turning it is what keeps them off the shell; gradual, so the
			// chase camera follows instead of snapping, and wingmen follow their leader out.
			const FVector Fwd = Craft->GetActorForwardVector();
			const float Inward = -FVector::DotProduct(Fwd, Out);
			if (Inward > -0.25f)
			{
				const FVector Reflected = Inward > 0.f ? Fwd + 2.f * Inward * Out : Fwd;
				const FQuat Want = (Reflected + Out * 0.35f).GetSafeNormal().ToOrientationQuat();
				const float Closeness = 1.f - FMath::Clamp((Dist - FieldRadius) / FMath::Max(DeflectBuffer, 1.f), 0.f, 1.f);
				const float Angle = FMath::RadiansToDegrees(Craft->GetActorQuat().AngularDistance(Want));
				if (Angle > 0.5f)
				{
					const float Step = FMath::Min(1.f, DeflectTurnRate * FMath::Max(Closeness, 0.2f) * DeltaSeconds / Angle);
					Craft->SetActorRotation(FQuat::Slerp(Craft->GetActorQuat(), Want, Step));
				}
			}
			if (Dist >= FieldRadius)
			{
				continue;
			}
			// Inside the shell: back out to it, the inward part of the velocity gone and a push outward.
			Craft->SetActorLocation(C + Out * FieldRadius * 1.002f);
			if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
			{
				const FVector Vel = Move->Velocity;
				const FVector Tangent = Vel - Out * FMath::Min(0.f, (float)FVector::DotProduct(Vel, Out));
				Move->SetVelocityImmediate(Tangent + Out * RepelSpeed);
			}
		}
	}
	SetWarning(bNear);
}

void ATDDistortionField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetWarning(false);
	Super::EndPlay(EndPlayReason);
}

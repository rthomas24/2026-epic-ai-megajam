// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDPortalGate.h"

#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "Misc/PackageName.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDSpacecraft.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATDPortalGate::ATDPortalGate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;	// after movement, so this frame's motion is known

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Square frame of four bars around the disc, with collision (flying into the frame hurts).
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Bar = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Frame%d"), i));
		Bar->SetupAttachment(Root);
		Bar->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (Cube.Succeeded()) { Bar->SetStaticMesh(Cube.Object); }
		Frame.Add(Bar);
	}

	// Engineering membrane until Nick's PortalIdle VFX lands (hard reference: always cooked).
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Membrane(TEXT("/Game/TriadDrift/Gameplay/Materials/M_TD_PortalMembrane.M_TD_PortalMembrane"));
	MembraneMaterial = Membrane.Get();

	Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Surface"));
	Surface->SetupAttachment(Root);
	Surface->SetCollisionProfileName(TEXT("NoCollision"));
	Surface->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));	// cylinder axis along X: the disc faces forward
	if (Cylinder.Succeeded()) { Surface->SetStaticMesh(Cylinder.Object); }

	// Permanent gates are shortcuts, not reflectors: shots pass through unchanged (no return, no stat).
	TransitParams.bReturnEnemyProjectiles = false;
}

void ATDPortalGate::LinkGates(ATDPortalGate* A, ATDPortalGate* B)
{
	if (!A || !B || A == B)
	{
		return;
	}
	A->Partner = B;
	B->Partner = A;
	A->UpdateLook();
	B->UpdateLook();
}

void ATDPortalGate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutParts();	// placed gates look right in the editor
}

void ATDPortalGate::LayoutParts()
{
	const float Bar = 300.f;
	const float Span = Radius * 2.f + Bar * 2.f;
	Frame[0]->SetRelativeLocation(FVector(0.f, 0.f, Radius + Bar * 0.5f));
	Frame[1]->SetRelativeLocation(FVector(0.f, 0.f, -Radius - Bar * 0.5f));
	Frame[2]->SetRelativeLocation(FVector(0.f, Radius + Bar * 0.5f, 0.f));
	Frame[3]->SetRelativeLocation(FVector(0.f, -Radius - Bar * 0.5f, 0.f));
	Frame[0]->SetRelativeScale3D(FVector(Bar, Span, Bar) / 100.f);
	Frame[1]->SetRelativeScale3D(FVector(Bar, Span, Bar) / 100.f);
	Frame[2]->SetRelativeScale3D(FVector(Bar, Bar, Span) / 100.f);
	Frame[3]->SetRelativeScale3D(FVector(Bar, Bar, Span) / 100.f);
	Surface->SetRelativeScale3D(FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, 0.05f));

	// Art: a frame mesh replaces the four placeholder bars (bar 0 carries it, keeping collision).
	if (!FrameMeshOverride.IsNull() && FPackageName::DoesPackageExist(FrameMeshOverride.ToSoftObjectPath().GetLongPackageName()))
	{
		if (UStaticMesh* Art = FrameMeshOverride.LoadSynchronous())
		{
			Frame[0]->SetStaticMesh(Art);
			Frame[0]->SetRelativeLocation(FVector::ZeroVector);
			Frame[0]->SetRelativeRotation(FRotator::ZeroRotator);
			// One round frame: scale its opening to the transit disc (the disc stays the gameplay truth).
			const float RingScale = FrameMeshInnerDiameter > 0.f ? (Radius * 2.f) / FrameMeshInnerDiameter : 1.f;
			Frame[0]->SetRelativeScale3D(FVector(RingScale));
			Frame[0]->EmptyOverrideMaterials();
			for (int32 i = 1; i < Frame.Num(); ++i)
			{
				Frame[i]->SetVisibility(false);
				Frame[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}

void ATDPortalGate::BeginPlay()
{
	Super::BeginPlay();
	LayoutParts();
	UpdateLook();
	if (TravelFX)
	{
		UTDStatics::PlayCueAttached(TravelFX->PortalIdle, Surface);
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnSpacecraftTeleported.AddUniqueDynamic(this, &ATDPortalGate::HandleSpacecraftTeleported);
	}
}

void ATDPortalGate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnSpacecraftTeleported.RemoveDynamic(this, &ATDPortalGate::HandleSpacecraftTeleported);
	}
	Super::EndPlay(EndPlayReason);
}

void ATDPortalGate::HandleSpacecraftTeleported(AActor* Spacecraft, FVector /*From*/, FVector To)
{
	// Anything that arrives right by this gate (a transit out of our partner, or a wingman follow-warp
	// placed behind its leader) must not be bounced straight back through us.
	// Only arrivals caused by a transit through our partner (the traveller itself, or wingmen
	// follow-warping after their leader). A ship merely spawned near a gate is not guarded.
	const UWorld* World = GetWorld();
	if (!Spacecraft || !World)
	{
		return;
	}
	// Any arrival that is NOT a gate transit (level travel, a stage jump, a retake respawn, a cheat): this ship
	// can't take ANY gate for ArrivalNoTransitSeconds, so a spawn point in front of a gate never pops the player.
	if (!ATDPortalGate::IsTransitInProgress())
	{
		ArrivalBlockedUntil.Add(Spacecraft, World->GetTimeSeconds() + ArrivalNoTransitSeconds);
		return;
	}
	if (!Partner || World->GetTimeSeconds() - Partner->LastTransitTime > ArrivalGuardSeconds)
	{
		return;
	}
	const FVector Rel = To - GetActorLocation();
	const float Along = FVector::DotProduct(Rel, GetActorForwardVector());
	const float Lateral = (Rel - GetActorForwardVector() * Along).Size();
	if (FMath::Abs(Along) <= ArrivalGuardDepth && Lateral <= Radius * 1.5f)
	{
		ArrivalBlockedUntil.Add(Spacecraft, World->GetTimeSeconds() + ArrivalGuardSeconds);
	}
}

void ATDPortalGate::SetGateActive(bool bInActive)
{
	bActive = bInActive;
	UpdateLook();
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastPortalState(this, bActive ? ETDPortalState::Open : ETDPortalState::Closed);
	}
}

void ATDPortalGate::UpdateLook()
{
	if (!Surface)
	{
		return;
	}
	const bool bArtSurface = TravelFX && !TravelFX->PortalIdle.VFX.IsNull();
	Surface->SetVisibility(IsGateActive() && !bArtSurface);
	const FLinearColor Col = IsGateActive() ? FLinearColor(0.75f, 0.2f, 1.f) : FLinearColor(0.15f, 0.15f, 0.2f);
	for (UStaticMeshComponent* Bar : Frame)
	{
		if (!FrameMeshOverride.IsNull()) { break; }	// art keeps its own materials
		UTDStatics::TintPlaceholder(Bar, Col, IsGateActive() ? 3.f : 0.f);
	}
	if (MembraneMaterial)
	{
		// Translucent swirl with a bright rim and a see-through centre, tinted per gate (not an opaque disc).
		if (!MembraneMID) { MembraneMID = Surface->CreateDynamicMaterialInstance(0, MembraneMaterial); }
		if (MembraneMID)
		{
			MembraneMID->SetVectorParameterValue(TEXT("Tint"), GetMembraneTint());
			MembraneMID->SetScalarParameterValue(TEXT("Brightness"), MembraneBrightness);
		}
		Surface->SetCastShadow(false);
	}
	else
	{
		UTDStatics::TintPlaceholder(Surface, FLinearColor(0.7f, 0.2f, 1.f), 2.f);
	}
}

FLinearColor ATDPortalGate::GetMembraneTint() const
{
	// The destination's colour: Verdance teal, Cinder ember, anything else (the flank pair) portal magenta.
	const FString Id = GateId.ToString();
	if (Id.Contains(TEXT("Verdance"))) { return FLinearColor(0.2f, 1.f, 0.8f); }
	if (Id.Contains(TEXT("Cinder"))) { return FLinearColor(1.f, 0.45f, 0.1f); }
	return FLinearColor(1.f, 0.35f, 1.f);
}

bool& ATDPortalGate::TransitInProgressFlag()
{
	static bool bInTransit = false;
	return bInTransit;
}

bool ATDPortalGate::SegmentCrossesGate(const FTransform& GateTransform, float InRadius, const FVector& From, const FVector& To, bool& bOutForward)
{
	const FVector P = GateTransform.GetLocation();
	const FVector F = GateTransform.GetUnitAxis(EAxis::X);
	const float S0 = FVector::DotProduct(From - P, F);
	const float S1 = FVector::DotProduct(To - P, F);
	// Strictly from one side to the other (starting exactly on the plane counts, ending on it doesn't).
	if (!((S0 < 0.f && S1 >= 0.f) || (S0 > 0.f && S1 <= 0.f)))
	{
		return false;
	}
	const float T = S0 / (S0 - S1);
	const FVector Hit = From + (To - From) * T;
	const FVector Lateral = (Hit - P) - F * FVector::DotProduct(Hit - P, F);
	if (Lateral.SizeSquared() > FMath::Square(InRadius))
	{
		return false;
	}
	bOutForward = S0 < 0.f;
	return true;
}

bool ATDPortalGate::TryTransit(AActor* Traveller, float DeltaSeconds)
{
	if (!IsValid(Traveller) || !Partner || !UTDPortalTransit::CanTransit(Traveller, TransitParams.ReentryCooldownSeconds))
	{
		return false;
	}
	// A freshly spawned ship (level travel, respawn) gets the same grace before any gate takes it.
	if (Traveller->IsA<ATDSpacecraft>() && Traveller->GetGameTimeSinceCreation() < ArrivalNoTransitSeconds)
	{
		return false;
	}
	if (const double* Until = ArrivalBlockedUntil.Find(Traveller))
	{
		if (GetWorld() && GetWorld()->GetTimeSeconds() < *Until) { return false; }
		ArrivalBlockedUntil.Remove(Traveller);
	}
	const FVector Now = Traveller->GetActorLocation();
	// Projectiles report where their last step started (launch point / post-teleport point), so a shot
	// fired this frame isn't treated as having crossed the plane. Ships integrate a full step.
	const ATDProjectile* Proj = Cast<ATDProjectile>(Traveller);
	const FVector Before = Proj ? Proj->GetPreviousLocation() : Now - Traveller->GetVelocity() * DeltaSeconds;
	bool bForward = true;
	if (!SegmentCrossesGate(GetActorTransform(), Radius, Before, Now, bForward))
	{
		return false;
	}

	// Approaching from the back: flip both ends so "forward" is still the direction of travel.
	const FQuat Flip(FVector::UpVector, PI);
	FTransform Entry = GetActorTransform();
	FTransform Exit = Partner->GetActorTransform();
	Entry.SetScale3D(FVector::OneVector);
	Exit.SetScale3D(FVector::OneVector);
	if (!bForward)
	{
		Entry.SetRotation(Entry.GetRotation() * Flip);
		Exit.SetRotation(Exit.GetRotation() * Flip);
	}

	// Mark the transit BEFORE moving: the teleport broadcast makes wingmen follow-warp right away
	// (inside this call), and the partner gate must already know a transit is happening to guard them.
	const double PrevTransitTime = LastTransitTime;
	LastTransitTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	TGuardValue<bool> InTransit(ATDPortalGate::TransitInProgressFlag(), true);	// our own teleport is a gate transit
	if (!UTDPortalTransit::TransitActor(Traveller, Entry, Exit, this, TransitParams))
	{
		LastTransitTime = PrevTransitTime;
		return false;
	}

	++TransitCount;
	if (TravelFX)
	{
		UTDStatics::PlayCue(this, TravelFX->PortalTransit, Now, Entry.Rotator());
		UTDStatics::PlayCue(this, TravelFX->PortalTransit, Traveller->GetActorLocation(), Exit.Rotator());
	}
	OnTransit.Broadcast(this, Traveller);
	return true;
}

void ATDPortalGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsGateActive() || DeltaSeconds <= 0.f)
	{
		return;
	}

	const FVector P = GetActorLocation();
	auto IsNear = [&](const AActor* A)
	{
		const float Reach = Radius + A->GetVelocity().Size() * DeltaSeconds + 1000.f;
		return FVector::DistSquared(A->GetActorLocation(), P) <= FMath::Square(Reach);
	};

	// Ships: everything damageable that flies (squad and enemies) is in the combat registry.
	if (const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		Scratch.Reset();
		for (const TWeakObjectPtr<AActor>& Weak : Registry->GetAll())
		{
			AActor* A = Weak.Get();
			// Squad ships only: an Armada ship that wandered through an always-open planet gate would reappear
			// ~400 000 cm away and stall its wave (review minor 6). Enemy ships fly through the frame.
			if (A && A->IsA<ATDSpacecraft>() && (bTransitEnemyShips || UTDStatics::GetTeam(A) != ETDTeam::Enemy) && IsNear(A))
			{
				Scratch.Add(A);	// copy: a transit moves the actor, never mutate while iterating
			}
		}
		for (AActor* A : Scratch)
		{
			TryTransit(A, DeltaSeconds);
		}
	}

	// Projectiles have no collision (they sweep), so scan the live list.
	if (bTransitProjectiles)
	{
		if (const UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this))
		{
			Scratch.Reset();
			for (ATDProjectile* Proj : Projectiles->GetLiveProjectiles())
			{
				if (Proj && IsNear(Proj)) { Scratch.Add(Proj); }
			}
			for (AActor* A : Scratch)
			{
				TryTransit(A, DeltaSeconds);
			}
		}
	}
}

void ATDPortalGate::PlayTransitFlare(const FVector& Where)
{
	if (TravelFX)
	{
		UTDStatics::PlayCue(this, TravelFX->PortalTransit, Where, GetActorRotation());
	}
	if (!TravelFX || !FTDCombatPlaceholders::HasVisualArt(TravelFX->PortalTransit))
	{
		if (UTDProjectileSubsystem* Proj = UTDProjectileSubsystem::Get(this))
		{
			Proj->PlayPlaceholderFlash(Where, Radius * 0.6f, FLinearColor(1.f, 0.35f, 1.f) * 6.f, 0.6f);
		}
	}
}

// Triad Drift - auto-landing and take-off (ENG-20). Owner: Teammate 1.

#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

#if __has_include("TriadDrift/Squad/TDSquadManager.h")
#include "TriadDrift/Squad/TDSquadManager.h"
#define TD_HAS_SQUAD 1
#else
#define TD_HAS_SQUAD 0
#endif

namespace TDLandingSquad
{
#if TD_HAS_SQUAD
	template <typename T>
	concept HasWingmenPassive = requires(T* M) { M->SetWingmenPassive(true); };

	/** Template so the call is only compiled once T3's SetWingmenPassive exists. */
	template <typename T>
	void CallSetWingmenPassive(T* Manager, bool bPassive)
	{
		if constexpr (HasWingmenPassive<T>)
		{
			Manager->SetWingmenPassive(bPassive);
		}
	}
#endif

	/** Tell T3's squad manager to park / resume the wingman AI (no-op until that API exists). */
	void SetWingmenPassive(const UObject* Context, bool bPassive)
	{
#if TD_HAS_SQUAD
		if (ATDSquadManager* Squad = ATDSquadManager::Get(Context))
		{
			CallSetWingmenPassive(Squad, bPassive);
		}
#endif
	}
}

namespace
{
	constexpr ETDLandingState TDLanding_LandingSteps[] = {
		ETDLandingState::Aligning, ETDLandingState::GearDown, ETDLandingState::Descending,
		ETDLandingState::Touchdown, ETDLandingState::EnginesOff, ETDLandingState::CanopyOpening };
	constexpr ETDLandingState TDLanding_TakeOffSteps[] = {
		ETDLandingState::CanopyClosing, ETDLandingState::EnginesOn, ETDLandingState::Ascending, ETDLandingState::GearUp };

	float TDLanding_Smooth(float A)
	{
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(A, 0.f, 1.f));
	}
}

UTDLandingComponent::UTDLandingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTDLandingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A destroyed ship never ticks again: drop any pad marker it set.
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->UpdatePadMarker(this, nullptr, false);
	}
	Super::EndPlay(EndPlayReason);
}

void UTDLandingComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!TravelFX)
	{
		TravelFX = UTDTravelSubsystem::GetDefaultTravelFX(this);
	}
	if (ATDShip* Ship = GetShip())
	{
		if (UTDHealthComponent* Health = Ship->GetHealth())
		{
			Health->OnDeath.AddDynamic(this, &UTDLandingComponent::HandleOwnerDeath);
		}
	}
}

void UTDLandingComponent::HandleOwnerDeath(UTDHealthComponent* HealthComp, const FTDDamageInfo& KillingBlow)
{
	AbortToFlying(); // a ship that dies mid-landing (or landed) frees its pad and never respawns "Landed"
}

ATDShip* UTDLandingComponent::GetShip() const
{
	return Cast<ATDShip>(GetOwner());
}

ATDLandingPad* UTDLandingComponent::GetAvailablePad() const
{
	const ATDShip* Ship = GetShip();
	const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
	if (!Ship || !Travel || State != ETDLandingState::Flying)
	{
		return nullptr;
	}
	const UTDShipMovementComponent* Move = Ship->GetShipMovement();
	if (!Move)
	{
		return nullptr;
	}
	if (Ship->GetVelocity().Size() > GetLandingSpeedLimit())
	{
		return nullptr;
	}
	if (const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
	{
		if (Hyper->GetState() != ETDHyperdriveState::Idle)
		{
			return nullptr;
		}
	}
	ATDLandingPad* Best = Travel->FindPlayerPad(Ship->GetActorLocation(), TNumericLimits<float>::Max());
	if (Best && FVector::Dist(Best->GetBeaconLocation(), Ship->GetActorLocation()) <= Best->GetPromptRange())
	{
		return Best;
	}
	return nullptr;
}

float UTDLandingComponent::GetLandingSpeedLimit() const
{
	const ATDShip* Ship = GetShip();
	const UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
	return Move ? Move->GetCruiseCapSpeed() * Tuning.MaxSpeedFraction + 1.f : 0.f; // +1: "at or below"
}

ATDLandingPad* UTDLandingComponent::GetAssistPad(float RangeScale) const
{
	const ATDShip* Ship = GetShip();
	const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
	if (!Ship || !Travel || State != ETDLandingState::Flying || !Ship->IsPlayerFlown() || Ship->IsBoosting())
	{
		return nullptr;
	}
	if (const UTDHealthComponent* Health = Ship->GetHealth())
	{
		if (Health->IsDead())
		{
			return nullptr;
		}
	}
	if (const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
	{
		if (Hyper->GetState() != ETDHyperdriveState::Idle)
		{
			return nullptr;
		}
	}
	ATDLandingPad* Near = Travel->FindPlayerPad(Ship->GetActorLocation(), TNumericLimits<float>::Max());
	if (Near && !Near->IsOccupied() && FVector::Dist(Near->GetBeaconLocation(), Ship->GetActorLocation()) <= Tuning.SlowDownPromptRange * RangeScale)
	{
		return Near;
	}
	return nullptr;
}

bool UTDLandingComponent::StartApproachAssist()
{
	if (IsApproachAssisting())
	{
		return true;
	}
	ATDLandingPad* Target = GetAssistPad();
	if (!Target)
	{
		return false;
	}
	AssistPad = Target;
	TickApproachAssist(0.f);
	return true;
}

void UTDLandingComponent::StopApproachAssist()
{
	const bool bWasAssisting = AssistPad.IsValid() || AssistPad.IsStale();
	AssistPad.Reset();
	if (bWasAssisting)
	{
		if (ATDShip* Ship = GetShip())
		{
			Ship->ClearSpeedOverrideFor(TEXT("LandingAssist"));
		}
	}
}

void UTDLandingComponent::TickApproachAssist(float DeltaTime)
{
	ATDShip* Ship = GetShip();
	ATDLandingPad* Target = AssistPad.Get();
	// Anything that would refuse a new assist (death, a switch, hyperdrive, boost, out of range) ends this one.
	// Twice the start range: an assist begun near the edge (or past the pad) has room to turn the ship around.
	if (!Ship || !Target || GetAssistPad(2.f) != Target)
	{
		StopApproachAssist();
		return;
	}
	const float Radius = Ship->GetCollisionSphere() ? Ship->GetCollisionSphere()->GetScaledSphereRadius() : 200.f;
	const FVector Hover = Target->GetBeaconLocation() + FVector(0.f, 0.f, Radius + 5.f + Tuning.HoverHeight);
	const FVector ToHover = Hover - Ship->GetActorLocation();
	const float Dist = ToHover.Size();
	if (Dist <= Tuning.AssistHandOffDistance)
	{
		StopApproachAssist();
		if (BeginLanding(Target))
		{
			LandWingmen(Target);
		}
		return;
	}
	const float ToGo = FMath::Max(0.f, Dist - Tuning.AssistHandOffDistance);
	const float Speed = FMath::Clamp(FMath::Sqrt(FMath::Square(Tuning.AssistEndSpeed) + 2.f * Tuning.AssistBrake * ToGo),
		Tuning.AssistEndSpeed, FMath::Max(Tuning.AssistSpeed, Tuning.AssistEndSpeed));
	// Steering 0: the assist owns the nose. Acceleration high enough to follow the braking curve.
	Ship->SetSpeedOverrideFor(TEXT("LandingAssist"), Speed, 0.f, Tuning.AssistBrake * 2.f);
	if (DeltaTime > 0.f)
	{
		const FQuat Want = ToHover.GetSafeNormal().ToOrientationQuat();
		const FQuat Turned = FMath::QInterpConstantTo(Ship->GetActorQuat(), Want, DeltaTime, FMath::DegreesToRadians(Tuning.AssistTurnRate));
		Ship->SetActorRotation(Turned);
	}
}

void UTDLandingComponent::UpdatePadMarker()
{
	UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
	const ATDShip* Ship = GetShip();
	if (!Travel || !Ship)
	{
		return;
	}
	const UTDShipMovementComponent* Move = Ship->GetShipMovement();
	const bool bInAtmosphere = Move && Move->IsInAtmosphere();
	if (State == ETDLandingState::Landed)
	{
		bLandedThisVisit = true;
	}
	else if (!bInAtmosphere && State == ETDLandingState::Flying)
	{
		bLandedThisVisit = false;
	}
	const UTDHealthComponent* Health = Ship->GetHealth();
	const bool bAlive = !Health || !Health->IsDead();
	const bool bPlayer = Ship->IsPlayerFlown();
	ATDLandingPad* Wanted = nullptr;
	if (bPlayer && bAlive && bInAtmosphere && !bLandedThisVisit)
	{
		switch (State)
		{
		case ETDLandingState::Flying:
			Wanted = AssistPad.IsValid() ? AssistPad.Get() : Travel->FindPlayerPad(Ship->GetActorLocation(), TNumericLimits<float>::Max());
			break;
		case ETDLandingState::Aligning:
		case ETDLandingState::GearDown:
		case ETDLandingState::Descending:
			Wanted = Pad.Get(); // still coming down: the pad stays marked until touchdown
			break;
		default:
			break; // touchdown onward, take-off: not the objective
		}
	}
	Travel->UpdatePadMarker(this, Wanted, bPlayer);
}

FText UTDLandingComponent::GetLandPrompt() const
{
	if (const ATDLandingPad* Available = GetAvailablePad())
	{
		return Available->GetPromptText();
	}
	if (AssistPad.IsValid())
	{
		return NSLOCTEXT("TDLanding", "AssistActive", "LANDING ASSIST - TAP F OR BOOST TO TAKE CONTROL");
	}
	if (const ATDLandingPad* Assist = GetAssistPad())
	{
		return Assist->GetPromptText();
	}
	// Near a pad but too fast: say why there is no prompt (a player who never brakes would otherwise never know).
	const ATDShip* Ship = GetShip();
	const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
	if (!Ship || !Travel || State != ETDLandingState::Flying || !Ship->IsPlayerFlown() || Ship->GetVelocity().Size() <= GetLandingSpeedLimit())
	{
		return FText::GetEmpty();
	}
	if (const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
	{
		if (Hyper->GetState() != ETDHyperdriveState::Idle)
		{
			return FText::GetEmpty();
		}
	}
	const ATDLandingPad* Near = Travel->FindPlayerPad(Ship->GetActorLocation(), TNumericLimits<float>::Max());
	if (Near && FVector::Dist(Near->GetBeaconLocation(), Ship->GetActorLocation()) <= Tuning.SlowDownPromptRange)
	{
		return NSLOCTEXT("TDLanding", "SlowDownToLand", "SLOW DOWN TO LAND - HOLD SPACE / LT TO BRAKE");
	}
	return FText::GetEmpty();
}

bool UTDLandingComponent::RequestLand()
{
	ATDLandingPad* Available = GetAvailablePad();
	if (!Available || !BeginLanding(Available))
	{
		return false;
	}
	LandWingmen(Available);
	return true;
}

bool UTDLandingComponent::LandOnPad(ATDLandingPad* InPad)
{
	return BeginLanding(InPad);
}

bool UTDLandingComponent::BeginLanding(ATDLandingPad* InPad)
{
	ATDShip* Ship = GetShip();
	if (!Ship || !InPad || State != ETDLandingState::Flying || !InPad->Reserve(Ship))
	{
		return false;
	}
	StopApproachAssist();
	Pad = InPad;
	// Aligning covers the distance to the hover point at no more than the speed the ship arrived with.
	{
		const float Dist = FVector::Dist(Ship->GetActorLocation(), HoverPoint());
		const float Speed = FMath::Max((float)Ship->GetVelocity().Size(), 500.f);
		AlignSecondsNow = FMath::Clamp(1.5f * Dist / Speed, Tuning.AlignSeconds, 4.f); // smoothstep peak = 1.5 x average
	}
	if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
	{
		Hyper->CancelSpool();
	}
	PauseFlight(true);
	SetState(ETDLandingState::Aligning);
	PlanAlignArc();
	return true;
}

void UTDLandingComponent::PlanAlignArc()
{
	AlignArc = 0.f;
	ATDShip* Ship = GetShip();
	const UWorld* World = GetWorld();
	if (!Ship || !World)
	{
		return;
	}
	// The sequence moves the ship directly (no flight model): a tall rock or tower between it and the pad would be
	// flown through. Sweep the straight line; if it is blocked, arc over the obstacle's top (review, Nov 12).
	const float Radius = Ship->GetCollisionSphere() ? Ship->GetCollisionSphere()->GetScaledSphereRadius() : 200.f;
	const FVector From = SegmentStart;
	const FVector To = HoverPoint();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDLandingAlign), false, Ship);
	if (Pad.IsValid())
	{
		Params.AddIgnoredActor(Pad.Get());
	}
	FHitResult Hit;
	if (!UTDShipMovementComponent::SweepTerrain(World, Hit, From, To, Radius, Params))
	{
		return;
	}
	// Find the obstacle's top above the hit point.
	FHitResult Top;
	const FVector Probe(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, FMath::Max(From.Z, To.Z) + 100000.f);
	float TopZ = Hit.ImpactPoint.Z;
	if (UTDShipMovementComponent::SweepTerrain(World, Top, Probe, FVector(Probe.X, Probe.Y, Hit.ImpactPoint.Z), 0.f, Params))
	{
		TopZ = Top.ImpactPoint.Z;
	}
	const float F = FMath::Clamp(Hit.Time, 0.1f, 0.9f);
	const float LineZ = FMath::Lerp(From.Z, To.Z, F);
	const float Need = TopZ + Radius * 2.5f - LineZ;
	AlignArc = FMath::Clamp(Need / (4.f * F * (1.f - F)), 0.f, 30000.f);
	// Give the longer path a little more time so it doesn't rush.
	AlignSecondsNow = FMath::Min(AlignSecondsNow + AlignArc / 3000.f, 6.f);
}

bool UTDLandingComponent::SnapLanded(ATDLandingPad* InPad)
{
	ATDShip* Ship = GetShip();
	if (!Ship || !InPad || State != ETDLandingState::Flying || !InPad->Reserve(Ship))
	{
		return false;
	}
	Pad = InPad;
	PauseFlight(true);
	if (UTDShipMovementComponent* Move = Ship->GetShipMovement()) { Move->SetMoveAnnounced(true); }
	Ship->SetActorLocationAndRotation(TouchPoint(), PadFacing(), false, nullptr, ETeleportType::TeleportPhysics);
	SetParkedBlock(true);
	if (UTDShipMovementComponent* Move = Ship->GetShipMovement()) { Move->SetMoveAnnounced(false); }
	bGearDown = true;
	bEnginesOn = false;
	bCanopyOpen = true;
	SetState(ETDLandingState::Landed);
	return true;
}

void UTDLandingComponent::AbortToFlying()
{
	SetParkedBlock(false);
	StopApproachAssist(); // a retake mid-approach hands the ship straight back
	if (State == ETDLandingState::Flying)
	{
		return;
	}
	ATDShip* Ship = GetShip();
	if (ATDLandingPad* OldPad = Pad.Get())
	{
		OldPad->Release(Ship);
	}
	Pad.Reset();
	bGearDown = false;
	bCanopyOpen = false;
	bEnginesOn = true;
	PauseFlight(false);
	SetState(ETDLandingState::Flying);
	TDLandingSquad::SetWingmenPassive(this, false);
}

bool UTDLandingComponent::RequestTakeOff()
{
	if (State != ETDLandingState::Landed)
	{
		return false;
	}
	SetState(ETDLandingState::CanopyClosing);
	const ATDShip* Ship = GetShip();
	if (Ship && Ship->IsPlayerFlown())
	{
		TakeOffWingmen();
	}
	return true;
}

void UTDLandingComponent::LandWingmen(ATDLandingPad* PlayerPad)
{
	const ATDShip* Self = GetShip();
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
	if (!Self || !Registry || !Travel)
	{
		return;
	}
	TArray<AActor*> Squad;
	Registry->GetActorsOfTeam(Self->GetTeam(), Squad, true);
	TArray<ATDShip*> Mates;
	for (AActor* Actor : Squad)
	{
		ATDShip* Mate = Cast<ATDShip>(Actor);
		if (Mate && Mate != Self && Mate->GetLanding() && !Mate->GetLanding()->IsBusy()
			&& FVector::Dist(Mate->GetActorLocation(), Self->GetActorLocation()) <= Tuning.WingmanLandRadius)
		{
			Mates.Add(Mate);
		}
	}
	TArray<ATDLandingPad*> SidePads;
	Travel->FindWingmanPads(PlayerPad, Mates.Num(), SidePads);
	if (Mates.Num() > 0)
	{
		TDLandingSquad::SetWingmenPassive(this, true); // before we take the ships off the AI
	}
	for (int32 i = 0; i < Mates.Num() && i < SidePads.Num(); ++i)
	{
		Mates[i]->GetLanding()->LandOnPad(SidePads[i]);
	}
}

void UTDLandingComponent::TakeOffWingmen()
{
	const ATDShip* Self = GetShip();
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Self || !Registry)
	{
		return;
	}
	TArray<AActor*> Squad;
	Registry->GetActorsOfTeam(Self->GetTeam(), Squad, true);
	for (AActor* Actor : Squad)
	{
		ATDShip* Mate = Cast<ATDShip>(Actor);
		UTDLandingComponent* MateLanding = (Mate && Mate != Self) ? Mate->GetLanding() : nullptr;
		if (!MateLanding)
		{
			continue;
		}
		if (MateLanding->IsLanded())
		{
			MateLanding->RequestTakeOff();
		}
		else if (MateLanding->IsLandingInProgress())
		{
			// Still coming down when the player re-boards (review minor 3): straight back to flying, so it never
			// finishes landing afterwards and stays parked, invulnerable and holding its pad.
			MateLanding->AbortToFlying();
		}
	}
}

void UTDLandingComponent::PauseFlight(bool bPause)
{
	if (ATDShip* Ship = GetShip())
	{
		Ship->SetFlightParked(bPause);
		// Invulnerable through the landing sequence and while landed (the pilot is on foot / boarding).
		if (UTDHealthComponent* Health = Ship->GetHealth())
		{
			Health->SetInvulnerable(bPause);
		}
	}
}

FBox UTDLandingComponent::VisibleHullBoxLocal(const ATDShip* Ship)
{
	FBox Box(ForceInit);
	if (!Ship) { return Box; }
	TArray<const UStaticMeshComponent*> Meshes;
	if (const UStaticMeshComponent* Hull = Ship->GetHullMesh()) { Meshes.Add(Hull); }
	for (const UStaticMeshComponent* Part : Ship->GetVisualParts()) { Meshes.AddUnique(Part); }
	const FTransform ActorXf = Ship->GetActorTransform();
	for (const UStaticMeshComponent* M : Meshes)
	{
		if (!M || !M->GetStaticMesh() || !M->IsVisible() || M == Ship->GetEngineGlow()) { continue; }
		Box += M->CalcBounds(M->GetComponentTransform().GetRelativeTransform(ActorXf)).GetBox();
	}
	return Box;
}

bool UTDLandingComponent::IsParkedBlockOn() const
{
	return ParkedBlock && ParkedBlock->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
}

void UTDLandingComponent::SetParkedBlock(bool bOn)
{
	ATDShip* Ship = GetShip();
	if (!Ship) { return; }
	if (bOn)
	{
		const FBox Local = VisibleHullBoxLocal(Ship);
		if (!Local.IsValid) { return; }
		if (!ParkedBlock)
		{
			ParkedBlock = NewObject<UBoxComponent>(Ship, TEXT("ParkedBlock"));
			ParkedBlock->SetupAttachment(Ship->GetRootComponent());
			ParkedBlock->SetCollisionObjectType(ECC_WorldStatic);
			ParkedBlock->SetCollisionResponseToAllChannels(ECR_Ignore);
			ParkedBlock->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // the pilot on foot, nothing else
			ParkedBlock->SetGenerateOverlapEvents(false);
			ParkedBlock->SetCanEverAffectNavigation(false);
			ParkedBlock->CanCharacterStepUpOn = ECB_No;
			ParkedBlock->RegisterComponent();
		}
		ParkedBlock->SetRelativeLocation(Local.GetCenter());
		ParkedBlock->SetRelativeRotation(FRotator::ZeroRotator);
		ParkedBlock->SetBoxExtent(Local.GetExtent() * ParkedBlockShrink);
		ParkedBlock->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else if (ParkedBlock)
	{
		ParkedBlock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

float UTDLandingComponent::HullBottomBelowOrigin(const ATDShip* Ship)
{
	// Parked ships sat on their flight collision sphere, ~1 m over the pad (Ryan's screenshot, Nov 13). Park on the
	// visible hull instead: a "Skid" socket if a mesh has one, else the lowest point of the visible meshes' bounds, in the
	// actor's own frame (landed ships are level). Bounds, not a fixed offset: the squad ships are being rebuilt.
	if (!Ship) { return 200.f; }
	TArray<const UStaticMeshComponent*> Meshes;
	if (const UStaticMeshComponent* Hull = Ship->GetHullMesh()) { Meshes.Add(Hull); }
	for (const UStaticMeshComponent* Part : Ship->GetVisualParts()) { Meshes.AddUnique(Part); }
	const FTransform ActorXf = Ship->GetActorTransform();
	float SkidZ = TNumericLimits<float>::Max(), MinZ = TNumericLimits<float>::Max();
	for (const UStaticMeshComponent* M : Meshes)
	{
		if (!M || !M->GetStaticMesh() || !M->IsVisible() || M == Ship->GetEngineGlow()) { continue; }
		if (M->DoesSocketExist(TEXT("Skid")))
		{
			SkidZ = FMath::Min(SkidZ, (float)ActorXf.InverseTransformPosition(M->GetSocketLocation(TEXT("Skid"))).Z);
		}
		const FBox B = M->CalcBounds(M->GetComponentTransform().GetRelativeTransform(ActorXf)).GetBox();
		MinZ = FMath::Min(MinZ, (float)B.Min.Z);
	}
	const float Lowest = SkidZ < TNumericLimits<float>::Max() ? SkidZ : MinZ;
	if (Lowest < TNumericLimits<float>::Max() && Lowest < 0.f)
	{
		return -Lowest;
	}
	return Ship->GetCollisionSphere() ? Ship->GetCollisionSphere()->GetScaledSphereRadius() : 200.f; // no visible mesh: as before
}

FVector UTDLandingComponent::TouchPoint() const
{
	const ATDLandingPad* P = Pad.Get();
	const ATDShip* Ship = GetShip();
	return P ? P->GetBeaconLocation() + FVector(0.f, 0.f, HullBottomBelowOrigin(Ship) + ParkClearance) : (Ship ? Ship->GetActorLocation() : FVector::ZeroVector);
}

FVector UTDLandingComponent::HoverPoint() const
{
	// The hover stays where it was (over the collision sphere's rest) so the align, and the whole 4.9 s landing, keep
	// their timing; only the final descent goes on down to the hull's rest point (TouchPoint), in the same time.
	const ATDLandingPad* P = Pad.Get();
	const ATDShip* Ship = GetShip();
	const float Radius = (Ship && Ship->GetCollisionSphere()) ? Ship->GetCollisionSphere()->GetScaledSphereRadius() : 200.f;
	return P ? P->GetBeaconLocation() + FVector(0.f, 0.f, Radius + 5.f + Tuning.HoverHeight) : TouchPoint() + FVector(0.f, 0.f, Tuning.HoverHeight);
}

FQuat UTDLandingComponent::PadFacing() const
{
	const ATDLandingPad* P = Pad.Get();
	return P ? FRotator(0.f, P->GetActorRotation().Yaw, 0.f).Quaternion() : FQuat::Identity;
}

FTransform UTDLandingComponent::GetExitTransform() const
{
	const ATDShip* Ship = GetShip();
	if (!Ship)
	{
		return FTransform::Identity;
	}
	const FRotator Facing(0.f, Ship->GetActorRotation().Yaw, 0.f);
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
	// Beside the hull, clear of the parked block (the pilot's capsule is ~40 cm wide; 60 cm more for comfort).
	const FBox Hull = VisibleHullBoxLocal(Ship);
	const float Side = Hull.IsValid ? FMath::Max(Tuning.ExitSideOffset, Hull.Max.Y * ParkedBlockShrink + 220.f) : Tuning.ExitSideOffset;
	FVector Location = Ship->GetActorLocation() + Right * Side;
	if (const ATDLandingPad* P = Pad.Get())
	{
		Location.Z = P->GetBeaconLocation().Z + Tuning.ExitHeightAboveDeck;
	}
	return FTransform(Facing, Location);
}

float UTDLandingComponent::StateDuration(ETDLandingState S) const
{
	switch (S)
	{
	case ETDLandingState::Aligning: return AlignSecondsNow;
	case ETDLandingState::GearDown: return Tuning.GearSeconds;
	case ETDLandingState::Descending: return Tuning.DescendSeconds;
	case ETDLandingState::Touchdown: return Tuning.TouchdownSeconds;
	case ETDLandingState::EnginesOff: return Tuning.EnginesSeconds;
	case ETDLandingState::CanopyOpening: return Tuning.CanopySeconds;
	case ETDLandingState::CanopyClosing: return Tuning.CanopySeconds;
	case ETDLandingState::EnginesOn: return Tuning.EnginesSeconds;
	case ETDLandingState::Ascending: return Tuning.AscendSeconds;
	case ETDLandingState::GearUp: return Tuning.GearSeconds;
	default: return 0.f;
	}
}

float UTDLandingComponent::GetLanding01() const
{
	if (State == ETDLandingState::Flying)
	{
		return 0.f;
	}
	if (State == ETDLandingState::Landed)
	{
		return 1.f;
	}
	const float StepAlpha = FMath::Clamp(StateTime / FMath::Max(StateDuration(State), KINDA_SMALL_NUMBER), 0.f, 1.f);
	for (int32 i = 0; i < UE_ARRAY_COUNT(TDLanding_LandingSteps); ++i)
	{
		if (TDLanding_LandingSteps[i] == State)
		{
			return (i + StepAlpha) / UE_ARRAY_COUNT(TDLanding_LandingSteps);
		}
	}
	for (int32 i = 0; i < UE_ARRAY_COUNT(TDLanding_TakeOffSteps); ++i)
	{
		if (TDLanding_TakeOffSteps[i] == State)
		{
			return 1.f - (i + StepAlpha) / UE_ARRAY_COUNT(TDLanding_TakeOffSteps);
		}
	}
	return 0.f;
}

void UTDLandingComponent::SetState(ETDLandingState NewState)
{
	State = NewState;
	StateTime = 0.f;
	if (const ATDShip* Ship = GetShip())
	{
		SegmentStart = Ship->GetActorLocation();
		SegmentStartRot = Ship->GetActorQuat();
	}

	switch (NewState)
	{
	case ETDLandingState::GearDown: bGearDown = true; break;
	case ETDLandingState::Touchdown:
		if (TravelFX)
		{
			UTDStatics::PlayCue(this, TravelFX->LandingDust, Pad.IsValid() ? Pad->GetBeaconLocation() : SegmentStart);
		}
		break;
	case ETDLandingState::EnginesOff: bEnginesOn = false; SetParkedBlock(true); break; // parked: the pilot can't walk through it
	case ETDLandingState::CanopyOpening: bCanopyOpen = true; break;
	case ETDLandingState::CanopyClosing: bCanopyOpen = false; SetParkedBlock(false); break; // taking off: flight collision only
	case ETDLandingState::EnginesOn: bEnginesOn = true; break;
	case ETDLandingState::Ascending:
		if (TravelFX)
		{
			UTDStatics::PlayCue(this, TravelFX->TakeOff, SegmentStart);
		}
		break;
	case ETDLandingState::GearUp: bGearDown = false; break;
	default: break;
	}

	OnLandingStateChanged.Broadcast(this, NewState);

	if (NewState == ETDLandingState::Landed)
	{
		if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
		{
			Travel->OnShipLanded.Broadcast(GetShip(), Pad.Get());
		}
	}
}

void UTDLandingComponent::FinishTakeOff()
{
	ATDShip* Ship = GetShip();
	ATDLandingPad* OldPad = Pad.Get();
	if (OldPad)
	{
		OldPad->Release(Ship);
	}
	Pad.Reset();
	bGearDown = false;
	bCanopyOpen = false;
	bEnginesOn = true;
	PauseFlight(false);
	if (Ship && Ship->GetShipMovement())
	{
		UTDShipMovementComponent* Move = Ship->GetShipMovement();
		Move->SetForwardSpeedImmediate(Move->GetFlightTuning().CruiseSpeed * Tuning.TakeOffSpeedFraction);
	}
	SetState(ETDLandingState::Flying);
	// Unconditional: whichever ship lifts off last, the wingman AI must not stay parked.
	TDLandingSquad::SetWingmenPassive(this, false); // movement ticks again: AI resumes in Regroup
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->OnShipTookOff.Broadcast(Ship, OldPad);
	}
}

void UTDLandingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePadMarker();
	if (State == ETDLandingState::Flying && (AssistPad.IsValid() || AssistPad.IsStale()) && DeltaTime > 0.f)
	{
		TickApproachAssist(DeltaTime);
	}
	if (State == ETDLandingState::Flying || State == ETDLandingState::Landed || DeltaTime <= 0.f)
	{
		return;
	}
	ATDShip* Ship = GetShip();
	if (!Ship)
	{
		return;
	}
	if (!Pad.IsValid() && State < ETDLandingState::Landed)
	{
		// Pad vanished mid-landing: abort back to flight, and give the wingmen their AI back.
		bGearDown = bCanopyOpen = false;
		bEnginesOn = true;
		PauseFlight(false);
		SetState(ETDLandingState::Flying);
		TDLandingSquad::SetWingmenPassive(this, false);
		return;
	}

	StateTime += DeltaTime;
	const float Duration = FMath::Max(StateDuration(State), KINDA_SMALL_NUMBER);
	const float A = TDLanding_Smooth(StateTime / Duration);

	FVector Target = Ship->GetActorLocation();
	FQuat TargetRot = Ship->GetActorQuat();
	switch (State)
	{
	case ETDLandingState::Aligning:
		Target = FMath::Lerp(SegmentStart, HoverPoint(), A) + FVector(0.f, 0.f, AlignArc * 4.f * A * (1.f - A));
		TargetRot = FQuat::Slerp(SegmentStartRot, PadFacing(), A);
		break;
	case ETDLandingState::Descending:
		Target = FMath::Lerp(SegmentStart, TouchPoint(), A);
		TargetRot = PadFacing();
		break;
	case ETDLandingState::Ascending:
		Target = FMath::Lerp(SegmentStart, HoverPoint() + FVector(0.f, 0.f, Tuning.TakeOffClimb), A);
		break;
	case ETDLandingState::GearUp:
		// Keep climbing gently and start moving forward so the hand-back to flight is smooth.
		Target = SegmentStart + (Ship->GetActorForwardVector() * Tuning.GearUpForward + FVector(0.f, 0.f, Tuning.GearUpClimb)) * A;
		break;
	default:
		break;
	}
	Ship->SetActorLocationAndRotation(Target, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);

	if (StateTime >= Duration)
	{
		switch (State)
		{
		case ETDLandingState::Aligning: SetState(ETDLandingState::GearDown); break;
		case ETDLandingState::GearDown: SetState(ETDLandingState::Descending); break;
		case ETDLandingState::Descending: SetState(ETDLandingState::Touchdown); break;
		case ETDLandingState::Touchdown: SetState(ETDLandingState::EnginesOff); break;
		case ETDLandingState::EnginesOff: SetState(ETDLandingState::CanopyOpening); break;
		case ETDLandingState::CanopyOpening: SetState(ETDLandingState::Landed); break;
		case ETDLandingState::CanopyClosing: SetState(ETDLandingState::EnginesOn); break;
		case ETDLandingState::EnginesOn: SetState(ETDLandingState::Ascending); break;
		case ETDLandingState::Ascending: SetState(ETDLandingState::GearUp); break;
		case ETDLandingState::GearUp: FinishTakeOff(); break;
		default: break;
		}
	}
}

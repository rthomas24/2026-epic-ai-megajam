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
	const float SpeedLimit = Move->GetFlightTuning().CruiseSpeed * Tuning.MaxSpeedFraction;
	if (Ship->GetVelocity().Size() > SpeedLimit)
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

FText UTDLandingComponent::GetLandPrompt() const
{
	const ATDLandingPad* Available = GetAvailablePad();
	return Available ? Available->GetPromptText() : FText::GetEmpty();
}

bool UTDLandingComponent::RequestLand()
{
	ATDLandingPad* Available = GetAvailablePad();
	if (!Available || !LandOnPad(Available))
	{
		return false;
	}
	LandWingmen(Available);
	return true;
}

bool UTDLandingComponent::LandOnPad(ATDLandingPad* InPad)
{
	ATDShip* Ship = GetShip();
	if (!Ship || !InPad || State != ETDLandingState::Flying || !InPad->Reserve(Ship))
	{
		return false;
	}
	Pad = InPad;
	if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
	{
		Hyper->CancelSpool();
	}
	PauseFlight(true);
	SetState(ETDLandingState::Aligning);
	return true;
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
	Ship->SetActorLocationAndRotation(TouchPoint(), PadFacing(), false, nullptr, ETeleportType::TeleportPhysics);
	bGearDown = true;
	bEnginesOn = false;
	bCanopyOpen = true;
	SetState(ETDLandingState::Landed);
	return true;
}

void UTDLandingComponent::AbortToFlying()
{
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
		if (Mate && Mate != Self && Mate->GetLanding() && Mate->GetLanding()->IsLanded())
		{
			Mate->GetLanding()->RequestTakeOff();
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

FVector UTDLandingComponent::TouchPoint() const
{
	const ATDLandingPad* P = Pad.Get();
	const ATDShip* Ship = GetShip();
	const float Radius = (Ship && Ship->GetCollisionSphere()) ? Ship->GetCollisionSphere()->GetScaledSphereRadius() : 200.f;
	return P ? P->GetBeaconLocation() + FVector(0.f, 0.f, Radius + 5.f) : (Ship ? Ship->GetActorLocation() : FVector::ZeroVector);
}

FVector UTDLandingComponent::HoverPoint() const
{
	return TouchPoint() + FVector(0.f, 0.f, Tuning.HoverHeight);
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
	FVector Location = Ship->GetActorLocation() + Right * Tuning.ExitSideOffset;
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
	case ETDLandingState::Aligning: return Tuning.AlignSeconds;
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
	case ETDLandingState::EnginesOff: bEnginesOn = false; break;
	case ETDLandingState::CanopyOpening: bCanopyOpen = true; break;
	case ETDLandingState::CanopyClosing: bCanopyOpen = false; break;
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
		// Pad vanished mid-landing: abort back to flight.
		bGearDown = bCanopyOpen = false;
		bEnginesOn = true;
		PauseFlight(false);
		SetState(ETDLandingState::Flying);
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
		Target = FMath::Lerp(SegmentStart, HoverPoint(), A);
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
		Target = SegmentStart + (Ship->GetActorForwardVector() * 600.f + FVector(0.f, 0.f, 300.f)) * A;
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

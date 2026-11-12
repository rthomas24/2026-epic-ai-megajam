// Triad Drift - Squad (Teammate 3). ENG-07: squad manager & instant switching (PROTECT), ENG-03 respawn.

#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDPresentation.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDSquad, Log, All);

const FName ATDSquadManager::SwitchStrikeKey(TEXT("SwitchStrike"));
const FName ATDSquadManager::DownedWeaponsKey(TEXT("SquadDowned"));

namespace TDSquadManagerPrivate
{
	/** One manager per world. Weak, so a destroyed manager never dangles. */
	static TArray<TWeakObjectPtr<ATDSquadManager>> GInstances;
}

ATDSquadManager::ATDSquadManager()
{
	PrimaryActorTick.bCanEverTick = true;
	// Tick before pawns so death/respawn/switch state is settled before ships and wingmen act.
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetCanBeDamaged(false);

	ShipClass = ATDShip::StaticClass();
	WingmanControllerClass = ATDWingmanAIController::StaticClass();
}

ATDSquadManager* ATDSquadManager::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (int32 i = TDSquadManagerPrivate::GInstances.Num() - 1; i >= 0; --i)
	{
		ATDSquadManager* M = TDSquadManagerPrivate::GInstances[i].Get();
		if (!M)
		{
			TDSquadManagerPrivate::GInstances.RemoveAtSwap(i);
			continue;
		}
		if (M->GetWorld() == World)
		{
			return M;
		}
	}
	return nullptr;
}

double ATDSquadManager::RealNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

void ATDSquadManager::Refuse(ETDSwitchRefusal Reason, ETDShipId Ship)
{
	OnSwitchRefused.Broadcast(Reason, Ship);
}

double ATDSquadManager::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void ATDSquadManager::BeginPlay()
{
	Super::BeginPlay();
	TDSquadManagerPrivate::GInstances.AddUnique(this);
	TDSquad::BindPowerUsed(this);
	TDSquad::BindWaveEvents(this);
}

void ATDSquadManager::EndPlay(const EEndPlayReason::Type Reason)
{
	TDSquad::UnbindPowerUsed(this);
	TDSquad::UnbindWaveEvents(this);
	EndSwitchStrike();
	TDSquadManagerPrivate::GInstances.Remove(this);
	Super::EndPlay(Reason);
}

// ---------------------------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------------------------

const FTDShipSpec& ATDSquadManager::GetSpec(ETDShipId Ship) const
{
	if (const FTDShipSpec* Override = ShipSpecOverrides.Find(Ship))
	{
		return *Override;
	}
	return UTDShipLibrary::Get(Ship);
}

ATDWingmanAIController* ATDSquadManager::SpawnWingmanFor(FTDSquadSlot& Slot)
{
	if (Slot.Wingman)
	{
		return Slot.Wingman;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* Cls = WingmanControllerClass ? WingmanControllerClass.Get() : ATDWingmanAIController::StaticClass();
	Slot.Wingman = World->SpawnActor<ATDWingmanAIController>(Cls, FTransform::Identity, Params);
	if (Slot.Wingman)
	{
		Slot.Wingman->InitWingman(this, Slot.ShipId);
	}
	return Slot.Wingman;
}

void ATDSquadManager::SpawnSquad(const FTransform& Origin)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Reset squad-level state (a scene restart calls this again).
	bSquadWiped = false;
	bWipeHandledByFlow = false;
	LastSwitchTime = -1000.0;
	LastPowerTime = -1000.0;
	FocusTarget.Reset();
	FocusEndTime = -1000.0;
	EndSwitchStrike();
	SwitchCount = 0;
	LastSwitchCut = FTDSwitchCut();

	if (SwitchOrder.Num() == 0)
	{
		SwitchOrder = UTDShipLibrary::AllShips();
	}
	if (!SwitchOrder.Contains(StartingShip))
	{
		StartingShip = SwitchOrder[0];
	}

	// Formation slots around the origin: starting ship leads, the others take left/right.
	const FVector LeaderLoc = Origin.GetLocation();
	const FQuat LeaderRot = Origin.GetRotation();
	int32 WingIndex = 0;

	for (const ETDShipId Id : SwitchOrder)
	{
		FTDSquadSlot* Slot = FindSlot(Id);
		if (!Slot)
		{
			FTDSquadSlot NewSlot;
			NewSlot.ShipId = Id;
			Slot = &Slots.Add_GetRef(NewSlot);
		}

		FVector Loc = LeaderLoc;
		if (Id != StartingShip)
		{
			const FVector Offset = (WingIndex++ == 0) ? FormationLeftOffset : FormationRightOffset;
			Loc = LeaderLoc + LeaderRot.RotateVector(Offset);
		}
		const FRotator Facing = LeaderRot.Rotator();

		if (!IsValid(Slot->Ship))
		{
			FActorSpawnParameters Params;
			Params.Owner = this;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			UClass* Cls = ShipClass ? ShipClass.Get() : ATDShip::StaticClass();
			Slot->Ship = World->SpawnActor<ATDShip>(Cls, FTransform(Facing, Loc), Params);
			if (!Slot->Ship)
			{
				UE_LOG(LogTDSquad, Error, TEXT("SpawnSquad: failed to spawn ship %d"), static_cast<int32>(Id));
				continue;
			}
		}
		else
		{
			// A retake: whatever the ship was doing (downed, mid-jump, parked), stop it first.
			TDSquad::StopHyperdrive(Slot->Ship);
		}

		TDSquad::ApplySpec(Slot->Ship, GetSpec(Id));
		// Start the take at cruise along the facing, whatever the ship was doing before.
		{
			float MinS, CruiseS, BoostS;
			TDSquad::GetSpeedRange(Slot->Ship, MinS, CruiseS, BoostS);
			RestoreShipForFlight(Slot->Ship, Loc, Facing, Facing.Vector() * CruiseS);
		}
		Slot->State = ETDSquadSlotState::Alive;
		Slot->RespawnAtTime = 0.0;
		SpawnWingmanFor(*Slot);
	}

	// Hand out control: wingmen first, then the player takes the starting ship. A new take is active.
	ActiveShipId = StartingShip;
	SetWingmenPassive(false);
	for (FTDSquadSlot& Slot : Slots)
	{
		if (!Slot.Ship || !Slot.Wingman)
		{
			continue;
		}
		if (Slot.ShipId == ActiveShipId)
		{
			if (Slot.Wingman->GetPawn() == Slot.Ship)
			{
				Slot.Wingman->UnPossess();
			}
		}
		else
		{
			if (Slot.Wingman->GetPawn() != Slot.Ship)
			{
				Slot.Wingman->Possess(Slot.Ship);
			}
			// Always: a wingman left in Downed by the last take must fly again.
			Slot.Wingman->NotifyShipRespawned();
		}
		TDSquad::SetPlayerControlled(Slot.Ship, Slot.ShipId == ActiveShipId);
	}

	if (APlayerController* PC = GetSquadPlayerController())
	{
		if (ATDShip* Active = GetShip(ActiveShipId))
		{
			if (PC->GetPawn() != Active)
			{
				PC->Possess(Active);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------------------------

FTDSquadSlot* ATDSquadManager::FindSlot(ETDShipId Ship)
{
	return Slots.FindByPredicate([Ship](const FTDSquadSlot& S) { return S.ShipId == Ship; });
}

const FTDSquadSlot* ATDSquadManager::FindSlot(ETDShipId Ship) const
{
	return Slots.FindByPredicate([Ship](const FTDSquadSlot& S) { return S.ShipId == Ship; });
}

ATDShip* ATDSquadManager::GetShip(ETDShipId Ship) const
{
	const FTDSquadSlot* Slot = FindSlot(Ship);
	return (Slot && IsValid(Slot->Ship)) ? Slot->Ship.Get() : nullptr;
}

ATDShip* ATDSquadManager::GetActiveShip() const
{
	return GetShip(ActiveShipId);
}

TArray<ATDShip*> ATDSquadManager::GetLivingShips() const
{
	TArray<ATDShip*> Out;
	for (const ETDShipId Id : SwitchOrder)
	{
		const FTDSquadSlot* Slot = FindSlot(Id);
		if (Slot && Slot->State == ETDSquadSlotState::Alive && IsValid(Slot->Ship))
		{
			Out.Add(Slot->Ship);
		}
	}
	return Out;
}

bool ATDSquadManager::IsShipAlive(ETDShipId Ship) const
{
	const FTDSquadSlot* Slot = FindSlot(Ship);
	return Slot && Slot->State == ETDSquadSlotState::Alive && IsValid(Slot->Ship);
}

ETDSquadSlotState ATDSquadManager::GetShipState(ETDShipId Ship) const
{
	const FTDSquadSlot* Slot = FindSlot(Ship);
	return Slot ? Slot->State : ETDSquadSlotState::NotSpawned;
}

float ATDSquadManager::GetRespawnSeconds(ETDShipId Ship) const
{
	const FTDSquadSlot* Slot = FindSlot(Ship);
	if (!Slot || Slot->State == ETDSquadSlotState::Alive || Slot->State == ETDSquadSlotState::NotSpawned)
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(Slot->RespawnAtTime - Now()));
}

float ATDSquadManager::GetSwitchCooldown01() const
{
	if (SwitchCooldown <= 0.f)
	{
		return 0.f;
	}
	const double Remaining = (LastSwitchTime + SwitchCooldown) - RealNow();
	return FMath::Clamp(static_cast<float>(Remaining / SwitchCooldown), 0.f, 1.f);
}

APlayerController* ATDSquadManager::GetSquadPlayerController() const
{
	if (APlayerController* PC = SquadPC.Get())
	{
		return PC;
	}
	const UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

void ATDSquadManager::SetSquadPlayerController(APlayerController* InController)
{
	SquadPC = InController;
}

FVector ATDSquadManager::GetFormationSlotLocation(ETDShipId Ship) const
{
	const ATDShip* Leader = GetActiveShip();
	if (!Leader)
	{
		const ATDShip* Self = GetShip(Ship);
		return Self ? Self->GetActorLocation() : GetActorLocation();
	}
	if (Ship == ActiveShipId)
	{
		return Leader->GetActorLocation();
	}

	// Left slot = first non-active ship in switch order, right slot = the other one. Stable for a
	// given leader, so wingmen don't swap sides mid-flight.
	int32 WingIndex = 0;
	for (const ETDShipId Id : SwitchOrder)
	{
		if (Id == ActiveShipId)
		{
			continue;
		}
		if (Id == Ship)
		{
			break;
		}
		++WingIndex;
	}
	const FVector Offset = (WingIndex == 0) ? FormationLeftOffset : FormationRightOffset;
	return Leader->GetActorTransform().TransformPositionNoScale(Offset);
}

AActor* ATDSquadManager::GetFocusTarget() const
{
	AActor* Target = FocusTarget.Get();
	if (!Target || Now() >= FocusEndTime || TDSquad::IsDead(Target))
	{
		return nullptr;
	}
	return Target;
}

// ---------------------------------------------------------------------------------------------
// Switching
// ---------------------------------------------------------------------------------------------

bool ATDSquadManager::RequestSwitchTo(ETDShipId Ship)
{
	return SwitchToInternal(Ship, false);
}

bool ATDSquadManager::RequestSwitchNext()
{
	return CycleSwitch(+1);
}

bool ATDSquadManager::RequestSwitchPrev()
{
	return CycleSwitch(-1);
}

bool ATDSquadManager::CycleSwitch(int32 Direction)
{
	const int32 Count = SwitchOrder.Num();
	if (Count < 2)
	{
		return false;
	}
	const int32 Start = FMath::Max(0, SwitchOrder.IndexOfByKey(ActiveShipId));
	for (int32 Step = 1; Step < Count; ++Step)
	{
		const int32 Index = ((Start + Direction * Step) % Count + Count) % Count;
		const ETDShipId Candidate = SwitchOrder[Index];
		if (IsShipAlive(Candidate))
		{
			return SwitchToInternal(Candidate, false);
		}
	}
	return false;
}

bool ATDSquadManager::SwitchToInternal(ETDShipId Ship, bool bForced)
{
	if (Ship == ActiveShipId)
	{
		if (!bForced)
		{
			Refuse(ETDSwitchRefusal::AlreadyActive, Ship);
		}
		return false;
	}
	FTDSquadSlot* NewSlot = FindSlot(Ship);
	if (!NewSlot || NewSlot->State != ETDSquadSlotState::Alive || !IsValid(NewSlot->Ship) || TDSquad::IsDead(NewSlot->Ship))
	{
		if (!bForced)
		{
			Refuse(ETDSwitchRefusal::TargetDown, Ship);
		}
		return false;
	}
	if (!bForced)
	{
		if (RealNow() < LastSwitchTime + SwitchCooldown)
		{
			Refuse(ETDSwitchRefusal::Cooldown, Ship);
			return false;
		}
		// M6: no manual switch mid-jump or mid-landing (an auto-switch on death still goes through).
		const ATDShip* Current = GetActiveShip();
		if (Current && IsShipAlive(ActiveShipId))
		{
			if (TDSquad::GetHyperdriveState(Current) != ETDHyperdriveState::Idle)
			{
				Refuse(ETDSwitchRefusal::HyperdriveActive, Ship);
				return false;
			}
			if (TDSquad::IsLandingBusy(Current))
			{
				Refuse(ETDSwitchRefusal::Landing, Ship);
				return false;
			}
		}
	}

	const ETDShipId OldId = ActiveShipId;
	FTDSquadSlot* OldSlot = FindSlot(OldId);
	ATDShip* NewShip = NewSlot->Ship;
	ATDShip* OldShip = OldSlot ? OldSlot->Ship.Get() : nullptr;

	// Capture velocities: possession must never change how either ship is moving.
	const FVector NewVel = TDSquad::GetVelocity(NewShip);
	const FVector OldVel = OldShip ? TDSquad::GetVelocity(OldShip) : FVector::ZeroVector;
	// The camera we cut FROM is the old ship's chase camera, before it goes dark.
	const FTransform FromCamera = OldShip ? TDSquad::GetChaseCameraTransform(OldShip) : NewShip->GetActorTransform();
	const double PrevSwitchTime = LastSwitchTime;

	// Switch Strike ends on the next switch (no chaining); this switch may arm a fresh one.
	const bool bArmSwitchStrike = (Now() - LastPowerTime) <= SwitchStrikeWindow;
	EndSwitchStrike();

	// 1) Release the target from its wingman AI.
	if (NewSlot->Wingman && NewSlot->Wingman->GetPawn() == NewShip)
	{
		NewSlot->Wingman->UnPossess();
	}

	// 2) Player takes the new ship this frame (APlayerController::Possess unpossesses the old one).
	if (APlayerController* PC = GetSquadPlayerController())
	{
		PC->Possess(NewShip);
	}
	ActiveShipId = Ship;
	TDSquad::SetPlayerControlled(NewShip, true);

	// 3) The ship we left goes to its wingman, unless it's already down (it stays dormant).
	if (OldShip && OldSlot)
	{
		TDSquad::SetPlayerControlled(OldShip, false);
		if (OldSlot->State == ETDSquadSlotState::Alive)
		{
			if (ATDWingmanAIController* Wing = SpawnWingmanFor(*OldSlot))
			{
				Wing->Possess(OldShip);
				Wing->SetPassive(bWingmenPassive);
			}
		}
		TDSquad::SetVelocity(OldShip, OldVel);
	}
	TDSquad::SetVelocity(NewShip, NewVel);

	if (const TObjectPtr<UTDShipVisuals>* V = ShipVisuals.Find(Ship); V && *V)
	{
		TDSquad::PlayShipCue(NewShip, (*V)->SwitchIn, true);
	}
	if (const TObjectPtr<UTDShipVisuals>* V = ShipVisuals.Find(OldId); V && *V && OldShip)
	{
		TDSquad::PlayShipCue(OldShip, (*V)->SwitchOut, true);
	}

	// 4) Switch-in grace (ENG-03) and Switch Strike (ENG-07).
	TDSquad::SetInvulnerable(NewShip, SwitchInInvulnerability);
	if (bArmSwitchStrike)
	{
		StartSwitchStrike(NewShip);
	}

	LastSwitchTime = RealNow();
	TDSquad::AddShipSwitchStat(this);
	TDSquad::BroadcastShipSwitched(this, Ship, OldId);

	// C-03 #1: the cut payload, same frame.
	const FTDShipSpec& Spec = GetSpec(Ship);
	FTDSwitchCut Cut;
	Cut.NewShip = Ship;
	Cut.OldShip = OldId;
	Cut.NewShipActor = NewShip;
	Cut.OldShipActor = OldShip;
	Cut.PilotName = Spec.PilotName;
	Cut.ShipName = Spec.ShipName;
	Cut.RoleName = Spec.RoleName;
	Cut.NameCardText = FText::Format(NSLOCTEXT("TDSquad", "NameCard", "{0} \u00B7 {1}"), Spec.PilotName, Spec.ShipName);
	Cut.SquadColor = Spec.SquadColor;
	Cut.bAutoSwitch = bForced;
	Cut.bSwitchStrike = bArmSwitchStrike;
	Cut.bQuickCut = (LastSwitchTime - PrevSwitchTime) < CutTiming.QuickCutWindowSeconds;
	Cut.SwitchIndex = ++SwitchCount;
	Cut.WorldTimeSeconds = static_cast<float>(LastSwitchTime);
	Cut.FromCamera = FromCamera;
	Cut.ToCamera = TDSquad::GetChaseCameraTransform(NewShip);
	Cut.Timing = CutTiming;
	LastSwitchCut = Cut;
	OnSwitchCut.Broadcast(LastSwitchCut);
	return true;
}

// ---------------------------------------------------------------------------------------------
// Powers & Switch Strike
// ---------------------------------------------------------------------------------------------

void ATDSquadManager::HandleWaveCleared(int32 WaveNumber)
{
	bBetweenWaves = true;
}

void ATDSquadManager::HandleWaveStarted(int32 WaveNumber, float Intensity01)
{
	bBetweenWaves = false;
}

void ATDSquadManager::NotifyPowerUsed(ETDShipId Ship)
{
	// Only the player's ship uses powers, but record any use: the window is about the player's timing.
	LastPowerTime = Now();
}

void ATDSquadManager::HandlePowerUsed(ETDShipId Ship, FName PowerName)
{
	NotifyPowerUsed(Ship);
}

void ATDSquadManager::StartSwitchStrike(ATDShip* OnShip)
{
	if (!OnShip)
	{
		return;
	}
	SwitchStrikeShip = OnShip;
	SwitchStrikeEndTime = Now() + SwitchStrikeDuration;
	TDSquad::SetOutgoingDamageMultiplier(OnShip, SwitchStrikeKey, SwitchStrikeDamageMultiplier);
	// Consume the power so one power can't arm two chained switches.
	LastPowerTime = -1000.0;
}

void ATDSquadManager::EndSwitchStrike()
{
	if (ATDShip* Ship = SwitchStrikeShip.Get())
	{
		TDSquad::ClearOutgoingDamageMultiplier(Ship, SwitchStrikeKey);
	}
	SwitchStrikeShip.Reset();
	SwitchStrikeEndTime = -1000.0;
}

bool ATDSquadManager::IsSwitchStrikeActive() const
{
	return SwitchStrikeShip.IsValid() && Now() < SwitchStrikeEndTime;
}

float ATDSquadManager::GetDamageMultiplier(const AActor* SourceShip) const
{
	return (IsSwitchStrikeActive() && SourceShip && SourceShip == SwitchStrikeShip.Get())
		? SwitchStrikeDamageMultiplier
		: 1.f;
}

// ---------------------------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------------------------

void ATDSquadManager::SetWingmenPassive(bool bPassive)
{
	bWingmenPassive = bPassive;
	for (FTDSquadSlot& Slot : Slots)
	{
		if (Slot.Wingman)
		{
			Slot.Wingman->SetPassive(bPassive);
		}
	}
}

void ATDSquadManager::IssueFocusCommand(AActor* Target)
{
	if (!Target || TDSquad::IsDead(Target))
	{
		FocusTarget.Reset();
		FocusEndTime = -1000.0;
		return;
	}
	FocusTarget = Target;
	FocusEndTime = Now() + FocusCommandSeconds;
	for (FTDSquadSlot& Slot : Slots)
	{
		if (Slot.Wingman && Slot.ShipId != ActiveShipId)
		{
			Slot.Wingman->SetFocusTarget(Target);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Death, respawn, wipe (ENG-03)
// ---------------------------------------------------------------------------------------------

void ATDSquadManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double T = Now();

	if (SwitchStrikeShip.IsValid() && T >= SwitchStrikeEndTime)
	{
		EndSwitchStrike();
	}

	// ENG-03: the hull doesn't regenerate in combat but slowly repairs between waves.
	if (bBetweenWaves && BetweenWaveHullRepairPerSecond > 0.f)
	{
		for (FTDSquadSlot& Slot : Slots)
		{
			if (Slot.State == ETDSquadSlotState::Alive)
			{
				TDSquad::RepairHull(Slot.Ship, BetweenWaveHullRepairPerSecond * DeltaSeconds);
			}
		}
	}

	for (FTDSquadSlot& Slot : Slots)
	{
		switch (Slot.State)
		{
		case ETDSquadSlotState::Alive:
			if (!IsValid(Slot.Ship) || TDSquad::IsDead(Slot.Ship))
			{
				HandleShipDeath(Slot.ShipId);
			}
			break;

		case ETDSquadSlotState::Dying:
			if (ATDShip* Tumbling = Slot.Ship)
			{
				// Spin and drift to a stop while the flight model is parked.
				Tumbling->AddActorWorldOffset(Slot.TumbleVelocity * DeltaSeconds);
				Tumbling->AddActorLocalRotation(DeathTumbleSpin * DeltaSeconds);
				Slot.TumbleVelocity *= FMath::Exp(-DeathTumbleDrag * DeltaSeconds);
			}
			if (T >= Slot.DownedAtTime + DeathTumbleSeconds)
			{
				FinishDying(Slot);
			}
			break;

		case ETDSquadSlotState::Downed:
			if (!bWipeHandledByFlow && T >= Slot.RespawnAtTime)
			{
				RespawnShip(Slot);
			}
			break;

		default:
			break;
		}
	}
}

void ATDSquadManager::HandleShipDeath(ETDShipId Ship)
{
	FTDSquadSlot* Slot = FindSlot(Ship);
	if (!Slot || Slot->State != ETDSquadSlotState::Alive)
	{
		return;
	}
	const double T = Now();
	Slot->State = ETDSquadSlotState::Dying;
	Slot->DownedAtTime = T;
	Slot->RespawnAtTime = T + RespawnDelay;

	// M3: the ship is out of the fight right now. Take its motion over for the tumble, stop the flight
	// model (so a held stick or boost does nothing), block its guns and end any jump.
	if (ATDShip* Dead = Slot->Ship)
	{
		Slot->TumbleVelocity = TDSquad::GetVelocity(Dead);
		TDSquad::SendNeutralInput(Dead);
		TDSquad::SetTriggerHeld(Dead, ETDWeaponSlot::Primary, false);
		TDSquad::SetTriggerHeld(Dead, ETDWeaponSlot::Secondary, false);
		TDSquad::SetWeaponsBlocked(Dead, DownedWeaponsKey, true);
		TDSquad::ClearAim(Dead);
		TDSquad::StopHyperdrive(Dead);
		TDSquad::SetFlightParked(Dead, true);
		if (const TObjectPtr<UTDShipVisuals>* V = ShipVisuals.Find(Ship); V && *V)
		{
			TDSquad::PlayShipCue(Dead, (*V)->HullHit, true);
		}
	}

	if (Slot->Wingman)
	{
		Slot->Wingman->NotifyShipDowned();
	}
	if (SwitchStrikeShip.Get() == Slot->Ship)
	{
		EndSwitchStrike();
	}
	TDSquad::BroadcastShipDowned(this, Ship);

	if (GetLivingShips().Num() == 0)
	{
		HandleSquadWiped();
	}
}

void ATDSquadManager::FinishDying(FTDSquadSlot& Slot)
{
	Slot.State = ETDSquadSlotState::Downed;
	if (const TObjectPtr<UTDShipVisuals>* V = ShipVisuals.Find(Slot.ShipId); V && *V)
	{
		TDSquad::PlayShipCue(Slot.Ship, (*V)->Explosion, false);
	}
	if (ATDShip* Ship = Slot.Ship)
	{
		TDSquad::SetTriggerHeld(Ship, ETDWeaponSlot::Primary, false);
		TDSquad::SetTriggerHeld(Ship, ETDWeaponSlot::Secondary, false);
		Ship->SetActorHiddenInGame(true);
		Ship->SetActorEnableCollision(false);
	}

	// The tumble is over: if the player was in this ship, cut to the healthiest survivor.
	if (Slot.ShipId == ActiveShipId)
	{
		ETDShipId Best = ETDShipId::Rift;
		if (FindHealthiestLivingShip(Slot.ShipId, Best))
		{
			SwitchToInternal(Best, /*bForced*/ true);
		}
	}
}

void ATDSquadManager::RespawnShip(FTDSquadSlot& Slot)
{
	ATDShip* Ship = Slot.Ship;
	if (!IsValid(Ship))
	{
		Slot.State = ETDSquadSlotState::NotSpawned;
		return;
	}

	// Warp back in behind the leader (or in place if there is no other leader).
	const ATDShip* Leader = GetActiveShip();
	FVector Loc = Ship->GetActorLocation();
	FRotator Facing = Ship->GetActorRotation();
	FVector Vel = FVector::ZeroVector;
	if (Leader && Leader != Ship && IsShipAlive(ActiveShipId))
	{
		Facing = Leader->GetActorRotation();
		Loc = GetFormationSlotLocation(Slot.ShipId) - Leader->GetActorForwardVector() * RespawnBehindDistance;
		Vel = TDSquad::GetVelocity(Leader);
	}

	RestoreShipForFlight(Ship, Loc, Facing, Vel);
	TDSquad::SetInvulnerable(Ship, RespawnInvulnerability);
	Slot.State = ETDSquadSlotState::Alive;
	Slot.RespawnAtTime = 0.0;

	if (Slot.ShipId != ActiveShipId)
	{
		if (ATDWingmanAIController* Wing = SpawnWingmanFor(Slot))
		{
			if (Wing->GetPawn() != Ship)
			{
				Wing->Possess(Ship);
			}
			Wing->NotifyShipRespawned();
		}
	}
	bSquadWiped = false;
	if (const TObjectPtr<UTDShipVisuals>* V = ShipVisuals.Find(Slot.ShipId); V && *V)
	{
		TDSquad::PlayShipCue(Ship, (*V)->RespawnWarpIn, false);
	}
	TDSquad::BroadcastShipRespawned(this, Slot.ShipId);
}

void ATDSquadManager::RestoreShipForFlight(ATDShip* Ship, const FVector& Location, const FRotator& Facing, const FVector& Velocity)
{
	if (!Ship)
	{
		return;
	}
	Ship->SetActorHiddenInGame(false);
	Ship->SetActorEnableCollision(true);
	TDSquad::SetFlightParked(Ship, false);
	TDSquad::TeleportShip(Ship, Location, Facing, false);
	TDSquad::ResetFlightState(Ship, /*bKeepVelocity*/ false);
	TDSquad::SetVelocity(Ship, Velocity);
	TDSquad::RestoreFullHealth(Ship);
	TDSquad::SetWeaponsBlocked(Ship, DownedWeaponsKey, false);
	TDSquad::ResetWeapons(Ship);
	TDSquad::ClearAim(Ship);
	TDSquad::ResetPowers(Ship);
	TDSquad::EnableHyperdrive(Ship);
}

void ATDSquadManager::HandleSquadWiped()
{
	if (bSquadWiped)
	{
		return;
	}
	bSquadWiped = true;
	TDSquad::BroadcastSquadWiped(this);
	// With a game flow, it owns the restart (C-03 #3 "CUT!" -> next take) and calls SpawnSquad again.
	// Without one (sandbox), downed ships simply keep their respawn timers.
	bWipeHandledByFlow = TDSquad::ReportPlayerDefeated(this);
}

bool ATDSquadManager::FindHealthiestLivingShip(ETDShipId Except, ETDShipId& OutShip) const
{
	float BestHealth = -1.f;
	bool bFound = false;
	for (const ETDShipId Id : SwitchOrder)
	{
		if (Id == Except || !IsShipAlive(Id))
		{
			continue;
		}
		const float H = TDSquad::GetHealth01(GetShip(Id));
		if (H > BestHealth)
		{
			BestHealth = H;
			OutShip = Id;
			bFound = true;
		}
	}
	return bFound;
}

// ---------------------------------------------------------------------------------------------
// ITDHUDData
// ---------------------------------------------------------------------------------------------

TArray<FTDShipHUD> ATDSquadManager::GetSquadHUD() const
{
	TArray<FTDShipHUD> Out;
	Out.Reserve(SwitchOrder.Num());
	for (const ETDShipId Id : SwitchOrder)
	{
		const FTDSquadSlot* Slot = FindSlot(Id);
		const ATDShip* Ship = Slot ? Slot->Ship.Get() : nullptr;
		const bool bAlive = Slot && Slot->State == ETDSquadSlotState::Alive && Ship;

		FTDShipHUD Card;
		Card.Ship = Id;
		Card.bAlive = bAlive;
		Card.bPlayerControlled = (Id == ActiveShipId);
		Card.RespawnSeconds = GetRespawnSeconds(Id);
		Card.Health01 = bAlive ? TDSquad::GetHull01(Ship) : 0.f;
		Card.Shield01 = bAlive ? TDSquad::GetShield01(Ship) : 0.f;
		Card.Boost01 = Ship ? TDSquad::GetBoost01(Ship) : 0.f;
		Card.Heat01 = Ship ? TDSquad::GetHeat01(Ship, ETDWeaponSlot::Primary) : 0.f;
		Card.SecondaryAmmo = Ship ? TDSquad::GetAmmo(Ship, ETDWeaponSlot::Secondary) : 0;
		Card.Ability = GetSpec(Id).UnlockableAbility;
		if (Ship)
		{
			TDSquad::GetPowerCooldowns(Ship, Card.PowerCooldown01, Card.Power2Cooldown01, Card.AbilityCooldown01);
		}
		Out.Add(Card);
	}
	return Out;
}

AActor* ATDSquadManager::GetCurrentTarget() const
{
	return TDSquad::GetCurrentTarget(GetActiveShip());
}

TArray<AActor*> ATDSquadManager::GetLockedTargets() const
{
	TArray<AActor*> Out;
	TDSquad::GetLockedTargets(GetActiveShip(), Out);
	return Out;
}

FTDWaveInfo ATDSquadManager::GetWaveInfo() const
{
	return TDSquad::GetWaveInfo(this, WaveDirectorCache);
}

float ATDSquadManager::GetSpeed01() const
{
	return TDSquad::GetSpeed01(GetActiveShip());
}

TArray<FTDTargetMarker> ATDSquadManager::GetTargetMarkers() const
{
	TArray<FTDTargetMarker> Out;
	TDSquad::GetTargetMarkers(GetActiveShip(), Out);
	return Out;
}

ETDMissileWarning ATDSquadManager::GetMissileWarning() const
{
	return TDSquad::GetMissileWarning(GetActiveShip());
}

bool ATDSquadManager::GetLeadIndicator(FVector& OutWorldLocation) const
{
	return TDSquad::GetLeadIndicator(GetActiveShip(), OutWorldLocation);
}

bool ATDSquadManager::IsOutOfBounds() const
{
	return TDSquad::IsOutOfBounds(GetActiveShip());
}

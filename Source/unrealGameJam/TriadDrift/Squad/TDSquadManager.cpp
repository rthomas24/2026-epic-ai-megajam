// Triad Drift - Squad (Teammate 3). ENG-07: squad manager & instant switching (PROTECT), ENG-03 respawn.

#include "TriadDrift/Squad/TDSquadManager.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"

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
	TDSquad::BindActorKilled(this);
	// Arriving after the flow's last broadcast (level travel): start from its current line.
	ObjectiveText = TDSquad::GetCurrentObjectiveText(this);
}

void ATDSquadManager::EndPlay(const EEndPlayReason::Type Reason)
{
	TDSquad::UnbindPowerUsed(this);
	TDSquad::UnbindWaveEvents(this);
	TDSquad::UnbindActorKilled(this);
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
	BarkLimiter.Reset();

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
			// A retake: whatever the ship was doing (downed, mid-jump, landing), stop it at once.
			TDSquad::ResetTravelForRetake(Slot->Ship);
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
		Slot->bLowHullBarkArmed = true;
		SpawnWingmanFor(*Slot);
	}

	// M7: squad ships never physically block each other (the AI keeps them apart instead).
	for (FTDSquadSlot& A : Slots)
	{
		for (FTDSquadSlot& B : Slots)
		{
			if (A.Ship && B.Ship && A.Ship != B.Ship)
			{
				TDSquad::IgnoreEachOther(A.Ship, B.Ship);
			}
		}
	}

	// Hand out control: wingmen first, then the player takes the starting ship. A new take is active.
	if (ActiveShipId != StartingShip && GetShip(ActiveShipId) != nullptr)
	{
		LogSwitch(ActiveShipId, StartingShip, ETDSwitchCause::Retake);
		LastSwitchCause = ETDSwitchCause::Retake;
	}
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
		}
		// Every slot, the player's too (review BLOCKER): a wipe left all three controllers Downed, and
		// the player's one takes its ship back on the first switch away.
		Slot.Wingman->NotifyShipRespawned();
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
	// Review minor 8: a wipe the flow took restarts the scene; nothing respawns on this take's timers.
	if (!Slot || Slot->State == ETDSquadSlotState::Alive || Slot->State == ETDSquadSlotState::NotSpawned || (bSquadWiped && bWipeHandledByFlow))
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
	// M7: the V hangs off the leader's heading and WORLD up, never its roll, so a barrel roll doesn't
	// swing the two slots through each other (or through the player).
	const FVector Fwd = Leader->GetActorForwardVector();
	const FVector Up = FMath::Abs(FVector::DotProduct(Fwd, FVector::UpVector)) > 0.95f ? Leader->GetActorUpVector() : FVector::UpVector;
	const FMatrix Frame = FRotationMatrix::MakeFromXZ(Fwd, Up);
	return Leader->GetActorLocation() + Frame.TransformVector(Offset);
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
	if (SwitchWithCause(Ship, false, ETDSwitchCause::Manual))
	{
		return true;
	}
	BufferIfCoolingDown(2, Ship);
	return false;
}

bool ATDSquadManager::RequestSwitchNext()
{
	TGuardValue<ETDSwitchCause> Cause(CurrentSwitchCause, ETDSwitchCause::Manual);
	if (CycleSwitch(+1))
	{
		return true;
	}
	BufferIfCoolingDown(+1, ETDShipId::Rift);
	return false;
}

bool ATDSquadManager::RequestSwitchPrev()
{
	TGuardValue<ETDSwitchCause> Cause(CurrentSwitchCause, ETDSwitchCause::Manual);
	if (CycleSwitch(-1))
	{
		return true;
	}
	BufferIfCoolingDown(-1, ETDShipId::Rift);
	return false;
}

void ATDSquadManager::BufferIfCoolingDown(int32 Kind, ETDShipId Ship)
{
	// m14: a press during the cooldown isn't lost: it runs once the cooldown ends (double-tap Tab
	// reaches the third ship). Only the latest press is kept, and only for one cooldown length.
	if (RealNow() < LastSwitchTime + SwitchCooldown)
	{
		PendingSwitchKind = Kind;
		PendingSwitchShip = Ship;
		PendingSwitchExpire = LastSwitchTime + 2.0 * SwitchCooldown;
	}
}

void ATDSquadManager::RunPendingSwitch()
{
	if (PendingSwitchKind == 0 || RealNow() < LastSwitchTime + SwitchCooldown)
	{
		return;
	}
	const int32 Kind = PendingSwitchKind;
	PendingSwitchKind = 0;
	if (RealNow() > PendingSwitchExpire)
	{
		return;
	}
	TGuardValue<ETDSwitchCause> Cause(CurrentSwitchCause, ETDSwitchCause::Buffered);
	if (Kind == 2)
	{
		SwitchToInternal(PendingSwitchShip, false);
	}
	else
	{
		CycleSwitch(Kind);
	}
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

bool ATDSquadManager::SwitchWithCause(ETDShipId Ship, bool bForced, ETDSwitchCause Cause)
{
	TGuardValue<ETDSwitchCause> Guard(CurrentSwitchCause, Cause);
	return SwitchToInternal(Ship, bForced);
}

void ATDSquadManager::LogSwitch(ETDShipId From, ETDShipId To, ETDSwitchCause Cause) const
{
	// Every change of the player's ship, with why (T2's wave-5 run: "Rift became Lancer, nothing in the log").
	UE_LOG(LogTriadDrift, Display, TEXT("TDSQUAD switch %s -> %s cause=%s"), *UEnum::GetDisplayValueAsText(From).ToString(),
		*UEnum::GetDisplayValueAsText(To).ToString(), *UEnum::GetDisplayValueAsText(Cause).ToString());
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
		// M6: no manual switch mid-jump, mid-landing or during atmospheric entry (T1's IsSwitchLocked);
		// an auto-switch on death still goes through.
		const ATDShip* Current = GetActiveShip();
		FText LockReason;
		if (Current && IsShipAlive(ActiveShipId) && TDSquad::IsSwitchLocked(Current, LockReason))
		{
			LastRefusalText = LockReason;
			LastLockRefusalRealTime = RealNow();
			const ETDSwitchRefusal Why = TDSquad::GetHyperdriveState(Current) != ETDHyperdriveState::Idle ? ETDSwitchRefusal::HyperdriveActive
				: TDSquad::IsLandingBusy(Current) ? ETDSwitchRefusal::Landing : ETDSwitchRefusal::Locked;
			Refuse(Why, Ship);
			return false;
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
	// Review MAJOR (ENG-03): Tab during your own death tumble is a rescue, not a manual switch: it gets
	// the auto-switch grace (and FinishDying then has nothing left to switch).
	const bool bLeavingDowned = !IsShipAlive(OldId);

	// Switch Strike ends on the next switch (no chaining); this switch may arm a fresh one.
	const bool bArmSwitchStrike = (Now() - LastPowerTime) <= SwitchStrikeWindow;
	EndSwitchStrike();

	// 0) The ship we're leaving: release the player's held power/trigger inputs first (m15), so a
	// half-held dome or a charged Splitter doesn't fire under the AI.
	TDSquad::ReleasePlayerHolds(OldShip);

	// 1) Release the target from its wingman AI.
	if (NewSlot->Wingman && NewSlot->Wingman->GetPawn() == NewShip)
	{
		NewSlot->Wingman->UnPossess();
	}

	// A target the player picked by hand carries over the cut (T2).
	TDSquad::CarryManualTarget(OldShip, NewShip);

	// 2) Player takes the new ship this frame (APlayerController::Possess unpossesses the old one).
	if (APlayerController* PC = GetSquadPlayerController())
	{
		PC->Possess(NewShip);
	}
	LogSwitch(OldId, Ship, CurrentSwitchCause);
	LastSwitchCause = CurrentSwitchCause;
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

	if (const UTDShipVisuals* V = TDSquad::GetShipVisuals(NewShip))
	{
		TDSquad::PlayShipCue(NewShip, V->SwitchIn, true);
	}
	if (const UTDShipVisuals* V = TDSquad::GetShipVisuals(OldShip))
	{
		TDSquad::PlayShipCue(OldShip, V->SwitchOut, true);
	}

	// 4) Switch-in grace (ENG-03, M4): 2 s on an auto-switch; a manual switch gets a short grace, once
	// per ship per lockout, so Tab-spamming can't keep the squad invulnerable.
	if (bForced || bLeavingDowned)
	{
		TDSquad::SetInvulnerable(NewShip, AutoSwitchInvulnerability);
	}
	else if (RealNow() - NewSlot->LastManualGraceTime >= ManualSwitchGraceLockout)
	{
		TDSquad::SetInvulnerable(NewShip, ManualSwitchInvulnerability);
		NewSlot->LastManualGraceTime = RealNow();
	}
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
	Cut.SquadColor = UTDShipLibrary::GetSquadColor(Spec);
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
	if (!bForced)
	{
		RequestSquadBark(TDBarkIds::SwitchIn, Ship); // the new pilot answers the cut
	}
	return true;
}

// ---------------------------------------------------------------------------------------------
// Powers & Switch Strike
// ---------------------------------------------------------------------------------------------

namespace TDSquadManagerPrivate
{
	// Playtest log (dev): kills by squad ships, "steals" of the player's target, and every squad bark
	// asked for (played or held by the limiter). For judging how the wing feels from the cockpit.
	static TAutoConsoleVariable<int32> CVarLogEvents(TEXT("td.Squad.LogEvents"), 0,
		TEXT("1: log squad kills, steals of the player's target and barks (TDSQUAD lines)."), ECVF_Default);
}

void ATDSquadManager::NotifyWingmanEngaged(ETDShipId Ship)
{
	FTDSquadSlot* Slot = FindSlot(Ship);
	const ATDShip* Player = GetActiveShip();
	if (!Slot || !Slot->Ship || !Player || Ship == ActiveShipId || Slot->State != ETDSquadSlotState::Alive)
	{
		return;
	}
	if (RealNow() - Slot->LastEngageCallTime < EngageCallCooldownSeconds)
	{
		return;
	}
	const FName Line = TDBarkIds::EngageCallFor(Player->GetChaseCameraTransformNow(), Slot->Ship->GetActorLocation());
	if (RequestSquadBark(Line, Ship))
	{
		Slot->LastEngageCallTime = RealNow();
	}
}

bool ATDSquadManager::RequestSquadBark(FName BarkId, ETDShipId Speaker, ETDBarkPriority Priority)
{
	const bool bPlayed = BarkLimiter.TryConsume(BarkTuning, BarkId, Priority, RealNow());
	if (TDSquadManagerPrivate::CVarLogEvents.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogTriadDrift, Display, TEXT("TDSQUAD t=%.1f bark %s by %s: %s"), RealNow(), *BarkId.ToString(),
			*UEnum::GetDisplayValueAsText(Speaker).ToString(), bPlayed ? TEXT("played") : TEXT("held"));
	}
	if (!bPlayed)
	{
		return false;
	}
	TDSquad::RequestBark(this, BarkId, TDBarkIds::SpeakerFor(Speaker));
	return true;
}

void ATDSquadManager::HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
	if (TDSquadManagerPrivate::CVarLogEvents.GetValueOnGameThread() != 0 && UTDStatics::AreHostile(Killer, Victim))
	{
		const ATDShip* Player = GetActiveShip();
		const FTDSquadSlot* KillerSlot = Slots.FindByPredicate([Killer](const FTDSquadSlot& S) { return S.Ship == Killer; });
		if (KillerSlot)
		{
			const bool bSteal = Killer != Player && TDSquad::GetCurrentTarget(Player) == Victim;
			UE_LOG(LogTriadDrift, Display, TEXT("TDSQUAD t=%.1f kill by %s%s%s"), RealNow(), *UEnum::GetDisplayValueAsText(KillerSlot->ShipId).ToString(),
				Killer == Player ? TEXT(" (player)") : TEXT(" (wingman)"), bSteal ? TEXT(" STEAL of the player's target") : TEXT(""));
		}
	}
	// A wingman scored: that pilot crows about it.
	for (const FTDSquadSlot& Slot : Slots)
	{
		if (Slot.Ship && Killer == Slot.Ship && Slot.ShipId != ActiveShipId && Slot.State == ETDSquadSlotState::Alive
			&& UTDStatics::AreHostile(Killer, Victim))
		{
			RequestSquadBark(TDBarkIds::WingmanKill, Slot.ShipId);
			return;
		}
	}
}

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
	for (const ETDShipId Id : SwitchOrder)
	{
		if (Id != ActiveShipId && IsShipAlive(Id))
		{
			RequestSquadBark(TDBarkIds::FocusAck, Id);
			break;
		}
	}
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

	RunPendingSwitch();

	// Low hull line, once per dip (re-armed after a repair or respawn).
	for (FTDSquadSlot& Slot : Slots)
	{
		if (Slot.State != ETDSquadSlotState::Alive || !Slot.Ship)
		{
			continue;
		}
		const float Hull = TDSquad::GetHull01(Slot.Ship);
		if (Slot.bLowHullBarkArmed && Hull > 0.f && Hull < BarkTuning.LowHullFraction)
		{
			// Live playtest: held by the limiter, the line was retried every frame and played long after
			// the moment. It gets LowHullBarkWindowSeconds, then this dip goes unannounced.
			if (Slot.LowHullSince < 0.0)
			{
				Slot.LowHullSince = RealNow();
			}
			if (RealNow() - Slot.LowHullSince > LowHullBarkWindowSeconds || RequestSquadBark(TDBarkIds::LowHull, Slot.ShipId))
			{
				Slot.bLowHullBarkArmed = false;
				Slot.LowHullSince = -1.0;
			}
		}
		else if (!Slot.bLowHullBarkArmed && Hull > BarkTuning.LowHullRearmFraction)
		{
			Slot.bLowHullBarkArmed = true;
		}
	}

	// ENG-03: the hull doesn't regenerate in combat but slowly repairs between waves.
	if (bBetweenWaves && BetweenWaveHullRepairPercentPerSecond > 0.f)
	{
		for (FTDSquadSlot& Slot : Slots)
		{
			if (Slot.State == ETDSquadSlotState::Alive)
			{
				// features.md ENG-03: a percentage of the ship's own hull (Bastion and Lancer alike).
				TDSquad::RepairHull(Slot.Ship, TDSquad::GetMaxHull(Slot.Ship) * BetweenWaveHullRepairPercentPerSecond * 0.01f * DeltaSeconds);
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
		if (const UTDShipVisuals* V = TDSquad::GetShipVisuals(Dead))
		{
			TDSquad::PlayShipCue(Dead, V->HullHit, true);
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
	else
	{
		// The healthiest survivor calls it ("Kato's down!") - unless everyone is going down this same
		// frame, when the squad-wipe line says it all.
		ETDShipId Caller = ETDShipId::Rift;
		if (FindHealthiestLivingShip(Ship, Caller) && !TDSquad::IsDead(GetShip(Caller)))
		{
			RequestSquadBark(TDBarkIds::ShipDownFor(Ship), Caller, ETDBarkPriority::High);
		}
	}
}

void ATDSquadManager::FinishDying(FTDSquadSlot& Slot)
{
	Slot.State = ETDSquadSlotState::Downed;
	if (const UTDShipVisuals* V = TDSquad::GetShipVisuals(Slot.Ship))
	{
		TDSquad::PlayShipCue(Slot.Ship, V->Explosion, false);
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
			SwitchWithCause(Best, /*bForced*/ true, ETDSwitchCause::AutoOnDeath);
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
		Loc = TDSquad::FindSafeLocation(this, GetFormationSlotLocation(Slot.ShipId) - Leader->GetActorForwardVector() * RespawnBehindDistance, Ship);
		Vel = TDSquad::GetVelocity(Leader);
	}

	RestoreShipForFlight(Ship, Loc, Facing, Vel);
	TDSquad::SetInvulnerable(Ship, RespawnInvulnerability);
	Slot.State = ETDSquadSlotState::Alive;
	Slot.RespawnAtTime = 0.0;
	Slot.bLowHullBarkArmed = true;
	RequestSquadBark(TDBarkIds::Respawn, Slot.ShipId);

	if (ATDWingmanAIController* Wing = SpawnWingmanFor(Slot))
	{
		if (Slot.ShipId != ActiveShipId && Wing->GetPawn() != Ship)
		{
			Wing->Possess(Ship);
		}
		// Unconditionally (review BLOCKER): an in-place respawn of the player's ship must not leave its
		// controller Downed for when the player switches away.
		Wing->NotifyShipRespawned();
	}
	bSquadWiped = false;
	// m4: no game flow took the wipe and the player sits in a downed ship: cut to the one that's back.
	if (Slot.ShipId != ActiveShipId && !IsShipAlive(ActiveShipId))
	{
		SwitchWithCause(Slot.ShipId, /*bForced*/ true, ETDSwitchCause::RespawnRescue);
	}
	if (const UTDShipVisuals* V = TDSquad::GetShipVisuals(Ship))
	{
		TDSquad::PlayShipCue(Ship, V->RespawnWarpIn, false);
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
	TDSquad::AbortLanding(Ship); // a ship that died mid-landing comes back flying (T1)
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
	RequestSquadBark(TDBarkIds::SquadWipe, ActiveShipId, ETDBarkPriority::Critical);
	// With a game flow, it owns the restart (C-03 #3 "CUT!" -> next take) and calls SpawnSquad again.
	// Without one (sandbox), downed ships simply keep their respawn timers.
	// m1: decide before the call (a flow with no retake delay respawns the squad inside it).
	bWipeHandledByFlow = true;
	const bool bHandled = TDSquad::ReportPlayerDefeated(this);
	// Review minor 3: with no retake delay the flow's SpawnSquad already ran inside that call and
	// started a clean take; don't mark the new take's deaths as "the flow handles it".
	bWipeHandledByFlow = bSquadWiped && bHandled;
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
		// N-004: only the ability the ship has EARNED, readiness 0 while locked.
		TDSquad::GetEarnedAbility(Ship, Card.Ability, Card.AbilityCooldown01);
		if (Ship)
		{
			TDSquad::GetPowerCooldowns(Ship, Card.PowerCooldown01, Card.Power2Cooldown01);
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

const FName ATDSquadManager::ObjectiveMarkerTag = TDTags::ObjectiveMarker;

TArray<AActor*> ATDSquadManager::GetObjectiveMarkers() const
{
	UWorld* World = GetWorld();
	TArray<AActor*> Out;
	if (!World)
	{
		return Out;
	}
	const double Now = World->GetTimeSeconds();
	if (Now >= ObjectiveRefreshAt || ObjectiveRefreshAt - Now > ObjectiveRefreshSeconds)
	{
		ObjectiveRefreshAt = Now + ObjectiveRefreshSeconds;
		ObjectiveCache.Reset();
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(ObjectiveMarkerTag))
			{
				ObjectiveCache.Add(*It);
			}
		}
	}
	// Filtered on every read: a destroyed or dead marker (a generator just killed) disappears at once.
	for (const TWeakObjectPtr<AActor>& Weak : ObjectiveCache)
	{
		AActor* A = Weak.Get();
		if (IsValid(A) && !TDSquad::IsDead(A) && A->ActorHasTag(ObjectiveMarkerTag))
		{
			Out.Add(A);
		}
	}
	if (Out.Num() > 0)
	{
		return Out;
	}

	// Fallback: a wave is on and the whole fight is off the player's screen -> point at the nearest enemy.
	const ATDShip* Player = GetActiveShip();
	if (!Player || !GetWaveInfo().bWaveActive)
	{
		return Out;
	}
	static TArray<AActor*> Hostiles; // game thread only
	TDSquad::GetPlayerHostiles(World, Hostiles);
	const FTransform Cam = Player->GetChaseCameraTransformNow();
	const float CosHalfView = FMath::Cos(FMath::DegreesToRadians(Player->GetChaseCameraFOVNow() * 0.5f));
	AActor* Nearest = nullptr;
	float Best = TNumericLimits<float>::Max();
	for (AActor* H : Hostiles)
	{
		const FVector To = H->GetActorLocation() - Cam.GetLocation();
		if (FVector::DotProduct(To.GetSafeNormal(), Cam.GetRotation().GetForwardVector()) > CosHalfView)
		{
			return Out; // something to fight is already in view: no marker needed
		}
		const float D = To.SizeSquared();
		if (D < Best)
		{
			Best = D;
			Nearest = H;
		}
	}
	if (Nearest)
	{
		Out.Add(Nearest);
	}
	return Out;
}

void ATDSquadManager::HandleObjectiveUpdated(FName ObjectiveId, int32 Step, FText Text)
{
	ObjectiveText = Text;
}

FText ATDSquadManager::GetBoundaryWarningText() const
{
	return TDSquad::GetBoundaryWarningText(GetActiveShip());
}

TArray<FTDSquadMarker> ATDSquadManager::GetSquadMarkers() const
{
	TArray<FTDSquadMarker> Out;
	Out.Reserve(Slots.Num());
	for (const FTDSquadSlot& Slot : Slots)
	{
		FTDSquadMarker M;
		M.Ship = Slot.ShipId;
		M.Actor = Slot.Ship;
		M.bPlayerControlled = Slot.ShipId == ActiveShipId;
		M.bAlive = Slot.State == ETDSquadSlotState::Alive && IsValid(Slot.Ship) && !TDSquad::IsDead(Slot.Ship);
		// Engaging = the wingman flying it has a live target (Engage and defending the player).
		if (M.bAlive && !M.bPlayerControlled && Slot.Wingman && Slot.Wingman->GetPawn() == Slot.Ship)
		{
			const AActor* T = Slot.Wingman->GetCurrentTarget();
			M.bEngaging = IsValid(T) && !TDSquad::IsDead(T);
		}
		Out.Add(M);
	}
	return Out;
}

FText ATDSquadManager::GetInteractPrompt() const
{
	const FText Land = TDSquad::GetLandPrompt(GetActiveShip());
	if (!Land.IsEmpty())
	{
		return Land;
	}
	// A refused switch (mid-jump, landing, atmospheric entry) says why for a moment instead of failing
	// silently. Cooldown refusals stay silent: that press is buffered and goes through.
	if (!LastRefusalText.IsEmpty() && RealNow() - LastLockRefusalRealTime < RefusalPromptSeconds)
	{
		return LastRefusalText;
	}
	return FText::GetEmpty();
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

float ATDSquadManager::GetHyperdriveCharge01() const
{
	return TDSquad::GetHyperdriveCharge01(GetActiveShip());
}

FText ATDSquadManager::GetHyperdriveBlockText() const
{
	return TDSquad::GetHyperdriveBlockText(GetActiveShip());
}

float ATDSquadManager::GetAltitude() const
{
	return TDSquad::GetAltitudeMeters(GetActiveShip());
}

bool ATDSquadManager::IsOutOfBounds() const
{
	return TDSquad::IsOutOfBounds(GetActiveShip());
}

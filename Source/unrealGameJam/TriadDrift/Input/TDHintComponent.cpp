// Triad Drift - first-minute teaching. Owner: Teammate 1 (Input).

#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Waves/TDWaveTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreMisc.h"

#define LOCTEXT_NAMESPACE "TDHints"

namespace TDHints
{
	static const TCHAR* SlotName = TEXT("TD_Hints");

	static TAutoConsoleVariable<int32> CVarHints(TEXT("TD.Hints"), 1,
		TEXT("First-minute control hints: 1 on (default), 0 off for this session."));

	static const ETDHint Core[] = { ETDHint::Fly, ETDHint::Fire, ETDHint::Boost, ETDHint::Dodge, ETDHint::Switch,
		ETDHint::Power, ETDHint::TargetCycle };

	static bool IsCore(ETDHint H)
	{
		for (ETDHint C : Core) { if (C == H) { return true; } }
		return false;
	}
}

UTDHintComponent::UTDHintComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

ATDSpacePlayerController* UTDHintComponent::GetPC() const
{
	return Cast<ATDSpacePlayerController>(GetOwner());
}

ATDShip* UTDHintComponent::GetFlownShip() const
{
	const ATDSpacePlayerController* PC = GetPC();
	ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
	return (Ship && Ship->IsPlayerFlown()) ? Ship : nullptr;
}

void UTDHintComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GIsAutomationTesting)
	{
		bPersist = false; // tests never touch the player's hint profile
	}
	Load();
	if (ATDSpacePlayerController* PC = GetPC())
	{
		PC->OnWeaponInput.AddDynamic(this, &UTDHintComponent::HandleWeaponInput);
		PC->OnPowerInput.AddDynamic(this, &UTDHintComponent::HandlePowerInput);
		PC->OnTargetCycleInput.AddDynamic(this, &UTDHintComponent::HandleTargetCycle);
	}
	BindBus();
}

void UTDHintComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDEventBus* Bus = bBusBound ? UTDEventBus::Get(this) : nullptr)
	{
		Bus->OnWaveStarted.RemoveDynamic(this, &UTDHintComponent::HandleWaveStarted);
		Bus->OnShipSwitched.RemoveDynamic(this, &UTDHintComponent::HandleShipSwitched);
		Bus->OnAbilityUnlocked.RemoveDynamic(this, &UTDHintComponent::HandleAbilityUnlocked);
		Bus->OnTipRequested.RemoveDynamic(this, &UTDHintComponent::HandleTip);
	}
	bBusBound = false;
	Super::EndPlay(EndPlayReason);
}

void UTDHintComponent::BindBus()
{
	if (bBusBound)
	{
		return;
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnWaveStarted.AddUniqueDynamic(this, &UTDHintComponent::HandleWaveStarted);
		Bus->OnShipSwitched.AddUniqueDynamic(this, &UTDHintComponent::HandleShipSwitched);
		Bus->OnAbilityUnlocked.AddUniqueDynamic(this, &UTDHintComponent::HandleAbilityUnlocked);
		Bus->OnTipRequested.AddUniqueDynamic(this, &UTDHintComponent::HandleTip);
		bBusBound = true;
	}
}

void UTDHintComponent::UseMemoryOnlyForTests()
{
	bPersist = false;
	Done.Reset();
	Queue.Reset();
	Current = ETDHint::None;
	CurrentAge = 0.f;
	Gap = 0.f;
	FlySeconds = 0.f;
	bEnabled = true;
}

void UTDHintComponent::Load()
{
	if (!bPersist || !UGameplayStatics::DoesSaveGameExist(TDHints::SlotName, 0))
	{
		return;
	}
	if (const UTDHintSave* SaveObj = Cast<UTDHintSave>(UGameplayStatics::LoadGameFromSlot(TDHints::SlotName, 0)))
	{
		Done = TSet<ETDHint>(SaveObj->Done);
		bEnabled = SaveObj->bEnabled;
	}
}

void UTDHintComponent::Save() const
{
	if (!bPersist)
	{
		return;
	}
	if (UTDHintSave* SaveObj = Cast<UTDHintSave>(UGameplayStatics::CreateSaveGameObject(UTDHintSave::StaticClass())))
	{
		SaveObj->Done = Done.Array();
		SaveObj->bEnabled = bEnabled;
		UGameplayStatics::SaveGameToSlot(SaveObj, TDHints::SlotName, 0);
	}
}

void UTDHintComponent::SetHintsEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
	Save();
}

void UTDHintComponent::ResetHints()
{
	Done.Reset();
	Queue.Reset();
	Current = ETDHint::None;
	Gap = 0.f;
	Save();
}

void UTDHintComponent::Offer(ETDHint Hint)
{
	if (Hint == ETDHint::None || Done.Contains(Hint) || Current == Hint || Queue.Contains(Hint))
	{
		return;
	}
	if (TDHints::IsCore(Hint))
	{
		Queue.Add(Hint);
	}
	else
	{
		Queue.Insert(Hint, 0); // a hint that just became relevant goes next
	}
}

void UTDHintComponent::Complete(ETDHint Hint)
{
	if (Hint == ETDHint::None || Done.Contains(Hint))
	{
		return;
	}
	Done.Add(Hint);
	Queue.Remove(Hint);
	if (Current == Hint)
	{
		Current = ETDHint::None;
		Gap = GapSeconds;
	}
	Save();
}

void UTDHintComponent::Observe(ETDHint Hint)
{
	// The player did it: done, whether its hint is up, queued or not offered yet (experienced players see nothing).
	Complete(Hint);
}

// ---- Event handlers

void UTDHintComponent::HandleWaveStarted(int32 WaveNumber, float Intensity01)
{
	const bool bFirstOffer = Queue.Num() == 0 && Current == ETDHint::None;
	for (ETDHint H : TDHints::Core)
	{
		Offer(H);
	}
	if (bFirstOffer && Queue.Num() > 0)
	{
		Gap = FMath::Max(Gap, FirstHintDelay);
	}
	if (WaveNumber >= 2)
	{
		Offer(ETDHint::Drift);
	}
}

void UTDHintComponent::HandleShipSwitched(ETDShipId NewShip, ETDShipId OldShip) { Observe(ETDHint::Switch); }

void UTDHintComponent::HandleAbilityUnlocked(ETDAbilityId Ability, ETDShipId Ship)
{
	LastAbility = Ability;
	Offer(ETDHint::Ability);
}

void UTDHintComponent::HandleTip(FName TipId)
{
	if (TipId == TDTipIds::Drift) { Offer(ETDHint::Drift); }
	else if (TipId == TDTipIds::Hyperdrive) { Offer(ETDHint::Hyperdrive); }
	else if (TipId == TDTipIds::Switching) { Offer(ETDHint::Switch); }
}

void UTDHintComponent::HandleWeaponInput(ETDWeaponSlot Slot, bool bHeld)
{
	if (bHeld && Slot == ETDWeaponSlot::Primary) { Observe(ETDHint::Fire); }
}

void UTDHintComponent::HandlePowerInput(ETDPowerInput Power)
{
	Observe(Power == ETDPowerInput::Ability ? ETDHint::Ability : ETDHint::Power);
}

void UTDHintComponent::HandleTargetCycle() { Observe(ETDHint::TargetCycle); }

// ---- Showing

bool UTDHintComponent::IsSuppressed() const
{
	if (!bEnabled || TDHints::CVarHints.GetValueOnGameThread() == 0)
	{
		return true;
	}
	const ATDSpacePlayerController* PC = GetPC();
	const ATDShip* Ship = GetFlownShip();
	if (!PC || !Ship || PC->IsGamePausedByPlayer() || PC->IsPaused())
	{
		return true; // on foot, between ships, paused
	}
	// Moments that own the screen, and the more important prompts (landing; the squad's switch refusal is checked first by
	// ATDSquadManager::GetInteractPrompt itself).
	if (const UTDLandingComponent* Landing = Ship->GetLanding())
	{
		if (Landing->IsBusy() || !Landing->GetLandPrompt().IsEmpty())
		{
			return true;
		}
	}
	if (const UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry())
	{
		if (Entry->IsEntering()) { return true; }
	}
	if (const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
	{
		if (Hyper->GetState() != ETDHyperdriveState::Idle) { return true; }
	}
	return false;
}

void UTDHintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	BindBus(); // the bus may arrive after the controller

	// Watch what the pilot does (state that has no event of its own).
	if (const ATDShip* Ship = GetFlownShip())
	{
		const FTDFlightInput& In = Ship->GetFlightInput();
		FlySeconds += (In.Aim.Size() > 0.25f) ? DeltaTime : 0.f;
		if (FlySeconds >= FlySecondsToComplete) { Observe(ETDHint::Fly); }
		if (Ship->IsBoosting()) { Observe(ETDHint::Boost); }
		if (Ship->IsDrifting()) { Observe(ETDHint::Drift); }
		if (const UTDShipMovementComponent* Move = Ship->GetShipMovement())
		{
			if (Move->IsDodging()) { Observe(ETDHint::Dodge); }
		}
		if (const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			if (Hyper->GetState() != ETDHyperdriveState::Idle) { Observe(ETDHint::Hyperdrive); }
		}
	}

	if (IsSuppressed())
	{
		return; // the clock stops: a hint never runs out behind a landing prompt or a pause
	}
	if (Current != ETDHint::None)
	{
		CurrentAge += DeltaTime;
		if (CurrentAge >= TimeoutSeconds)
		{
			Complete(Current); // ignored: counts as seen
		}
		return;
	}
	Gap -= DeltaTime;
	while (Gap <= 0.f && Queue.Num() > 0)
	{
		const ETDHint Next = Queue[0];
		Queue.RemoveAt(0);
		if (!Done.Contains(Next))
		{
			Current = Next;
			CurrentAge = 0.f;
			UE_LOG(LogTriadDrift, Log, TEXT("TD hint: %s"), *GetCurrentText().ToString());
			break;
		}
	}
}

ETDHint UTDHintComponent::GetCurrentHint() const
{
	return IsSuppressed() ? ETDHint::None : Current;
}

FText UTDHintComponent::GetCurrentText() const
{
	const ETDHint H = GetCurrentHint();
	const ATDSpacePlayerController* PC = GetPC();
	return H == ETDHint::None ? FText::GetEmpty() : TextFor(H, PC && PC->IsUsingGamepad(), LastAbility);
}

FText UTDHintComponent::GetHintText(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const UTDHintComponent* Hints = PC ? PC->FindComponentByClass<UTDHintComponent>() : nullptr;
	return Hints ? Hints->GetCurrentText() : FText::GetEmpty();
}

FText UTDHintComponent::TextFor(ETDHint Hint, bool bGamepad, ETDAbilityId Ability)
{
	switch (Hint)
	{
	case ETDHint::Fly:
		return bGamepad ? LOCTEXT("FlyPad", "RIGHT STICK  AIM   ·   RT / LT  THROTTLE")
			: LOCTEXT("FlyKey", "MOUSE  AIM   ·   W / S  THROTTLE");
	case ETDHint::Fire:
		return bGamepad ? LOCTEXT("FirePad", "RB  FIRE") : LOCTEXT("FireKey", "LEFT MOUSE  FIRE");
	case ETDHint::Boost:
		return bGamepad ? LOCTEXT("BoostPad", "CLICK LEFT STICK  BOOST") : LOCTEXT("BoostKey", "HOLD SHIFT  BOOST");
	case ETDHint::Dodge:
		return bGamepad ? LOCTEXT("DodgePad", "FLICK LEFT STICK SIDEWAYS  DODGE")
			: LOCTEXT("DodgeKey", "DOUBLE-TAP A / D  (OR ALT)  DODGE");
	case ETDHint::Switch:
		return bGamepad ? LOCTEXT("SwitchPad", "D-PAD LEFT / RIGHT  SWITCH SHIP - THE OTHER TWO FIGHT ON THEIR OWN")
			: LOCTEXT("SwitchKey", "1 / 2 / 3  SWITCH SHIP - THE OTHER TWO FIGHT ON THEIR OWN");
	case ETDHint::Power:
		return bGamepad ? LOCTEXT("PowerPad", "X  SIGNATURE POWER   ·   HOLD X  SECOND POWER")
			: LOCTEXT("PowerKey", "Q  SIGNATURE POWER   ·   HOLD Q  SECOND POWER");
	case ETDHint::TargetCycle:
		return bGamepad ? LOCTEXT("TargetPad", "CLICK RIGHT STICK  NEXT TARGET") : LOCTEXT("TargetKey", "T  NEXT TARGET");
	case ETDHint::Drift:
		return bGamepad ? LOCTEXT("DriftPad", "HOLD B  DRIFT - KEEP YOUR SPEED, TURN TO SHOOT BEHIND")
			: LOCTEXT("DriftKey", "HOLD C  DRIFT - KEEP YOUR SPEED, TURN TO SHOOT BEHIND");
	case ETDHint::Hyperdrive:
		return bGamepad ? LOCTEXT("HyperPad", "HOLD D-PAD DOWN  HYPERDRIVE TO THE MARKER")
			: LOCTEXT("HyperKey", "HOLD H  HYPERDRIVE TO THE MARKER");
	case ETDHint::Ability:
	{
		const FText Name = Ability == ETDAbilityId::None ? LOCTEXT("NewAbility", "NEW ABILITY")
			: FText::FromString(StaticEnum<ETDAbilityId>()->GetDisplayNameTextByValue((int64)Ability).ToString().ToUpper());
		return FText::Format(bGamepad ? LOCTEXT("AbilityPad", "Y  {0}") : LOCTEXT("AbilityKey", "E  {0}"), Name);
	}
	default:
		return FText::GetEmpty();
	}
}

// ---- Dev commands

namespace TDHints
{
	static UTDHintComponent* Find(UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? PC->FindComponentByClass<UTDHintComponent>() : nullptr;
	}

	static FAutoConsoleCommandWithWorld CmdReset(TEXT("TD.Hints.Reset"), TEXT("Forget every seen control hint (they show again)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UTDHintComponent* H = Find(World)) { H->ResetHints(); H->Offer(ETDHint::Fly); }
		}));

	static FAutoConsoleCommandWithWorld CmdStatus(TEXT("TD.Hints.Status"), TEXT("Log the current control hint and which are done."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UTDHintComponent* H = Find(World);
			if (!H) { UE_LOG(LogTriadDrift, Log, TEXT("TD.Hints.Status: no hint component")); return; }
			FString DoneList;
			for (int32 i = 1; i <= (int32)ETDHint::Ability; ++i)
			{
				if (H->IsHintDone((ETDHint)i)) { DoneList += StaticEnum<ETDHint>()->GetNameStringByValue(i) + TEXT(" "); }
			}
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Hints.Status: enabled %d | showing '%s' | done: %s"), H->AreHintsEnabled() ? 1 : 0,
				*H->GetCurrentText().ToString(), *DoneList);
		}));
}

#undef LOCTEXT_NAMESPACE

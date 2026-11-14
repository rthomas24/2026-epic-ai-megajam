// Triad Drift - first-minute teaching. Owner: Teammate 1 (Input).

#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Waves/TDWaveTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreMisc.h"
#include "TriadDrift/Powers/TDAbilityRewards.h"

#define LOCTEXT_NAMESPACE "TDHints"

namespace TDHints
{
	static const TCHAR* SlotName = TEXT("TD_Hints");

	static TAutoConsoleVariable<int32> CVarHints(TEXT("TD.Hints"), 1,
		TEXT("First-minute control hints: 1 on (default), 0 off for this session."));

	// Onboarding (T3): the four essentials first (fly, shoot, switch, power), then the rest.
	static const ETDHint Core[] = { ETDHint::Fly, ETDHint::Fire, ETDHint::Switch, ETDHint::Power, ETDHint::Boost,
		ETDHint::Dodge, ETDHint::TargetCycle };
	static const ETDHint Essentials[] = { ETDHint::Fly, ETDHint::Fire, ETDHint::Switch, ETDHint::Power };
	static bool IsEssential(ETDHint Hint) { for (ETDHint H : Essentials) { if (H == Hint) { return true; } } return false; }

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

namespace
{
	/** "EMPPulse" -> "EMP PULSE", "GravityWell" -> "GRAVITY WELL" (the enum's display name ran the acronym in, live). */
	FString TDHintsAbilityName(ETDAbilityId Ability)
	{
		const FString N = StaticEnum<ETDAbilityId>()->GetNameStringByValue(static_cast<int64>(Ability));
		FString Out;
		for (int32 I = 0; I < N.Len(); ++I)
		{
			const bool bSplit = FChar::IsUpper(N[I]) && I > 0
				&& (FChar::IsLower(N[I - 1]) || (FChar::IsUpper(N[I - 1]) && I + 1 < N.Len() && FChar::IsLower(N[I + 1])));
			if (bSplit) { Out.AppendChar(TEXT(' ')); }
			Out.AppendChar(N[I]);
		}
		return Out.ToUpper();
	}
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
	Missed.Reset();
	JustDone = ETDHint::None;
	JustDoneLeft = 0.f;
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
	Missed.Reset();
	JustDone = ETDHint::None;
	JustDoneLeft = 0.f;
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

void UTDHintComponent::Complete(ETDHint Hint, bool bByPlayer)
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
		if (bByPlayer && DoneBeatSeconds > 0.f)
		{
			// Onboarding: the HUD ticks it off for a moment before the next one (the player sees they got it).
			JustDone = Hint;
			JustDoneLeft = DoneBeatSeconds;
			Gap += DoneBeatSeconds;
		}
	}
	Save();
}

void UTDHintComponent::MissCurrent()
{
	// Onboarding: a beginner who missed a hint gets it once more, after the others; a second miss counts as seen.
	const ETDHint H = Current;
	if (H == ETDHint::None)
	{
		return;
	}
	if (Missed.Contains(H))
	{
		Complete(H);
		return;
	}
	Missed.Add(H);
	Current = ETDHint::None;
	Gap = GapSeconds;
	Queue.Remove(H);
	Queue.Add(H);
}

void UTDHintComponent::Observe(ETDHint Hint)
{
	// The player did it: done, whether its hint is up, queued or not offered yet (experienced players see nothing).
	Complete(Hint, /*bByPlayer*/ true);
}

bool UTDHintComponent::IsContinuedRun() const
{
	const UTDGameInstance* GI = UTDGameInstance::Get(this);
	return GI && GI->IsRunFromContinue();
}

// ---- Event handlers

void UTDHintComponent::HandleWaveStarted(int32 WaveNumber, float Intensity01)
{
	const bool bFirstOffer = Queue.Num() == 0 && Current == ETDHint::None;
	// Onboarding: a run resumed with Continue is someone who has played; the control lessons stay off (the contextual
	// hints below still come when they apply).
	if (!IsContinuedRun())
	{
		// T1's first-ten-minutes play (Nov 14): a New Game on a machine that has been played before showed no key for any
		// lesson (hints are remembered across runs). A new run is a new pilot: once per run, at wave 1, the core lessons
		// are offered again (doing the action still ticks each off at once, shown or not).
		static uint32 OnboardedRun = MAX_uint32;
		const UTDGameInstance* GI = UTDGameInstance::Get(this);
		if (WaveNumber == 1 && GI && !GIsAutomationTesting && GI->GetRunSerial() != OnboardedRun)
		{
			OnboardedRun = GI->GetRunSerial();
			for (ETDHint H : TDHints::Core) { Done.Remove(H); Missed.Remove(H); }
			UE_LOG(LogTriadDrift, Log, TEXT("TD hints: a new game - the core control hints are offered again (run %u)"), OnboardedRun);
		}
		for (ETDHint H : TDHints::Core)
		{
			Offer(H);
		}
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
	// Board N-070 (T2): the unlock itself shows nothing here (on foot, the UI's reward screen has it). The flight hint
	// comes from UTDAbilityRewardSubsystem the next time the player flies the owning ship, one per award (TickComponent).
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
	// A story transmission on air owns the lower lane (T2's HUD holds the card back): the hint waits, its clock stopped.
	// Never forever: after MaxStoryHoldSeconds of lines on air, the waiting hint shows under them.
	if (const UTDStorySubsystem* Story = UTDStorySubsystem::Get(this);
		Story && Story->GetCurrentTransmission().bActive && StoryHeldSeconds < MaxStoryHoldSeconds)
	{
		return true;
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

	// The story's hold is counted while a hint is waiting through one run of lines (queued back to back, with their
	// short gaps); it starts again once the story is quiet or nothing waits.
	const UTDStorySubsystem* Story = UTDStorySubsystem::Get(this);
	const bool bStoryBusy = Story && (Story->GetCurrentTransmission().bActive || Story->GetQueueLength() > 0);
	if (!bStoryBusy || (Current == ETDHint::None && Queue.Num() == 0))
	{
		StoryHeldSeconds = 0.f;
	}
	else if (Story->GetCurrentTransmission().bActive)
	{
		StoryHeldSeconds += DeltaTime;
	}
	if (IsSuppressed())
	{
		return; // the clock stops: a hint never runs out behind a landing prompt or a pause
	}
	// An award's own flight hint (board N-070): due while its owning ship is flown; each award has its own flag there, so
	// the shared Done entry for ETDHint::Ability never suppresses a later unlock.
	// Live (T2's play, Nov 14): the card "E CLOAK" stayed up after switching to Bastion. An ability's card belongs to its
	// owning ship: switched away, it comes off and is due again the next time that ship is flown.
	if ((Current == ETDHint::Ability || Queue.Contains(ETDHint::Ability)) && LastAbility != ETDAbilityId::None)
	{
		ETDShipId Owner;
		const ATDShip* Flown = GetFlownShip();
		UTDAbilityRewardSubsystem* Rewards = UTDAbilityRewardSubsystem::Get(this);
		if (Flown && Rewards && Rewards->GetAbilityShip(LastAbility, Owner) && Owner != Flown->GetShipId())
		{
			Queue.Remove(ETDHint::Ability);
			if (Current == ETDHint::Ability)
			{
				Current = ETDHint::None;
				Gap = GapSeconds;
			}
			Rewards->ReopenFlightHint(LastAbility);
		}
	}
	if (Current != ETDHint::Ability && !Queue.Contains(ETDHint::Ability))
	{
		ETDAbilityId Due = ETDAbilityId::None;
		if (UTDAbilityRewardSubsystem* Rewards = UTDAbilityRewardSubsystem::Get(this); Rewards && Rewards->GetAbilityFlightHint(Due, this))
		{
			LastAbility = Due;
			Done.Remove(ETDHint::Ability);
			Offer(ETDHint::Ability);
			Rewards->MarkAbilityFlightHintShown(Due);
		}
	}
	if (JustDoneLeft > 0.f)
	{
		JustDoneLeft -= DeltaTime;
		if (JustDoneLeft <= 0.f)
		{
			JustDone = ETDHint::None;
		}
	}
	if (Current != ETDHint::None)
	{
		CurrentAge += DeltaTime;
		// The four essentials (fly, fire, switch, power) are lessons: on a new game their card stays, key named, until the
		// pilot does it (the Lead, Nov 14: a lesson must not pass silently). Everything else still times out.
		const bool bLessonStays = !IsContinuedRun() && TDHints::IsEssential(Current);
		if (CurrentAge >= TimeoutSeconds && !bLessonStays)
		{
			MissCurrent(); // ignored: once more later, then it counts as seen
		}
		return;
	}
	Gap -= DeltaTime;
	if (Gap <= 0.f && Queue.Num() > 0)
	{
		Queue.RemoveAll([this](ETDHint H) { return Done.Contains(H); });
		// The first queued hint that fits the teaching phase on screen (T3's ENG-40); the others wait their phase.
		const ETDTeachPhase Phase = CurrentTeachPhase();
		const int32 Pick = Queue.IndexOfByPredicate([Phase](ETDHint H) { return FitsTeachPhase(H, Phase); });
		if (Pick != INDEX_NONE)
		{
			Current = Queue[Pick];
			Queue.RemoveAt(Pick);
			CurrentAge = 0.f;
			UE_LOG(LogTriadDrift, Log, TEXT("TD hint: %s"), *GetCurrentText().ToString());
		}
	}
}

bool UTDHintComponent::FitsTeachPhase(ETDHint Hint, ETDTeachPhase Phase)
{
	switch (Phase)
	{
	case ETDTeachPhase::Evade: break;
	case ETDTeachPhase::TargetAndFire: break;
	case ETDTeachPhase::SquadSystems: break;
	default: return true; // no lesson running: today's order
	}
	const bool bLesson = Hint == ETDHint::Fly || Hint == ETDHint::Fire || Hint == ETDHint::Boost || Hint == ETDHint::Dodge
		|| Hint == ETDHint::Switch || Hint == ETDHint::Power || Hint == ETDHint::TargetCycle;
	if (!bLesson) { return true; } // drift, hyperdrive, abilities: when they apply
	switch (Phase)
	{
	case ETDTeachPhase::Evade: return Hint == ETDHint::Fly || Hint == ETDHint::Boost || Hint == ETDHint::Dodge;
	case ETDTeachPhase::TargetAndFire: return Hint == ETDHint::Fire || Hint == ETDHint::TargetCycle;
	default: return Hint == ETDHint::Switch || Hint == ETDHint::Power;
	}
}

ETDTeachPhase UTDHintComponent::CurrentTeachPhase() const
{
	if (TeachPhaseOverrideForTests.IsSet()) { return TeachPhaseOverrideForTests.GetValue(); }
	return UTDTutorialPhases::GetTeachPhase(this);
}

ETDHint UTDHintComponent::GetCurrentHint() const
{
	if (DebugPinned != ETDHint::None) { return DebugPinned; } // dev captures
	if (IsSuppressed())
	{
		return ETDHint::None;
	}
	return Current != ETDHint::None ? Current : (JustDoneLeft > 0.f ? JustDone : ETDHint::None);
}

ETDHintState UTDHintComponent::GetCurrentHintState() const
{
	if (DebugPinned != ETDHint::None) { return ETDHintState::Showing; } // dev captures
	if (IsSuppressed())
	{
		return ETDHintState::None;
	}
	if (Current != ETDHint::None)
	{
		return ETDHintState::Showing;
	}
	return (JustDoneLeft > 0.f && JustDone != ETDHint::None) ? ETDHintState::Done : ETDHintState::None;
}

void UTDHintComponent::GetEssentialsProgress(int32& OutDone, int32& OutTotal) const
{
	OutTotal = UE_ARRAY_COUNT(TDHints::Essentials);
	OutDone = 0;
	for (ETDHint H : TDHints::Essentials)
	{
		OutDone += Done.Contains(H) ? 1 : 0;
	}
}

ETDHintState UTDHintComponent::GetHintStateFor(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const UTDHintComponent* Hints = PC ? PC->FindComponentByClass<UTDHintComponent>() : nullptr;
	return Hints ? Hints->GetCurrentHintState() : ETDHintState::None;
}

void UTDHintComponent::GetEssentialsProgressFor(const UObject* WorldContextObject, int32& OutDone, int32& OutTotal)
{
	OutDone = OutTotal = 0;
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (const UTDHintComponent* Hints = PC ? PC->FindComponentByClass<UTDHintComponent>() : nullptr)
	{
		Hints->GetEssentialsProgress(OutDone, OutTotal);
	}
}

namespace TDHints
{
	/** B2-06 (Nick's playtest): the power hint names the flown ship's own actions (placeholder words for Nick's review).
	 *  Part 0 = the whole line, 1 = the key, 2 = the action. Empty = not a squad ship: the generic text stays. */
	FText PowerFor(const APlayerController* PC, bool bGamepad, int32 Part)
	{
		const ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
		if (!Ship) { return FText::GetEmpty(); }
		const bool P = bGamepad;
		switch (Ship->GetShipId())
		{
		case ETDShipId::Rift:
			return Part == 1 ? (P ? LOCTEXT("RiftKeyP", "X") : LOCTEXT("RiftKeyK", "Q"))
				: Part == 2 ? LOCTEXT("RiftAct", "PORTAL  ·  AGAIN FOR THE EXIT")
				: (P ? LOCTEXT("RiftLineP", "X  PORTAL   ·   X AGAIN  EXIT PORTAL") : LOCTEXT("RiftLineK", "Q  PORTAL   ·   Q AGAIN  EXIT PORTAL"));
		case ETDShipId::Lancer:
			return Part == 1 ? (P ? LOCTEXT("LancerKeyP", "X") : LOCTEXT("LancerKeyK", "Q"))
				: Part == 2 ? LOCTEXT("LancerAct", "AFTERBURNER RAM")
				: (P ? LOCTEXT("LancerLineP", "X  AFTERBURNER RAM") : LOCTEXT("LancerLineK", "Q  AFTERBURNER RAM"));
		case ETDShipId::Bastion:
			return Part == 1 ? (P ? LOCTEXT("BastionKeyP", "TAP X  ·  HOLD X") : LOCTEXT("BastionKeyK", "TAP Q  ·  HOLD Q"))
				: Part == 2 ? LOCTEXT("BastionAct", "MISSILE SWARM  ·  SHIELD DOME")
				: (P ? LOCTEXT("BastionLineP", "TAP X: MISSILE SWARM   ·   HOLD X: SHIELD DOME") : LOCTEXT("BastionLineK", "TAP Q: MISSILE SWARM   ·   HOLD Q: SHIELD DOME"));
		default:
			return FText::GetEmpty();
		}
	}
}

FText UTDHintComponent::GetCurrentText() const
{
	const ETDHint H = GetCurrentHint();
	const ATDSpacePlayerController* PC = GetPC();
	if (H == ETDHint::Power)
	{
		const FText Own = TDHints::PowerFor(PC, PC && PC->IsUsingGamepad(), 0);
		if (!Own.IsEmpty()) { return Own; }
	}
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
		return bGamepad ? LOCTEXT("FlyPad", "R-STICK  AIM   ·   RT / LT  THROTTLE")
			: LOCTEXT("FlyKey", "MOUSE  AIM   ·   W / S  THROTTLE");
	case ETDHint::Fire:
		return bGamepad ? LOCTEXT("FirePad", "RB  FIRE") : LOCTEXT("FireKey", "LEFT MOUSE  FIRE");
	case ETDHint::Boost:
		return bGamepad ? LOCTEXT("BoostPad", "CLICK LEFT STICK  BOOST") : LOCTEXT("BoostKey", "HOLD SHIFT  BOOST");
	case ETDHint::Dodge:
		return bGamepad ? LOCTEXT("DodgePad", "DOUBLE-FLICK L-STICK  DODGE")
			: LOCTEXT("DodgeKey", "DOUBLE-TAP A / D  (OR ALT)  DODGE");
	case ETDHint::Switch:
		return bGamepad ? LOCTEXT("SwitchPad", "D-PAD LEFT / RIGHT  SWITCH SHIP")
			: LOCTEXT("SwitchKey", "1 / 2 / 3  SWITCH SHIP");
	case ETDHint::Power:
		return bGamepad ? LOCTEXT("PowerPad", "X  POWER   ·   HOLD X  2ND POWER")
			: LOCTEXT("PowerKey", "Q  POWER   ·   HOLD Q  2ND POWER");
	case ETDHint::TargetCycle:
		return bGamepad ? LOCTEXT("TargetPad", "CLICK RIGHT STICK  NEXT TARGET") : LOCTEXT("TargetKey", "T  NEXT TARGET");
	case ETDHint::Drift:
		return bGamepad ? LOCTEXT("DriftPad", "HOLD B  DRIFT  (TURN, KEEP SPEED)")
			: LOCTEXT("DriftKey", "HOLD C  DRIFT  (TURN, KEEP SPEED)");
	case ETDHint::Hyperdrive:
		return bGamepad ? LOCTEXT("HyperPad", "HOLD D-PAD DOWN  HYPERDRIVE")
			: LOCTEXT("HyperKey", "HOLD H  HYPERDRIVE TO THE MARKER");
	case ETDHint::Ability:
	{
		const FText Name = Ability == ETDAbilityId::None ? LOCTEXT("NewAbility", "NEW ABILITY")
			: FText::FromString(TDHintsAbilityName(Ability));
		return FText::Format(bGamepad ? LOCTEXT("AbilityPad", "Y  {0}") : LOCTEXT("AbilityKey", "E  {0}"), Name);
	}
	default:
		return FText::GetEmpty();
	}
}

FText UTDHintComponent::KeyFor(ETDHint Hint, bool bGamepad)
{
	switch (Hint)
	{
	case ETDHint::Fly: return bGamepad ? LOCTEXT("KFlyPad", "R-STICK · RT/LT") : LOCTEXT("KFlyKey", "MOUSE  ·  W / S");
	case ETDHint::Fire: return bGamepad ? LOCTEXT("KFirePad", "RB") : LOCTEXT("KFireKey", "LMB");
	case ETDHint::Boost: return bGamepad ? LOCTEXT("KBoostPad", "L3") : LOCTEXT("KBoostKey", "SHIFT");
	case ETDHint::Dodge: return bGamepad ? LOCTEXT("KDodgePad", "L-STICK x2") : LOCTEXT("KDodgeKey", "A A / D D");
	case ETDHint::Switch: return bGamepad ? LOCTEXT("KSwitchPad", "D-PAD L / R") : LOCTEXT("KSwitchKey", "1  2  3");
	case ETDHint::Power: return bGamepad ? LOCTEXT("KPowerPad", "X") : LOCTEXT("KPowerKey", "Q");
	case ETDHint::TargetCycle: return bGamepad ? LOCTEXT("KTargetPad", "R3") : LOCTEXT("KTargetKey", "T");
	case ETDHint::Drift: return bGamepad ? LOCTEXT("KDriftPad", "B") : LOCTEXT("KDriftKey", "C");
	case ETDHint::Hyperdrive: return bGamepad ? LOCTEXT("KHyperPad", "D-PAD DOWN") : LOCTEXT("KHyperKey", "H");
	case ETDHint::Ability: return bGamepad ? LOCTEXT("KAbilityPad", "Y") : LOCTEXT("KAbilityKey", "E");
	default: return FText::GetEmpty();
	}
}

FText UTDHintComponent::ActionFor(ETDHint Hint, ETDAbilityId Ability)
{
	switch (Hint)
	{
	case ETDHint::Fly: return LOCTEXT("AFly", "AIM  ·  THROTTLE");
	case ETDHint::Fire: return LOCTEXT("AFire", "FIRE");
	case ETDHint::Boost: return LOCTEXT("ABoost", "HOLD TO BOOST");
	case ETDHint::Dodge: return LOCTEXT("ADodge", "DODGE");
	case ETDHint::Switch: return LOCTEXT("ASwitch", "SWITCH SHIP");
	case ETDHint::Power: return LOCTEXT("APower", "POWER  ·  HOLD FOR THE 2ND");
	case ETDHint::TargetCycle: return LOCTEXT("ATarget", "NEXT TARGET");
	case ETDHint::Drift: return LOCTEXT("ADrift", "HOLD TO DRIFT");
	case ETDHint::Hyperdrive: return LOCTEXT("AHyper", "HOLD: HYPERDRIVE");
	case ETDHint::Ability:
		return Ability == ETDAbilityId::None ? LOCTEXT("AAbility", "NEW ABILITY")
			: FText::FromString(TDHintsAbilityName(Ability));
	default: return FText::GetEmpty();
	}
}

namespace TDHints
{
	static const UTDHintComponent* FindFor(const UObject* WorldContextObject)
	{
		const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? PC->FindComponentByClass<UTDHintComponent>() : nullptr;
	}
}

FText UTDHintComponent::GetHintKey(const UObject* WorldContextObject)
{
	const UTDHintComponent* H = TDHints::FindFor(WorldContextObject);
	const ATDSpacePlayerController* PC = H ? H->GetPC() : nullptr;
	if (H && H->GetCurrentHint() == ETDHint::Power)
	{
		const FText Own = TDHints::PowerFor(PC, PC && PC->IsUsingGamepad(), 1);
		if (!Own.IsEmpty()) { return Own; }
	}
	return H ? KeyFor(H->GetCurrentHint(), PC && PC->IsUsingGamepad()) : FText::GetEmpty();
}

FText UTDHintComponent::GetHintAction(const UObject* WorldContextObject)
{
	const UTDHintComponent* H = TDHints::FindFor(WorldContextObject);
	if (H && H->GetCurrentHint() == ETDHint::Power)
	{
		const ATDSpacePlayerController* PC = H->GetPC();
		const FText Own = TDHints::PowerFor(PC, PC && PC->IsUsingGamepad(), 2);
		if (!Own.IsEmpty()) { return Own; }
	}
	return H ? ActionFor(H->GetCurrentHint(), H->LastAbility) : FText::GetEmpty();
}

bool UTDHintComponent::GetHintStep(const UObject* WorldContextObject, int32& OutStep, int32& OutCount)
{
	OutStep = OutCount = 0;
	const UTDHintComponent* H = TDHints::FindFor(WorldContextObject);
	const ETDHint Hint = H ? H->GetCurrentHint() : ETDHint::None;
	for (int32 i = 0; i < UE_ARRAY_COUNT(TDHints::Essentials); ++i)
	{
		if (TDHints::Essentials[i] == Hint)
		{
			OutStep = i + 1;
			OutCount = UE_ARRAY_COUNT(TDHints::Essentials);
			return true;
		}
	}
	return false;
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

#if !UE_BUILD_SHIPPING
	static FAutoConsoleCommandWithWorldAndArgs CmdShow(TEXT("TD.Hints.Show"),
		TEXT("Captures: put one control hint on screen for the flown ship and hold it: TD.Hints.Show <Fly|Fire|Boost|Dodge|Switch|Power|TargetCycle|Drift|Hyperdrive|Ability|None> (None releases it)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDHintComponent* H = Find(World);
			if (!H || Args.Num() == 0) { return; }
			const int64 V = StaticEnum<ETDHint>()->GetValueByNameString(Args[0], EGetByNameFlags::None);
			if (V == INDEX_NONE) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Hints.Show: no hint named %s"), *Args[0]); return; }
			H->DebugPinHint((ETDHint)V);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Hints.Show: %s -> '%s'"), *Args[0], *H->GetCurrentText().ToString());
		}));
	static FAutoConsoleCommandWithWorld CmdNext(TEXT("TD.Hints.Next"), TEXT("Captures: show the next control hint in order (Fly, Fire, Boost, ... Ability, then back to Fly)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UTDHintComponent* H = Find(World);
			if (!H) { return; }
			const int32 Now = (int32)H->GetDebugPinnedHint();
			const int32 Next = Now >= (int32)ETDHint::Ability ? (int32)ETDHint::Fly : Now + 1;
			H->DebugPinHint((ETDHint)Next);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Hints.Next: %s -> '%s'"), *StaticEnum<ETDHint>()->GetNameStringByValue(Next), *H->GetCurrentText().ToString());
		}));
#endif

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

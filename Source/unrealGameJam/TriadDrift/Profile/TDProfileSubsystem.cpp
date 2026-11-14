// Triad Drift - the player profile: first-play tutorial flag and "skip the tutorial". Owner: Teammate 7.

#include "TriadDrift/Profile/TDProfileSubsystem.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreMisc.h"

namespace TDProfileImpl
{
	static const TCHAR* DefaultSlot = TEXT("TD_Profile");
	static const FName ReasonSkipped(TEXT("TutorialSkipped"));
	/** The tutorial battle is waves 1-2 (storyline.md, reconciliation of Nov 13): a skip leaves them cleared. */
	static constexpr int32 TutorialLastWave = 2;

	/** An engine save file starts with 'GVAS'; anything else (truncated, garbage) never reaches the engine's loader. */
	static bool LooksLikeProfileSave(const TArray<uint8>& Bytes)
	{
		return Bytes.Num() > 64 && Bytes[0] == 'G' && Bytes[1] == 'V' && Bytes[2] == 'A' && Bytes[3] == 'S';
	}

	/** Phases that come after the tutorial battle in a run. */
	static bool IsPastTutorial(ETDGamePhase P)
	{
		return P != ETDGamePhase::Intro && P != ETDGamePhase::Waves1to2;
	}
}

UTDProfileSubsystem* UTDProfileSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UGameInstance* GI = World ? World->GetGameInstance() : Cast<UGameInstance>(const_cast<UObject*>(WorldContext));
	return GI ? GI->GetSubsystem<UTDProfileSubsystem>() : nullptr;
}

void UTDProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UTDEventBus* Bus = Collection.InitializeDependency<UTDEventBus>();
	if (Bus)
	{
		Bus->OnPhaseChanged.AddUniqueDynamic(this, &UTDProfileSubsystem::HandlePhaseChanged);
		bBusBound = true;
	}
	Load();
}

void UTDProfileSubsystem::Deinitialize()
{
	if (bBusBound)
	{
		if (UTDEventBus* Bus = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTDEventBus>() : nullptr)
		{
			Bus->OnPhaseChanged.RemoveDynamic(this, &UTDProfileSubsystem::HandlePhaseChanged);
		}
		bBusBound = false;
	}
	Super::Deinitialize();
}

// ------------------------------------------------------------------------------------------------ Persistence

bool UTDProfileSubsystem::ShouldPersist() const
{
	// Automation never touches the player's profile unless a test chose its own slot.
	return !GIsAutomationTesting || !SlotOverride.IsEmpty();
}

FString UTDProfileSubsystem::GetSlotName() const
{
	return SlotOverride.IsEmpty() ? FString(TDProfileImpl::DefaultSlot) : SlotOverride;
}

void UTDProfileSubsystem::SetSlotOverrideForTests(const FString& Slot)
{
	SlotOverride = Slot;
	Load();
}

void UTDProfileSubsystem::Load()
{
	bTutorialComplete = false;
	TutorialCompletions = 0;
	FirstCompletedUtc = FDateTime();
	if (!ShouldPersist())
	{
		return;
	}
	const FString Slot = GetSlotName();
	const int32 User = UTDFlowSettings::Get()->SaveUserIndex;
	if (!UGameplayStatics::DoesSaveGameExist(Slot, User))
	{
		return; // a fresh profile
	}
	TArray<uint8> Bytes;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, Slot, User) || !TDProfileImpl::LooksLikeProfileSave(Bytes))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Profile slot '%s' is corrupt; starting a fresh profile."), *Slot);
		return;
	}
	const UTDProfileSaveGame* Saved = Cast<UTDProfileSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!Saved || Saved->Version != UTDProfileSaveGame::CurrentVersion)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Profile slot '%s' is not a Triad Drift profile of version %d; starting a fresh profile."), *Slot, UTDProfileSaveGame::CurrentVersion);
		return;
	}
	bTutorialComplete = Saved->bTutorialComplete;
	TutorialCompletions = Saved->TutorialCompletions;
	FirstCompletedUtc = Saved->TutorialFirstCompletedUtc;
}

bool UTDProfileSubsystem::Save() const
{
	if (!ShouldPersist())
	{
		return false;
	}
	UTDProfileSaveGame* Out = Cast<UTDProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UTDProfileSaveGame::StaticClass()));
	if (!Out)
	{
		return false;
	}
	Out->Version = UTDProfileSaveGame::CurrentVersion;
	Out->bTutorialComplete = bTutorialComplete;
	Out->TutorialCompletions = TutorialCompletions;
	Out->TutorialFirstCompletedUtc = FirstCompletedUtc;
	const bool bOk = UGameplayStatics::SaveGameToSlot(Out, GetSlotName(), UTDFlowSettings::Get()->SaveUserIndex);
	if (!bOk)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Failed to write profile slot '%s'."), *GetSlotName());
	}
	return bOk;
}

// ------------------------------------------------------------------------------------------------ The flag

void UTDProfileSubsystem::NotifyTutorialCompleted(const TCHAR* Why)
{
	if (bCompletedThisRun || bRunSkipped)
	{
		return; // once per run; a skipped run never plays the tutorial
	}
	bCompletedThisRun = true;
	if (!bTutorialComplete)
	{
		FirstCompletedUtc = FDateTime::UtcNow();
	}
	bTutorialComplete = true;
	++TutorialCompletions;
	Save();
	UE_LOG(LogTriadDrift, Log, TEXT("TD profile: tutorial complete (%s); a later New Game may skip it (completions %d)."), Why, TutorialCompletions);
	bAftermathPending = true;
	OnAftermathBegins.Broadcast(false);
}

void UTDProfileSubsystem::ResetProfile()
{
	bTutorialComplete = false;
	TutorialCompletions = 0;
	FirstCompletedUtc = FDateTime();
	Save();
}

void UTDProfileSubsystem::HandlePhaseChanged(ETDGamePhase Phase)
{
	// Fallback until T4's escape event calls NotifyTutorialCompleted: the run leaving the tutorial battle (Waves1to2)
	// for a later phase. A skip (Intro -> Verdance) and a cheat stage (a new run, so Intro first) never count.
	const ETDGamePhase Prev = LastPhase;
	LastPhase = Phase;
	if (Prev == ETDGamePhase::Waves1to2 && TDProfileImpl::IsPastTutorial(Phase) && Phase != ETDGamePhase::Victory)
	{
		NotifyTutorialCompleted(TEXT("the run moved past the tutorial battle"));
	}
}

// ------------------------------------------------------------------------------------------------ Skip

void UTDProfileSubsystem::HandleNewRun()
{
	bSkipPending = false;
	bRunSkipped = false;
	bCompletedThisRun = false;
	bAftermathPending = false;
	LastPhase = ETDGamePhase::Intro;
}

bool UTDProfileSubsystem::StartNewRunSkippingTutorial()
{
	UTDGameInstance* GI = Cast<UTDGameInstance>(GetGameInstance());
	if (!GI || !CanSkipTutorial())
	{
		return false;
	}
	GI->StartNewRun(); // calls HandleNewRun: nothing armed
	bSkipPending = true;
	bRunSkipped = true;
	UE_LOG(LogTriadDrift, Log, TEXT("TD profile: New Game skipping the tutorial (the intro plays, then Aftermath)."));
	return true;
}

bool UTDProfileSubsystem::LeaveIntro(UTDGameInstance* GI)
{
	if (!GI)
	{
		return false;
	}
	UTDProfileSubsystem* Profile = GI->GetSubsystem<UTDProfileSubsystem>();
	if (Profile && Profile->bSkipPending)
	{
		Profile->ApplySkip(GI);
		return true;
	}
	GI->SetPhase(ETDGamePhase::Waves1to2);
	return false;
}

void UTDProfileSubsystem::ApplySkip(UTDGameInstance* GI)
{
	bSkipPending = false;
	// The state a finished tutorial leaves (the same as a run whose wave 2 just ended): waves 1-2 cleared, the next phase
	// from the flow settings (Verdance), no abilities (they are earned on the planets), takes and stats untouched.
	FTDProgress& P = GI->GetMutableProgress();
	P.WavesCleared = FMath::Max(P.WavesCleared, TDProfileImpl::TutorialLastWave);
	const ETDGamePhase* Next = UTDFlowSettings::Get()->PhaseAfterWaveCleared.Find(TDProfileImpl::TutorialLastWave);
	GI->SetPhase(Next ? *Next : ETDGamePhase::Verdance);
	GI->TakeCheckpoint(TDProfileImpl::ReasonSkipped); // a CUT or a Continue lands here, not in the tutorial
	UE_LOG(LogTriadDrift, Log, TEXT("TD profile: tutorial skipped; the run is at Aftermath (phase %s, waves cleared %d)."),
		*UEnum::GetValueAsString(GI->GetProgressRef().Phase), GI->GetProgressRef().WavesCleared);
	bAftermathPending = true;
	OnAftermathBegins.Broadcast(true);
}

bool UTDProfileSubsystem::ConsumeAftermathPending()
{
	const bool bWas = bAftermathPending;
	bAftermathPending = false;
	return bWas;
}

// ------------------------------------------------------------------------------------------------ Dev commands

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

namespace TDProfileDebug
{
	static FAutoConsoleCommandWithWorldAndArgs GProfile(TEXT("TD.Profile"),
		TEXT("TD.Profile [status|complete|reset]: the player profile's tutorial flag (status logs it)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDProfileSubsystem* P = UTDProfileSubsystem::Get(World);
			if (!P) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Profile: no profile subsystem")); return; }
			const FString Op = Args.Num() ? Args[0].ToLower() : FString(TEXT("status"));
			if (Op == TEXT("complete")) { P->NotifyTutorialCompleted(TEXT("TD.Profile complete")); }
			else if (Op == TEXT("reset")) { P->ResetProfile(); }
			else if (Op != TEXT("status")) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Profile: unknown '%s' (status|complete|reset)"), *Op); }
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Profile: tutorial complete %d, can skip %d, skip pending %d, run skipped %d, slot '%s'"),
				P->IsTutorialComplete() ? 1 : 0, P->CanSkipTutorial() ? 1 : 0, P->IsSkipTutorialPending() ? 1 : 0, P->DidRunSkipTutorial() ? 1 : 0, *P->GetSlotName());
		}));

	// As the menu's SKIP THE TUTORIAL: a New Game into the intro, then Aftermath.
	static FAutoConsoleCommandWithWorldAndArgs GSkip(TEXT("TD.Tutorial.SkipNewGame"),
		TEXT("Start a New Game that skips the tutorial (as the menu's SKIP THE TUTORIAL; needs a completed tutorial)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			UTDProfileSubsystem* P = UTDProfileSubsystem::Get(World);
			UTDGameInstance* GI = UTDGameInstance::Get(World);
			if (!P || !GI) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Tutorial.SkipNewGame: no Triad Drift game instance")); return; }
			if (!P->StartNewRunSkippingTutorial())
			{
				UE_LOG(LogTriadDrift, Warning, TEXT("TD.Tutorial.SkipNewGame: refused, the tutorial was never completed on this profile (TD.Profile complete)"));
				return;
			}
			const bool bOk = GI->TravelTo(ETDDestination::Galaxy, NAME_None);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Tutorial.SkipNewGame: %s"), bOk ? TEXT("travelling to the galaxy, intro then Aftermath") : TEXT("travel refused"));
		}));
}
#endif

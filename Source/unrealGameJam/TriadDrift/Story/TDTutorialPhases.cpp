// Triad Drift - ENG-40 tutorial phase prompts. Owner: Teammate 3.

#include "TriadDrift/Story/TDTutorialPhases.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Waves/TDWaveDirector.h"

#define LOCTEXT_NAMESPACE "TDTutorialPhases"

namespace TDTutorialPhasesImpl
{
	/** A run marker in the played set (never a line): the lesson was taught, so a retake or a Continue never replays it. */
	static const TCHAR* TaughtId = TEXT("Seq_TutorialTaught");

	static const TCHAR* Name(ETDTeachPhase P)
	{
		switch (P)
		{
		case ETDTeachPhase::Evade: return TEXT("Evade");
		case ETDTeachPhase::TargetAndFire: return TEXT("TargetAndFire");
		case ETDTeachPhase::SquadSystems: return TEXT("SquadSystems");
		case ETDTeachPhase::Done: return TEXT("Done");
		default: return TEXT("None");
		}
	}
}

UTDTutorialPhases* UTDTutorialPhases::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDTutorialPhases>() : nullptr;
}

ETDTeachPhase UTDTutorialPhases::GetTeachPhase(const UObject* WorldContext)
{
	const UTDTutorialPhases* S = Get(WorldContext);
	return S ? S->Phase : ETDTeachPhase::None;
}

bool UTDTutorialPhases::GetPrompt(const UObject* WorldContext, FName& OutId, FText& OutText)
{
	const UTDTutorialPhases* S = Get(WorldContext);
	if (!S)
	{
		return false;
	}
	switch (S->Phase)
	{
	case ETDTeachPhase::Evade:
		OutId = TEXT("Tutorial_Evade");
		OutText = LOCTEXT("Evade", "EVASIVE MANEUVERS");
		return true;
	case ETDTeachPhase::TargetAndFire:
		OutId = TEXT("Tutorial_TargetAndFire");
		OutText = FText::Format(LOCTEXT("TargetAndFire", "TARGET AND FIRE  {0} / {1}"), FMath::Min(S->Kills, S->KillsToTeach), S->KillsToTeach);
		return true;
	case ETDTeachPhase::SquadSystems:
		OutId = TEXT("Tutorial_SquadSystems");
		OutText = !S->bSwitched ? LOCTEXT("SquadSwitch", "SQUAD SYSTEMS · SWITCH SHIPS") : LOCTEXT("SquadPower", "SQUAD SYSTEMS · USE A POWER");
		return true;
	default:
		return false;
	}
}

void UTDTutorialPhases::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UTDEventBus* Bus = UTDEventBus::Get(&InWorld))
	{
		Bus->OnWaveStarted.AddUniqueDynamic(this, &UTDTutorialPhases::HandleWaveStarted);
		Bus->OnActorKilled.AddUniqueDynamic(this, &UTDTutorialPhases::HandleActorKilled);
		Bus->OnShipSwitched.AddUniqueDynamic(this, &UTDTutorialPhases::HandleShipSwitched);
		Bus->OnPowerUsed.AddUniqueDynamic(this, &UTDTutorialPhases::HandlePowerUsed);
	}
}

void UTDTutorialPhases::Deinitialize()
{
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnWaveStarted.RemoveDynamic(this, &UTDTutorialPhases::HandleWaveStarted);
		Bus->OnActorKilled.RemoveDynamic(this, &UTDTutorialPhases::HandleActorKilled);
		Bus->OnShipSwitched.RemoveDynamic(this, &UTDTutorialPhases::HandleShipSwitched);
		Bus->OnPowerUsed.RemoveDynamic(this, &UTDTutorialPhases::HandlePowerUsed);
	}
	Super::Deinitialize();
}

bool UTDTutorialPhases::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UTDTutorialPhases::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDTutorialPhases, STATGROUP_Tickables);
}

bool UTDTutorialPhases::IsTutorialRun() const
{
	const UWorld* World = GetWorld();
	const ATDWaveDirector* D = ATDWaveDirector::Get(this);
	if (!World || !D || D->TutorialMode == ETDTutorialMode::ForceOff)
	{
		return false;
	}
	if (const UTDGameInstance* GI = UTDGameInstance::Get(this); GI && GI->HasPlayedTransmission(TDTutorialPhasesImpl::TaughtId))
	{
		return false; // taught this run (a retake, a Continue)
	}
#if WITH_DEV_AUTOMATION_TESTS
	if (bForceTutorialForTests)
	{
		return true;
	}
#endif
	// T4's rule: real levels only (Auto); test worlds stay None so their wave tests are deterministic.
	return D->TutorialMode == ETDTutorialMode::ForceOn || !World->GetOutermost()->GetName().StartsWith(TEXT("/Temp/"));
}

void UTDTutorialPhases::SetPhase(ETDTeachPhase NewPhase)
{
	if (NewPhase == Phase)
	{
		return;
	}
	Phase = NewPhase;
	PhaseSeconds = 0.f;
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Tutorial: phase %s"), TDTutorialPhasesImpl::Name(Phase));
	if (Phase == ETDTeachPhase::TargetAndFire)
	{
		Kills = 0;
		UTDStorySubsystem::Fire(this, TEXT("TB_Kato_Marking"));
	}
	else if (Phase == ETDTeachPhase::SquadSystems)
	{
		bSwitched = bPowerUsed = false;
		UTDStorySubsystem::Fire(this, TEXT("TB_Moss_ThreeShips"));
	}
	else if (Phase == ETDTeachPhase::Done)
	{
		if (UTDGameInstance* GI = UTDGameInstance::Get(this))
		{
			GI->MarkTransmissionPlayed(TDTutorialPhasesImpl::TaughtId); // T4: sticky for the run
		}
	}
	OnTeachPhaseChanged.Broadcast(Phase);
}

void UTDTutorialPhases::SkipAll()
{
	if (Phase != ETDTeachPhase::None && Phase != ETDTeachPhase::Done)
	{
		SetPhase(ETDTeachPhase::Done);
	}
}

APawn* UTDTutorialPhases::GetPlayerPawn() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return PC ? PC->GetPawn() : nullptr;
}

void UTDTutorialPhases::HandleWaveStarted(int32 WaveNumber, float /*Intensity01*/)
{
	if (Phase != ETDTeachPhase::None || !IsTutorialRun())
	{
		return;
	}
	// Wave 1 opens the lesson; a run that starts at wave 2 (a checkpoint) still learns the squad.
	if (WaveNumber == 1) { SetPhase(ETDTeachPhase::Evade); }
	else if (WaveNumber == 2) { SetPhase(ETDTeachPhase::SquadSystems); }
}

void UTDTutorialPhases::HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind /*Kind*/)
{
	if (Phase != ETDTeachPhase::TargetAndFire || !Killer)
	{
		return;
	}
	const APawn* Pawn = GetPlayerPawn();
	if (Pawn && (Killer == Pawn || Killer->GetOwner() == Pawn || Killer->GetInstigator() == Pawn))
	{
		++Kills;
	}
}

void UTDTutorialPhases::HandleShipSwitched(ETDShipId /*NewShip*/, ETDShipId /*OldShip*/)
{
	if (Phase == ETDTeachPhase::SquadSystems) { bSwitched = true; }
}

void UTDTutorialPhases::HandlePowerUsed(ETDShipId /*Ship*/, FName /*PowerName*/)
{
	// The order teaches the switch first; a power used before it still counts.
	if (Phase == ETDTeachPhase::SquadSystems) { bPowerUsed = true; }
}

void UTDTutorialPhases::Tick(float DeltaTime)
{
	if (Phase == ETDTeachPhase::None || Phase == ETDTeachPhase::Done)
	{
		return;
	}
	// The withdrawal (T4) ends the lesson; so does a run that moved past waves 1-2.
	if (const ATDWaveDirector* D = ATDWaveDirector::Get(this); D && D->GetTutorialPhase() != ETDTutorialPhase::Battle)
	{
		SetPhase(ETDTeachPhase::Done);
		return;
	}
	// Live walk: nothing counts while the intro holds the screen or the player isn't flying (no control, no lesson).
	if (const UTDIntroDirector* Intro = UTDIntroDirector::Get(this); Intro && Intro->IsPlaying())
	{
		return;
	}
	if (bRequireFlyingShip && !Cast<ATDShip>(GetPlayerPawn()))
	{
		return;
	}
	const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : DeltaTime;
	PhaseSeconds += Dt;
	switch (Phase)
	{
	case ETDTeachPhase::Evade:
	{
		if (const ATDShip* Ship = Cast<ATDShip>(GetPlayerPawn()))
		{
			FlySeconds += Dt;
			const UTDShipMovementComponent* Move = Ship->FindComponentByClass<UTDShipMovementComponent>();
			bEvaded |= Ship->IsBoosting() || (Move && Move->IsDodging());
		}
		if ((bEvaded && FlySeconds >= EvadeFlySeconds) || PhaseSeconds >= EvadeSubstituteSeconds)
		{
			SetPhase(ETDTeachPhase::TargetAndFire);
		}
		break;
	}
	case ETDTeachPhase::TargetAndFire:
	{
		// Wave 1 cleared some other way (the wingmen) also ends it: never a hard lock.
		const ATDWaveDirector* D = ATDWaveDirector::Get(this);
		const bool bWaveOneOver = D && D->GetWaveInfo().WaveNumber >= 2;
		if (Kills >= KillsToTeach || bWaveOneOver)
		{
			SetPhase(ETDTeachPhase::SquadSystems);
		}
		break;
	}
	case ETDTeachPhase::SquadSystems:
	{
		// T1's first-ten-minutes play (Lead: a lesson must not pass silently): SWITCH SHIPS stood for 62 s, no switch made,
		// then went to Done on its timer. It ends only on the real actions now. The timer stays for the one case with no
		// way to do them: no other squad ship alive to switch to (never a hard lock).
		bool bCanSwitch = false;
		if (const ATDSquadManager* Squad = ATDSquadManager::Get(this))
		{
			for (ETDShipId Id : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
			{
				bCanSwitch |= Id != Squad->GetActiveShipId() && Squad->IsShipAlive(Id);
			}
		}
		if ((bSwitched && bPowerUsed) || (!bCanSwitch && PhaseSeconds >= SquadSubstituteSeconds))
		{
			SetPhase(ETDTeachPhase::Done);
		}
		break;
	}
	case ETDTeachPhase::Done:
		break;
	default:
		break;
	}
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GTDTutorialPhasesCmd(TEXT("TD.Tutorial.Phases"),
	TEXT("TD.Tutorial.Phases [status|skip]: the ENG-40 taught phases (EVASIVE MANEUVERS, TARGET AND FIRE, SQUAD SYSTEMS)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UTDTutorialPhases* S = UTDTutorialPhases::Get(World);
		if (!S) { return; }
		if (Args.Num() > 0 && Args[0].Equals(TEXT("skip"), ESearchCase::IgnoreCase)) { S->SkipAll(); }
		FName Id; FText Prompt;
		UTDTutorialPhases::GetPrompt(World, Id, Prompt);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Tutorial.Phases: %s '%s'"), TDTutorialPhasesImpl::Name(S->GetPhase()), *Prompt.ToString());
	}));
#endif

#undef LOCTEXT_NAMESPACE

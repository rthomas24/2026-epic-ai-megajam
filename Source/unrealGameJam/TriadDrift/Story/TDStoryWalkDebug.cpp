// Triad Drift - story walk debug command (dev builds only). Owner: Teammate 3.
// For live story walks over MCP: stand the on-foot pilot where the course's real steps happen, then press the real keys
// (TD.Key E <seconds>), so the story beats fire from the game's own objectives and consoles, not from TD.Story.Sequence.
//   TD.Story.Goto Trailhead <ObjectiveId>   on the objective's trailhead marker (starts it the normal way)
//   TD.Story.Goto Step                      the active objective's current step: beside its console, facing it, or on
//                                           the step's waypoint
//   TD.Story.Goto Survey                    beside Cinder's survey terminal, facing it

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"

namespace TDStoryWalk
{
	static void StandBeside(UWorld* World, AActor* Target, float Gap, bool bOnTop)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || !Target)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Story.Goto: need an on-foot pawn and a target"));
			return;
		}
		FVector Origin, Extent;
		Target->GetActorBounds(true, Origin, Extent);
		const FVector At = Target->GetActorLocation();
		FVector Dir = FVector(At.X - Pawn->GetActorLocation().X, At.Y - Pawn->GetActorLocation().Y, 0.f).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const float Half = bOnTop ? 0.f : (float)FMath::Max(Extent.X, Extent.Y) + Gap;
		const FVector Stand(At.X - Dir.X * Half, At.Y - Dir.Y * Half, At.Z + 120.f);
		Pawn->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
		if (!bOnTop)
		{
			PC->SetControlRotation((At + FVector(0.f, 0.f, 40.f) - (Stand + FVector(0.f, 0.f, 60.f))).Rotation());
		}
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Story.Goto: pilot at %s, target %s at %s"), *Stand.ToCompactString(), *GetNameSafe(Target), *At.ToCompactString());
	}

	static void Goto(const TArray<FString>& Args, UWorld* World)
	{
		ATDPlanetCourse* Course = nullptr;
		for (TActorIterator<ATDPlanetCourse> It(World); It && !Course; ++It) { Course = *It; }
		const FString Mode = Args.Num() > 0 ? Args[0] : TEXT("Step");
		if (Mode.Equals(TEXT("Survey"), ESearchCase::IgnoreCase))
		{
			StandBeside(World, Course ? Course->GetSurveyTerminal() : nullptr, 70.f, false);
			return;
		}
		if (Mode.Equals(TEXT("Trailhead"), ESearchCase::IgnoreCase) && Args.Num() > 1 && Course)
		{
			for (const FTDTrailhead& H : Course->GetTrailheads())
			{
				if (H.Objective && H.Objective->ObjectiveId == FName(*Args[1]) && H.Marker)
				{
					StandBeside(World, H.Marker, 0.f, true);
					return;
				}
			}
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Story.Goto: no trailhead for %s"), *Args[1]);
			return;
		}
		for (TActorIterator<ATDObjective> O(World); O; ++O)
		{
			if (!O->IsActive() || O->IsComplete())
			{
				continue;
			}
			const int32 Step = O->GetCurrentStepIndex();
			for (TActorIterator<ATDInteractableActor> It(World); It; ++It)
			{
				if (It->LinkedObjective == *O && It->LinkedStep == Step)
				{
					UE_LOG(LogTriadDrift, Log, TEXT("TD.Story.Goto: %s step %d, console"), *O->ObjectiveId.ToString(), Step);
					StandBeside(World, *It, 70.f, false);
					return;
				}
			}
			if (O->Steps.IsValidIndex(Step) && O->Steps[Step].Waypoint)
			{
				UE_LOG(LogTriadDrift, Log, TEXT("TD.Story.Goto: %s step %d, waypoint"), *O->ObjectiveId.ToString(), Step);
				StandBeside(World, O->Steps[Step].Waypoint, 0.f, true);
				return;
			}
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Story.Goto: %s step %d has no console or waypoint"), *O->ObjectiveId.ToString(), Step);
			return;
		}
		UE_LOG(LogTriadDrift, Warning, TEXT("TD.Story.Goto: no active objective"));
	}

	// The menu's CONTINUE runs UTDGameInstance::ContinueRun (TDFrontEndGameModes.cpp); this calls the same function for
	// agents, whose injected keys never reach the menu's Slate focus.
	static FAutoConsoleCommandWithWorldAndArgs CmdConsole(TEXT("TD.Story.Console"),
		TEXT("TD.Story.Console [n]: stand the pilot in front of the n-th recording console of the interior (0 = nearest the start), facing it; E plays it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Pawn) { return; }
			TArray<ATDSurveyTerminal*> Consoles;
			for (TActorIterator<ATDSurveyTerminal> It(World); It; ++It)
			{
				if (!It->bAnnounceRecords) { Consoles.Add(*It); } // the interior's recordings (T5's story consoles)
			}
			const FVector From = Pawn->GetActorLocation();
			Consoles.Sort([&From](const ATDSurveyTerminal& A, const ATDSurveyTerminal& B) { return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From); });
			const int32 N = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
			if (!Consoles.IsValidIndex(N))
			{
				UE_LOG(LogTriadDrift, Warning, TEXT("TD.Story.Console: %d recording consoles, no %d"), Consoles.Num(), N);
				return;
			}
			const ATDSurveyTerminal* C = Consoles[N];
			const FVector Front = C->GetActorForwardVector().GetSafeNormal2D();
			const FVector At = C->GetActorLocation() + Front * 140.f + FVector(0.f, 0.f, 30.f);
			Pawn->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
			PC->SetControlRotation((C->GetActorLocation() - At).Rotation());
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Story.Console: at %s (%d of %d), in front of %s"), *At.ToCompactString(), N, Consoles.Num(), *C->GetName());
		}));
	static FAutoConsoleCommandWithWorldAndArgs CmdSubtitles(TEXT("TD.Story.Subtitles"),
		TEXT("TD.Story.Subtitles 0|1: the player's subtitles setting (as the Options menu sets it)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UTDSettingsSubsystem* S = UTDSettingsSubsystem::GetTDSettings(World))
			{
				S->SetSubtitlesEnabled(Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0);
				if (Args.Num() > 1) // TD.Story.Subtitles 1 Small|Medium|Large
				{
					S->SetSubtitleSize(Args[1].Equals(TEXT("Small"), ESearchCase::IgnoreCase) ? ETDSubtitleSize::Small
						: (Args[1].Equals(TEXT("Large"), ESearchCase::IgnoreCase) ? ETDSubtitleSize::Large : ETDSubtitleSize::Medium));
				}
				UE_LOG(LogTriadDrift, Log, TEXT("TD.Story.Subtitles: %s"), S->AreSubtitlesEnabled() ? TEXT("on") : TEXT("off"));
			}
		}));
	static FAutoConsoleCommandWithWorldAndArgs CmdContinue(TEXT("TD.Story.Continue"),
		TEXT("TD.Story.Continue: what the main menu's CONTINUE does (UTDGameInstance::ContinueRun)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			UTDGameInstance* GI = UTDGameInstance::Get(World);
			const bool bOk = GI && GI->ContinueRun();
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Story.Continue: %s"), bOk ? TEXT("continuing the saved run") : TEXT("nothing to continue"));
		}));
	static FAutoConsoleCommandWithWorldAndArgs CmdGoto(TEXT("TD.Story.Goto"),
		TEXT("TD.Story.Goto Trailhead <ObjectiveId> | Step | Survey: stand the on-foot pilot at a real course step (then press E for real)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Goto));
}

#endif // !UE_BUILD_SHIPPING

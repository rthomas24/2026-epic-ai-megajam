// Copyright (c) 2026 Triad Drift team. Epic AI MegaJam entry.
// T7 playtest aid (development builds only): agent-driven playtests walk the on-foot courses with real keys, which needs
// the objective marker's bearing. In a -game session there is no editor Python to read it, so this turns the pilot's view
// toward the nearest current objective marker and logs where it is; the playtest then holds W.
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace TDPlaytest
{
	static void FaceObjective(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
		ATDFirstPersonCharacter* Pilot = PC ? Cast<ATDFirstPersonCharacter>(PC->GetPawn()) : nullptr;
		if (!Pilot)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Play.FaceObjective: no pilot on foot"));
			return;
		}
		const FVector From = Pilot->GetActorLocation();
		AActor* Best = nullptr;
		double BestD = TNumericLimits<double>::Max();
		for (AActor* M : Pilot->GetObjectiveMarkers())
		{
			if (IsValid(M))
			{
				const double D = FVector::Dist2D(M->GetActorLocation(), From);
				if (D < BestD) { BestD = D; Best = M; }
			}
		}
		if (!Best)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Play.FaceObjective: no objective marker"));
			return;
		}
		const FVector To = Best->GetActorLocation() - From;
		const float Pitch = Args.Num() > 0 ? FCString::Atof(*Args[0]) : -5.f;
		PC->SetControlRotation(FRotator(Pitch, To.Rotation().Yaw, 0.f));
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Play.FaceObjective: %s at %.0f m, height %+.0f m (pos %s)"), *GetNameSafe(Best),
			BestD / 100.0, To.Z / 100.0, *Best->GetActorLocation().ToCompactString());
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdFaceObjective(TEXT("TD.Play.FaceObjective"),
		TEXT("TD.Play.FaceObjective [Pitch]: turn the on-foot pilot toward the nearest current objective marker (playtests)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FaceObjective));
}

#endif

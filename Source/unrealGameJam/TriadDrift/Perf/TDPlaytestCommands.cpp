// Copyright (c) 2026 Triad Drift team. Epic AI MegaJam entry.
// T7 playtest aid (development builds only): agent-driven playtests walk the on-foot courses with real keys, which needs
// the objective marker's bearing. In a -game session there is no editor Python to read it, so this turns the pilot's view
// toward the nearest current objective marker and logs where it is; the playtest then holds W.
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Engine/Engine.h"
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
		// The on-foot courses tag their live objective markers (the HUD reads the same tag).
		TArray<AActor*> Marks = Pilot->GetObjectiveMarkers();
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(World, FName(TEXT("TD.Objective.Marker")), Tagged);
		Marks.Append(Tagged);
		for (AActor* M : Marks)
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

	// Space: steer the flown ship at the nearest actor carrying a tag, through the real input (TD.Input.TurnToward), e.g.
	// TD.Play.AimShipAtTag TD.Docking.Approach 2
	static void AimShipAtTag(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || Args.Num() < 1)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Play.AimShipAtTag <Tag> [Seconds]: no pawn or tag"));
			return;
		}
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(World, FName(*Args[0]), Tagged);
		AActor* Best = nullptr;
		double BestD = TNumericLimits<double>::Max();
		for (AActor* A : Tagged)
		{
			const double D = FVector::Dist(A->GetActorLocation(), Pawn->GetActorLocation());
			if (D < BestD) { BestD = D; Best = A; }
		}
		if (!Best)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Play.AimShipAtTag: no actor tagged %s"), *Args[0]);
			return;
		}
		const FRotator To = (Best->GetActorLocation() - Pawn->GetActorLocation()).Rotation();
		const float Secs = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 2.f;
		GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.TurnToward %.1f %.1f %.1f"), To.Yaw, To.Pitch, Secs));
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Play.AimShipAtTag: %s at %.0f m (yaw %.0f pitch %.0f)"), *GetNameSafe(Best), BestD / 100.0, To.Yaw, To.Pitch);
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdAimShipAtTag(TEXT("TD.Play.AimShipAtTag"),
		TEXT("TD.Play.AimShipAtTag <Tag> [Seconds]: steer the flown ship at the nearest actor with the tag, by real input (playtests)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AimShipAtTag));

	static FAutoConsoleCommandWithWorldAndArgs CmdFaceObjective(TEXT("TD.Play.FaceObjective"),
		TEXT("TD.Play.FaceObjective [Pitch]: turn the on-foot pilot toward the nearest current objective marker (playtests)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FaceObjective));
}

#endif

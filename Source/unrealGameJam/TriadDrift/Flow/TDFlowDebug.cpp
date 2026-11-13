// Triad Drift - dev console: what the flow tells the player right now (phase, scene, take, objective line and
// the objective markers the HUD points at). For agents playing over MCP: they steer by the same markers. Owner: T5.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Core/TDTypes.h"

namespace TDFlowDebug
{
	static void Status(const TArray<FString>& Args, UWorld* World)
	{
		const UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr;
		if (!GI)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Flow: no game instance"));
			return;
		}
		const FTDProgress& P = GI->GetProgressRef();
		const FName Scene = GI->GetCurrentScene();
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Flow: map=%s phase=%s scene=%s take=%d retake=%d travel=%d waves=%d run=%.0fs kills=%d cuts=%d line='%s'"),
			*World->GetMapName(), *UEnum::GetValueAsString(P.Phase), *Scene.ToString(), Scene.IsNone() ? 0 : GI->GetTakeNumber(Scene),
			GI->IsRetakePending(), GI->IsTravelPending(), P.WavesCleared, P.RunTimeSeconds, P.Stats.Kills, P.Stats.Deaths,
			*GI->GetCurrentObjectiveText().ToString().Replace(TEXT("\n"), TEXT(" / ")));
		const APlayerController* PC = World->GetFirstPlayerController();
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		FVector Eye = FVector::ZeroVector;
		FRotator View = FRotator::ZeroRotator;
		if (PC)
		{
			PC->GetPlayerViewPoint(Eye, View);
		}
		if (Pawn)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Flow: pawn=%s at (%.0f %.0f %.0f) yaw=%.1f pitch=%.1f"), *Pawn->GetClass()->GetName(),
				Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y, Pawn->GetActorLocation().Z, View.Yaw, View.Pitch);
		}
		for (const AActor* M : GI->GetFlowMarkers())
		{
			if (!M)
			{
				continue;
			}
			const FVector To = M->GetActorLocation() - (Pawn ? Pawn->GetActorLocation() : Eye);
			const FRotator R = To.Rotation();
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Flow: marker=%s at (%.0f %.0f %.0f) dist=%.0f bearing_yaw=%.1f bearing_pitch=%.1f"), *M->GetName(),
				M->GetActorLocation().X, M->GetActorLocation().Y, M->GetActorLocation().Z, To.Size(), R.Yaw, R.Pitch);
		}
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdStatus(TEXT("TD.Flow.Status"),
		TEXT("Log the phase, scene, take, objective line and the objective markers (with bearings from the player)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Status));
}

#endif // !UE_BUILD_SHIPPING

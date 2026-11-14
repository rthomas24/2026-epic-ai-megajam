// Triad Drift - animation builder's dev command (Nov 14): drive the pilot like an analog stick, for captures of the
// slow walk, strafes and backpedals at a chosen speed (the keyboard only gives full speed).
//   TD.Pilot.Drive <ForwardCmS> <Seconds> [RightCmS]   relative to the view; negative = back / left. Game seconds.

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Containers/Ticker.h"
#include "GameFramework/WorldSettings.h"

namespace TDPilotAnimDebug
{
	static void Drive(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ACharacter* Char = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		if (!Char || Args.Num() < 2)
		{
			UE_LOG(LogTemp, Display, TEXT("TD.Pilot.Drive <ForwardCmS> <Seconds> [RightCmS]"));
			return;
		}
		const float Fwd = FCString::Atof(*Args[0]);
		const float Seconds = FCString::Atof(*Args[1]);
		const float Right = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.f;
		TWeakObjectPtr<ACharacter> Weak(Char);
		TWeakObjectPtr<APlayerController> WeakPC(PC);
		TSharedRef<float> Left = MakeShared<float>(Seconds);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, WeakPC, Fwd, Right, Left](float Dt)
		{
			ACharacter* C = Weak.Get();
			APlayerController* P = WeakPC.Get();
			if (!C || !P || !C->GetWorld())
			{
				return false;
			}
			*Left -= Dt * C->GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
			const UCharacterMovementComponent* Move = C->GetCharacterMovement();
			const float Max = Move ? FMath::Max(1.f, Move->GetMaxSpeed()) : 600.f;
			const FRotator Yaw(0.f, P->GetControlRotation().Yaw, 0.f);
			const FVector Want = Yaw.RotateVector(FVector(Fwd, Right, 0.f)) / Max; // input scale = wanted / max speed
			C->AddMovementInput(Want.GetSafeNormal(), FMath::Min(1.f, (float)Want.Size()));
			return *Left > 0.f;
		}));
		UE_LOG(LogTemp, Display, TEXT("TD.Pilot.Drive fwd %.0f right %.0f for %.2f s (max %.0f)"), Fwd, Right, Seconds,
			Char->GetCharacterMovement() ? Char->GetCharacterMovement()->GetMaxSpeed() : 0.f);
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdDrive(TEXT("TD.Pilot.Drive"),
		TEXT("TD.Pilot.Drive <ForwardCmS> <Seconds> [RightCmS]: move the pilot at that speed relative to the view (game seconds)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Drive));
}

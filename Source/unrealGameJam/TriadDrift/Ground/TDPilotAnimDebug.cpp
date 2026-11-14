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
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "TriadDrift/Ground/TDPilotAnim.h"

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

	// TD.Pilot.FootTrace <GameSeconds>: every frame, log the game time, both feet's world positions (foot_l / foot_r,
	// ball bones if present), the capsule velocity and the playing clips, for measuring foot slide while planted.
	static void FootTrace(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ACharacter* Char = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		if (!Char || !Char->GetMesh())
		{
			return;
		}
		const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 2.f;
		TWeakObjectPtr<ACharacter> Weak(Char);
		TSharedRef<float> Left = MakeShared<float>(Seconds);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Left](float Dt)
		{
			ACharacter* C = Weak.Get();
			if (!C || !C->GetWorld() || !C->GetMesh())
			{
				return false;
			}
			*Left -= Dt * C->GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
			USkeletalMeshComponent* M = C->GetMesh();
			const FName LB = M->GetBoneIndex(TEXT("ball_l")) != INDEX_NONE ? FName(TEXT("ball_l")) : FName(TEXT("foot_l"));
			const FName RB = M->GetBoneIndex(TEXT("ball_r")) != INDEX_NONE ? FName(TEXT("ball_r")) : FName(TEXT("foot_r"));
			const FVector L = M->GetSocketLocation(LB), R = M->GetSocketLocation(RB), V = C->GetVelocity();
			const UTDLoopBlendAnimInstance* A = Cast<UTDLoopBlendAnimInstance>(M->GetAnimInstance());
			UE_LOG(LogTemp, Display, TEXT("TD.FootTrace t=%.4f L=%.1f,%.1f,%.1f R=%.1f,%.1f,%.1f V=%.0f,%.0f yaw=%.1f loop=%s rate=%.2f legs=%s upper=%s shot=%s"),
				C->GetWorld()->GetTimeSeconds(), L.X, L.Y, L.Z, R.X, R.Y, R.Z, V.X, V.Y, C->GetActorRotation().Yaw,
				A ? *GetNameSafe(A->GetLoop()) : TEXT("-"), A ? A->GetRate() : 0.f, A ? *GetNameSafe(A->GetLegLoop()) : TEXT("-"),
				A ? *GetNameSafe(A->GetUpperLoop()) : TEXT("-"), A ? *GetNameSafe(A->GetOneShot()) : TEXT("-"));
			return *Left > 0.f;
		}));
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdFootTrace(TEXT("TD.Pilot.FootTrace"),
		TEXT("TD.Pilot.FootTrace <GameSeconds>: log both feet's world positions every frame (foot slide measurement)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FootTrace));
}

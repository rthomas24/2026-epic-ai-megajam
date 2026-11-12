// Triad Drift - dev console commands for agent playtests through MCP (no Shipping). Owner: Teammate 5.
//
//   TD.Key <KeyName> [Seconds] [Value]  press a real key on the first local player (InputKey), release after
//                                       Seconds (0 = one-frame tap). Goes through Enhanced Input like a keyboard.
//   TD.Look <YawDegrees> [PitchDegrees] set the first player's view
//   TD.Ground.Teleport <X> <Y> <Z>      move the player pawn
//   TD.Ground.Spawn <Grunt|Trooper|Heavy|Drone|Warden> [Distance]   spawn crew in front of the player

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

namespace TDGroundDebug
{
	static APlayerController* LocalPC(UWorld* World)
	{
		return World ? World->GetFirstPlayerController() : nullptr;
	}

	static void Key(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = LocalPC(World);
		if (!PC || Args.Num() < 1)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Key <KeyName> [Seconds] [Value]"));
			return;
		}
		const FKey K(*Args[0]);
		if (!K.IsValid())
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Key: unknown key '%s'"), *Args[0]);
			return;
		}
		const float Seconds = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f;
		const float Value = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 1.f;
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Pressed, Value));
		TWeakObjectPtr<APlayerController> WeakPC(PC);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakPC, K]()
		{
			if (APlayerController* P = WeakPC.Get())
			{
				P->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Released, 0.f));
			}
		}), FMath::Max(Seconds, 0.02f), false);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Key %s for %.2f s"), *K.ToString(), Seconds);
	}

	static void Look(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = LocalPC(World);
		if (PC && Args.Num() >= 1)
		{
			PC->SetControlRotation(FRotator(Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f, FCString::Atof(*Args[0]), 0.f));
		}
	}

	static void Teleport(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = LocalPC(World);
		if (PC && PC->GetPawn() && Args.Num() >= 3)
		{
			PC->GetPawn()->SetActorLocation(FVector(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2])), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	static void Spawn(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = LocalPC(World);
		APawn* P = PC ? PC->GetPawn() : nullptr;
		const UEnum* E = StaticEnum<ETDCrewType>();
		if (!P || !E || Args.Num() < 1)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Ground.Spawn <Grunt|Trooper|Heavy|Drone|Warden> [Distance]"));
			return;
		}
		const int64 Value = E->GetValueByNameString(Args[0]);
		if (Value == INDEX_NONE)
		{
			return;
		}
		const float Dist = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 800.f;
		const FVector Fwd = FRotator(0.f, PC->GetControlRotation().Yaw, 0.f).Vector();
		const FTransform Xf((-Fwd).Rotation(), P->GetActorLocation() + Fwd * Dist + FVector(0.f, 0.f, 60.f));
		ATDCrewCharacter* Crew = World->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Crew)
		{
			Crew->CrewType = static_cast<ETDCrewType>(Value);
			Crew->FinishSpawning(Xf);
		}
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdKey(TEXT("TD.Key"), TEXT("TD.Key <KeyName> [Seconds] [Value]: press a real key on the local player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Key));
	static FAutoConsoleCommandWithWorldAndArgs CmdLook(TEXT("TD.Look"), TEXT("TD.Look <Yaw> [Pitch]: set the player's view."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Look));
	static FAutoConsoleCommandWithWorldAndArgs CmdTeleport(TEXT("TD.Ground.Teleport"), TEXT("TD.Ground.Teleport <X> <Y> <Z>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Teleport));
	static FAutoConsoleCommandWithWorldAndArgs CmdSpawn(TEXT("TD.Ground.Spawn"), TEXT("TD.Ground.Spawn <CrewType> [Distance]: spawn crew facing the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Spawn));
}

#endif // !UE_BUILD_SHIPPING

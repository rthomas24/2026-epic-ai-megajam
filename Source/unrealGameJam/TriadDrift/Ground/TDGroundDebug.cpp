// Triad Drift - dev console commands for agent playtests through MCP (no Shipping). Owner: Teammate 5.
//
//   TD.Key <KeyName> [Seconds] [Value]  press a real key on the first local player (InputKey), release after
//                                       Seconds (0 = one-frame tap). Goes through Enhanced Input like a keyboard.
//   TD.Look <YawDegrees> [PitchDegrees] set the first player's view
//   TD.Ground.Teleport <X> <Y> <Z>      move the player pawn
//   TD.Ground.Spawn <Grunt|Trooper|Heavy|Drone|Warden> [Distance]   spawn crew in front of the player
//   TD.Ground.Where                    log the player position, the objective line and the nearest live enemy
//   TD.Ground.FaceNearest              aim at the nearest live enemy (agent playtests over MCP)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

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

	static APawn* NearestEnemy(UWorld* World, const FVector& From, float& OutDist)
	{
		APawn* Best = nullptr;
		OutDist = 1e12f;
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			const UTDHealthComponent* H = UTDStatics::GetHealth(*It);
			if (!H || !H->IsAlive() || UTDStatics::GetTeam(*It) != ETDTeam::Enemy)
			{
				continue;
			}
			const float D = (float)FVector::Dist(It->GetActorLocation(), From);
			if (D < OutDist) { OutDist = D; Best = *It; }
		}
		return Best;
	}

	static void Where(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = LocalPC(World);
		APawn* P = PC ? PC->GetPawn() : nullptr;
		if (!P) { return; }
		float D = 0.f;
		APawn* E = NearestEnemy(World, P->GetActorLocation(), D);
		const UTDGameInstance* GI = UTDGameInstance::Get(World);
		const FString Msg = FString::Printf(TEXT("TD.Where pos=(%.0f %.0f %.0f) yaw=%.0f obj='%s' nearest=%s at (%s) %.0fcm"),
			P->GetActorLocation().X, P->GetActorLocation().Y, P->GetActorLocation().Z, PC->GetControlRotation().Yaw,
			GI ? *GI->GetCurrentObjectiveText().ToString() : TEXT(""), *GetNameSafe(E), E ? *E->GetActorLocation().ToCompactString() : TEXT("-"), D);
		UE_LOG(LogTriadDrift, Display, TEXT("%s"), *Msg);
		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, Msg); }
	}

	static void FaceNearest(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = LocalPC(World);
		APawn* P = PC ? PC->GetPawn() : nullptr;
		float D = 0.f;
		APawn* E = P ? NearestEnemy(World, P->GetActorLocation(), D) : nullptr;
		if (E)
		{
			PC->SetControlRotation((E->GetActorLocation() - P->GetPawnViewLocation()).Rotation());
		}
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdWhere(TEXT("TD.Ground.Where"), TEXT("Log the player position, objective line and nearest enemy."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Where));
	static FAutoConsoleCommandWithWorldAndArgs CmdFace(TEXT("TD.Ground.FaceNearest"), TEXT("Aim at the nearest live enemy."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FaceNearest));
	static FAutoConsoleCommandWithWorldAndArgs CmdKey(TEXT("TD.Key"), TEXT("TD.Key <KeyName> [Seconds] [Value]: press a real key on the local player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Key));
	static FAutoConsoleCommandWithWorldAndArgs CmdLook(TEXT("TD.Look"), TEXT("TD.Look <Yaw> [Pitch]: set the player's view."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Look));
	static FAutoConsoleCommandWithWorldAndArgs CmdTeleport(TEXT("TD.Ground.Teleport"), TEXT("TD.Ground.Teleport <X> <Y> <Z>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Teleport));
	static FAutoConsoleCommandWithWorldAndArgs CmdSpawn(TEXT("TD.Ground.Spawn"), TEXT("TD.Ground.Spawn <CrewType> [Distance]: spawn crew facing the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Spawn));
}

namespace TDGroundDebug
{
	// TD.Ground.Bones: is every skeletal look standing? Head and pelvis heights above the feet, anim mode, montage.
	static void Bones(const TArray<FString>& Args, UWorld* World)
	{
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			const USkeletalMeshComponent* M = It->GetMesh();
			if (!M || !M->GetSkeletalMeshAsset() || !(It->IsA<ATDCrewCharacter>() || It->IsA<ATDFirstPersonCharacter>()))
			{
				continue;
			}
			const float Feet = (float)(It->GetActorLocation().Z - It->GetSimpleCollisionHalfHeight());
			auto Z = [M, Feet](const TCHAR* Bone) { return M->GetBoneIndex(Bone) != INDEX_NONE ? (float)M->GetBoneLocation(Bone).Z - Feet : -1.f; };
			const UAnimInstance* A = M->GetAnimInstance();
			const UAnimMontage* Mont = A ? A->GetCurrentActiveMontage() : nullptr;
			const UTDLoopBlendAnimInstance* L = Cast<UTDLoopBlendAnimInstance>(A);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Bones %s mesh=%s visible=%d mode=%d anim=%s montage=%s head=%.0f pelvis=%.0f root=%.0f sim=%d scale=%.2f loop=%s shot=%s upper=%s proc=%d"),
				*It->GetName(), *GetNameSafe(M->GetSkeletalMeshAsset()), M->IsVisible(), (int32)M->GetAnimationMode(), *GetNameSafe(A ? A->GetClass() : nullptr),
				*GetNameSafe(Mont), Z(TEXT("head")), Z(TEXT("pelvis")), Z(TEXT("root")), M->IsSimulatingPhysics(), (float)M->GetComponentScale().Z,
				*GetNameSafe(L ? L->GetLoop() : nullptr), *GetNameSafe(L ? L->GetOneShot() : nullptr), *GetNameSafe(L ? L->GetUpperLoop() : nullptr), L ? (int32)L->bProcedural : -1);
		}
	}
	static FAutoConsoleCommandWithWorldAndArgs CmdBones(TEXT("TD.Ground.Bones"), TEXT("Log head / pelvis heights and anim state of every skeletal crew and pilot body."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Bones));

	static ATDFirstPersonCharacter* Pilot(UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? Cast<ATDFirstPersonCharacter>(PC->GetPawn()) : nullptr;
	}

	// TD.Ground.ThirdPerson [0|1]: switch the on-foot camera (toggles without an argument). Not saved.
	static void ThirdPerson(const TArray<FString>& Args, UWorld* World)
	{
		if (ATDFirstPersonCharacter* P = Pilot(World))
		{
			const bool bWant = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !P->IsThirdPersonView();
			P->SetThirdPerson(bWant);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.ThirdPerson want=%d"), bWant);
		}
	}
	// TD.Ground.HandBlade X Y Z Pitch Yaw Roll: place the hand blade on hand_r live (to set HandBladeOffset by eye).
	static void HandBlade(const TArray<FString>& Args, UWorld* World)
	{
		ATDFirstPersonCharacter* P = Pilot(World);
		if (!P || !P->GetHandBladeHilt())
		{
			return;
		}
		if (Args.Num() >= 6)
		{
			const FTransform T(FRotator(FCString::Atof(*Args[3]), FCString::Atof(*Args[4]), FCString::Atof(*Args[5])),
				FVector(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2])));
			P->HandBladeOffset = T;
			P->GetHandBladeHilt()->SetRelativeLocationAndRotation(T.GetLocation(), T.Rotator());
		}
		const UStaticMeshComponent* Beam = P->GetHandBladeBeam();
		UE_LOG(LogTriadDrift, Display, TEXT("TD.HandBlade offset=%s hilt=%s visible=%d beam=%s visible=%d tip=%s extend=%.2f"),
			*P->HandBladeOffset.ToString(), *GetNameSafe(P->GetHandBladeHilt()->GetStaticMesh()), P->GetHandBladeHilt()->IsVisible(),
			*GetNameSafe(Beam->GetStaticMesh()), Beam->IsVisible(),
			*(Beam->GetComponentTransform().TransformPosition(FVector(100.f, 0.f, 0.f))).ToCompactString(), P->GetHandBladeExtend01());
	}
	static FAutoConsoleCommandWithWorldAndArgs CmdThird(TEXT("TD.Ground.ThirdPerson"), TEXT("TD.Ground.ThirdPerson [0|1]: switch the on-foot camera (not saved)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ThirdPerson));
	static FAutoConsoleCommandWithWorldAndArgs CmdHandBlade(TEXT("TD.Ground.HandBlade"), TEXT("TD.Ground.HandBlade [X Y Z Pitch Yaw Roll]: place / report the hand blade."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandBlade));
}

#endif // !UE_BUILD_SHIPPING

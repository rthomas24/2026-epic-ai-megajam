// Triad Drift - debug commands for the pilot's right-hand grip on the blade hilt (animation builder, Nov 13).
//   TD.Ground.Grip [0|1]                       grip layer off / on (toggles without an argument); logs its state
//   TD.Ground.GripHold <Clip> <Time01> [upper] hold a pilot clip (Pilot/Anims) at a fraction of its length; "off" stops
//   TD.Ground.GripSocket X Y Z Pitch Yaw Roll  try a hilt placement on hand_r live; "reset" puts it back on BladeSocket
//   TD.Ground.GripWhere                        log hand_r, the hilt and the finger joints in world space (for captures)
// Console only; nothing here runs in normal play.

#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMeshSocket.h"

namespace TDPilotGripDebug
{
	static ATDFirstPersonCharacter* GripPilot(UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? Cast<ATDFirstPersonCharacter>(PC->GetPawn()) : nullptr;
	}

	static UTDLoopBlendAnimInstance* GripLoops(ATDFirstPersonCharacter* P)
	{
		return P && P->GetMesh() ? Cast<UTDLoopBlendAnimInstance>(P->GetMesh()->GetAnimInstance()) : nullptr;
	}

	static void Grip(const TArray<FString>& Args, UWorld* World)
	{
		UTDLoopBlendAnimInstance* L = GripLoops(GripPilot(World));
		if (!L)
		{
			return;
		}
		L->bGripEnabled = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !L->bGripEnabled;
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Grip enabled=%d pose=%s alpha=%.2f hand=%s"), L->bGripEnabled,
			*GetNameSafe(L->GetGripPose()), L->GetGripAlpha(), *L->GripHandBone.ToString());
	}

	static void GripHold(const TArray<FString>& Args, UWorld* World)
	{
		UTDLoopBlendAnimInstance* L = GripLoops(GripPilot(World));
		if (!L || Args.Num() < 1)
		{
			return;
		}
		if (Args[0] == TEXT("off"))
		{
			L->StopOneShot();
			UE_LOG(LogTriadDrift, Display, TEXT("TD.GripHold off"));
			return;
		}
		FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/%s"), *Args[0]);
		if (!FPackageName::DoesPackageExist(Package))
		{
			Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Pilot/%s"), *Args[0]); // the look's own loops
		}
		UAnimSequenceBase* Seq = FPackageName::DoesPackageExist(Package) ? TDLoadPlayableAnim(FSoftObjectPath(Package + TEXT(".") + Args[0])) : nullptr;
		if (!Seq)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.GripHold: no clip %s"), *Package);
			return;
		}
		const float T01 = Args.Num() > 1 ? FMath::Clamp(FCString::Atof(*Args[1]), 0.f, 1.f) : 0.5f;
		const bool bUpper = Args.Num() > 2 && Args[2] == TEXT("upper");
		L->PlayOneShot(Seq, 0.f, bUpper, true);
		L->ShotTime = FMath::Min(T01 * Seq->GetPlayLength(), FMath::Max(0.f, Seq->GetPlayLength() - 0.001f));
		UE_LOG(LogTriadDrift, Display, TEXT("TD.GripHold %s at %.2f (%.3f s of %.3f) upper=%d"), *Args[0], T01, L->ShotTime, Seq->GetPlayLength(), bUpper);
	}

	static void GripSocket(const TArray<FString>& Args, UWorld* World)
	{
		ATDFirstPersonCharacter* P = GripPilot(World);
		UStaticMeshComponent* Hilt = P ? P->GetHandBladeHilt() : nullptr;
		USkeletalMeshComponent* Body = P ? P->GetMesh() : nullptr;
		if (!Hilt || !Body)
		{
			return;
		}
		const FVector Scale = Hilt->GetRelativeScale3D();
		if (Args.Num() >= 1 && Args[0] == TEXT("reset"))
		{
			const FName Socket = Body->DoesSocketExist(TEXT("BladeSocket")) ? FName(TEXT("BladeSocket")) : FName(TEXT("hand_r"));
			Hilt->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
			if (Socket == TEXT("hand_r"))
			{
				Hilt->SetRelativeTransform(P->HandBladeOffset);
			}
		}
		else if (Args.Num() >= 6)
		{
			Hilt->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("hand_r"));
			Hilt->SetRelativeLocationAndRotation(FVector(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2])),
				FRotator(FCString::Atof(*Args[3]), FCString::Atof(*Args[4]), FCString::Atof(*Args[5])));
		}
		Hilt->SetRelativeScale3D(Scale);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.GripSocket on=%s rel=%s"), *Hilt->GetAttachSocketName().ToString(), *Hilt->GetRelativeTransform().ToString());
	}

	static void GripWhere(const TArray<FString>& Args, UWorld* World)
	{
		ATDFirstPersonCharacter* P = GripPilot(World);
		USkeletalMeshComponent* Body = P ? P->GetMesh() : nullptr;
		if (!Body)
		{
			return;
		}
		auto Line = [](const TCHAR* What, const FTransform& T)
		{
			const FVector L = T.GetLocation();
			const FVector X = T.GetUnitAxis(EAxis::X), Y = T.GetUnitAxis(EAxis::Y), Z = T.GetUnitAxis(EAxis::Z);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.GripWhere %s loc=%.2f,%.2f,%.2f x=%.3f,%.3f,%.3f y=%.3f,%.3f,%.3f z=%.3f,%.3f,%.3f"), What,
				L.X, L.Y, L.Z, X.X, X.Y, X.Z, Y.X, Y.Y, Y.Z, Z.X, Z.Y, Z.Z);
		};
		Line(TEXT("actor"), P->GetActorTransform());
		for (const TCHAR* Bone : { TEXT("hand_r"), TEXT("lowerarm_r"), TEXT("index_01_r"), TEXT("index_02_r"), TEXT("index_03_r"), TEXT("middle_01_r"),
			TEXT("middle_02_r"), TEXT("middle_03_r"), TEXT("ring_01_r"), TEXT("ring_02_r"), TEXT("ring_03_r"), TEXT("pinky_01_r"), TEXT("pinky_02_r"),
			TEXT("pinky_03_r"), TEXT("thumb_01_r"), TEXT("thumb_02_r"), TEXT("thumb_03_r") })
		{
			if (Body->GetBoneIndex(Bone) != INDEX_NONE)
			{
				Line(Bone, Body->GetSocketTransform(Bone));
			}
		}
		if (const UStaticMeshComponent* Hilt = P->GetHandBladeHilt())
		{
			Line(TEXT("hilt"), Hilt->GetComponentTransform());
		}
		const UTDLoopBlendAnimInstance* L = GripLoops(P);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.GripWhere grip=%.2f enabled=%d shot=%s"), L ? L->GetGripAlpha() : -1.f, L ? (int32)L->bGripEnabled : -1,
			*GetNameSafe(L ? L->GetOneShot() : nullptr));
	}

#if WITH_EDITOR
	// TD.Ground.GripWriteSocket <SkeletonPath> X Y Z Pitch Yaw Roll: add or move BladeSocket on hand_r of a skeleton
	// (editor only; the authoring script ue_make_grip.py calls it, then saves the skeleton: Python cannot name a socket).
	static void GripWriteSocket(const TArray<FString>& Args, UWorld*)
	{
		USkeleton* Skel = Args.Num() >= 7 ? LoadObject<USkeleton>(nullptr, *Args[0]) : nullptr;
		if (!Skel)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.GripWriteSocket: usage <SkeletonPath> X Y Z Pitch Yaw Roll"));
			return;
		}
		static const FName SocketName(TEXT("BladeSocket"));
		USkeletalMeshSocket* Socket = nullptr;
		for (USkeletalMeshSocket* S : Skel->Sockets)
		{
			if (S && S->SocketName == SocketName)
			{
				Socket = S;
			}
		}
		Skel->Modify();
		if (!Socket)
		{
			Socket = NewObject<USkeletalMeshSocket>(Skel, NAME_None, RF_Transactional);
			Socket->SocketName = SocketName;
			Skel->Sockets.Add(Socket);
		}
		Socket->BoneName = TEXT("hand_r");
		Socket->RelativeLocation = FVector(FCString::Atod(*Args[1]), FCString::Atod(*Args[2]), FCString::Atod(*Args[3]));
		Socket->RelativeRotation = FRotator(FCString::Atod(*Args[4]), FCString::Atod(*Args[5]), FCString::Atod(*Args[6]));
		Socket->RelativeScale = FVector::OneVector;
		Skel->MarkPackageDirty();
		UE_LOG(LogTriadDrift, Display, TEXT("TD.GripWriteSocket %s: %s on %s at %s %s"), *Skel->GetPathName(), *Socket->SocketName.ToString(),
			*Socket->BoneName.ToString(), *Socket->RelativeLocation.ToString(), *Socket->RelativeRotation.ToString());
	}
	static FAutoConsoleCommandWithWorldAndArgs CmdGripWriteSocket(TEXT("TD.Ground.GripWriteSocket"),
		TEXT("Editor: TD.Ground.GripWriteSocket <SkeletonPath> X Y Z Pitch Yaw Roll - BladeSocket on hand_r."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GripWriteSocket));
#endif

	static FAutoConsoleCommandWithWorldAndArgs CmdGrip(TEXT("TD.Ground.Grip"), TEXT("TD.Ground.Grip [0|1]: the pilot's right-hand grip layer off / on."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Grip));
	static FAutoConsoleCommandWithWorldAndArgs CmdGripHold(TEXT("TD.Ground.GripHold"), TEXT("TD.Ground.GripHold <Clip>|off [Time01] [upper]: hold a pilot clip at a fraction."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GripHold));
	static FAutoConsoleCommandWithWorldAndArgs CmdGripSocket(TEXT("TD.Ground.GripSocket"), TEXT("TD.Ground.GripSocket X Y Z Pitch Yaw Roll | reset: hilt on hand_r live."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GripSocket));
	static FAutoConsoleCommandWithWorldAndArgs CmdGripWhere(TEXT("TD.Ground.GripWhere"), TEXT("Log hand_r, the finger joints and the hilt in world space."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GripWhere));
}

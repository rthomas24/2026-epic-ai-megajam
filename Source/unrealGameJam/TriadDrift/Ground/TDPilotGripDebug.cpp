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
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "EngineUtils.h"
#include "TriadDrift/Core/TDStatics.h"
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
		for (const TCHAR* Bone : { TEXT("hand_r"), TEXT("lowerarm_r"), TEXT("pelvis"), TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("index_01_r"), TEXT("index_02_r"), TEXT("index_03_r"), TEXT("middle_01_r"),
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

	// TD.Ground.CrewHold <Clip>|off [Time01]: the nearest live crew member holds a crew clip (Crew/Anims) at a fraction
	// and freezes there (actor time dilation ~0, so its AI and animation stop); logs where it stands, for captures.
	static void CrewHold(const TArray<FString>& Args, UWorld* World)
	{
		ATDFirstPersonCharacter* P = GripPilot(World);
		if (!P || Args.Num() < 1)
		{
			return;
		}
		ATDCrewCharacter* Best = nullptr;
		double BestD = TNumericLimits<double>::Max();
		for (TActorIterator<ATDCrewCharacter> It(World); It; ++It)
		{
			const double D = FVector::DistSquared(It->GetActorLocation(), P->GetActorLocation());
			if (!It->IsDead() && D < BestD)
			{
				Best = *It;
				BestD = D;
			}
		}
		UTDLoopBlendAnimInstance* L = Best && Best->GetMesh() ? Cast<UTDLoopBlendAnimInstance>(Best->GetMesh()->GetAnimInstance()) : nullptr;
		if (!L)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.CrewHold: no crew with the C++ loop player nearby"));
			return;
		}
		if (Args[0] == TEXT("off"))
		{
			L->StopOneShot();
			Best->CustomTimeDilation = 1.f;
			return;
		}
		const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/%s"), *Args[0]);
		UAnimSequenceBase* Seq = FPackageName::DoesPackageExist(Package) ? TDLoadPlayableAnim(FSoftObjectPath(Package + TEXT(".") + Args[0])) : nullptr;
		if (!Seq)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.CrewHold: no clip %s"), *Package);
			return;
		}
		const float T01 = Args.Num() > 1 ? FMath::Clamp(FCString::Atof(*Args[1]), 0.f, 1.f) : 0.5f;
		Best->CustomTimeDilation = 1.f;
		// An offscreen capture does not count as "rendered": evaluate and refresh the bones regardless, every frame.
		Best->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Best->GetMesh()->bEnableUpdateRateOptimizations = false;
		L->StopOneShot();
		L->PlayOneShot(Seq, 0.f, false, true);
		L->ShotTime = FMath::Min(T01 * Seq->GetPlayLength(), FMath::Max(0.f, Seq->GetPlayLength() - 0.001f));
		L->ShotAlpha = 1.f;
		Best->GetMesh()->TickAnimation(0.f, false);
		Best->GetMesh()->RefreshBoneTransforms();
		Best->CustomTimeDilation = 0.001f; // ~frozen: its AI and timers wait, the pose still evaluates each frame
		const FVector Loc = Best->GetActorLocation();
		const FVector X = Best->GetActorForwardVector(), Y = Best->GetActorRightVector();
		UE_LOG(LogTriadDrift, Display, TEXT("TD.CrewWhere %s loc=%.2f,%.2f,%.2f x=%.3f,%.3f,%.3f y=%.3f,%.3f,%.3f h=%.1f clip=%s t=%.3f"),
			*UEnum::GetValueAsString(Best->CrewType), Loc.X, Loc.Y, Loc.Z, X.X, X.Y, X.Z, Y.X, Y.Y, Y.Z,
			(float)Best->GetSimpleCollisionHalfHeight(), *Args[0], L->ShotTime);
	}
	static FAutoConsoleCommandWithWorldAndArgs CmdCrewHold(TEXT("TD.Ground.CrewHold"), TEXT("TD.Ground.CrewHold <Clip>|off [Time01]: nearest crew holds a crew clip, frozen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CrewHold));

	// TD.Ground.CrewKill: kill every live crew member with a real melee hit from the pilot (the game's own death path).
	// TD.Ground.CrewWhere: log every crew member, dead or alive: capsule, mesh root, pelvis and head, lowest bone.
	static void CrewKill(const TArray<FString>& Args, UWorld* World)
	{
		ATDFirstPersonCharacter* P = GripPilot(World);
		for (TActorIterator<ATDCrewCharacter> It(World); It && P; ++It)
		{
			if (!It->IsDead())
			{
				const FVector Dir = (It->GetActorLocation() - P->GetActorLocation()).GetSafeNormal2D();
				UTDStatics::ApplyTDDamage(*It, 1000000.f, ETDDamageKind::Melee, P, P, It->GetActorLocation(), Dir);
			}
		}
	}
	static void CrewWhere(const TArray<FString>& Args, UWorld* World)
	{
		for (TActorIterator<ATDCrewCharacter> It(World); It; ++It)
		{
			const USkeletalMeshComponent* M = It->GetMesh();
			if (!M || !M->GetSkeletalMeshAsset())
			{
				continue;
			}
			const float Floor = (float)(It->GetActorLocation().Z - It->GetSimpleCollisionHalfHeight());
			float Low = 1e6f;
			for (const TCHAR* B : { TEXT("pelvis"), TEXT("head"), TEXT("spine_03"), TEXT("hand_l"), TEXT("hand_r"), TEXT("foot_l"), TEXT("foot_r") })
			{
				if (M->GetBoneIndex(B) != INDEX_NONE)
				{
					Low = FMath::Min(Low, (float)M->GetBoneLocation(B).Z - Floor);
				}
			}
			const FVector A = It->GetActorLocation(), Pel = M->GetBoneLocation(TEXT("pelvis")), Head = M->GetBoneLocation(TEXT("head"));
			const FVector X = It->GetActorForwardVector(), Y = It->GetActorRightVector();
			UE_LOG(LogTriadDrift, Display, TEXT("TD.CrewAll %s dead=%d loc=%.1f,%.1f,%.1f x=%.3f,%.3f,%.3f y=%.3f,%.3f,%.3f pelvis=%.1f,%.1f,%.1f head=%.1f,%.1f,%.1f floor=%.1f low=%.1f"),
				*UEnum::GetValueAsString(It->CrewType), (int32)It->IsDead(), A.X, A.Y, A.Z, X.X, X.Y, X.Z, Y.X, Y.Y, Y.Z,
				Pel.X, Pel.Y, Pel.Z, Head.X, Head.Y, Head.Z, Floor, Low);
		}
	}
	static FAutoConsoleCommandWithWorldAndArgs CmdCrewKill(TEXT("TD.Ground.CrewKill"), TEXT("Kill every live crew member with a melee hit from the pilot."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CrewKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdCrewWhere(TEXT("TD.Ground.CrewWhere"), TEXT("Log every crew member's capsule, pelvis, head and lowest bone."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CrewWhere));

	static FAutoConsoleCommandWithWorldAndArgs CmdGrip(TEXT("TD.Ground.Grip"), TEXT("TD.Ground.Grip [0|1]: the pilot's right-hand grip layer off / on."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Grip));
	static FAutoConsoleCommandWithWorldAndArgs CmdGripHold(TEXT("TD.Ground.GripHold"), TEXT("TD.Ground.GripHold <Clip>|off [Time01] [upper]: hold a pilot clip at a fraction."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GripHold));
	static FAutoConsoleCommandWithWorldAndArgs CmdGripSocket(TEXT("TD.Ground.GripSocket"), TEXT("TD.Ground.GripSocket X Y Z Pitch Yaw Roll | reset: hilt on hand_r live."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GripSocket));
	static FAutoConsoleCommandWithWorldAndArgs CmdGripWhere(TEXT("TD.Ground.GripWhere"), TEXT("Log hand_r, the finger joints and the hilt in world space."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GripWhere));
}

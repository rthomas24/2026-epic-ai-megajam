// Triad Drift - debug commands for the pilot's right-hand grip on the blade hilt (animation builder, Nov 13).
//   TD.Ground.Grip [0|1]                       grip layer off / on (toggles without an argument); logs its state
//   TD.Ground.GripHold <Clip> <Time01> [upper] hold a pilot clip (Pilot/Anims) at a fraction of its length; "off" stops
//   TD.Ground.GripSocket X Y Z Pitch Yaw Roll  try a hilt placement on hand_r live; "reset" puts it back on BladeSocket
//   TD.Ground.GripWhere                        log hand_r, the hilt and the finger joints in world space (for captures)
// Console only; nothing here runs in normal play.

#include "Animation/AnimSingleNodeInstance.h"
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
#include "Containers/Ticker.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
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
				// "behind": the hit travels the way the crew member faces (a blow in the back, past a Heavy's shield)
				// "front" / "left" / "right": a blow from that side of the crew member, also with no instigator (past the shield)
				const FString How = Args.Num() > 0 ? Args[0].ToLower() : FString();
				const bool bBehind = !How.IsEmpty();
				const FVector Dir = How == TEXT("behind") ? It->GetActorForwardVector() : How == TEXT("front") ? -It->GetActorForwardVector()
					: How == TEXT("left") ? It->GetActorRightVector() : How == TEXT("right") ? -It->GetActorRightVector()
					: (It->GetActorLocation() - P->GetActorLocation()).GetSafeNormal2D();
				// a Heavy's shield arc is judged from the instigator's position: none here, so the blow counts as coming from behind
				UTDStatics::ApplyTDDamage(*It, 1000000.f, ETDDamageKind::Melee, bBehind ? nullptr : P, P, It->GetActorLocation(), Dir);
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
			UE_LOG(LogTriadDrift, Display, TEXT("TD.CrewAll %s dead=%d loc=%.1f,%.1f,%.1f x=%.3f,%.3f,%.3f y=%.3f,%.3f,%.3f pelvis=%.1f,%.1f,%.1f head=%.1f,%.1f,%.1f floor=%.1f low=%.1f clip=%s sim=%d"),
				*UEnum::GetValueAsString(It->CrewType), (int32)It->IsDead(), A.X, A.Y, A.Z, X.X, X.Y, X.Z, Y.X, Y.Y, Y.Z,
				Pel.X, Pel.Y, Pel.Z, Head.X, Head.Y, Head.Z, Floor, Low,
				*GetNameSafe(M->GetSingleNodeInstance() ? M->GetSingleNodeInstance()->GetAnimationAsset() : nullptr), (int32)M->IsSimulatingPhysics());
		}
	}
	static FAutoConsoleCommandWithWorldAndArgs CmdCrewKill(TEXT("TD.Ground.CrewKill"), TEXT("Kill every live crew member with a melee hit from the pilot."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CrewKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdCrewWhere(TEXT("TD.Ground.CrewWhere"), TEXT("Log every crew member's capsule, pelvis, head and lowest bone."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CrewWhere));

	// TD.Ground.CrewRagdoll: the nearest live crew member goes limp (skeletal ragdoll from its current pose); its pelvis
	// height above the floor is logged every 0.25 s for 4 s, with the highest point reached (a launch shows here).
	static void CrewRagdoll(const TArray<FString>& Args, UWorld* World)
	{
		ATDFirstPersonCharacter* P = GripPilot(World);
		ATDCrewCharacter* Best = nullptr;
		double BestD = TNumericLimits<double>::Max();
		for (TActorIterator<ATDCrewCharacter> It(World); It && P; ++It)
		{
			const double D = FVector::DistSquared(It->GetActorLocation(), P->GetActorLocation());
			if (!It->IsDead() && D < BestD)
			{
				Best = *It;
				BestD = D;
			}
		}
		USkeletalMeshComponent* M = Best ? Best->GetMesh() : nullptr;
		if (!M || !M->GetPhysicsAsset())
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.CrewRagdoll: no crew with a physics asset nearby"));
			return;
		}
		const float Floor = (float)(Best->GetActorLocation().Z - Best->GetSimpleCollisionHalfHeight());
		Best->CustomTimeDilation = 1.f;
		Best->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Best->GetCharacterMovement()->DisableMovement();
		M->SetCollisionProfileName(TEXT("Ragdoll"));
		M->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		M->SetAllBodiesSimulatePhysics(true);
		M->SetSimulatePhysics(true);
		M->WakeAllRigidBodies();
		if (Args.Num() > 0 && Args[0] == TEXT("impulse"))
		{
			// the kill push the death code gives a ragdoll: 450 cm/s along the blow (from the pilot) and 250 up
			const FVector Dir = (Best->GetActorLocation() - P->GetActorLocation()).GetSafeNormal2D();
			M->AddImpulse(Dir * 450.f + FVector(0.f, 0.f, 250.f), NAME_None, true);
		}
		TWeakObjectPtr<USkeletalMeshComponent> Weak(M);
		const FString Name = GetNameSafe(M->GetPhysicsAsset());
		TSharedRef<float> Time = MakeShared<float>(0.f);
		TSharedRef<float> Peak = MakeShared<float>(-1.e6f);
		TSharedRef<float> NextLog = MakeShared<float>(0.f);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Floor, Name, Time, Peak, NextLog](float Dt)
		{
			if (!Weak.IsValid())
			{
				return false;
			}
			*Time += Dt;
			const float H = (float)Weak->GetBoneLocation(TEXT("pelvis")).Z - Floor;
			*Peak = FMath::Max(*Peak, H);
			if (*Time >= *NextLog)
			{
				*NextLog += 0.25f;
				UE_LOG(LogTriadDrift, Display, TEXT("TD.CrewRagdoll %s t=%.2f pelvis=%.0f cm peak=%.0f cm speed=%.0f"), *Name, *Time, H, *Peak,
					(float)Weak->GetPhysicsLinearVelocity(TEXT("pelvis")).Size());
			}
			return *Time < 4.f;
		}));
	}
	static FAutoConsoleCommandWithWorldAndArgs CmdCrewRagdoll(TEXT("TD.Ground.CrewRagdoll"), TEXT("Nearest crew goes limp; logs its pelvis height for 4 s."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CrewRagdoll));

#if WITH_EDITOR
	// TD.Ground.PAReport <PhysicsAssetPath> [fix]: list a physics asset's bodies (bone, shapes, size, mass override) and how
	// many body pairs collide with each other. "fix": no body collides with another body of the same ragdoll (it still
	// collides with the world) and every body's depenetration speed is capped, so bodies that overlap in the reference
	// pose (the Warden's chest shield and gun arm) cannot throw the ragdoll into the air. The caller saves the asset.
	static void PAReport(const TArray<FString>& Args, UWorld*)
	{
		UPhysicsAsset* PA = Args.Num() >= 1 ? LoadObject<UPhysicsAsset>(nullptr, *Args[0]) : nullptr;
		if (!PA)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.PAReport: usage <PhysicsAssetPath> [fix]"));
			return;
		}
		const bool bFix = Args.Num() > 1 && Args[1] == TEXT("fix");
		const int32 N = PA->SkeletalBodySetups.Num();
		int32 Pairs = 0;
		for (int32 i = 0; i < N; ++i)
		{
			for (int32 j = i + 1; j < N; ++j)
			{
				Pairs += PA->IsCollisionEnabled(i, j) ? 1 : 0;
			}
		}
		UE_LOG(LogTriadDrift, Display, TEXT("TD.PAReport %s: %d bodies, %d constraints, %d of %d body pairs collide"), *PA->GetName(), N,
			PA->ConstraintSetup.Num(), Pairs, N * (N - 1) / 2);
		if (bFix)
		{
			PA->Modify();
		}
		for (int32 i = 0; i < N; ++i)
		{
			USkeletalBodySetup* B = PA->SkeletalBodySetups[i];
			if (!B)
			{
				continue;
			}
			const FVector Size = B->AggGeom.CalcAABB(FTransform::Identity).GetSize();
			UE_LOG(LogTriadDrift, Display, TEXT("TD.PAReport   %s: %d spheres %d capsules %d boxes %d convex, size %.0f x %.0f x %.0f cm, mass override %d (%.0f kg)"),
				*B->BoneName.ToString(), B->AggGeom.SphereElems.Num(), B->AggGeom.SphylElems.Num(), B->AggGeom.BoxElems.Num(), B->AggGeom.ConvexElems.Num(),
				Size.X, Size.Y, Size.Z, (int32)B->DefaultInstance.bOverrideMass, B->DefaultInstance.GetMassOverride());
			if (bFix)
			{
				B->Modify();
				B->DefaultInstance.SetMaxDepenetrationVelocity(100.f);
				// Bodies fitted round a rigid prop (the chest shield: 220 x 136 x 173 cm; the gun arm: 119 cm) are cut back
				// to the limb itself: a capsule of at most 32 cm radius and 70 cm length, near its bone.
				for (FKSphylElem& C : B->AggGeom.SphylElems)
				{
					const float OldR = C.Radius, OldL = C.Length;
					C.Radius = FMath::Min(C.Radius, 32.f);
					C.Length = FMath::Min(C.Length, 70.f);
					if (C.Radius != OldR || C.Length != OldL)
					{
						C.Center = C.Center.GetClampedToMaxSize(20.f);
						UE_LOG(LogTriadDrift, Display, TEXT("TD.PAReport   %s: capsule %.0f / %.0f -> %.0f / %.0f cm (radius / length)"), *B->BoneName.ToString(), OldR, OldL, C.Radius, C.Length);
					}
				}
				B->InvalidatePhysicsData();
				B->CreatePhysicsMeshes();
				for (int32 j = i + 1; j < N; ++j)
				{
					PA->DisableCollision(i, j);
				}
			}
		}
		if (bFix)
		{
			PA->MarkPackageDirty();
			UE_LOG(LogTriadDrift, Display, TEXT("TD.PAReport %s: fixed (no body-to-body collision, depenetration capped at 100 cm/s)"), *PA->GetName());
		}
	}
	static FAutoConsoleCommandWithWorldAndArgs CmdPAReport(TEXT("TD.Ground.PAReport"), TEXT("Editor: TD.Ground.PAReport <PhysicsAssetPath> [fix]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PAReport));
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

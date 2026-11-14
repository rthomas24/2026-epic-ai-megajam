#include "TriadDrift/Ground/TDPilotAnim.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimNodeBase.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "Animation/AnimSequence.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "Components/StaticMeshComponent.h"

UAnimSequenceBase* TDMakePlayable(UAnimSequenceBase* Anim)
{
	// Kept alive for the session once used. A sequence that was garbage-collected (its last crew member gone, a test
	// world torn down) and loaded again evaluated to an empty pose - every bone at the root - in every later world
	// (Nov 12 probe: first world stands, every later one collapses). No compression calls: forcing or waiting on it
	// from gameplay made it worse.
	if (Anim && !Anim->IsRooted())
	{
		Anim->AddToRoot();
	}
	return Anim;
}

UAnimSequenceBase* TDLoadPlayableAnim(const FSoftObjectPath& Path)
{
	return TDMakePlayable(Cast<UAnimSequenceBase>(Path.TryLoad()));
}

namespace TDLoopBlend
{
	/** Evaluates the current loop and, while a cross-fade runs, blends the previous one under it. */
	struct FProxy : public FAnimInstanceProxy
	{
		FProxy() = default;
		explicit FProxy(UAnimInstance* InInstance) : FAnimInstanceProxy(InInstance) {}

		const UAnimSequenceBase* Current = nullptr;
		const UAnimSequenceBase* Previous = nullptr;
		const UAnimSequenceBase* Shot = nullptr;
		const UAnimSequenceBase* Upper = nullptr;
		double UpperTime = 0.0;
		float UpperAlpha = 0.f;
		bool bShotUpper = false;
		FName UpperRoot;
		bool bProcedural = false;
		float ArmDown = 0.f, Speed01 = 0.f, Phase = 0.f, Attack = 0.f, Flinch = 0.f, SideSign = 1.f, ForwardSign = 1.f;
		FVector ForwardCS = FVector(0.f, 1.f, 0.f);
		double ShotTime = 0.0;
		float ShotAlpha = 0.f;
		float FootUpL = 0.f, FootUpR = 0.f, PelvisDown = 0.f;
		float GaitLean = 0.f;
		float GaitDrop = 0.f;
		const UAnimSequenceBase* Legs = nullptr;
		double LegTime = 0.0;
		float LegAlpha = 0.f;
		const UAnimSequenceBase* PrevShot = nullptr;
		double PrevShotTime = 0.0;
		float PrevShotAlpha = 0.f;
		bool bPrevShotUpper = false;
		const UAnimSequenceBase* Grip = nullptr;
		float GripAlpha = 0.f;
		FName GripHand;
		double CurrentTime = 0.0;
		double PreviousTime = 0.0;
		float Alpha = 1.f;

		virtual void PreUpdate(UAnimInstance* InInstance, float DeltaSeconds) override
		{
			FAnimInstanceProxy::PreUpdate(InInstance, DeltaSeconds);
			const UTDLoopBlendAnimInstance* A = CastChecked<UTDLoopBlendAnimInstance>(InInstance);
			Current = A->Current;
			Previous = A->Previous;
			CurrentTime = A->CurrentTime;
			PreviousTime = A->PreviousTime;
			Alpha = A->Alpha;
			Shot = A->Shot;
			ShotTime = A->ShotTime;
			ShotAlpha = A->ShotAlpha;
			bShotUpper = A->bShotUpper;
			FootUpL = A->FootUpL;
			FootUpR = A->FootUpR;
			PelvisDown = A->PelvisDown;
			GaitLean = A->GaitLean;
			GaitDrop = A->GaitDrop;
			Legs = A->Legs;
			LegTime = A->LegTime;
			LegAlpha = A->LegAlpha;
			PrevShot = A->PrevShot;
			PrevShotTime = A->PrevShotTime;
			PrevShotAlpha = A->PrevShotAlpha;
			bPrevShotUpper = A->bPrevShotUpper;
			Upper = A->Upper;
			UpperTime = A->UpperTime;
			UpperAlpha = A->UpperAlpha;
			UpperRoot = A->UpperRootBone;
			bProcedural = A->bProcedural;
			ArmDown = A->ProcArmDown;
			Speed01 = A->ProcSpeed01;
			Phase = A->ProcPhase;
			Attack = A->ProcAttack;
			Flinch = A->ProcFlinch;
			SideSign = A->ProcSideSign;
			ForwardSign = A->ProcForwardSign;
			ForwardCS = A->ProcForwardCS;
			Grip = A->GripPose;
			GripAlpha = A->GripAlpha;
			GripHand = A->GripHandBone;
		}

		/** The grip pose over the hand bone's children (fingers, thumb, metacarpals), local space, by GripAlpha. */
		void ApplyGrip(FPoseContext& Output)
		{
			const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
			const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
			const int32 Hand = GripHand.IsNone() ? INDEX_NONE : Ref.FindBoneIndex(GripHand);
			if (Hand == INDEX_NONE)
			{
				return;
			}
			FPoseContext Fist(this);
			FAnimationPoseData FistData(Fist);
			Grip->GetAnimationPose(FistData, FAnimExtractContext(0.0, false, {}, false));
			for (const FCompactPoseBoneIndex B : Output.Pose.ForEachBoneIndex())
			{
				const int32 Mesh = Bones.MakeMeshPoseIndex(B).GetInt();
				if (Mesh != Hand && Ref.BoneIsChildOf(Mesh, Hand))
				{
					Output.Pose[B].BlendWith(Fist.Pose[B], GripAlpha);
				}
			}
		}

		/** Rotate a bone by Delta given in COMPONENT space (about its own pivot), using the reference pose's parent frame. */
		static void RotateCS(FCompactPose& Pose, const FReferenceSkeleton& Ref, FName Bone, const FQuat& Delta)
		{
			const int32 MeshIndex = Ref.FindBoneIndex(Bone);
			if (MeshIndex == INDEX_NONE)
			{
				return;
			}
			const FCompactPoseBoneIndex Compact = Pose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
			if (!Compact.IsValid())
			{
				return;
			}
			const int32 Parent = Ref.GetParentIndex(MeshIndex);
			const FQuat ParentCS = Parent != INDEX_NONE ? FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Parent).GetRotation() : FQuat::Identity;
			FTransform& Local = Pose[Compact];
			Local.SetRotation((ParentCS.Inverse() * Delta * ParentCS * Local.GetRotation()).GetNormalized());
		}

		void EvaluateProcedural(FPoseContext& Output) const
		{
			Output.ResetToRefPose();
			const FReferenceSkeleton& Ref = Output.Pose.GetBoneContainer().GetReferenceSkeleton();
			// Axes from the body itself, whatever the mesh's import convention: the owner passes its facing in component
			// space; a positive turn about Swing carries a hanging limb forward (u x v takes u toward v).
			FVector Fwd = FVector(ForwardCS.X, ForwardCS.Y, 0.f).GetSafeNormal();
			if (Fwd.IsNearlyZero())
			{
				Fwd = FVector(0.f, 1.f, 0.f);
			}
			Fwd *= ForwardSign;
			const FVector Swing = (FVector::CrossProduct(FVector(0.f, 0.f, -1.f), Fwd) * SideSign).GetSafeNormal();
			auto Q = [](const FVector& Axis, float Deg) { return FQuat(Axis, FMath::DegreesToRadians(Deg)); };
			auto CS = [&Ref](const TCHAR* Bone) -> FVector
			{
				const int32 I = Ref.FindBoneIndex(Bone);
				return I != INDEX_NONE ? FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, I).GetLocation() : FVector::ZeroVector;
			};
			// Arms down out of the T/A-pose: about the axis that takes each arm's outward direction toward straight down.
			for (const TCHAR* Side2 : { TEXT("l"), TEXT("r") })
			{
				const FString UpperArm = FString::Printf(TEXT("upperarm_%s"), Side2);
				const FString Lower = FString::Printf(TEXT("lowerarm_%s"), Side2);
				const FVector Out = (CS(*Lower) - CS(*UpperArm)).GetSafeNormal();
				const FVector Axis = FVector::CrossProduct(Out, FVector(0.f, 0.f, -1.f)).GetSafeNormal();
				if (!Axis.IsNearlyZero())
				{
					// Only as far as straight down: the reference arm may already hang partly (A-pose).
					const float Below = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(-Out.Z, -1.f, 1.f)));
					RotateCS(Output.Pose, Ref, *UpperArm, Q(Axis, FMath::Clamp(ArmDown + 20.f - Below, 0.f, 85.f)));
				}
			}
			const FVector Side = Swing;
			// Stride.
			const float S = FMath::Sin(Phase);
			const float Leg = 32.f * Speed01;
			const float Arm = 24.f * Speed01;
			RotateCS(Output.Pose, Ref, TEXT("thigh_l"), Q(Side, Leg * S));
			RotateCS(Output.Pose, Ref, TEXT("thigh_r"), Q(Side, -Leg * S));
			RotateCS(Output.Pose, Ref, TEXT("calf_l"), Q(Side, -40.f * Speed01 * FMath::Max(0.f, -S)));
			RotateCS(Output.Pose, Ref, TEXT("calf_r"), Q(Side, -40.f * Speed01 * FMath::Max(0.f, S)));
			RotateCS(Output.Pose, Ref, TEXT("upperarm_l"), Q(Side, -Arm * S));
			RotateCS(Output.Pose, Ref, TEXT("upperarm_r"), Q(Side, Arm * S + 70.f * Attack));
			RotateCS(Output.Pose, Ref, TEXT("lowerarm_r"), Q(Side, -35.f * FMath::Abs(Attack)));
			// Lean into a run, flinch back from a hit, breathe at rest.
			RotateCS(Output.Pose, Ref, TEXT("spine_01"), Q(Side, -6.f * Speed01 + 18.f * Flinch + 1.5f * FMath::Sin(Phase * 0.5f) * (1.f - Speed01)));
		}

		/** Blend Over into Output by W on UpperRoot and its children only (local space); everything without the root bone: all bones. */
		void BlendUpper(FPoseContext& Output, const FPoseContext& Over, float W) const
		{
			const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
			const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
			const int32 Root = UpperRoot.IsNone() ? INDEX_NONE : Ref.FindBoneIndex(UpperRoot);
			for (const FCompactPoseBoneIndex B : Output.Pose.ForEachBoneIndex())
			{
				const int32 Mesh = Bones.MakeMeshPoseIndex(B).GetInt();
				if (Root == INDEX_NONE || Mesh == Root || Ref.BoneIsChildOf(Mesh, Root))
				{
					Output.Pose[B].BlendWith(Over.Pose[B], W);
				}
			}
		}

		/** Blend Over into Output by W on every bone OUTSIDE UpperRoot's subtree (root, pelvis, legs), local space. */
		void BlendLower(FPoseContext& Output, const FPoseContext& Over, float W) const
		{
			const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
			const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
			const int32 Root = UpperRoot.IsNone() ? INDEX_NONE : Ref.FindBoneIndex(UpperRoot);
			if (Root == INDEX_NONE)
			{
				return;
			}
			// The rotation the upper body hangs from (UpperRoot's parent chain), before and after: the chest keeps its
			// facing however the strafe clip turns the hips (an orientation-warp-like counter-twist).
			const int32 Parent = Ref.GetParentIndex(Root);
			auto ChainRot = [&Bones, &Ref, &Output](int32 MeshIndex)
			{
				FQuat Q = FQuat::Identity;
				for (int32 I = MeshIndex; I != INDEX_NONE; I = Ref.GetParentIndex(I))
				{
					const FCompactPoseBoneIndex C = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(I));
					if (C.IsValid())
					{
						Q = Output.Pose[C].GetRotation() * Q;
					}
				}
				return Q;
			};
			const FQuat Before = Parent != INDEX_NONE ? ChainRot(Parent) : FQuat::Identity;
			for (const FCompactPoseBoneIndex B : Output.Pose.ForEachBoneIndex())
			{
				const int32 Mesh = Bones.MakeMeshPoseIndex(B).GetInt();
				if (Mesh != Root && !Ref.BoneIsChildOf(Mesh, Root))
				{
					Output.Pose[B].BlendWith(Over.Pose[B], W);
				}
			}
			const FCompactPoseBoneIndex RootC = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Root));
			if (Parent != INDEX_NONE && RootC.IsValid())
			{
				const FQuat After = ChainRot(Parent);
				Output.Pose[RootC].SetRotation((After.Inverse() * Before * Output.Pose[RootC].GetRotation()).GetNormalized());
			}
		}

		/** Component-space transform of a bone from the local pose (walks the parents). */
		static FTransform CSOf(const FCompactPose& Pose, const FReferenceSkeleton& Ref, int32 Mesh)
		{
			FTransform T = FTransform::Identity;
			for (int32 I = Mesh; I != INDEX_NONE; I = Ref.GetParentIndex(I))
			{
				const FCompactPoseBoneIndex C = Pose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(I));
				if (C.IsValid())
				{
					T = T * Pose[C];
				}
			}
			return T;
		}

		/** Lift one foot by Up (component cm) with a two-bone solve on thigh / calf; the foot keeps its CS rotation. */
		static void LiftFoot(FCompactPose& Pose, const FReferenceSkeleton& Ref, const TCHAR* Thigh, const TCHAR* Calf, const TCHAR* Foot, float Up)
		{
			const int32 HI = Ref.FindBoneIndex(Thigh), KI = Ref.FindBoneIndex(Calf), FI = Ref.FindBoneIndex(Foot);
			if (HI == INDEX_NONE || KI == INDEX_NONE || FI == INDEX_NONE || FMath::IsNearlyZero(Up, 0.2f))
			{
				return;
			}
			const FBoneContainer& Bones = Pose.GetBoneContainer();
			const FCompactPoseBoneIndex HC = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(HI));
			const FCompactPoseBoneIndex KC = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(KI));
			const FCompactPoseBoneIndex FC = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(FI));
			if (!HC.IsValid() || !KC.IsValid() || !FC.IsValid())
			{
				return;
			}
			const FTransform ParentCS = CSOf(Pose, Ref, Ref.GetParentIndex(HI));
			const FTransform HCS = Pose[HC] * ParentCS;
			const FTransform KCS = Pose[KC] * HCS;
			const FTransform FCS = Pose[FC] * KCS;
			const FVector H = HCS.GetLocation(), K = KCS.GetLocation(), F = FCS.GetLocation();
			const FVector T = F + FVector(0.f, 0.f, Up);
			const double L1 = (K - H).Size(), L2 = (F - K).Size();
			const FVector ToT = T - H;
			const double D = FMath::Clamp(ToT.Size(), FMath::Abs(L1 - L2) + 0.5, L1 + L2 - 0.1);
			const FVector Dir = ToT.GetSafeNormal();
			FVector Bend = (K - H) - Dir * FVector::DotProduct(K - H, Dir);
			Bend = Bend.IsNearlyZero() ? FVector(0.f, 1.f, 0.f) : Bend.GetSafeNormal();
			const double A = (L1 * L1 + D * D - L2 * L2) / (2.0 * D);
			const double Hh = FMath::Sqrt(FMath::Max(0.0, L1 * L1 - A * A));
			const FVector K2 = H + Dir * A + Bend * Hh;
			const FVector T2 = H + Dir * D;
			const FQuat Q1 = FQuat::FindBetweenVectors(K - H, K2 - H);
			const FQuat HRot = Q1 * HCS.GetRotation();
			const FQuat KRot1 = Q1 * KCS.GetRotation();
			const FQuat Q2 = FQuat::FindBetweenVectors(Q1.RotateVector(F - K), T2 - K2);
			const FQuat KRot = Q2 * KRot1;
			Pose[HC].SetRotation((ParentCS.GetRotation().Inverse() * HRot).GetNormalized());
			Pose[KC].SetRotation((HRot.Inverse() * KRot).GetNormalized());
			Pose[FC].SetRotation((KRot.Inverse() * FCS.GetRotation()).GetNormalized());
		}

		void ApplyFootIK(FPoseContext& Output) const
		{
			if (FMath::IsNearlyZero(FootUpL, 0.2f) && FMath::IsNearlyZero(FootUpR, 0.2f) && FMath::IsNearlyZero(PelvisDown, 0.2f))
			{
				return;
			}
			const FReferenceSkeleton& Ref = Output.Pose.GetBoneContainer().GetReferenceSkeleton();
			const int32 PelvisMesh = Ref.FindBoneIndex(TEXT("pelvis"));
			const FCompactPoseBoneIndex Pelvis = PelvisMesh != INDEX_NONE ? Output.Pose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(PelvisMesh)) : FCompactPoseBoneIndex(INDEX_NONE);
			if (Pelvis.IsValid())
			{
				Output.Pose[Pelvis].AddToTranslation(FVector(0.f, 0.f, -PelvisDown)); // the feet come down with it ...
			}
			// ... and are lifted back by PelvisDown, plus their own ground offset.
			LiftFoot(Output.Pose, Ref, TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), FootUpL);
			LiftFoot(Output.Pose, Ref, TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), FootUpR);
		}

		virtual bool Evaluate(FPoseContext& Output) override
		{
			if (bProcedural)
			{
				EvaluateProcedural(Output);
				return true;
			}
			if (!Current)
			{
				return false; // reference pose
			}
			FAnimationPoseData OutData(Output);
			Current->GetAnimationPose(OutData, FAnimExtractContext(CurrentTime, false, {}, true));
			if (Previous && Alpha < 1.f)
			{
				FPoseContext Under(this);
				FAnimationPoseData UnderData(Under);
				Previous->GetAnimationPose(UnderData, FAnimExtractContext(PreviousTime, false, {}, true));
				FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, UnderData, Alpha);
			}
			if (Legs && LegAlpha > 0.f)
			{
				FPoseContext Step(this);
				FAnimationPoseData StepData(Step);
				Legs->GetAnimationPose(StepData, FAnimExtractContext(LegTime, false, {}, true));
				BlendLower(Output, Step, LegAlpha);
			}
			// Start / stop: lean into the first step, rock back and settle on a stop (spine_01 pitch about the body's
			// right axis, mesh space: forward +Y, right -X), the pelvis dipping with it.
			if (!FMath::IsNearlyZero(GaitLean) || !FMath::IsNearlyZero(GaitDrop))
			{
				const FReferenceSkeleton& Ref = Output.Pose.GetBoneContainer().GetReferenceSkeleton();
				RotateCS(Output.Pose, Ref, UpperRoot, FQuat(FVector(-1.f, 0.f, 0.f), FMath::DegreesToRadians(GaitLean)));
				const int32 PelvisMesh = Ref.FindBoneIndex(TEXT("pelvis"));
				const FCompactPoseBoneIndex Pelvis = PelvisMesh != INDEX_NONE ? Output.Pose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(PelvisMesh)) : FCompactPoseBoneIndex(INDEX_NONE);
				if (Pelvis.IsValid())
				{
					Output.Pose[Pelvis].AddToTranslation(FVector(0.f, 0.f, -GaitDrop));
				}
			}
			if (Upper && UpperAlpha > 0.f)
			{
				FPoseContext Guard(this);
				FAnimationPoseData GuardData(Guard);
				Upper->GetAnimationPose(GuardData, FAnimExtractContext(UpperTime, false, {}, true));
				BlendUpper(Output, Guard, UpperAlpha);
			}
			// The swing being replaced (a chained combo hit) fades out under the new one instead of snapping to its
			// first frame: the hand carries on from where the last cut ended.
			if (PrevShot && PrevShotAlpha > 0.f)
			{
				FPoseContext Out(this);
				FAnimationPoseData OutShotData(Out);
				PrevShot->GetAnimationPose(OutShotData, FAnimExtractContext(PrevShotTime, false, {}, false));
				if (bPrevShotUpper)
				{
					BlendUpper(Output, Out, PrevShotAlpha);
				}
				else
				{
					FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, OutShotData, 1.f - PrevShotAlpha);
				}
			}
			if (Shot && ShotAlpha > 0.f)
			{
				FPoseContext Over(this);
				FAnimationPoseData OverData(Over);
				Shot->GetAnimationPose(OverData, FAnimExtractContext(ShotTime, false, {}, false));
				if (bShotUpper)
				{
					BlendUpper(Output, Over, ShotAlpha);
				}
				else
				{
					FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, OverData, 1.f - ShotAlpha);
				}
			}
			// The hand closes round the hilt over whatever the clips did (none of them grip).
			if (Grip && GripAlpha > 0.f)
			{
				ApplyGrip(Output);
			}
			ApplyFootIK(Output);
			// In place, always: a sequence that keeps root motion (the crew attacks travel 1.4-1.5 m) would walk the
			// body out of its capsule. The capsule does the moving.
			if (Output.Pose.GetNumBones() > 0)
			{
				FTransform& Root = Output.Pose[FCompactPoseBoneIndex(0)];
				Root.SetTranslation(FVector(0.f, 0.f, Root.GetTranslation().Z));
			}
			return true;
		}
	};
}

void UTDLoopBlendAnimInstance::SetLoop(UAnimSequenceBase* Loop, float Rate)
{
	PlayRate = Rate;
	if (Loop == Current)
	{
		return;
	}
	TDMakePlayable(Loop);
	Previous = Current;
	PreviousTime = CurrentTime;
	// Keep the stride phase across a walk/run swap so the feet don't skip.
	const float OldLength = Previous ? Previous->GetPlayLength() : 0.f;
	const double Phase = OldLength > 0.f ? FMath::Fmod(PreviousTime, (double)OldLength) / OldLength : 0.0;
	Current = Loop;
	CurrentTime = Current ? Phase * Current->GetPlayLength() : 0.0;
	Alpha = Previous ? 0.f : 1.f;
}

void UTDLoopBlendAnimInstance::SetGait(int32 NewGait)
{
	if (NewGait == Gait)
	{
		return;
	}
	if (TDPilotMeleePassOn())
	{
		if (Gait == 0 && NewGait > 0)
		{
			StartLeft = StartSeconds;
			StopLeft = 0.f;
		}
		else if (Gait > 0 && NewGait == 0)
		{
			StopLeft = StopSeconds;
			StartLeft = 0.f;
		}
	}
	Gait = NewGait;
}

void UTDLoopBlendAnimInstance::SetLegLoop(UAnimSequenceBase* Loop, float Rate)
{
	bLegsWanted = Loop != nullptr;
	if (Loop)
	{
		LegRate = Rate;
		if (Loop != Legs)
		{
			// A new direction: start from the main loop's stride phase and fade in from the legs on screen.
			const float MainLength = Current ? Current->GetPlayLength() : 0.f;
			const double Phase = MainLength > 0.f ? FMath::Fmod(CurrentTime, (double)MainLength) / MainLength : 0.0;
			Legs = TDMakePlayable(Loop);
			LegTime = Phase * Legs->GetPlayLength();
			LegAlpha = 0.f;
		}
	}
}

void UTDLoopBlendAnimInstance::SetUpperLoop(UAnimSequenceBase* Loop)
{
	bUpperWanted = Loop != nullptr;
	if (Loop && Loop != Upper)
	{
		Upper = TDMakePlayable(Loop);
		UpperTime = 0.0;
	}
}

void UTDLoopBlendAnimInstance::PlayOneShot(UAnimSequenceBase* InShot, float Rate, bool bUpperBodyOnly, bool bHold, float BlendIn)
{
	if (!InShot)
	{
		return;
	}
	// An additive sequence (hit reacts, rifle fire) played as a full pose collapses the body: only full poses here.
	if (const UAnimSequence* Seq = Cast<UAnimSequence>(InShot); Seq && Seq->IsValidAdditive())
	{
		return;
	}
	// A shot already showing (a combo hit chained on the last): keep its pose and fade it out under the new one
	// over ChainBlendSeconds, so the new swing does not snap to its own first frame.
	if (Shot && ShotAlpha > 0.f && TDPilotMeleePassOn())
	{
		PrevShot = Shot;
		PrevShotTime = ShotTime;
		PrevShotRate = ShotRate;
		PrevShotAlpha = ShotAlpha;
		bPrevShotUpper = bShotUpper;
		ShotAlpha = 0.f;
	}
	Shot = TDMakePlayable(InShot);
	ShotTime = 0.0;
	ShotRate = Rate;
	bShotUpper = bUpperBodyOnly;
	bShotHold = bHold;
	ShotBlendIn = BlendIn;
	ShotAlpha = FMath::Max(ShotAlpha, 0.01f);
}

void UTDLoopBlendAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (PrevShot)
	{
		// The outgoing swing keeps playing (its follow-through) under the new one, at its own weight, until the new
		// one is fully in (no dip toward the loop in between).
		PrevShotTime = FMath::Min(PrevShotTime + DeltaSeconds * PrevShotRate, (double)FMath::Max(0.f, PrevShot->GetPlayLength() - 0.001f));
		if (!Shot || ShotAlpha >= 1.f)
		{
			PrevShot = nullptr;
			PrevShotAlpha = 0.f;
		}
	}
	if (Shot)
	{
		const float Length = Shot->GetPlayLength();
		ShotTime += DeltaSeconds * ShotRate;
		const float Fade = PrevShot ? ChainBlendSeconds : (ShotBlendIn > 0.f ? ShotBlendIn : 0.1f);
		if (bShotHold)
		{
			ShotTime = FMath::Min(ShotTime, (double)FMath::Max(0.f, Length - 0.001f));
		}
		const bool bEnding = !bShotHold && (Length <= 0.f || ShotTime >= Length - Fade);
		ShotAlpha = bEnding ? ShotAlpha - DeltaSeconds / Fade : FMath::Min(1.f, ShotAlpha + DeltaSeconds / Fade);
		if (!bShotHold && (ShotAlpha <= 0.f || ShotTime >= Length))
		{
			Shot = nullptr;
			ShotAlpha = 0.f;
		}
	}
	auto Advance = [DeltaSeconds, this](const UAnimSequenceBase* Seq, double& Time)
	{
		const float Length = Seq ? Seq->GetPlayLength() : 0.f;
		if (Length > 0.f)
		{
			Time = FMath::Fmod(Time + DeltaSeconds * PlayRate, (double)Length);
		}
	};
	Advance(Current, CurrentTime);
	GaitLean = GaitDrop = 0.f;
	if (StartLeft > 0.f)
	{
		StartLeft = FMath::Max(0.f, StartLeft - DeltaSeconds);
		const float U = 1.f - StartLeft / FMath::Max(StartSeconds, 0.01f);
		const float Bump = FMath::Sin(PI * U);
		GaitLean = StartLeanDeg * Bump;
		GaitDrop = StartDropCm * Bump;
	}
	else if (StopLeft > 0.f)
	{
		StopLeft = FMath::Max(0.f, StopLeft - DeltaSeconds);
		const float U = 1.f - StopLeft / FMath::Max(StopSeconds, 0.01f);
		const float Bump = FMath::Sin(PI * U);
		GaitLean = -StopLeanDeg * Bump * (1.f - 0.5f * U);
		GaitDrop = StopDropCm * Bump;
	}
	if (Legs)
	{
		const float Length = Legs->GetPlayLength();
		LegTime = Length > 0.f ? FMath::Fmod(LegTime + DeltaSeconds * LegRate, (double)Length) : 0.0;
		LegAlpha = FMath::Clamp(LegAlpha + (bLegsWanted ? 1.f : -1.f) * (LegBlendSeconds > 0.f ? DeltaSeconds / LegBlendSeconds : 1.f), 0.f, 1.f);
		if (!bLegsWanted && LegAlpha <= 0.f)
		{
			Legs = nullptr;
		}
	}
	if (Upper)
	{
		const float Length = Upper->GetPlayLength();
		UpperTime = Length > 0.f ? FMath::Fmod(UpperTime + DeltaSeconds, (double)Length) : 0.0;
		UpperAlpha = FMath::Clamp(UpperAlpha + (bUpperWanted ? 1.f : -1.f) * DeltaSeconds / 0.12f, 0.f, 1.f);
		if (!bUpperWanted && UpperAlpha <= 0.f)
		{
			Upper = nullptr;
		}
	}
	Advance(Previous, PreviousTime);
	Alpha = BlendSeconds > 0.f ? FMath::Min(1.f, Alpha + DeltaSeconds / BlendSeconds) : 1.f;
	// The grip: the pilot with a blade in the hand (the hilt, or a presentation blade mesh that replaces it).
	const ATDFirstPersonCharacter* Pilot = Cast<ATDFirstPersonCharacter>(GetOwningActor());
	const UStaticMeshComponent* Hilt = Pilot ? Pilot->GetHandBladeHilt() : nullptr;
	const UStaticMeshComponent* Beam = Pilot ? Pilot->GetHandBladeBeam() : nullptr;
	const bool bHolding = Hilt && Hilt->GetAttachParent() == GetSkelMeshComponent() && (Hilt->GetStaticMesh() || (Beam && Beam->GetStaticMesh()));
	const bool bGripWanted = bGripEnabled && bHolding && !Pilot->IsDead();
	if (bGripWanted && !GripPose && !bGripPoseTried)
	{
		bGripPoseTried = true; // once: a missing pose leaves the hand as the clips have it
		GripPose = GripPoseAsset.IsNull() ? nullptr : TDLoadPlayableAnim(GripPoseAsset.ToSoftObjectPath());
	}
	GripAlpha = FMath::Clamp(GripAlpha + (bGripWanted && GripPose ? 1.f : -1.f) * (GripBlendSeconds > 0.f ? DeltaSeconds / GripBlendSeconds : 1.f), 0.f, 1.f);
	if (Alpha >= 1.f)
	{
		Previous = nullptr;
	}
}

FAnimInstanceProxy* UTDLoopBlendAnimInstance::CreateAnimInstanceProxy()
{
	return new TDLoopBlend::FProxy(this);
}

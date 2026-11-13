#include "TriadDrift/Ground/TDPilotAnim.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimNodeBase.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "Animation/AnimSequence.h"

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
			if (Upper && UpperAlpha > 0.f)
			{
				FPoseContext Guard(this);
				FAnimationPoseData GuardData(Guard);
				Upper->GetAnimationPose(GuardData, FAnimExtractContext(UpperTime, false, {}, true));
				BlendUpper(Output, Guard, UpperAlpha);
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

void UTDLoopBlendAnimInstance::SetUpperLoop(UAnimSequenceBase* Loop)
{
	bUpperWanted = Loop != nullptr;
	if (Loop && Loop != Upper)
	{
		Upper = TDMakePlayable(Loop);
		UpperTime = 0.0;
	}
}

void UTDLoopBlendAnimInstance::PlayOneShot(UAnimSequenceBase* InShot, float Rate, bool bUpperBodyOnly, bool bHold)
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
	Shot = TDMakePlayable(InShot);
	ShotTime = 0.0;
	ShotRate = Rate;
	bShotUpper = bUpperBodyOnly;
	bShotHold = bHold;
	ShotAlpha = FMath::Max(ShotAlpha, 0.01f);
}

void UTDLoopBlendAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (Shot)
	{
		const float Length = Shot->GetPlayLength();
		ShotTime += DeltaSeconds * ShotRate;
		const float Fade = 0.1f;
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
	if (Alpha >= 1.f)
	{
		Previous = nullptr;
	}
}

FAnimInstanceProxy* UTDLoopBlendAnimInstance::CreateAnimInstanceProxy()
{
	return new TDLoopBlend::FProxy(this);
}

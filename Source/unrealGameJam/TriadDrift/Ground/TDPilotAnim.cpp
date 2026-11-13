#include "TriadDrift/Ground/TDPilotAnim.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimNodeBase.h"
#include "AnimationRuntime.h"
#include "BonePose.h"

namespace TDLoopBlend
{
	/** Evaluates the current loop and, while a cross-fade runs, blends the previous one under it. */
	struct FProxy : public FAnimInstanceProxy
	{
		FProxy() = default;
		explicit FProxy(UAnimInstance* InInstance) : FAnimInstanceProxy(InInstance) {}

		const UAnimSequenceBase* Current = nullptr;
		const UAnimSequenceBase* Previous = nullptr;
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
		}

		virtual bool Evaluate(FPoseContext& Output) override
		{
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
	Previous = Current;
	PreviousTime = CurrentTime;
	// Keep the stride phase across a walk/run swap so the feet don't skip.
	const float OldLength = Previous ? Previous->GetPlayLength() : 0.f;
	const double Phase = OldLength > 0.f ? FMath::Fmod(PreviousTime, (double)OldLength) / OldLength : 0.0;
	Current = Loop;
	CurrentTime = Current ? Phase * Current->GetPlayLength() : 0.0;
	Alpha = Previous ? 0.f : 1.f;
}

void UTDLoopBlendAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	auto Advance = [DeltaSeconds, this](const UAnimSequenceBase* Seq, double& Time)
	{
		const float Length = Seq ? Seq->GetPlayLength() : 0.f;
		if (Length > 0.f)
		{
			Time = FMath::Fmod(Time + DeltaSeconds * PlayRate, (double)Length);
		}
	};
	Advance(Current, CurrentTime);
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

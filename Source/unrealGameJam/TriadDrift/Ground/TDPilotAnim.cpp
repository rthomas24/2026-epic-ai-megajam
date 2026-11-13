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
			if (Shot && ShotAlpha > 0.f)
			{
				FPoseContext Over(this);
				FAnimationPoseData OverData(Over);
				Shot->GetAnimationPose(OverData, FAnimExtractContext(ShotTime, false, {}, false));
				FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, OverData, 1.f - ShotAlpha);
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

void UTDLoopBlendAnimInstance::PlayOneShot(UAnimSequenceBase* InShot, float Rate)
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
		const bool bEnding = Length <= 0.f || ShotTime >= Length - Fade;
		ShotAlpha = bEnding ? ShotAlpha - DeltaSeconds / Fade : FMath::Min(1.f, ShotAlpha + DeltaSeconds / Fade);
		if (ShotAlpha <= 0.f || ShotTime >= Length)
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

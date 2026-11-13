// Triad Drift - a tiny C++ anim instance for skeletal looks that come with loops but no AnimBP (the pilot body,
// N-021): it plays one looping sequence in place and cross-fades to the next one asked for. Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TDPilotAnim.generated.h"

class UAnimSequenceBase;

UCLASS(Transient, NotBlueprintable)
class UNREALGAMEJAM_API UTDLoopBlendAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Play Loop (looping, in place); a different loop cross-fades in over BlendSeconds. Rate scales playback. */
	void SetLoop(UAnimSequenceBase* Loop, float Rate = 1.f);

	UAnimSequenceBase* GetLoop() const { return Current; }
	float GetBlendAlpha() const { return Alpha; }
	float GetRate() const { return PlayRate; }

	float BlendSeconds = 0.2f;

	// Read by the proxy before each update (game thread).
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Current;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Previous;
	double CurrentTime = 0.0;
	double PreviousTime = 0.0;
	float Alpha = 1.f;
	float PlayRate = 1.f;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};

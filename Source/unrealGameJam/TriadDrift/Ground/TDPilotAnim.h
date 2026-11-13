// Triad Drift - a tiny C++ anim instance for skeletal looks that come with loops but no AnimBP (the pilot body,
// N-021): it plays one looping sequence in place and cross-fades to the next one asked for. Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TDPilotAnim.generated.h"

class UAnimSequenceBase;

/**
 * Make an animation ready to evaluate. In an uncooked build the engine compresses animations asynchronously after
 * load; evaluated before that finishes, a sequence gives an empty pose (every bone at the root: the collapsed crew of
 * playtest 17e0437). What fixes it is keeping each sequence loaded for the session once used: a sequence collected
 * and loaded again evaluated empty in every later world (see .cpp).
 */
UNREALGAMEJAM_API UAnimSequenceBase* TDLoadPlayableAnim(const FSoftObjectPath& Path);
UNREALGAMEJAM_API UAnimSequenceBase* TDMakePlayable(UAnimSequenceBase* Anim);

UCLASS(Transient, NotBlueprintable)
class UNREALGAMEJAM_API UTDLoopBlendAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Play Loop (looping, in place); a different loop cross-fades in over BlendSeconds. Rate scales playback. */
	void SetLoop(UAnimSequenceBase* Loop, float Rate = 1.f);

	UAnimSequenceBase* GetLoop() const { return Current; }

	/** Play Shot once over the loop (attack, flinch): fades in, plays to its end, fades back to the loop. */
	void PlayOneShot(UAnimSequenceBase* Shot, float Rate = 1.f);
	UAnimSequenceBase* GetOneShot() const { return Shot; }
	float GetBlendAlpha() const { return Alpha; }
	float GetRate() const { return PlayRate; }

	float BlendSeconds = 0.2f;

	// Read by the proxy before each update (game thread).
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Current;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Previous;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Shot;
	double ShotTime = 0.0;
	float ShotRate = 1.f;
	float ShotAlpha = 0.f;
	double CurrentTime = 0.0;
	double PreviousTime = 0.0;
	float Alpha = 1.f;
	float PlayRate = 1.f;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};

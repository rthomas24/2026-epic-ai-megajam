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
/** TD.Pilot.MeleePass (TDFirstPersonCharacter.cpp): the Nov 14 pilot melee pass on (default) or off for A/B captures. */
UNREALGAMEJAM_API bool TDPilotMeleePassOn();

UCLASS(Transient, NotBlueprintable)
class UNREALGAMEJAM_API UTDLoopBlendAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Play Loop (looping, in place); a different loop cross-fades in over BlendSeconds. Rate scales playback. */
	void SetLoop(UAnimSequenceBase* Loop, float Rate = 1.f);

	UAnimSequenceBase* GetLoop() const { return Current; }

	/**
	 * Play Shot once over the loop (attack, flinch): fades in, plays to its end, fades back to the loop.
	 * bUpperBodyOnly: only UpperRootBone and its children take it (the legs keep walking). bHold: stay on its last
	 * frame until StopOneShot or another shot (a pose held on purpose, not a montage left playing).
	 */
	void PlayOneShot(UAnimSequenceBase* Shot, float Rate = 1.f, bool bUpperBodyOnly = false, bool bHold = false, float BlendIn = -1.f);
	void StopOneShot() { Shot = nullptr; ShotAlpha = 0.f; bShotHold = false; PrevShot = nullptr; PrevShotAlpha = 0.f; }
	UAnimSequenceBase* GetOneShot() const { return Shot; }
	bool IsOneShotUpperBody() const { return bShotUpper; }

	/**
	 * Loop on the legs only (every bone outside UpperRootBone's subtree: root, pelvis, legs) over the main loop: a
	 * strafe or backpedal under a body that faces the camera. Null fades it out. A different clip fades in afresh.
	 */
	void SetLegLoop(UAnimSequenceBase* Loop, float Rate = 1.f);
	UAnimSequenceBase* GetLegLoop() const { return bLegsWanted ? Legs.Get() : nullptr; }
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Legs;
	double LegTime = 0.0;
	float LegRate = 1.f;
	float LegAlpha = 0.f;
	bool bLegsWanted = false;
	float LegBlendSeconds = 0.15f;

	/**
	 * Starts and stops (no clips on disk): the owner says which gait it is in (0 idle, 1 walk, 2 run, -1 air); from
	 * idle into a move the body leans into the first step (StartSeconds), from a move into idle it plants, rocks back
	 * and settles (StopSeconds). Procedural on spine_01 (pitch) and the pelvis (drop), over whatever loop is playing.
	 */
	void SetGait(int32 Gait);
	int32 Gait = 0;
	float StartLeft = 0.f;
	float StopLeft = 0.f;
	float StartSeconds = 0.3f;
	float StopSeconds = 0.4f;
	/** Peak forward lean into a start / back-rock of a stop (deg) and the pelvis dip (cm). */
	float StartLeanDeg = 11.f;
	float StopLeanDeg = 9.f;
	float StartDropCm = 3.f;
	float StopDropCm = 6.f;
	/** Now (read by the proxy): degrees forward (+) / back (-) on spine_01, cm the pelvis is lowered. */
	float GaitLean = 0.f;
	float GaitDrop = 0.f;

	/**
	 * Foot IK on uneven ground (owner traces under each foot): the pelvis comes down by PelvisDown so the lower foot
	 * reaches its ground, and each foot is lifted by FootUp (component cm) with a two-bone solve on thigh / calf.
	 */
	void SetFootIK(float InFootUpL, float InFootUpR, float InPelvisDown) { FootUpL = InFootUpL; FootUpR = InFootUpR; PelvisDown = InPelvisDown; }
	float FootUpL = 0.f;
	float FootUpR = 0.f;
	float PelvisDown = 0.f;

	/**
	 * Per-character variation (#209, the squad on foot): so two followers beside the player are not clones. RateScale
	 * multiplies every loop and one-shot rate; PhaseOffset (0..1) shifts the stride / idle phase of every new loop;
	 * StanceYaw / StancePitch (deg) turn and lean the upper body a little in every pose. All neutral by default.
	 * Set automatically from the owner for a squad follower (class name contains "GroundAlly"), or by SetVariation.
	 */
	void SetVariation(float InRateScale, float InPhaseOffset, float InStanceYaw, float InStancePitch)
	{
		RateScale = InRateScale; PhaseOffset = InPhaseOffset; StanceYaw = InStanceYaw; StancePitch = InStancePitch; bVariationSet = true;
	}
	float RateScale = 1.f;
	float PhaseOffset = 0.f;
	float StanceYaw = 0.f;
	float StancePitch = 0.f;
	bool bVariationSet = false;
	bool bVariationTried = false;
	/** Run-to-fight: a move (walk, run, sprint) into the combat guard loop plays this on the upper body (followers by default). */
	bool bRunToFight = false;
	FName CombatReadyClip = TEXT("A_Pilot_Combat_Ready");

	/** Loop on the upper body only (the block guard) over whatever the legs do; null fades it back out (0.12 s). */
	void SetUpperLoop(UAnimSequenceBase* Loop);
	UAnimSequenceBase* GetUpperLoop() const { return bUpperWanted ? Upper.Get() : nullptr; }
	float GetUpperAlpha() const { return UpperAlpha; }
	/** The first bone of the upper-body layer (it and every child take the upper loop / an upper-body shot). */
	FName UpperRootBone = TEXT("spine_01");
	float GetBlendAlpha() const { return Alpha; }
	float GetRate() const { return PlayRate; }

	float BlendSeconds = 0.2f;

	// ---- right-hand grip (the pilot's blade hilt): one pose on the hand bone's children only (fingers and thumb),
	// laid over every loop, upper layer and one-shot, so every clip closes the hand round the hilt. On while the
	// owner is the pilot holding a blade (ATDFirstPersonCharacter with a hilt or blade mesh in the hand); off for crew.
	/** The closed-fist pose (a single-frame sequence on the body's skeleton). Always cooked: Gameplay/Characters. */
	UPROPERTY(EditAnywhere, Category = "Grip")
	TSoftObjectPtr<UAnimSequenceBase> GripPoseAsset = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_Grip_R.A_Pilot_Grip_R")));
	/** Its children (not the bone itself) take the grip pose. */
	UPROPERTY(EditAnywhere, Category = "Grip")
	FName GripHandBone = TEXT("hand_r");
	UPROPERTY(EditAnywhere, Category = "Grip", meta = (Units = "s"))
	float GripBlendSeconds = 0.1f;
	/** Master switch (TD.Ground.Grip, tests). */
	bool bGripEnabled = true;
	float GetGripAlpha() const { return GripAlpha; }
	UAnimSequenceBase* GetGripPose() const { return GripPose; }
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> GripPose;
	float GripAlpha = 0.f;
	bool bGripPoseTried = false;

	// ---- procedural pose (no animation data): built from the reference pose in C++. Used for crew meshes whose
	// sequences evaluate collapsed (Nov 13: SK_Mannequin compressed data on the Null crew meshes).
	/** Pose the body procedurally instead of playing loops. */
	bool bProcedural = false;
	/** Arms lowered out of the A-pose (degrees). */
	float ProcArmDown = 55.f;
	/** 0 standing .. 1 running: stride amplitude and pace. */
	float ProcSpeed01 = 0.f;
	/** Stride phase in radians (advanced by the owner from its ground speed). */
	float ProcPhase = 0.f;
	/** 0..1 right arm raised for an attack (wind-up) / swung through (release, negative). */
	float ProcAttack = 0.f;
	/** 0..1 spine flinch from a hit. */
	float ProcFlinch = 0.f;
	/** Sign flips per axis for skeletons whose local conventions differ (tuned by eye on SK_Mannequin). */
	float ProcSideSign = 1.f;
	float ProcForwardSign = 1.f;
	/** The body's facing in COMPONENT space (set by the owner from its actor forward); swing axes derive from it. */
	FVector ProcForwardCS = FVector(0.f, 1.f, 0.f);

	// Read by the proxy before each update (game thread).
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Current;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Previous;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Shot;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> Upper;
	double ShotTime = 0.0;
	float ShotRate = 1.f;
	float ShotAlpha = 0.f;
	/** A shot replaced while showing (a chained combo hit): held under the new one until that is fully in. */
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> PrevShot;
	double PrevShotTime = 0.0;
	float PrevShotRate = 1.f;
	float PrevShotAlpha = 0.f;
	bool bPrevShotUpper = false;
	/** Fade-in of a shot that replaces another one (the chained swing). */
	float ChainBlendSeconds = 0.08f;
	bool bShotUpper = false;
	bool bShotHold = false;
	/** This shot's fade-in from the loops (seconds; <= 0: the default 0.1 s). */
	float ShotBlendIn = -1.f;
	double UpperTime = 0.0;
	float UpperAlpha = 0.f;
	bool bUpperWanted = false;
	double CurrentTime = 0.0;
	double PreviousTime = 0.0;
	float Alpha = 1.f;
	float PlayRate = 1.f;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};

// Triad Drift - the pilot's right hand closes round the blade hilt (animation builder, Nov 13): the grip layer of
// UTDLoopBlendAnimInstance poses the fingers over every clip, and the hilt sits in the palm on BladeSocket.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

namespace TDPilotGripTest
{
	static const TCHAR* GripSlot = TEXT("TD_PilotGrip_UnitTest");

	/** The pilot with its real look in third person, possessed by a local player, on a floor. */
	struct FGripScene
	{
		FTDTestWorld W;
		ATDFirstPersonCharacter* P = nullptr;

		FGripScene()
		{
			UGameplayStatics::DeleteGameInSlot(GripSlot, 0);
			if (UTDSettingsSubsystem* S = UTDSettingsSubsystem::GetTDSettings(W.GetWorld()))
			{
				S->UseSlotForTests(GripSlot);
				S->SetOnFootCameraMode(ETDOnFootCamera::ThirdPerson);
			}
			TDGroundTest::SpawnFloor(W);
			P = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
			if (APlayerController* PC = P ? W.SpawnLocalPlayerController(P) : nullptr)
			{
				PC->SetControlRotation(FRotator::ZeroRotator);
			}
			W.Advance(0.5f);
		}
		~FGripScene() { UGameplayStatics::DeleteGameInSlot(GripSlot, 0); }

		USkeletalMeshComponent* Body() const { return P ? P->GetMesh() : nullptr; }
		UTDLoopBlendAnimInstance* Loops() const { return Body() ? Cast<UTDLoopBlendAnimInstance>(Body()->GetAnimInstance()) : nullptr; }
		FVector Bone(const TCHAR* Name) const { return Body()->GetBoneLocation(Name); }

		/** Local (parent-relative) rotation of a bone as posed this frame. */
		FQuat Local(const TCHAR* Name) const
		{
			const int32 I = Body()->GetBoneIndex(Name);
			const FName Parent = Body()->GetParentBone(Name);
			const FTransform Child = Body()->GetBoneTransform(I);
			const FTransform Par = Body()->GetBoneTransform(Body()->GetBoneIndex(Parent));
			return Child.GetRelativeTransform(Par).GetRotation();
		}

		/**
		 * The palm: the middle of the hand bone and the four knuckles (bone centres, inside the hand), carried 2 cm out
		 * to the palm's surface along the palm normal (across the knuckles x along the hand, toward the thumb's side).
		 */
		FVector Palm() const
		{
			const FVector H = Bone(TEXT("hand_r"));
			const FVector K = (Bone(TEXT("index_01_r")) + Bone(TEXT("middle_01_r")) + Bone(TEXT("ring_01_r")) + Bone(TEXT("pinky_01_r"))) / 4.f;
			const FVector Along = (K - H).GetSafeNormal();
			const FVector Across = (Bone(TEXT("index_01_r")) - Bone(TEXT("pinky_01_r"))).GetSafeNormal();
			FVector Normal = FVector::CrossProduct(Along, Across).GetSafeNormal();
			if (FVector::DotProduct(Normal, Bone(TEXT("thumb_02_r")) - H) < 0.f)
			{
				Normal = -Normal;
			}
			return (H + 4.f * K) / 5.f + Normal * PalmSkinCm;
		}
		static constexpr float PalmSkinCm = 2.f;

		/** Distance of a point from the hilt's axis (its +X through the pivot, the grip centre). */
		float FromHiltAxis(const FVector& At) const
		{
			const FTransform H = P->GetHandBladeHilt()->GetComponentTransform();
			const FVector Axis = H.GetUnitAxis(EAxis::X);
			const FVector D = At - H.GetLocation();
			return (float)(D - Axis * FVector::DotProduct(D, Axis)).Size();
		}
	};

	static const TCHAR* const MiddleJoints[] = { TEXT("index_02_r"), TEXT("middle_02_r"), TEXT("ring_02_r"), TEXT("pinky_02_r") };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPilotGripCloses, "TriadDrift.Ground.ThirdPerson.HandGripsTheHilt", TD_TEST_FLAGS)
bool FTDPilotGripCloses::RunTest(const FString& Parameters)
{
	using namespace TDPilotGripTest;
	FGripScene T;
	UTDLoopBlendAnimInstance* L = T.Loops();
	if (!TestTrue(TEXT("pilot with its look and the C++ loop player"), T.P && T.P->HasBody() && L != nullptr)) { return false; }
	TestNotNull(TEXT("the grip pose loaded (A_Pilot_Grip_R)"), L->GetGripPose());
	TestEqual(TEXT("grip fully on with the hilt in the hand"), L->GetGripAlpha(), 1.f, 0.001f);
	TestTrue(*FString::Printf(TEXT("the hilt rides the palm socket (%s)"), *T.P->GetHandBladeHilt()->GetAttachSocketName().ToString()),
		T.P->GetHandBladeHilt()->GetAttachSocketName() == FName(TEXT("BladeSocket")));

	// The clips' own hand (grip off) against the grip.
	L->bGripEnabled = false;
	T.W.Advance(0.3f);
	TestEqual(TEXT("grip off fades out"), L->GetGripAlpha(), 0.f, 0.001f);
	TArray<FQuat> Open;
	for (const TCHAR* J : MiddleJoints) { Open.Add(T.Local(J)); }
	L->bGripEnabled = true;
	T.W.Advance(0.15f); // 0.1 s blend
	TestEqual(TEXT("grip back within its 0.1 s blend"), L->GetGripAlpha(), 1.f, 0.001f);
	int32 Curled = 0;
	for (int32 i = 0; i < UE_ARRAY_COUNT(MiddleJoints); ++i)
	{
		const float Deg = FMath::RadiansToDegrees(T.Local(MiddleJoints[i]).AngularDistance(Open[i]));
		TestTrue(*FString::Printf(TEXT("%s is posed by the grip, not the clip (%.1f deg)"), MiddleJoints[i], Deg), Deg > 10.f);
		Curled += Deg > 10.f ? 1 : 0;
	}

	// The hilt is in the palm and every finger closes on it: the first phalanx turned toward the palm, the middle joint
	// against the hilt (the gauntlet's short, thick fingers press it into the palm; they cannot ring it), the thumb over it.
	const float Palm = T.FromHiltAxis(T.Palm());
	TestTrue(*FString::Printf(TEXT("hilt axis within 4 cm of the palm (%.2f cm)"), Palm), Palm < 4.f);
	const float PalmPivot = (float)FVector::Dist(T.P->GetHandBladeHilt()->GetComponentLocation(), T.Palm());
	TestTrue(*FString::Printf(TEXT("hilt grip centre within 4 cm of the palm (%.2f cm)"), PalmPivot), PalmPivot < 4.f);
	const FVector Along = ((T.Bone(TEXT("index_01_r")) + T.Bone(TEXT("pinky_01_r"))) * 0.5f - T.Bone(TEXT("hand_r"))).GetSafeNormal();
	for (int32 i = 0; i < UE_ARRAY_COUNT(MiddleJoints); ++i)
	{
		const FString F = FString(MiddleJoints[i]).Replace(TEXT("_02_r"), TEXT(""));
		const FVector P1 = (T.Bone(*(F + TEXT("_02_r"))) - T.Bone(*(F + TEXT("_01_r")))).GetSafeNormal();
		const float Curl = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp((float)FVector::DotProduct(P1, Along), -1.f, 1.f)));
		TestTrue(*FString::Printf(TEXT("%s curled toward the palm (%.0f deg off the hand's line)"), *F, Curl), Curl > 50.f);
		const float Mid = T.FromHiltAxis(T.Bone(MiddleJoints[i]));
		TestTrue(*FString::Printf(TEXT("%s on the hilt, not through it (%.2f cm from the axis)"), MiddleJoints[i], Mid), Mid > 1.f && Mid < 3.f);
	}
	const float Thumb = T.FromHiltAxis(T.Bone(TEXT("thumb_03_r")));
	TestTrue(*FString::Printf(TEXT("the thumb closes over the hilt (%.2f cm from the axis)"), Thumb), Thumb > 1.f && Thumb < 4.5f);
	// Blade out past the thumb, away from the body.
	const FVector Out = T.P->GetHandBladeHilt()->GetComponentTransform().GetUnitAxis(EAxis::X);
	const FVector ToThumb = (T.Bone(TEXT("thumb_02_r")) - T.Bone(TEXT("pinky_01_r"))).GetSafeNormal();
	TestTrue(*FString::Printf(TEXT("the blade leaves the fist on the thumb side (dot %.2f)"), FVector::DotProduct(Out, ToThumb)), FVector::DotProduct(Out, ToThumb) > 0.3f);
	return Curled >= 3;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPilotGripEveryClip, "TriadDrift.Ground.ThirdPerson.HandGripsThroughEverySwing", TD_TEST_FLAGS)
bool FTDPilotGripEveryClip::RunTest(const FString& Parameters)
{
	using namespace TDPilotGripTest;
	FGripScene T;
	UTDLoopBlendAnimInstance* L = T.Loops();
	if (!TestTrue(TEXT("pilot with its look and the C++ loop player"), T.P && T.P->HasBody() && L != nullptr && L->GetGripPose() != nullptr)) { return false; }
	// Each blade clip at mid-swing (full body and upper body): the grip sits on top of them all.
	for (const TCHAR* Clip : { TEXT("A_Pilot_Light1"), TEXT("A_Pilot_Light2"), TEXT("A_Pilot_Light3"), TEXT("A_Pilot_Heavy"), TEXT("A_Pilot_Block"), TEXT("A_Pilot_Dash") })
	{
		const FString Path = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/%s.%s"), Clip, Clip);
		UAnimSequenceBase* Seq = TDLoadPlayableAnim(FSoftObjectPath(Path));
		if (!TestNotNull(*FString::Printf(TEXT("%s exists"), Clip), Seq)) { continue; }
		for (const bool bUpper : { false, true })
		{
			L->PlayOneShot(Seq, 0.f, bUpper, true);
			L->ShotTime = Seq->GetPlayLength() * 0.5f;
			T.W.Advance(0.2f);
			TestEqual(*FString::Printf(TEXT("%s upper=%d: grip on"), Clip, bUpper), L->GetGripAlpha(), 1.f, 0.001f);
			for (const TCHAR* J : MiddleJoints)
			{
				const float D = T.FromHiltAxis(T.Bone(J));
				TestTrue(*FString::Printf(TEXT("%s upper=%d: %s on the hilt (%.2f cm)"), Clip, bUpper, J, D), D > 1.f && D < 3.f);
			}
			const float Palm = T.FromHiltAxis(T.Palm());
			TestTrue(*FString::Printf(TEXT("%s upper=%d: hilt in the palm (%.2f cm)"), Clip, bUpper, Palm), Palm < 4.f);
		}
		L->StopOneShot();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPilotGripOnlyThePilot, "TriadDrift.Ground.ThirdPerson.GripOnlyOnThePilot", TD_TEST_FLAGS)
bool FTDPilotGripOnlyThePilot::RunTest(const FString& Parameters)
{
	using namespace TDPilotGripTest;
	FGripScene T;
	UTDLoopBlendAnimInstance* L = T.Loops();
	if (!TestTrue(TEXT("pilot with its look"), L != nullptr)) { return false; }
	// No blade in the hand (hilt off the body): the hand is the clips' again.
	T.P->GetHandBladeHilt()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	T.W.Advance(0.2f);
	TestEqual(TEXT("no blade held: no grip"), L->GetGripAlpha(), 0.f, 0.001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

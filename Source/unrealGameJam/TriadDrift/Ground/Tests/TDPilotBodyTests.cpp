// Triad Drift - the pilot's body (N-021) on the first-person character, tested with a stand-in: the template
// mannequin mesh and its unarmed loops. Owner: Teammate 5.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequenceBase.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace TDPilotBodyTest
{
	static const TCHAR* Mesh = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	static const TCHAR* Idle = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle");
	static const TCHAR* Walk = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd");
	static const TCHAR* Run = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd");

	inline UTDPilotVisuals* StandIn(float Yaw = -90.f, float Scale = 1.f)
	{
		UTDPilotVisuals* V = NewObject<UTDPilotVisuals>(GetTransientPackage());
		V->SkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Mesh));
		V->Idle = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Idle));
		V->Walk = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Walk));
		V->Run = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Run));
		V->MeshYaw = Yaw;
		V->MeshScale = Scale;
		V->HeadBone = TEXT("head");
		return V;
	}

	inline ATDFirstPersonCharacter* SpawnPilot(FTDTestWorld& W, UTDPilotVisuals* Look)
	{
		const FTransform Xf(FRotator::ZeroRotator, FVector(0.f, 0.f, 95.f));
		ATDFirstPersonCharacter* P = W.GetWorld()->SpawnActorDeferred<ATDFirstPersonCharacter>(ATDFirstPersonCharacter::StaticClass(), Xf, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (P)
		{
			P->PilotVisuals = Look;
			P->FinishSpawning(Xf);
		}
		return P;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPilotBodyLoads, "TriadDrift.Ground.PilotBody.LoadsWhenPresentNothingWhenAbsent", TD_TEST_FLAGS)
bool FTDPilotBodyLoads::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("stand-in mesh"), LoadObject<USkeletalMesh>(nullptr, TDPilotBodyTest::Mesh))) { return false; }
	{
		// Absent (no look assigned and no DA_Pilot_Visuals on this branch): nothing changes.
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDPilotBodyTest::SpawnPilot(W, nullptr);
		if (ATDCrewCharacter::FindLookAsset(TEXT("DA_Pilot_Visuals"), UTDPilotVisuals::StaticClass()) == nullptr)
		{
			TestFalse(TEXT("no look: no body"), P->HasBody());
		}
		else
		{
			AddInfo(TEXT("DA_Pilot_Visuals exists on this branch: the absent case is covered by the null look below"));
		}
		P->PilotVisuals = NewObject<UTDPilotVisuals>(GetTransientPackage()); // a look with no mesh
		P->ApplyPilotVisuals();
		TestFalse(TEXT("a look without a mesh: no body"), P->HasBody());
	}
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDPilotBodyTest::SpawnPilot(W, TDPilotBodyTest::StandIn(-90.f, 1.1f));
		W.Tick(1.f / 60.f);
		TestTrue(TEXT("present: the body is shown"), P->HasBody());
		USkeletalMeshComponent* Body = P->GetMesh();
		TestEqual(TEXT("mesh yaw applied"), (float)Body->GetRelativeRotation().Yaw, -90.f, 0.01f);
		TestEqual(TEXT("mesh scale applied"), (float)Body->GetRelativeScale3D().X, 1.1f, 0.001f);
		TestTrue(TEXT("feet at the bottom of the capsule"), FMath::IsNearlyEqual((float)Body->GetRelativeLocation().Z, -P->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 0.5f));
		TestTrue(TEXT("casts shadows (the player sees their own shadow)"), Body->CastShadow && Body->bCastHiddenShadow);
		TestTrue(TEXT("no AnimBP: the C++ loop player drives it"), Cast<UTDLoopBlendAnimInstance>(Body->GetAnimInstance()) != nullptr);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPilotBodyHead, "TriadDrift.Ground.PilotBody.HeadHiddenInFirstPersonOnly", TD_TEST_FLAGS)
bool FTDPilotBodyHead::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDPilotBodyTest::SpawnPilot(W, TDPilotBodyTest::StandIn());
	USkeletalMeshComponent* Body = P->GetMesh();
	if (!TestTrue(TEXT("body"), P->HasBody())) { return false; }
	P->SetBodyFirstPersonView(true);
	TestTrue(TEXT("first person: the head is hidden"), Body->IsBoneHiddenByName(TEXT("head")));
	TestTrue(TEXT("first person: the body sits behind the camera (no chest clipping)"), Body->GetRelativeLocation().X < -1.f);
	const float StandZ = (float)Body->GetRelativeLocation().Z;
	P->SetBodyFirstPersonView(false);
	TestFalse(TEXT("other views (director, death, boarding): the full body"), Body->IsBoneHiddenByName(TEXT("head")));
	TestEqual(TEXT("other views: no push-back"), (float)Body->GetRelativeLocation().X, 0.f, 0.01f);
	TestEqual(TEXT("same height either way"), (float)Body->GetRelativeLocation().Z, StandZ, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPilotBodyLoops, "TriadDrift.Ground.PilotBody.LoopChosenBySpeedAndCrossFades", TD_TEST_FLAGS)
bool FTDPilotBodyLoops::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("standing: idle"), ATDFirstPersonCharacter::ChooseBodyLoop(0.f), 0);
	TestEqual(TEXT("slow: walk"), ATDFirstPersonCharacter::ChooseBodyLoop(300.f), 1);
	TestEqual(TEXT("run speed (650): run"), ATDFirstPersonCharacter::ChooseBodyLoop(650.f), 2);
	TestEqual(TEXT("sprint (950): run"), ATDFirstPersonCharacter::ChooseBodyLoop(950.f), 2);

	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDPilotBodyTest::SpawnPilot(W, TDPilotBodyTest::StandIn());
	UTDLoopBlendAnimInstance* Loops = Cast<UTDLoopBlendAnimInstance>(P->GetMesh()->GetAnimInstance());
	if (!TestNotNull(TEXT("loop player"), Loops)) { return false; }
	for (int32 i = 0; i < 10; ++i) { W.Tick(1.f / 60.f); }
	TestEqual(TEXT("standing plays Idle"), GetNameSafe(Loops->GetLoop()), FString(TEXT("MM_Idle")));
	// Run: real movement through the character movement component.
	P->GetCharacterMovement()->Velocity = FVector(650.f, 0.f, 0.f);
	for (int32 i = 0; i < 2; ++i) { P->AddMovementInput(FVector::ForwardVector, 1.f); W.Tick(1.f / 60.f); }
	TestEqual(TEXT("running plays Run"), GetNameSafe(Loops->GetLoop()), FString(TEXT("MF_Unarmed_Jog_Fwd")));
	TestTrue(TEXT("and cross-fades in (no pop)"), Loops->GetBlendAlpha() < 1.f);
	for (int32 i = 0; i < 30; ++i) { P->AddMovementInput(FVector::ForwardVector, 1.f); W.Tick(1.f / 60.f); }
	TestEqual(TEXT("the fade completes"), Loops->GetBlendAlpha(), 1.f, 0.001f);
	return true;
}

// The real pilot look (DA_Pilot_Visuals) stands upright: head above the pelvis and above 140 cm x scale, standing
// and running, sampled across a second (the 17e0437 lesson: assert the pose, not the mode).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPilotBodyUpright, "TriadDrift.Ground.PilotBody.RealLookStandsUpright", TD_TEST_FLAGS)
bool FTDPilotBodyUpright::RunTest(const FString& Parameters)
{
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		// Made per pass: a test world's teardown collects transient objects made before it.
		UTDPilotVisuals* Look = Pass == 0 ? Cast<UTDPilotVisuals>(ATDCrewCharacter::FindLookAsset(TEXT("DA_Pilot_Visuals"), UTDPilotVisuals::StaticClass()))
			: TDPilotBodyTest::StandIn();
		if (!Look)
		{
			AddInfo(TEXT("no DA_Pilot_Visuals on this branch: the stand-in only"));
			continue;
		}
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* P = TDPilotBodyTest::SpawnPilot(W, Look);
		AddInfo(FString::Printf(TEXT("look %s: mesh %s, scale %.2f, head bone %s"), *GetNameSafe(Look), *Look->SkeletalMesh.ToString(), Look->MeshScale, *Look->HeadBone.ToString()));
		if (!TestTrue(TEXT("body"), P->HasBody())) { return false; }
		USkeletalMeshComponent* Body = P->GetMesh();
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones; // nothing renders in tests
		const float MinHead = 140.f * Look->MeshScale;
		auto Check = [&](const TCHAR* What)
		{
			const float Feet = (float)(P->GetActorLocation().Z - P->GetSimpleCollisionHalfHeight());
			const float Head = (float)Body->GetBoneLocation(Look->HeadBone).Z - Feet;
			const float Pelvis = (float)Body->GetBoneLocation(TEXT("pelvis")).Z - Feet;
			TestTrue(*FString::Printf(TEXT("%s %s stands: head %.0f cm (min %.0f), pelvis %.0f"), *GetNameSafe(Body->GetSkeletalMeshAsset()), What, Head, MinHead, Pelvis),
				Head > Pelvis + 30.f && Head > MinHead);
		};
		for (int32 i = 0; i < 60; ++i) { W.Tick(1.f / 60.f); if (i % 20 == 19) { Check(TEXT("standing")); } }
		for (int32 i = 0; i < 60; ++i) { P->AddMovementInput(FVector::ForwardVector, 1.f); W.Tick(1.f / 60.f); if (i % 20 == 19) { Check(TEXT("running")); } }
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// Triad Drift - ENG-32 on the pilot: T3's third-person rig wired into ATDFirstPersonCharacter. The saved choice
// switches it (start-up and live), the whole body shows with the blade in its right hand, the facing rule turns the
// body, melee / interaction aim through the crosshair, and every move reads on the body. Owner: Teammate 5.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Camera/TDThirdPersonCameraComponent.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

namespace TDThirdPersonPilotTest
{
	static const TCHAR* Slot = TEXT("TD_ThirdPersonPilot_UnitTest");

	/** A pilot with its real look, possessed by a local player, on a floor; settings on a private slot. */
	struct FScene
	{
		FTDTestWorld W;
		UTDSettingsSubsystem* S = nullptr;
		ATDFirstPersonCharacter* P = nullptr;
		APlayerController* PC = nullptr;

		explicit FScene(bool bThirdPerson)
		{
			UGameplayStatics::DeleteGameInSlot(Slot, 0);
			S = UTDSettingsSubsystem::GetTDSettings(W.GetWorld());
			if (S)
			{
				S->UseSlotForTests(Slot);
				S->SetOnFootCameraMode(bThirdPerson ? ETDOnFootCamera::ThirdPerson : ETDOnFootCamera::FirstPerson);
			}
			TDGroundTest::SpawnFloor(W);
			P = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
			PC = P ? W.SpawnLocalPlayerController(P) : nullptr;
			if (PC)
			{
				PC->SetControlRotation(FRotator::ZeroRotator);
			}
			W.Advance(0.5f);
		}
		~FScene() { UGameplayStatics::DeleteGameInSlot(Slot, 0); }

		bool Ok() const { return S && P && PC && P->GetThirdPersonRig() && P->HasBody(); }
		float Yaw() const { return (float)P->GetActorRotation().Yaw; }
		UTDLoopBlendAnimInstance* Loops() const { return Cast<UTDLoopBlendAnimInstance>(P->GetMesh()->GetAnimInstance()); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTPPilotSwitch, "TriadDrift.Ground.ThirdPerson.PilotFollowsTheSavedChoice", TD_TEST_FLAGS)
bool FTDTPPilotSwitch::RunTest(const FString& Parameters)
{
	TDThirdPersonPilotTest::FScene T(true);
	if (!TestTrue(TEXT("pilot with its look, a local player and the settings"), T.Ok())) { return false; }
	ATDFirstPersonCharacter* P = T.P;
	USkeletalMeshComponent* Body = P->GetMesh();

	// Saved as third person: the pilot starts in it (start-up and every level travel make a new pilot).
	TestTrue(TEXT("starts in third person"), P->IsThirdPersonView());
	TestTrue(TEXT("the chase camera is the view camera"), P->GetViewCamera() == P->GetThirdPersonRig()->GetThirdPersonCamera());
	TestFalse(TEXT("the body turns by the facing rule, not the mouse"), P->bUseControllerRotationYaw);
	TestFalse(TEXT("the whole pilot: head shown"), Body->IsBoneHiddenByName(TEXT("head")));
	TestFalse(TEXT("the camera-rig blade is hidden"), P->GetBladeMesh()->IsVisible());
	TestTrue(TEXT("the blade is in the hand"), P->GetHandBladeBeam()->IsVisible() && P->GetHandBladeHilt()->IsVisible());
	const FName Socket = P->GetHandBladeHilt()->GetAttachSocketName();
	TestTrue(*FString::Printf(TEXT("held in the right hand (%s)"), *Socket.ToString()), Socket == TEXT("hand_r") || Socket == TEXT("BladeSocket"));
	TestTrue(TEXT("the hand is where the hilt is"), P->GetHandBladeHilt()->GetComponentLocation().Equals(Body->GetSocketLocation(Socket), 25.f));
	TestEqual(TEXT("the engineering hilt"), GetNameSafe(P->GetHandBladeHilt()->GetStaticMesh()), FString(TEXT("SM_TD_BladeHilt")));
	TestEqual(TEXT("the engineering beam"), GetNameSafe(P->GetHandBladeBeam()->GetStaticMesh()), FString(TEXT("SM_TD_BladeBeam")));
	const float Beam = (float)P->GetHandBladeBeam()->Bounds.GetBox().GetSize().Size();
	TestTrue(*FString::Printf(TEXT("a full-length beam (%.0f cm)"), Beam), Beam > 70.f && Beam < 130.f);

	// V / View / the Settings page all go through the subsystem: switching back is live.
	T.S->ToggleOnFootCameraMode();
	T.W.Advance(0.4f);
	TestFalse(TEXT("first person again"), P->IsThirdPersonView());
	TestTrue(TEXT("the mouse turns the body again"), P->bUseControllerRotationYaw);
	TestTrue(TEXT("head hidden in first person"), Body->IsBoneHiddenByName(TEXT("head")));
	TestTrue(TEXT("the camera-rig blade is back"), P->GetBladeMesh()->IsVisible());
	TestFalse(TEXT("no second blade in the hand"), P->GetHandBladeBeam()->IsVisible() || P->GetHandBladeHilt()->IsVisible());
	TestEqual(TEXT("saved"), T.S->GetOnFootCameraMode(), ETDOnFootCamera::FirstPerson);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTPPilotFacing, "TriadDrift.Ground.ThirdPerson.PilotFacingRule", TD_TEST_FLAGS)
bool FTDTPPilotFacing::RunTest(const FString& Parameters)
{
	TDThirdPersonPilotTest::FScene T(true);
	if (!TestTrue(TEXT("scene"), T.Ok())) { return false; }
	ATDFirstPersonCharacter* P = T.P;

	// Just running (strafing right with the camera ahead): the body turns to the way it moves.
	P->SetMoveInput(FVector2D(1.f, 0.f));
	T.W.Advance(0.8f);
	TestTrue(*FString::Printf(TEXT("faces the movement (%.0f)"), T.Yaw()), FMath::Abs(FMath::FindDeltaAngleDegrees(T.Yaw(), 90.f)) < 15.f);
	TestTrue(TEXT("the camera did not turn with it"), FMath::Abs(FMath::FindDeltaAngleDegrees((float)T.PC->GetControlRotation().Yaw, 0.f)) < 1.f);

	// Blocking while moving: face the camera (strafe), so the guard is where the crosshair is.
	P->GetBlade()->SetBlockHeld(true);
	T.W.Advance(0.3f);
	TestTrue(*FString::Printf(TEXT("blocking faces the camera (%.0f)"), T.Yaw()), FMath::Abs(FMath::FindDeltaAngleDegrees(T.Yaw(), 0.f)) < 10.f);
	P->GetBlade()->SetBlockHeld(false);
	P->SetMoveInput(FVector2D::ZeroVector);
	T.W.Advance(0.6f);

	// Standing still with the camera turned more than 60 degrees: the body follows it.
	const float Before = T.Yaw();
	T.PC->SetControlRotation(FRotator(0.f, Before + 100.f, 0.f));
	T.W.Advance(0.6f);
	TestTrue(*FString::Printf(TEXT("turns to a camera > 60 deg off (%.0f -> %.0f)"), Before, T.Yaw()), FMath::Abs(FMath::FindDeltaAngleDegrees(T.Yaw(), Before + 100.f)) < 10.f);
	// ... but a small look around leaves it alone.
	const float Settled = T.Yaw();
	T.PC->SetControlRotation(FRotator(0.f, Settled + 30.f, 0.f));
	T.W.Advance(0.5f);
	TestTrue(TEXT("a 30 deg look does not turn the body"), FMath::Abs(FMath::FindDeltaAngleDegrees(T.Yaw(), Settled)) < 2.f);

	// An attack swings where the crosshair points.
	P->GetBlade()->LightAttack();
	T.W.Advance(0.15f);
	TestTrue(*FString::Printf(TEXT("attacking faces the camera (%.0f)"), T.Yaw()), FMath::Abs(FMath::FindDeltaAngleDegrees(T.Yaw(), Settled + 30.f)) < 10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTPPilotAim, "TriadDrift.Ground.ThirdPerson.MeleeAndAimThroughTheCrosshair", TD_TEST_FLAGS)
bool FTDTPPilotAim::RunTest(const FString& Parameters)
{
	TDThirdPersonPilotTest::FScene T(true);
	if (!TestTrue(TEXT("scene"), T.Ok())) { return false; }
	ATDFirstPersonCharacter* P = T.P;
	const UCameraComponent* Chase = P->GetThirdPersonRig()->GetThirdPersonCamera();

	// Looking a little down: the crosshair (the chase lens's centre ray) lands on the floor ahead. The lens is over the
	// right shoulder, so the eye's own forward would miss that point; the eye must aim at exactly it.
	T.PC->SetControlRotation(FRotator(-12.f, 0.f, 0.f));
	T.W.Advance(0.6f); // the rig's rotation lag settles
	const FVector Lens = Chase->GetComponentLocation();
	const FVector Ray = Chase->GetForwardVector();
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDTest), false, P);
	const bool bOnFloor = T.W.GetWorld()->LineTraceSingleByChannel(Hit, Lens, Lens + Ray * 5000.f, ECC_Visibility, Params);
	if (!TestTrue(TEXT("the crosshair is on the floor ahead"), bOnFloor)) { return false; }
	FVector Eye;
	FRotator Aim;
	P->GetActorEyesViewPoint(Eye, Aim);
	const FVector ToPoint = (Hit.ImpactPoint - Eye).GetSafeNormal();
	const float Off = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp((float)FVector::DotProduct(Aim.Vector(), ToPoint), -1.f, 1.f)));
	TestTrue(*FString::Printf(TEXT("the eye aims at the crosshair point (%.2f deg off)"), Off), Off < 1.f);
	const float Naive = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp((float)FVector::DotProduct(T.PC->GetControlRotation().Vector(), ToPoint), -1.f, 1.f)));
	TestTrue(*FString::Printf(TEXT("(the bare control rotation would be %.1f deg off)"), Naive), Naive > Off);

	// A crewman right ahead: the blade lands on it, exactly as in first person.
	T.PC->SetControlRotation(FRotator(-5.f, 0.f, 0.f));
	ATDCrewDummy* D = TDGroundTest::SpawnDummy(T.W, P->GetActorLocation() + P->GetActorForwardVector() * 140.f);
	T.W.Advance(0.4f);
	const float HealthBefore = D->GetHealth()->GetHull();
	P->GetBlade()->LightAttack();
	T.W.Advance(0.4f);
	TestTrue(*FString::Printf(TEXT("the swing hits it (%.0f -> %.0f)"), HealthBefore, D->GetHealth()->GetHull()), D->GetHealth()->GetHull() < HealthBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTPPilotMoves, "TriadDrift.Ground.ThirdPerson.EveryMoveReadsOnTheBody", TD_TEST_FLAGS)
bool FTDTPPilotMoves::RunTest(const FString& Parameters)
{
	TDThirdPersonPilotTest::FScene T(true);
	if (!TestTrue(TEXT("scene"), T.Ok())) { return false; }
	ATDFirstPersonCharacter* P = T.P;
	UTDLoopBlendAnimInstance* L = T.Loops();
	if (!TestNotNull(TEXT("the pilot's loop player"), L)) { return false; }

	P->GetBlade()->LightAttack();
	T.W.Tick(1.f / 60.f);
	TestEqual(TEXT("light swing"), P->GetBodyActionName(), FName(TEXT("A_Pilot_Light1")));
	TestEqual(TEXT("playing"), GetNameSafe(L->GetOneShot()), FString(TEXT("A_Pilot_Light1")));
	TestFalse(TEXT("standing: the whole body commits"), L->IsOneShotUpperBody());
	T.W.Advance(1.2f);

	P->GetBlade()->SetBlockHeld(true);
	T.W.Advance(0.3f);
	TestEqual(TEXT("the guard is held on the upper body"), GetNameSafe(L->GetUpperLoop()), FString(TEXT("A_Pilot_Block")));
	TestTrue(TEXT("faded in"), L->GetUpperAlpha() > 0.9f);
	P->GetBlade()->SetBlockHeld(false);
	T.W.Advance(0.4f);
	TestNull(TEXT("and let go"), L->GetUpperLoop());

	TestTrue(TEXT("dash"), P->TryDash());
	T.W.Tick(1.f / 60.f);
	TestEqual(TEXT("dash reads"), P->GetBodyActionName(), FName(TEXT("A_Pilot_Dash")));
	T.W.Advance(1.f);

	// Running and swinging: the legs keep running, the swing plays from the spine up.
	P->SetMoveInput(FVector2D(0.f, 1.f));
	T.W.Advance(0.6f);
	P->GetBlade()->LightAttack();
	T.W.Tick(1.f / 60.f);
	TestTrue(TEXT("a swing on the run is upper body only"), L->IsOneShotUpperBody());
	P->SetMoveInput(FVector2D::ZeroVector);
	T.W.Advance(1.2f);

	// A hit from behind flinches forward.
	ATDCrewDummy* D = TDGroundTest::SpawnDummy(T.W, P->GetActorLocation() - P->GetActorForwardVector() * 200.f);
	UTDStatics::ApplyTDDamage(P, 5.f, ETDDamageKind::Kinetic, D, D);
	T.W.Tick(1.f / 60.f);
	TestEqual(TEXT("hit react from behind"), P->GetBodyActionName(), FName(TEXT("A_Pilot_HitReact_Back")));

	// The pose never collapses while all that plays: the head stays well above the feet.
	const float Feet = (float)(P->GetActorLocation().Z - P->GetSimpleCollisionHalfHeight());
	const float Head = (float)P->GetMesh()->GetBoneLocation(TEXT("head")).Z - Feet;
	TestTrue(*FString::Printf(TEXT("standing (head %.0f cm)"), Head), Head > 120.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTPPilotBladeRetract, "TriadDrift.Ground.ThirdPerson.HandBladeRetractsOnDeathAndReturns", TD_TEST_FLAGS)
bool FTDTPPilotBladeRetract::RunTest(const FString& Parameters)
{
	TDThirdPersonPilotTest::FScene T(true);
	if (!TestTrue(TEXT("scene"), T.Ok())) { return false; }
	ATDFirstPersonCharacter* P = T.P;
	TestTrue(TEXT("out"), P->GetHandBladeExtend01() > 0.99f);
	const float Long = (float)P->GetHandBladeBeam()->GetComponentScale().X;
	UTDStatics::ApplyTDDamage(P, 1000.f, ETDDamageKind::Energy, nullptr, nullptr);
	T.W.Advance(0.08f);
	TestTrue(TEXT("retracting along the blade"), P->GetHandBladeBeam()->GetComponentScale().X < Long * 0.8f);
	T.W.Advance(0.4f);
	TestTrue(TEXT("put away"), P->GetHandBladeExtend01() < 0.01f && !P->GetHandBladeBeam()->IsVisible());
	P->RespawnAt(FTransform(FVector(0.f, 0.f, 95.f)));
	T.W.Advance(0.4f);
	TestTrue(TEXT("back out on the next take"), P->GetHandBladeExtend01() > 0.99f && P->GetHandBladeBeam()->IsVisible());
	TestTrue(TEXT("back in the hand after the ragdoll"), P->GetHandBladeHilt()->GetAttachParent() == P->GetMesh());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// #214: movement-scaled first-person weapon motion, isolated from combat and camera state.
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

namespace TDWeaponBobTest
{
	ATDFirstPersonCharacter* SpawnPilot(FTDTestWorld& W)
	{
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* Pilot = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
		if (!Pilot || !W.SpawnLocalPlayerController(Pilot, ATDSpacePlayerController::StaticClass())) { return nullptr; }
		Pilot->SetThirdPerson(false);
		W.Advance(.3f);
		return Pilot;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWeaponBobLocomotion, "TriadDrift.Ground.WeaponBob.WalkRunAndStopAtSeveralFrameRates", TD_TEST_FLAGS)
bool FTDWeaponBobLocomotion::RunTest(const FString& Parameters)
{
	for (const float Dt : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		ATDFirstPersonCharacter* Pilot = TDWeaponBobTest::SpawnPilot(W);
		if (!TestNotNull(TEXT("pilot"), Pilot)) { return false; }
		const FVector Rest = Pilot->GetBladeGrip()->GetRelativeLocation();
		TestTrue(TEXT("idle has no persistent weapon bob"), Pilot->GetWeaponBobOffset().IsNearlyZero(.001f));
		auto SampleMotion = [&](float& OutRange, int32& OutCrossings)
		{
			float MinZ = TNumericLimits<float>::Max(), MaxZ = TNumericLimits<float>::Lowest();
			float PreviousY = Pilot->GetBladeGrip()->GetRelativeLocation().Y - Rest.Y;
			OutCrossings = 0;
			for (int32 Frame = 0; Frame < FMath::RoundToInt(2.f / Dt); ++Frame)
			{
				W.Tick(Dt);
				const FVector At = Pilot->GetBladeGrip()->GetRelativeLocation();
				MinZ = FMath::Min(MinZ, (float)At.Z);
				MaxZ = FMath::Max(MaxZ, (float)At.Z);
				const float Y = At.Y - Rest.Y;
				if ((PreviousY < 0.f && Y >= 0.f) || (PreviousY >= 0.f && Y < 0.f)) { ++OutCrossings; }
				PreviousY = Y;
			}
			OutRange = MaxZ - MinZ;
		};
		Pilot->SetMoveInput(FVector2D(0.f, 1.f));
		W.Advance(1.f, Dt);
		TestTrue(TEXT("fixture actually moves at ordinary speed"), Pilot->GetVelocity().Size2D() > Pilot->Tuning.RunSpeed * .9f);
		float WalkRange = 0.f, RunRange = 0.f;
		int32 WalkCrossings = 0, RunCrossings = 0;
		SampleMotion(WalkRange, WalkCrossings);
		Pilot->SetSprintHeld(true);
		W.Advance(1.f, Dt);
		TestTrue(TEXT("fixture actually runs faster"), Pilot->IsSprinting() && Pilot->GetVelocity().Size2D() > Pilot->Tuning.RunSpeed);
		SampleMotion(RunRange, RunCrossings);
		AddInfo(FString::Printf(TEXT("%.0f fps: walk/run visible Z ranges %.3f/%.3f cm, lateral crossings %d/%d"), 1.f / Dt,
			WalkRange, RunRange, WalkCrossings, RunCrossings));
		TestTrue(TEXT("walking visibly moves the real weapon grip"), WalkRange > .1f && WalkCrossings >= 4);
		TestTrue(TEXT("running has a larger visible bob"), RunRange > WalkRange * 1.1f);
		TestTrue(TEXT("running has a quicker cadence"), RunCrossings > WalkCrossings);
		const FVector BeforeStop = Pilot->GetBladeGrip()->GetRelativeLocation();
		Pilot->SetMoveInput(FVector2D::ZeroVector);
		Pilot->SetSprintHeld(false);
		W.Tick(Dt);
		TestTrue(TEXT("stop does not snap the grip"), FVector::Dist(BeforeStop, Pilot->GetBladeGrip()->GetRelativeLocation()) < 2.f);
		W.Advance(2.f, Dt);
		TestTrue(TEXT("actual movement stopped"), Pilot->GetVelocity().Size2D() < 1.f);
		TestTrue(TEXT("bob fades out at rest"), Pilot->GetWeaponBobOffset().IsNearlyZero(.01f));
		TestTrue(TEXT("weapon settles back to its authored idle pose"), Pilot->GetBladeGrip()->GetRelativeLocation().Equals(Rest, .05f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWeaponBobCombat, "TriadDrift.Ground.WeaponBob.CombatAndCameraRemainIndependent", TD_TEST_FLAGS)
bool FTDWeaponBobCombat::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Pilot = TDWeaponBobTest::SpawnPilot(W);
	if (!TestNotNull(TEXT("pilot"), Pilot)) { return false; }
	UTDEnergyBladeComponent* Blade = Pilot->GetBlade();
	const FTransform CameraBefore = Pilot->GetCamera()->GetRelativeTransform();
	const float HitDelay = Blade->HitDelay, HeavyDamage = Blade->HeavyDamage, HeavyRange = Blade->HeavyLungeDistance;
	Pilot->SetMoveInput(FVector2D(0.f, 1.f));
	W.Advance(1.f);
	TestTrue(TEXT("locomotion bob is active before combat"), !Pilot->GetWeaponBobOffset().IsNearlyZero(.01f));
	Blade->SetBlockHeld(true);
	W.Tick();
	TestTrue(TEXT("real block pose is active"), Blade->IsBlocking());
	TestTrue(TEXT("block alignment has no extra locomotion offset"), Pilot->GetWeaponBobOffset().IsNearlyZero());
	Blade->SetBlockHeld(false);
	TestTrue(TEXT("real light attack starts"), Blade->LightAttack());
	W.Tick();
	TestTrue(TEXT("attack alignment has no extra locomotion offset"), Blade->IsAttacking() && Pilot->GetWeaponBobOffset().IsNearlyZero());
	TestTrue(TEXT("weapon motion never moves the camera component"), Pilot->GetCamera()->GetRelativeTransform().Equals(CameraBefore));
	TestEqual(TEXT("hit timing unchanged"), Blade->HitDelay, HitDelay);
	TestEqual(TEXT("damage unchanged"), Blade->HeavyDamage, HeavyDamage);
	TestEqual(TEXT("gameplay lunge range unchanged"), Blade->HeavyLungeDistance, HeavyRange);
	Pilot->bWeaponBobEnabled = false;
	W.Advance(1.f);
	TestTrue(TEXT("setting can disable cosmetic bob"), Pilot->GetWeaponBobOffset().IsNearlyZero());
	Pilot->bWeaponBobEnabled = true;
	Pilot->SetThirdPerson(true);
	W.Tick();
	TestTrue(TEXT("third-person pose is unaffected"), Pilot->GetWeaponBobOffset().IsNearlyZero());
	return true;
}
#endif

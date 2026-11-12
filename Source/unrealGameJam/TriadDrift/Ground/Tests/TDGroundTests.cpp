// Triad Drift - Ground automation tests (ENG-21, ENG-22, ENG-24). Owner: Teammate 5.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Ground

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundGameMode.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "AIController.h"
#include "TriadDrift/Flow/Tests/TDFlowTestListener.h"
#include "TriadDrift/Core/TDEventBus.h"

namespace TDGroundTest
{
	/** A floor (top at Z = 0) plus an optional box, in a fresh test world. */
	static AActor* SpawnBox(FTDTestWorld& W, const FVector& Center, const FVector& SizeCm)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		AStaticMeshActor* Box = W.Spawn<AStaticMeshActor>(Center);
		if (Box && Cube)
		{
			UStaticMeshComponent* SMC = Box->GetStaticMeshComponent();
			SMC->SetMobility(EComponentMobility::Movable);
			SMC->SetStaticMesh(Cube);
			SMC->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Box->SetActorScale3D(SizeCm / 100.f);
		}
		return Box;
	}

	static void SpawnFloor(FTDTestWorld& W)
	{
		TDGroundTest::SpawnBox(W, FVector(0.f, 0.f, -50.f), FVector(20000.f, 20000.f, 100.f));
	}

	/** Player character standing on the floor at X, possessed by a player controller, settled. */
	static ATDFirstPersonCharacter* SpawnPlayer(FTDTestWorld& W, const FVector& At = FVector(0.f, 0.f, 95.f))
	{
		ATDFirstPersonCharacter* C = W.Spawn<ATDFirstPersonCharacter>(At);
		if (C)
		{
			// The harness's PlayerController has no ULocalPlayer, so it isn't "locally controlled" and
			// CharacterMovement wouldn't simulate. An AIController drives the same public move API.
			AAIController* AI = W.Spawn<AAIController>(At);
			AI->Possess(C);
			if (AController* PC = C->GetController())
			{
				PC->SetControlRotation(FRotator::ZeroRotator);
			}
			W.Advance(0.3f);
		}
		return C;
	}

	static ATDCrewDummy* SpawnDummy(FTDTestWorld& W, const FVector& At, float Health = 1000.f)
	{
		ATDCrewDummy* D = W.Spawn<ATDCrewDummy>(At, FRotator(0.f, 180.f, 0.f));
		if (D)
		{
			D->MeleeInterval = 0.f;
			D->GetHealth()->InitMaxValues(Health, 0.f, true);
			D->GetHealth()->DestroyOwnerAfterDeathSeconds = -1.f;
		}
		return D;
	}

	static float Hull(AActor* A)
	{
		const UTDHealthComponent* H = UTDStatics::GetHealth(A);
		return H ? H->GetHull() : -1.f;
	}

	/** Never write the real save from ground tests that use the TD game instance. */
	struct FNoSaveWrites
	{
		UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
		bool bWas = S->bWriteSlotOnCheckpoint;
		FNoSaveWrites() { S->bWriteSlotOnCheckpoint = false; }
		~FNoSaveWrites() { S->bWriteSlotOnCheckpoint = bWas; }
	};
}


// ================================================================================================
// ENG-21 movement
// ================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundRunSprint, "TriadDrift.Ground.Move.RunAndSprintSpeeds", TD_TEST_FLAGS)
bool FTDGroundRunSprint::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
		if (!TestNotNull(TEXT("character"), C)) { return false; }
		C->SetMoveInput(FVector2D(0.f, 1.f));
		W.Advance(1.f, Step);
		TestEqual(FString::Printf(TEXT("run 650 cm/s @%.0f fps"), 1.f / Step), float(C->GetVelocity().Size2D()), 650.f, 15.f);
		C->SetSprintHeld(true);
		W.Advance(1.f, Step);
		TestTrue(TEXT("sprinting"), C->IsSprinting());
		TestEqual(FString::Printf(TEXT("sprint 950 cm/s @%.0f fps"), 1.f / Step), float(C->GetVelocity().Size2D()), 950.f, 15.f);
		C->SetMoveInput(FVector2D(0.f, -1.f));
		W.Advance(0.5f, Step);
		TestFalse(TEXT("no sprinting backwards"), C->IsSprinting());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundJumps, "TriadDrift.Ground.Move.JumpAndDoubleJump", TD_TEST_FLAGS)
bool FTDGroundJumps::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	if (!TestNotNull(TEXT("character"), C)) { return false; }
	TestTrue(TEXT("jump"), C->TryJump());
	W.Tick();
	TestEqual(TEXT("jump Z 650"), float(C->GetVelocity().Z), 650.f, 40.f);
	W.Advance(0.15f);
	TestTrue(TEXT("double jump"), C->TryJump());
	W.Tick();
	TestEqual(TEXT("double jump Z 500"), float(C->GetVelocity().Z), 500.f, 40.f);
	TestFalse(TEXT("no triple jump"), C->TryJump());
	W.Advance(2.f);
	TestTrue(TEXT("landed"), C->GetCharacterMovement()->IsMovingOnGround());
	TestEqual(TEXT("jumps reset on landing"), C->GetJumpsUsed(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundDash, "TriadDrift.Ground.Move.DashDistanceIFramesCharges", TD_TEST_FLAGS)
bool FTDGroundDash::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
		if (!TestNotNull(TEXT("character"), C)) { return false; }
		const FVector Start = C->GetActorLocation();
		TestEqual(TEXT("2 charges"), C->GetDashCharges(), 2);
		TestTrue(TEXT("dash"), C->TryDash());
		float T = 0.f;
		bool bIFramesMid = false;
		while (C->IsDashing() && T < 1.f)
		{
			W.Tick(Step);
			T += Step;
			if (T <= 0.1f + 1e-4f) { bIFramesMid = C->GetHealth()->IsInvulnerable(); }
		}
		TestTrue(TEXT("i-frames at 0.1 s"), bIFramesMid);
		TestTrue(FString::Printf(TEXT("dash lasts 0.18 s (+1 frame to hand back) @%.0f fps"), 1.f / Step), T >= 0.18f - 1e-3f && T <= FMath::CeilToFloat(0.18f / Step - 1e-3f) * Step + Step + 1e-3f);
		// The frame the dash ends already carries 950 cm/s of exit speed.
		const float Dist = (float)FVector::Dist2D(Start, C->GetActorLocation()) - C->Tuning.DashExitSpeed * Step;
		TestEqual(FString::Printf(TEXT("dash 1200 cm @%.0f fps"), 1.f / Step), Dist, 1200.f, 40.f);
		W.Advance(0.05f, Step);
		TestFalse(TEXT("i-frames over after 0.15 s"), C->GetHealth()->IsInvulnerable());
		TestFalse(TEXT("dash finished"), C->IsDashing());
		TestTrue(TEXT("second dash"), C->TryDash());
		W.Advance(0.25f, Step);
		TestFalse(TEXT("no third dash"), C->TryDash());
		TestEqual(TEXT("no charges"), C->GetDashCharges(), 0);
		W.Advance(1.2f, Step);
		TestEqual(TEXT("one charge back after 1.2 s"), C->GetDashCharges(), 1);
		W.Advance(1.2f, Step);
		TestEqual(TEXT("both charges back after 2.4 s"), C->GetDashCharges(), 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundSlide, "TriadDrift.Ground.Move.Slide", TD_TEST_FLAGS)
bool FTDGroundSlide::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	if (!TestNotNull(TEXT("character"), C)) { return false; }
	TestFalse(TEXT("no slide from standing"), C->TrySlide());
	C->SetMoveInput(FVector2D(0.f, 1.f));
	C->SetSprintHeld(true);
	W.Advance(1.f);
	TestTrue(TEXT("slide while sprinting"), C->TrySlide());
	W.Advance(0.3f);
	TestTrue(TEXT("still sliding at 0.3 s"), C->IsSliding());
	const float Speed = float(C->GetVelocity().Size2D());
	TestTrue(TEXT("slide faster than sprint early on"), Speed > 960.f && Speed < 1260.f);
	W.Advance(0.45f);
	TestFalse(TEXT("slide over after 0.7 s"), C->IsSliding());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundMantle, "TriadDrift.Ground.Move.MantleUpTo180", TD_TEST_FLAGS)
bool FTDGroundMantle::RunTest(const FString& Parameters)
{
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		TDGroundTest::SpawnBox(W, FVector(200.f, 0.f, 75.f), FVector(200.f, 400.f, 150.f)); // 150 cm ledge
		ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
		if (!TestNotNull(TEXT("character"), C)) { return false; }
		C->SetMoveInput(FVector2D(0.f, 1.f));
		bool bSawMantle = false;
		float MantleTime = 0.f;
		for (int32 i = 0; i < 60; ++i)
		{
			W.Tick();
			if (C->IsMantling()) { bSawMantle = true; MantleTime += 1.f / 60.f; }
			else if (bSawMantle) { C->SetMoveInput(FVector2D::ZeroVector); break; }
		}
		W.Advance(0.2f);
		TestTrue(TEXT("auto-mantled the 150 cm ledge"), bSawMantle);
		TestEqual(TEXT("mantle takes 0.35 s"), MantleTime, 0.35f, 0.04f);
		const float FeetZ = float(C->GetActorLocation().Z - 90.0);
		TestEqual(TEXT("standing on top"), FeetZ, 150.f, 15.f);
	}
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		TDGroundTest::SpawnBox(W, FVector(200.f, 0.f, 125.f), FVector(200.f, 400.f, 250.f)); // 250 cm: too tall
		ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
		if (!TestNotNull(TEXT("character"), C)) { return false; }
		C->SetMoveInput(FVector2D(0.f, 1.f));
		bool bSawMantle = false;
		for (int32 i = 0; i < 60; ++i)
		{
			W.Tick();
			bSawMantle |= C->IsMantling();
		}
		TestFalse(TEXT("no mantle on a 250 cm wall"), bSawMantle);
		TestEqual(TEXT("still on the floor"), float(C->GetActorLocation().Z - 90.0), 0.f, 15.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundHealth, "TriadDrift.Ground.HealthRegenAndDeathIsATake", TD_TEST_FLAGS)
bool FTDGroundHealth::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* Enemy = TDGroundTest::SpawnDummy(W, FVector(300.f, 0.f, 95.f));
	if (!TestNotNull(TEXT("character"), C) || !TestNotNull(TEXT("enemy"), Enemy)) { return false; }
	TestEqual(TEXT("100 health"), TDGroundTest::Hull(C), 100.f);
	TestEqual(TEXT("no shield"), C->GetHealth()->MaxShield, 0.f);
	UTDStatics::ApplyTDDamage(C, 50.f, ETDDamageKind::Energy, Enemy, Enemy);
	TestEqual(TEXT("took 50"), TDGroundTest::Hull(C), 50.f, 0.1f);
	W.Advance(3.9f);
	TestEqual(TEXT("no regen before 4 s"), TDGroundTest::Hull(C), 50.f, 0.5f);
	W.Advance(2.1f); // 4 s delay + 2 s at 15/s
	TestEqual(TEXT("regen 15/s after 4 s"), TDGroundTest::Hull(C), 80.f, 4.f);

	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	UTDStatics::ApplyTDDamage(C, 500.f, ETDDamageKind::Energy, Enemy, Enemy);
	TestTrue(TEXT("dead"), C->IsDead());
	TestTrue(TEXT("death calls CUT (retake pending)"), GI && GI->IsRetakePending());
	C->RespawnAt(FTransform(FVector(0.f, 0.f, 95.f)));
	TestFalse(TEXT("respawned alive"), C->IsDead());
	TestEqual(TEXT("respawned at full health"), TDGroundTest::Hull(C), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundInteract, "TriadDrift.Ground.InteractTapHoldAndInterrupt", TD_TEST_FLAGS)
bool FTDGroundInteract::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	if (!TestNotNull(TEXT("character"), C)) { return false; }
	const float EyeZ = C->GetActorLocation().Z + C->BaseEyeHeight;
	ATDInteractableActor* Tap = W.Spawn<ATDInteractableActor>(FVector(150.f, 0.f, EyeZ));
	if (!TestNotNull(TEXT("interactable"), Tap)) { return false; }
	W.Tick();
	TestEqual(TEXT("target found"), C->GetInteractTarget(), (AActor*)Tap);
	TestFalse(TEXT("prompt shown"), C->GetInteractPrompt().IsEmpty());
	C->BeginInteract();
	TestTrue(TEXT("tap interact"), Tap->WasUsed());
	TestTrue(TEXT("single use: prompt gone"), C->GetInteractPrompt().IsEmpty());

	Tap->Destroy();
	ATDInteractableActor* Hold = W.Spawn<ATDInteractableActor>(FVector(150.f, 0.f, EyeZ));
	Hold->HoldTime = 2.f;
	W.Tick();
	C->BeginInteract();
	W.Advance(1.f);
	TestEqual(TEXT("half way"), C->GetInteractProgress01(), 0.5f, 0.05f);
	ATDCrewDummy* Enemy = TDGroundTest::SpawnDummy(W, FVector(-300.f, 0.f, 95.f));
	UTDStatics::ApplyTDDamage(C, 5.f, ETDDamageKind::Energy, Enemy, Enemy);
	TestEqual(TEXT("damage interrupts the hold"), C->GetInteractProgress01(), 0.f);
	TestFalse(TEXT("not used"), Hold->WasUsed());
	C->BeginInteract();
	W.Advance(2.1f);
	TestTrue(TEXT("held for 2 s"), Hold->WasUsed());
	return true;
}

// ================================================================================================
// ENG-22 energy blade
// ================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundLightCombo, "TriadDrift.Ground.Blade.LightCombo", TD_TEST_FLAGS)
bool FTDGroundLightCombo::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* Front = TDGroundTest::SpawnDummy(W, FVector(130.f, 0.f, 95.f));
	ATDCrewDummy* Behind = TDGroundTest::SpawnDummy(W, FVector(-130.f, 0.f, 95.f));
	ATDCrewDummy* Far = TDGroundTest::SpawnDummy(W, FVector(0.f, 400.f, 95.f));
	W.Advance(0.2f);
	UTDEnergyBladeComponent* B = C ? C->GetBlade() : nullptr;
	if (!TestNotNull(TEXT("blade"), B)) { return false; }

	TestTrue(TEXT("swing 1"), B->LightAttack());
	TestEqual(TEXT("hit 1 = 25"), 1000.f - TDGroundTest::Hull(Front), 25.f, 0.1f);
	TestEqual(TEXT("100° arc: nothing behind"), TDGroundTest::Hull(Behind), 1000.f);
	TestEqual(TEXT("180 cm reach: nothing far away"), TDGroundTest::Hull(Far), 1000.f);
	TestTrue(TEXT("swing 2 buffered"), B->LightAttack());
	W.Advance(0.75f); // swing 1 (0.28) + buffered swing 2 (0.28) + hit-stops, still inside the 0.45 s window
	TestEqual(TEXT("hit 2 = 25"), 1000.f - TDGroundTest::Hull(Front), 50.f, 0.1f);
	TestTrue(TEXT("swing 3 within the window"), B->LightAttack());
	TestEqual(TEXT("hit 3 = 45"), 1000.f - TDGroundTest::Hull(Front), 95.f, 0.1f);
	TestEqual(TEXT("combo index 3"), B->GetComboIndex(), 3);
	W.Advance(0.5f);
	TestTrue(TEXT("third hit knocks back"), Front->GetActorLocation().X > 230.f);
	Front->SetActorLocation(FVector(130.f, 0.f, 95.f));
	W.Advance(1.0f); // swing ends + 0.45 s window expires
	TestTrue(TEXT("new combo"), B->LightAttack());
	TestEqual(TEXT("back to 25 after the window"), 1000.f - TDGroundTest::Hull(Front), 120.f, 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundHeavy, "TriadDrift.Ground.Blade.HeavyLungeAndCooldown", TD_TEST_FLAGS)
bool FTDGroundHeavy::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* Target = TDGroundTest::SpawnDummy(W, FVector(500.f, 0.f, 95.f));
	W.Advance(0.2f);
	UTDEnergyBladeComponent* B = C ? C->GetBlade() : nullptr;
	if (!TestNotNull(TEXT("blade"), B)) { return false; }
	const float StartX = C->GetActorLocation().X;
	TestTrue(TEXT("lunge"), B->HeavyLunge());
	TestEqual(TEXT("lunge damage 70"), 1000.f - TDGroundTest::Hull(Target), 70.f, 0.1f);
	TestTrue(TEXT("breaks guards"), Target->IsGuardBroken());
	W.Advance(0.3f);
	TestTrue(TEXT("lunged forward"), C->GetActorLocation().X - StartX > 250.f);
	TestFalse(TEXT("2 s cooldown"), B->HeavyLunge());
	W.Advance(1.8f);
	TestTrue(TEXT("ready after 2 s"), B->HeavyLunge());

	// Hold-to-heavy through the input path.
	W.Advance(2.1f);
	const float Before = TDGroundTest::Hull(Target);
	B->PressAttack();
	W.Advance(0.4f);
	B->ReleaseAttack();
	TestEqual(TEXT("holding attack 0.35 s = heavy (70)"), Before - TDGroundTest::Hull(Target), 70.f, 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundAirSlam, "TriadDrift.Ground.Blade.AirSlamAoE", TD_TEST_FLAGS)
bool FTDGroundAirSlam::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* Near = TDGroundTest::SpawnDummy(W, FVector(200.f, 100.f, 95.f));
	ATDCrewDummy* Out = TDGroundTest::SpawnDummy(W, FVector(700.f, 0.f, 95.f));
	W.Advance(0.2f);
	UTDEnergyBladeComponent* B = C ? C->GetBlade() : nullptr;
	if (!TestNotNull(TEXT("blade"), B)) { return false; }
	TestFalse(TEXT("no slam on the ground"), B->AirSlam());
	C->TryJump();
	W.Advance(0.2f);
	TestTrue(TEXT("slam in the air"), B->AirSlam());
	W.Advance(1.f);
	TestEqual(TEXT("slam 50 within 300 cm"), 1000.f - TDGroundTest::Hull(Near), 50.f, 0.1f);
	TestEqual(TEXT("nothing beyond 300 cm"), TDGroundTest::Hull(Out), 1000.f);
	TestEqual(TEXT("back to idle"), B->GetBladeState(), ETDBladeState::Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundDashStrike, "TriadDrift.Ground.Blade.DashStrike", TD_TEST_FLAGS)
bool FTDGroundDashStrike::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* InPath = TDGroundTest::SpawnDummy(W, FVector(700.f, 0.f, 95.f));
	W.Advance(0.2f);
	UTDEnergyBladeComponent* B = C ? C->GetBlade() : nullptr;
	if (!TestNotNull(TEXT("blade"), B)) { return false; }
	C->TryDash();
	W.Tick();
	TestTrue(TEXT("dashing"), C->IsDashing());
	B->PressAttack();
	B->ReleaseAttack();
	TestEqual(TEXT("dash strike 35"), 1000.f - TDGroundTest::Hull(InPath), 35.f, 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundBlock, "TriadDrift.Ground.Blade.BlockStaminaGuardBreakParry", TD_TEST_FLAGS)
bool FTDGroundBlock::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* Front = TDGroundTest::SpawnDummy(W, FVector(130.f, 0.f, 95.f));
	ATDCrewDummy* Back = TDGroundTest::SpawnDummy(W, FVector(-130.f, 0.f, 95.f));
	W.Advance(0.2f);
	UTDEnergyBladeComponent* B = C ? C->GetBlade() : nullptr;
	if (!TestNotNull(TEXT("blade"), B)) { return false; }

	// Perfect-timed block vs melee = parry: attacker staggers 1.2 s, next hit crits.
	B->SetBlockHeld(true);
	TestTrue(TEXT("blocking"), B->IsBlocking());
	TestTrue(TEXT("perfect window open"), B->IsInPerfectWindow());
	TestEqual(TEXT("parried hit deals nothing"), Front->MeleeAttack(C), 0.f);
	TestTrue(TEXT("attacker staggered"), Front->IsStaggered());
	TestTrue(TEXT("crit ready"), B->HasCritReady());
	W.Advance(0.3f);
	TestFalse(TEXT("perfect window closed after 0.2 s"), B->IsInPerfectWindow());
	TestEqual(TEXT("normal block stops front melee"), Front->MeleeAttack(C), 0.f); // staggered attackers can't swing anyway
	W.Advance(1.f);
	TestFalse(TEXT("stagger over after 1.2 s"), Front->IsStaggered());
	TestEqual(TEXT("normal block: front melee blocked"), Front->MeleeAttack(C), 0.f);
	TestFalse(TEXT("no parry outside the window"), Front->IsStaggered());
	TestEqual(TEXT("120° arc: hit from behind lands"), Back->MeleeAttack(C), 15.f, 0.1f);
	TestEqual(TEXT("move speed halved while blocking"), C->GetCharacterMovement()->MaxWalkSpeed, 325.f, 1.f);

	// 3 s of stamina, then a guard break.
	W.Advance(2.f);
	TestTrue(TEXT("guard broken after 3 s of blocking"), B->IsGuardBroken());
	TestFalse(TEXT("can't block while broken"), B->IsBlocking());
	B->SetBlockHeld(false);
	W.Advance(B->GuardBreakSeconds + 0.1f);
	TestFalse(TEXT("guard recovers"), B->IsGuardBroken());

	// The crit from the parry lands on the next blade hit: 25 x 2.
	const float Before = TDGroundTest::Hull(Front);
	B->LightAttack();
	TestEqual(TEXT("crit after parry = 50"), Before - TDGroundTest::Hull(Front), 50.f, 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundDeflect, "TriadDrift.Ground.Blade.DeflectNormalAndPerfect", TD_TEST_FLAGS)
bool FTDGroundDeflect::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	// Turret 30° to the left, inside the 120° guard; the crosshair looks straight down +X.
	const FVector TurretLoc = FVector(1400.f, -800.f, 0.f);
	ATDDummyTurret* T = W.Spawn<ATDDummyTurret>(TurretLoc + FVector(0.f, 0.f, 60.f)); // base sits on the floor
	if (!TestNotNull(TEXT("character"), C) || !TestNotNull(TEXT("turret"), T)) { return false; }
	T->bActive = false;
	UTDEnergyBladeComponent* B = C->GetBlade();

	int32 Normal = 0, Perfect = 0;
	B->OnDeflect.AddLambda([&](AActor*, bool bPerfect) { (bPerfect ? Perfect : Normal)++; });

	// Unblocked: the bolt hurts (10).
	T->FireAt(C);
	W.Advance(1.2f);
	TestEqual(TEXT("unblocked bolt hits for 10"), TDGroundTest::Hull(C), 90.f, 0.5f);

	// Normal deflect: block raised well before the shot -> flies along the crosshair, x1.
	B->SetBlockHeld(true);
	W.Advance(0.3f);
	ATDProjectile* P1 = Cast<ATDProjectile>(T->FireAt(C));
	for (int32 i = 0; i < 120 && Normal == 0; ++i) { W.Tick(); }
	TestEqual(TEXT("normal deflect"), Normal, 1);
	TestEqual(TEXT("no damage while blocking"), TDGroundTest::Hull(C), 90.f, 0.5f);
	if (P1 && P1->IsActive())
	{
		TestEqual(TEXT("deflected to the player team"), P1->GetTeam(), ETDTeam::Player);
		TestTrue(TEXT("normal deflect follows the crosshair"), FVector::DotProduct(P1->GetVelocity().GetSafeNormal(), FVector::ForwardVector) > 0.95f);
		TestNull(TEXT("normal deflect doesn't home"), P1->GetHomingTarget());
		TestEqual(TEXT("normal deflect x1"), P1->GetDamageMultiplier(), 1.f, 0.01f);
	}
	B->SetBlockHeld(false);
	W.Advance(1.f);

	// Perfect deflect: raise the block just before it lands -> back at the shooter, x2, slow-mo.
	const float TurretBefore = TDGroundTest::Hull(T);
	const float PlayerBefore = TDGroundTest::Hull(C);
	ATDProjectile* P2 = Cast<ATDProjectile>(T->FireAt(C));
	for (int32 i = 0; i < 240 && P2 && P2->IsActive() && FVector::Dist(P2->GetActorLocation(), C->GetActorLocation()) > 450.f; ++i) { W.Tick(); }
	B->SetBlockHeld(true);
	for (int32 i = 0; i < 60 && Perfect == 0; ++i) { W.Tick(); }
	TestEqual(TEXT("perfect deflect"), Perfect, 1);
	if (P2 && P2->IsActive())
	{
		TestEqual(TEXT("perfect deflect x2"), P2->GetDamageMultiplier(), 2.f, 0.01f);
		TestEqual(TEXT("perfect deflect homes on the shooter"), P2->GetHomingTarget(), (AActor*)T);
	}
	const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
	TestTrue(TEXT("0.15 s slow-mo"), Feel && Feel->IsSlowMoActive());
	W.Advance(2.f);
	TestEqual(TEXT("returned bolt hits the turret for 20"), TurretBefore - TDGroundTest::Hull(T), 20.f, 0.5f);
	TestTrue(TEXT("player unharmed by the deflected shot"), TDGroundTest::Hull(C) >= PlayerBefore);
	return true;
}

// ================================================================================================
// ENG-24 objectives
// ================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundObjective, "TriadDrift.Ground.ObjectiveStepsRewardAndRetake", TD_TEST_FLAGS)
bool FTDGroundObjective::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	ATDObjective* O = W.Spawn<ATDObjective>();
	AActor* Marker = W.Spawn<AActor>(FVector(500.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("objective"), O) || !TestNotNull(TEXT("GI"), GI)) { return false; }
	O->ObjectiveId = TEXT("Obj_WreckCore");
	O->RewardAbility = ETDAbilityId::GravityWell;
	O->RewardShip = ETDShipId::Bastion;
	FTDObjectiveStep S0; S0.Text = FText::FromString(TEXT("Follow the beacon")); S0.Waypoint = Marker;
	FTDObjectiveStep S1; S1.Text = FText::FromString(TEXT("Clear the wreck site"));
	O->Steps = { S0, S1 };

	UTDFlowTestListener* Listener = NewObject<UTDFlowTestListener>();
	Listener->AddToRoot();
	Listener->Bind(UTDEventBus::Get(W.GetWorld()));
	GI->NotifyLevelEntered(ETDDestination::Verdance);
	O->StartObjective();
	TestTrue(TEXT("OnObjectiveUpdated(Id, 0) on start"), Listener->ObjectiveSteps.Num() == 1 && Listener->ObjectiveSteps[0].Key == FName(TEXT("Obj_WreckCore")) && Listener->ObjectiveSteps[0].Value == 0);
	TestTrue(TEXT("active"), O->IsActive());
	TestEqual(TEXT("opens its scene"), GI->GetCurrentScene(), FName(TEXT("Obj_WreckCore")));
	TestEqual(TEXT("take 1"), GI->GetTakeNumber(TEXT("Obj_WreckCore")), 1);
	TArray<AActor*> Markers;
	UTDObjectiveSubsystem::Get(W.GetWorld())->GetActiveWaypoints(Markers);
	TestTrue(TEXT("waypoint exposed"), Markers.Contains(Marker));

	TestFalse(TEXT("out-of-order step ignored"), O->CompleteStep(1));
	TestTrue(TEXT("step 0"), O->CompleteStep(0));
	TestEqual(TEXT("now step 1"), O->GetCurrentStepIndex(), 1);

	// CUT: the objective restarts from step 0 as take 2.
	GI->RestartSceneNow();
	TestEqual(TEXT("retake resets to step 0"), O->GetCurrentStepIndex(), 0);
	TestTrue(TEXT("still active"), O->IsActive());
	TestEqual(TEXT("take 2"), GI->GetTakeNumber(TEXT("Obj_WreckCore")), 2);

	O->AdvanceStep();
	O->AdvanceStep();
	TestTrue(TEXT("complete"), O->IsComplete());
	TestTrue(TEXT("OnObjectiveUpdated(Id, StepCount) on completion"), Listener->ObjectiveSteps.Num() > 0 && Listener->ObjectiveSteps.Last().Value == 2);
	Listener->RemoveFromRoot();
	TestTrue(TEXT("recorded in progress"), GI->GetProgressRef().CompletedObjectives.Contains(TEXT("Obj_WreckCore")));
	TestEqual(TEXT("reward unlocked for Bastion"), GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);
	Markers.Reset();
	UTDObjectiveSubsystem::Get(W.GetWorld())->GetActiveWaypoints(Markers);
	TestEqual(TEXT("no markers once complete"), Markers.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundEncounter, "TriadDrift.Ground.EncounterSpawnsClearsOpensDoors", TD_TEST_FLAGS)
bool FTDGroundEncounter::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDObjective* O = W.Spawn<ATDObjective>();
	ATDDoor* Exit = W.Spawn<ATDDoor>(FVector(1000.f, 0.f, 0.f));
	ATDDoor* Entry = W.Spawn<ATDDoor>(FVector(-1000.f, 0.f, 0.f));
	ATDEncounterVolume* E = W.Spawn<ATDEncounterVolume>(FVector(0.f, 0.f, 150.f));
	if (!TestNotNull(TEXT("encounter"), E) || !TestNotNull(TEXT("objective"), O)) { return false; }
	Entry->bStartOpen = true;
	O->ObjectiveId = TEXT("Obj_Test");
	O->Steps.SetNum(2);
	E->Count = 3;
	E->DoorsToOpen = { Exit };
	E->DoorsToLock = { Entry };
	E->LinkedObjective = O;
	E->LinkedStep = 1;
	W.Advance(0.1f);
	Entry->Open();

	E->Activate();
	TestFalse(TEXT("waits for its objective step"), E->IsActive());
	O->StartObjective();
	O->CompleteStep(0);
	E->Activate();
	TestTrue(TEXT("active"), E->IsActive());
	TestEqual(TEXT("3 crew spawned"), E->GetAliveCount(), 3);
	TestFalse(TEXT("entry door shut behind you"), Entry->IsOpen());
	TestFalse(TEXT("exit closed"), Exit->IsOpen());

	for (const TWeakObjectPtr<AActor>& M : E->GetSpawned())
	{
		if (UTDHealthComponent* H = M.IsValid() ? UTDStatics::GetHealth(M.Get()) : nullptr)
		{
			TestFalse(TEXT("not cleared while crew alive"), E->IsCleared());
			H->Kill(nullptr, ETDDamageKind::Melee);
		}
	}
	TestTrue(TEXT("cleared"), E->IsCleared());
	TestTrue(TEXT("exit opens"), Exit->IsOpen());
	TestTrue(TEXT("entry reopens"), Entry->IsOpen());
	TestTrue(TEXT("objective step completed"), O->IsComplete());
	W.Advance(1.f);
	TestEqual(TEXT("door slid open"), Exit->GetOpen01(), 1.f, 0.01f);

	E->ResetEncounter();
	TestFalse(TEXT("reset"), E->IsCleared() || E->IsActive());
	TestEqual(TEXT("crew removed"), E->GetAliveCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundSandbox, "TriadDrift.Ground.SandboxBoots", TD_TEST_FLAGS)
bool FTDGroundSandbox::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass(), ATDGroundGameMode::StaticClass());
	TestTrue(TEXT("world"), W.IsValid());
	W.Advance(0.5f);
	const ATDGroundGameMode* GM = W.GetWorld()->GetAuthGameMode<ATDGroundGameMode>();
	if (!TestNotNull(TEXT("ground mode"), GM)) { return false; }
	TestNotNull(TEXT("turret"), GM->GetTurret());
	TestNotNull(TEXT("encounter"), GM->GetEncounter());
	TestTrue(TEXT("sample objective running"), GM->GetSampleObjective() && GM->GetSampleObjective()->IsActive());
	int32 Blocks = 0;
	for (TActorIterator<AStaticMeshActor> It(W.GetWorld()); It; ++It) { ++Blocks; }
	TestTrue(TEXT("floor, ledges, platforms and room built"), Blocks >= 15);
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	TestEqual(TEXT("entered Verdance"), GI ? GI->GetCurrentDestination() : ETDDestination::MainMenu, ETDDestination::Verdance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundPerfectWindow, "TriadDrift.Ground.Blade.PerfectWindowAt30_60_120", TD_TEST_FLAGS)
bool FTDGroundPerfectWindow::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		TDGroundTest::SpawnFloor(W);
		ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
		ATDCrewDummy* Early = TDGroundTest::SpawnDummy(W, FVector(130.f, -40.f, 95.f));
		ATDCrewDummy* Late = TDGroundTest::SpawnDummy(W, FVector(130.f, 40.f, 95.f));
		UTDEnergyBladeComponent* B = C ? C->GetBlade() : nullptr;
		if (!TestNotNull(TEXT("blade"), B)) { return false; }
		B->SetBlockHeld(true);
		float T = 0.f;
		while (T + Step < 0.19f) { W.Tick(Step); T += Step; }
		Early->MeleeAttack(C);
		TestTrue(FString::Printf(TEXT("parry at %.3f s @%.0f fps"), T, 1.f / Step), Early->IsStaggered());
		while (T <= 0.2f + 1e-4f) { W.Tick(Step); T += Step; }
		Late->MeleeAttack(C);
		TestFalse(FString::Printf(TEXT("no parry at %.3f s @%.0f fps"), T, 1.f / Step), Late->IsStaggered());
		TestEqual(TEXT("blocked anyway"), TDGroundTest::Hull(C), 100.f, 0.1f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundHudPing, "TriadDrift.Ground.HudDataAndObjectivePing", TD_TEST_FLAGS)
bool FTDGroundHudPing::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* Enemy = TDGroundTest::SpawnDummy(W, FVector(-400.f, 0.f, 95.f));
	ATDObjective* O = W.Spawn<ATDObjective>();
	AActor* Marker = W.Spawn<AActor>(FVector(900.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("character"), C) || !TestNotNull(TEXT("objective"), O)) { return false; }
	FTDObjectiveStep S0; S0.Waypoint = Marker;
	O->ObjectiveId = TEXT("Obj_Hud");
	O->Steps = { S0 };
	O->StartObjective();

	const ITDHUDData* Hud = C;
	TestEqual(TEXT("HUD health 1"), Hud->GetHealth01(), 1.f, 1e-3f);
	UTDStatics::ApplyTDDamage(C, 25.f, ETDDamageKind::Energy, Enemy, Enemy);
	TestEqual(TEXT("HUD health 0.75"), Hud->GetHealth01(), 0.75f, 1e-3f);
	TestEqual(TEXT("HUD dash charges 2"), Hud->GetDashCharges(), 2);
	C->TryDash();
	TestEqual(TEXT("HUD dash charges 1"), Hud->GetDashCharges(), 1);
	W.Advance(0.3f);
	C->GetBlade()->SetBlockHeld(true);
	W.Advance(1.5f);
	TestEqual(TEXT("HUD block stamina halves after 1.5 s"), Hud->GetBlockStamina01(), 0.5f, 0.05f);
	C->GetBlade()->SetBlockHeld(false);
	TestTrue(TEXT("HUD speed01 in range"), Hud->GetSpeed01() >= 0.f && Hud->GetSpeed01() <= 1.f);
	const TArray<AActor*> Markers = Hud->GetObjectiveMarkers();
	TestTrue(TEXT("HUD objective marker"), Markers.Contains(Marker));

	TestFalse(TEXT("not pinged"), C->IsObjectivePinged());
	C->PingObjective();
	TestTrue(TEXT("pinged"), C->IsObjectivePinged());
	W.Advance(3.9f);
	TestTrue(TEXT("ping lasts 4 s"), C->IsObjectivePinged());
	W.Advance(0.2f);
	TestFalse(TEXT("ping gone after 4 s"), C->IsObjectivePinged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundHitStop, "TriadDrift.Ground.Blade.HitStop50And120", TD_TEST_FLAGS)
bool FTDGroundHitStop::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* Tough = TDGroundTest::SpawnDummy(W, FVector(130.f, 0.f, 95.f), 1000.f);
	UTDEnergyBladeComponent* B = C ? C->GetBlade() : nullptr;
	const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("blade"), B) || !TestNotNull(TEXT("feel"), Feel)) { return false; }
	B->LightAttack();
	W.Advance(0.03f, 0.01f);
	TestTrue(TEXT("hit-stop active at 30 ms"), Feel->GetCurrentDilation() < 0.99f);
	W.Advance(0.04f, 0.01f);
	TestEqual(TEXT("hit-stop over after 50 ms"), Feel->GetCurrentDilation(), 1.f, 1e-3f);

	W.Advance(1.5f);
	Tough->SetActorLocation(FVector(500.f, 500.f, 95.f));
	ATDCrewDummy* Weak = TDGroundTest::SpawnDummy(W, FVector(130.f, 0.f, 95.f), 10.f);
	W.Advance(0.2f);
	B->LightAttack();
	TestTrue(TEXT("killed"), Weak->GetHealth()->IsDead());
	W.Advance(0.09f, 0.01f);
	TestTrue(TEXT("kill hit-stop still active at 90 ms"), Feel->GetCurrentDilation() < 0.99f);
	W.Advance(0.05f, 0.01f);
	TestEqual(TEXT("kill hit-stop over after 120 ms"), Feel->GetCurrentDilation(), 1.f, 1e-3f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

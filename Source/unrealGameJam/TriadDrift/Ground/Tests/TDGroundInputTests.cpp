// Triad Drift - real key injection and blade feel (review M1, M2, M8, M9, M11). Owner: Teammate 5.
// Uses FTDTestWorld::SpawnLocalPlayerController: a real ULocalPlayer, Enhanced Input subsystem and keys.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Flow/Tests/TDFlowTestListener.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

namespace TDInputTest
{
	inline UEnhancedInputLocalPlayerSubsystem* InputOf(APlayerController* PC)
	{
		const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
		return LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	}

	/** Press, run a frame, release, run a frame: a real tap. */
	inline void Tap(FTDTestWorld& W, APlayerController* PC, const FKey& Key)
	{
		FTDTestWorld::InjectKey(PC, Key, true);
		W.Tick();
		FTDTestWorld::InjectKey(PC, Key, false);
		W.Tick();
	}
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundRealKeys, "TriadDrift.Ground.RealKeyInjection", TD_TEST_FLAGS)
bool FTDGroundRealKeys::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	APlayerController* PC = W.SpawnLocalPlayerController(C);
	if (!TestNotNull(TEXT("local player controller"), PC) || !TestTrue(TEXT("local"), PC->IsLocalController())) { return false; }
	UEnhancedInputLocalPlayerSubsystem* Input = TDInputTest::InputOf(PC);
	TestTrue(TEXT("IMC_Ground active on possess"), Input && Input->HasMappingContext(C->GetGroundInputContext()));
	W.Advance(0.3f);

	// W: run at 650.
	FTDTestWorld::InjectKey(PC, EKeys::W, true);
	W.Advance(1.f);
	TestEqual(TEXT("holding W runs at 650"), float(C->GetVelocity().Size2D()), 650.f, 20.f);
	FTDTestWorld::InjectKey(PC, EKeys::W, false);
	W.Advance(0.5f);
	TestTrue(TEXT("releasing W stops"), C->GetVelocity().Size2D() < 50.f);

	// Q: dash.
	const FVector Before = C->GetActorLocation();
	FTDTestWorld::InjectKey(PC, EKeys::Q, true);
	W.Tick();
	TestTrue(TEXT("Q dashes"), C->IsDashing());
	FTDTestWorld::InjectKey(PC, EKeys::Q, false);
	W.Advance(0.3f);
	TestTrue(TEXT("dash carried us forward"), FVector::Dist2D(Before, C->GetActorLocation()) > 1000.f);

	// LMB: the light hit lands on the press frame (M8), before the button is released.
	ATDCrewDummy* D = TDGroundTest::SpawnDummy(W, C->GetActorLocation() + FVector(130.f, 0.f, 0.f));
	W.Advance(0.2f);
	FTDTestWorld::InjectKey(PC, EKeys::LeftMouseButton, true);
	W.Tick();
	TestEqual(TEXT("LMB press lands the light hit at once"), 1000.f - TDGroundTest::Hull(D), 25.f, 0.1f);
	FTDTestWorld::InjectKey(PC, EKeys::LeftMouseButton, false);
	W.Advance(0.6f);
	TDInputTest::Tap(W, PC, EKeys::LeftMouseButton);
	TestEqual(TEXT("a second tap continues the combo"), 1000.f - TDGroundTest::Hull(D), 50.f, 0.1f);

	// RMB: block while held.
	W.Advance(1.f);
	FTDTestWorld::InjectKey(PC, EKeys::RightMouseButton, true);
	W.Tick();
	TestTrue(TEXT("RMB raises the block"), C->GetBlade()->IsBlocking());
	FTDTestWorld::InjectKey(PC, EKeys::RightMouseButton, false);
	W.Tick();
	TestFalse(TEXT("releasing RMB lowers it"), C->GetBlade()->IsBlocking());

	// Attack out of a block (minor): LMB while RMB is still held swings, and the guard comes back.
	FTDTestWorld::InjectKey(PC, EKeys::RightMouseButton, true);
	W.Advance(0.3f);
	const float Hp = TDGroundTest::Hull(D);
	TDInputTest::Tap(W, PC, EKeys::LeftMouseButton);
	TestTrue(TEXT("can swing out of a block"), TDGroundTest::Hull(D) < Hp);
	W.Advance(0.6f);
	TestTrue(TEXT("guard comes back while RMB is held"), C->GetBlade()->IsBlocking());
	TestFalse(TEXT("an auto re-raise has no perfect window"), C->GetBlade()->IsInPerfectWindow());
	FTDTestWorld::InjectKey(PC, EKeys::RightMouseButton, false);

	// M11: leaving the character takes IMC_Ground away and clears held input.
	PC->UnPossess();
	TestFalse(TEXT("IMC_Ground removed on unpossess"), Input && Input->HasMappingContext(C->GetGroundInputContext()));
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundMenuInput, "TriadDrift.Ground.PossessRestoresGameplayInput", TD_TEST_FLAGS)
bool FTDGroundMenuInput::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	APlayerController* PC = W.SpawnLocalPlayerController(nullptr);
	if (!TestNotNull(TEXT("PC"), PC)) { return false; }
	// As if we came from the main menu: UI-only input and a cursor.
	PC->SetInputMode(FInputModeUIOnly());
	PC->SetShowMouseCursor(true);
	PC->Possess(C);
	TestFalse(TEXT("cursor hidden once the pilot is possessed"), PC->ShouldShowMouseCursor());
	W.Advance(0.3f);
	FTDTestWorld::InjectKey(PC, EKeys::W, true);
	W.Advance(0.8f);
	TestTrue(TEXT("keys move the character after a menu"), C->GetVelocity().Size2D() > 500.f);
	FTDTestWorld::InjectKey(PC, EKeys::W, false);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundTwoBursts, "TriadDrift.Ground.Blade.DeflectsEveryShotOfRepeatedBursts", TD_TEST_FLAGS)
bool FTDGroundTwoBursts::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	ATDDummyTurret* T = W.Spawn<ATDDummyTurret>(FVector(1500.f, 0.f, 60.f));
	if (!TestNotNull(TEXT("turret"), T)) { return false; }
	T->bActive = false;
	UTDEnergyBladeComponent* B = C->GetBlade();
	B->BlockStaminaSeconds = 30.f; // the test is about pooling, not stamina
	B->ResetState();
	B->SetBlockHeld(true);
	W.Advance(0.4f);
	int32 Deflected = 0;
	B->OnDeflect.AddLambda([&](AActor*, bool) { ++Deflected; });
	for (int32 Burst = 0; Burst < 2; ++Burst)
	{
		for (int32 Shot = 0; Shot < 3; ++Shot)
		{
			T->FireAt(C);
			W.Advance(0.2f);
		}
		W.Advance(1.5f); // the first burst's pooled bolts are released and reused by the second
	}
	TestEqual(TEXT("all 6 bolts of two bursts deflected inside one block (M1)"), Deflected, 6);
	TestEqual(TEXT("no bolt absorbed as damage"), TDGroundTest::Hull(C), 100.f, 0.1f);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundCrosshairDeflect, "TriadDrift.Ground.Blade.NormalDeflectHitsTheCrosshairTarget", TD_TEST_FLAGS)
bool FTDGroundCrosshairDeflect::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	// The crosshair is on a dummy straight ahead; the turret shoots from 40 degrees to the left.
	ATDCrewDummy* Mark = TDGroundTest::SpawnDummy(W, FVector(900.f, 0.f, 95.f));
	ATDDummyTurret* T = W.Spawn<ATDDummyTurret>(FVector(1100.f, -900.f, 60.f));
	if (!TestNotNull(TEXT("turret"), T) || !TestNotNull(TEXT("mark"), Mark)) { return false; }
	T->bActive = false;
	if (AController* Ctrl = C->GetController())
	{
		const FVector Eye = C->GetActorLocation() + FVector(0.f, 0.f, C->BaseEyeHeight);
		Ctrl->SetControlRotation((Mark->GetActorLocation() - Eye).Rotation());
	}
	UTDEnergyBladeComponent* B = C->GetBlade();
	B->SetBlockHeld(true);
	W.Advance(0.4f); // past the perfect window: a normal deflect
	int32 Normal = 0;
	B->OnDeflect.AddLambda([&](AActor*, bool bPerfect) { Normal += bPerfect ? 0 : 1; });
	T->FireAt(C);
	W.Advance(2.f);
	TestEqual(TEXT("normal deflect"), Normal, 1);
	TestEqual(TEXT("the returned bolt hits what the crosshair is on (M9)"), 1000.f - TDGroundTest::Hull(Mark), 10.f, 0.1f);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundSlamNeverSticks, "TriadDrift.Ground.Blade.AirSlamNeverSticks", TD_TEST_FLAGS)
bool FTDGroundSlamNeverSticks::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	TDGroundTest::SpawnBox(W, FVector(200.f, 0.f, 75.f), FVector(200.f, 400.f, 150.f)); // a 150 cm ledge ahead
	ATDFirstPersonCharacter* C = TDGroundTest::SpawnPlayer(W);
	UTDEnergyBladeComponent* B = C ? C->GetBlade() : nullptr;
	if (!TestNotNull(TEXT("blade"), B)) { return false; }
	C->TryJump();
	W.Advance(0.15f);
	TestTrue(TEXT("slam"), B->AirSlam());
	TestFalse(TEXT("no mantle during a slam (M2)"), C->TryMantle());
	W.Advance(1.f);
	TestEqual(TEXT("slam resolved on touching down"), B->GetBladeState(), ETDBladeState::Idle);

	// A slam that can never land (no gravity) still expires and frees the blade.
	C->TryJump();
	W.Advance(0.15f);
	TestTrue(TEXT("second slam"), B->AirSlam());
	C->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	C->GetCharacterMovement()->StopMovementImmediately();
	W.Advance(B->AirSlamTimeout + 0.2f);
	TestEqual(TEXT("timed out to Idle"), B->GetBladeState(), ETDBladeState::Idle);
	C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	W.Advance(1.f);
	TestTrue(TEXT("blade works again"), B->LightAttack());
	return true;
}


// ------------------------------------------------------------------------------------------------
// Review MAJOR 3: Escape / Start pauses on foot too (IMC_SpaceCommon is off while walking).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundPauseKey, "TriadDrift.Ground.EscapePausesOnFoot", TD_TEST_FLAGS)
bool FTDGroundPauseKey::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(C, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("space controller"), PC)) { return false; }
	W.Advance(0.3f);
	TestTrue(TEXT("possessing the pilot"), PC->GetPawn() == C);
	UTDFlowTestListener* Counter = NewObject<UTDFlowTestListener>();
	Counter->AddToRoot();
	ON_SCOPE_EXIT { Counter->RemoveFromRoot(); };
	int32 Pauses = 0;
	PC->OnPauseInput.AddDynamic(Counter, &UTDFlowTestListener::OnSimple);
	FTDTestWorld::InjectKey(PC, EKeys::Escape, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::Escape, false);
	W.Tick();
	Pauses = Counter->SimpleCount;
	TestEqual(TEXT("Escape on foot reaches OnPauseInput once"), Pauses, 1);
	// And while the game is paused (the same key unpauses).
	W.GetWorld()->GetFirstPlayerController()->SetPause(true);
	FTDTestWorld::InjectKey(PC, EKeys::Escape, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::Escape, false);
	W.Tick();
	TestEqual(TEXT("Escape still reaches it while paused"), Counter->SimpleCount, 2);
	PC->SetPause(false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

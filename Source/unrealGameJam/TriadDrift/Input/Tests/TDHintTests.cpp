// First-minute control hints. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Hints

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Waves/TDWaveTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Powers/TDAbilityRewards.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "InputActionValue.h"

/** Mouse aim on the controller (the headless world has no viewport to pair MouseX/Y into Mouse2D). */
struct FTDHintTestAccess
{
	ATDSpacePlayerController* PC;
	void Mouse(const FVector2D& D) { PC->OnMouseAim(FInputActionValue(D)); }
};

namespace TDHintTest
{
	struct FSquadWorld
	{
		FTDTestWorld W;
		explicit FSquadWorld(UClass* GameInstanceClass = nullptr) : W(GameInstanceClass) {}
		ATDSquadManager* Squad = nullptr;
		ATDSpacePlayerController* PC = nullptr;
		UTDHintComponent* Hints = nullptr;

		bool Init()
		{
			Squad = W.Spawn<ATDSquadManager>();
			PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
			if (!Squad || !PC) { return false; }
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20000.f)));
			Hints = PC->GetHints();
			if (!Hints) { return false; }
			Hints->UseMemoryOnlyForTests();
			W.Advance(0.5f);
			return true;
		}

		void WaveStarts(int32 Wave)
		{
			if (UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld())) { Bus->BroadcastWaveStarted(Wave, 0.2f); }
		}

		void Press(const FKey& Key, float Seconds)
		{
			FTDTestWorld::InjectKey(PC, Key, true);
			W.Advance(Seconds);
			FTDTestWorld::InjectKey(PC, Key, false);
			W.Tick();
		}

		/** Ticks until a hint is SHOWING (not ticked off) or Seconds pass. */
		ETDHint WaitForHint(float Seconds = 9.f) // the wave 1 story line (A1_HailNoAnswer) plays before the first hint
		{
			for (float T = 0.f; T < Seconds && Hints->GetCurrentHintState() != ETDHintState::Showing; T += 1.f / 60.f) { W.Tick(); }
			return Hints->GetCurrentHintState() == ETDHintState::Showing ? Hints->GetCurrentHint() : ETDHint::None;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHintsCoreSequence, "TriadDrift.Input.Hints.CoreSequenceCompletesByDoingIt", TD_TEST_FLAGS)
bool FTDHintsCoreSequence::RunTest(const FString& Parameters)
{
	TDHintTest::FSquadWorld S;
	if (!TestTrue(TEXT("squad, controller and hints"), S.Init())) { return false; }
	TestEqual(TEXT("nothing before wave 1"), S.Hints->GetCurrentHint(), ETDHint::None);
	S.WaveStarts(1);
	S.W.Advance(0.5f);
	TestEqual(TEXT("the first hint waits for the slate card"), S.Hints->GetCurrentHint(), ETDHint::None);

	// 1. Aim / throttle: mouse steering completes it.
	TestEqual(TEXT("first: fly"), S.WaitForHint(), ETDHint::Fly);
	TestTrue(TEXT("keyboard wording"), S.Hints->GetCurrentText().ToString().Contains(TEXT("MOUSE")));
	TestTrue(TEXT("shown through the HUD prompt channel"), UTDHintComponent::GetHintText(S.W.GetWorld()).EqualTo(S.Hints->GetCurrentText()));
	FTDHintTestAccess Access{ S.PC };
	for (int32 i = 0; i < 90; ++i) { Access.Mouse(FVector2D(12.f, 0.f)); S.W.Tick(); }
	TestTrue(TEXT("steering completes fly"), S.Hints->IsHintDone(ETDHint::Fly));

	// 2. Fire.
	TestEqual(TEXT("then: fire"), S.WaitForHint(), ETDHint::Fire);
	S.Press(EKeys::LeftMouseButton, 0.3f);
	TestTrue(TEXT("LMB completes fire"), S.Hints->IsHintDone(ETDHint::Fire));

	// 3. Switch ship (onboarding: the four essentials first: fly, shoot, switch, power).
	TestEqual(TEXT("then: switch"), S.WaitForHint(), ETDHint::Switch);
	S.Press(EKeys::Two, 0.05f);
	TestTrue(TEXT("2 completes switch"), S.Hints->IsHintDone(ETDHint::Switch));

	// 4. Signature power.
	TestEqual(TEXT("then: power"), S.WaitForHint(), ETDHint::Power);
	S.Press(EKeys::Q, 0.05f);
	TestTrue(TEXT("Q completes power"), S.Hints->IsHintDone(ETDHint::Power));
	int32 EssDone = 0, EssTotal = 0;
	S.Hints->GetEssentialsProgress(EssDone, EssTotal);
	TestTrue(TEXT("the four essentials are done"), EssDone == 4 && EssTotal == 4);

	// 5. Boost.
	TestEqual(TEXT("then: boost"), S.WaitForHint(), ETDHint::Boost);
	S.Press(EKeys::LeftShift, 0.5f);
	TestTrue(TEXT("shift completes boost"), S.Hints->IsHintDone(ETDHint::Boost));

	// 6. Dodge.
	TestEqual(TEXT("then: dodge"), S.WaitForHint(), ETDHint::Dodge);
	S.Press(EKeys::LeftAlt, 0.05f);
	S.W.Tick();
	TestTrue(TEXT("alt completes dodge"), S.Hints->IsHintDone(ETDHint::Dodge));

	// 7. Target cycle.
	TestEqual(TEXT("then: target cycle"), S.WaitForHint(), ETDHint::TargetCycle);
	S.Press(EKeys::T, 0.05f);
	TestTrue(TEXT("T completes target cycle"), S.Hints->IsHintDone(ETDHint::TargetCycle));

	TestEqual(TEXT("the core sequence is over"), S.WaitForHint(3.f), ETDHint::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHintsLater, "TriadDrift.Input.Hints.LaterHintsWhenTheyApply", TD_TEST_FLAGS)
bool FTDHintsLater::RunTest(const FString& Parameters)
{
	TDHintTest::FSquadWorld S;
	if (!TestTrue(TEXT("squad, controller and hints"), S.Init())) { return false; }
	UTDEventBus* Bus = UTDEventBus::Get(S.W.GetWorld());
	if (!TestNotNull(TEXT("bus"), Bus)) { return false; }

	// An experienced pilot: everything done before the hints come up, so none shows.
	FTDHintTestAccess Access{ S.PC };
	for (int32 i = 0; i < 90; ++i) { Access.Mouse(FVector2D(12.f, 0.f)); S.W.Tick(); }
	S.Press(EKeys::LeftMouseButton, 0.2f);
	S.Press(EKeys::LeftShift, 0.4f);
	S.Press(EKeys::LeftAlt, 0.05f);
	S.W.Advance(0.5f);
	S.Press(EKeys::Two, 0.05f);
	S.Press(EKeys::Q, 0.05f);
	S.Press(EKeys::T, 0.05f);
	S.WaveStarts(1);
	TestEqual(TEXT("an experienced pilot sees no core hint"), S.WaitForHint(5.f), ETDHint::None);

	// The wave 2 drift tip: drift shows next, holding C completes it.
	Bus->BroadcastTipRequested(TDTipIds::Drift);
	TestEqual(TEXT("drift hint"), S.WaitForHint(), ETDHint::Drift);
	S.Press(EKeys::C, 0.6f);
	TestTrue(TEXT("C completes drift"), S.Hints->IsHintDone(ETDHint::Drift));

	// The first planet marker: hyperdrive (completed by a spool; H starts one with no enemies near).
	Bus->BroadcastTipRequested(TDTipIds::Hyperdrive);
	TestEqual(TEXT("hyperdrive hint"), S.WaitForHint(), ETDHint::Hyperdrive);
	TestTrue(TEXT("hyperdrive wording"), S.Hints->GetCurrentText().ToString().Contains(TEXT("HOLD H")));
	FTDTestWorld::InjectKey(S.PC, EKeys::H, true);
	S.W.Advance(0.4f);
	FTDTestWorld::InjectKey(S.PC, EKeys::H, false);
	S.W.Tick();
	TestTrue(TEXT("a spool completes hyperdrive"), S.Hints->IsHintDone(ETDHint::Hyperdrive));
	S.W.Advance(2.f);

	// An unlocked ability: its hint waits for its own ship (board N-070), named, completed by E.
	if (UTDAbilityRewardSubsystem* Rewards = UTDAbilityRewardSubsystem::Get(S.W.GetWorld())) { Rewards->ResetRewards(); }
	if (S.Squad->GetActiveShipId() == ETDShipId::Bastion) { S.Squad->RequestSwitchTo(ETDShipId::Rift); S.W.Advance(1.f); }
	Bus->OnAbilityUnlocked.Broadcast(ETDAbilityId::GravityWell, ETDShipId::Bastion);
	TestEqual(TEXT("another ship: no ability hint"), S.WaitForHint(2.f), ETDHint::None);
	S.Squad->RequestSwitchTo(ETDShipId::Bastion);
	S.W.Advance(1.f);
	TestEqual(TEXT("ability hint"), S.WaitForHint(), ETDHint::Ability);
	TestTrue(*FString::Printf(TEXT("names the ability (%s)"), *S.Hints->GetCurrentText().ToString()), S.Hints->GetCurrentText().ToString().Contains(TEXT("GRAVITY WELL")));
	// Live (Nov 14): the card stayed up on the wrong ship. Switched away it comes off; back on Bastion it returns.
	S.Squad->RequestSwitchTo(ETDShipId::Rift);
	S.W.Advance(1.f);
	TestNotEqual(TEXT("on Rift: no Gravity Well card"), S.Hints->GetCurrentHint(), ETDHint::Ability);
	S.Squad->RequestSwitchTo(ETDShipId::Bastion);
	S.W.Advance(1.f);
	TestEqual(TEXT("back on Bastion: its card again"), S.WaitForHint(), ETDHint::Ability);
	S.Press(EKeys::E, 0.05f);
	TestTrue(TEXT("E completes the ability hint"), S.Hints->IsHintDone(ETDHint::Ability));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHintsRules, "TriadDrift.Input.Hints.OncePriorityTimeoutDeviceAndSetting", TD_TEST_FLAGS)
bool FTDHintsRules::RunTest(const FString& Parameters)
{
	TDHintTest::FSquadWorld S;
	if (!TestTrue(TEXT("squad, controller and hints"), S.Init())) { return false; }
	S.Hints->TimeoutSeconds = 3.f;
	S.WaveStarts(1);
	TestEqual(TEXT("fly"), S.WaitForHint(), ETDHint::Fly);

	// Paused: no hint, and its clock stops.
	TestTrue(TEXT("pause"), S.PC->SetPlayerPaused(true));
	S.W.Tick();
	TestEqual(TEXT("paused: no hint"), S.Hints->GetCurrentHint(), ETDHint::None);
	TestTrue(TEXT("paused: the HUD channel is empty"), UTDHintComponent::GetHintText(S.W.GetWorld()).IsEmpty());
	S.PC->SetPlayerPaused(false);
	S.W.Tick();
	TestEqual(TEXT("unpaused: the same hint is back"), S.Hints->GetCurrentHint(), ETDHint::Fly);

	// Ignored once: not done, it goes to the back of the queue, and the next one comes up (onboarding: a second chance).
	S.W.Advance(3.2f);
	TestFalse(TEXT("an ignored hint is not done after one timeout"), S.Hints->IsHintDone(ETDHint::Fly));
	TestTrue(TEXT("it was missed once"), S.Hints->WasMissed(ETDHint::Fly));
	TestEqual(TEXT("next hint"), S.WaitForHint(), ETDHint::Fire);

	// Device-aware wording.
	TestTrue(TEXT("keyboard: LEFT MOUSE"), S.Hints->GetCurrentText().ToString().Contains(TEXT("LEFT MOUSE")));
	TestTrue(TEXT("gamepad wording differs"), UTDHintComponent::TextFor(ETDHint::Fire, true, ETDAbilityId::None).ToString().Contains(TEXT("RB")));

	// Once per profile: a second wave start never brings back a done hint.
	S.Press(EKeys::LeftMouseButton, 0.2f);
	FTDHintTestAccess Steer{ S.PC };
	for (int32 i = 0; i < 90; ++i) { Steer.Mouse(FVector2D(12.f, 0.f)); S.W.Tick(); }
	S.WaveStarts(2);
	for (int32 i = 0; i < 60 * 8; ++i)
	{
		S.W.Tick();
		const ETDHint H = S.Hints->GetCurrentHint();
		if (S.Hints->GetCurrentHintState() == ETDHintState::Showing && (H == ETDHint::Fly || H == ETDHint::Fire))
		{
			AddError(TEXT("a done hint came back"));
			break;
		}
	}

	// The setting turns them off.
	S.Hints->SetHintsEnabled(false);
	S.W.Tick();
	TestEqual(TEXT("off: no hint"), S.Hints->GetCurrentHint(), ETDHint::None);
	TestTrue(TEXT("off: nothing in the HUD channel"), UTDHintComponent::GetHintText(S.W.GetWorld()).IsEmpty());
	return true;
}

// Onboarding (T3, polish item 4): the four essentials first with a progress count, a tick-off beat when the player
// does it, a second chance for a missed hint (a second miss counts as seen), and no control lessons on a Continue.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHintsOnboarding, "TriadDrift.Input.Hints.OnboardingDoneBeatSecondChanceAndContinue", TD_TEST_FLAGS)
bool FTDHintsOnboarding::RunTest(const FString& Parameters)
{
	{
		TDHintTest::FSquadWorld S;
		if (!TestTrue(TEXT("squad, controller and hints"), S.Init())) { return false; }
		S.Hints->TimeoutSeconds = 2.f;
		int32 Done = -1, Total = -1;
		S.Hints->GetEssentialsProgress(Done, Total);
		TestTrue(TEXT("0 / 4 essentials at the start"), Done == 0 && Total == 4);
		S.WaveStarts(1);

		// Fly ignored: it comes back after the others.
		TestEqual(TEXT("fly first"), S.WaitForHint(), ETDHint::Fly);
		TestEqual(TEXT("showing"), S.Hints->GetCurrentHintState(), ETDHintState::Showing);
		S.W.Advance(2.1f);
		TestFalse(TEXT("missed once: not done"), S.Hints->IsHintDone(ETDHint::Fly));

		// Fire done by the player: ticked off for a moment, then the next one.
		TestEqual(TEXT("fire next"), S.WaitForHint(), ETDHint::Fire);
		// T2's card: key glyph, action words and the essentials step.
		TestEqual(TEXT("card key"), UTDHintComponent::GetHintKey(S.W.GetWorld()).ToString(), FString(TEXT("LMB")));
		TestEqual(TEXT("card action"), UTDHintComponent::GetHintAction(S.W.GetWorld()).ToString(), FString(TEXT("FIRE")));
		int32 Step = 0, Count = 0;
		TestTrue(TEXT("fire is an essentials step"), UTDHintComponent::GetHintStep(S.W.GetWorld(), Step, Count));
		TestTrue(TEXT("step 2 of 4"), Step == 2 && Count == 4);
		TestEqual(TEXT("card state"), UTDHintComponent::GetHintState(S.W.GetWorld()), ETDHintState::Showing);
		for (int32 i = (int32)ETDHint::Fly; i <= (int32)ETDHint::Ability; ++i)
		{
			for (const bool bPad : { false, true })
			{
				TestFalse(TEXT("every hint has a key"), UTDHintComponent::KeyFor((ETDHint)i, bPad).IsEmpty());
				TestTrue(TEXT("key + action fit the card (<= 36)"), UTDHintComponent::KeyFor((ETDHint)i, bPad).ToString().Len()
					+ UTDHintComponent::ActionFor((ETDHint)i, ETDAbilityId::GravityWell).ToString().Len() + 2 <= 36);
			}
		}
		S.Press(EKeys::LeftMouseButton, 0.1f);
		TestEqual(TEXT("fire ticked off: state Done"), S.Hints->GetCurrentHintState(), ETDHintState::Done);
		TestEqual(TEXT("still the fire hint on screen"), S.Hints->GetCurrentHint(), ETDHint::Fire);
		TestEqual(TEXT("the HUD helper agrees"), UTDHintComponent::GetHintStateFor(S.W.GetWorld()), ETDHintState::Done);
		UTDHintComponent::GetEssentialsProgressFor(S.W.GetWorld(), Done, Total);
		TestEqual(TEXT("1 / 4 essentials"), Done, 1);
		S.W.Advance(S.Hints->DoneBeatSeconds + 0.05f);
		TestTrue(TEXT("the beat ends"), S.Hints->GetCurrentHintState() != ETDHintState::Done);

		// The rest done quickly; then Fly's second chance comes up, and a second miss counts it as seen.
		S.Press(EKeys::Two, 0.05f);
		S.W.Advance(0.5f);
		S.Press(EKeys::LeftShift, 0.4f);
		S.W.Advance(1.f); // a dodge needs the boost to end first (and comes before Q: Lancer's ram blocks it)
		S.Press(EKeys::LeftAlt, 0.05f);
		S.W.Tick();
		S.W.Advance(0.5f);
		S.Press(EKeys::Q, 0.05f);
		S.Press(EKeys::T, 0.05f);
		TestTrue(TEXT("everything but fly done by the player"), S.Hints->IsHintDone(ETDHint::Dodge) && S.Hints->IsHintDone(ETDHint::TargetCycle));
		const ETDHint Again = S.WaitForHint(6.f);
		TestEqual(FString::Printf(TEXT("fly again: the second chance (showing %s, state %d, done fire %d switch %d power %d boost %d dodge %d target %d)"),
			*StaticEnum<ETDHint>()->GetNameStringByValue((int64)S.Hints->GetCurrentHint()), (int32)S.Hints->GetCurrentHintState(),
			S.Hints->IsHintDone(ETDHint::Fire), S.Hints->IsHintDone(ETDHint::Switch), S.Hints->IsHintDone(ETDHint::Power),
			S.Hints->IsHintDone(ETDHint::Boost), S.Hints->IsHintDone(ETDHint::Dodge), S.Hints->IsHintDone(ETDHint::TargetCycle)), Again, ETDHint::Fly);
		S.W.Advance(2.1f);
		TestTrue(TEXT("missed twice: counts as seen"), S.Hints->IsHintDone(ETDHint::Fly));
		TestEqual(TEXT("nothing left"), S.WaitForHint(3.f), ETDHint::None);
	}
	{
		// Continue: no control lessons; contextual hints still come.
		TDHintTest::FSquadWorld S(UTDGameInstance::StaticClass());
		if (!TestTrue(TEXT("squad, controller and hints (TD game instance)"), S.Init())) { return false; }
		UTDGameInstance* GI = Cast<UTDGameInstance>(S.W.GetGameInstance());
		if (!TestNotNull(TEXT("TD game instance"), GI)) { return false; }
		GI->SetRunFromContinueForTests(true);
		S.WaveStarts(1);
		TestEqual(TEXT("a Continue run: no control lesson"), S.WaitForHint(4.f), ETDHint::None);
		if (UTDEventBus* Bus = UTDEventBus::Get(S.W.GetWorld())) { Bus->BroadcastTipRequested(TDTipIds::Drift); }
		TestEqual(TEXT("contextual hints still come"), S.WaitForHint(), ETDHint::Drift);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHintsWaitForTransmission, "TriadDrift.Input.Hints.WaitWhileAStoryLineIsOnAir", TD_TEST_FLAGS)
bool FTDHintsWaitForTransmission::RunTest(const FString& Parameters)
{
	// T2's HUD holds the tutorial card back while a story transmission is on air; the hint waits with its clock stopped.
	TDHintTest::FSquadWorld S;
	if (!TestTrue(TEXT("squad, controller and hints"), S.Init())) { return false; }
	S.WaveStarts(1);
	TestEqual(TEXT("fly"), S.WaitForHint(), ETDHint::Fly);
	S.Hints->TimeoutSeconds = 3.f; // a hint left showing this long would be missed: proves the clock stops on air
	TestTrue(TEXT("a story line"), UTDStorySubsystem::Fire(S.W.GetWorld(), TEXT("A2_DeviceFound")));
	S.W.Tick();
	TestTrue(TEXT("on air"), UTDStorySubsystem::GetCurrentTransmissionFor(S.W.GetWorld()).bActive);
	TestEqual(TEXT("no hint while it plays"), S.Hints->GetCurrentHint(), ETDHint::None);
	S.W.Advance(2.5f);
	TestFalse(TEXT("the hint's clock stopped: not missed"), S.Hints->WasMissed(ETDHint::Fly));
	for (float T = 0.f; T < 8.f && UTDStorySubsystem::GetCurrentTransmissionFor(S.W.GetWorld()).bActive; T += 1.f / 60.f) { S.W.Tick(); }
	S.W.Tick();
	TestFalse(TEXT("the line is over"), UTDStorySubsystem::GetCurrentTransmissionFor(S.W.GetWorld()).bActive);
	TestEqual(TEXT("the same hint is back"), S.Hints->GetCurrentHint(), ETDHint::Fly);

	// Lines back to back never hold a hint forever: after MaxStoryHoldSeconds it shows under them.
	for (const TCHAR* Id : { TEXT("A1_TheySearch"), TEXT("A1_TrailToVerdance"), TEXT("A2_Voices"), TEXT("A2_TrailToCinder"),
		TEXT("A3_CrystalsFound"), TEXT("A3_PassageOpen") })
	{
		UTDStorySubsystem::Fire(S.W.GetWorld(), Id);
	}
	for (float T = 0.f; T < 2.f && !UTDStorySubsystem::GetCurrentTransmissionFor(S.W.GetWorld()).bActive; T += 1.f / 60.f) { S.W.Tick(); }
	TestEqual(TEXT("held by the first line"), S.Hints->GetCurrentHint(), ETDHint::None);
	float Waited = 0.f;
	for (; Waited < 20.f && S.Hints->GetCurrentHint() == ETDHint::None; Waited += 1.f / 60.f) { S.W.Tick(); }
	TestTrue(FString::Printf(TEXT("the hint shows within the cap (%.1f s of %.1f)"), Waited, S.Hints->MaxStoryHoldSeconds),
		S.Hints->GetCurrentHint() == ETDHint::Fly && Waited <= S.Hints->MaxStoryHoldSeconds + 1.5f); // + the gaps between lines
	TestTrue(TEXT("while the run of lines goes on"), UTDStorySubsystem::GetCurrentTransmissionFor(S.W.GetWorld()).bActive
		|| UTDStorySubsystem::Get(S.W.GetWorld())->GetQueueLength() > 0);
	return true;
}

#endif

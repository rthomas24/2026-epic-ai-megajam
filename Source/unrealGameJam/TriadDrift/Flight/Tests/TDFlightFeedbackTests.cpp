// No-art flight feedback (placeholder kicks, bursts, speed dust). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.Feedback

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "NiagaraSystem.h"

namespace TDFeedbackTest
{
	static ATDShip* SpawnFlying(FTDTestWorld& W, bool bPlayer)
	{
		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		Ship->SetShipVisuals(NewObject<UTDShipVisuals>()); // every slot empty: placeholders stand in
		Ship->SetPlayerControlled(bPlayer);
		W.Tick();
		return Ship;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFeedbackDodge, "TriadDrift.Flight.Feedback.DodgeBurstAndKickWithNoArt", TD_TEST_FLAGS)
bool FTDFeedbackDodge::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDFeedbackTest::SpawnFlying(W, true);
	UTDFlightFeedbackComponent* FB = Ship->GetFlightFeedback();
	if (!TestNotNull(TEXT("feedback component"), FB)) { return false; }
	TestEqual(TEXT("nothing yet"), FB->GetActiveBurstCount(), 0);
	TestTrue(TEXT("dodge right"), Ship->TryDodge(1.f));
	W.Advance(0.08f);
	TestTrue(TEXT("a thruster burst glows"), FB->GetActiveBurstCount() > 0);
	TestTrue(TEXT("the camera rolls into the move"), FB->GetCameraOffset().Roll > 1.f);
	TestTrue(TEXT("and the view punches wider"), FB->GetFOVOffset() > 0.5f);
	TestEqual(TEXT("counted once"), FB->GetPlaceholderCount(TEXT("Dodge")), 1);
	TInlineComponentArray<UCameraComponent*> Cameras(Ship);
	UCameraComponent* const* Chase = Cameras.FindByPredicate([](const UCameraComponent* C) { return C->GetFName() == TEXT("ChaseCamera"); });
	TestTrue(TEXT("the chase camera carries the roll"), Chase && (*Chase)->GetRelativeRotation().Roll > 0.5f);
	W.Advance(0.8f);
	TestEqual(TEXT("the burst is gone"), FB->GetActiveBurstCount(), 0);
	TestTrue(TEXT("the kick decays to nothing"), FMath::Abs(FB->GetCameraOffset().Roll) < 0.2f && FMath::Abs(FB->GetFOVOffset()) < 0.2f);

	// Left dodge after the cooldown: the roll goes the other way.
	W.Advance(1.f);
	TestTrue(TEXT("dodge left"), Ship->TryDodge(-1.f));
	W.Advance(0.08f);
	TestTrue(TEXT("rolls left"), FB->GetCameraOffset().Roll < -1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFeedbackArtWins, "TriadDrift.Flight.Feedback.ArtReplacesThePlaceholder", TD_TEST_FLAGS)
bool FTDFeedbackArtWins::RunTest(const FString& Parameters)
{
	FTDFXCue Empty;
	FTDFXCue WithShake;
	WithShake.Shake = TSoftClassPtr<UCameraShakeBase>(UCameraShakeBase::StaticClass());
	FTDFXCue WithVFX;
	WithVFX.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/TriadDrift/VFX/NS_Anything.NS_Anything")));
	TestTrue(TEXT("empty cue: glow placeholder"), UTDFlightFeedbackComponent::WantsVisualPlaceholder(&Empty));
	TestTrue(TEXT("empty cue: camera placeholder"), UTDFlightFeedbackComponent::WantsCameraPlaceholder(&Empty));
	TestFalse(TEXT("VFX assigned: no glow"), UTDFlightFeedbackComponent::WantsVisualPlaceholder(&WithVFX));
	TestTrue(TEXT("VFX only: camera kick still stands in"), UTDFlightFeedbackComponent::WantsCameraPlaceholder(&WithVFX));
	TestFalse(TEXT("shake assigned: no placeholder kick"), UTDFlightFeedbackComponent::WantsCameraPlaceholder(&WithShake));

	// In the ship: a Dodge cue with a real shake gets no placeholder kick (the glow still shows).
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	UTDShipVisuals* Visuals = NewObject<UTDShipVisuals>();
	Visuals->Dodge = WithShake;
	Ship->SetShipVisuals(Visuals);
	Ship->SetPlayerControlled(true);
	W.Tick();
	TestTrue(TEXT("dodge"), Ship->TryDodge(1.f));
	W.Advance(0.08f);
	UTDFlightFeedbackComponent* FB = Ship->GetFlightFeedback();
	TestTrue(TEXT("art shake: no placeholder roll"), FMath::Abs(FB->GetCameraOffset().Roll) < 0.01f);
	TestTrue(TEXT("no VFX yet: the glow stands in"), FB->GetActiveBurstCount() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFeedbackMoments, "TriadDrift.Flight.Feedback.BoostBrakeHyperdriveRead", TD_TEST_FLAGS)
bool FTDFeedbackMoments::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDFeedbackTest::SpawnFlying(W, true);
	UTDFlightFeedbackComponent* FB = Ship->GetFlightFeedback();
	FTDFlightInput In;
	In.Throttle = 1.f;
	Ship->SetFlightInput(In);
	W.Advance(1.5f);
	In.bBoost = true;
	Ship->SetFlightInput(In);
	W.Advance(0.1f);
	TestEqual(TEXT("boost start fires its placeholder"), FB->GetPlaceholderCount(TEXT("BoostStart")), 1);
	TestTrue(TEXT("boost: FOV punch"), FB->GetFOVOffset() > 1.f);
	W.Advance(1.f);
	In.bBoost = false;
	In.bBrake = true;
	Ship->SetFlightInput(In);
	W.Advance(0.1f);
	TestEqual(TEXT("brake has a placeholder (it has no cue slot)"), FB->GetPlaceholderCount(TEXT("Brake")), 1);
	TestTrue(TEXT("brake: the view dips"), FB->GetCameraOffset().Pitch < -0.2f);
	In.bBrake = false;
	Ship->SetFlightInput(In);
	W.Advance(1.f);

	UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
	W.Advance(3.f); // cooldowns
	TestTrue(TEXT("spool"), Hyper->RequestSpoolFromPilot());
	W.Advance(0.8f);
	TestEqual(TEXT("spooling"), Hyper->GetState(), ETDHyperdriveState::Spooling);
	TestTrue(TEXT("spool: the view tightens"), FB->GetFOVOffset() < -0.5f);
	W.Advance(3.f);
	TestTrue(TEXT("the jump has its kick"), FB->GetPlaceholderCount(TEXT("HyperdriveJump")) >= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFeedbackDust, "TriadDrift.Flight.Feedback.SpeedDustOnlyForThePlayerAtSpeed", TD_TEST_FLAGS)
bool FTDFeedbackDust::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Wingman = TDFeedbackTest::SpawnFlying(W, false);
	ATDShip* Ship = TDFeedbackTest::SpawnFlying(W, true);
	FTDFlightInput In;
	In.Throttle = 1.f;
	Ship->SetFlightInput(In);
	Wingman->SetFlightInput(In);
	W.Advance(2.f);
	UTDFlightFeedbackComponent* FB = Ship->GetFlightFeedback();
	TestEqual(TEXT("the player's ship streaks dust"), FB->GetVisibleDustCount(), FB->Tuning.DustCount);
	TestEqual(TEXT("wingmen draw none"), Wingman->GetFlightFeedback()->GetVisibleDustCount(), 0);
	TestTrue(TEXT("frame cost under 0.3 ms"), FB->GetAverageTickMs() < 0.3f);
	AddInfo(FString::Printf(TEXT("feedback tick (dust on): %.4f ms"), FB->GetAverageTickMs()));
	// Every mote stays inside the box around the ship as it flies on.
	W.Advance(2.f);
	Ship->SetPlayerControlled(false);
	W.Tick();
	TestEqual(TEXT("not flown: dust hidden"), FB->GetVisibleDustCount(), 0);
	return true;
}

#endif

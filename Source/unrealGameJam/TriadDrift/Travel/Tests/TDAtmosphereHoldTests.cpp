// T3's live walk (the Lead, Nov 13): boosting into Verdance's atmosphere during the Aftermath sequence must not travel;
// it commits when the sequence ends. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Atmosphere.Hold

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDT1AtmoHold
{
	// A story sequence that exists on every branch stands in for Aftermath (T3's; the default in HoldEntryForSequences).
	static const FName Seq(TEXT("HangarArrival"));

	ATDShip* Setup(FTDTestWorld& W, UTDAtmosphereEntryComponent*& OutEntry)
	{
		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		if (!Ship) { return nullptr; }
		Ship->SetPlayerControlled(true);
		Ship->TeleportSpacecraft(FVector::ZeroVector, FRotator(0.f, 180.f, 0.f), false); // facing the planet
		Ship->GetShipMovement()->SetSpeedOverrideFor(TEXT("TDT1HoldTest"), 0.f, 0.f);     // hold still (not a sequence key)
		OutEntry = Ship->GetAtmosphereEntry();
		if (OutEntry) { OutEntry->HoldEntryForSequences = { Seq }; }
		W.Advance(0.3f);
		return Ship;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1AtmoHoldCommitsAfter, "TriadDrift.Travel.Atmosphere.HoldDuringAMustHearSequence", TD_TEST_FLAGS)
bool FTDT1AtmoHoldCommitsAfter::RunTest(const FString& Parameters)
{
	using namespace TDT1AtmoHold;
	{
		FTDTestWorld W;
		UTDAtmosphereEntryComponent* Entry = nullptr;
		ATDShip* Ship = Setup(W, Entry);
		UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
		if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("entry"), Entry) || !TestNotNull(TEXT("story"), Story)) { return false; }
		TestTrue(TEXT("the sequence plays"), UTDStorySubsystem::PlaySequence(W.GetWorld(), Seq) && Story->IsSequencePlaying(Seq));

		TestTrue(TEXT("the planet's trigger is taken (no fallback travel)"), Entry->BeginEntry(ETDDestination::Verdance, TEXT("Verdance_Orbit"), FVector(-200000.f, 0.f, 0.f)));
		TestTrue(TEXT("held for the story"), Entry->IsHoldingForStory());
		TestFalse(TEXT("not committed: no entry"), Entry->IsEntering());
		TestFalse(TEXT("no entry speed override: the ship flies freely"), Ship->GetShipMovement()->HasSpeedOverrideFor(UTDAtmosphereEntryComponent::OverrideOwner));
		TestEqual(TEXT("the HUD's travel line says AWAITING ORDERS"), Ship->GetHyperdrive()->GetBlockReasonText().ToString(), FString(TEXT("AWAITING ORDERS")));
		W.Advance(1.f);
		TestTrue(TEXT("still held a second later"), Entry->IsHoldingForStory() && !Entry->IsEntering());
		// T7's live play: the planet asks again as the ship grazes back in; still taken (no fallback travel), still held.
		TestTrue(TEXT("a second request during the hold is taken"), Entry->BeginEntry(ETDDestination::Verdance, TEXT("Verdance_Orbit"), FVector(-200000.f, 0.f, 0.f)));
		TestTrue(TEXT("... and stays a hold, not an entry"), Entry->IsHoldingForStory() && !Entry->IsEntering());
		TestTrue(TEXT("held: the ship crawls at the edge (not a silent wall, not into the planet)"), Ship->GetShipMovement()->HasSpeedOverrideFor(UTDAtmosphereEntryComponent::StoryHoldOverride));

		float Waited = 0.f;
		for (; Waited < 60.f && Story->IsSequencePlaying(Seq); Waited += 0.1f) { W.Advance(0.1f, 1.f / 30.f); }
		TestFalse(TEXT("the sequence ended"), Story->IsSequencePlaying(Seq));
		W.Advance(0.1f);
		AddInfo(FString::Printf(TEXT("sequence ran %.1f s"), Waited));
		TestTrue(TEXT("the sequence over: entry commits"), Entry->IsEntering());
		TestFalse(TEXT("... and the hold is gone"), Entry->IsHoldingForStory());
		TestFalse(TEXT("... with its crawl"), Ship->GetShipMovement()->HasSpeedOverrideFor(UTDAtmosphereEntryComponent::StoryHoldOverride));
	}
	{
		// Flying back out of the atmosphere during the hold drops it: nothing commits later.
		FTDTestWorld W;
		UTDAtmosphereEntryComponent* Entry = nullptr;
		ATDShip* Ship = Setup(W, Entry);
		UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
		if (!Ship || !Entry || !Story) { return false; }
		UTDStorySubsystem::PlaySequence(W.GetWorld(), Seq);
		Entry->BeginEntry(ETDDestination::Verdance, TEXT("Verdance_Orbit"), FVector(-200000.f, 0.f, 0.f));
		TestTrue(TEXT("held"), Entry->IsHoldingForStory());
		Ship->TeleportSpacecraft(FVector(Entry->HoldLeaveMargin + 20000.f, 0.f, 0.f), FRotator::ZeroRotator, false);
		W.Advance(0.2f);
		TestFalse(TEXT("left the atmosphere: hold dropped"), Entry->IsHoldingForStory());
		TestFalse(TEXT("... the crawl released"), Ship->GetShipMovement()->HasSpeedOverrideFor(UTDAtmosphereEntryComponent::StoryHoldOverride));
		for (int32 i = 0; i < 600 && Story->IsSequencePlaying(Seq); ++i) { W.Advance(0.1f, 1.f / 30.f); }
		W.Advance(0.2f);
		TestFalse(TEXT("no entry after the sequence"), Entry->IsEntering());
	}
	{
		// No sequence: the entry commits at once, as before.
		FTDTestWorld W;
		UTDAtmosphereEntryComponent* Entry = nullptr;
		ATDShip* Ship = Setup(W, Entry);
		if (!Ship || !Entry) { return false; }
		TestTrue(TEXT("no sequence: entry begins"), Entry->BeginEntry(ETDDestination::Verdance, TEXT("Verdance_Orbit"), FVector(-200000.f, 0.f, 0.f)));
		TestTrue(TEXT("committed at once"), Entry->IsEntering() && !Entry->IsHoldingForStory());
	}
	return true;
}

#endif

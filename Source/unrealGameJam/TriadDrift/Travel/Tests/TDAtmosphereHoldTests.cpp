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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1AtmoHoldTail, "TriadDrift.Travel.Atmosphere.HoldWaitsForTheLineOnAirThenCommits", TD_TEST_FLAGS)
bool FTDT1AtmoHoldTail::RunTest(const FString& Parameters)
{
	// T7's play: the sequence ended 2 s into A1_TrailToVerdance and the entry cut it. The hold now waits for the line on air
	// (or queued next) after the sequence, at most StoryTailCapSeconds.
	using namespace TDT1AtmoHold;
	FTDTestWorld W;
	UTDAtmosphereEntryComponent* Entry = nullptr;
	ATDShip* Ship = Setup(W, Entry);
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
	if (!Ship || !Entry || !TestNotNull(TEXT("story"), Story)) { return false; }
	Story->ResetPlayed();
	UTDStorySubsystem::PlaySequence(W.GetWorld(), Seq);
	Entry->BeginEntry(ETDDestination::Verdance, TEXT("Verdance_Orbit"), FVector(-200000.f, 0.f, 0.f));
	TestTrue(TEXT("held"), Entry->IsHoldingForStory());
	// The line that follows the sequence (as A1_TrailToVerdance does), queued while the sequence still plays.
	UTDStorySubsystem::Fire(W.GetWorld(), TEXT("A1_TrailToVerdance"));
	float T = 0.f, SeqEnd = -1.f, Commit = -1.f;
	for (; T < 60.f && Commit < 0.f; T += 0.1f)
	{
		W.Advance(0.1f, 1.f / 30.f);
		if (SeqEnd < 0.f && !Story->IsSequencePlaying(Seq)) { SeqEnd = T; }
		if (Commit < 0.f && Entry->IsEntering()) { Commit = T; }
	}
	const bool bA1Aired = Story->HasAired(TEXT("A1_TrailToVerdance"));
	AddInfo(FString::Printf(TEXT("sequence ended at %.1f s, entry committed at %.1f s (%.1f s later); A1 aired %d"), SeqEnd, Commit, Commit - SeqEnd, bA1Aired ? 1 : 0));
	TestTrue(TEXT("the sequence ended and the entry committed"), SeqEnd >= 0.f && Commit >= 0.f);
	TestTrue(TEXT("the line after the sequence aired before the commit"), bA1Aired);
	TestTrue(*FString::Printf(TEXT("not before the line's end (A1 is ~4.8 s): %.1f s after the sequence"), Commit - SeqEnd), Commit - SeqEnd >= 3.f);
	TestTrue(*FString::Printf(TEXT("never past the %.0f s cap (+ a tick): %.1f s"), Entry->StoryTailCapSeconds, Commit - SeqEnd), Commit - SeqEnd <= Entry->StoryTailCapSeconds + 0.3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAtmoHoldSurfaceSafety, "TriadDrift.Travel.Atmosphere.HoldKeepsHullOutsideAndReleases", TD_TEST_FLAGS)
bool FTDAtmoHoldSurfaceSafety::RunTest(const FString& Parameters)
{
	using namespace TDT1AtmoHold;
	// Exercise the old crawl and a high-speed override, not only Setup's stationary fixture.
	for (const float Speed : { 1500.f, 90000.f })
	{
		FTDTestWorld W;
		UTDAtmosphereEntryComponent* Entry = nullptr;
		ATDShip* Ship = Setup(W, Entry);
		if (!Ship || !Entry) { return false; }
		UTDShipMovementComponent* Move = Ship->GetShipMovement();
		Entry->StoryHoldSpeed = Speed;
		const FVector Center(-200000.f, 0.f, 0.f);
		const float SurfaceRadius = 199500.f; // only five metres ahead of the test ship
		const float SafeRadius = SurfaceRadius + Ship->GetRootComponent()->Bounds.SphereRadius + Entry->EntrySurfaceClearance;
		UTDStorySubsystem::PlaySequence(W.GetWorld(), Seq);
		TestTrue(TEXT("entry held"), Entry->BeginEntry(ETDDestination::Verdance, TEXT("Verdance_Orbit"), Center, SurfaceRadius));
		float MinRadius = TNumericLimits<float>::Max();
		for (int32 i = 0; i < 30 && Entry->IsHoldingForStory(); ++i)
		{
			W.Advance(0.1f, 1.f / 60.f);
			MinRadius = FMath::Min(MinRadius, (float)FVector::Dist(Ship->GetActorLocation(), Center));
		}
		TestTrue(*FString::Printf(TEXT("hold speed %.0f: hull stays outside surface (%.0f >= %.0f)"), Speed, MinRadius, SafeRadius), MinRadius >= SafeRadius - 2.f);
		Entry->AbortEntry();
		TestFalse(TEXT("abort releases story speed override"), Move->HasSpeedOverrideFor(UTDAtmosphereEntryComponent::StoryHoldOverride));
		// With no real planet in this fixture, the old hold must not push the ship out again after an abort.
		Ship->TeleportSpacecraft(FVector::ZeroVector, FRotator::ZeroRotator, false);
		W.Advance(0.1f);
		TestTrue(TEXT("abort releases radial protection"), FVector::Dist(Ship->GetActorLocation(), Center) < SafeRadius - 100.f);
	}
	return true;
}

#endif

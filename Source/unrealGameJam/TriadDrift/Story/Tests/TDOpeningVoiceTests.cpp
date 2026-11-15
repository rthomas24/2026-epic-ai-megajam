// #135: suppress the isolated opening recording without changing the story beat or shared finale master.
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/ScopeExit.h"
#include "Sound/SoundWave.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flow/Tests/TDFlowTestListener.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpeningVoicePolicy,
	"TriadDrift.Story.OpeningVoice.SuppressOnlyOpeningPlaybackKeepSharedMaster", TD_TEST_FLAGS)
bool FTDOpeningVoicePolicy::RunTest(const FString& Parameters)
{
	UTDTransmissionSet* Set = NewObject<UTDTransmissionSet>();
	Set->AddToRoot();
	const auto Previous = UTDStorySubsystem::SetOverrideForTests;
	ON_SCOPE_EXIT { UTDStorySubsystem::SetOverrideForTests = Previous; Set->RemoveFromRoot(); };
	USoundWave* Master = NewObject<USoundWave>(Set);
	Master->AddToRoot();
	ON_SCOPE_EXIT { Master->RemoveFromRoot(); };
	Master->Duration = 12.f;
	Set->SharedSounds.Add(TEXT("Dont"), TSoftObjectPtr<USoundBase>(Master));
	UTDStorySubsystem::SetOverrideForTests = Set;

	FTDTransmission Opening, Finale;
	if (!TestTrue(TEXT("both story beats resolve"), UTDStorySubsystem::FindEntry(TEXT("O_Unknown_Dont"), Opening)
		&& UTDStorySubsystem::FindEntry(TEXT("F_FutureWren_Dont"), Finale))) { return false; }
	TestTrue(TEXT("source master retained for both beats"), Opening.Sound.Get() == Master && Finale.Sound.Get() == Master);
	TestEqual(TEXT("shared source key retained"), Opening.SharedSound, Finale.SharedSound);
	TestFalse(TEXT("opening voice suppressed"), UTDStorySubsystem::ShouldPlayVoice(Opening.Id));
	TestTrue(TEXT("finale master remains eligible"), UTDStorySubsystem::ShouldPlayVoice(Finale.Id));
	TestEqual(TEXT("source-derived duration retained"), UTDStorySubsystem::SecondsFor(Opening), 12.3f);
	for (const FTDTransmission& Entry : UTDStorySubsystem::GetAllEntries())
	{
		if (Entry.Id != Opening.Id)
		{
			TestTrue(FString::Printf(TEXT("%s playback policy unchanged"), *Entry.Id.ToString()),
				UTDStorySubsystem::ShouldPlayVoice(Entry.Id));
		}
	}
	// A direct per-line override must not bypass the policy or change the shared finale master.
	USoundWave* Direct = NewObject<USoundWave>(Set);
	Direct->AddToRoot();
	ON_SCOPE_EXIT { Direct->RemoveFromRoot(); };
	FTDTransmission Override;
	Override.Id = Opening.Id;
	Override.Sound = TSoftObjectPtr<USoundBase>(Direct);
	Set->Entries.Add(Override);
	UTDStorySubsystem::FindEntry(Opening.Id, Opening);
	UTDStorySubsystem::FindEntry(Finale.Id, Finale);
	TestTrue(TEXT("direct source retained"), Opening.Sound.Get() == Direct);
	TestFalse(TEXT("direct override remains silent in opening"), UTDStorySubsystem::ShouldPlayVoice(Opening.Id));
	TestTrue(TEXT("finale source unaffected by opening override"), Finale.Sound.Get() == Master);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpeningSubtitleFlow,
	"TriadDrift.Story.OpeningVoice.CaptionTriggerDurationAndFollowingLineRemain", TD_TEST_FLAGS)
bool FTDOpeningSubtitleFlow::RunTest(const FString& Parameters)
{
	UTDTransmissionSet* Set = NewObject<UTDTransmissionSet>();
	Set->AddToRoot();
	const auto Previous = UTDStorySubsystem::SetOverrideForTests;
	ON_SCOPE_EXIT { UTDStorySubsystem::SetOverrideForTests = Previous; Set->RemoveFromRoot(); };
	FTDTransmission Override;
	Override.Id = TEXT("O_Unknown_Dont");
	Override.Duration = 3.f;
	USoundWave* Master = NewObject<USoundWave>(Set);
	Master->Duration = 1.f;
	Master->AddToRoot();
	ON_SCOPE_EXIT { Master->RemoveFromRoot(); };
	Override.Sound = TSoftObjectPtr<USoundBase>(Master);
	Set->Entries.Add(Override);
	UTDStorySubsystem::SetOverrideForTests = Set;
	FTDTestWorld World;
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(World.GetWorld());
	UTDEventBus* Bus = UTDEventBus::Get(World.GetWorld());
	if (!TestNotNull(TEXT("story"), Story) || !TestNotNull(TEXT("event bus"), Bus)) { return false; }
	UTDFlowTestListener* Listener = NewObject<UTDFlowTestListener>();
	Listener->AddToRoot();
	ON_SCOPE_EXIT { Listener->RemoveFromRoot(); };
	Listener->Bind(Bus);
	FTDTransmission Entry;
	UTDStorySubsystem::FindEntry(Override.Id, Entry);
	const float Duration = UTDStorySubsystem::SecondsFor(Entry);
	TestTrue(TEXT("opening trigger still accepted"), Story->FireTransmission(Entry.Id));
	TestTrue(TEXT("following beat queues"), Story->FireTransmission(TEXT("O_Wren_Break")));
	World.Tick();
	const FTDTransmissionView View = Story->GetCurrentTransmission();
	TestTrue(TEXT("opening caption airs"), View.bActive && View.Id == Entry.Id && View.Text.EqualTo(Entry.Text));
	TestTrue(TEXT("speaker and delivery retained"), View.Speaker == Entry.Speaker && View.Style == Entry.Style);
	TestTrue(TEXT("bark trigger retained exactly once"), Listener->Barks.Num() == 1 && Listener->Barks[0].Key == Entry.Id);
	TestTrue(TEXT("played tracking retained"), Story->HasPlayed(Entry.Id) && Story->HasAired(Entry.Id));
	TestFalse(TEXT("once-only trigger still enforced"), Story->FireTransmission(Entry.Id));
	World.Advance(Duration - 0.2f);
	TestTrue(TEXT("caption retains its duration"), Story->GetCurrentTransmission().Id == Entry.Id);
	World.Advance(0.2f + Story->GapSeconds + 0.1f);
	TestEqual(TEXT("next beat still airs"), Story->GetCurrentTransmission().Id, FName(TEXT("O_Wren_Break")));
	return true;
}

#endif

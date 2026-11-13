// Triad Drift - story transmissions (storyline.md, the First Contact brief). Owner: Teammate 3.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/Tests/TDFlowTestListener.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "EngineUtils.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDStoryTable, "TriadDrift.Story.Transmissions.EveryBeatHasAPlaceholderAndNoEarlyTwist", TD_TEST_FLAGS)
bool FTDStoryTable::RunTest(const FString& Parameters)
{
	const TCHAR* Required[] = {
		TEXT("A1_HailNoAnswer"), TEXT("A1_TheySearch"), TEXT("A2_DeviceFound"), TEXT("A2_Voices"), TEXT("A3_CrystalsFound"),
		TEXT("A3_EchoWren"), TEXT("A3_EchoKato"), TEXT("A3_EchoMoss"), TEXT("A3_PassageOpen"), TEXT("A4_GeneratorDown1"),
		TEXT("A4_GeneratorDown2"), TEXT("A4_GeneratorDown3"), TEXT("A4_GeneratorDown4"), TEXT("A4_DoorsKnowUs"),
		TEXT("A4_Recording_Hangar"), TEXT("A4_Recording_Corridor"), TEXT("A4_Recording_Barracks"), TEXT("A4_Recording_Core"),
		TEXT("A5_Choice"), TEXT("A5_Escape"), TEXT("A5_DistressSignal"),
		TEXT("O_PatrolSeven"), TEXT("O_Command_Orders"), TEXT("O_Wren_HowBad"), TEXT("O_Kato_StopSaying"), TEXT("O_Moss_Especially"),
		TEXT("O_Wren_BigShip"), TEXT("O_Kato_NoSignature"), TEXT("O_Moss_Message"), TEXT("O_Kato_Handshake"), TEXT("O_Wren_WhyNotFiring"),
		TEXT("O_Kato_Contacts"), TEXT("O_Command_Disengage"), TEXT("O_Unknown_Dont"), TEXT("O_Wren_Break"), TEXT("O_Command_Evade"),
		TEXT("F_FutureKato_Signatures"), TEXT("F_FutureMoss_Where"), TEXT("F_FutureWren_Channel"), TEXT("F_FutureKato_Interference"),
		TEXT("F_FutureWren_Dont"), TEXT("Intro_N0"), TEXT("Intro_N4"),
		TEXT("S2_Moss_Accounted"), TEXT("S2_Kato_NotPursuing"), TEXT("S2_Command_Orders"), TEXT("S2_Kato_Recorders"),
		TEXT("TB_Kato_Marking"), TEXT("TB_Command_FallBack"), TEXT("TB_Kato_BreakingOff") };
	for (const TCHAR* Id : Required)
	{
		FTDTransmission E;
		TestTrue(FString::Printf(TEXT("%s exists with a line"), Id), UTDStorySubsystem::FindEntry(FName(Id), E) && !E.Text.IsEmpty());
	}
	// storyline.md §4 / §8: before act 4 nothing says "future", "time" or "us", and no future speaker talks.
	for (const FTDTransmission& E : UTDStorySubsystem::GetAllEntries())
	{
		const FString Id = E.Id.ToString();
		const bool bLate = Id.StartsWith(TEXT("A4_")) || Id.StartsWith(TEXT("A5_")) || Id.StartsWith(TEXT("F_"));
		if (bLate)
		{
			continue;
		}
		const FString Line = TEXT(" ") + E.Text.ToString().ToLower() + TEXT(" ");
		TestFalse(FString::Printf(TEXT("%s: no future speaker before act 4"), *Id),
			E.Speaker == ETDSpeaker::FutureWren || E.Speaker == ETDSpeaker::FutureKato || E.Speaker == ETDSpeaker::FutureMoss);
		TestFalse(FString::Printf(TEXT("%s: never says 'future' or 'time' before act 4"), *Id), Line.Contains(TEXT("future")) || Line.Contains(TEXT(" time")));
		TestFalse(FString::Printf(TEXT("%s: never says 'us' as the enemy before act 4"), *Id), Line.Contains(TEXT("it's us")));
	}
	// Aftermath: the "Don't-" fragment is never used here (kept for the finale).
	for (const FTDTransmission& E : UTDStorySubsystem::GetAllEntries())
	{
		if (E.Id.ToString().StartsWith(TEXT("S2_")))
		{
			TestTrue(FString::Printf(TEXT("%s: no shared 'Dont' sound in Aftermath"), *E.Id.ToString()), E.SharedSound.IsNone());
		}
	}
	// The First Contact brief: the two "Don't-" lines are ONE sound.
	FTDTransmission Open, Fin;
	UTDStorySubsystem::FindEntry(TEXT("O_Unknown_Dont"), Open);
	UTDStorySubsystem::FindEntry(TEXT("F_FutureWren_Dont"), Fin);
	TestTrue(TEXT("both 'Don't-' lines share one sound slot"), !Open.SharedSound.IsNone() && Open.SharedSound == Fin.SharedSound);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDStoryFire, "TriadDrift.Story.Transmissions.OncePerRunQueuedHeldAndKeptAcrossRetakes", TD_TEST_FLAGS)
bool FTDStoryFire::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	if (!TestNotNull(TEXT("GI"), GI) || !TestNotNull(TEXT("story"), Story) || !TestNotNull(TEXT("bus"), Bus))
	{
		return false;
	}
	UTDFlowTestListener* L = NewObject<UTDFlowTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	L->Bind(Bus);
	GI->NotifyLevelEntered(ETDDestination::Galaxy); // a checkpoint

	// Once per run.
	TestTrue(TEXT("fires"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("A1_HailNoAnswer")));
	TestFalse(TEXT("not twice"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("A1_HailNoAnswer")));
	TestFalse(TEXT("unknown ids refused"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("No_Such_Line")));
	TestTrue(TEXT("a second line queues"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("A1_TheySearch")));
	W.Tick();
	FTDTransmissionView V = Story->GetCurrentTransmission();
	TestTrue(TEXT("on air: the first"), V.bActive && V.Id == FName(TEXT("A1_HailNoAnswer")));
	TestEqual(TEXT("speaker tag"), V.SpeakerTag.ToString(), FString(TEXT("KATO")));
	TestTrue(TEXT("the bark hook fired with the id and the speaker"), L->Barks.Num() == 1 && L->Barks[0].Key == FName(TEXT("A1_HailNoAnswer"))
		&& L->Barks[0].Value == FName(TEXT("Kato")));
	TestEqual(TEXT("the second waits"), Story->GetQueueLength(), 1);

	// Never overlapping: the second goes on air only after the first ends.
	FTDTransmission First;
	UTDStorySubsystem::FindEntry(TEXT("A1_HailNoAnswer"), First);
	W.Advance(UTDStorySubsystem::SecondsFor(First) - 0.3f, 1.f / 30.f);
	TestTrue(TEXT("still the first"), Story->GetCurrentTransmission().Id == FName(TEXT("A1_HailNoAnswer")));
	W.Advance(0.3f + Story->GapSeconds + 0.1f, 1.f / 30.f);
	TestTrue(TEXT("then the second"), Story->GetCurrentTransmission().Id == FName(TEXT("A1_TheySearch")));
	W.Advance(10.f, 1.f / 30.f);
	TestFalse(TEXT("then silence"), Story->GetCurrentTransmission().bActive);

	// Held during a money shot.
	AActor* Subject = W.Spawn<AActor>(FVector(4000.f, 0.f, 0.f));
	UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(W.GetWorld());
	TestTrue(TEXT("money shot"), Director && Director->ReportHighlight(ETDHighlightKind::ReactorKill, Subject, 1.f));
	TestTrue(TEXT("fires during the shot"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("A1_FieldUnreachable")));
	W.Tick();
	TestFalse(TEXT("held while the money shot runs"), Story->GetCurrentTransmission().bActive);
	W.Advance(5.f, 1.f / 30.f);
	TestTrue(TEXT("airs after it"), Story->GetCurrentTransmission().Id == FName(TEXT("A1_FieldUnreachable")) || L->Barks.Num() == 3);

	// Kept across a retake (the checkpoint restore) and in the checkpoint a Continue loads.
	GI->RestartSceneNow();
	W.Tick();
	TestTrue(TEXT("a retake keeps what played"), GI->HasPlayedTransmission(TEXT("A1_HailNoAnswer")) && GI->HasPlayedTransmission(TEXT("A1_FieldUnreachable")));
	TestFalse(TEXT("so it never replays"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("A1_HailNoAnswer")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDStoryOverride, "TriadDrift.Story.Transmissions.PresentationWinsFieldByFieldAndSharedSound", TD_TEST_FLAGS)
bool FTDStoryOverride::RunTest(const FString& Parameters)
{
	UTDTransmissionSet* Set = NewObject<UTDTransmissionSet>(GetTransientPackage());
	Set->AddToRoot();
	ON_SCOPE_EXIT { UTDStorySubsystem::SetOverrideForTests = nullptr; Set->RemoveFromRoot(); };
	FTDTransmission Hail;
	Hail.Id = TEXT("A1_HailNoAnswer");
	Hail.Speaker = ETDSpeaker::Wren;
	Hail.Text = FText::FromString(TEXT("Nicholas's line."));
	Set->Entries.Add(Hail);
	FTDTransmission SoundOnly;
	SoundOnly.Id = TEXT("A1_TheySearch");
	SoundOnly.Speaker = ETDSpeaker::Moss;
	SoundOnly.Sound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Engine/EngineSounds/WhiteNoise.WhiteNoise")));
	Set->Entries.Add(SoundOnly);
	FTDTransmission New;
	New.Id = TEXT("N_OnlyNicholas");
	New.Text = FText::FromString(TEXT("A beat only Nicholas defines."));
	New.bRepeatable = true;
	Set->Entries.Add(New);
	Set->SharedSounds.Add(TEXT("Dont"), TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Engine/EngineSounds/WhiteNoise.WhiteNoise"))));
	UTDStorySubsystem::SetOverrideForTests = Set;

	FTDTransmission E;
	TestTrue(TEXT("override text and speaker"), UTDStorySubsystem::FindEntry(TEXT("A1_HailNoAnswer"), E) && E.Text.ToString() == TEXT("Nicholas's line.")
		&& E.Speaker == ETDSpeaker::Wren);
	TestTrue(TEXT("a sound-only entry keeps the placeholder text"), UTDStorySubsystem::FindEntry(TEXT("A1_TheySearch"), E) && !E.Text.IsEmpty()
		&& !E.Sound.IsNull());
	TestTrue(TEXT("a beat only Nicholas defines"), UTDStorySubsystem::FindEntry(TEXT("N_OnlyNicholas"), E) && E.bRepeatable);
	FTDTransmission Open, Fin;
	UTDStorySubsystem::FindEntry(TEXT("O_Unknown_Dont"), Open);
	UTDStorySubsystem::FindEntry(TEXT("F_FutureWren_Dont"), Fin);
	TestTrue(TEXT("both 'Don't-' lines play the one shared sound"), !Open.Sound.IsNull() && Open.Sound == Fin.Sound);

	// Repeatable lines fire every time.
	FTDTestWorld W;
	TestTrue(TEXT("repeatable once"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("N_OnlyNicholas")));
	W.Advance(8.f, 1.f / 30.f);
	TestTrue(TEXT("repeatable again"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("N_OnlyNicholas")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDStoryTriggers, "TriadDrift.Story.Transmissions.ActOneWavesTraitsIntroAndEndingStinger", TD_TEST_FLAGS)
bool FTDStoryTriggers::RunTest(const FString& Parameters)
{
	{
		// Act 1 rides the waves: hail on wave 1, they search when it clears, the field on wave 2, Verdance after it.
		FTDTestWorld W;
		UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
		UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
		if (!TestNotNull(TEXT("story"), Story) || !TestNotNull(TEXT("bus"), Bus))
		{
			return false;
		}
		W.Tick();
		Bus->BroadcastWaveStarted(1, 0.2f);
		TestTrue(TEXT("wave 1: the hail"), Story->HasPlayed(TEXT("A1_HailNoAnswer")));
		Bus->BroadcastWaveCleared(1);
		TestTrue(TEXT("wave 1 cleared: they search"), Story->HasPlayed(TEXT("A1_TheySearch")));
		Bus->BroadcastWaveStarted(2, 0.3f);
		TestFalse(TEXT("wave 2: the field line is T4's (the field approach), not the wave's"), Story->HasPlayed(TEXT("A1_FieldUnreachable")));
		Bus->BroadcastWaveCleared(2);
		TestTrue(TEXT("wave 2 cleared: to Verdance"), Story->HasPlayed(TEXT("A1_TrailToVerdance")));
		TestFalse(TEXT("nothing else fires on its own"), Story->HasPlayed(TEXT("A2_DeviceFound")));
	}
	// The pilot traits on the name cards (storyline.md §3), and the intro's narration from the story table.
	TestTrue(TEXT("Wren is bold"), UTDStorySubsystem::PilotTrait(ETDShipId::Rift).ToString().Contains(TEXT("BOLD")));
	TestTrue(TEXT("the trait line starts with the role, never the ship"), UTDStorySubsystem::PilotTrait(ETDShipId::Bastion).ToString().StartsWith(TEXT("HEAVY"))
		&& !UTDStorySubsystem::PilotTrait(ETDShipId::Bastion).ToString().Contains(TEXT("BASTION")));
	TestTrue(TEXT("Kato is analytical"), UTDStorySubsystem::PilotTrait(ETDShipId::Lancer).ToString().Contains(TEXT("ANALYTICAL")));
	TestTrue(TEXT("Moss is steady"), UTDStorySubsystem::PilotTrait(ETDShipId::Bastion).ToString().Contains(TEXT("STEADY")));
	TestTrue(TEXT("the intro opens on the premise"),
		UTDStorySubsystem::GetText(TEXT("Intro_N0"), FText::GetEmpty()).ToString().Contains(TEXT("without warning")));
	{
		// The ending stinger on the victory screen: the distress signal, then black.
		FTDTestWorld W(nullptr, ATDMenuGameMode::StaticClass());
		W.SpawnLocalPlayerController();
		for (TActorIterator<ATDMenuBackdrop> It(W.GetWorld()); It; ++It) { It->Destroy(); }
		W.Tick();
		ATDMenuBackdrop* B = ATDMenuBackdrop::SpawnFor(W.GetWorld()->GetAuthGameMode(), ETDBackdropScene::Victory);
		if (!TestNotNull(TEXT("backdrop"), B))
		{
			return false;
		}
		W.Tick();
		UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
		if (B->GetDominion())
		{
			W.Advance(B->StingerSeconds - 1.f, 1.f / 30.f);
			TestFalse(TEXT("not before the breakup has played out"), B->HasPlayedStinger());
			W.Advance(1.5f, 1.f / 30.f);
			TestTrue(TEXT("the stinger"), B->HasPlayedStinger() && Story && Story->HasPlayed(TEXT("A5_DistressSignal")));
		}
		else
		{
			AddInfo(TEXT("no victory Dominion in this world: stinger checked through the menu-mode test"));
		}
	}
	return true;
}

#endif

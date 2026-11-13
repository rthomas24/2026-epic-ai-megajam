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
#include "TriadDrift/Flow/TDFlowSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDStoryTable, "TriadDrift.Story.Transmissions.EveryBeatHasAPlaceholderAndNoEarlyTwist", TD_TEST_FLAGS)
bool FTDStoryTable::RunTest(const FString& Parameters)
{
	const TCHAR* Required[] = {
		TEXT("A1_HailNoAnswer"), TEXT("A1_TheySearch"), TEXT("A2_DeviceFound"), TEXT("A2_Voices"), TEXT("A3_CrystalsFound"),
		TEXT("A3_PassageOpen"), TEXT("A4_GeneratorDown1"),
		TEXT("A4_GeneratorDown2"), TEXT("A4_GeneratorDown3"), TEXT("A4_GeneratorDown4"), TEXT("A4_DoorsKnowUs"),
		TEXT("A4_Recording_Hangar"), TEXT("A4_Recording_Corridor"), TEXT("A4_Recording_Barracks"), TEXT("A4_Recording_Core"),
		TEXT("A5_Choice"), TEXT("A5_Escape"), TEXT("A5_DistressSignal"),
		TEXT("O_PatrolSeven"), TEXT("O_Command_Orders"), TEXT("O_Wren_HowBad"), TEXT("O_Kato_StopSaying"), TEXT("O_Moss_Especially"),
		TEXT("O_Wren_BigShip"), TEXT("O_Kato_NoSignature"), TEXT("O_Moss_Message"), TEXT("O_Kato_Handshake"), TEXT("O_Wren_WhyNotFiring"),
		TEXT("O_Kato_Contacts"), TEXT("O_Command_Disengage"), TEXT("O_Unknown_Dont"), TEXT("O_Wren_Break"), TEXT("O_Command_Evade"),
		TEXT("F_FutureKato_Signatures"), TEXT("F_FutureMoss_Where"), TEXT("F_FutureWren_Channel"), TEXT("F_FutureKato_Interference"),
		TEXT("F_FutureWren_Dont"), TEXT("Intro_N0"), TEXT("Intro_N4"),
		TEXT("S2_Moss_Accounted"), TEXT("S2_Kato_NotPursuing"), TEXT("S2_Command_Orders"), TEXT("S2_Kato_Recorders"),
		TEXT("TB_Kato_Marking"), TEXT("TB_Command_FallBack"), TEXT("TB_Kato_BreakingOff"),
		TEXT("A2_ConvoyScanned"), TEXT("A2_TransponderSighted"), TEXT("G2_Command_Archive"), TEXT("C3_Kato_Artificial"),
		TEXT("C3_Wren_Answered"), TEXT("G3_TransferStart"), TEXT("G3_Disruption"), TEXT("G3_DataSecured"), TEXT("C4_Command_Proceed"), TEXT("C4_Wren_SetCourse"),
		TEXT("G4_Kato_Transports"), TEXT("C5_Kato_SameConstruction"), TEXT("C5_Unknown_SameConstruction"), TEXT("C5_Kato_Feedback"),
		TEXT("C5_Kato_OriginCore"), TEXT("G5_Wren_ClearRoute"), TEXT("C6_Command_FindOutWhy"), TEXT("C6_Wren_StayTogether"),
		TEXT("G6_Kato_MarkingTargets"), TEXT("G6_Command_ShieldCollapsed"), TEXT("G6_Clue1"), TEXT("G6_Clue2"),
		TEXT("C7_Moss_WayOut"), TEXT("C7_Kato_Layout"), TEXT("C7_Kato_Archive"), TEXT("C7_Wren_Record"), TEXT("G7_Moss_MyVoice"),
		TEXT("G7_Kato_Authorisations"), TEXT("G7_Wren_FullRecord"), TEXT("F_FutureWren_FullWarning"), TEXT("C8_RecKato_OneEvent"),
		TEXT("C8_RecWren_LastChance"), TEXT("C8_RecMoss_SaveYou"), TEXT("C8_Kato_NoSafeShutdown"), TEXT("C8_Moss_CantLetThem"),
		TEXT("C8_Wren_Order"), TEXT("G8_Wren_ConduitsFirst"), TEXT("G8_Kato_ConduitDown"), TEXT("G8_Moss_KeepMoving"),
		TEXT("G8_Kato_HeartExposed"), TEXT("G8_Command_ReactorLost"), TEXT("G9_Moss_HoldingWayOut"), TEXT("G9_Kato_HangarConfirmed"),
		TEXT("G9_Wren_PullAway"), TEXT("C9_Kato_Clear"), TEXT("C9_Moss_Clear"), TEXT("C9_Wren_Clear"), TEXT("C9_Moss_TakeThat"),
		TEXT("C9_Kato_Identifiers"), TEXT("C9_Wren_Source"), TEXT("C9_Kato_Decades") };
	for (const TCHAR* Gone : { TEXT("A3_EchoWren"), TEXT("A3_EchoKato"), TEXT("A3_EchoMoss") })
	{
		FTDTransmission E;
		TestFalse(FString::Printf(TEXT("%s is gone (one echo only, at the Cinder terminal)"), Gone), UTDStorySubsystem::FindEntry(FName(Gone), E));
	}
	for (const TCHAR* Id : Required)
	{
		FTDTransmission E;
		TestTrue(FString::Printf(TEXT("%s exists with a line"), Id), UTDStorySubsystem::FindEntry(FName(Id), E) && !E.Text.IsEmpty());
	}
	// The reveal (storyline.md §8, §25; Story/README.md): only aboard the Dominion does the squad learn who commands it.
	// After the reveal: the interior (A4_Recording_*, A4_DoorsKnowUs), the command deck (F_*), act 5 (A5_*) and any
	// space-only fallback reveal (A4F_*). Everything else, the assault included, is before it.
	const TCHAR* EarlierThanTerminal[] = { TEXT("Intro_"), TEXT("O_"), TEXT("TB_"), TEXT("S2_"), TEXT("A1_"), TEXT("A2_"), TEXT("G2_"),
		TEXT("C3_"), TEXT("G3_"), TEXT("C4_"), TEXT("G4_") };
	for (const FTDTransmission& E : UTDStorySubsystem::GetAllEntries())
	{
		const FString Id = E.Id.ToString();
		// C7_ (the boarding) is still before it; the interior climbs to it (G7_), then the record, the fight, the ending.
		const bool bAfterReveal = Id.StartsWith(TEXT("A4_Recording_")) || Id == TEXT("A4_DoorsKnowUs") || Id.StartsWith(TEXT("A4F_"))
			|| Id.StartsWith(TEXT("A5_")) || Id.StartsWith(TEXT("F_")) || Id.StartsWith(TEXT("G7_")) || Id.StartsWith(TEXT("C8_"))
			|| Id.StartsWith(TEXT("G8_")) || Id.StartsWith(TEXT("G9_")) || Id.StartsWith(TEXT("C9_"));
		if (bAfterReveal)
		{
			continue;
		}
		const FString Line = TEXT(" ") + E.Text.ToString().ToLower() + TEXT(" ");
		TestFalse(FString::Printf(TEXT("%s: no future speaker before the reveal"), *Id),
			E.Speaker == ETDSpeaker::FutureWren || E.Speaker == ETDSpeaker::FutureKato || E.Speaker == ETDSpeaker::FutureMoss);
		TestFalse(FString::Printf(TEXT("%s: never says 'future' or 'time' before the reveal"), *Id), Line.Contains(TEXT("future")) || Line.Contains(TEXT(" time")));
		TestFalse(FString::Printf(TEXT("%s: never says 'us' as the enemy before the reveal"), *Id), Line.Contains(TEXT("it's us")));
		for (const TCHAR* Word : { TEXT("older"), TEXT("ourselves"), TEXT("our own voice"), TEXT("years from") })
		{
			TestFalse(FString::Printf(TEXT("%s: never says '%s' before the reveal"), *Id, Word), Line.Contains(Word));
		}
		// The one approved hint (Nicholas, issue #29): Kato's "a temporal device" at the Cinder terminal, nowhere else.
		TestTrue(FString::Printf(TEXT("%s: 'temporal' only in Kato's terminal line"), *Id), !Line.Contains(TEXT("temporal")) || Id == TEXT("C5_Kato_OriginCore"));
		bool bEarly = false;
		for (const TCHAR* Prefix : EarlierThanTerminal)
		{
			bEarly = bEarly || Id.StartsWith(Prefix);
		}
		TestFalse(FString::Printf(TEXT("%s: the Origin Core is not named before the Cinder terminal"), *Id), bEarly && Line.Contains(TEXT("origin core")));
		// "Don't-" is reserved for the opening and the finale.
		TestTrue(FString::Printf(TEXT("%s: no shared 'Dont' sound outside the opening"), *Id), E.SharedSound.IsNone() || Id == TEXT("O_Unknown_Dont"));
	}
	// The identity climbs over the recordings: the first is an unidentifiable voice, the last names them (storyline.md 27).
	FTDTransmission First, Last, Full, Dont2, Signal;
	UTDStorySubsystem::FindEntry(TEXT("A4_Recording_Hangar"), First);
	UTDStorySubsystem::FindEntry(TEXT("A4_Recording_Core"), Last);
	TestTrue(TEXT("the hangar recording is an unknown voice"), First.Speaker == ETDSpeaker::Unknown);
	TestTrue(TEXT("the final record is a future voice"), Last.Speaker == ETDSpeaker::FutureKato || Last.Speaker == ETDSpeaker::FutureWren || Last.Speaker == ETDSpeaker::FutureMoss);
	// The full warning is new words: never the opening's "Don't-" sound, which plays only as itself.
	UTDStorySubsystem::FindEntry(TEXT("F_FutureWren_FullWarning"), Full);
	UTDStorySubsystem::FindEntry(TEXT("F_FutureWren_Dont"), Dont2);
	TestTrue(TEXT("the full warning has no shared sound"), Full.SharedSound.IsNone() && !Dont2.SharedSound.IsNone());
	// The stinger's date (Nicholas: "decades ahead", not the forty-one-year placeholder).
	UTDStorySubsystem::FindEntry(TEXT("A5_DistressSignal"), Signal);
	TestTrue(TEXT("the signal is dated decades ahead"), Signal.Text.ToString().Contains(TEXT("DECADES AHEAD")) && !Signal.Text.ToString().Contains(TEXT("FORTY")));
	// The terminal echo stays an echo: an unknown voice repeating words Kato has just said.
	FTDTransmission KatoLine, EchoLine;
	UTDStorySubsystem::FindEntry(TEXT("C5_Kato_SameConstruction"), KatoLine);
	UTDStorySubsystem::FindEntry(TEXT("C5_Unknown_SameConstruction"), EchoLine);
	TestTrue(TEXT("the echo is Unknown, Echo style, and repeats Kato's words"), EchoLine.Speaker == ETDSpeaker::Unknown
		&& EchoLine.Style == ETDTransmissionStyle::Echo && !EchoLine.Text.IsEmpty() && KatoLine.Text.ToString().Contains(EchoLine.Text.ToString()));
	// The assault is an access mission: nothing before docking orders the Dominion or its reactor destroyed (§23-24).
	for (const FTDTransmission& E : UTDStorySubsystem::GetAllEntries())
	{
		const FString Id = E.Id.ToString();
		if (Id.StartsWith(TEXT("C6_")) || Id.StartsWith(TEXT("G6_")) || Id.StartsWith(TEXT("A4_Generator")))
		{
			const FString Line = E.Text.ToString().ToLower();
			TestFalse(FString::Printf(TEXT("%s: no order to destroy the ship or reactor"), *Id),
				Line.Contains(TEXT("destroy the dominion")) || Line.Contains(TEXT("reactor")) || Line.Contains(TEXT("field is")));
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
			TestTrue(TEXT("Moss's 'I'll take that' before it"), Story && Story->HasPlayed(TEXT("C9_Moss_TakeThat")));
			TestTrue(TEXT("then the three lines at the signal"), Story && Story->HasPlayed(TEXT("C9_Kato_Identifiers"))
				&& Story->HasPlayed(TEXT("C9_Wren_Source")) && Story->HasPlayed(TEXT("C9_Kato_Decades")));
			TestTrue(TEXT("the caption held on the black says decades ahead"), B->GetStingerCaption().ToString().Contains(TEXT("DECADES")));
			TestEqual(TEXT("once"), B->PlayStinger(), 0.f);
		}
		else
		{
			AddInfo(TEXT("no victory Dominion in this world: stinger checked through the menu-mode test"));
		}
	}
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDStoryDepartures, "TriadDrift.Story.Transmissions.DepartureSetsOverTheTakeOff", TD_TEST_FLAGS)
bool FTDStoryDepartures::RunTest(const FString& Parameters)
{
	// Cinematics 04 and 06 are transmissions over the take-off: back in the galaxy after a finished planet, its set
	// plays once, in order; Cinematic 06's last two lines wait for wave 5. A planet half done plays nothing.
	struct FCase { bool bVerdance; bool bCinder; ETDDestination Expect; };
	for (const FCase& C : { FCase{ false, false, ETDDestination::Galaxy }, FCase{ true, false, ETDDestination::Verdance },
		FCase{ true, true, ETDDestination::Cinder } })
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
		if (!TestNotNull(TEXT("GI"), GI) || !TestNotNull(TEXT("story"), Story))
		{
			return false;
		}
		const UTDFlowSettings* Flow = UTDFlowSettings::Get();
		if (C.bVerdance) { for (const FName& Id : Flow->VerdanceObjectives) { GI->CompleteObjective(Id, ETDAbilityId::None, ETDShipId::Rift); } }
		else if (Flow->VerdanceObjectives.Num() > 0) { GI->CompleteObjective(Flow->VerdanceObjectives[0], ETDAbilityId::None, ETDShipId::Rift); } // half done
		if (C.bCinder) { for (const FName& Id : Flow->CinderObjectives) { GI->CompleteObjective(Id, ETDAbilityId::None, ETDShipId::Rift); } }
		GI->NotifyLevelEntered(ETDDestination::Galaxy);
		W.Advance(Story->DepartureDelaySeconds * 0.5f, 1.f / 30.f);
		TestFalse(TEXT("nothing during the ascent"), Story->GetCurrentTransmission().bActive);
		W.Advance(Story->DepartureDelaySeconds, 1.f / 30.f);
		const TArray<FName>& Set = UTDStorySubsystem::DepartureSet(C.Expect);
		const FString Case = C.bCinder ? TEXT("Cinder done") : (C.bVerdance ? TEXT("Verdance done") : TEXT("Verdance half done"));
		if (Set.Num() == 0)
		{
			TestFalse(Case + TEXT(": no departure"), Story->GetCurrentTransmission().bActive || Story->GetQueueLength() > 0);
			continue;
		}
		TestTrue(Case + TEXT(": the first line is on air"), Story->GetCurrentTransmission().Id == Set[0]);
		TestEqual(Case + TEXT(": the rest queued, in order"), Story->GetQueueLength(), Set.Num() - 1);
		if (C.bCinder)
		{
			TestFalse(TEXT("Cinder done: Verdance's departure is not replayed late"), Story->HasPlayed(TEXT("C4_Moss_Trouble")));
			if (UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld()))
			{
				Bus->BroadcastWaveStarted(5, 1.f);
			}
			TestEqual(TEXT("wave 5 adds the guard-line pair"), Story->GetQueueLength(), Set.Num() - 1 + 2);
		}
	}
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDStoryBoarder, "TriadDrift.Story.Transmissions.BoardingPilotSpeaksForAnyShip", TD_TEST_FLAGS)
bool FTDStoryBoarder::RunTest(const FString& Parameters)
{
	// Nicholas (issue #29): the pilot on foot is whoever flew the docked ship. Every on-foot line gets a real pilot.
	ON_SCOPE_EXIT { UTDStorySubsystem::BoardingShipResolver = nullptr; };
	int32 OnFoot = 0;
	for (const FTDTransmission& E : UTDStorySubsystem::GetAllEntries())
	{
		OnFoot += E.Speaker == ETDSpeaker::Boarder ? 1 : 0;
	}
	TestTrue(TEXT("the on-foot lines are keyed to the boarding pilot"), OnFoot >= 5);
	const TPair<ETDShipId, ETDSpeaker> Expect[] = { { ETDShipId::Rift, ETDSpeaker::Wren }, { ETDShipId::Lancer, ETDSpeaker::Kato },
		{ ETDShipId::Bastion, ETDSpeaker::Moss } };
	for (const TPair<ETDShipId, ETDSpeaker>& P : Expect)
	{
		const ETDShipId Ship = P.Key;
		UTDStorySubsystem::BoardingShipResolver = [Ship](const UObject*) { return Ship; };
		for (const FTDTransmission& E : UTDStorySubsystem::GetAllEntries())
		{
			if (E.Speaker == ETDSpeaker::Boarder)
			{
				TestEqual(FString::Printf(TEXT("%s spoken by the pilot of ship %d"), *E.Id.ToString(), (int32)Ship),
					UTDStorySubsystem::ResolveSpeaker(nullptr, E.Speaker), P.Value);
			}
		}
	}
	// Nothing else is changed by the resolver.
	TestEqual(TEXT("Kato stays Kato"), UTDStorySubsystem::ResolveSpeaker(nullptr, ETDSpeaker::Kato), ETDSpeaker::Kato);

	// On air, the tag is the pilot's, not "BOARDING PILOT".
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDStorySubsystem::BoardingShipResolver = [](const UObject*) { return ETDShipId::Bastion; };
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
	TestTrue(TEXT("fires"), UTDStorySubsystem::Fire(W.GetWorld(), TEXT("C8_Wren_Order")));
	W.Tick();
	TestTrue(TEXT("Moss gives the order when Moss boarded"), Story && Story->GetCurrentTransmission().Speaker == ETDSpeaker::Moss
		&& Story->GetCurrentTransmission().SpeakerTag.ToString() == TEXT("MOSS"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDStorySequence, "TriadDrift.Story.Transmissions.Cinematic08PlaysInOrderAndSkips", TD_TEST_FLAGS)
bool FTDStorySequence::RunTest(const FString& Parameters)
{
	// T5's threshold console plays Cinematic 08 and opens the reactor door when it ends or is skipped. The five command
	// deck lines (the "Dont" payoff) finally play in the game, in order, then the full warning and the case.
	const TArray<FName>& Ids = UTDStorySubsystem::SequenceIds(TEXT("Cinematic08"));
	TestTrue(TEXT("starts with the command deck and ends with the order"), Ids.Num() >= 10 && Ids[0] == FName(TEXT("F_FutureKato_Signatures"))
		&& Ids.Contains(FName(TEXT("F_FutureWren_Dont"))) && Ids.Contains(FName(TEXT("F_FutureWren_FullWarning"))));
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
		TestTrue(TEXT("plays"), UTDStorySubsystem::PlaySequence(W.GetWorld(), TEXT("Cinematic08")));
		W.Tick();
		TestTrue(TEXT("the first line on air"), Story->GetCurrentTransmission().Id == Ids[0] && Story->IsSequencePlaying(TEXT("Cinematic08")));
		TArray<FName> Heard;
		for (float T = 0.f; T < 180.f && Story->IsSequencePlaying(); T += 1.f / 20.f)
		{
			const FName Now = Story->GetCurrentTransmission().Id;
			if (Story->GetCurrentTransmission().bActive && (Heard.Num() == 0 || Heard.Last() != Now))
			{
				Heard.Add(Now);
			}
			W.Tick(1.f / 20.f);
		}
		TestFalse(TEXT("it ends"), Story->IsSequencePlaying());
		TestTrue(TEXT("every line, in order"), Heard == Ids);
	}
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
		UTDStorySubsystem::PlaySequence(W.GetWorld(), TEXT("Cinematic08"));
		W.Advance(1.f, 1.f / 30.f);
		TestTrue(TEXT("skip"), Story->SkipSequence());
		TestFalse(TEXT("skipped: over at once"), Story->IsSequencePlaying());
		TestEqual(TEXT("nothing left queued"), Story->GetQueueLength(), 0);
		TestTrue(TEXT("its discoveries kept: the order counts as heard"), Story->HasPlayed(TEXT("C8_Wren_Order")));
		W.Advance(1.f, 1.f / 30.f);
		TestFalse(TEXT("nothing of it plays after the skip"), Story->GetCurrentTransmission().bActive);
		// A retake: the gate's sequence is already heard, so it finishes at once and the door opens.
		UTDStorySubsystem::PlaySequence(W.GetWorld(), TEXT("Cinematic08"));
		W.Tick();
		W.Tick();
		TestFalse(TEXT("a retake finishes at once"), Story->IsSequencePlaying());
	}
	return true;
}
#endif

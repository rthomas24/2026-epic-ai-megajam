// Triad Drift - the bark lines (dialogs.md §2, §10). Owner: Teammate 3.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/DataTable.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Squad/TDBarkLines.h"
#include "TriadDrift/Squad/TDSquadBarks.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/Tests/TDSquadTestListener.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDBarkLinesTest
{
	static UDataTable* MakeTable()
	{
		UDataTable* T = NewObject<UDataTable>(GetTransientPackage());
		T->RowStruct = FTDBarkRow::StaticStruct();
		return T;
	}

	static void Add(UDataTable* T, const TCHAR* Id, const TCHAR* Speaker, TArray<FString> Lines, ETDBarkPriority P = ETDBarkPriority::Normal)
	{
		FTDBarkRow Row;
		Row.Id = Id;
		Row.Speaker = Speaker;
		Row.Priority = P;
		Row.Lines = MoveTemp(Lines);
		T->AddRow(FName(*FString::Printf(TEXT("%s__%s"), Id, Speaker)), Row);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarkLinesPick, "TriadDrift.Squad.Barks.LinesRotateNicholasWinsAndNoRawIds", TD_TEST_FLAGS)
bool FTDBarkLinesPick::RunTest(const FString& Parameters)
{
	using namespace TDBarkLinesTest;
	UDataTable* Ours = MakeTable();
	UDataTable* Nicks = MakeTable();
	Ours->AddToRoot();
	Nicks->AddToRoot();
	ON_SCOPE_EXIT
	{
		FTDBarkLines::PresentationOverrideForTests = nullptr;
		FTDBarkLines::FallbackOverrideForTests = nullptr;
		FTDBarkLines::ResetForTests();
		Ours->RemoveFromRoot();
		Nicks->RemoveFromRoot();
	};
	Add(Ours, TEXT("Bark_SquadSwitchIn"), TEXT("Moss"), { TEXT("Bastion's yours."), TEXT("I'm with you."), TEXT("Third line.") });
	Add(Ours, TEXT("Bark_SquadSwitchIn"), TEXT("Kato"), { TEXT("Lancer, ready.") });
	Add(Ours, TEXT("Bark_SquadEngage<Dir>"), TEXT("Kato"), { TEXT("Contact {dir}. Engaging.") });
	Add(Nicks, TEXT("Bark_SquadSwitchIn"), TEXT("Kato"), { TEXT("Nicholas's Kato.") });
	FTDBarkLines::FallbackOverrideForTests = Ours;
	FTDBarkLines::PresentationOverrideForTests = Nicks;
	FTDBarkLines::ResetForTests();

	FTDTestWorld W;
	FText Line;
	// Rotation: never the same line twice in a row.
	FString Prev;
	TSet<FString> Seen;
	for (int32 i = 0; i < 6; ++i)
	{
		W.Advance(3.5f, 0.1f); // past the 3 s "same request" window
		TestTrue(TEXT("Moss has a line"), FTDBarkLines::PickLine(W.GetWorld(), TEXT("Bark_SquadSwitchIn"), TEXT("Moss"), Line));
		TestNotEqual(TEXT("never twice in a row"), Line.ToString(), Prev);
		Prev = Line.ToString();
		Seen.Add(Prev);
	}
	TestEqual(TEXT("every alternative gets used"), Seen.Num(), 3);
	// One pick per request: readers within 3 s see the same line.
	FText A, B;
	W.Advance(3.5f, 0.1f);
	FTDBarkLines::PickLine(W.GetWorld(), TEXT("Bark_SquadSwitchIn"), TEXT("Moss"), A);
	FTDBarkLines::PickLine(W.GetWorld(), TEXT("Bark_SquadSwitchIn"), TEXT("Moss"), B);
	TestTrue(TEXT("the HUD and the subtitles agree"), A.EqualTo(B));
	// Nicholas's row wins over ours.
	TestTrue(TEXT("Kato"), FTDBarkLines::PickLine(W.GetWorld(), TEXT("Bark_SquadSwitchIn"), TEXT("Kato"), Line));
	TestEqual(TEXT("Nicholas's row wins"), Line.ToString(), FString(TEXT("Nicholas's Kato.")));
	// Engage calls fill the direction.
	TestTrue(TEXT("engage"), FTDBarkLines::PickLine(W.GetWorld(), TEXT("Bark_SquadEngageHighLeft"), TEXT("Kato"), Line));
	TestEqual(TEXT("{dir} filled"), Line.ToString(), FString(TEXT("Contact high left. Engaging.")));
	// No line: nothing (never the raw id). The dev HUD asks through the resolver.
	TestFalse(TEXT("unknown id: no line"), FTDBarkLines::PickLine(W.GetWorld(), TEXT("Bark_Nothing"), TEXT("Moss"), Line));
	TestFalse(TEXT("a story transmission is not a bark"), FTDBarkLines::PickLine(W.GetWorld(), TEXT("A1_HailNoAnswer"), TEXT("Kato"), Line));
	TestNotNull(TEXT("the dev HUD's resolver is registered"), (void*)ATDDebugHUD::BarkLineResolver);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarkFlavourBanter, "TriadDrift.Squad.Barks.FlavourWaitsAndBanterPlaysOnce", TD_TEST_FLAGS)
bool FTDBarkFlavourBanter::RunTest(const FString& Parameters)
{
	using namespace TDBarkLinesTest;
	// Flavour needs quiet.
	FTDBarkTuning Tuning;
	FTDBarkLimiter L;
	TestTrue(TEXT("a normal line"), L.TryConsume(Tuning, TEXT("Bark_A"), ETDBarkPriority::Normal, 100.0));
	TestFalse(TEXT("flavour right after: no"), L.TryConsume(Tuning, TEXT("Bark_Flavour"), ETDBarkPriority::Flavour, 103.0));
	TestTrue(TEXT("flavour after the quiet"), L.TryConsume(Tuning, TEXT("Bark_Flavour"), ETDBarkPriority::Flavour, 100.0 + Tuning.FlavourQuietSeconds + 0.1));

	// Banter: one exchange per breather, the replies in order, never twice.
	UDataTable* Ours = MakeTable();
	Ours->AddToRoot();
	ON_SCOPE_EXIT
	{
		FTDBarkLines::FallbackOverrideForTests = nullptr;
		FTDBarkLines::PresentationOverrideForTests = nullptr;
		FTDBarkLines::ResetForTests();
		Ours->RemoveFromRoot();
	};
	Add(Ours, TEXT("Bark_Banter1_1"), TEXT("Wren"), { TEXT("Bet you ten.") }, ETDBarkPriority::Flavour);
	Add(Ours, TEXT("Bark_Banter1_2"), TEXT("Kato"), { TEXT("I do not bet.") }, ETDBarkPriority::Flavour);
	FTDBarkLines::FallbackOverrideForTests = Ours;
	TestEqual(TEXT("one exchange of two steps"), FTDBarkLines::GetBanters().Num() == 1 ? FTDBarkLines::GetBanters()[0].Num() : 0, 2);

	FTDTestWorld W;
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	UTDSquadTestListener* Events = NewObject<UTDSquadTestListener>();
	Events->AddToRoot();
	ON_SCOPE_EXIT { Events->StopListening(); Events->RemoveFromRoot(); };
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	Events->Listen(Bus);
	W.Advance(10.f, 0.1f); // quiet
	Bus->BroadcastWaveCleared(1);
	TestEqual(TEXT("the first line plays"), Events->CountBarks(TEXT("Bark_Banter1_1")), 1);
	TestEqual(TEXT("one reply to come"), Squad->GetPendingBanterSteps(), 1);
	W.Advance(3.f, 0.1f);
	TestEqual(TEXT("the reply"), Events->CountBarks(TEXT("Bark_Banter1_2")), 1);
	Bus->BroadcastWaveStarted(2, 0.3f);
	W.Advance(10.f, 0.1f);
	Bus->BroadcastWaveCleared(2);
	W.Advance(3.f, 0.1f);
	TestEqual(TEXT("never twice"), Events->CountBarks(TEXT("Bark_Banter1_1")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarkTableFromDialogs, "TriadDrift.Squad.Barks.FallbackTableFromDialogs", TD_TEST_FLAGS)
bool FTDBarkTableFromDialogs::RunTest(const FString& Parameters)
{
	// The generated DT_BarksFallback (Tools/td_barks_from_dialogs.py + td-barks-import.py): dialogs.md's lines.
	FTDBarkLines::ResetForTests();
	FTDTestWorld W;
	FText Line;
	if (!FTDBarkLines::FindRow(TEXT("Bark_SquadSwitchIn"), TEXT("Moss")))
	{
		AddWarning(TEXT("DT_BarksFallback is not imported in this checkout (run Tools/td-barks-import.py)"));
		return true;
	}
	TestTrue(TEXT("switch-in, Moss"), FTDBarkLines::PickLine(W.GetWorld(), TEXT("Bark_SquadSwitchIn"), TEXT("Moss"), Line)
		&& (Line.ToString() == TEXT("Bastion's yours.") || Line.ToString() == TEXT("I'm with you.")));
	TestTrue(TEXT("an engage call with its direction"), FTDBarkLines::PickLine(W.GetWorld(), TEXT("Bark_SquadEngageLowBehind"), TEXT("Wren"), Line)
		&& Line.ToString().Contains(TEXT("low behind")));
	TestTrue(TEXT("Command"), FTDBarkLines::FindRow(TEXT("Bark_Cmd_Retake"), TEXT("Command")) != nullptr);
	TestTrue(TEXT("six banter exchanges"), FTDBarkLines::GetBanters().Num() == 6);
	return true;
}

#endif

// Triad Drift - the victory credits roll. Owner: Teammate 3.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/ScopeExit.h"
#include "TriadDrift/Director/Menu/TDCredits.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCreditsRoll, "TriadDrift.Director.Credits.FallbackNamesTheTeamAndNicholasWins", TD_TEST_FLAGS)
bool FTDCreditsRoll::RunTest(const FString& Parameters)
{
	// The fallback: the team, the story, the AI tools (the jam's "tools used"), thanks.
	FString All;
	for (const FTDCreditsSection& S : FTDCredits::Fallback())
	{
		All += S.Heading.ToString() + TEXT("\\n");
		for (const FText& L : S.Lines)
		{
			All += L.ToString() + TEXT("\\n");
		}
	}
	for (const TCHAR* Must : { TEXT("Ryan Thomas"), TEXT("Nicholas Tosti"), TEXT("Story by Nicholas Tosti"), TEXT("Claude Code"), TEXT("OpenAI Codex"),
		TEXT("Unreal Engine 5.9"), TEXT("MegaJam") })
	{
		TestTrue(FString::Printf(TEXT("the credits name %s"), Must), All.Contains(Must));
	}
	TestTrue(TEXT("no future reveal in the credits"), !All.Contains(TEXT("future")));
	// StoryFidelity01 (Nick's Story Director): the story's title gives the twist away and the credits can be read before the reveal.
	TestTrue(TEXT("the credits do not name the story's title"), !All.Contains(TEXT("The Paradox")));

	// Nicholas's DA_Credits replaces the whole list.
	UTDCreditsData* Nicks = NewObject<UTDCreditsData>(GetTransientPackage());
	Nicks->AddToRoot();
	ON_SCOPE_EXIT { FTDCredits::OverrideForTests = nullptr; Nicks->RemoveFromRoot(); };
	FTDCreditsSection Sec;
	Sec.Heading = FText::FromString(TEXT("NICHOLAS'S CREDITS"));
	Sec.Lines.Add(FText::FromString(TEXT("A line")));
	Nicks->Sections.Add(Sec);
	FTDCredits::OverrideForTests = Nicks;
	const TArray<FTDCreditsSection> Got = FTDCredits::GetSections();
	TestTrue(TEXT("his sections win"), Got.Num() == 1 && Got[0].Heading.ToString() == TEXT("NICHOLAS'S CREDITS"));
	// An empty DA_Credits keeps the fallback.
	Nicks->Sections.Reset();
	TestEqual(TEXT("an empty asset keeps the fallback"), FTDCredits::GetSections().Num(), FTDCredits::Fallback().Num());

	// The roll is a fixed-size, clipped box (it never pushes RETURN TO MENU off the panel).
	const TSharedRef<SWidget> Roll = FTDCredits::MakeRoll(620.f, 210.f);
	TestTrue(TEXT("clips its contents"), Roll->GetClipping() == EWidgetClipping::ClipToBounds);
	return true;
}

#endif

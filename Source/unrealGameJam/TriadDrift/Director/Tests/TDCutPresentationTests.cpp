// Fallback cut presentation (C-03 #1 / #2 / #3). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Director.CutPresentation

#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/UI/TDUIHostSettings.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDCutPresentationTests
{
	struct FFixture
	{
		FTDTestWorld W;
		ATDSquadManager* Squad = nullptr;
		UTDDirectorSubsystem* Director = nullptr;

		explicit FFixture(TSubclassOf<UGameInstance> GIClass = nullptr) : W(GIClass)
		{
			APlayerController* PC = W.SpawnPlayerController();
			Squad = W.Spawn<ATDSquadManager>();
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform::Identity);
			Director = UTDDirectorSubsystem::Get(W.GetWorld());
			W.Tick();
		}
		const FTDCutPresentationState& S() const { return Director->GetCutPresentation()->GetState(); }
		UTDEventBus* Bus() const { return UTDEventBus::Get(W.GetWorld()); }
	};
}

using namespace TDCutPresentationTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresSwitch, "TriadDrift.Director.CutPresentation.SwitchFlashLetterboxNameCard", TD_TEST_FLAGS)
bool FTDCutPresSwitch::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FFixture F;
		TestTrue(TEXT("nothing drawn before") + Fps, F.S().IsEmpty());
		TestTrue(TEXT("switch") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Lancer));
		F.W.Tick(Step);
		TestEqual(TEXT("1-frame white flash") + Fps, F.S().FlashAlpha, 1.f);
		F.W.Tick(Step);
		TestEqual(TEXT("flash gone the next frame") + Fps, F.S().FlashAlpha, 0.f);
		F.W.Advance(0.15f, Step);
		TestTrue(TEXT("letterbox in") + Fps, F.S().Letterbox > 0.1f);
		TestEqual(TEXT("name card"), F.S().NameCardText.ToString(), FString(TEXT("KATO · LANCER")));
		TestTrue(TEXT("in the squad colour") + Fps, F.S().NameCardColor.Equals(UTDShipLibrary::GetSquadColor(UTDShipLibrary::Get(ETDShipId::Lancer))));
		TestTrue(TEXT("card visible") + Fps, F.S().NameCardAlpha > 0.5f);
		F.W.Advance(0.6f, Step);
		TestTrue(TEXT("letterbox eases out") + Fps, F.S().Letterbox < 0.11f);
		F.W.Advance(1.f, Step);
		TestTrue(TEXT("everything gone after the card") + Fps, F.S().IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresRealHUD, "TriadDrift.Director.CutPresentation.RealHUDOwnsTheNameCard", TD_TEST_FLAGS)
bool FTDCutPresRealHUD::RunTest(const FString& Parameters)
{
	// N-007: Nick's HUD draws its own "CUT TO" card, so ours steps aside for it (or for any HUD claiming
	// bCoversNameCard). Engineering's flight HUD, the default since ENG-36, draws none: the card is ours under it
	// (the Lead's integration catch: with "any real HUD" the pilot card vanished from the game). The dev HUD: ours.
	// The flash and the letterbox always play.
	IConsoleVariable* Choice = IConsoleManager::Get().FindConsoleVariable(TEXT("TD.UI.SpaceHUD"));
	if (!TestNotNull(TEXT("TD.UI.SpaceHUD"), Choice))
	{
		return false;
	}
	ON_SCOPE_EXIT { Choice->Set(-1, ECVF_SetByCode); };
	for (const int32 Hud : { 2, 0, 1 })
	{
		const FString Case = Hud == 2 ? TEXT(" (engineering HUD)") : (Hud == 1 ? TEXT(" (Nick's HUD)") : TEXT(" (dev HUD)"));
		Choice->Set(Hud, ECVF_SetByCode);
		FFixture F;
		F.W.Tick();
		const UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(F.W.GetWorld());
		const bool bHosted = Host && Host->GetSpaceHUD();
		TestTrue(TEXT("switch") + Case, F.Squad->RequestSwitchTo(ETDShipId::Lancer));
		F.W.Tick();
		TestEqual(TEXT("flash still plays") + Case, F.S().FlashAlpha, 1.f);
		float MaxCard = 0.f, MaxBars = 0.f;
		for (int32 i = 0; i < 90; ++i)
		{
			F.W.Tick();
			MaxCard = FMath::Max(MaxCard, F.S().NameCardAlpha);
			MaxBars = FMath::Max(MaxBars, F.S().Letterbox);
		}
		TestTrue(TEXT("letterbox still plays") + Case, MaxBars > 0.1f);
		if (Hud == 1)
		{
			if (bHosted)
			{
				TestEqual(TEXT("our name card never shows over Nick's") + Case, MaxCard, 0.f);
			}
			else
			{
				AddInfo(TEXT("Nick's HUD isn't hosted in this test world: its case is checked live"));
			}
		}
		else
		{
			TestTrue(TEXT("our name card shows") + Case, MaxCard > 0.5f);
			TestFalse(TEXT("nothing claims it") + Case, UTDCutPresentation::IsNameCardClaimedByHUD(F.W.GetWorld()));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresSlateLayout, "TriadDrift.Director.CutPresentation.SlateClearOfBothHUDs", TD_TEST_FLAGS)
bool FTDCutPresSlateLayout::RunTest(const FString& Parameters)
{
	// Lead playtest 3: the slate faded in behind Nick's OBJECTIVE block (top 12-15%); the dev HUD's
	// squad rows sit at 12-21% on the left. The slate must clear both, at any size and bar height.
	for (const FVector2D Size : { FVector2D(1280.f, 720.f), FVector2D(1920.f, 1080.f), FVector2D(2560.f, 1440.f), FVector2D(1024.f, 768.f) })
	{
		for (const float Bars : { 0.f, 0.06f, 0.12f })
		{
			const FBox2D R = UTDCutPresentation::SlateRect(Size, Bars);
			const FString Case = FString::Printf(TEXT(" (%.0fx%.0f, bars %.2f)"), Size.X, Size.Y, Bars);
			TestTrue(TEXT("below the real HUD's top 15%") + Case, R.Min.Y >= Size.Y * 0.15f);
			const FBox2D DevRows(FVector2D(0.f, Size.Y * 0.12f), FVector2D(600.f, Size.Y * 0.12f + 3.f * 22.f));
			TestFalse(TEXT("clear of the dev HUD squad rows") + Case, R.Intersect(DevRows));
			TestTrue(TEXT("below the letterbox bar") + Case, R.Min.Y >= Size.Y * Bars);
			TestTrue(TEXT("in the upper third") + Case, R.Max.Y <= Size.Y * 0.36f);
			TestTrue(TEXT("on screen, centred") + Case, R.Min.X >= 0.f && R.Max.X <= Size.X && FMath::IsNearlyEqual(R.GetCenter().X, Size.X * 0.5f));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresLayer, "TriadDrift.Director.CutPresentation.UnderTheRealHUD", TD_TEST_FLAGS)
bool FTDCutPresLayer::RunTest(const FString& Parameters)
{
	// Live capture: the letterbox covered Nick's squad cards during a switch. With his HUD up our overlay
	// sits just under it; without it, above gameplay widgets and under the F10 menu (1000).
	TestTrue(TEXT("real HUD up: under it"), UTDCutPresentation::OverlayZOrder(true) < UTDUIHostSettings::Get()->ZOrder);
	TestTrue(TEXT("no real HUD: above gameplay, under the menu"), UTDCutPresentation::OverlayZOrder(false) > UTDUIHostSettings::Get()->ZOrder && UTDCutPresentation::OverlayZOrder(false) < 1000);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresSlateFitsAndClears, "TriadDrift.Director.CutPresentation.SlateFitsItsTextAndClearsBeforeTheWave", TD_TEST_FLAGS)
bool FTDCutPresSlateFitsAndClears::RunTest(const FString& Parameters)
{
	// Lead item 4: the long numbered slate fits its box at 1280x720 and 1920x1080, and it is gone by
	// the time a wave's first enemies arrive (warp-in 1.5 s), so it never covers incoming fire.
	for (const FVector2D Size : { FVector2D(1280.f, 720.f), FVector2D(1920.f, 1080.f) })
	{
		const float LongText = 640.f; // about "SCENE 6 · CINDER · OUTPOST · TAKE 1" at 28 pt
		const FBox2D R = UTDCutPresentation::SlateRect(Size, 0.f, LongText);
		TestTrue(FString::Printf(TEXT("box wider than the text at %.0fx%.0f"), Size.X, Size.Y), R.GetSize().X >= LongText + 40.f);
		TestTrue(FString::Printf(TEXT("still on screen at %.0fx%.0f"), Size.X, Size.Y), R.Min.X >= 0.f && R.Max.X <= Size.X);
	}
	FFixture F;
	F.Bus()->BroadcastTakeStarted(TEXT("Wave3"), 1);
	F.W.Tick();
	TestTrue(TEXT("slate up"), F.S().SlateAlpha > 0.f);
	F.W.Advance(1.6f);
	TestEqual(TEXT("gone before the wave's enemies arrive (1.5 s warp-in + a beat)"), F.S().SlateAlpha, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresSlowMo, "TriadDrift.Director.CutPresentation.RealTimeUnderSlowMoAndQuickCut", TD_TEST_FLAGS)
bool FTDCutPresSlowMo::RunTest(const FString& Parameters)
{
	FFixture F;
	UTDFeelSubsystem::Get(F.W.GetWorld())->RequestSlowMo(5.f, 0.2f);
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	F.W.Advance(1.8f); // real seconds; only ~0.36 s of game time
	TestTrue(TEXT("slow-mo doesn't stretch the cut presentation"), F.S().IsEmpty());

	// Quick re-switch: flash and card, no letterbox.
	F.W.Advance(0.1f);
	UTDFeelSubsystem::Get(F.W.GetWorld())->CancelAll();
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	F.W.Advance(0.45f);
	TestTrue(TEXT("switch again quickly"), F.Squad->RequestSwitchTo(ETDShipId::Rift));
	F.W.Tick();
	TestEqual(TEXT("quick cut still flashes"), F.S().FlashAlpha, 1.f);
	F.W.Advance(0.15f);
	TestEqual(TEXT("quick cut: no letterbox"), F.S().Letterbox, 0.f);
	TestTrue(TEXT("quick cut: name card"), F.S().NameCardText.ToString().StartsWith(TEXT("WREN")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresExternal, "TriadDrift.Director.CutPresentation.ExternalHUDSuppressesIt", TD_TEST_FLAGS)
bool FTDCutPresExternal::RunTest(const FString& Parameters)
{
	FFixture F;
	F.Director->bCutPresentationHandledExternally = true;
	F.W.Tick();
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	F.Bus()->BroadcastSceneCut(TEXT("Wave3"), 2, 1.5f);
	F.Bus()->BroadcastTakeStarted(TEXT("Wave3"), 2);
	for (int32 i = 0; i < 30; ++i)
	{
		F.W.Tick();
		TestTrue(TEXT("nothing drawn while a real HUD handles it"), F.S().IsEmpty());
	}
	// Handing it back mid-way: the fallback draws the next event again.
	F.Director->bCutPresentationHandledExternally = false;
	F.W.Advance(0.5f);
	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
	F.W.Tick();
	TestEqual(TEXT("fallback active again"), F.S().FlashAlpha, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresCutAndSlate, "TriadDrift.Director.CutPresentation.CutCardThenSlate", TD_TEST_FLAGS)
bool FTDCutPresCutAndSlate::RunTest(const FString& Parameters)
{
	// features.md C-03 #3: "SCENE 3 · CINDER · TAKE 1".
	TestEqual(TEXT("wave scene"), UTDCutPresentation::MakeSlateText(TEXT("Wave3"), 2, 3).ToString(), FString(TEXT("SCENE 3 · WAVE 3 · TAKE 2")));
	TestEqual(TEXT("planet objective"), UTDCutPresentation::MakeSlateText(TEXT("Obj_Outpost"), 1, 6, TEXT("Cinder")).ToString(), FString(TEXT("SCENE 6 · CINDER · OUTPOST · TAKE 1")));
	TestEqual(TEXT("CamelCase objective"), UTDCutPresentation::MakeSlateText(TEXT("Obj_WreckCore"), 2, 4, TEXT("Verdance")).ToString(), FString(TEXT("SCENE 4 · VERDANCE · WRECK CORE · TAKE 2")));
	TestEqual(TEXT("the finale"), UTDCutPresentation::MakeSlateText(TEXT("Dominion"), 1, 9).ToString(), FString(TEXT("SCENE 9 · DOMINION · TAKE 1")));
	TestEqual(TEXT("no number known"), UTDCutPresentation::MakeSlateText(TEXT("Wave3"), 2).ToString(), FString(TEXT("WAVE 3 · TAKE 2")));
	// The story name wins over the id (T5's renames: ids unchanged, titles new).
	TestEqual(TEXT("objective title"), UTDCutPresentation::MakeSlateText(TEXT("Obj_WreckCore"), 2, 4, TEXT("Verdance"),
		FText::FromString(TEXT("The Research Site"))).ToString(), FString(TEXT("SCENE 4 · VERDANCE · THE RESEARCH SITE · TAKE 2")));
	TestEqual(TEXT("title without a number"), UTDCutPresentation::MakeSlateText(TEXT("Obj_Jammer"), 1, 0, FString(),
		FText::FromString(TEXT("The Field Anchor"))).ToString(), FString(TEXT("THE FIELD ANCHOR · TAKE 1")));
	{
		FTDTestWorld TW;
		ATDObjective* Obj = TW.Spawn<ATDObjective>(FVector::ZeroVector);
		if (TestNotNull(TEXT("objective"), Obj))
		{
			Obj->ObjectiveId = TEXT("Obj_Outpost");
			Obj->Title = FText::FromString(TEXT("The Mining Complex"));
			TestEqual(TEXT("the slate finds the objective's title"), UTDCutPresentation::FindSceneTitle(TW.GetWorld(), TEXT("Obj_Outpost")).ToString(),
				FString(TEXT("The Mining Complex")));
			TestTrue(TEXT("a wave has no title: the id is used"), UTDCutPresentation::FindSceneTitle(TW.GetWorld(), TEXT("Wave3")).IsEmpty());
		}
	}
	FFixture F;
	F.Bus()->BroadcastSceneCut(TEXT("Wave3"), 2, 1.5f);
	F.W.Advance(0.2f);
	TestEqual(TEXT("CUT! card"), F.S().CutCardText.ToString(), FString(TEXT("CUT!")));
	TestTrue(TEXT("CUT! visible"), F.S().CutCardAlpha > 0.5f);
	TestTrue(TEXT("bars with it"), F.S().Letterbox > 0.05f);
	F.W.Advance(1.0f);
	TestTrue(TEXT("CUT! held for the retake delay"), F.S().CutCardAlpha > 0.f);
	F.Bus()->BroadcastTakeStarted(TEXT("Wave3"), 2);
	F.W.Advance(0.3f);
	TestEqual(TEXT("CUT! replaced by the slate"), F.S().CutCardAlpha, 0.f);
	TestEqual(TEXT("slate"), F.S().SlateText.ToString(), FString(TEXT("WAVE 3 · TAKE 2"))); // no game flow here: no scene number
	TestTrue(TEXT("slate visible"), F.S().SlateAlpha > 0.5f);
	F.W.Advance(2.5f);
	TestTrue(TEXT("slate gone"), F.S().IsEmpty());
	// A death outside any scene: OnTakeStarted(None) shows no slate.
	F.Bus()->BroadcastTakeStarted(NAME_None, 0);
	F.W.Tick();
	TestEqual(TEXT("no slate for an unnamed take"), F.S().SlateAlpha, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresSlateRealFlow, "TriadDrift.Director.CutPresentation.SlateNumbersScenesThroughTheFlow", TD_TEST_FLAGS)
bool FTDCutPresSlateRealFlow::RunTest(const FString& Parameters)
{
	// C-03 #3 through T5's real game flow: scenes are numbered in the order the run opens them.
	FFixture F(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(F.W.GetGameInstance());
	if (!TestNotNull(TEXT("game instance"), GI)) { return false; }
	GI->BeginScene(TEXT("Wave1"));
	F.W.Tick();
	TestEqual(TEXT("first scene"), F.S().SlateText.ToString(), FString(TEXT("SCENE 1 · WAVE 1 · TAKE 1")));
	F.W.Advance(3.f);
	GI->BeginScene(TEXT("Wave2"));
	F.W.Tick();
	TestEqual(TEXT("second scene"), F.S().SlateText.ToString(), FString(TEXT("SCENE 2 · WAVE 2 · TAKE 1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresShot, "TriadDrift.Director.CutPresentation.MoneyShotThinBars", TD_TEST_FLAGS)
bool FTDCutPresShot::RunTest(const FString& Parameters)
{
	FFixture F;
	AActor* Subject = TDPowerTest::SpawnBody(F.W, ETDTeam::Enemy, FVector(20000.f, 0.f, 0.f), FVector(200.f), 100.f, 0.f);
	TestTrue(TEXT("shot accepted"), F.Director->ReportHighlight(ETDHighlightKind::MissileReturn, Subject));
	F.W.Advance(0.25f);
	TestTrue(TEXT("thin bars during the shot"), F.S().Letterbox > 0.03f && F.S().Letterbox <= 0.061f);
	F.W.Advance(1.5f);
	TestEqual(TEXT("bars gone after"), F.S().Letterbox, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCutPresCleanup, "TriadDrift.Director.CutPresentation.NothingSurvivesTravelOrShutdown", TD_TEST_FLAGS)
bool FTDCutPresCleanup::RunTest(const FString& Parameters)
{
	FFixture F(UTDGameInstance::StaticClass());
	UTDFlowSettings* Settings = GetMutableDefault<UTDFlowSettings>();
	const TMap<ETDDestination, FSoftObjectPath> SavedMaps = Settings->DestinationMaps;
	const FSoftObjectPath SavedTransition = Settings->TransitionMap;
	ON_SCOPE_EXIT { Settings->DestinationMaps = SavedMaps; Settings->TransitionMap = SavedTransition; };
	Settings->DestinationMaps.Add(ETDDestination::Verdance, FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry")));
	Settings->TransitionMap = FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry"));
	UTDGameInstance* GI = Cast<UTDGameInstance>(F.W.GetGameInstance());
	GI->OpenLevelOverride = [](const FString&) {};

	TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Lancer));
	F.W.Advance(0.1f);
	TestFalse(TEXT("presentation running"), F.S().IsEmpty());
	TestTrue(TEXT("travel"), GI->TravelTo(ETDDestination::Verdance, NAME_None));
	F.W.Tick();
	TestTrue(TEXT("cleared on travel"), F.S().IsEmpty());

	UTDCutPresentation* P = F.Director->GetCutPresentation();
	P->HandleSceneCut(2.f);
	P->Shutdown();
	TestTrue(TEXT("shutdown clears the state"), P->GetState().IsEmpty());
	TestFalse(TEXT("and removes any viewport widget"), P->HasViewportWidget());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

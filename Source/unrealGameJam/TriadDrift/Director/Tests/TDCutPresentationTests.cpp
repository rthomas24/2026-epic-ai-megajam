// Fallback cut presentation (C-03 #1 / #2 / #3). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Director.CutPresentation

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"
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
	TestEqual(TEXT("slate wording"), UTDCutPresentation::MakeSlateText(TEXT("Wave3"), 2).ToString(), FString(TEXT("SCENE Wave 3 · TAKE 2")));
	TestEqual(TEXT("CamelCase scene ids"), UTDCutPresentation::MakeSlateText(TEXT("VerdanceWreckCore"), 1).ToString(), FString(TEXT("SCENE Verdance Wreck Core · TAKE 1")));
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
	TestEqual(TEXT("slate"), F.S().SlateText.ToString(), FString(TEXT("SCENE Wave 3 · TAKE 2")));
	TestTrue(TEXT("slate visible"), F.S().SlateAlpha > 0.5f);
	F.W.Advance(2.5f);
	TestTrue(TEXT("slate gone"), F.S().IsEmpty());
	// A death outside any scene: OnTakeStarted(None) shows no slate.
	F.Bus()->BroadcastTakeStarted(NAME_None, 0);
	F.W.Tick();
	TestEqual(TEXT("no slate for an unnamed take"), F.S().SlateAlpha, 0.f);
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

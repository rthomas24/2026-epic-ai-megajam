// ENG-45 (T7): subtitle size and graphics preset - persisted, applied, on the settings page.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Settings.Accessibility

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Input/TDSettingsPage.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"

namespace TDAccessibilityTest
{
	static const TCHAR* Slot = TEXT("TD_Settings_AccessibilityTest");

	struct FWorld
	{
		FTDTestWorld W;
		UTDSettingsSubsystem* S = nullptr;
		bool Init()
		{
			UGameplayStatics::DeleteGameInSlot(Slot, 0);
			S = UTDSettingsSubsystem::GetTDSettings(W.GetWorld());
			if (S) { S->UseSlotForTests(Slot); }
			return S != nullptr;
		}
		~FWorld() { UGameplayStatics::DeleteGameInSlot(Slot, 0); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAccessibilityPersist, "TriadDrift.Input.Settings.Accessibility.SubtitleAndGraphicsPersist", TD_TEST_FLAGS)
bool FTDAccessibilityPersist::RunTest(const FString& Parameters)
{
	TDAccessibilityTest::FWorld T;
	if (!TestTrue(TEXT("settings subsystem"), T.Init())) { return false; }

	// Defaults: medium subtitles (scale 1), the graphics preset not chosen yet.
	TestEqual(TEXT("default subtitle size"), T.S->GetSubtitleSize(), ETDSubtitleSize::Medium);
	TestEqual(TEXT("default subtitle scale"), UTDSettingsSubsystem::GetSubtitleScale(T.W.GetWorld()), 1.f);
	TestFalse(TEXT("graphics not chosen by default"), T.S->GetSettings().bGraphicsChosen);

	// Choose, then a fresh start-up reads them back.
	T.S->SetSubtitleSize(ETDSubtitleSize::Large);
	T.S->SetGraphicsPreset(ETDGraphicsPreset::Low);
	TestEqual(TEXT("Low requests overall scalability 0"), T.S->GetAppliedScalabilityLevel(), 0);
	T.S->ReloadForTests();
	TestEqual(TEXT("subtitle size persists"), T.S->GetSubtitleSize(), ETDSubtitleSize::Large);
	TestEqual(TEXT("large subtitles scale 1.3"), UTDSettingsSubsystem::GetSubtitleScale(T.W.GetWorld()), 1.3f);
	TestEqual(TEXT("graphics preset persists"), T.S->GetGraphicsPreset(), ETDGraphicsPreset::Low);
	TestTrue(TEXT("the choice persists"), T.S->GetSettings().bGraphicsChosen);

	// Every preset maps to its engine level and back; a custom mix reads as High.
	for (int32 L = 0; L <= 3; ++L)
	{
		TestEqual(*FString::Printf(TEXT("level %d round trip"), L), UTDSettingsSubsystem::ScalabilityLevelFor(UTDSettingsSubsystem::PresetForLevel(L)), L);
	}
	TestEqual(TEXT("Epic = 3"), UTDSettingsSubsystem::ScalabilityLevelFor(ETDGraphicsPreset::Epic), 3);
	TestEqual(TEXT("custom (-1) reads as High"), UTDSettingsSubsystem::PresetForLevel(-1), ETDGraphicsPreset::High);
	TestEqual(TEXT("small subtitles scale 0.8"), UTDSettingsSubsystem::SubtitleScaleFor(ETDSubtitleSize::Small), 0.8f);

	// The page's rows drive the same setters and clamp at the ends.
	FTDSettingsPageModel Page(T.S);
	Page.Select((int32)ETDSettingsRow::SubtitleSize);
	Page.Adjust(+1); // Large stays Large
	TestEqual(TEXT("page: subtitle size clamps at Large"), T.S->GetSubtitleSize(), ETDSubtitleSize::Large);
	Page.Adjust(-1);
	TestEqual(TEXT("page: subtitle size down"), T.S->GetSubtitleSize(), ETDSubtitleSize::Medium);
	TestEqual(TEXT("page: label"), FTDSettingsPageModel::RowLabel(ETDSettingsRow::SubtitleSize).ToString(), FString(TEXT("Subtitle size")));
	Page.Select((int32)ETDSettingsRow::GraphicsPreset);
	Page.Adjust(+1);
	Page.Adjust(+1);
	TestEqual(TEXT("page: graphics Low -> High"), T.S->GetGraphicsPreset(), ETDGraphicsPreset::High);
	TestEqual(TEXT("page: High requests level 2"), T.S->GetAppliedScalabilityLevel(), 2);
	TestEqual(TEXT("page: value shows the choice"), Page.RowValue(ETDSettingsRow::GraphicsPreset).ToString(), FString(TEXT("High")));

	// Reset to defaults forgets the choice (back to the detected preset) and medium subtitles.
	T.S->ResetToDefaults();
	T.S->ReloadForTests();
	TestFalse(TEXT("reset: graphics back to detected"), T.S->GetSettings().bGraphicsChosen);
	TestEqual(TEXT("reset: medium subtitles"), T.S->GetSubtitleSize(), ETDSubtitleSize::Medium);
	return true;
}

// The page fits short screens (T7, playtest 2: at 900p Reset was cut and Back off screen): the option rows scroll in the
// space left over, Back is pinned below the list.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSettingsPageFits, "TriadDrift.Input.Settings.Accessibility.PageFitsShortScreens", TD_TEST_FLAGS)
bool FTDSettingsPageFits::RunTest(const FString& Parameters)
{
	TDAccessibilityTest::FWorld T;
	if (!TestTrue(TEXT("settings subsystem"), T.Init())) { return false; }
	if (!FSlateApplication::IsInitialized())
	{
		AddInfo(TEXT("no Slate in this runner: the layout is checked in play"));
		return true;
	}
	const TSharedRef<STDSettingsPage> Page = SNew(STDSettingsPage).Settings(T.S);
	TestFalse(TEXT("Back is pinned outside the scrolling list"), Page->IsRowInScrollList(ETDSettingsRow::Back));
	TestTrue(TEXT("Reset scrolls with the options"), Page->IsRowInScrollList(ETDSettingsRow::Reset));
	TestTrue(TEXT("every option row is in the list"), Page->IsRowInScrollList(ETDSettingsRow::SubtitleSize) && Page->IsRowInScrollList(ETDSettingsRow::OnFootCamera));
	for (const float H : { 720.f, 900.f, 1080.f })
	{
		Page->SetPageHeightForTests(H);
		const float Max = Page->GetRowListMaxHeight();
		TestTrue(*FString::Printf(TEXT("%.0f: the list leaves room for title, hint and Back (max %.0f)"), H, Max), Max + STDSettingsPage::FixedPartsHeight <= H + 0.5f);
	}
	return true;
}

#endif

// Triad Drift - the Director's Cut cards for a renderer widget (N-038). Owner: Teammate 3.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Squad/TDSwitchCut.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Director/Tests/TDDirectorTestWidget.h"

namespace TDDirectorCardTest
{
	static FTDSwitchCut MakeCut()
	{
		FTDSwitchCut Cut;
		Cut.PilotName = FText::FromString(TEXT("KATO"));
		Cut.ShipName = FText::FromString(TEXT("LANCER"));
		Cut.RoleName = FText::FromString(TEXT("INTERCEPTOR"));
		Cut.NameCardText = FText::FromString(TEXT("KATO · LANCER"));
		Cut.SquadColor = FLinearColor(0.1f, 0.9f, 1.f);
		return Cut;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCards_Renderer, "TriadDrift.Director.Cards.StateForARendererWidget", TD_TEST_FLAGS)
bool FTDDirectorCards_Renderer::RunTest(const FString& Parameters)
{
	for (const bool bRenderer : { false, true })
	{
		const FString Tag = bRenderer ? TEXT(" (renderer widget)") : TEXT(" (fallback overlay)");
		FTDTestWorld W;
		APlayerController* PC = W.SpawnLocalPlayerController();
		UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(W.GetWorld());
		UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
		if (!TestNotNull(TEXT("director") + Tag, Director) || !TestNotNull(TEXT("bus") + Tag, Bus) || !TestNotNull(TEXT("player") + Tag, PC))
		{
			return false;
		}
		if (bRenderer)
		{
			Director->DirectorWidgetClassOverride = UTDDirectorTestWidget::StaticClass();
		}
		UTDCutPresentation* Cut = Director->GetCutPresentation();
		int32 Changes = 0;
		Cut->OnCardsChanged.AddLambda([&Changes](const FTDDirectorCards&) { ++Changes; });
		W.Tick();
		TestEqual(TEXT("a widget only when the style names one") + Tag, Director->GetDirectorWidget() != nullptr, bRenderer);
		if (bRenderer && Director->GetDirectorWidget())
		{
			TestEqual(TEXT("the widget never takes input"), Director->GetDirectorWidget()->GetVisibility(), ESlateVisibility::HitTestInvisible);
		}

		// The slate.
		Bus->BroadcastTakeStarted(TEXT("Wave2"), 3);
		W.Tick();
		FTDDirectorCards C = UTDDirectorSubsystem::GetDirectorCardsFor(W.GetWorld());
		TestTrue(TEXT("slate active") + Tag, C.Slate.bActive);
		TestTrue(TEXT("slate text says the take") + Tag, C.Slate.Text.ToString().Contains(TEXT("TAKE 3")));
		TestTrue(TEXT("slate scene and take") + Tag, C.SlateScene == FName(TEXT("Wave2")) && C.SlateTake == 3);
		TestTrue(TEXT("slate progress and time left") + Tag, C.Slate.Progress01 >= 0.f && C.Slate.Progress01 < 0.2f && C.Slate.TimeLeft > 1.f);
		TestTrue(TEXT("the change event fired") + Tag, Changes > 0);
		// The fallback draws unless a renderer does; the state is the same either way.
		TestEqual(TEXT("fallback overlay draws only without a renderer") + Tag, Cut->GetState().SlateAlpha > 0.f, !bRenderer);
		const int32 ChangesBefore = Changes;
		W.Tick();
		TestEqual(TEXT("no change event while nothing comes or goes") + Tag, Changes, ChangesBefore);

		// The switch cut and the pilot card.
		Cut->HandleSwitchCut(TDDirectorCardTest::MakeCut());
		W.Tick();
		C = Director->GetDirectorCards();
		TestTrue(TEXT("switch cut active") + Tag, C.SwitchCut.bActive && C.SwitchCount == 1);
		TestTrue(TEXT("pilot card") + Tag, C.NameCard.bActive && C.NameCard.Text.ToString().Contains(TEXT("KATO")));
		TestTrue(TEXT("pilot card second line") + Tag, C.NameCard.SubText.ToString().Contains(TEXT("INTERCEPTOR")));
		TestTrue(TEXT("squad colour") + Tag, C.NameCard.Color.Equals(FLinearColor(0.1f, 0.9f, 1.f)));
		TestTrue(TEXT("bars") + Tag, C.Letterbox > 0.f);
		if (bRenderer)
		{
			TestTrue(TEXT("a renderer: the overlay draws nothing at all"), Cut->GetState().IsEmpty());
		}

		// A money shot.
		AActor* Subject = W.Spawn<AActor>(FVector(4000.f, 0.f, 0.f));
		W.Advance(3.f, 1.f / 30.f);
		TestTrue(TEXT("a money shot") + Tag, Director->ReportHighlight(ETDHighlightKind::RamKill, Subject, 0.7f));
		W.Tick();
		C = Director->GetDirectorCards();
		TestTrue(TEXT("money shot card") + Tag, C.MoneyShot.bActive && C.MoneyShot.SubText.ToString() == TEXT("RamKill"));
		TestTrue(TEXT("its time left") + Tag, C.MoneyShot.TimeLeft > 0.f);

		// CUT!
		Bus->BroadcastSceneCut(TEXT("Wave2"), 4, 1.5f);
		W.Tick();
		C = Director->GetDirectorCards();
		TestTrue(TEXT("CUT! card") + Tag, C.Cut.bActive && C.Cut.Text.ToString() == TEXT("CUT!"));
		W.Advance(3.f, 1.f / 30.f);
		TestFalse(TEXT("cards end on time") + Tag, Director->GetDirectorCards().AnyActive());

		// bCutPresentationHandledExternally keeps its meaning: everything off.
		Bus->BroadcastTakeStarted(TEXT("Wave2"), 4);
		Director->bCutPresentationHandledExternally = true;
		W.Tick();
		W.Tick();
		TestFalse(TEXT("handled externally: no cards") + Tag, Director->GetDirectorCards().AnyActive());
	}
	return true;
}

#endif

#include "TDLoadingWidget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"

#if UE_BUILD_DEVELOPMENT
#include "HAL/IConsoleManager.h"
namespace TDLoadingReview
{
    // Module lifetime, not world/widget lifetime. Never written to an asset, CDO or config.
    static FText Override;
    static FText Resolve(const UTDLoadingWidget& Widget)
    { return Override.IsEmpty()?Widget.StatusText:Override; }

    static void Set(const TArray<FString>& Args)
    {
        if(Args.IsEmpty())
        {
            UE_LOG(LogTemp,Display,TEXT("TD_LOADING_STATUS_OVERRIDE active=%d chars=%d; use TD.UI.LoadingStatus \"text\" or TD.UI.LoadingStatus.Clear"),!Override.IsEmpty(),Override.ToString().Len());
            return;
        }
        Override=FText::FromString(FString::Join(Args,TEXT(" ")));
        UE_LOG(LogTemp,Display,TEXT("TD_LOADING_STATUS_OVERRIDE set chars=%d; process-local, survives travel"),Override.ToString().Len());
    }
    static void Clear()
    {
        Override=FText::GetEmpty();
        UE_LOG(LogTemp,Display,TEXT("TD_LOADING_STATUS_OVERRIDE cleared; host status restored"));
    }
    static FAutoConsoleCommand SetCommand(TEXT("TD.UI.LoadingStatus"),
        TEXT("Development review: override rendered loading status across travel. Quote text; no args reports state."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&Set));
    static FAutoConsoleCommand ClearCommand(TEXT("TD.UI.LoadingStatus.Clear"),
        TEXT("Development review: clear override and show each widget's unchanged host status."),
        FConsoleCommandDelegate::CreateStatic(&Clear));
}
#endif

TSharedRef<SWidget> UTDLoadingWidget::RebuildWidget()
{
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    const FLinearColor Ink(.85f,.93f,.98f);
    const FLinearColor Muted(.40f,.57f,.67f);
    const FLinearColor Cyan(.22f,.76f,.91f);
    const TWeakObjectPtr<UTDLoadingWidget> Weak(this);
    const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
    TSharedRef<SHorizontalBox> Colours=SNew(SHorizontalBox);
    for(const FLinearColor& Colour:{FLinearColor(.96f,.18f,.82f),Cyan,FLinearColor(1.f,.65f,.16f)})
        Colours->AddSlot().FillWidth(1).Padding(0,0,8,0)
        [SNew(SBox).HeightOverride(2)[SNew(SBorder).BorderImage(White).BorderBackgroundColor(Colour).Padding(0)]];
    return SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(.008f,.014f,.025f,1.f)).Padding(0)
    [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
        [SNew(SBox).WidthOverride(1920).HeightOverride(1080)
            [SNew(SOverlay)
                +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                [SNew(SBox).WidthOverride(560)
                    [SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(.07f,.15f,.21f,.8f)).Padding(1)
                        [SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(.018f,.033f,.053f,1.f)).Padding(28)
                            [SNew(SVerticalBox)
                                +SVerticalBox::Slot().AutoHeight()
                                [SNew(STextBlock).Text(NSLOCTEXT("TDLoading","Director","DIRECTOR'S CUT"))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(Cyan)]
                                +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
                                [SNew(STextBlock).Text_Lambda([Weak]{return Weak.IsValid()?Weak->SceneTitle:FText::GetEmpty();}).AutoWrapText(true)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",30)).ColorAndOpacity(Ink)]
                                +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,22)
                                [SNew(STextBlock).Text_Lambda([Weak]
                                {
                                    if(!Weak.IsValid()) return FText::GetEmpty();
#if UE_BUILD_DEVELOPMENT
                                    return TDLoadingReview::Resolve(*Weak.Get());
#else
                                    return Weak->StatusText;
#endif
                                }).AutoWrapText(true)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Regular",24)).ColorAndOpacity(Ink)]
                                +SVerticalBox::Slot().AutoHeight()[Colours]
                            ]
                        ]
                    ]
                ]
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(100,0,0,66)
                [SNew(STextBlock).Text(NSLOCTEXT("TDLoading","Squad","TRIAD / DRIFT     RIFT / LANCER / BASTION"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(Muted)]
            ]
        ]
    ];
}

#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDLoadingReviewOverride,"TriadDrift.UI.Loading.DevelopmentOverrideSurvivesWidgetReplacementAndClears",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDLoadingReviewOverride::RunTest(const FString&)
{
    const FText Previous=TDLoadingReview::Override;
    ON_SCOPE_EXIT { TDLoadingReview::Override=Previous; };
    TDLoadingReview::Clear();
    const FText DefaultStatus=GetDefault<UTDLoadingWidget>()->StatusText;
    TStrongObjectPtr<UTDLoadingWidget> First(NewObject<UTDLoadingWidget>());
    First->StatusText=FText::FromString(TEXT("First host status"));
    TestTrue(TEXT("normal rendering uses the host value"),TDLoadingReview::Resolve(*First.Get()).EqualTo(First->StatusText));
    const FString Sample=TEXT("Development layout sample: a longer status remains visible after the transition widget is replaced without changing its host-supplied status.");
    const FString Command=FString::Printf(TEXT("TD.UI.LoadingStatus \"%s\""),*Sample);
    TestTrue(TEXT("real console set command is registered"),IConsoleManager::Get().ProcessUserConsoleInput(*Command,*GLog,nullptr));
    TestEqual(TEXT("quoted text reaches first widget"),TDLoadingReview::Resolve(*First.Get()).ToString(),Sample);
    TStrongObjectPtr<UTDLoadingWidget> Next(NewObject<UTDLoadingWidget>());
    Next->StatusText=FText::FromString(TEXT("Next host status"));
    TestEqual(TEXT("new transition widget keeps the override"),TDLoadingReview::Resolve(*Next.Get()).ToString(),Sample);
    TestEqual(TEXT("first host value is untouched"),First->StatusText.ToString(),FString(TEXT("First host status")));
    TestTrue(TEXT("real clear command is registered"),IConsoleManager::Get().ProcessUserConsoleInput(TEXT("TD.UI.LoadingStatus.Clear"),*GLog,nullptr));
    TestTrue(TEXT("clear restores current host value"),TDLoadingReview::Resolve(*Next.Get()).EqualTo(Next->StatusText));
    TestTrue(TEXT("class default never changed"),GetDefault<UTDLoadingWidget>()->StatusText.EqualTo(DefaultStatus));
    return true;
}
#endif

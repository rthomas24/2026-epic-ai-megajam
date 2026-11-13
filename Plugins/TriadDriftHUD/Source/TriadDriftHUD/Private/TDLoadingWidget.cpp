#include "TDLoadingWidget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"

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
                                [SNew(STextBlock).Text_Lambda([Weak]{return Weak.IsValid()?Weak->StatusText:FText::GetEmpty();}).AutoWrapText(true)
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

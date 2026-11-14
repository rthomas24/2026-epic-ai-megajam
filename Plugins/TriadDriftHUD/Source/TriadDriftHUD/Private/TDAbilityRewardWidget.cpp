#include "TDAbilityRewardWidget.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace TDIndustrialReward
{
    const FLinearColor Ice(.46f,.90f,1.f), Gold(1.f,.70f,.25f), Ink(.88f,.94f,.97f);

    /** Static decorative surface: no tick, blur, render target, or gameplay polling. */
    class SSurface final : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SSurface) {} SLATE_ATTRIBUTE(FLinearColor,Accent) SLATE_ARGUMENT(const FSlateBrush*,Frame) SLATE_END_ARGS()
        void Construct(const FArguments& Args) { Accent=Args._Accent;Frame=Args._Frame;SetVisibility(EVisibility::HitTestInvisible); }
        virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1320,740); }
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& List,int32 Layer,const FWidgetStyle& Style,bool) const override
        {
            const FVector2D Size=G.GetLocalSize();
            const FVector2D Scale(Size.X/1320.f,Size.Y/740.f);
            const FLinearColor Tint=Style.GetColorAndOpacityTint();
            auto Line=[&](TArray<FVector2D> Points,FLinearColor C,float Width=1.f)
            {
                for(FVector2D& P:Points) P*=Scale;
                FSlateDrawElement::MakeLines(List,Layer+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,C*Tint,true,Width);
            };
            const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
            FSlateDrawElement::MakeBox(List,Layer,G.ToPaintGeometry(FVector2D(1200,620)*Scale,FSlateLayoutTransform(FVector2D(60,60)*Scale)),White,ESlateDrawEffect::None,FLinearColor(.009f,.018f,.025f,.97f)*Tint);
            // Very faint static screen texture, kept below every live label.
            for(int32 Y=72;Y<670;Y+=8) Line({{74.f,float(Y)},{1246.f,float(Y)}},FLinearColor(.20f,.38f,.43f,.022f));
            for(int32 I=0;I<3;++I)
            {
                const float Y=548.f+float(I-1)*3.f;
                Line({{88,Y},{1232,Y}},Gold.CopyWithNewOpacity(I==1?.95f:.16f),I==1?2.f:4.f);
            }
            // Original emitter schematic: projected rings and longitudinal cage, no invented readouts.
            const FVector2D Center(977,300);
            auto Project=[&](float X,float Y,float Z){return Center+FVector2D(X*.81f+Z*.48f,Y*.86f-X*.28f+Z*.19f);};
            const FLinearColor Wire=Ice.CopyWithNewOpacity(.57f);
            for(int32 Ring=0;Ring<7;++Ring)
            {
                const float Z=-108.f+Ring*36.f;
                const float Radius=Ring==0||Ring==6?72.f:110.f;
                TArray<FVector2D> Points;
                for(int32 N=0;N<=64;++N){const float A=N*2.f*PI/64.f;Points.Add(Project(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,Z));}
                Line(Points,Wire,1.f);
            }
            for(int32 N=0;N<20;++N)
            {
                const float A=N*2.f*PI/20.f;
                TArray<FVector2D> Points;
                for(int32 Ring=0;Ring<7;++Ring){const float R=Ring==0||Ring==6?72.f:110.f;Points.Add(Project(FMath::Cos(A)*R,FMath::Sin(A)*R,-108.f+Ring*36.f));}
                Line(Points,Wire.CopyWithNewOpacity(.36f));
            }
            const FLinearColor Identity=Accent.Get(Ice).CopyWithNewOpacity(.72f);
            Line({{802,155},{802,132},{828,132}},Identity);
            Line({{1130,132},{1156,132},{1156,155}},Identity);
            Line({{802,448},{802,470},{828,470}},Identity);
            Line({{1130,470},{1156,470},{1156,448}},Identity);
            for(int32 I=0;I<=30;++I){const float X=824+I*10.f;Line({{X,483},{X,483+(I%5==0?9.f:4.f)}},Gold.CopyWithNewOpacity(.42f));}
            if(Frame && Frame->GetResourceObject())
                FSlateDrawElement::MakeBox(List,Layer+2,G.ToPaintGeometry(),Frame,ESlateDrawEffect::None,Tint);
            else
                Line({{40,10},{1280,10},{1310,40},{1310,700},{1280,730},{40,730},{10,700},{10,40},{40,10}},FLinearColor(.31f,.36f,.39f),12.f);
            return Layer+2;
        }
    private:
        TAttribute<FLinearColor> Accent;
        const FSlateBrush* Frame=nullptr;
    };

    /** Paint focus directly; does not depend on an engine focus-texture's outline tint. */
    class SActionEdge final : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SActionEdge) {} SLATE_ARGUMENT(TWeakPtr<SButton>,Button) SLATE_END_ARGS()
        void Construct(const FArguments& Args){Button=Args._Button;SetVisibility(EVisibility::HitTestInvisible);ForceVolatile(true);}
        virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(300,72);}
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& List,int32 Layer,const FWidgetStyle& Style,bool) const override
        {
            const auto B=Button.Pin();
            const bool Active=B && B->IsEnabled() && (B->IsHovered() || B->HasAnyUserFocus().IsSet());
            const FVector2D S=G.GetLocalSize();
            TArray<FVector2D> P={{12,3},{S.X-12,3},{S.X-3,12},{S.X-3,S.Y-12},{S.X-12,S.Y-3},{12,S.Y-3},{3,S.Y-12},{3,12},{12,3}};
            FSlateDrawElement::MakeLines(List,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Ice.CopyWithNewOpacity(Active?1.f:.50f)*Style.GetColorAndOpacityTint(),true,Active?3.f:1.f);
            return Layer;
        }
    private:
        TWeakPtr<SButton> Button;
    };
}

UTDAbilityRewardWidget::UTDAbilityRewardWidget(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
    static ConstructorHelpers::FObjectFinderOptional<UTexture2D> Frame(TEXT("/Game/TriadDrift/UI/TriadHUD/IndustrialTerminal01/T_UI_IndustrialTerminal_Frame01.T_UI_IndustrialTerminal_Frame01"));
    TerminalFrameTexture=Frame.Get();
    TerminalFrameBrush.SetResourceObject(TerminalFrameTexture);
    TerminalFrameBrush.ImageSize=FVector2D(1320,740);
    TerminalFrameBrush.DrawAs=ESlateBrushDrawType::Image;
}

bool UTDAbilityRewardWidget::PresentReward(const FTDAbilityRewardDisplay& Display)
{
    if(Display.AbilityId.IsNone() || Display.AbilityName.IsEmpty() || Display.ShipName.IsEmpty()) return false;
    if(bActive && Data.AbilityId!=Display.AbilityId) return false; // owner queues, never lose an unacknowledged award
    if(bDismissed && Data.AbilityId==Display.AbilityId) return false;
    Data=Display;
    bActive=true;
    bDismissed=false;
    SetVisibility(ESlateVisibility::SelfHitTestInvisible); // only the actual button receives pointer input
    InvalidateLayoutAndVolatility();
    return true;
}

FText UTDAbilityRewardWidget::GetActivationKey() const
{ return bActive && Data.bCanUseNow?Data.ActivationKey:FText::GetEmpty(); }

FText UTDAbilityRewardWidget::GetAvailabilityText() const
{
    if(!bActive) return FText::GetEmpty();
    if(!Data.AvailabilityText.IsEmpty()) return Data.AvailabilityText;
    if(Data.bCanUseNow) return Data.AbilityName;
    return FText::Format(NSLOCTEXT("TDReward","Aboard","AVAILABLE ABOARD {0}"),Data.ShipName);
}

bool UTDAbilityRewardWidget::FocusDismissButton()
{
    if(!bActive || !DismissButton || !FSlateApplication::IsInitialized()) return false;
    FSlateApplication& App=FSlateApplication::Get();
    if(App.GetUserFocusedWidget(0)!=DismissButton) PreviousFocus=App.GetUserFocusedWidget(0);
    return App.SetUserFocus(0,DismissButton,EFocusCause::SetDirectly);
}

void UTDAbilityRewardWidget::RestoreFocusIfOwned()
{
    if(FSlateApplication::IsInitialized() && DismissButton && FSlateApplication::Get().GetUserFocusedWidget(0)==DismissButton)
    {
        const TSharedPtr<SWidget> Before=PreviousFocus.Pin();
        if(Before && Before->IsEnabled() && Before->SupportsKeyboardFocus())
            FSlateApplication::Get().SetUserFocus(0,Before,EFocusCause::SetDirectly);
        // Owner restores its gameplay/other-page focus if the previous widget was destroyed.
    }
    PreviousFocus.Reset();
}

void UTDAbilityRewardWidget::RequestDismiss()
{
    if(!bActive || bDismissed) return;
    const FName Id=Data.AbilityId;
    bActive=false;
    bDismissed=true;
    RestoreFocusIfOwned();
    SetVisibility(ESlateVisibility::Collapsed);
    ++DismissNotifications;
    OnRewardDismissed.Broadcast(Id); // presentation notification only; owner validates and acknowledges the award
}

void UTDAbilityRewardWidget::ResetPresentation()
{
    RestoreFocusIfOwned();
    bActive=false;
    bDismissed=false;
    Data=FTDAbilityRewardDisplay();
    SetVisibility(ESlateVisibility::Collapsed);
}

FReply UTDAbilityRewardWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(bActive && (Event.GetKey()==EKeys::Enter || Event.GetKey()==EKeys::Gamepad_FaceButton_Bottom))
    { if(!Event.IsRepeat()) RequestDismiss();return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry,Event);
}

TSharedRef<SWidget> UTDAbilityRewardWidget::RebuildWidget()
{
    using namespace TDIndustrialReward;
    SetIsFocusable(true);
    SetVisibility(bActive?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
    const TWeakObjectPtr<UTDAbilityRewardWidget> Weak(this);
    const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
    auto Accent=[Weak](){return Weak.IsValid()?Weak->Data.Accent:FLinearColor::White;};
    FSlateBrush Normal=*White,Hover=*White,Pressed=*White;
    Normal.TintColor=FLinearColor(.018f,.044f,.052f,.98f);
    Hover.TintColor=FLinearColor(.045f,.13f,.16f,.98f);
    Pressed.TintColor=FLinearColor(.025f,.08f,.10f,.98f);
    TerminalButtonStyle=FButtonStyle().SetNormal(Normal).SetHovered(Hover).SetPressed(Pressed).SetDisabled(Normal);
    SAssignNew(DismissButton,SButton).ButtonStyle(&TerminalButtonStyle)
        .IsEnabled_Lambda([Weak](){return Weak.IsValid() && Weak->bActive;})
        .OnClicked_Lambda([Weak](){if(Weak.IsValid()) Weak->RequestDismiss();return FReply::Handled();})
        .ContentPadding(FMargin(24,14)).HAlign(HAlign_Center)
        [SNew(STextBlock).Text(NSLOCTEXT("TDReward","Continue","CONTINUE"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",28)).ColorAndOpacity(Ice)];
    return SNew(SScaleBox).Stretch(EStretch::ScaleToFit).Visibility(EVisibility::SelfHitTestInvisible)
    [SNew(SBox).WidthOverride(1920).HeightOverride(1080).Visibility(EVisibility::SelfHitTestInvisible)
        [SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
            +SOverlay::Slot()
            [SNew(SBorder).Visibility(EVisibility::HitTestInvisible).Padding(0).BorderImage(White).BorderBackgroundColor(FLinearColor(.003f,.009f,.014f,.46f))]
            +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
            [SNew(SBox).WidthOverride(1320).HeightOverride(740).Visibility(EVisibility::SelfHitTestInvisible)
                [SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
                    +SOverlay::Slot()
                    [SNew(SSurface).Frame(&TerminalFrameBrush).Accent_Lambda(Accent)]
                    +SOverlay::Slot().Padding(108,108,550,218)
                    [SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible)
                        +SVerticalBox::Slot().AutoHeight()
                        [SNew(STextBlock).Text(NSLOCTEXT("TDReward","Unlocked","ABILITY UNLOCKED"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular",32)).ColorAndOpacity(Gold)]
                        +SVerticalBox::Slot().FillHeight(1).Padding(0,14,0,12)
                        [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Left).VAlign(VAlign_Center)
                            [SNew(SBox).WidthOverride(650)
                                [SNew(STextBlock).Text_Lambda([Weak](){return Weak.IsValid()?Weak->Data.AbilityName:FText::GetEmpty();})
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",76)).ColorAndOpacity(Ink)
                                    .ShadowOffset(FVector2D(0,3)).ShadowColorAndOpacity(FLinearColor(0,0,0,.6f)).WrapTextAt(650)]]]
                        +SVerticalBox::Slot().AutoHeight()
                        [SNew(STextBlock).Text_Lambda([Weak](){return Weak.IsValid()?Weak->Data.ShipName:FText::GetEmpty();})
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular",36)).ColorAndOpacity_Lambda(Accent)]
                    ]
                    +SOverlay::Slot().Padding(108,574,425,78)
                    [SNew(SHorizontalBox).Visibility(EVisibility::HitTestInvisible)
                        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,18,0)
                        [SNew(STextBlock).Text_Lambda([Weak](){return Weak.IsValid()?Weak->GetActivationKey():FText::GetEmpty();})
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold",28)).ColorAndOpacity_Lambda(Accent)]
                        +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                        [SNew(STextBlock).Text_Lambda([Weak](){return Weak.IsValid()?Weak->GetAvailabilityText():FText::GetEmpty();})
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular",26)).ColorAndOpacity(Ink).AutoWrapText(true)]]
                    +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,104,90)
                    [SNew(SBox).WidthOverride(300).HeightOverride(72)
                        [SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
                            +SOverlay::Slot()[DismissButton.ToSharedRef()]
                            +SOverlay::Slot()[SNew(SActionEdge).Button(TWeakPtr<SButton>(DismissButton))]]]
                ]
            ]
        ]
    ];
}

void UTDAbilityRewardWidget::NativeDestruct()
{ RestoreFocusIfOwned();Super::NativeDestruct(); }
void UTDAbilityRewardWidget::ReleaseSlateResources(bool bReleaseChildren)
{ RestoreFocusIfOwned();DismissButton.Reset();Super::ReleaseSlateResources(bReleaseChildren); }

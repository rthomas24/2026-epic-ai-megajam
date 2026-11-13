#include "TDMainMenuWidget.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

namespace TDMenu
{
const FLinearColor Cyan(.22f,.76f,.91f);
const FLinearColor Ink(.85f,.93f,.98f);
const FLinearColor Muted(.40f,.57f,.67f);

UFunction* Contract(UObject* Host,FName Action)
{
    UFunction* Fn=IsValid(Host)?Host->FindFunction(Action):nullptr;
    if(!Fn) return nullptr;
    for(TFieldIterator<FProperty> It(Fn);It;++It)
        if(It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm)) return nullptr;
    FProperty* Return=Fn->GetReturnProperty();
    return !Return || CastField<FBoolProperty>(Return)?Fn:nullptr;
}

/** Lightweight animated galaxy plate: no scene capture, lights, actors, materials or asset loads. */
class SOrbitalBackdrop : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SOrbitalBackdrop) {} SLATE_END_ARGS()
    void Construct(const FArguments&,UTDMainMenuWidget* InOwner)
    { Owner=InOwner; SetVisibility(EVisibility::HitTestInvisible); SetCanTick(true); }
    virtual FVector2D ComputeDesiredSize(float) const override { return {1920,1080}; }
    virtual void Tick(const FGeometry&,double,float Delta) override
    {
        if(Owner.IsValid() && !Owner->bReducedMenuMotion)
        { Time+=FMath::Min(Delta,.1f); Invalidate(EInvalidateWidgetReason::Paint); }
    }
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,
        FSlateWindowElementList& List,int32 Layer,const FWidgetStyle&,bool) const override
    {
        const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
        auto Box=[&](FVector2D P,FVector2D Size,FLinearColor C)
        { FSlateDrawElement::MakeBox(List,Layer,G.ToPaintGeometry(Size,FSlateLayoutTransform(P)),White,ESlateDrawEffect::None,C); };
        auto Line=[&](const TArray<FVector2D>& Points,FLinearColor C,float Width=1.f)
        { FSlateDrawElement::MakeLines(List,Layer+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,C,true,Width); };
        Box({0,0},{1920,1080},FLinearColor(.003f,.008f,.017f));
        // Broad, smoothly layered colour field; no flashing or full-screen distortion.
        for(int32 I=0;I<45;++I)
        {
            const float Y=I*24.f;
            const float Glow=FMath::Exp(-FMath::Square((Y-460.f)/320.f));
            Box({650,Y},{1270,25},FLinearColor(.008f+Glow*.012f,.017f+Glow*.026f,.04f+Glow*.055f));
        }
        for(int32 I=0;I<100;++I)
        {
            const float X=FMath::Fmod(I*137.508f+Time*(I%3+1)*.14f,1920.f);
            const float Y=FMath::Fmod(I*83.719f+31,1080.f);
            const float A=.12f+(I%7)*.055f;
            Box({X,Y},{I%9==0?2.f:1.f,I%9==0?2.f:1.f},FLinearColor(.5f,.75f,.9f,A));
        }
        // Orbital planet limb, lit on its upper edge. Content remains clear on the left.
        const FVector2D Center(1420,700);
        for(int32 I=0;I<36;++I)
        {
            const float Radius=420.f-I*11.f;
            TArray<FVector2D,TInlineAllocator<65>> Ring;
            for(int32 J=0;J<=64;++J)
            {
                const float Angle=2*PI*J/64.f;
                Ring.Add(Center+FVector2D(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius*.78f));
            }
            const float Alpha=I==0?.72f:I<3?.22f:.025f;
            Line(TArray<FVector2D>(Ring),FLinearColor(.11f,.51f,.69f,Alpha),I==0?2.f:1.f);
        }
        for(int32 I=0;I<3;++I)
        {
            TArray<FVector2D> Orbit; Orbit.Reserve(81);
            for(int32 J=0;J<=80;++J)
            {
                const float A=2*PI*J/80.f;
                Orbit.Add(Center+FVector2D(FMath::Cos(A)*(530+I*27),FMath::Sin(A)*(155+I*12)-FMath::Cos(A)*115));
            }
            const FLinearColor C=I==0?FLinearColor(.8f,.25f,.85f,.24f):I==1?Cyan.CopyWithNewOpacity(.28f):FLinearColor(.95f,.63f,.21f,.20f);
            Line(Orbit,C);
            const float Phase=Time*.025f+I*2.1f;
            const FVector2D P=Center+FVector2D(FMath::Cos(Phase)*(530+I*27),FMath::Sin(Phase)*(155+I*12)-FMath::Cos(Phase)*115);
            Box(P-FVector2D(3,3),{6,6},C.CopyWithNewOpacity(.9f));
        }
        // Static protective plate beneath the title and actions, with restrained squad accents.
        Box({0,0},{640,1080},FLinearColor(.003f,.008f,.017f,.92f));
        for(int32 I=0;I<24;++I)
            Box({640.f+I*9.f,0},{10,1080},FLinearColor(.003f,.008f,.017f,.92f*(1.f-I/24.f)));
        Box({100,166},{3,74},Cyan);
        Box({100,871},{170,2},FLinearColor(.82f,.24f,.85f));
        Box({279,871},{170,2},Cyan);
        Box({458,871},{170,2},FLinearColor(.94f,.64f,.25f));
        Line({{100,128},{1820,128}},Muted.CopyWithNewOpacity(.25f));
        Line({{100,964},{1820,964}},Muted.CopyWithNewOpacity(.25f));
        return Layer+1;
    }
private:
    TWeakObjectPtr<UTDMainMenuWidget> Owner;
    float Time=0;
};
}

bool UTDMainMenuWidget::InvokeHost(UObject* Host,FName Action,bool& Result)
{
    Result=false;
    UFunction* Fn=TDMenu::Contract(Host,Action);
    if(!Fn) return false;
    FStructOnScope Params(Fn);
    Host->ProcessEvent(Fn,Params.GetStructMemory());
    if(const FBoolProperty* Return=CastField<FBoolProperty>(Fn->GetReturnProperty()))
        Result=Return->GetPropertyValue_InContainer(Params.GetStructMemory());
    else Result=true;
    return true;
}
bool UTDMainMenuWidget::QueryHost(FName Action) const
{
    bool Result=false;
    InvokeHost(GetWorld()?GetWorld()->GetAuthGameMode():nullptr,Action,Result);
    return Result;
}
bool UTDMainMenuWidget::HasHostAction(FName Action) const
{ return TDMenu::Contract(GetWorld()?GetWorld()->GetAuthGameMode():nullptr,Action)!=nullptr; }

TSharedRef<SWidget> UTDMainMenuWidget::MakeAction(const FText& Label,FName Action,bool bEnabled)
{
    TSharedPtr<SButton> Button;
    TSharedRef<SWidget> Out=SNew(SBox).HeightOverride(68).Padding(FMargin(0,5))
    [ SAssignNew(Button,SButton).ButtonStyle(&ActionStyle).HAlign(HAlign_Left)
      .IsEnabled_Lambda([Weak=TWeakObjectPtr<UTDMainMenuWidget>(this),bEnabled]() { return Weak.IsValid() && bEnabled && !Weak->bTransitionPending; })
      .OnClicked_Lambda([Weak=TWeakObjectPtr<UTDMainMenuWidget>(this),Action]() { return Weak.IsValid()?Weak->Activate(Action):FReply::Unhandled(); })
      [ SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Bold",22)).ColorAndOpacity(TDMenu::Ink) ] ];
    if(bEnabled && !FirstButton) FirstButton=Button;
    return Out;
}
TSharedRef<SWidget> UTDMainMenuWidget::BuildActions()
{
    FirstButton.Reset();
    TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
    if(bSettingsOpen)
    {
        Box->AddSlot().AutoHeight()[SNew(STextBlock).Text(NSLOCTEXT("TDMenu","Access","ACCESSIBILITY"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",22)).ColorAndOpacity(TDMenu::Cyan)];
        Box->AddSlot().AutoHeight().Padding(0,12)[SNew(STextBlock).Text(NSLOCTEXT("TDMenu","MotionNote","Decorative menu motion only.\nGameplay camera settings remain unchanged."))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity(TDMenu::Muted)];
        Box->AddSlot().AutoHeight()[MakeAction(bReducedMenuMotion?NSLOCTEXT("TDMenu","MotionOff","MENU MOTION   /   REDUCED"):NSLOCTEXT("TDMenu","MotionOn","MENU MOTION   /   FULL"),TEXT("ToggleMotion"))];
        Box->AddSlot().AutoHeight()[MakeAction(NSLOCTEXT("TDMenu","Back","BACK"),TEXT("Back"))];
    }
    else
    {
        Box->AddSlot().AutoHeight()[MakeAction(NSLOCTEXT("TDMenu","New","NEW GAME"),TEXT("NewGame"),HasHostAction(TEXT("NewGame")))];
        Box->AddSlot().AutoHeight()[MakeAction(NSLOCTEXT("TDMenu","Continue","CONTINUE"),TEXT("ContinueGame"),HasHostAction(TEXT("ContinueGame")) && QueryHost(TEXT("CanContinue")))];
        Box->AddSlot().AutoHeight()[MakeAction(NSLOCTEXT("TDMenu","Settings","SETTINGS"),TEXT("Settings"))];
        Box->AddSlot().AutoHeight()[MakeAction(NSLOCTEXT("TDMenu","Quit","QUIT"),TEXT("QuitGame"),HasHostAction(TEXT("QuitGame")))];
    }
    return Box;
}
TSharedRef<SWidget> UTDMainMenuWidget::RebuildWidget()
{
    ActionStyle=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
    FSlateBrush Normal=*FCoreStyle::Get().GetBrush("WhiteBrush"); Normal.TintColor=FLinearColor(.025f,.065f,.09f,.92f);
    FSlateBrush Hover=Normal; Hover.TintColor=FLinearColor(.055f,.19f,.25f);
    FSlateBrush Press=Normal; Press.TintColor=FLinearColor(.09f,.29f,.36f);
    ActionStyle.SetNormal(Normal).SetHovered(Hover).SetPressed(Press).SetDisabled(Normal)
        .SetNormalPadding(FMargin(22,14)).SetPressedPadding(FMargin(24,14));
    Backdrop=SNew(TDMenu::SOrbitalBackdrop,this);
    TSharedRef<SVerticalBox> Left=SNew(SVerticalBox);
    Left->AddSlot().AutoHeight()[SNew(STextBlock).Text(NSLOCTEXT("TDMenu","Edition","DIRECTOR'S CUT  /  SQUADRON 03"))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular",17)).ColorAndOpacity(TDMenu::Cyan)];
    Left->AddSlot().AutoHeight().Padding(0,18,0,8)[SNew(STextBlock).Text(NSLOCTEXT("TDMenu","Title","TRIAD\nDRIFT"))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold",82)).ColorAndOpacity(TDMenu::Ink)];
    Left->AddSlot().AutoHeight().Padding(0,4,0,34)[SNew(STextBlock).Text(NSLOCTEXT("TDMenu","Motto","THREE SHIPS. ONE SQUADRON. YOUR CUT."))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity(TDMenu::Muted)];
    Left->AddSlot().AutoHeight()[SAssignNew(ActionsBox,SBox).WidthOverride(520)[BuildActions()]];
    Left->AddSlot().AutoHeight().Padding(0,16)[SAssignNew(StatusLabel,STextBlock).Text(FText::GetEmpty()).AutoWrapText(true)
        .Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).ColorAndOpacity(TDMenu::Cyan)];
    return SNew(SOverlay)
      +SOverlay::Slot()[SNew(SBox).HAlign(HAlign_Fill).VAlign(VAlign_Fill)[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Backdrop.ToSharedRef()]]]]
      +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)
        [SNew(SOverlay)
          +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(120,180,0,0)[SNew(SBox).WidthOverride(520)[Left]]
          +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(100,0,0,66)[SNew(STextBlock).Text(NSLOCTEXT("TDMenu","Nav","MOUSE / ARROWS / D-PAD    SELECT: ENTER / A"))
              .Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(TDMenu::Muted)]
          +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,78,100,0)[SNew(STextBlock).Text(NSLOCTEXT("TDMenu","Squad","RIFT   /   LANCER   /   BASTION"))
              .Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).ColorAndOpacity(TDMenu::Muted)]
        ]]];
}
void UTDMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    bTransitionPending=false;
    if(FirstButton && FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocus(FirstButton,EFocusCause::SetDirectly);
}
void UTDMainMenuWidget::NativeTick(const FGeometry& G,float DeltaSeconds)
{
    Super::NativeTick(G,DeltaSeconds);
    if(!FirstButton || bTransitionPending || !FSlateApplication::IsInitialized()) return;
    FSlateApplication& App=FSlateApplication::Get();
    const TSharedPtr<SWidget> Focus=App.GetUserFocusedWidget(0);
    // Construction happens before viewport attachment. Restore only absent/viewport focus
    // once mounted (and after a resize); never steal it from another menu control.
    if(!Focus || Focus->GetType()==FName(TEXT("SViewport")))
        App.SetAllUserFocus(FirstButton,EFocusCause::SetDirectly);
}
void UTDMainMenuWidget::Status(const FText& Message) { if(StatusLabel) StatusLabel->SetText(Message); }
void UTDMainMenuWidget::ShowSettings(bool bShow)
{
    bSettingsOpen=bShow;
    if(ActionsBox) ActionsBox->SetContent(BuildActions());
    if(FirstButton && FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocus(FirstButton,EFocusCause::SetDirectly);
}
void UTDMainMenuWidget::SetReducedMenuMotion(bool bReduced)
{ bReducedMenuMotion=bReduced; if(Backdrop) Backdrop->Invalidate(EInvalidateWidgetReason::Paint); }
FReply UTDMainMenuWidget::Activate(FName Action)
{
    if(bTransitionPending) return FReply::Handled();
    if(Action==TEXT("Settings")) { ShowSettings(true); return FReply::Handled(); }
    if(Action==TEXT("Back")) { ShowSettings(false); return FReply::Handled(); }
    if(Action==TEXT("ToggleMotion")) { SetReducedMenuMotion(!bReducedMenuMotion); ShowSettings(true); return FReply::Handled(); }
    bool Accepted=false;
    bTransitionPending=true;
    Status(NSLOCTEXT("TDMenu","Opening","OPENING FLIGHT SESSION..."));
    const bool Called=InvokeHost(GetWorld()?GetWorld()->GetAuthGameMode():nullptr,Action,Accepted);
    if(!Called || !Accepted)
    {
        bTransitionPending=false;
        Status(NSLOCTEXT("TDMenu","Unavailable","Action unavailable. Select another option or try again."));
    }
    return FReply::Handled();
}
FReply UTDMainMenuWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    if(bSettingsOpen && (E.GetKey()==EKeys::Escape || E.GetKey()==EKeys::Gamepad_FaceButton_Right))
    { ShowSettings(false); return FReply::Handled(); }
    return Super::NativeOnKeyDown(G,E);
}
void UTDMainMenuWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    FirstButton.Reset(); StatusLabel.Reset(); ActionsBox.Reset(); Backdrop.Reset();
}
